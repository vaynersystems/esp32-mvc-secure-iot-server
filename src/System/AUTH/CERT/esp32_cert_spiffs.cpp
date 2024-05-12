#include "esp32_cert_base.hpp"
#include "esp32_cert_spiffs.hpp"
#include <string.h>
extern const char* PUBLIC_TEMP_PATH;
extern const char* PRIVATE_TEMP_PATH;

bool esp32_cert_spiffs::hasPrivateKey()
{
    return SPIFFS.exists(SPIFFS_PRIVATE_KEY_PATH);
}

bool esp32_cert_spiffs::hasPublicCert()
{
    return SPIFFS.exists(SPIFFS_PUBLIC_KEY_PATH);
}

void esp32_cert_spiffs::loadCertificates()
{   
    //get public and private keys from spiffs. get lengths, generate new cert object
    if(!hasPublicCert() || !hasPrivateKey()) {
        generateCert("esp32-dev","Fun company");
        return;
    } else{
        Serial.println("Certificates found in NVS... retrieveing");
    }
    
    bool errorReadingCert = false;

    File pubFile = SPIFFS.open(SPIFFS_PUBLIC_KEY_PATH, "r");
    File priFile = SPIFFS.open(SPIFFS_PRIVATE_KEY_PATH, "r");

    char *publicKey = new char[pubFile.size()];
    char *privateKey = new char[priFile.size()];
    
    pubFile.readBytes(publicKey, pubFile.size());
    priFile.readBytes(privateKey, priFile.size());

    _cert = new SSLCert((unsigned char *)publicKey, pubFile.size(),(unsigned char *)privateKey, priFile.size());
}

void esp32_cert_spiffs::generateCert(const char* deviceName, const char* companyName, const char* validFrom, const char* validTo)
{
    esp32_cert_base::generateCert(deviceName, companyName, validFrom, validTo);
    saveCertificates();
    
}

void esp32_cert_spiffs::generateTemporaryCertificate(const char *deviceName, const char *companyName, const char *validFrom, const char *validTo)
{
    SSLCert* tempCert = new SSLCert(*_cert);

    generateCert(deviceName, companyName, validFrom, validTo);
    // SAVING TO SPIFFS TEMP LOCATION
    File pubFile = SPIFFS.open(PUBLIC_TEMP_PATH, "w");
    //print out certificate date
    pubFile.write(_cert->getCertData(), _cert->getCertLength());

    pubFile.close();

    File priFile = SPIFFS.open(PRIVATE_TEMP_PATH, "w");
    priFile.write(_cert->getPKData(), _cert->getPKLength());
    priFile.close();

    _cert = tempCert; //restore cert
}

void esp32_cert_spiffs::saveCertificates(){
    // SAVING TO SPIFFS
    File pubFile = SPIFFS.open(SPIFFS_PUBLIC_KEY_PATH, "w");
    //print out certificate date
    pubFile.write(_cert->getCertData(), _cert->getCertLength());

    pubFile.close();

    File priFile = SPIFFS.open(SPIFFS_PRIVATE_KEY_PATH, "w");
    priFile.write(_cert->getPKData(), _cert->getPKLength());
    priFile.close();
}

bool esp32_cert_spiffs::importFromTemporary()
{
    Serial.println("[CERT_SPIFFS] Importing certificats from temporary storage");

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

