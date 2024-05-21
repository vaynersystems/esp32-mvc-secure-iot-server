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
    logger.logDebug(string_format("Setting certificate storage to %s",
        serverConfig["certificates"]["source"].isNull() ? "NVS by default" : serverConfig["certificates"]["source"].as<const char*>()
    ));
    
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

    if(_enableSSL)
        secureServer->start();

    unsecureServer->start();

    bool isReady =  (!_enableSSL || secureServer->isRunning()) && unsecureServer->isRunning();
    if (isReady) {
        #ifdef DEBUG
        Serial.println("Server ready.");
        #endif
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

void esp32_server::registerNode(HTTPNode *node)
{
    if(_enableSSL)
        secureServer->registerNode(node);
    unsecureServer->registerNode(node);
}

SSLCert *esp32_server::getCertificate()
{
    //if(_certManager == nullptr) sleep(500);
    return _certManager->getCert();
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

