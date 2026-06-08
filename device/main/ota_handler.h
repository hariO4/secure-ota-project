#ifndef OTA_HANDLER_H
#define OTA_HANDLER_H

#include <stdint.h>
#include <stddef.h>
#include "esp_err.h"

esp_err_t perform_ota_update(const uint8_t *fw_data, size_t fw_len,
                              const uint8_t *sig_data, size_t sig_len);

#endif