#ifndef _ESP32__CONTROLLER_esp32_dashboard_H
#define _ESP32__CONTROLLER_esp32_dashboard_H
#include <HTTPRequest.hpp>
#include <HTTPResponse.hpp>
#include "System/CORE/base_controller.hpp"
#include "System/ROUTER/esp32_template.h"

using namespace httpsserver;
class esp32_dashboard_controller : public Base_Controller {
public:
	inline virtual void Index(HTTPRequest* req, HTTPResponse* res);
    inline virtual bool isIndexImplemented(){ return true;}

private:
	static DerivedController<esp32_dashboard_controller> reg; //register the controller
};
#endif