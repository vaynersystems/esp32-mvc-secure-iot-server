#include "esp32_server.h"
#include <HTTPMiddlewareFunction.hpp>
#include <esp_task_wdt.h>

// esp32_server::esp32_server() : unsecureServer(new HTTPServer())
// {
// }
esp32_server::esp32_server() : unsecureServer(new HTTPServer())
{
    
   
}

bool esp32_server::start() {
    //check if ssl setting is enabled
    StaticJsonDocument<1024> systemConfig;
    esp32_config::getConfigSection("system", &systemConfig);    

    if(systemConfig["enableSSL"].isNull())
        _enableSSL = true; //default to on
    else 
        _enableSSL = systemConfig["enableSSL"].as<bool>();

    // check for certificate storage location
    StaticJsonDocument<1024> serverConfig;
    esp32_config::getConfigSection("server", &serverConfig);
    #ifdef DEBUG
     Serial.printf("Setting certificate storage to %s\n", serverConfig["certificates"]["source"].isNull() ? "NVS by default" : serverConfig["certificates"]["source"].as<const char*>());
    #endif
    if(serverConfig["certificates"]["source"].isNull() || 
        iequals(serverConfig["certificates"]["source"].as<const char*>(),"nvs",3))            
        _certManager = new esp32_cert_nvs();
        
    else
        _certManager = new esp32_cert_spiffs();
    //initialize certificates
    _certManager->loadCertificates();   

    if(_enableSSL){       
        secureServer = new HTTPSServer(_certManager->getCert());        
        secureServer->addMiddleware(middleware->middlewareAuthentication);
        secureServer->addMiddleware(middleware->middlewareAuthorization);
    } 
    
    unsecureServer->addMiddleware(middleware->middlewareAuthentication);
    unsecureServer->addMiddleware(middleware->middlewareAuthorization);

    _router = new esp32_router();
    middleware = new esp32_middleware();
    
    middleware->middlewareSetTokenizer((char*)_certManager->getCert()->getPKData());
    //load exclusions list
    middleware->initPublicPages();
    _router->RegisterHandlers(SPIFFS, SITE_ROOT, 3);
    _router->RegisterHandler( "/", HTTPMETHOD_GET, &esp32_router::handleRoot);

    _router->RegisterHandler( "/list", HTTPMETHOD_GET, &esp32_router::handleFileList);
    _router->RegisterHandler( "/logout", HTTPMETHOD_GET, &esp32_router::dummyPageHandler);
    
    //edit page handler.
    //TODO: add system config variable to control if handlers are registered
    _router->RegisterHandler( "/edit", HTTPMETHOD_PUT, &esp32_router::handleFileUpload);
    _router->RegisterHandler( "/edit", HTTPMETHOD_POST, &esp32_router::handleFileUpload);
    _router->RegisterHandler( "/edit", HTTPMETHOD_DELETE, &esp32_router::handleFileUpload);
    
    ResourceNode* node404 = new ResourceNode("", "GET", &esp32_router::handle404);
    ResourceNode* nodeRoot = new ResourceNode("", "GET", &esp32_router::handleRoot);
    ResourceNode* nodeSpecial = new ResourceNode("/special/*", "GET", &esp32_router::handleFileList);
    ResourceNode* corsNode = new ResourceNode("/*", "OPTIONS", &esp32_router::handleCORS);

    WebsocketNode * persistanceNode = new WebsocketNode("/socket", &esp32_socket::createSocket);
    _router->RegisterWebsocket(persistanceNode);
    
    _router->RegisterHandler( corsNode);
    _router->RegisterHandler( node404);
    _router->RegisterHandler( nodeSpecial);
    //_router->RegisterHandler("/esp32_*", HTTPMETHOD_GET, &esp32_router::handleRoot);

    if(_enableSSL){
        secureServer->setDefaultNode(nodeRoot);
    }
    unsecureServer->setDefaultNode(nodeRoot);
    #ifdef DEBUG
    Serial.println("Starting server...");
    #endif
    // Serial.printf("Cert with length %d:\n", _cert->getCertLength());
    // for(int idx=0;idx < _cert->getCertLength(); idx++){
    //     Serial.printf("%02X",_cert->getCertData()[idx]);
    // }
    // //Serial.write(_cert->getCertData(), _cert->getCertLength());
    // Serial.println();

    // Serial.printf("Key with length %d:\n", _cert->getPKLength());
    // for(int idx=0;idx < _cert->getPKLength(); idx++){
    //     Serial.printf("%02X",_cert->getPKData()[idx]);
    // }
    // Serial.write(_cert->getPKData(), _cert->getPKLength());
    // Serial.println();
   

    if(_enableSSL)
        secureServer->start();

    unsecureServer->start();

    bool isReady =  (!_enableSSL || secureServer->isRunning()) && unsecureServer->isRunning();
    if (isReady) {
        Serial.println("Server ready.");
    }
    return isReady;
}

