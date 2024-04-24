#ifndef _ESP32__CONTROLLER_CUSTOM_TASK_MANAGER_H
#define _ESP32__CONTROLLER_CUSTOM_TASK_MANAGER_H
#include <HTTPRequest.hpp>
#include <HTTPResponse.hpp>
#include "System/CORE/base_controller.hpp"

using namespace httpsserver;
class custom_task_manager_controller : public Base_Controller {
public:
    inline virtual void Index(HTTPRequest* req, HTTPResponse* res);
    inline virtual bool isIndexImplemented(){ return true;}

	inline virtual void List(HTTPRequest* req, HTTPResponse* res);
    inline virtual bool isListImplemented(){ return true;}

    inline virtual void Put(HTTPRequest* req, HTTPResponse* res);
    inline virtual bool isPutImplemented(){ return true;}

    inline virtual void Post(HTTPRequest* req, HTTPResponse* res);
    inline virtual bool isPostImplemented(){ return true;}

    inline virtual void Delete(HTTPRequest* req, HTTPResponse* res);
    inline virtual bool isDeleteImplemented(){ return true;}

private:
	static DerivedController<custom_task_manager_controller> reg; //register the controller
};
#endif