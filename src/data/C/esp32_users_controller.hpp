#ifndef _ESP32__CONTROLLER_USERS_H
#define _ESP32__CONTROLLER_USERS_H
#include "string"
#include <HTTPRequest.hpp>
#include <HTTPResponse.hpp>
#include "System/CORE/base_controller.hpp"
#include "ArduinoJson.h"
#include "system_helper.h"

#include "System/AUTH/esp32_authentication.h"
using namespace std;
using namespace httpsserver;

enum ChangePasswordResult{
    WrongPassword = 0,
    BadPasswordFormat = 1,
    SamePassword = 2,
    Ok = 3,
    AuthSystemError = 4,
};

//void (esp32_config_controller::*getAvailableWifi)(HTTPRequest* req, HTTPResponse* res) = NULL;
class esp32_users_controller : public Base_Controller {
public:
    inline void Index(HTTPRequest* req, HTTPResponse* res);
    inline virtual bool isIndexImplemented(){ return true;}

    inline void List(HTTPRequest* req, HTTPResponse* res);
    inline virtual bool isListImplemented(){ return true;}


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
    JsonObject findUser(JsonArray users, const char* userName);
    virtual JsonVariant LoadUsers();
    virtual JsonVariant LoadUserData(const char* username);
    virtual bool SaveNewUserData(const char* username,const char * password, const char* role, bool enabled);
    virtual bool SaveExistingUserData(const char* username,const char* role, bool enabled);
    virtual ChangePasswordResult SaveUserPassword(const char* username, const char* oldPassword, const char* newPassword);
};

#endif