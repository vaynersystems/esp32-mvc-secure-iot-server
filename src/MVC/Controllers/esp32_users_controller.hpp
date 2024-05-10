#ifndef _ESP32__CONTROLLER_USERS_H
#define _ESP32__CONTROLLER_USERS_H
#include "string"
#include <HTTPRequest.hpp>
#include <HTTPResponse.hpp>
#include "System/CORE/esp32_base_controller.hpp"
#include "ArduinoJson.h"
#include "system_helper.h"

#include "System/AUTH/esp32_authentication.h"
using namespace std;
using namespace httpsserver;



//void (esp32_config_controller::*getAvailableWifi)(HTTPRequest* req, HTTPResponse* res) = NULL;
class esp32_users_controller : public esp32_base_controller {
public:
    inline void Index(HTTPRequest* req, HTTPResponse* res);
    inline virtual bool isIndexImplemented(){ return true;}

    inline void List(HTTPRequest* req, HTTPResponse* res);
    inline virtual bool isListImplemented(){ return true;}

    inline void Delete(HTTPRequest* req, HTTPResponse* res);
    inline virtual bool isDeleteImplemented(){ return true;}
    // inline void Post(HTTPRequest* req, HTTPResponse* res);
    // inline virtual bool isPostImplemented(){ return true;}

	
    virtual void ChangePassword(HTTPRequest* req, HTTPResponse* res);
    virtual bool CreateUser(HTTPRequest* req, HTTPResponse* res);    
    virtual bool UpdateUser(HTTPRequest* req, HTTPResponse* res);    
    
    
protected:
    inline virtual void Action(HTTPRequest* req, HTTPResponse* res);
    inline virtual bool HasAction(const char * action);

private:
	static DerivedController<esp32_users_controller> reg; //register the controller
    //JsonObject findUser(JsonArray users, const char* userName);
    virtual JsonVariant LoadUsers();
    virtual JsonVariant LoadUserData(const char* username);
    virtual bool SaveExistingUserData(const char* username,const char* role, bool enabled);
    virtual bool DeleteUser(const char* username);

    //virtual ChangePasswordResult SaveUserPassword(const char* username, const char* oldPassword, const char* newPassword);
};

#endif