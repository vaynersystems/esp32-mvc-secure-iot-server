#ifndef _ESP32__CONTROLLER_CONFIG_H
#define _ESP32__CONTROLLER_CONFIG_H
#include <HTTPRequest.hpp>
#include <HTTPResponse.hpp>
#include "System/CORE/base_controller.hpp"

using namespace httpsserver;

//void (esp32_config_controller::*getAvailableWifi)(HTTPRequest* req, HTTPResponse* res) = NULL;
class esp32_config_controller : public Base_Controller {
public:
    inline virtual void Index(HTTPRequest* req, HTTPResponse* res);
    inline virtual bool isIndexImplemented(){ return true;}


	// inline virtual void List(HTTPRequest* req, HTTPResponse* res);
    virtual void GetAvailableWifi(HTTPRequest* req, HTTPResponse* res);
    
    
protected:
    inline virtual void Action(HTTPRequest* req, HTTPResponse* res);

private:
	static DerivedController<esp32_config_controller> reg; //register the controller
};
#endif