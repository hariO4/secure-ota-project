#include <stdio.h>
#include <string.h>
#include "nvs_flash.h"
#include "wifi_manager.h"
#include "my_mqtt_client.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_app_desc.h"

static const char *TAG = "MAIN";

void app_main(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
        ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    const esp_partition_t *running = esp_ota_get_running_partition();
    ESP_LOGI(TAG, "Running partition: %s", running->label);

    const esp_app_desc_t *app_desc = esp_app_get_description();
    ESP_LOGI(TAG, "Firmware version: %s", app_desc->version);

    ESP_LOGI(TAG, "Starting WiFi and MQTT...");

    wifi_init_sta();
    wifi_wait_connected();

    esp_ota_mark_app_valid_cancel_rollback();
    ESP_LOGI(TAG, "✅ App marked valid — rollback cancelled");

    ESP_LOGI(TAG, "Waiting for OTA update from server...");
    mqtt_app_start();
}