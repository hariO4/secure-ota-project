#include "crypto_verify.h"
#include "esp_log.h"
#include "mbedtls/pk.h"
#include "mbedtls/md.h"
#include "mbedtls/error.h"
#include "public_key_data.h"

static const char *TAG = "CRYPTO";

bool verify_firmware_signature(const uint8_t *fw_data, size_t fw_len,
                                const uint8_t *sig_data, size_t sig_len)
{
    mbedtls_pk_context pk;
    mbedtls_md_context_t md_ctx;
    const mbedtls_md_info_t *md_info;
    uint8_t hash[32];
    int ret;
    char error_buf[100];

    // Step 1 - Initialize
    mbedtls_pk_init(&pk);
    mbedtls_md_init(&md_ctx);

    ESP_LOGI(TAG, "Starting signature verification...");
    ESP_LOGI(TAG, "Firmware size: %d bytes", (int)fw_len);
    ESP_LOGI(TAG, "Signature size: %d bytes", (int)sig_len);

    // Step 2 - Parse public key
    ret = mbedtls_pk_parse_public_key(
        &pk,
        public_key_der,
        public_key_der_len
    );
    if (ret != 0) {
        mbedtls_strerror(ret, error_buf, sizeof(error_buf));
        ESP_LOGE(TAG, "Failed to parse public key: %s", error_buf);
        mbedtls_pk_free(&pk);
        mbedtls_md_free(&md_ctx);
        return false;
    }
    ESP_LOGI(TAG, "Public key parsed OK");

    // Step 3 - Compute SHA256 using mbedtls MD API
    md_info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
    if (md_info == NULL) {
        ESP_LOGE(TAG, "SHA256 not available");
        mbedtls_pk_free(&pk);
        mbedtls_md_free(&md_ctx);
        return false;
    }

    ret = mbedtls_md_setup(&md_ctx, md_info, 0);
    if (ret != 0) {
        mbedtls_strerror(ret, error_buf, sizeof(error_buf));
        ESP_LOGE(TAG, "MD setup failed: %s", error_buf);
        mbedtls_pk_free(&pk);
        mbedtls_md_free(&md_ctx);
        return false;
    }

    mbedtls_md_starts(&md_ctx);
    mbedtls_md_update(&md_ctx, fw_data, fw_len);
    mbedtls_md_finish(&md_ctx, hash);
    mbedtls_md_free(&md_ctx);

    ESP_LOGI(TAG, "SHA256 computed OK");
    ESP_LOGI(TAG, "Hash: %02x%02x%02x%02x%02x%02x%02x%02x...",
             hash[0], hash[1], hash[2], hash[3],
             hash[4], hash[5], hash[6], hash[7]);

    // Step 4 - Verify signature
    ret = mbedtls_pk_verify(
        &pk,
        MBEDTLS_MD_SHA256,
        hash,
        sizeof(hash),
        sig_data,
        sig_len
    );

    // Step 5 - Cleanup
    mbedtls_pk_free(&pk);

    // Step 6 - Result
    if (ret == 0) {
        ESP_LOGI(TAG, "✅ SIGNATURE VALID");
        return true;
    } else {
        mbedtls_strerror(ret, error_buf, sizeof(error_buf));
        ESP_LOGE(TAG, "❌ SIGNATURE INVALID: %s", error_buf);
        return false;
    }
}