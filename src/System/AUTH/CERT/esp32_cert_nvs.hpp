#ifndef _ESP32_CERT_NVS_H
#define _ESP32_CERT_NVS_H
#include "esp32_cert_base.hpp"
#include <SSLCert.hpp>
#include "nvs.h"
#include <string.h>
#include <SPIFFS.h>
#include "string_helper.h"

class esp32_cert_nvs : public esp32_cert_base{

public:
    inline bool hasPrivateKey();
    inline bool hasPublicCert();

    SSLCert * getCert(){
        return _cert;
    }

    void loadCertificates();
    void saveCertificates();
    bool importFromTemporary();
    void generateCert(
        const char* deviceName = "myesp32.local",
        const char* companyName = "Fancy Co",
        const char* validFrom = "20240101000000",
        const char* validTo = "20350101000000"
    );

private:
    const char *NVS_VOL = "esp32-dev";
    const char *NVS_CERT_LENGTH_FIELD = "public_length";
    const char *NVS_KEY_LENGTH_FIELD = "private_length";
    const char *NVS_CERT_FIELD = "public_key";
    const char *NVS_KEY_FIELD = "private_key";
};

#endif