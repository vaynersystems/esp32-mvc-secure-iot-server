#ifndef _ESP32__CONTROLLER_CONFIG_H
#define _ESP32__CONTROLLER_CONFIG_H
#include <WiFi.h>
#include <HTTPRequest.hpp>
#include <HTTPResponse.hpp>

#include "string_helper.h"
#include "System/CORE/esp32_base_controller.hpp"
#include "System/AUTH/CERT/esp32_cert_base.hpp"
#include "System/CORE/esp32_server.hpp"
#include "System/MODULES/OTA/esp32_ota_server.hpp"

using namespace httpsserver;
extern esp32_server server;
class esp32_config_controller : public esp32_base_controller {
public:
    inline void Index(HTTPRequest* request, HTTPResponse* response);
    inline virtual bool isIndexImplemented(){ return true;}

    inline void Post(HTTPRequest* request, HTTPResponse* response);
    inline virtual bool isPostImplemented(){ return true;}

    virtual void GetAvailableWifi(HTTPRequest* request, HTTPResponse* response);
    virtual void LoadConfigData(HTTPRequest* request, HTTPResponse* response);
    virtual bool SaveConfigData(HTTPRequest* request, HTTPResponse* response);
    virtual void ResetDevice(HTTPRequest* request, HTTPResponse* response);
    virtual void UploadCertificate(HTTPRequest * request, HTTPResponse * response);
    virtual void GenerateCertificate(HTTPRequest *request, HTTPResponse * response);
    virtual void UpdateFirmware(HTTPRequest * request, HTTPResponse * response);
    virtual void Backup(HTTPRequest *request, HTTPResponse * response);
    virtual void Restore(HTTPRequest *request, HTTPResponse * response);
    virtual void FactoryReset(HTTPRequest *request, HTTPResponse *response);
    
    virtual esp32_controller_category GetCategory(){
        return esp32_controller_category::System;
    }
    virtual const char* GetName(){
        return "Configuration";
    }
    virtual bool Authorized(HTTPRequest* req){
        return strcmp(req->getHeader(HEADER_GROUP).c_str(), "ADMIN") == 0;
    }
protected:
    inline virtual void Action(HTTPRequest* request, HTTPResponse* response);
    inline virtual bool HasAction(const char * action);

private:
	static DerivedController<esp32_config_controller> reg; //registry for the controller
    esp32_ota_server otaServer;
};

#endif