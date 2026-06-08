#include "esp_log.h"
#include "esp_event.h"
#include "string.h"
#include "stdlib.h"
#include "mqtt_client.h"
#include "my_mqtt_client.h"
#include "esp_ota_ops.h"
#include "esp_partition.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "mbedtls/md.h"
#include "mbedtls/pk.h"
#include "mbedtls/error.h"
#include "public_key_data.h"

static const char *TAG = "MQTT";

extern const uint8_t hivemq_ca_pem_start[] asm("_binary_hivemq_ca_pem_start");
extern const uint8_t hivemq_ca_pem_end[]   asm("_binary_hivemq_ca_pem_end");

#define MAX_SIG_SIZE 512
#define CHUNK_SIZE   4096

// Signature buffer
static uint8_t sig_buffer[MAX_SIG_SIZE];
static size_t  sig_len      = 0;
static bool    sig_received = false;

// OTA state
static esp_ota_handle_t      ota_handle       = 0;
static const esp_partition_t *ota_partition   = NULL;
static bool   ota_started      = false;
static size_t fw_total_len     = 0;
static size_t fw_written_total = 0;
static bool   fw_done          = false;

// Topic tracking
static bool receiving_firmware  = false;
static bool receiving_signature = false;

// ── Reset all OTA state ───────────────────────────────────────────────────────
static void reset_ota_state(void)
{
    if (ota_started && ota_handle) {
        esp_ota_abort(ota_handle);
    }
    ota_handle          = 0;
    ota_partition       = NULL;
    ota_started         = false;
    fw_total_len        = 0;
    fw_written_total    = 0;
    fw_done             = false;
    sig_received        = false;
    sig_len             = 0;
    receiving_firmware  = false;
    receiving_signature = false;
}

// ── Verify signature streaming from flash ─────────────────────────────────────
static bool verify_from_flash(void)
{
    mbedtls_md_context_t md_ctx;
    mbedtls_pk_context   pk_ctx;
    uint8_t hash[32];
    char    err_buf[100];
    bool    result = false;

    mbedtls_md_init(&md_ctx);
    mbedtls_pk_init(&pk_ctx);

    // Setup SHA256
    const mbedtls_md_info_t *md_info =
        mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
    if (md_info == NULL) {
        ESP_LOGE(TAG, "SHA256 not available");
        goto cleanup;
    }

    mbedtls_md_setup(&md_ctx, md_info, 0);
    mbedtls_md_starts(&md_ctx);

    // Allocate small chunk buffer
    uint8_t *chunk = (uint8_t*)malloc(CHUNK_SIZE);
    if (chunk == NULL) {
        ESP_LOGE(TAG, "Failed to allocate chunk buffer!");
        goto cleanup;
    }

    // Read flash in 4KB chunks — feed into SHA256
    size_t offset  = 0;
    bool   read_ok = true;

    while (offset < fw_total_len) {
        size_t to_read = fw_total_len - offset;
        if (to_read > CHUNK_SIZE) to_read = CHUNK_SIZE;

        esp_err_t err = esp_partition_read(
            ota_partition, offset, chunk, to_read
        );
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Flash read failed at offset %d: %s",
                     (int)offset, esp_err_to_name(err));
            read_ok = false;
            break;
        }

        mbedtls_md_update(&md_ctx, chunk, to_read);
        offset += to_read;

        // Progress every 64KB
        if (offset % (64 * 1024) == 0 || offset == fw_total_len) {
            ESP_LOGI(TAG, "Hashing: %d / %d bytes",
                     (int)offset, (int)fw_total_len);
        }
    }

    free(chunk);

    if (!read_ok) {
        goto cleanup;
    }

    // Finalize hash
    mbedtls_md_finish(&md_ctx, hash);
    mbedtls_md_free(&md_ctx);
    mbedtls_md_init(&md_ctx); // reinit so cleanup is safe

    ESP_LOGI(TAG, "SHA256: %02x%02x%02x%02x%02x%02x%02x%02x...",
             hash[0], hash[1], hash[2], hash[3],
             hash[4], hash[5], hash[6], hash[7]);

    // Parse public key
    int ret = mbedtls_pk_parse_public_key(
        &pk_ctx, public_key_der, public_key_der_len
    );
    if (ret != 0) {
        mbedtls_strerror(ret, err_buf, sizeof(err_buf));
        ESP_LOGE(TAG, "Public key parse failed: %s", err_buf);
        goto cleanup;
    }

    // Verify signature
    ret = mbedtls_pk_verify(
        &pk_ctx,
        MBEDTLS_MD_SHA256,
        hash, sizeof(hash),
        sig_buffer, sig_len
    );

    if (ret == 0) {
        ESP_LOGI(TAG, "✅ SIGNATURE VALID");
        result = true;
    } else {
        mbedtls_strerror(ret, err_buf, sizeof(err_buf));
        ESP_LOGE(TAG, "❌ SIGNATURE INVALID: %s", err_buf);
        result = false;
    }

cleanup:
    mbedtls_md_free(&md_ctx);
    mbedtls_pk_free(&pk_ctx);
    return result;
}

