#include "esp32_cert_base.hpp"
#include <string>
#include "string_helper.h"
#include <vector>
#include "mbedtls/pem.h"
#include <strings.h>
#include <IPAddress.h>
#include "System/CORE/esp32_config.hpp"

void esp32_cert_base::generateCert(const char *deviceName, const char *companyName, const char *validFrom, const char *validTo)
{  
    _cert = new SSLCert();
    string dn = "";
    dn = string_format("CN=%s,O=%s,C=US", deviceName, companyName);

    // Names a browser may use for the device: the certificate's name, the configured
    // hostname (plain and .local), and the default access-point address.
    vector<string> dnsNames;
    vector<uint32_t> addresses;
    auto addName = [&](const string& name){
        if(name.empty()) return;
        for(auto& existing : dnsNames) if(strcasecmp(existing.c_str(), name.c_str()) == 0) return;
        dnsNames.push_back(name);
    };
    addName(deviceName);
    DynamicJsonDocument systemConfig(2048);
    esp32_config::getConfigSection("system", &systemConfig);
    string hostname = systemConfig["hostname"] | "";
    if(!hostname.empty()){
        addName(hostname);
        addName(hostname + ".local");
    }
    addresses.push_back((uint32_t)IPAddress(192, 168, 4, 1));

    Serial.println("Generating ECDSA P-256 certificate...");
    auto certCreated = createSelfSignedCert(
        *_cert,
        SSLKeySize::KEYTYPE_ECDSA_P256, // handshakes several times cheaper than RSA-2048
        dn.c_str(),
        validFrom,
        validTo,
        dnsNames,
        addresses
    );
    if(certCreated == 0){
        // The public certificate (no secrets), so it can be checked or pinned by hand.
        size_t pemLength = 0;
        static unsigned char pem[2048];
        if(mbedtls_pem_write_buffer("-----BEGIN CERTIFICATE-----\n", "-----END CERTIFICATE-----\n",
                _cert->getCertData(), _cert->getCertLength(), pem, sizeof(pem), &pemLength) == 0)
            Serial.printf("New device certificate:\n%s", (const char*)pem);
    }
    if(certCreated != 0) {
        Serial.printf("Certificate generation failed: %d\n", certCreated);
        #if defined(DEBUG_SECURITY) && DEBUG_SECURITY > 0
        Serial.println("Failed to generate a new certificate");
        #endif
        return;
    }
}


/// @brief True for a certificate this device generated itself with an RSA key (older firmware).
/// @brief Those are replaced by an ECDSA one; a certificate the user uploaded (not self-issued) is kept.
bool esp32_cert_base::isSelfIssuedRsa(SSLCert* cert)
{
    if(cert == nullptr || cert->getPKLength() == 0 || cert->getCertLength() == 0) return false;
    bool rsa = false, selfIssued = false;
    mbedtls_pk_context key;
    mbedtls_pk_init(&key);
    if(mbedtls_pk_parse_key(&key, cert->getPKData(), cert->getPKLength(), NULL, 0) == 0)
        rsa = mbedtls_pk_get_type(&key) == MBEDTLS_PK_RSA;
    mbedtls_pk_free(&key);
    if(!rsa) return false;

    mbedtls_x509_crt crt;
    mbedtls_x509_crt_init(&crt);
    if(mbedtls_x509_crt_parse_der(&crt, cert->getCertData(), cert->getCertLength()) == 0)
        selfIssued = crt.issuer_raw.len == crt.subject_raw.len &&
                     memcmp(crt.issuer_raw.p, crt.subject_raw.p, crt.issuer_raw.len) == 0;
    mbedtls_x509_crt_free(&crt);
    return selfIssued;
}
