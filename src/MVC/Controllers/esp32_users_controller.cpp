#include "esp32_users_controller.hpp"


DerivedController<esp32_users_controller> esp32_users_controller::reg("esp32_users");

void esp32_users_controller::Index(HTTPRequest* req, HTTPResponse* res) {    
    if(strcmp(req->getHeader(HEADER_GROUP).c_str(),"ADMIN") != 0)
    {
        res->setStatusCode(401);
        return;
    }
    StaticJsonDocument<1024> doc;
    title = "User Listing";

    JsonArray users = LoadUsers();
    //doc.set(users);

    string configData;
    serializeJson(users, configData); 
    
    controllerTemplate.SetTemplateVariable("$_USER_LIST", configData.c_str());   
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
    DynamicJsonDocument doc(1024);
    doc.set(users);

    string configData;
    serializeJson(doc, configData);
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
    
    char * buf = new char[32];
    while(true){
        
        int bytesRead = req->readBytes((byte*)buf,32); 
        if(bytesRead <= 0) break;       
        content.append(buf,bytesRead);
    }
    delete[] buf;
    //Serial.printf("Creating user from data: %s...\n", content.c_str());
    auto error = deserializeJson(doc, content);
    if(error != DeserializationError::Ok){
        res->setStatusCode(500);
        res->setStatusText(error.c_str());
        return;
    }

    
    auto deleted = DeleteUser(
        doc["username"].as<const char*>()
    );

    if(!deleted){
        res->setStatusCode(500); 
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
    
    char * buf = new char[32];
    while(true){
        
        int bytesRead = req->readBytes((byte*)buf,32); 
        if(bytesRead <= 0) break;       
        content.append(buf,bytesRead);
    }
    delete[] buf;
    //Serial.printf("Creating user from data: %s...\n", content.c_str());
    auto error = deserializeJson(doc, content);
    if(error != DeserializationError::Ok){
        res->setStatusCode(500);
        res->setStatusText(error.c_str());
        return false;
    }

    auto saved = esp32_authentication::registerUser(
        doc["username"].as<const char*>(), 
        doc["password"].as<const char*>(), 
        doc["role"].as<const char*>(), 
        doc["enabled"].as<bool>()
    );

    if(!saved){
        res->setStatusCode(500);            
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
    
    char * buf = new char[32];
    while(true){
        
        int bytesRead = req->readBytes((byte*)buf,32); 
        if(bytesRead <= 0) break;       
        content.append(buf,bytesRead);
    }
    delete[] buf;
    Serial.printf("Updating user from data: %s...\n", content.c_str());
    auto error = deserializeJson(doc, content);
    if(error != DeserializationError::Ok){
        res->setStatusCode(500);
        res->setStatusText(error.c_str());
        return false;
    }

    auto saved = SaveExistingUserData(
        doc["username"].as<const char*>(), 
        doc["role"].as<const char*>(), 
        doc["enabled"].as<bool>()
    );

    if(!saved){
        res->setStatusCode(500);
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
    
    char * buf = new char[32];
    while(true){
        
        int bytesRead = req->readBytes((byte*)buf,32    ); 
        if(bytesRead <= 0) break;       
        content.append(buf,bytesRead);
    }
    delete[] buf;
    //Serial.printf("Updating user from data: %s...\n", content.c_str());
    auto error = deserializeJson(doc, content);
    if(error != DeserializationError::Ok){
        res->setStatusCode(500);
        res->setStatusText(error.c_str());
        return;
    }
    //serializeJson(doc,Serial);
    if(!doc.containsKey("username") || !doc.containsKey("oldPassword") || !doc.containsKey("newPassword"));
    string username = doc["username"].as<const char*>();
    string oldPassword = doc["oldPassword"].as<const char*>();
    string newPassword = doc["newPassword"].as<const char*>();
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

    esp32_user_auth_info info;
    bool firstUser = false;
    // Read the file
    auto filename = std::string(PATH_AUTH_FILE);
    // Check if the file exists
    if (!SPIFFS.exists(filename.c_str()))
    {     
        return false;
       
    }
    //get user object, update it, store back
    File file = SPIFFS.open(filename.c_str());
    StaticJsonDocument<512> d;
    DeserializationError error = deserializeJson(d,file);
    file.close();

    if (error) {
        Serial.print("deserializeJson() failed: ");
        Serial.println(error.c_str());
        return false;
    }
    auto existingUser = esp32_authentication::findUser(d.as<JsonArray>(), username);
    if(existingUser.isNull()) return false;
    
    existingUser["role"] = role;
    existingUser["enabled"] = enabled;
    file = SPIFFS.open(filename.c_str(),"w");
    serializeJson(d, file);
    file.close();
        
    
    return true;
}

bool esp32_users_controller::DeleteUser(const char *username)
{
    File file = SPIFFS.open(PATH_AUTH_FILE,"r");
    DynamicJsonDocument doc(file.size() * 2);   

    auto error = deserializeJson(doc,file);
    file.close();
    if(error != DeserializationError::Ok){
        Serial.printf("Error occured deserializing user data: %s\n", error.c_str());
        return false;
    }
    auto users =  doc.as<JsonArray>();
    for(int idx = 0; idx < users.size();idx++){
        if(strcmp(users[idx]["username"].as<const char *>(),username) == 0){
            users.remove(idx);
        }
    }
    
    file = SPIFFS.open(PATH_AUTH_FILE,"w");
    serializeJson(doc, file);
    file.close();

    return true;
}

    
    

/// @brief Load user information from json file on disk
JsonVariant esp32_users_controller::LoadUsers() {
    File f = SPIFFS.open(PATH_AUTH_FILE,"r");
    DynamicJsonDocument doc(f.size() * 2);
    StaticJsonDocument<128> filter; //filter out password field
    filter[0]["username"] = true;
    filter[0]["role"] = true;
    filter[0]["enabled"] = true;
    filter[0]["created"] = true;


    auto error = deserializeJson(doc,f,  DeserializationOption::Filter(filter));
    f.close();
    if(error != DeserializationError::Ok){
        Serial.printf("Error occured deserializing user data: %s\n", error.c_str());
        return JsonArray();
    }
    //serializeJson(doc, Serial);
    return doc.as<JsonArray>();
}



JsonVariant esp32_users_controller::LoadUserData(const char* username) {
    File f = SPIFFS.open(PATH_AUTH_FILE,"r");
    DynamicJsonDocument doc(f.size() * 2);
    StaticJsonDocument<128> filter;
    filter[0]["username"] = true;
    filter[0]["role"] = true;
    filter[0]["enabled"] = true;
    filter[0]["created"] = true;


    auto error = deserializeJson(doc,f,  DeserializationOption::Filter(filter));
    f.close();
    if(error != DeserializationError::Ok){
        Serial.printf("Error occured deserializing user data: %s\n", error.c_str());
        return JsonObject();
    }

   

    //serializeJson(doc, Serial);
    auto user = esp32_authentication::findUser(doc.as<JsonArray>(), username);
    return user;
}