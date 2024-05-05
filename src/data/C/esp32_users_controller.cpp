#include "esp32_users_controller.hpp"


DerivedController<esp32_users_controller> esp32_users_controller::reg("esp32_users");

void esp32_users_controller::Index(HTTPRequest* req, HTTPResponse* res) {    
    if(strcmp(req->getHeader(HEADER_GROUP).c_str(),"ADMIN") != 0)
    {
        res->setStatusCode(401);
        return;
    }
    StaticJsonDocument<2048> doc;
    title = "User Listing";

    JsonArray users = LoadUsers();
    doc.set(users);

    string configData;
    serializeJson(doc, configData); 
    //serializeJson(doc, Serial); 
    //Serial.print("."); //seems some statement has to go here, otherwise the string is not loaded   
    
    controllerTemplate.SetTemplateVariable("$_USER_LIST", configData.c_str());   
    Base_Controller::Index(req,res);    
}

void esp32_users_controller::List(HTTPRequest* req, HTTPResponse* res) {
    if(strcmp(req->getHeader(HEADER_GROUP).c_str(),"ADMIN") != 0)
    {
        res->setStatusCode(401);
        return;
    }
    StaticJsonDocument<2048> doc;
    title = "User Listing";

    JsonArray users = LoadUsers();
    doc.set(users);

    string configData;
    serializeJson(doc, configData);
    res->print(configData.c_str());
    res->setStatusCode(200);
    res->setStatusText("OK");
    res->setHeader("Content-Type", "text/html");
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

    auto saved = SaveNewUserData(
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
    serializeJson(doc,Serial);
    if(!doc.containsKey("username") || !doc.containsKey("oldPassword") || !doc.containsKey("newPassword"));
    string username = doc["username"].as<const char*>();
    string oldPassword = doc["oldPassword"].as<const char*>();
    string newPassword = doc["newPassword"].as<const char*>();
    auto saved = SaveUserPassword(
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
        Base_Controller::Action(req,res);
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
        return Base_Controller::HasAction(action);
}



/// @brief Save user data to disk
/// @param username 
/// @return 
bool esp32_users_controller::SaveNewUserData(const char* username,const char * password, const char* role, bool enabled){
    File authFile = SPIFFS.open(PATH_AUTH_FILE,"r");    
    
    DynamicJsonDocument doc(1024); 
    DeserializationError error = deserializeJson(doc, authFile);
    authFile.close();

    if(error){
        Serial.printf("Error occured deserializing authorization file: [%i]%s\n", error.code(), error.c_str()); 
        //SPIFFS.remove(PATH_AUTH_FILE);
    }

    JsonVariant existingUser = findUser(doc.as<JsonArray>(),username);

    if(!existingUser.isNull()){
        return false;
    }
    

    JsonObject newUser = doc.createNestedObject();
    newUser["username"] = username;
    newUser["password"] = password;
    newUser["role"] = role;
    newUser["enabled"] = true;
    newUser["created"] = getCurrentTime();
    
    
    authFile = SPIFFS.open(PATH_AUTH_FILE, "w");
    serializeJson(doc, authFile);
    authFile.flush();
    authFile.close();
    return true;
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
    //see if we have a matching entry
    for(JsonObject entry : d.as<JsonArray>()) {        
        //Serial.printf("Comparing %s with password %s\n", entry["username"].as<std::string>().c_str(), entry["password"].as<std::string>().c_str());
        if(strcmp(entry["username"].as<const char *>(),username) == 0){             
            info.role = role;
            info.enabled = enabled;
            break;
        }
    }  
    file = SPIFFS.open(filename.c_str(),"w");
    serializeJson(d, file);
    file.close();
        
    
    return true;
}

ChangePasswordResult esp32_users_controller::SaveUserPassword(const char* username, const char* oldPassword, const char* newPassword){
    
    Serial.printf("Request to save password for user %s.\n\t old: %s new: ...\n", username,oldPassword,newPassword);
     if(strcmp(oldPassword,newPassword) == 0)
        return ChangePasswordResult::SamePassword;
    // Read the file
    auto filename = std::string(PATH_AUTH_FILE);
    // Check if the file exists
    if (!SPIFFS.exists(filename.c_str()))
    {     
        return ChangePasswordResult::AuthSystemError;
       
    }
    //get user object, update it, store back
    File file = SPIFFS.open(filename.c_str());
    StaticJsonDocument<512> d;
    DeserializationError error = deserializeJson(d,file);
    file.close();

    if (error) {
        Serial.print("deserializeJson() failed: ");
        Serial.println(error.c_str());
        return ChangePasswordResult::AuthSystemError;
    }
    //see if we have a matching entry
    for(JsonObject entry : d.as<JsonArray>()) {        
        // Serial.printf("Comparing %s with password %s to %s with password %s\n", 
        //     entry["username"].as<std::string>().c_str(), entry["password"].as<std::string>().c_str(),
        //     username,oldPassword);
        if(strcmp(entry["username"].as<string>().c_str(),username) == 0){
            if( strcmp(entry["password"].as<string>().c_str(), oldPassword) != 0)
                return ChangePasswordResult::WrongPassword;
             
            entry["password"] = string(newPassword);
            break;
        }
    }  
    file = SPIFFS.open(filename.c_str(),"w");
    serializeJson(d, file);
    file.close();
        
    
    return ChangePasswordResult::Ok;
}
    
    

/// @brief Load user information from json file on disk
JsonVariant esp32_users_controller::LoadUsers() {
    File f = SPIFFS.open(PATH_AUTH_FILE,"r");
    DynamicJsonDocument doc(f.size() * 2);
    StaticJsonDocument<64> filter; //filter out password field
    filter[0]["username"] = true;
    filter[0]["role"] = true;
    filter[0]["enabled"] = true;


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
    StaticJsonDocument<64> filter;
    filter[0]["username"] = true;
    filter[0]["role"] = true;
    filter[0]["enabled"] = true;


    auto error = deserializeJson(doc,f,  DeserializationOption::Filter(filter));
    f.close();
    if(error != DeserializationError::Ok){
        Serial.printf("Error occured deserializing user data: %s\n", error.c_str());
        return JsonObject();
    }

   

    serializeJson(doc, Serial);
    auto user = findUser(doc.as<JsonArray>(), username);
    return user;
}


JsonObject esp32_users_controller::findUser(JsonArray users, const char* userName){
    for(JsonObject seekingUser : users){
        //Serial.printf("Comparing user %s in db to user %s being searched\n", seekingUser["username"].as<const char *>(), userName);
        if(strcmp(seekingUser["username"].as<const char *>(),userName) == 0)
            return seekingUser;        
    }
    return JsonObject();
}    