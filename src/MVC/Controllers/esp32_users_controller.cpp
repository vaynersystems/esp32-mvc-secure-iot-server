#include "request_body.h"
#include "esp32_users_controller.hpp"
#include <strings.h>

// ---- validation and safety helpers -------------------------------------------------------
static bool validUsername(const char* name){
    if(name == nullptr) return false;
    size_t n = strlen(name);
    if(n < 3 || n > 32) return false;
    for(size_t i = 0; i < n; i++){
        char c = name[i];
        if(!(isalnum((unsigned char)c) || c == '.' || c == '_' || c == '-')) return false;
    }
    return true;
}
static bool validRole(const char* role){
    return role != nullptr && (strcmp(role, "ADMIN") == 0 || strcmp(role, "USER") == 0);
}
static bool isEnabledAdmin(JsonObject user){
    return strcmp(user["role"] | "", "ADMIN") == 0 && (user["enabled"] | true);
}
static int enabledAdminCount(JsonArray users){
    int count = 0;
    for(JsonObject user : users) if(isEnabledAdmin(user)) count++;
    return count;
}
// Escape JSON for a single-quoted JavaScript string in a page ('$_USER_LIST').
static string escapeForJsString(const string& json){
    string out;
    out.reserve(json.size() + 16);
    for(char c : json){
        if(c == '\\') out += "\\\\";
        else if(c == '\'') out += "\\'";
        else if(c == '<') out += "\\x3c";
        else if(c == '\n') out += "\\n";
        else if(c == '\r') out += "\\r";
        else out += c;
    }
    return out;
}
// Users document shared by the Load* helpers. It used to be a local document whose
// contents were returned after it was freed; requests are handled one at a time, so a
// single long-lived document is safe.
static DynamicJsonDocument& usersDocument(){
    static DynamicJsonDocument doc(8192);
    return doc;
}


DerivedController<esp32_users_controller> esp32_users_controller::reg("esp32_users");

void esp32_users_controller::Index(HTTPRequest* req, HTTPResponse* res) {    
    if(strcmp(req->getHeader(HEADER_GROUP).c_str(),"ADMIN") != 0)
    {
        res->setStatusCode(401);
        return;
    }
    
    title = "User Listing";

    JsonArray users = LoadUsers();
    string configData;
    serializeJson(users, configData); 
    
    controllerTemplate.SetTemplateVariable(F("$_USER_LIST"), escapeForJsString(configData).c_str());   
    esp32_base_controller::Index(req,res);    
}

void esp32_users_controller::List(HTTPRequest* req, HTTPResponse* res) {
    if(strcmp(req->getHeader(HEADER_GROUP).c_str(),"ADMIN") != 0)
    {
        res->setStatusCode(401);
        return;
    }
    
    title = "User Listing";

    JsonArray users = LoadUsers();
    string configData;
    serializeJson(users, configData); 
    res->print(configData.c_str());
    res->setStatusCode(200);
    res->setStatusText("OK");
    res->setHeader("Content-Type", "application/json");
}

inline void esp32_users_controller::Delete(HTTPRequest *req, HTTPResponse *res)
{
     if(strcmp(req->getHeader(HEADER_GROUP).c_str(),"ADMIN") != 0)
    {
        res->setStatusCode(401);
        return;
    }
    
    const int length = req->getContentLength();    
    DynamicJsonDocument doc(length * 2);
    string content;
    
    // Read the whole body (it can arrive after the headers over HTTPS).
    if(!readRequestBody(req, content)){
        res->setStatusCode(400);
        res->setStatusText("Incomplete request body");
        return;
    }
    //Serial.printf("Creating user from data: %s...\n", content.c_str());
    auto error = deserializeJson(doc, content);
    if(error != DeserializationError::Ok){
        res->setStatusCode(500);
        res->setStatusText(error.c_str());
        return;
    }

    
    const char* username = doc["username"] | (const char*)nullptr;
    if(username == nullptr){
        res->setStatusCode(400);
        res->setStatusText("Missing username");
        return;
    }
    auto deleted = DeleteUser(username);

    if(!deleted){
        res->setStatusCode(409); 
        res->setStatusText("User not found, or it is the last administrator");
    } else
        res->setStatusCode(200);
    return;
}

// void esp32_users_controller::Post(HTTPRequest* req, HTTPResponse* res) {
    
// }

