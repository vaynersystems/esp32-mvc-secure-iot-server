#ifndef _ESP32__CONTROLLER__FOOTER_H
#define _ESP32__CONTROLLER__FOOTER_H
#include <HTTPRequest.hpp>
#include <HTTPResponse.hpp>
#include "System/CORE/base_controller.hpp"
#include "System/ROUTER/esp32_template.h"

using namespace httpsserver;
class _footer : public Base_Controller {
public:
	inline virtual void Index(HTTPRequest* req, HTTPResponse* res);

private:
	static DerivedController<_footer> reg; //register the controller
};
#endif