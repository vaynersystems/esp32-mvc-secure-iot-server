#ifndef _ESP32__CONTROLLER_DEVICES_H
#define _ESP32__CONTROLLER_DEVICES_H
#include <HTTPRequest.hpp>
#include <HTTPResponse.hpp>
#include <vector>
#include "System/CORE/esp32_base_controller.hpp"
#include "System/MODULES/DEVICES/esp32_devices.hpp"
#include "System/MODULES/PINS/esp32_pin_manager.hpp"
class esp32_devices_controller: public esp32_base_controller{

public:
    inline virtual void Index(HTTPRequest* request, HTTPResponse* response);
    inline virtual bool isIndexImplemented(){ return true;}

    inline void Post(HTTPRequest* request, HTTPResponse* response);
    inline virtual bool isPostImplemented(){ return true;}

    virtual void LoadDeviceData(HTTPRequest* request, HTTPResponse* response);
    virtual bool SaveDeviceData(HTTPRequest* request, HTTPResponse* response);
    virtual void ResetDevice(HTTPRequest* request, HTTPResponse* response);

    virtual esp32_controller_category GetCategory(){
        return esp32_controller_category::Devices;
    }
    virtual const char* GetName(){
        return "Management";
    }
    virtual bool Authorized(HTTPRequest* req){
        return strcmp(req->getHeader(HEADER_GROUP).c_str(), "ADMIN") == 0;
    }
protected:
    inline virtual void Action(HTTPRequest* request, HTTPResponse* response);
    inline virtual bool HasAction(const char * action);
private:
	static DerivedController<esp32_devices_controller> reg; //register the controller
    
};

#endif;