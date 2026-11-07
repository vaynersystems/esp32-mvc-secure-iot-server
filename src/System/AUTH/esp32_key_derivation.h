#ifndef _ESP32_KEY_DERIVATION_H
#define _ESP32_KEY_DERIVATION_H
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "mbedtls/md.h"

#define DERIVED_KEY_SIZE 32

// Purpose labels. Each purpose gets an independent key; changing a label invalidates
// everything produced with the old key (tokens, backups).
#define KEY_LABEL_JWT        "esp32-mvc/jwt-hs256/v1"
#define KEY_LABEL_BACKUP_ENC "esp32-mvc/backup-aes256/v1"
#define KEY_LABEL_BACKUP_MAC "esp32-mvc/backup-hmac/v1"

/// @brief Derive a 32-byte key for one purpose from the device's secret (the full private key DER).
/// @brief HMAC-SHA256(secret, label). The whole key is used, so the result is as secret as the private key.
/// @return true on success
static inline bool derive_device_key(const uint8_t* secret, size_t secretLength, const char* label, uint8_t out[DERIVED_KEY_SIZE]){
    if(secret == nullptr || secretLength < 64 || label == nullptr) return false;
    return mbedtls_md_hmac(mbedtls_md_info_from_type(MBEDTLS_MD_SHA256),
        secret, secretLength,
        (const unsigned char*)label, strlen(label),
        out) == 0;
}

static inline bool constant_time_equals(const uint8_t* a, const uint8_t* b, size_t length){
    uint8_t diff = 0;
    for(size_t i = 0; i < length; i++) diff |= a[i] ^ b[i];
    return diff == 0;
}

#endif
