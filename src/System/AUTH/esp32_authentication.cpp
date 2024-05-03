#include "esp32_authentication.h"
#include "system_helper.h"

esp32_user_auth_info esp32_authentication::authenticateUser(const char* username, const char* password){
    esp32_user_auth_info info;
    bool firstUser = false;
    // Read the file
    auto filename = std::string(PATH_AUTH_FILE);
    // Check if the file exists
    if (!SPIFFS.exists(filename.c_str()))
    {     
        Serial.println("Authorization file does not exist. Creating"); 
        firstUser = true; //if first user, authenticate and create file     
        bool registered = registerUser(username, password, "ADMIN"); 
        if(registered){
            info.username = username;
            info.password = password;
            info.role = "ADMIN";
            info.authenticated =registered;
        }
       
    } else { // check if valid user
        File file = SPIFFS.open(filename.c_str());
        StaticJsonDocument<512> d;
        DeserializationError error = deserializeJson(d,file);        
        if (error) {
            Serial.print("deserializeJson() failed: ");
            Serial.println(error.c_str());
            return info;
        }
        //see if we have a matching entry
        for(JsonObject entry : d.as<JsonArray>()) {        
            //Serial.printf("Comparing %s with password %s\n", entry["username"].as<std::string>().c_str(), entry["password"].as<std::string>().c_str());
            if(strcmp(entry["username"].as<const char *>(),username) == 0 && strcmp(entry["password"].as<const char *>(), password) == 0 ){             
                info.username = username;
                info.password = password;
                info.role =  entry["role"].as<const char *>();
                info.authenticated = true;
                break;            
            }
        }  

        file.close();
    }
    return info;

    
}
bool esp32_authentication::registerUser(const char* username, const char* password, const char* role){
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
    newUser["created"] =   getCurrentTime();
    
    
    authFile = SPIFFS.open(PATH_AUTH_FILE, "w");
    serializeJson(doc, authFile);
    authFile.flush();
    authFile.close();
    return true;
}

bool esp32_authentication::changePassword(const char* username, const char* oldPassword, const char* newPassword){
    //Read auth data
    File authFile = SPIFFS.open(PATH_AUTH_FILE,"r");    
    
    DynamicJsonDocument doc(1024); 
    DeserializationError error = deserializeJson(doc, authFile);
    authFile.close();
    if(error){
        Serial.printf("Error occured deserializing authorization file: [%i]%s\n", error.code(), error.c_str()); 
        return false;     
    }

    JsonVariant existingUser = findUser(doc.as<JsonArray>(),username);
    if(existingUser.isNull()){
        return false; //user not found
    }

    //check old password
    if(strcmp(existingUser["password"].as<const char*>(), oldPassword) != 0)
        return false;

    //change password
    existingUser["password"] = newPassword;

    //write back
    authFile = SPIFFS.open(PATH_AUTH_FILE, "w");
    serializeJson(doc, authFile);
    authFile.flush();
    authFile.close();
    return true;
}



// JsonVariant esp32_authentication::findNestedKey(JsonObject obj, const char* key) {
//     JsonVariant foundObject = obj[key];
//     if (!foundObject.isNull())
//         return foundObject;

//     for (JsonPair pair : obj) {
//         JsonVariant nestedObject = findNestedKey(pair.value(), key);
//         if (!nestedObject.isNull())
//             return nestedObject;
//     }

//     return JsonVariant();
// }

JsonObject esp32_authentication::findUser(JsonArray users, const char* userName){
    for(JsonObject seekingUser : users){
        if(strcmp(seekingUser["username"].as<const char *>(),userName) == 0)
            return seekingUser;        
    }
    return JsonObject();
}