bool esp32_server::stop()
{
    if (isRunning()){
        secureServer->stop();
        unsecureServer->stop();        
    }
    //esp_task_wdt_deinit();
    // delete publicKey;
    // delete privateKey;
    return true;
}

bool esp32_server::isRunning() {
    return (!_enableSSL || secureServer->isRunning()) && unsecureServer->isRunning();
}

void esp32_server::step()
{
    if(_enableSSL)
        secureServer->loop();
    unsecureServer->loop();

    //TODO: process any encryption/decryption requests
    
}

bool esp32_server::importCertFromTemporaryStorage()
{
    return _certManager->importFromTemporary();
}
/// @brief Generates a certificate and places it into temporary storage
/// @param deviceName 
/// @param companyName 
/// @param validFrom format is YYYYMMDDHHMMSS
/// @param validTo format is YYYYMMDDHHMMSS
void esp32_server::generateCertificate(const char *deviceName, const char *companyName, const char *validFrom, const char *validTo)
{
    esp32_cert_spiffs *spiffsCertManager = new esp32_cert_spiffs();
    spiffsCertManager->generateTemporaryCertificate(deviceName, companyName, validFrom, validTo);    

    delete spiffsCertManager;
}

void esp32_server::registerNode(HTTPNode *node)
{
    if(_enableSSL)
        secureServer->registerNode(node);
    unsecureServer->registerNode(node);
}

// bool esp32_server::hasPrivateKey()
// {
//     nvs_handle_t storage_handle;
//     esp_err_t err = nvs_open(NVS_VOL, NVS_READONLY, &storage_handle);
//     auto it = nvs_entry_find(NVS_DEFAULT_PART_NAME, NVS_VOL, NVS_TYPE_BLOB);
//     while (it != NULL) { 
//         nvs_entry_info_t info; 
//         nvs_entry_info(it, &info); 
//         it = nvs_entry_next(it); 
//         if(strcmp(info.key, "private_key") == 0) return true;
//         printf("key '%s', type '%d' ", info.key, info.type); 
//     };
//     return false;
// }

// bool esp32_server::hasPublicCert()
// {
//     nvs_handle_t storage_handle;
//     esp_err_t err = nvs_open(NVS_VOL, NVS_READONLY, &storage_handle);
//     auto it = nvs_entry_find(NVS_DEFAULT_PART_NAME, NVS_VOL, NVS_TYPE_BLOB);
//     while (it != NULL) { 
//         nvs_entry_info_t info; 
//         nvs_entry_info(it, &info); 
//         it = nvs_entry_next(it); 
//         if(strcmp(info.key, "public_key") == 0) return true;
//         printf("key '%s', type '%d' ", info.key, info.type); 
//     };
//     return false;
// }

// void esp32_server::loadCertificates()
// {   
//     //generateCert("esp32-dev","Fun company"); return;
//     //get public and private keys from spiffs. get lengths, generate new cert object
//     if(!hasPublicCert() || !hasPrivateKey()) {
//         generateCert("esp32-dev","Fun company");
//         return;
//     } else{
//         Serial.println("Certificates found in NVS... retrieveing");
//     }
    
//     bool errorReadingCert = false;

//     nvs_handle_t storage_handle;
//     esp_err_t err = nvs_open(NVS_VOL, NVS_READONLY, &storage_handle);
//     uint16_t publicLength, privateLength;

//     if(nvs_get_u16(storage_handle, NVS_CERT_LENGTH_FIELD,&publicLength) != ESP_OK){
//         Serial.printf("Error, failed to get public key length from NVS reading field %s\n", NVS_CERT_LENGTH_FIELD);
//         errorReadingCert = true;
//     }

//     if(nvs_get_u16(storage_handle,NVS_KEY_LENGTH_FIELD ,&privateLength) != ESP_OK){
//         Serial.printf("Error, failed to get private key length from NVS reading field %s\n", NVS_KEY_LENGTH_FIELD);
//         errorReadingCert = true;
//     }

//     Serial.printf("Loading certificate from NVS with length of %u and private key length of %u\n", 
//         publicLength, privateLength    
//     );

//     publicKey = new char[publicLength];
//     privateKey = new char[privateLength];

