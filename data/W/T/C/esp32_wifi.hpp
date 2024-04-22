#ifndef _ESP32__CONTROLLER_WIFI_H
#define _ESP32__CONTROLLER_WIFI_H
#include <HTTPRequest.hpp>
#include <HTTPResponse.hpp>
#include "System/CORE/base_controller.hpp"

using namespace httpsserver;
class esp32_wifi : public Base_Controller{
public:
	inline virtual void List(HTTPRequest* req, HTTPResponse* res);

private:
	static DerivedController<esp32_wifi> reg; //register the controller
};
#endif