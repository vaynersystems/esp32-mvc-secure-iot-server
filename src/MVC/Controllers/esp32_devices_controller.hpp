#ifndef _ESP32__CONTROLLER_DEVICES_H
#define _ESP32__CONTROLLER_DEVICES_H
#include <HTTPRequest.hpp>
#include <HTTPResponse.hpp>
#include <vector>
#include "System/CORE/esp32_base_controller.hpp"
#include "System/MODULES/DEVICES/esp32_devices.hpp"
class esp32_devices_controller: public esp32_base_controller{

public:
    inline virtual void Index(HTTPRequest* req, HTTPResponse* res);
    inline virtual bool isIndexImplemented(){ return true;}

    virtual esp32_controller_category GetCategory(){
        return esp32_controller_category::Devices;
    }
    virtual const char* GetName(){
        return "Devices";
    }
private:
	static DerivedController<esp32_devices_controller> reg; //register the controller
    
};

#endif;