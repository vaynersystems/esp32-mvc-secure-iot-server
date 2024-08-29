#include "esp32_authentication.h"
#include "system_helper.h"
#include "base64.hpp"
#include "esp32_sha256.h"
#include "string_helper.h"
//case sensitive char literal to binary
#define SHORT_FROM_CHAR(c) (c == '1' ? 1 : c == '2' ? 2 : c=='3' ? 3 : c=='4' ? 4 : c=='5' ? 5 : c=='6' ? 6 : c=='7' ? 7 : c=='8' ? 8 : c=='9' ? 9 : c=='A' ? 0xA : c=='B' ? 0xB : c=='C' ? 0xC : c=='D' ? 0xD : c=='E' ? 0xE : c=='F' ? 0xF : 0x0 )

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
        #ifdef DEBUG
        Serial.println("Authorization file does not exist..");
        #endif
        if(strlen(password) == 64) //ecoded password, must reject
            return info;
        if(strlen(username) < 3 || strlen(username) > 32) return info;
        #ifdef DEBUG
        Serial.println("Creating"); 
        #endif
        byte encryptedPass[SHA256_SIZE];
        char* requestPassEncrypted = new char[SHA256_SIZE*2 + 1];
        requestPassEncrypted[SHA256_SIZE*2] = '\0'; //null terminate
        firstUser = true; //if first user, authenticate and create file     
        bool registered = registerUser(username, password, "ADMIN"); 
        encryptPassword(password, encryptedPass);
        binaryPasswordToString(encryptedPass, requestPassEncrypted);
        delete[] requestPassEncrypted;
        if(registered){
            info.username = username;
            info.role = "ADMIN";
            info.authenticated =registered;
            info.password = string(requestPassEncrypted);
        }
       
    } else { // check if valid user
        info.authenticated = verifyPassword(username,password);
        if(!info.authenticated) return info;
        
        info.username = username;
        byte* requestPasswordHash = new byte[SHA256_SIZE];
        char* requestPassEncrypted = new char[SHA256_SIZE*2 + 1];
        requestPassEncrypted[SHA256_SIZE*2] = '\0'; //null terminate
        if(strlen(password) == 64){ //if verifying hash, then no need to encrypt
            //Serial.println("Passing hash from request to auth info");
            //memcpy(info.password, password,64);
            info.password = password;
        }
        else {
            encryptPassword(password, requestPasswordHash);
            binaryPasswordToString(requestPasswordHash, requestPassEncrypted);
            //memcpy(info.password, requestPassEncrypted,64);
            info.password = string(requestPassEncrypted);
        }

        delete[] requestPasswordHash;
        delete[] requestPassEncrypted;

        File file = SPIFFS.open(filename.c_str());
        DynamicJsonDocument d(2048);
        DeserializationError error = deserializeJson(d,file);        
        file.close();
        if (error) {
            #ifdef DEBUG
            Serial.print("deserializeJson() failed: ");
            Serial.println(error.c_str());
            #endif
            return info;
        }
        auto existingUser = findUser(d.as<JsonArray>(),username);
        if(!existingUser.isNull()){
            info.username =  existingUser["username"].as<const char *>();  
            info.role =  existingUser["role"].as<const char *>();  
        }
    }
    return info;

    
}
bool esp32_authentication::registerUser(const char* username, const char* password, const char* role, bool enabled){
    File authFile = SPIFFS.open(PATH_AUTH_FILE,"r");    
    byte encryptedPass[SHA256_SIZE];
    char storedPass[(SHA256_SIZE*2) + 1];
    DynamicJsonDocument doc(1024); 
    DeserializationError error = deserializeJson(doc, authFile);
    authFile.close();

    if(error){
        //if("Error occured deserializing authorization file: [%i]%s\n", error.code(), error.c_str()); 
        //SPIFFS.remove(PATH_AUTH_FILE);
    }else {
        JsonVariant existingUser = findUser(doc.as<JsonArray>(),username);

        if(!existingUser.isNull()){
            return false;
        }
    }
    encryptPassword(password, encryptedPass);
    binaryPasswordToString(encryptedPass, storedPass);
    
    JsonObject newUser = doc.createNestedObject();
    newUser["username"] = username;
    newUser["password"] = storedPass;
    newUser["role"] = role;
    newUser["enabled"] = enabled;
    newUser["created"] =   getCurrentTime();

    //TODO: password complexity and validation
    
    
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
    char storedPass[(SHA256_SIZE*2) + 1]; //2 bytes per hex digit and null terminator
    //get user object, update it, store back
    File file = SPIFFS.open(PATH_AUTH_FILE);
    DynamicJsonDocument doc(2048);
    DeserializationError error = deserializeJson(doc,file);
    file.close();

    if (error) {
        // Serial.print("deserializeJson() failed: ");
        // Serial.println(error.c_str());
        return ChangePasswordResult::AuthSystemError;
    }

    JsonVariant existingUser = findUser(doc.as<JsonArray>(),username);
    if(!verifyPassword(username,oldPassword))        
        return ChangePasswordResult::WrongPassword;
        
             
    encryptPassword(newPassword, encryptedPass);
    binaryPasswordToString(encryptedPass, storedPass);         
    existingUser["password"] = storedPass;

    //write back
    file = SPIFFS.open(PATH_AUTH_FILE,"w");
    serializeJson(doc, file);
//    file.flush();
    file.close();
    return ChangePasswordResult::Ok;    
}