bool esp32_users_controller::CreateUser(HTTPRequest* req, HTTPResponse* res){
    if(strcmp(req->getHeader(HEADER_GROUP).c_str(),"ADMIN") != 0)
    {
        res->setStatusCode(401);
        return false;
    }
    const int length = req->getContentLength();    
    DynamicJsonDocument doc(length * 2);
    string content;
    
    // Read the whole body (it can arrive after the headers over HTTPS).
    if(!readRequestBody(req, content)){
        res->setStatusCode(400);
        res->setStatusText("Incomplete request body");
        return false;
    }
    //Serial.printf("Creating user from data: %s...\n", content.c_str());
    auto error = deserializeJson(doc, content);
    if(error != DeserializationError::Ok){
        res->setStatusCode(500);
        res->setStatusText(error.c_str());
        return false;
    }

    const char* username = doc["username"] | (const char*)nullptr;
    const char* password = doc["password"] | (const char*)nullptr;
    const char* role = doc["role"] | "USER";
    if(!validUsername(username)){
        res->setStatusCode(400);
        res->setStatusText("Username must be 3-32 letters, digits, '.', '_' or '-'");
        return false;
    }
    if(password == nullptr || strlen(password) < 8 || strlen(password) > 64){
        res->setStatusCode(400);
        res->setStatusText("Password must be 8-64 characters");
        return false;
    }
    if(!validRole(role)){
        res->setStatusCode(400);
        res->setStatusText("Role must be ADMIN or USER");
        return false;
    }
    auto saved = esp32_authentication::registerUser(username, password, role, doc["enabled"] | true);

    if(!saved){
        res->setStatusCode(409);            
        res->setStatusText("User already exists");
        return false;
    }

    res->setStatusCode(200);
    return true;
    
    

}
 
bool esp32_users_controller::UpdateUser(HTTPRequest* req, HTTPResponse* res){
    if(strcmp(req->getHeader(HEADER_GROUP).c_str(),"ADMIN") != 0)
    {
        res->setStatusCode(401);
        return false;
    }
    const int length = req->getContentLength();    
    DynamicJsonDocument doc(length * 2);
    string content;
    
    // Read the whole body (it can arrive after the headers over HTTPS).
    if(!readRequestBody(req, content)){
        res->setStatusCode(400);
        res->setStatusText("Incomplete request body");
        return false;
    }
    //Serial.printf("Updating user from data: %s...\n", content.c_str());
    auto error = deserializeJson(doc, content);
    if(error != DeserializationError::Ok){
        res->setStatusCode(500);
        res->setStatusText(error.c_str());
        return false;
    }

    const char* username = doc["username"] | (const char*)nullptr;
    const char* role = doc["role"] | (const char*)nullptr;
    if(username == nullptr || !validRole(role)){
        res->setStatusCode(400);
        res->setStatusText("A username and a role of ADMIN or USER are required");
        return false;
    }
    auto saved = SaveExistingUserData(username, role, doc["enabled"] | true);

    if(!saved){
        res->setStatusCode(409);
        res->setStatusText("User not found, or this would leave no enabled administrator");
        return false;
    }
    res->setStatusCode(200);
    return true;
}
 
void esp32_users_controller::ChangePassword(HTTPRequest* req, HTTPResponse* res){
    if(strcmp(req->getHeader(HEADER_GROUP).c_str(),"ADMIN") != 0)
    {
        res->setStatusCode(401);
        return;
    }
    const int length = req->getContentLength();    
    DynamicJsonDocument doc(length * 2);
    string content;
    
    // Read the whole body (it can arrive after the headers over HTTPS).
    if(!readRequestBody(req, content)){
        res->setStatusCode(400);
        res->setStatusText("Incomplete request body");
        return;
    }
    //Serial.printf("Updating user from data: %s...\n", content.c_str());
    auto error = deserializeJson(doc, content);
    if(error != DeserializationError::Ok){
        res->setStatusCode(500);
        res->setStatusText(error.c_str());
        return;
    }
    //serializeJson(doc,Serial);
    // (This check used to end in a stray ';' so it did nothing, and a missing field then built a
    // std::string from NULL and crashed the device.)
    const char* usernameField = doc["username"] | (const char*)nullptr;
    const char* oldField = doc["oldPassword"] | (const char*)nullptr;
    const char* newField = doc["newPassword"] | (const char*)nullptr;
    if(usernameField == nullptr || oldField == nullptr || newField == nullptr){
        res->setStatusCode(400);
        res->setStatusText("username, oldPassword and newPassword are required");
        return;
    }
    if(strlen(newField) < 8 || strlen(newField) > 64){
        res->setStatusCode(400);
        res->setStatusText("Password must be 8-64 characters");
        return;
    }
    string username = usernameField;
    string oldPassword = oldField;
    string newPassword = newField;
    auto saved = esp32_authentication::changePassword(
        username.c_str(), 
        oldPassword.c_str(), 
        newPassword.c_str()
    );

    switch(saved){
        case ChangePasswordResult::BadPasswordFormat:
            res->setStatusCode(400);            
            res->setStatusText("Bad Password Format");
        break;           
        case ChangePasswordResult::SamePassword:
            res->setStatusCode(400);            
            res->setStatusText("Old and new passwords must be different");
        break;            
        case ChangePasswordResult::WrongPassword:
            res->setStatusCode(401);            
            res->setStatusText("Incorrect password provided.");
        break;

        case ChangePasswordResult::Ok:
            res->setStatusCode(200);
        break;
    }
    
}



