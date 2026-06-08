#ifndef CRYPTO_VERIFY_H
#define CRYPTO_VERIFY_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

bool verify_firmware_signature(const uint8_t *fw_data, size_t fw_len,
                                const uint8_t *sig_data, size_t sig_len);

#endif