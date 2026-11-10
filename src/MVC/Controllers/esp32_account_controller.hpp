#ifndef _ESP32__CONTROLLER_ACCOUNT_H
#define _ESP32__CONTROLLER_ACCOUNT_H
#include <HTTPRequest.hpp>
#include <HTTPResponse.hpp>
#include "System/CORE/esp32_base_controller.hpp"
#include "System/AUTH/esp32_authentication.hpp"

using namespace httpsserver;

/// @brief The signed-in user's own account. Any logged-in user (not just administrators) can
/// change their own password here; they must give their current password. Administrators set
/// other users' passwords on the Users page instead.
class esp32_account_controller : public esp32_base_controller {
public:
    void Index(HTTPRequest* req, HTTPResponse* res);
    bool isIndexImplemented(){ return true;}

    void ChangePassword(HTTPRequest* req, HTTPResponse* res);

    virtual esp32_controller_category GetCategory(){
        return esp32_controller_category::Account;
    }
    virtual const char* GetName(){
        return "Change Password";
    }
    // Any signed-in user. The authorization middleware sets the username only for a valid session.
    virtual bool Authorized(HTTPRequest* req){
        return req->getHeader(HEADER_USERNAME).length() > 0;
    }

protected:
    void Action(HTTPRequest* req, HTTPResponse* res);
    bool HasAction(const char * action);

private:
    static DerivedController<esp32_account_controller> reg; //register the controller
};
#endif
