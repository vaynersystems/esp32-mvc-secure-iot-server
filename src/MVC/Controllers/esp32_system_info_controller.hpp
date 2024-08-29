#ifndef _ESP32__CONTROLLER_SYSTEM_INFO_H
#define _ESP32__CONTROLLER_SYSTEM_INFO_H
#include <HTTPRequest.hpp>
#include <HTTPResponse.hpp>
#include "System/CORE/esp32_base_controller.hpp"
#include "System/ROUTER/esp32_template.h"

using namespace httpsserver;
class esp32_system_info_controller : public esp32_base_controller {
public:
	inline virtual void Index(HTTPRequest* req, HTTPResponse* res);
    inline virtual bool isIndexImplemented(){ return true;}

    virtual esp32_controller_category GetCategory(){
        return esp32_controller_category::Site;
    }
    virtual const char* GetName(){
        return "System Information";
    }

private:
	static DerivedController<esp32_system_info_controller> reg; //register the controller
    void prettyFlashModeString(string &flashMode);
};
#endif