#include <string>
#include <string.h>
#include <mbedtls/aes.h>
#include <esp_log.h>
#include "Arduino.h"
using namespace std;

size_t ecrypted_string_length(const char * input){
    int padded_input_len = 0;
    int input_len = strlen(input) + 1;
    int modulo16 = input_len % 16;
    if (input_len < 16)
        padded_input_len = 16;
    else
        padded_input_len = (strlen(input) / 16 + 1) * 16;

    return padded_input_len;
}
size_t encrypt_string(const char *input, uint8_t *key, uint8_t *iv, unsigned char *encrypt_output) {
    int padded_input_len = 0;
    int input_len = strlen(input) + 1;
    int modulo16 = input_len % 16;
    if (input_len < 16)
        padded_input_len = 16;
    else
        padded_input_len = (strlen(input) / 16 + 1) * 16;
    char *padded_input = (char *)malloc(padded_input_len);
    
    if (!padded_input) {
        printf("Failed to allocate memory\n");
        return 0;
    }

    memcpy(padded_input, input, strlen(input));
    uint8_t pkc5_value = (17 - modulo16);
    for (int i = strlen(input); i < padded_input_len; i++) {
        padded_input[i] = pkc5_value;
    }
    
    mbedtls_aes_context aes;
    mbedtls_aes_init(&aes);
    mbedtls_aes_setkey_enc(&aes, key, 256);
    mbedtls_aes_crypt_cbc(&aes, MBEDTLS_AES_ENCRYPT, padded_input_len, iv,
    (unsigned char *)padded_input, encrypt_output);
    // (removed hex dump of ciphertext to the log)
    //output = string((const char*)encrypt_output);
    return padded_input_len;
}

size_t decrypt_string(const unsigned char *input, size_t input_len, const char *key, const char *iv, unsigned char* decrypted_data)
{
    if (input_len % 16 == 1)
    {
        return -1;
    }
    uint8_t key_copy[32] = {0};
    uint8_t iv_copy[16] = {0};

    memcpy(key_copy, key, 32);
    memcpy(iv_copy, iv, 16);
    

    mbedtls_aes_context aes;
    mbedtls_aes_init(&aes);
    if(mbedtls_aes_setkey_dec(&aes, key_copy, 256) != 0){
        Serial.printf("Error setting key, AES invalid length\n");
    }    
    Serial.printf("startMBED TLS AES DECRYPT with %d bytes of data\n", input_len);
    auto decryptResult = 
        mbedtls_aes_crypt_cbc(&aes, MBEDTLS_AES_DECRYPT, input_len, iv_copy, input, decrypted_data);
    if(decryptResult != 0){
        Serial.printf("Error decrypting string \n");
    }
    //esp_aes_free(&aes);

    return input_len;
}
// ---------------------------------------------------------------------------
// Backup container v2: authenticated encryption (AES-256-CBC + HMAC-SHA256, encrypt-then-MAC)
//   magic[8] | iv[16] | ciphertext (PKCS#7 padded) | tag[32]
//   tag = HMAC-SHA256(macKey, magic | iv | ciphertext)
// Keys come from derive_device_key() with KEY_LABEL_BACKUP_ENC / KEY_LABEL_BACKUP_MAC.
// ---------------------------------------------------------------------------
#include <mbedtls/md.h>
#include <esp_system.h>
#include "System/AUTH/esp32_key_derivation.h"

#define BACKUP_MAGIC "ESPBAK02"
#define BACKUP_MAGIC_SIZE 8
#define BACKUP_IV_SIZE 16
#define BACKUP_TAG_SIZE 32
#define BACKUP_OVERHEAD (BACKUP_MAGIC_SIZE + BACKUP_IV_SIZE + BACKUP_TAG_SIZE)

static inline size_t backup_sealed_length(size_t plainLength){
    return BACKUP_OVERHEAD + (plainLength / 16 + 1) * 16;
}

static inline bool backup_is_v2(const uint8_t* data, size_t length){
    return length >= BACKUP_OVERHEAD && memcmp(data, BACKUP_MAGIC, BACKUP_MAGIC_SIZE) == 0;
}