// ── MQTT Event Handler ────────────────────────────────────────────────────────
static void mqtt_event_handler(void *handler_args, esp_event_base_t base,
                                int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t  event  = event_data;
    esp_mqtt_client_handle_t client = event->client;

    switch ((esp_mqtt_event_id_t)event_id) {

    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT Connected ✅");
        esp_mqtt_client_subscribe(client, "ota/firmware/binary",    0);
        esp_mqtt_client_subscribe(client, "ota/firmware/signature", 0);
        ESP_LOGI(TAG, "Subscribed to OTA topics");
        break;

    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT Disconnected ❌");
        reset_ota_state();
        break;

    case MQTT_EVENT_DATA:
    {
        esp_err_t err;

        // Identify topic on first chunk
        if (event->topic != NULL && event->topic_len > 0) {
            if (strncmp(event->topic, "ota/firmware/binary",
                        event->topic_len) == 0) {
                receiving_firmware  = true;
                receiving_signature = false;
            } else if (strncmp(event->topic, "ota/firmware/signature",
                               event->topic_len) == 0) {
                receiving_firmware  = false;
                receiving_signature = true;
            } else {
                break; // Unknown topic
            }
        }

        // ── Firmware chunks → write directly to flash ─────────────────
        if (receiving_firmware) {

            // First chunk — begin OTA
            if (event->current_data_offset == 0) {
                fw_total_len     = event->total_data_len;
                fw_written_total = 0;
                fw_done          = false;

                ESP_LOGI(TAG, "Firmware total: %d bytes", (int)fw_total_len);

                ota_partition = esp_ota_get_next_update_partition(NULL);
                if (ota_partition == NULL) {
                    ESP_LOGE(TAG, "No OTA partition found!");
                    reset_ota_state();
                    break;
                }
                ESP_LOGI(TAG, "Writing to: %s", ota_partition->label);

                err = esp_ota_begin(ota_partition, fw_total_len, &ota_handle);
                if (err != ESP_OK) {
                    ESP_LOGE(TAG, "OTA begin failed: %s", esp_err_to_name(err));
                    reset_ota_state();
                    break;
                }

                ota_started = true;
                ESP_LOGI(TAG, "OTA begin OK ✅");
            }

            if (!ota_started) {
                ESP_LOGE(TAG, "OTA not started — dropping chunk");
                break;
            }

            // Write chunk to flash
            err = esp_ota_write(ota_handle, event->data, event->data_len);
            if (err != ESP_OK) {
                ESP_LOGE(TAG, "OTA write failed: %s", esp_err_to_name(err));
                reset_ota_state();
                break;
            }

            fw_written_total += event->data_len;
            ESP_LOGI(TAG, "Written: %d / %d bytes",
                     (int)fw_written_total, (int)fw_total_len);

            // All chunks written
            if (fw_written_total >= fw_total_len) {
                ESP_LOGI(TAG, "✅ All firmware written to flash");
                fw_done            = true;
                receiving_firmware = false;
            }
        }

        // ── Signature ─────────────────────────────────────────────────
        else if (receiving_signature) {

            size_t copy_offset = (event->current_data_offset == 0)
                                  ? 0 : sig_len;
            size_t copy_len    = event->data_len;

            if (copy_offset + copy_len > MAX_SIG_SIZE) {
                ESP_LOGE(TAG, "Signature too large!");
                reset_ota_state();
                break;
            }

            memcpy(sig_buffer + copy_offset, event->data, copy_len);
            sig_len = copy_offset + copy_len;

            // Check if signature fully received
            if ((size_t)(event->current_data_offset + event->data_len)
                    >= (size_t)event->total_data_len) {
                sig_received        = true;
                receiving_signature = false;
                ESP_LOGI(TAG, "✅ Signature received: %d bytes", (int)sig_len);
            }
        }

        // ── Both ready — verify then finalize OTA ─────────────────────
        if (fw_done && sig_received) {
            ESP_LOGI(TAG, "Both received! Verifying from flash...");

            bool valid = verify_from_flash();

            if (!valid) {
                ESP_LOGE(TAG, "❌ Signature INVALID — OTA REJECTED");
                reset_ota_state();
                break;
            }

            ESP_LOGI(TAG, "✅ Signature VALID — Finalizing OTA...");

            // End OTA
            err = esp_ota_end(ota_handle);
            if (err != ESP_OK) {
                ESP_LOGE(TAG, "OTA end failed: %s", esp_err_to_name(err));
                reset_ota_state();
                break;
            }
            ESP_LOGI(TAG, "OTA end OK");

            // Set boot partition
            err = esp_ota_set_boot_partition(ota_partition);
            if (err != ESP_OK) {
                ESP_LOGE(TAG, "Set boot partition failed: %s",
                         esp_err_to_name(err));
                reset_ota_state();
                break;
            }
            ESP_LOGI(TAG, "Boot partition set ✅");

            ESP_LOGI(TAG, "🔄 Rebooting in 3 seconds...");
            vTaskDelay(pdMS_TO_TICKS(3000));
            esp_restart();
        }
        break;
    }

    case MQTT_EVENT_ERROR:
        ESP_LOGE(TAG, "MQTT Error!");
        reset_ota_state();
        break;

    default:
        ESP_LOGI(TAG, "Other event: %d", event->event_id);
        break;
    }
}

// ── Start MQTT client ─────────────────────────────────────────────────────────
void mqtt_app_start(void)
{
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = "mqtts://220b2344066a4e288a4babcc5d307788.s1.eu.hivemq.cloud:8883",
        .broker.verification.certificate = (const char *)hivemq_ca_pem_start,
        .credentials.username            = "espuser",
        .credentials.authentication.password = "Espmodule@32",
        .buffer.size     = 8192,
        .buffer.out_size = 1024,
    };

    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID,
                                   mqtt_event_handler, NULL);
    esp_mqtt_client_start(client);
}