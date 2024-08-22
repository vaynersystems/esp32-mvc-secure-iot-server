#ifndef _ESP32__CONTROLLER__FOOTER_H
#define _ESP32__CONTROLLER__FOOTER_H
#include <HTTPRequest.hpp>
#include <HTTPResponse.hpp>
#include "System/CORE/esp32_base_controller.hpp"
#include "System/ROUTER/esp32_template.h"

using namespace httpsserver;
class _footer_controller : public esp32_base_controller {
public:
	inline virtual void Index(HTTPRequest* req, HTTPResponse* res);
    inline virtual bool isIndexImplemented(){ return true;}

    virtual esp32_controller_category GetCategory(){
        return esp32_controller_category::_Internal;
    }
    virtual const char* GetName(){
        return "Footer";
    }

private:
	static DerivedController<_footer_controller> reg; //register the controller
};
#endif