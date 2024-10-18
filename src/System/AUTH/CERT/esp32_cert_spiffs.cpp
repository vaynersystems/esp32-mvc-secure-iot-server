#include "esp32_cert_base.hpp"
#include "esp32_cert_spiffs.hpp"
#include <string.h>

extern esp32_file_system filesystem;
// extern const char* PUBLIC_TEMP_PATH;
// extern const char* PRIVATE_TEMP_PATH;

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
        logger.logDebug("Certificates found in SPIFFS... retrieveing");
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
    auto drive = filesystem.getDisk(0);

    auto pubFile = drive->open(PUBLIC_TEMP_PATH, "w", true);
    auto priFile = drive->open(PRIVATE_TEMP_PATH, "w", true);    
    
    pubFile.write(_cert->getCertData(), _cert->getCertLength());
    pubFile.close();
    
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
    logger.logDebug("[CERT_SPIFFS] Importing certificats from temporary storage");
    auto drive = filesystem.getDisk(0);

    File pubFile = drive->open(PUBLIC_TEMP_PATH, "r");
    File priFile = drive->open(PRIVATE_TEMP_PATH, "r");

    char *publicKey = new char[pubFile.size()];
    char *privateKey = new char[priFile.size()];
    
    pubFile.readBytes(publicKey, pubFile.size());
    priFile.readBytes(privateKey, priFile.size());    
    
    auto cert = new SSLCert((unsigned char *)publicKey, pubFile.size(),(unsigned char *)privateKey, priFile.size());

    pubFile.close();
    priFile.close();

    drive->remove(PUBLIC_TEMP_PATH);
    drive->remove(PRIVATE_TEMP_PATH);

    if(cert->getCertLength() > 0 && cert->getPKLength() > 0)
    {
        _cert = cert;
        saveCertificates();
        #if defined(DEBUG_SECURITY) && DEBUG_SECURITY > 0
        Serial.printf("Imported certificate with %d bytes of data and %d bytes key\n", cert->getCertLength(), cert->getPKLength());
        #endif 
        return true;
    }
    return false;
}

