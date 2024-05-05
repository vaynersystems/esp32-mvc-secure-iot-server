#ifndef _ESP32__CONTROLLER_LOGIN_H
#define _ESP32__CONTROLLER_LOGIN_H
#include <HTTPRequest.hpp>
#include <HTTPResponse.hpp>
#include "System/CORE/base_controller.hpp"

using namespace httpsserver;
class esp32_login_controller : public Base_Controller {
public:
    inline virtual void Index(HTTPRequest* req, HTTPResponse* res);
    inline virtual bool isIndexImplemented(){ return true;}
    inline virtual void Post(HTTPRequest* req, HTTPResponse* res);
    inline virtual bool isPostImplemented(){ return true;}

private:
	static DerivedController<esp32_login_controller> reg; //register the controller
};
#endif