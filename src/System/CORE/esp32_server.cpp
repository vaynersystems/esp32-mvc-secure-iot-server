#include "esp32_server.h"
#include <HTTPMiddlewareFunction.hpp>
#include <esp_task_wdt.h>

esp32_server::esp32_server(SSLCert* cert) : secureServer(new HTTPSServer(cert)), unsecureServer(new HTTPServer())
{
    _cert = cert;
    
    // Add the middleware. These functions will be called globally for every request
    // Note: The functions are called in the order they are added to the server.
    // This means, we need to add the authentication middleware first, because the
    // authorization middleware needs the headers that will be set by the authentication
    // middleware (First we check the identity, then we see what the user is allowed to do)
    secureServer->addMiddleware(middleware->middlewareAuthentication);
    secureServer->addMiddleware(middleware->middlewareAuthorization);
    unsecureServer->addMiddleware(middleware->middlewareAuthentication);
    unsecureServer->addMiddleware(middleware->middlewareAuthorization);
}

bool esp32_server::start() {
    
    //sp_task_wdt_init(32,false);
    _router = new esp32_router();
    middleware = new esp32_middleware();
    middleware->middlewareSetTokenizer((char*)_cert->getPKData());
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
    _router->RegisterHandler( corsNode);
    _router->RegisterHandler( node404);
    _router->RegisterHandler( nodeSpecial);
    //_router->RegisterHandler("/esp32_*", HTTPMETHOD_GET, &esp32_router::handleRoot);

    secureServer->setDefaultNode(nodeRoot);
    unsecureServer->setDefaultNode(nodeRoot);
    
    Serial.println("Starting server...");
    secureServer->start();
    unsecureServer->start();
    if (secureServer->isRunning() && unsecureServer->isRunning()) {
        Serial.println("Server ready.");
    }
    return secureServer->isRunning() && unsecureServer->isRunning();
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
    return secureServer->isRunning() && unsecureServer->isRunning();
}

void esp32_server::step()
{
    secureServer->loop();
    unsecureServer->loop();
}

bool esp32_server::RegisterNewCert(SSLCert* cert)
{
    if (secureServer->isRunning())
    {
        ESP_LOGD("Stopping server to register new cert", ESP_LOG_INFO);
        secureServer->stop();
    }

    secureServer =new HTTPSServer(cert);
    _cert = cert;
}