//     if (nvs_get_blob(storage_handle, NVS_CERT_FIELD, publicKey, (size_t*)&publicLength) == ESP_OK) {
//         ESP_LOGI(TAG, "Successfully read blob from NVS: name:%s, id:%d", read_blob.name, read_blob.id);
//     } else {
//         Serial.printf("Error reading public certificate from NVS. ");
//         errorReadingCert = true;
//         //ESP_LOGE(TAG, "Failed to read blob from NVS");
//     }
//     if (nvs_get_blob(storage_handle,NVS_KEY_FIELD , privateKey, (size_t*)&privateLength) == ESP_OK) {
//         ESP_LOGI(TAG, "Successfully read blob from NVS: name:%s, id:%d", read_blob.name, read_blob.id);
//     } else {
//         Serial.printf("Error reading private certificate from NVS. ");
//         errorReadingCert = true;
//         //ESP_LOGE(TAG, "Failed to read blob from NVS");
//     }
//     if(errorReadingCert) {
//         generateCert("esp32-device", "fun co");
//         return;
//     }
//     // **** SPIFFS **/    
//     // File pubFile = SPIFFS.open(PATH_PUBLIC_CERT_FILE, "r");
//     // File priFile = SPIFFS.open(PATH_PRIVATE_KEY_FILE, "r");

//     // char *publicKey = new char[pubFile.size()];
//     // char *privateKey = new char[priFile.size()];
    
//     // pubFile.readBytes(publicKey, pubFile.size());
//     // priFile.readBytes(privateKey, priFile.size());
    
//     /* One option is to set fields if ssl cert is initialized*/
//     // _cert->setCert((unsigned char *)publicKey, publicLength);
//     // _cert->setPK((unsigned char *)privateKey, privateLength);

//     _cert = new SSLCert((unsigned char *)publicKey,(uint16_t) publicLength,(unsigned char *)privateKey,(uint16_t) privateLength);
// }

// void esp32_server::generateCert(const char* deviceName, const char* companyName, const char* validFrom, const char* validTo)
// {
//     _cert = new SSLCert();
//     string dn = "";
//     dn = string_format("CN=%s,O=%s,C=US", deviceName, companyName);
//     Serial.printf("Generating certificate for DN %s\n", dn.c_str());
//     auto certCreated = createSelfSignedCert(
//         *_cert,
//         SSLKeySize::KEYSIZE_2048, 
//         dn.c_str(),
//         validFrom,
//         validTo
//     );
//     //TODO: use MSDN name from config for certificate
//     if(certCreated != 0) {
//         Serial.println("Failed to generate a new certificate");
//         return;
//     }
    
//     // save to NVS
//     nvs_handle_t storage_handle;
//     esp_err_t err = nvs_open(NVS_VOL, NVS_READWRITE, &storage_handle);

//     nvs_set_blob(storage_handle, NVS_CERT_FIELD, _cert->getCertData(), _cert->getCertLength());
//     err = nvs_commit(storage_handle);

//     if(err != ESP_OK){
//         Serial.println("Failed to store public key in NVS");
//     }

//     nvs_set_blob(storage_handle, NVS_KEY_FIELD, _cert->getPKData(), _cert->getPKLength());
//     err = nvs_commit(storage_handle);
//     if(err != ESP_OK){
//         Serial.println("Failed to store private key in NVS");
//     }
    
//     //store size info
//     nvs_set_u16(storage_handle, NVS_CERT_LENGTH_FIELD, _cert->getCertLength());
//     err = nvs_commit(storage_handle);
//     if(err != ESP_OK){
//         Serial.println("Failed to store public key length in NVS");
//     }
//     nvs_set_u16(storage_handle, NVS_KEY_LENGTH_FIELD, _cert->getPKLength());

//     err = nvs_commit(storage_handle);
//     if(err != ESP_OK){
//         Serial.println("Failed to store private key length in NVS");
//     }

//     nvs_close(storage_handle);

//     //auto storedPublic = nvs_write_blob(TAG,"public",_cert->getCertData(), _cert.getCertLength());
//     // SAVING TO SPIFFS
//     // File pubFile = SPIFFS.open(PATH_PUBLIC_CERT_FILE, "w");
//     // //print out certificate date
//     // pubFile.write(_cert->getCertData(), _cert->getCertLength());

//     // pubFile.close();

//     // File priFile = SPIFFS.open(PATH_PRIVATE_KEY_FILE, "w");
//     // priFile.write(_cert->getPKData(), _cert->getPKLength());
//     // priFile.close();
// }

// bool esp32_server::registerNewCert(SSLCert* cert)
// {
//     if(!_enableSSL ) return false;
//     if (secureServer->isRunning())
//     {
//         ESP_LOGD("Stopping server to register new cert", ESP_LOG_INFO);
//         secureServer->stop();
//     }

//     secureServer =new HTTPSServer(cert);
//     //_cert = cert;
//     return true;
// }