JsonObject esp32_authentication::findUser(JsonArray users, const char* userName){
    for(JsonObject seekingUser : users){
        if(!seekingUser["username"].isNull() && iequals(seekingUser["username"].as<const char *>(),userName, seekingUser["username"].size()))
            return seekingUser;        
    }
    return JsonObject();
}



bool esp32_authentication::verifyPassword(const char* username, const char* password){
    
    byte requestPasswordHash[SHA256_SIZE];
    byte storedPasswordHash[SHA256_SIZE];
    //Serial.printf("Calling verifyPassword with length of %d\n", strlen(password));
    if(strlen(password) == 64){ //if verifying hash, then no need to encrypt
        stringPasswordToBinary(password, requestPasswordHash);
    }
    else 
        encryptPassword(password, requestPasswordHash);

    //passwords are stored in hashed format
    File authFile = SPIFFS.open(PATH_AUTH_FILE,"r");    
    // 
    DynamicJsonDocument doc(1024); 
    DeserializationError error = deserializeJson(doc, authFile);
    authFile.close();
 
    if(error){
        //Serial.printf("Error occured deserializing authorization file: [%i]%s\n", error.code(), error.c_str()); 
        //SPIFFS.remove(PATH_AUTH_FILE);
        return false;
    }

    JsonVariant existingUser = findUser(doc.as<JsonArray>(),username);
 
    if(existingUser.isNull()){
        return false;
    }
    // if(existingUser["password"].isNull())
    //     Serial.println("Password field in existing user is null");
    // else
    //     Serial.printf("Password field in existing user is %s\n", existingUser["password"].as<const char*>());
    auto existingPassword = existingUser["password"].as<const char*>();
    
    stringPasswordToBinary(existingPassword, storedPasswordHash);
    //encryptPassword(existingPassword, storedPasswordHash);    
    
    //validate
    bool valid = true;
    for (byte i; i < SHA256_SIZE; i++){
        if(storedPasswordHash[i] != requestPasswordHash[i]){
            Serial.printf("Byte %d does not match. %02x in stored, %02X in request\n", i, storedPasswordHash[i], requestPasswordHash[i]);            
            valid = false;
            break;
        }         
    }

    // if(!valid){
    //     Serial.printf("Request password was %s. Encoded: \n", password);
    //     for(int idx=0;idx < SHA256_SIZE;idx++){
    //         Serial.printf("%02X",requestPasswordHash);
    //     }
    // }
   
    return valid;
}
/// @brief 
/// @param plainPassword 
/// @param output  32 byte SHA256 hash
/// @return true if valid
void esp32_authentication::encryptPassword(const char *plainPassword,byte * output)
{
    esp32_sha256 hasher;
    uint8_t *hash;
    hasher.init();
    hasher.write(plainPassword);
    hash = hasher.result();
    //Serial.println();
    // Serial.printf("encrypted hash of %s \n", plainPassword);
    // for(int idx = 0; idx < 32; idx++)
    //     Serial.printf("%02X",hash[idx]);
    // Serial.println();
    memcpy(output,hash,SHA256_SIZE); 
}

#pragma region Helper Functions

void esp32_authentication::binaryPasswordToString(byte *encryptedPass, char *storedPass)
{
    for (byte i; i < SHA256_SIZE; i++)
    {
        sprintf(storedPass + i*2,"%02X",encryptedPass[i]);        
    }  
    //Serial.printf("Stored password: %s\n",storedPass);
}

void esp32_authentication::stringPasswordToBinary(const char *storedPass, byte *encryptedPass)
{
    //Serial.printf("Converting string of length %d to binary\n", strlen(storedPass));
    for(int idx = 0; idx < SHA256_SIZE*2 - 1; idx+=2){
        //read two characters, store to byte
        encryptedPass[idx/2] = (SHORT_FROM_CHAR(storedPass[idx]) << 4) | SHORT_FROM_CHAR(storedPass[idx+1]);          
    }
}
#pragma endregion