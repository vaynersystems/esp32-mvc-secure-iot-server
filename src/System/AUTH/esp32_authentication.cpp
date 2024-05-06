#include "esp32_authentication.h"
#include "system_helper.h"
#include "base64.hpp"
#include "sha256.h"

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

ChangePasswordResult esp32_authentication::changePassword(const char* username, const char* oldPassword, const char* newPassword){

    //Serial.printf("Request to save password for user %s.\n\t old: %s new: ...\n", username,oldPassword,newPassword);
     if(strcmp(oldPassword,newPassword) == 0)
        return ChangePasswordResult::SamePassword;
    // Read the file
    // Check if the file exists
    if (!SPIFFS.exists(PATH_AUTH_FILE))
    {     
        return ChangePasswordResult::AuthSystemError;
       
    }
    //get user object, update it, store back
    File file = SPIFFS.open(PATH_AUTH_FILE);
    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc,file);
    file.close();

    if (error) {
        Serial.print("deserializeJson() failed: ");
        Serial.println(error.c_str());
        return ChangePasswordResult::AuthSystemError;
    }

    JsonVariant existingUser = findUser(doc.as<JsonArray>(),username);
    if( strcmp(existingUser["password"].as<string>().c_str(), oldPassword) != 0)
        return ChangePasswordResult::WrongPassword;
             
    existingUser["password"] = string(newPassword);

    //write back
    file = SPIFFS.open(PATH_AUTH_FILE,"w");
    serializeJson(doc, file);
//    file.flush();
    file.close();
        
    
    return ChangePasswordResult::Ok;    
}


JsonObject esp32_authentication::findUser(JsonArray users, const char* userName){
    for(JsonObject seekingUser : users){
        if(strcmp(seekingUser["username"].as<const char *>(),userName) == 0)
            return seekingUser;        
    }
    return JsonObject();
}

/// @brief 
/// @param plainPassword 
/// @param output  32 bytes of HMAC goodness
/// @return true if valid
bool esp32_authentication::encodePassword(const char *plainPassword, string &output)
{
//void ArduinoJWT::encodeJWT(char* payload, char* jwt) {
    char jwt[encode_base64_length(strlen(plainPassword) + 32)];
    memset(jwt,0,encode_base64_length(strlen(plainPassword) + 32));
    unsigned char* ptr = (unsigned char*)jwt;
    encode_base64((unsigned char*)plainPassword, strlen(plainPassword), ptr);
    ptr += encode_base64_length(strlen(plainPassword));  
    while(*(ptr - 1) == '=') {
        ptr--;
    }
    *(ptr) = 0;
    // Build the signature
    Sha256.initHmac((const unsigned char*)PASSWORD_ENCRYPTION_SECRET, strlen(PASSWORD_ENCRYPTION_SECRET));
    Sha256.print(jwt);
    // Add the signature to the jwt
    *ptr++ = '.';
    encode_base64(Sha256.resultHmac(), 32, ptr);
    ptr += encode_base64_length(32);
    // Get rid of any padding and replace / and +
    while(*(ptr - 1) == '=') {
        ptr--;
    }
    *(ptr) = 0;

    output = string((const char *)Sha256.result());

    return true;
}

bool esp32_authentication::verifyPassword(const char* username, const char* password){
    Sha256.initHmac((const unsigned char*)PASSWORD_ENCRYPTION_SECRET, strlen(PASSWORD_ENCRYPTION_SECRET));
    Sha256.print(password);
    unsigned char * encodedRequestPassword[64];
    memset(encodedRequestPassword,0,64);
    memcpy(encodedRequestPassword,Sha256.resultHmac(),64);
     

    File authFile = SPIFFS.open(PATH_AUTH_FILE,"r");    
    
    DynamicJsonDocument doc(1024); 
    DeserializationError error = deserializeJson(doc, authFile);
    authFile.close();

    if(error){
        Serial.printf("Error occured deserializing authorization file: [%i]%s\n", error.code(), error.c_str()); 
        //SPIFFS.remove(PATH_AUTH_FILE);
        return false;
    }

    JsonVariant existingUser = findUser(doc.as<JsonArray>(),username);

    if(existingUser.isNull()){
        return false;
    }
    auto existingPassword = existingUser["password"].as<const char*>();
    
    Sha256.initHmac((const unsigned char*)PASSWORD_ENCRYPTION_SECRET, strlen(PASSWORD_ENCRYPTION_SECRET));
    Sha256.print(existingPassword);

    unsigned char * encodedFilePassword[64];
    memset(encodedFilePassword,0,64);
    memcpy(encodedFilePassword,Sha256.resultHmac(),64);

    Serial.printf("Password from request: %08x, password from file %08x\n", encodedRequestPassword, encodedFilePassword);
    return strcmp((char*)encodedFilePassword, (char*)encodedRequestPassword) == 0;
    //get password from file, comapre to encrypted fromrequest
}

// bool esp32_authentication::decodePassword(const char *encodedPassword, string &output)
// {
//     int payloadLength = decode_base64_length((unsigned char*)encodedPassword) + 34;
//     if(payloadLength <= 0) { return false; }

//     char jsonPayload[payloadLength];
//     if( encodedPassword == NULL)
//     {
//     #ifdef DEBUG
//         Serial.printf("Missing password, {%s,\t%s,\t%s}\n",encodedPassword);
//     #endif
//         output = "";
//         return false;
//     }

//     // Build the signature
//     Sha256.initHmac((const unsigned char*)PASSWORD_ENCRYPTION_SECRET, strlen(PASSWORD_ENCRYPTION_SECRET));
//     Sha256.print(encodedPassword);

//     Serial.printf("Resulting HMAC: %02x", Sha256.resultHmac());

//     // Encode the signature as base64
//     unsigned char base64Signature[encode_base64_length(32)];
//     encode_base64(Sha256.resultHmac(), 32, base64Signature);
//     unsigned char* ptr = &base64Signature[0] + encode_base64_length(32);
//     // Get rid of any padding and replace / and +
//     while(*(ptr - 1) == '=') {
//         ptr--;
//     }
//     *(ptr) = 0;

//     // Do the signatures match?
//     if(strcmp((char*)encodedPassword, (char*)base64Signature) == 0) {
//         // Decode the payload
//         decode_base64((unsigned char*)encodedPassword, (unsigned char*)jsonPayload);
//         output = string(jsonPayload);
//         return true;
//     } else {  
//         Serial.printf("String encoded %s and signature %s do not match!\n", encodedPassword, base64Signature); 
//         output = "";
//         return false;
//     }
// }
