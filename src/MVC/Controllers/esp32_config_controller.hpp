#ifndef _ESP32__CONTROLLER_CONFIG_H
#define _ESP32__CONTROLLER_CONFIG_H
#include <WiFi.h>
#include <HTTPRequest.hpp>
#include <HTTPResponse.hpp>

#include "string_helper.h"
#include "System/CORE/esp32_base_controller.hpp"
#include <System/AUTH/CERT/esp32_cert_base.hpp>
#include <System/CORE/esp32_server.h>

using namespace httpsserver;
extern esp32_server server;
extern const char* PUBLIC_TEMP_PATH;
extern const char* PRIVATE_TEMP_PATH;
//void (esp32_config_controller::*getAvailableWifi)(HTTPRequest* request, HTTPResponse* response) = NULL;
class esp32_config_controller : public esp32_base_controller {
public:
    inline void Index(HTTPRequest* request, HTTPResponse* response);
    inline virtual bool isIndexImplemented(){ return true;}

    inline void Post(HTTPRequest* request, HTTPResponse* response);
    inline virtual bool isPostImplemented(){ return true;}

	// inline virtual void List(HTTPRequest* request, HTTPResponse* response);
    virtual void GetAvailableWifi(HTTPRequest* request, HTTPResponse* response);
    virtual void LoadConfigData(HTTPRequest* request, HTTPResponse* response);
    virtual bool SaveConfigData(HTTPRequest* request, HTTPResponse* response);
    virtual void ResetDevice(HTTPRequest* request, HTTPResponse* response);
    virtual void UploadCertificate(HTTPRequest * request, HTTPResponse * response);
    virtual void GenerateCertificate(HTTPRequest *request, HTTPResponse * response);
    virtual void Backup(HTTPRequest *request, HTTPResponse * response);
    virtual void Restore(HTTPRequest *request, HTTPResponse * response);
    
    virtual esp32_controller_category GetCategory(){
        return esp32_controller_category::Site;
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
    //TODO: use reg object to maintain list of custom action names and pointer to methods. 
    //      add logic to call action through DerivedController object
    //vector<pair<string,void(esp32_config_controller::*)(HTTPRequest* request, HTTPResponse* response) >> actions;
};

//void (esp32_config_controller::*method)(HTTPRequest* request, HTTPResponse* response) const = NULL;
#endif