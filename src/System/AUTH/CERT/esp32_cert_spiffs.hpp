#ifndef _ESP32_CERT_SPIFFS_H
#define _ESP32_CERT_SPIFFS_H
#include "esp32_cert_base.hpp"
#include <SSLCert.hpp>
#include "SPIFFS.h"
#include "string_helper.h"

class esp32_cert_spiffs : public esp32_cert_base{

public:
    inline bool hasPrivateKey();
    inline bool hasPublicCert();

    void loadCertificates();
    void saveCertificates();
    bool importFromTemporary();
    void generateCert(
        const char* deviceName = "myesp32.local",
        const char* companyName = "Fancy Co",
        const char* validFrom = "20240101000000",
        const char* validTo = "20350101000000"
    );

    void generateTemporaryCertificate(
        const char* deviceName,
        const char* companyName,
        const char* validFrom,
        const char* validTo);

private:
    const char *SPIFFS_PUBLIC_KEY_PATH = "/INT/cert.cer";
    const char *SPIFFS_PRIVATE_KEY_PATH = "/INT/key.der";
};

#endif