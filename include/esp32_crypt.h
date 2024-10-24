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
    ESP_LOG_BUFFER_HEX("cbc_encrypt", encrypt_output, padded_input_len);
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