/// @brief Overwrite Action since we have custom action implemented
/// @param req 
/// @param res 
void esp32_users_controller::Action(HTTPRequest* req, HTTPResponse* res) {
    if (route.action.compare("ChangePassword") == 0) {
        ChangePassword(req,res);
    }
    else if (route.action.compare("CreateUser") == 0) {
        CreateUser(req,res);
    } 
    else if (route.action.compare("UpdateUser") == 0) {
        UpdateUser(req,res);
    }   
    else
        esp32_base_controller::Action(req,res);
}

bool esp32_users_controller::HasAction(const char * action){
    if (strcmp(action, "ChangePassword") == 0) {
        return true;
    }
    
    else if (strcmp(action, "CreateUser") == 0) {
        return true;
    }
    else if (strcmp(action, "UpdateUser") == 0) {
        return true;
    }
    
    else
        return esp32_base_controller::HasAction(action);
}

// @brief Save user data to disk
/// @param username 
/// @return 
bool esp32_users_controller::SaveExistingUserData(const char* username,const char* role, bool enabled){
    auto drive = filesystem.getDisk(SYSTEM_DRIVE);
    if (!drive->exists(PATH_AUTH_FILE)) return false;
    File file = drive->open(PATH_AUTH_FILE, "r");
    DynamicJsonDocument doc(file.size() * 2 + 1024);
    DeserializationError error = deserializeJson(doc,file);
    file.close();
    if (error) return false;

    JsonArray users = doc.as<JsonArray>();
    auto existingUser = esp32_authentication::findUser(users, username);
    if(existingUser.isNull()) return false;

    // Never leave the device without an enabled administrator (nobody could log in to fix it).
    bool stillAdmin = strcmp(role, "ADMIN") == 0 && enabled;
    if(isEnabledAdmin(existingUser) && !stillAdmin && enabledAdminCount(users) <= 1) return false;

    existingUser["role"] = role;
    existingUser["enabled"] = enabled;
    if(doc.overflowed()) return false;
    file = drive->open(PATH_AUTH_FILE,"w");
    serializeJson(doc, file);
    file.close();
    return true;
}

bool esp32_users_controller::DeleteUser(const char *username)
{
    if(username == nullptr) return false;
    auto drive = filesystem.getDisk(SYSTEM_DRIVE);
    File file = drive->open(PATH_AUTH_FILE,"r");
    DynamicJsonDocument doc(file.size() * 2 + 1024);   
    auto error = deserializeJson(doc,file);
    file.close();
    if(error != DeserializationError::Ok) return false;

    JsonArray users = doc.as<JsonArray>();
    JsonObject target = esp32_authentication::findUser(users, username);
    if(target.isNull()) return false;
    // Deleting the last enabled administrator would lock everyone out.
    if(isEnabledAdmin(target) && enabledAdminCount(users) <= 1) return false;

    for(size_t idx = 0; idx < users.size(); idx++){
        if(users[idx] == target){
            users.remove(idx);
            break;
        }
    }
    file = drive->open(PATH_AUTH_FILE,"w");
    serializeJson(doc, file);
    file.close();
    return true;
}

    
    

/// @brief Load user information from json file on disk
JsonVariant esp32_users_controller::LoadUsers() {
    auto drive = filesystem.getDisk(SYSTEM_DRIVE);
    File f = drive->open(PATH_AUTH_FILE,"r");
    DynamicJsonDocument& doc = usersDocument();
    doc.clear();
    StaticJsonDocument<128> filter; //filter out password and session fields
    filter[0]["username"] = true;
    filter[0]["role"] = true;
    filter[0]["enabled"] = true;
    filter[0]["created"] = true;

    auto error = deserializeJson(doc,f,  DeserializationOption::Filter(filter));
    f.close();
    if(error != DeserializationError::Ok){
        doc.clear();
        return doc.to<JsonArray>();
    }
    return doc.as<JsonArray>();
}



JsonVariant esp32_users_controller::LoadUserData(const char* username) {
    JsonArray users = LoadUsers();
    return esp32_authentication::findUser(users, username);
}