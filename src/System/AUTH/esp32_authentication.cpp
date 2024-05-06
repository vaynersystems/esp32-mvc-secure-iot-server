#include "esp32_authentication.h"
#include "system_helper.h"
#include "base64.hpp"
#include "sha256.h"
/// @brief User authentication. If the user database has not been created, the first request to authenticate will become the system admin.
/// @brief This way the backdoor of default username/password doesn't exist.
/// @param username 
/// @param password 
/// @return Authentication object containing the user's information and status of request
esp32_user_auth_info esp32_authentication::authenticateUser(const char* username, const char* password){
    esp32_user_auth_info info;
    bool firstUser = false;
    // Read the file
    auto filename = std::string(PATH_AUTH_FILE);
    
    // Check if the file exists
    if (!SPIFFS.exists(filename.c_str()))
    {     
        Serial.println("Authorization file does not exist. Creating"); 
        byte encryptedPass[SHA256_SIZE];
        firstUser = true; //if first user, authenticate and create file     
        bool registered = registerUser(username, password, "ADMIN"); 
        encryptPassword(password, encryptedPass);
        if(registered){
            info.username = username;
            info.role = "ADMIN";
            info.authenticated =registered;
        }
       
    } else { // check if valid user
        info.authenticated = verifyPassword(username,password);
        if(!info.authenticated) return info;
        
        File file = SPIFFS.open(filename.c_str());
        StaticJsonDocument<512> d;
        DeserializationError error = deserializeJson(d,file);        
        file.close();
        if (error) {
            Serial.print("deserializeJson() failed: ");
            Serial.println(error.c_str());
            return info;
        }

        auto existingUser = findUser(d.as<JsonArray>(),username);

        if(!existingUser.isNull())
            info.role =  existingUser["role"].as<const char *>();  
    }
    return info;

    
}
bool esp32_authentication::registerUser(const char* username, const char* password, const char* role){
    File authFile = SPIFFS.open(PATH_AUTH_FILE,"r");    
    byte encryptedPass[SHA256_SIZE];
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
    
    encryptPassword(password, encryptedPass);

    JsonObject newUser = doc.createNestedObject();
    newUser["username"] = username;
    newUser["password"] = encryptedPass;
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
    byte encryptedPass[SHA256_SIZE];
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
    if(!verifyPassword(username,oldPassword))        
        return ChangePasswordResult::WrongPassword;
        
             
    encryptPassword(newPassword, encryptedPass);
    existingUser["password"] = encryptedPass;

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



bool esp32_authentication::verifyPassword(const char* username, const char* password){
    
    byte requestPasswordHash[SHA256_SIZE];
    byte storedPasswordHash[SHA256_SIZE];

    encryptPassword(password, requestPasswordHash);

    //get stored password (will be in hash, plain text now)
    File authFile = SPIFFS.open(PATH_AUTH_FILE,"r");    
    // 
    DynamicJsonDocument doc(1024); 
    DeserializationError error = deserializeJson(doc, authFile);
    authFile.close();
 
    // if(error){
    //     Serial.printf("Error occured deserializing authorization file: [%i]%s\n", error.code(), error.c_str()); 
    //     //SPIFFS.remove(PATH_AUTH_FILE);
    //     return false;
    // }

    // JsonVariant existingUser = findUser(doc.as<JsonArray>(),username);
 
    // if(existingUser.isNull()){
    //     return false;
    // }
    // auto existingPassword = existingUser["password"].as<const char*>();
    // encryptPassword(existingPassword, storedPasswordHash);

    // //print out
    // Serial.println("Request Password Hash:");
    // for (byte i; i < SHA256_SIZE; i++)
    // {
    //     Serial.print(requestPasswordHash[i], HEX);
    // }  
    // Serial.println();
    // Serial.println("Stored Password Hash:");
    // for (byte i; i < SHA256_SIZE; i++)
    // {
    //     Serial.print((byte)storedPasswordHash[i], HEX);
    // }  
    // Serial.println();
    
    // //validate
    // for (byte i; i < SHA256_SIZE; i++){
    //     if(storedPasswordHash[i] != requestPasswordHash[i])
    //     return false;
    // }

    return true;
}
/// @brief 
/// @param plainPassword 
/// @param output  32 bytes of HMAC goodness
/// @return true if valid
bool esp32_authentication::encryptPassword(const char *plainPassword,byte * output)
{
    SHA256 hasher;
    //hasher.doUpdate(plainPassword, strlen(plainPassword));

    // byte hash[SHA256_SIZE];
    // hasher.doFinal(hash);

    // //copy and return sucess
    // memcpy(output,&hash,SHA256_SIZE);  
    // return true;
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
