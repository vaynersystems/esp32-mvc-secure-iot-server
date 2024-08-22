#ifndef _ESP32__CONTROLLER_LOGIN_H
#define _ESP32__CONTROLLER_LOGIN_H
#include <HTTPRequest.hpp>
#include <HTTPResponse.hpp>
#include "System/CORE/esp32_base_controller.hpp"

using namespace httpsserver;
class esp32_login_controller : public esp32_base_controller {
public:
    inline virtual void Index(HTTPRequest* req, HTTPResponse* res);
    inline virtual bool isIndexImplemented(){ return true;}
    inline virtual void Post(HTTPRequest* req, HTTPResponse* res);
    inline virtual bool isPostImplemented(){ return true;}
    virtual const char* GetName(){
        return "Login";
    }

private:
	static DerivedController<esp32_login_controller> reg; //register the controller
};
#endif