#include "esp32_account_controller.hpp"
#include "js_escape.h"
#include "request_body.h"
#include "string_helper.h"
#include "System/MODULES/LOGGING/esp32_logging.hpp"
extern esp32_logging logger;

DerivedController<esp32_account_controller> esp32_account_controller::reg("esp32_account");

void esp32_account_controller::Index(HTTPRequest* req, HTTPResponse* res) {
    title = "Account";
    controllerTemplate.SetTemplateVariable(F("$_USERNAME"), escape_for_js_string(req->getHeader(HEADER_USERNAME)).c_str());
    controllerTemplate.SetTemplateVariable(F("$_PASSWORD_MIN"), string_format("%d", PASSWORD_MIN_LENGTH).c_str());
    controllerTemplate.SetTemplateVariable(F("$_PASSWORD_MAX"), string_format("%d", PASSWORD_MAX_LENGTH).c_str());
    esp32_base_controller::Index(req, res);
}

/// @brief Change the signed-in user's own password. The username comes from the session, never
/// from the request body, so a user can only change their own password.
void esp32_account_controller::ChangePassword(HTTPRequest* req, HTTPResponse* res){
    if(req->getMethod() != "POST"){
        req->discardRequestBody();
        res->setStatusCode(405);
        return;
    }
    string username = req->getHeader(HEADER_USERNAME);
    if(username.empty()){
        req->discardRequestBody();
        res->setStatusCode(401);
        return;
    }

    string content;
    // Read the whole body (it can arrive after the headers over HTTPS).
    if(!readRequestBody(req, content, 512)){
        res->setStatusCode(400);
        res->setStatusText("Incomplete request body");
        return;
    }
    DynamicJsonDocument doc(content.length() * 2 + 64);
    auto error = deserializeJson(doc, content);
    memset(&content[0], 0, content.size());
    if(error != DeserializationError::Ok){
        res->setStatusCode(400);
        res->setStatusText(error.c_str());
        return;
    }
    const char* oldField = doc["oldPassword"] | (const char*)nullptr;
    const char* newField = doc["newPassword"] | (const char*)nullptr;
    if(oldField == nullptr || newField == nullptr || strlen(oldField) == 0){
        res->setStatusCode(400);
        res->setStatusText("Current and new passwords are required");
        return;
    }
    if(strlen(newField) < PASSWORD_MIN_LENGTH || strlen(newField) > PASSWORD_MAX_LENGTH){
        res->setStatusCode(400);
        res->setStatusText(string_format("Password must be %d-%d characters", PASSWORD_MIN_LENGTH, PASSWORD_MAX_LENGTH));
        return;
    }

    auto saved = esp32_authentication::changePassword(username.c_str(), oldField, newField);
    switch(saved){
        case ChangePasswordResult::Ok:
            logger.logInfo(string_format("%s changed their password", username.c_str()), auth);
            res->setStatusCode(200);
            break;
        case ChangePasswordResult::SamePassword:
            res->setStatusCode(400);
            res->setStatusText("The new password must be different from the current one");
            break;
        case ChangePasswordResult::BadPasswordFormat:
            res->setStatusCode(400);
            res->setStatusText("Bad Password Format");
            break;
        case ChangePasswordResult::WrongPassword:
            logger.logInfo(string_format("%s gave a wrong current password when changing it", username.c_str()), auth);
            // 403, not 401: the session is fine, only the current password was wrong.
            res->setStatusCode(403);
            res->setStatusText("Current password is incorrect");
            break;
        default:
            res->setStatusCode(500);
            res->setStatusText("Could not save the password");
            break;
    }
}

void esp32_account_controller::Action(HTTPRequest* req, HTTPResponse* res) {
    if (route.action.compare("ChangePassword") == 0)
        ChangePassword(req, res);
    else
        esp32_base_controller::Action(req, res);
}

bool esp32_account_controller::HasAction(const char * action){
    if (strcmp(action, "ChangePassword") == 0) return true;
    return esp32_base_controller::HasAction(action);
}
