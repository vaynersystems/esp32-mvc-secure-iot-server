#include "esp32_cert_base.hpp"
#include "esp32_cert_nvs.hpp"



bool esp32_cert_nvs::hasPrivateKey()
{
    nvs_handle_t storage_handle;
    esp_err_t err = nvs_open(NVS_VOL, NVS_READONLY, &storage_handle);
    auto it = nvs_entry_find(NVS_DEFAULT_PART_NAME, NVS_VOL, NVS_TYPE_BLOB);
    while (it != NULL) { 
        nvs_entry_info_t info; 
        nvs_entry_info(it, &info); 
        it = nvs_entry_next(it); 
        if(strcmp(info.key, "private_key") == 0) return true;
        printf("key '%s', type '%d' ", info.key, info.type); 
    };
    return false;
}

bool esp32_cert_nvs::hasPublicCert()
{
    nvs_handle_t storage_handle;
    esp_err_t err = nvs_open(NVS_VOL, NVS_READONLY, &storage_handle);
    auto it = nvs_entry_find(NVS_DEFAULT_PART_NAME, NVS_VOL, NVS_TYPE_BLOB);
    while (it != NULL) { 
        nvs_entry_info_t info; 
        nvs_entry_info(it, &info); 
        it = nvs_entry_next(it); 
        if(strcmp(info.key, "public_key") == 0) return true;
        printf("key '%s', type '%d' ", info.key, info.type); 
    };
    return false;
}

void esp32_cert_nvs::loadCertificates()
{   
    //generateCert("esp32-dev","Fun company"); return;
    //get public and private keys from spiffs. get lengths, generate new cert object
    if(!hasPublicCert() || !hasPrivateKey()) {
        generateCert("esp32-dev","Fun company");
        return;
    } else{
        logger.logDebug("Certificates found in NVS... retrieveing");        
    }
    
    bool errorReadingCert = false;

    nvs_handle_t storage_handle;
    esp_err_t err = nvs_open(NVS_VOL, NVS_READONLY, &storage_handle);
    uint16_t publicLength, privateLength;

    if(nvs_get_u16(storage_handle, NVS_CERT_LENGTH_FIELD,&publicLength) != ESP_OK){
        logger.logError(string_format("Error, failed to get public key length from NVS reading field %s", NVS_CERT_LENGTH_FIELD));        
        errorReadingCert = true;
    }

    if(nvs_get_u16(storage_handle,NVS_KEY_LENGTH_FIELD ,&privateLength) != ESP_OK){
        logger.logError(string_format("Error, failed to get private key length from NVS reading field %s", NVS_KEY_LENGTH_FIELD));
        errorReadingCert = true;
    }

    logger.logInfo(string_format("Loading certificate from NVS with length of %u and private key length of %u", 
        publicLength, privateLength
    ));

    publicKey = new char[publicLength];
    privateKey = new char[privateLength];

    if (nvs_get_blob(storage_handle, NVS_CERT_FIELD, publicKey, (size_t*)&publicLength) == ESP_OK) {
        logger.logDebug(string_format("Successfully read blob from NVS: %s", NVS_CERT_FIELD));
    } else {
        logger.logError("Error reading public certificate from NVS.");
        errorReadingCert = true;
        //ESP_LOGE(TAG, "Failed to read blob from NVS");
    }
    if (nvs_get_blob(storage_handle,NVS_KEY_FIELD , privateKey, (size_t*)&privateLength) == ESP_OK) {
        logger.logDebug(string_format("Successfully read blob from NVS: %s", NVS_KEY_FIELD));
    } else {
        logger.logError("Error reading private certificate from NVS.");
        errorReadingCert = true;
        //ESP_LOGE(TAG, "Failed to read blob from NVS");
    }
    if(errorReadingCert) {
        generateCert("esp32-device", "fun co");
        return;
    }

    _cert = new SSLCert((unsigned char *)publicKey,(uint16_t) publicLength,(unsigned char *)privateKey,(uint16_t) privateLength);
}

void esp32_cert_nvs::generateCert(const char* deviceName, const char* companyName, const char* validFrom, const char* validTo)
{
    esp32_cert_base::generateCert(deviceName, companyName, validFrom, validTo);
    saveCertificates();  
}

void esp32_cert_nvs::saveCertificates(){
// save to NVS
    nvs_handle_t storage_handle;
    esp_err_t err = nvs_open(NVS_VOL, NVS_READWRITE, &storage_handle);

    nvs_set_blob(storage_handle, NVS_CERT_FIELD, _cert->getCertData(), _cert->getCertLength());
    err = nvs_commit(storage_handle);

    if(err != ESP_OK){
        logger.logError("Failed to store public key in NVS");
    }

    nvs_set_blob(storage_handle, NVS_KEY_FIELD, _cert->getPKData(), _cert->getPKLength());
    err = nvs_commit(storage_handle);
    if(err != ESP_OK){
        logger.logError("Failed to store private key in NVS");
    }
    
    //store size info
    nvs_set_u16(storage_handle, NVS_CERT_LENGTH_FIELD, _cert->getCertLength());
    err = nvs_commit(storage_handle);
    if(err != ESP_OK){
        logger.logError("Failed to store public key length in NVS");
    }
    nvs_set_u16(storage_handle, NVS_KEY_LENGTH_FIELD, _cert->getPKLength());

    err = nvs_commit(storage_handle);
    if(err != ESP_OK){
        logger.logError("Failed to store private key length in NVS");
    }

    nvs_close(storage_handle);
}

bool esp32_cert_nvs::importFromTemporary()
{
    logger.logDebug("Importing certificats from temporary storage");    
    File pubFile = SPIFFS.open(PUBLIC_TEMP_PATH, "r");
    File priFile = SPIFFS.open(PRIVATE_TEMP_PATH, "r");

    char *publicKey = new char[pubFile.size()];
    char *privateKey = new char[priFile.size()];
    
    pubFile.readBytes(publicKey, pubFile.size());
    priFile.readBytes(privateKey, priFile.size());
    
    /* One option is to set fields if ssl cert is initialized*/
    // _cert->setCert((unsigned char *)publicKey, publicLength);
    // _cert->setPK((unsigned char *)privateKey, privateLength);

    _cert = new SSLCert((unsigned char *)publicKey, pubFile.size(),(unsigned char *)privateKey, priFile.size());
    saveCertificates();
    pubFile.close();
    priFile.close();

    SPIFFS.remove(PUBLIC_TEMP_PATH);
    SPIFFS.remove(PRIVATE_TEMP_PATH);
}
