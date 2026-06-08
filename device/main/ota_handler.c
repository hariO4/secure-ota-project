#include "ota_handler.h"
#include "crypto_verify.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_partition.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "OTA";

esp_err_t perform_ota_update(const uint8_t *fw_data, size_t fw_len,
                              const uint8_t *sig_data, size_t sig_len)
{
    esp_err_t err;
    esp_ota_handle_t update_handle = 0;
    const esp_partition_t *update_partition = NULL;

    // Step 1 - Verify signature FIRST before touching flash
    ESP_LOGI(TAG, "Verifying firmware signature...");
    bool valid = verify_firmware_signature(fw_data, fw_len, sig_data, sig_len);
    if (!valid) {
        ESP_LOGE(TAG, "❌ Signature invalid — OTA update REJECTED");
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "✅ Signature valid — proceeding with OTA");

    // Step 2 - Get next OTA partition
    update_partition = esp_ota_get_next_update_partition(NULL);
    if (update_partition == NULL) {
        ESP_LOGE(TAG, "No OTA partition found!");
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "Writing to partition: %s at offset 0x%x",
             update_partition->label, (unsigned int)update_partition->address);

    // Step 3 - Begin OTA
    err = esp_ota_begin(update_partition, fw_len, &update_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_begin failed: %s", esp_err_to_name(err));
        return err;
    }
    ESP_LOGI(TAG, "OTA begin OK");

    // Step 4 - Write firmware data
    err = esp_ota_write(update_handle, fw_data, fw_len);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_write failed: %s", esp_err_to_name(err));
        esp_ota_abort(update_handle);
        return err;
    }
    ESP_LOGI(TAG, "OTA write OK — %d bytes written", (int)fw_len);

    // Step 5 - End OTA
    err = esp_ota_end(update_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_end failed: %s", esp_err_to_name(err));
        return err;
    }
    ESP_LOGI(TAG, "OTA end OK");

    // Step 6 - Set boot partition
    err = esp_ota_set_boot_partition(update_partition);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_set_boot_partition failed: %s", esp_err_to_name(err));
        return err;
    }
    ESP_LOGI(TAG, "Boot partition set to: %s", update_partition->label);

    // Step 7 - Reboot
    ESP_LOGI(TAG, "🔄 Rebooting into new firmware in 3 seconds...");
    vTaskDelay(pdMS_TO_TICKS(3000));
    esp_restart();

    return ESP_OK;
}