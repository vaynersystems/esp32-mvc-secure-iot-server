#ifndef _ESP32_CERT_BASE_H
#define _ESP32_CERT_BASE_H
#include <SSLCert.hpp>
using namespace httpsserver;
class esp32_cert_base{

public:
    esp32_cert_base(){};
    ~esp32_cert_base(){
        // if(publicKey != NULL)
        //     delete publicKey;
        // if(privateKey != NULL)
        //     delete privateKey;
        if(_cert != NULL)
            delete _cert;
    }
    SSLCert * getCert(){
        return _cert;
    }
    virtual bool hasPrivateKey(){ return false;}
    virtual bool hasPublicCert(){ return false;}

    virtual void loadCertificates(){};
    virtual void saveCertificates(){};
    virtual bool importFromTemporary(){};
    virtual void generateCert(
        const char* deviceName = "myesp32.local",
        const char* companyName = "Fancy Co",
        const char* validFrom = "20240101000000",
        const char* validTo = "20350101000000"
    );
    // virtual SSLCert* getCert(){
    //     return _cert;
    // }
    
protected:
    SSLCert* _cert;
    char *publicKey;
    char *privateKey;
};
static const char* PUBLIC_TEMP_PATH = "/TMP/public.cer";
static const char* PRIVATE_TEMP_PATH = "/TMP/private.key";
#endif