/// @brief Encrypt and authenticate a backup. output must hold backup_sealed_length(plainLength) bytes.
static bool backup_seal(const uint8_t* plain, size_t plainLength,
                        const uint8_t encKey[DERIVED_KEY_SIZE], const uint8_t macKey[DERIVED_KEY_SIZE],
                        uint8_t* output, size_t* outputLength){
    size_t paddedLength = (plainLength / 16 + 1) * 16;
    uint8_t* padded = (uint8_t*)malloc(paddedLength);
    if(padded == nullptr) return false;
    memcpy(padded, plain, plainLength);
    uint8_t pad = (uint8_t)(paddedLength - plainLength);
    memset(padded + plainLength, pad, pad);

    uint8_t* iv = output + BACKUP_MAGIC_SIZE;
    uint8_t* cipher = iv + BACKUP_IV_SIZE;
    memcpy(output, BACKUP_MAGIC, BACKUP_MAGIC_SIZE);
    esp_fill_random(iv, BACKUP_IV_SIZE);

    uint8_t ivWork[BACKUP_IV_SIZE];
    memcpy(ivWork, iv, BACKUP_IV_SIZE); // CBC mutates the IV buffer
    mbedtls_aes_context aes;
    mbedtls_aes_init(&aes);
    bool ok = mbedtls_aes_setkey_enc(&aes, encKey, 256) == 0 &&
              mbedtls_aes_crypt_cbc(&aes, MBEDTLS_AES_ENCRYPT, paddedLength, ivWork, padded, cipher) == 0;
    mbedtls_aes_free(&aes);
    memset(padded, 0, paddedLength);
    free(padded);
    if(!ok) return false;

    size_t macInputLength = BACKUP_MAGIC_SIZE + BACKUP_IV_SIZE + paddedLength;
    if(mbedtls_md_hmac(mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), macKey, DERIVED_KEY_SIZE,
                       output, macInputLength, output + macInputLength) != 0)
        return false;
    *outputLength = macInputLength + BACKUP_TAG_SIZE;
    return true;
}

/// @brief Verify and decrypt a v2 backup. output must hold at least length bytes; it is NUL terminated.
/// @return false if the backup was not made by this device (wrong key) or was modified.
static bool backup_open(const uint8_t* data, size_t length,
                        const uint8_t encKey[DERIVED_KEY_SIZE], const uint8_t macKey[DERIVED_KEY_SIZE],
                        uint8_t* output, size_t* outputLength){
    if(!backup_is_v2(data, length)) return false;
    size_t cipherLength = length - BACKUP_OVERHEAD;
    if(cipherLength == 0 || cipherLength % 16 != 0) return false;

    size_t macInputLength = length - BACKUP_TAG_SIZE;
    uint8_t tag[BACKUP_TAG_SIZE];
    if(mbedtls_md_hmac(mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), macKey, DERIVED_KEY_SIZE,
                       data, macInputLength, tag) != 0)
        return false;
    if(!constant_time_equals(tag, data + macInputLength, BACKUP_TAG_SIZE)) return false;

    uint8_t ivWork[BACKUP_IV_SIZE];
    memcpy(ivWork, data + BACKUP_MAGIC_SIZE, BACKUP_IV_SIZE);
    mbedtls_aes_context aes;
    mbedtls_aes_init(&aes);
    bool ok = mbedtls_aes_setkey_dec(&aes, encKey, 256) == 0 &&
              mbedtls_aes_crypt_cbc(&aes, MBEDTLS_AES_DECRYPT, cipherLength, ivWork,
                                    data + BACKUP_MAGIC_SIZE + BACKUP_IV_SIZE, output) == 0;
    mbedtls_aes_free(&aes);
    if(!ok) return false;

    uint8_t pad = output[cipherLength - 1];
    if(pad == 0 || pad > 16) return false;
    for(size_t i = cipherLength - pad; i < cipherLength; i++)
        if(output[i] != pad) return false;
    *outputLength = cipherLength - pad;
    output[*outputLength] = 0;
    return true;
}
