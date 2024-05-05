#include "esp32_server.h"
#include <HTTPMiddlewareFunction.hpp>
#include <esp_task_wdt.h>

esp32_server::esp32_server() : unsecureServer(new HTTPServer())
{
}
esp32_server::esp32_server(SSLCert* cert) : unsecureServer(new HTTPServer())
{
    StaticJsonDocument<1024> systemConfig;
    esp32_config::getConfigSection("system", &systemConfig);
     _enableSSL = systemConfig["enableSSL"].as<bool>();
    _cert = cert; //used for jwt as well

    if(_enableSSL){
        secureServer = new HTTPSServer(cert);        
        secureServer->addMiddleware(middleware->middlewareAuthentication);
        secureServer->addMiddleware(middleware->middlewareAuthorization);
    }
    
    unsecureServer->addMiddleware(middleware->middlewareAuthentication);
    unsecureServer->addMiddleware(middleware->middlewareAuthorization);
}

bool esp32_server::start() {
    
    //sp_task_wdt_init(32,false);
    _router = new esp32_router();
    middleware = new esp32_middleware();
    
    middleware->middlewareSetTokenizer((char*)_cert->getPKData());
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

    WebsocketNode * persistanceNode = new WebsocketNode("/persistance", &esp32_socket::createSocket);
    _router->RegisterHandler( corsNode);
    _router->RegisterHandler( node404);
    _router->RegisterHandler( nodeSpecial);
    //_router->RegisterHandler("/esp32_*", HTTPMETHOD_GET, &esp32_router::handleRoot);

    if(_enableSSL){
        secureServer->setDefaultNode(nodeRoot);
    }
    unsecureServer->setDefaultNode(nodeRoot);
    
    Serial.println("Starting server...");
   

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
    
}

void esp32_server::registerNode(HTTPNode * node){
    if(_enableSSL)
        secureServer->registerNode(node);
    unsecureServer->registerNode(node);    
}

bool esp32_server::registerNewCert(SSLCert* cert)
{
    if(!_enableSSL ) return false;
    if (secureServer->isRunning())
    {
        ESP_LOGD("Stopping server to register new cert", ESP_LOG_INFO);
        secureServer->stop();
    }

    secureServer =new HTTPSServer(cert);
    _cert = cert;
}

