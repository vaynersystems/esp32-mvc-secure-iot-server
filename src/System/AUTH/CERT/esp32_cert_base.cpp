#include "esp32_cert_base.hpp"
#include <string>
#include <string_extensions.h>

void esp32_cert_base::generateCert(const char *deviceName, const char *companyName, const char *validFrom, const char *validTo)
{  
    _cert = new SSLCert();
    string dn = "";
    dn = string_format("CN=%s,O=%s,C=US", deviceName, companyName);
    Serial.printf("Generating certificate for DN %s\n", dn.c_str());
    auto certCreated = createSelfSignedCert(
        *_cert,
        SSLKeySize::KEYSIZE_2048, 
        dn.c_str(),
        validFrom,
        validTo
    );
    //TODO: use MDSN name from config for certificate
    if(certCreated != 0) {
        Serial.println("Failed to generate a new certificate");
        return;
    }
}
