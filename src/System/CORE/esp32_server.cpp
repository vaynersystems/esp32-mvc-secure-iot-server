#include "esp32_server.hpp"
#include <HTTPMiddlewareFunction.hpp>
#include <esp_task_wdt.h>
#include "mbedtls/md.h"
#ifndef HTTPS_MAX_CONNECTIONS
  #ifdef BOARD_HAS_PSRAM
    #define HTTPS_MAX_CONNECTIONS 8   // TLS buffers go to PSRAM
  #else
    #define HTTPS_MAX_CONNECTIONS 4   // each TLS connection needs ~40 KB of internal RAM
  #endif
#endif
#include "esp32_wifi.h"
#include <HTTPSConnection.hpp>
#include "System/AUTH/esp32_authentication.hpp"

#include "ArduinoOTA.h"
esp32_server::esp32_server() : unsecureServer(new HTTPServer())
{
    
   
}

bool esp32_server::start() {
    //check if ssl setting is enabled
    DynamicJsonDocument systemConfig(2048);
    esp32_config::getConfigSection("system", &systemConfig);    

    if(systemConfig["enableSSL"].isNull())
        _enableSSL = true; //default to on
    else 
        _enableSSL = systemConfig["enableSSL"].as<bool>();

    // check for certificate storage location
    DynamicJsonDocument serverConfig(1024);
    esp32_config::getConfigSection("server", &serverConfig);
    logger.logDebug(string_format("Setting certificate storage to %s",
        serverConfig["certificates"]["source"].isNull() ? "NVS by default" : serverConfig["certificates"]["source"].as<const char*>()
    ));
    
    if(serverConfig["certificates"]["source"].isNull() || 
        iequals(serverConfig["certificates"]["source"].as<const char*>(),"nvs",3))
            _certManager = new esp32_cert_nvs();        
    
    else
        _certManager = new esp32_cert_fs();        
    
    //initialize certificates
    _certManager->loadCertificates();   
    if(_enableSSL)
        Serial.printf("HTTPS certificate SHA-256 fingerprint: %s\n", certificateFingerprint().c_str());
    esp32_authentication::announceSetupCode();

    _restartMillis = serverConfig["restartAfter"].isNull() ? 0 : atoi(serverConfig["restartAfter"].as<const char*>()) * 1000 * 3600; // to seconds, then to hours
    #ifdef DEBUG 
    uint64_t millisInAnHour = (uint64_t)1000 * (uint64_t)3600;
    if(_restartMillis > 0)
        Serial.printf("Setting reset timer to %lu hours\n", _restartMillis / millisInAnHour);
    #endif

    if(_enableSSL){       
        // While the device runs its own access point, DNS sends every name to it, so phones
        // open TLS connections for google.com and the like all the time. Each would fail on
        // the certificate anyway, but only after a ~0.5-1 s handshake that blocks the server.
        // Refuse those by their server name before any cryptography.
        HTTPSConnection::setServerNameFilter([](const char* serverName) -> bool {
            return !esp32_wifi::captivePortalActive() || !esp32_wifi::isForeignHost(serverName);
        });
        // More slots on PSRAM boards (library default 4): pages with a live view hold a
        // websocket open, and with TLS buffers in PSRAM internal RAM no longer limits the count.
        secureServer = new HTTPSServer(_certManager->getCert(), 443, HTTPS_MAX_CONNECTIONS);        
        secureServer->addMiddleware(middleware->middlewareAuthentication);
        secureServer->addMiddleware(middleware->middlewareAuthorization);
    } 
    
    unsecureServer->addMiddleware(middleware->middlewareAuthentication);
    unsecureServer->addMiddleware(middleware->middlewareAuthorization);

    _router = new esp32_router();
    middleware = new esp32_middleware();
    
    middleware->middlewareSetTokenizer(_certManager->getCert()->getPKData(), _certManager->getCert()->getPKLength());
    //load exclusions list
    middleware->initPublicPages();
    _router->RegisterHandler( "/", HTTPMETHOD_GET, &esp32_router::handleRoot);

    _router->RegisterHandler( "/list", HTTPMETHOD_GET, &esp32_router::handleFileList);
    _router->RegisterHandler( "/logout", HTTPMETHOD_GET, &esp32_router::dummyPageHandler);
    
    //edit page handler.
    bool enableEditor = true;
    if(!systemConfig["features"].isNull()){
        if(!systemConfig["features"]["enableEditor"].isNull())
            enableEditor = systemConfig["features"]["enableEditor"].as<bool>();
    }
    if(enableEditor){
        _router->RegisterHandler( "/edit", HTTPMETHOD_GET, &esp32_router::handleEditor);
        _router->RegisterHandler( "/edit", HTTPMETHOD_PUT, &esp32_router::handleFileUpload);
        _router->RegisterHandler( "/edit", HTTPMETHOD_POST, &esp32_router::handleFileUpload);
        _router->RegisterHandler( "/edit", HTTPMETHOD_DELETE, &esp32_router::handleFileUpload);            
    }
    
    ResourceNode* node404 = new ResourceNode("", "GET", &esp32_router::handle404);
    ResourceNode* nodeRoot = new ResourceNode("", "GET", &esp32_router::handleRoot);
    ResourceNode* nodeSpecial = new ResourceNode("/special/*", "GET", &esp32_router::handleFileList);
    ResourceNode* corsNode = new ResourceNode("/*", "OPTIONS", &esp32_router::handleCORS);

    WebsocketNode * persistanceNode = new WebsocketNode("/socket", &esp32_socket::createSocket);
    _router->RegisterWebsocket(persistanceNode);
    
    _router->RegisterHandler( corsNode);
    _router->RegisterHandler( node404);
    _router->RegisterHandler( nodeSpecial);

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
    return true;
}

bool esp32_server::isRunning() {
    return (!_enableSSL || secureServer->isRunning()) && unsecureServer->isRunning();
}
unsigned long _lastReporStatus = 0;
void esp32_server::step()
{
    // Repeat the setup code now and then: the serial monitor is often opened after boot.
    static unsigned long lastSetupAnnouncement = 0;
    if(millis() - lastSetupAnnouncement > 60000UL){
        lastSetupAnnouncement = millis();
        esp32_authentication::announceSetupCode();
    }
    if(_enableSSL)
        secureServer->loop();
    unsecureServer->loop();  

    if(_restartMillis > 0 && (esp_timer_get_time()/1000) > _restartMillis) {
        logger.logInfo("Restarting controller due to configuration 'restartAfter' parameter.");
        esp_restart();
    }         
}

void esp32_server::registerNode(HTTPNode *node)
{
    if(_enableSSL)
        secureServer->registerNode(node);
    unsecureServer->registerNode(node);
}

std::string esp32_server::certificateFingerprint()
{
    SSLCert* cert = getCertificate();
    if(cert == nullptr || cert->getCertLength() == 0) return "";
    uint8_t hash[32];
    if(mbedtls_md(mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), cert->getCertData(), cert->getCertLength(), hash) != 0)
        return "";
    char out[32 * 3];
    for(int i = 0; i < 32; i++)
        sprintf(out + i * 3, i < 31 ? "%02X:" : "%02X", hash[i]);
    return std::string(out);
}

SSLCert *esp32_server::getCertificate()
{
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
    esp32_cert_fs *fsCertManager = new esp32_cert_fs();
    fsCertManager->generateTemporaryCertificate(deviceName, companyName, validFrom, validTo);    

    delete fsCertManager;
}

