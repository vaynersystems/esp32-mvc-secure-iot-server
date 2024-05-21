#ifndef _ESP32__CONTROLLER_CONFIG_H
#define _ESP32__CONTROLLER_CONFIG_H
#include <WiFi.h>
#include <HTTPRequest.hpp>
#include <HTTPResponse.hpp>

#include "string_extensions.h"
#include "System/CORE/esp32_base_controller.hpp"
#include <System/AUTH/CERT/esp32_cert_base.hpp>
#include <System/CORE/esp32_server.h>

using namespace httpsserver;
extern esp32_server server;
extern const char* PUBLIC_TEMP_PATH;
extern const char* PRIVATE_TEMP_PATH;
//void (esp32_config_controller::*getAvailableWifi)(HTTPRequest* req, HTTPResponse* res) = NULL;
class esp32_config_controller : public esp32_base_controller {
public:
    inline void Index(HTTPRequest* req, HTTPResponse* res);
    inline virtual bool isIndexImplemented(){ return true;}

    inline void Post(HTTPRequest* req, HTTPResponse* res);
    inline virtual bool isPostImplemented(){ return true;}

	// inline virtual void List(HTTPRequest* req, HTTPResponse* res);
    virtual void GetAvailableWifi(HTTPRequest* req, HTTPResponse* res);
    virtual void LoadConfigData(HTTPRequest* req, HTTPResponse* res);
    virtual bool SaveConfigData(HTTPRequest* req, HTTPResponse* res);
    virtual void ResetDevice(HTTPRequest* req, HTTPResponse* res);
    virtual void UploadCertificate(HTTPRequest * req, HTTPResponse * res);
    virtual void GenerateCertificate(HTTPRequest *req, HTTPResponse * res);
    
    
protected:
    inline virtual void Action(HTTPRequest* req, HTTPResponse* res);
    inline virtual bool HasAction(const char * action);

private:
	static DerivedController<esp32_config_controller> reg; //registry for the controller
    //TODO: use reg object to maintain list of custom action names and pointer to methods. 
    //      add logic to call action through DerivedController object
    //vector<pair<string,void(esp32_config_controller::*)(HTTPRequest* req, HTTPResponse* res) >> actions;
};

//void (esp32_config_controller::*method)(HTTPRequest* req, HTTPResponse* res) const = NULL;
#endif