#ifndef _ESP32__CONTROLLER_HISTORIC_H
#define _ESP32__CONTROLLER_HISTORIC_H
#include <HTTPRequest.hpp>
#include <HTTPResponse.hpp>
#include "System/CORE/esp32_base_controller.hpp"
#include "System/ROUTER/esp32_template.h"
#include "esp32_filesystem.hpp"
using namespace httpsserver;
extern esp32_file_system filesystem;
class esp32_historic_controller : public esp32_base_controller {
public:
	void Index(HTTPRequest* req, HTTPResponse* res);
    bool isIndexImplemented(){ return true;}

    void Logs(HTTPRequest* req, HTTPResponse* res);

    virtual esp32_controller_category GetCategory(){
        return esp32_controller_category::Devices;
    }
    virtual const char* GetName(){
        return "History";
    }

protected:
    void Action(HTTPRequest* req, HTTPResponse* res);
    bool HasAction(const char * action);    
    void GetActions(vector<string>* actions);

private:
	static DerivedController<esp32_historic_controller> reg; //register the controller
};
#endif