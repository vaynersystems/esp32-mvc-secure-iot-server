#include "request_body.h"
#include "esp32_config_controller.hpp"
#include "esp32_crypt.h"
#include "System/AUTH/esp32_authentication.hpp"

#ifndef BACKUP_MAX_SIZE
#define BACKUP_MAX_SIZE (64 * 1024)
#endif
// Backups made before v2 have a predictable key and no integrity check, so a forged one could
// add an administrator. Build with -D BACKUP_ALLOW_LEGACY_RESTORE=1 only to migrate an old backup.
#ifndef BACKUP_ALLOW_LEGACY_RESTORE
#define BACKUP_ALLOW_LEGACY_RESTORE 0
#endif


DerivedController<esp32_config_controller> esp32_config_controller::reg("esp32_config");

static void redactSecrets(JsonDocument& doc);

void esp32_config_controller::Index(HTTPRequest* request, HTTPResponse* response) {
    
    title = "Module Configuration Page";
    auto drive = filesystem.getDisk(SYSTEM_DRIVE);

    File f = drive->open(PATH_SYSTEM_CONFIG,"r");
    if(!f){
        #if defined(DEBUG) && DEBUG > 0
        Serial.printf("Failed to open config file on %s\n", drive->label());
        #endif
        return;
    }
    DynamicJsonDocument doc(f.size() * 3 + 1024);
    deserializeJson(doc, f);
    f.close();
    // Never embed the Wi-Fi passwords in the page (LoadConfigData redacts them too).
    redactSecrets(doc);
    string json;
    serializeJson(doc, json);

    // The page places this inside a JavaScript template literal: escape what would end it or
    // be evaluated (backslash, backtick, ${) and "<" so "</script>" cannot close the script tag.
    string configData;
    configData.reserve(json.size() + 32);
    for(size_t i = 0; i < json.size(); i++){
        char c = json[i];
        if(c == '\\') configData += "\\\\";
        else if(c == '`') configData += "\\`";
        else if(c == '$') configData += "\\$";
        else if(c == '<') configData += "\\x3c";
        else configData += c;
    }

    controllerTemplate.SetTemplateVariable(F("$_CONFIGURATION_DATA"), configData.c_str());
    //controllerTemplate.SetTemplateVariable(F("$_CONFIG_FILE"), PATH_SYSTEM_CONFIG);
    
    esp32_base_controller::Index(request, response);    
}

void esp32_config_controller::Post(HTTPRequest* request, HTTPResponse* response) {
    if(strcmp(request->getHeader(HEADER_GROUP).c_str(),"ADMIN") != 0)
    {
        response->setStatusCode(401);
        return;
    }
    SaveConfigData(request, response);   
}

/// @brief Overwrite Action since we have custom action implemented
/// @param req 
/// @param res 
void esp32_config_controller::Action(HTTPRequest* request, HTTPResponse* response) {
    if (route.action.compare("GetAvailableWifi") == 0) {
        GetAvailableWifi(request, response);
    }
    else if (route.action.compare("LoadConfigData") == 0) {
        LoadConfigData(request, response);
    }
    else if (route.action.compare("SaveConfigData") == 0) {
        SaveConfigData(request, response);
    }
    else if (route.action.compare("ResetDevice") == 0) {
        ResetDevice(request, response);
    }
    else if (route.action.compare("FactoryReset") == 0) {
        FactoryReset(request, response);
    }
    else if (route.action.compare("UploadCertificate") == 0) {
        UploadCertificate(request, response);
    }
    else if (route.action.compare("GenerateCertificate") == 0) {
        GenerateCertificate(request, response);
    }
    else if (route.action.compare("UpdateFirmware") == 0) {
        UpdateFirmware(request, response);
    }
    
    else if (route.action.compare("Backup") == 0) {
        Backup(request, response);
    }
    else if (route.action.compare("Restore") == 0) {
        Restore(request, response);
    }
    else
        esp32_base_controller::Action(request, response);
}

bool esp32_config_controller::HasAction(const char * action){
    if (strcmp(action,"GetAvailableWifi") == 0) {
        return true;
    }
    
    else if (strcmp(action, "LoadConfigData") == 0) {
        return true;
    }
    
    else if (strcmp(action, "SaveConfigData") == 0) {
        return true;
    }
    
    else if (strcmp(action, "ResetDevice") == 0) {
        return true;
    }

    else if (strcmp(action, "FactoryReset") == 0) {
        return true;
    }

    else if (strcmp(action, "UploadCertificate") == 0) {
        return true;
    }

    else if (strcmp(action, "GenerateCertificate") == 0) {
        return true;
    }

    else if (strcmp(action, "UpdateFirmware") == 0) {
        return true;
    }

    else if (strcmp(action, "Backup") == 0) {
        return true;
    }

    else if (strcmp(action, "Restore") == 0) {
        return true;
    }
    
    else
        return esp32_base_controller::HasAction(action);
}

//custom action to get wifi list, or other list
void esp32_config_controller::GetAvailableWifi(HTTPRequest* request, HTTPResponse* response) {
     int n = WiFi.scanNetworks();
     string outputstring;   

    outputstring = "[";
     for (int i = 0; i < n; ++i) {
        //auto network = networks.createNestedObject();
        string encryption;
        switch (WiFi.encryptionType(i))
        {
            case WIFI_AUTH_OPEN:
                encryption = "open";
                break;
            case WIFI_AUTH_WEP:
                encryption = "WEP";
                break;
            case WIFI_AUTH_WPA_PSK:
                encryption = "WPA";
                break;
            case WIFI_AUTH_WPA2_PSK:
                encryption = "WPA2";
                break;
            case WIFI_AUTH_WPA_WPA2_PSK:
                encryption = "WPA+WPA2";
                break;
            case WIFI_AUTH_WPA2_ENTERPRISE:
                encryption = "WPA2-EAP";
                break;
            case WIFI_AUTH_WPA3_PSK:
                encryption = "WPA3";
                break;
            case WIFI_AUTH_WPA2_WPA3_PSK:
                encryption = "WPA2+WPA3";
                break;
            case WIFI_AUTH_WAPI_PSK:
                encryption = "WAPI";
                break;
            default:
                encryption = "unknown";
        }
        outputstring += string_format("%s { \"ssid\": \"%s\",  \"rssi\": \"%d\",  \"channel\": \"%d\",  \"encryption\": \"%s\"}",
            i == 0 ? "" : ",",
            WiFi.SSID(i).c_str(),
            WiFi.RSSI(i),
            WiFi.channel(i),
            encryption.c_str()
        );
    }
    outputstring += "]";
    response->print(outputstring.c_str());    
    response->setStatusCode(200);
}

// Configuration values that are never sent to the browser.
// Each is replaced by "" plus a "<key>Set" flag saying whether a value is stored.
static JsonObject secretParent(JsonDocument& doc, int index, const char*& key){
    switch(index){
        case 0: key = "password"; return doc["wifi"].as<JsonObject>();
        case 1: key = "password"; return doc["wifi"]["ap"].as<JsonObject>();
    }
    key = nullptr;
    return JsonObject();
}
static const int SECRET_COUNT = 2;

static void redactSecrets(JsonDocument& doc){
    for(int i = 0; i < SECRET_COUNT; i++){
        const char* key;
        JsonObject parent = secretParent(doc, i, key);
        if(parent.isNull()) continue;
        bool isSet = strlen(parent[key] | "") > 0;
        parent[key] = "";
        parent[string(key) + "Set"] = isSet;
    }
}

/// @brief Fill secrets the browser left blank from the stored configuration.
/// @brief "<key>Clear": true removes a stored secret (e.g. make the access point open).
/// @brief A different Wi-Fi network with no password given is an open network, not the old one's.
static void mergeSecrets(JsonDocument& incoming, JsonDocument& existing){
    for(int i = 0; i < SECRET_COUNT; i++){
        const char* key;
        JsonObject parent = secretParent(incoming, i, key);
        if(parent.isNull()) continue;
        string setFlag = string(key) + "Set", clearFlag = string(key) + "Clear";
        bool clear = parent[clearFlag] | false;
        parent.remove(setFlag);
        parent.remove(clearFlag);
        if(clear){ parent[key] = ""; continue; }
        if(strlen(parent[key] | "") > 0) continue; // new value given

        const char* existingKey;
        JsonObject existingParent = secretParent(existing, i, existingKey);
        if(existingParent.isNull()) { parent[key] = ""; continue; }
        if(i == 0 && strcmp(parent["network"] | "", existingParent["network"] | "") != 0){
            parent[key] = "";
            continue;
        }
        parent[key] = existingParent[existingKey].as<std::string>(); // copy: existing is freed after save
    }
}

/// @brief Load configuration information from json file on disk
/// @param req 
/// @param res 
void esp32_config_controller::LoadConfigData(HTTPRequest* request, HTTPResponse* response) {
    if(strcmp(request->getHeader(HEADER_GROUP).c_str(),"ADMIN") != 0)
    {
        response->setStatusCode(401);
        return;
    }
    auto drive = filesystem.getDisk(SYSTEM_DRIVE);

    File f = drive->open(PATH_SYSTEM_CONFIG,"r");
    DynamicJsonDocument doc(f.size() * 2 + 512);
    auto error = deserializeJson(doc, f);
    f.close();
    if(error){
        response->setStatusCode(500);
        response->setStatusText(error.c_str());
        return;
    }
    // Secrets never go back to the browser: only whether one is stored.
    redactSecrets(doc);

    string out;
    serializeJson(doc, out);
    response->setHeader("Content-Type", "application/json");
    response->setHeader("Cache-Control", "no-store");
    response->setStatusCode(200);
    response->print(out.c_str());
}

/// @brief Save configuration json to disk
/// @param req 
/// @param res 
/// @return 
bool esp32_config_controller::SaveConfigData(HTTPRequest* request, HTTPResponse* response){
    if(strcmp(request->getHeader(HEADER_GROUP).c_str(),"ADMIN") != 0)
    {
        response->setStatusCode(401);
        return false;
    }
    const int length = request->getContentLength();
    
    DynamicJsonDocument doc(length * 2 + 512);
    string content;
    // Read the whole body (it can arrive after the headers over HTTPS).
    if(!readRequestBody(request, content)){
        response->setStatusCode(400);
        response->setStatusText("Incomplete request body");
        return false;
    }

    #if defined(DEBUG) && DEBUG > 0
    Serial.printf("Saving %i bytes to config\n", content.length());
    #endif
    auto error = deserializeJson(doc, content);

    if(error.code() == DeserializationError::Ok){
        //if we need to apply certs do so and clear it
        if(!doc["server"]["certificates"].isNull() && !doc["server"]["certificates"]["uploaded"].isNull()){
            if(doc["server"]["certificates"]["uploaded"].as<bool>() == true){
                bool worked = server.importCertFromTemporaryStorage();
                doc["server"]["certificates"]["uploaded"] = NULL;
                if(!worked)
                {
                    response->setStatusCode(501);
                    response->setStatusText("Failed to store certificates.");
                    return false;
                }
            }
        }
        auto drive = filesystem.getDisk(0);
        // The page never receives secrets, so a blank one means "keep what is stored".
        File existingFile = drive->open(PATH_SYSTEM_CONFIG, "r");
        DynamicJsonDocument existing(existingFile.size() * 2 + 512);
        deserializeJson(existing, existingFile);
        existingFile.close();
        mergeSecrets(doc, existing);
        if(doc.overflowed()){
            response->setStatusCode(500);
            response->setStatusText("Configuration too large");
            return false;
        }

        File f = drive->open(PATH_SYSTEM_CONFIG, "w");
        serializeJson(doc,f);
        f.close();
        response->setStatusCode(200);
        logger.logInfo(string_format("%s saved configuration to %s", request->getBasicAuthUser().c_str(), PATH_SYSTEM_CONFIG));
    } else{
        response->setStatusCode(500);
        // String errorText = "Error saving configuration: ";
        // errorText += error.c_str();
        response->setStatusText(error.c_str());
        #if defined(DEBUG) && DEBUG > 0
        Serial.printf(error.c_str());
        #endif
        return false;
    }    
    
    return true;
}

void esp32_config_controller::ResetDevice(HTTPRequest* request, HTTPResponse* response){
    if(strcmp(request->getHeader(HEADER_GROUP).c_str(),"ADMIN") == 0)
        esp_restart();
    else response->setStatusCode(401);
}

void esp32_config_controller::UploadCertificate(HTTPRequest *request, HTTPResponse *response)
{
    if(strcmp(request->getHeader(HEADER_GROUP).c_str(),"ADMIN") != 0)
    {
        response->setStatusCode(401);
        return;
    }
    if(strcmp(route.params.c_str(), "Public") == 0){
        esp32_router::handleFileUpload(request, response, PUBLIC_TEMP_PATH);
    } else if(strcmp(route.params.c_str(), "Private") == 0){
        esp32_router::handleFileUpload(request, response, PRIVATE_TEMP_PATH);
    }
    
}

void esp32_config_controller::GenerateCertificate(HTTPRequest *request, HTTPResponse *response)
{
    if(strcmp(request->getHeader(HEADER_GROUP).c_str(),"ADMIN") != 0)
    {
        response->setStatusCode(401);
        return;
    }
    const int length = request->getContentLength();
    
    DynamicJsonDocument doc(length * 2);
    string content;
    #if defined(DEBUG) && DEBUG > 0
    Serial.printf("Generating certificate...\n");
    #endif
    // Read the whole body (it can arrive after the headers over HTTPS).
    if(!readRequestBody(request, content)){
        response->setStatusCode(400);
        response->setStatusText("Incomplete request body");
        return;
    }

    auto error = deserializeJson(doc, content);

    if(error.code() == DeserializationError::Ok){
        string deviceName = "", companyName = "", validFrom="", validTo="";
        //get certificate parameters
        if(doc["device"].isNull() || doc["company"].isNull()){
            response->println("Missing device or company. Please fill in fields and retry");
            return;
        }

        deviceName = doc["device"] | "";
        companyName = doc["company"] | "";
        validFrom = doc["from"] | "20240101000000";
        validTo = doc["to"] | "20350101000000";
        #if defined(DEBUG) && DEBUG > 0
        Serial.printf(".. parsed \n\tdevice: %s\n\t company %s\n\t valid-from: %s\n\t valid-to: %s\n",
            deviceName.c_str(), companyName.c_str(), validFrom.c_str(), validTo.c_str()
        );
        #endif

        if(deviceName.length() > 0 && deviceName.length() < 32 &&
            companyName.length() > 0 && companyName.length() < 32 && 
            validFrom.length() == 14 && validTo.length() == 14)
        {
            server.generateCertificate(deviceName.c_str(), companyName.c_str(), validFrom.c_str(), validTo.c_str());
        }
        
    } else {
        response->printf("Error deserializing input: [%d]%s\n", error.code(), error.c_str());
    }
}

void esp32_config_controller::UpdateFirmware(HTTPRequest *request, HTTPResponse *response)
{
    if(strcmp(request->getHeader(HEADER_GROUP).c_str(),"ADMIN") != 0)
    {
        response->setStatusCode(401);
        return;
    }
    if(request->getMethod() != "POST")
        return;
    lcd.pause();
    auto result = otaServer.updateFirmware(request);    
    lcd.play();
    response->flush();
    response->setStatusCode(result == ESP_OK ? 200 : 500);
    response->printf("Update %s: %s", result == ESP_OK ? "completed" : "failed", esp_err_to_name(result)); 
}

void esp32_config_controller::Backup(HTTPRequest *request, HTTPResponse *response)
{
    if(strcmp(request->getHeader(HEADER_GROUP).c_str(),"ADMIN") != 0)
    {
        response->setStatusCode(401);
        return;
    }

    //package config, auth, public files into one
    auto drive = filesystem.getDisk(SYSTEM_DRIVE);

    File authorize = drive->open(PATH_AUTH_FILE, "r");
    // size the user database from the file so no user is silently dropped from the backup
    DynamicJsonDocument docSecurity(authorize.size() * 2 + 1024);
    auto securityError = deserializeJson(docSecurity,authorize);
    authorize.close();
    if(securityError || docSecurity.overflowed()){
        response->setStatusCode(500);
        response->setStatusText("Failed to read user database for backup");
        return;
    }

    DynamicJsonDocument doc(8192 + docSecurity.capacity());
    DynamicJsonDocument docPublic(2048);
    DynamicJsonDocument docConfig(2048);
    DynamicJsonDocument docDevices(2048);

    File config = drive->open(PATH_SYSTEM_CONFIG, "r");
    deserializeJson(docConfig,config);
    config.close();

    File devices = drive->open(PATH_DEVICE_CONFIG, "r");
    deserializeJson(docDevices,devices);
    devices.close();

    File publicPages = drive->open(PATH_PUBLIC_PAGES, "r");
    auto publicArray = docPublic.to<JsonArray>();
    while(publicPages.available()){      
        publicArray.add(publicPages.readStringUntil('\n'));
    }
    publicPages.close();

    doc["security"] = docSecurity;
    doc["config"] = docConfig;
    doc["devices"] = docDevices;
    doc["public"] = docPublic;
    doc["type"] = "esp32-backup";
    if(doc.overflowed()){
        response->setStatusCode(500);
        response->setStatusText("Backup too large");
        return;
    }

    

    // Keys are derived from the whole device private key; a random IV is used per backup
    // and the file is authenticated so a modified backup is rejected on restore.
    uint8_t encKey[DERIVED_KEY_SIZE], macKey[DERIVED_KEY_SIZE];
    auto cert = server.getCertificate();
    if(!derive_device_key(cert->getPKData(), cert->getPKLength(), KEY_LABEL_BACKUP_ENC, encKey) ||
       !derive_device_key(cert->getPKData(), cert->getPKLength(), KEY_LABEL_BACKUP_MAC, macKey)){
        response->setStatusCode(500);
        response->setStatusText("Backup key unavailable");
        return;
    }

    string backupData = "";
    serializeJson(doc,backupData);
    size_t sealedLength = backup_sealed_length(backupData.length());
    uint8_t* sealedData = (uint8_t*)malloc(sealedLength);
    bool sealed = sealedData != nullptr && backup_seal(
        (const uint8_t*)backupData.data(), backupData.length(), encKey, macKey, sealedData, &sealedLength);
    memset(encKey, 0, sizeof(encKey));
    memset(macKey, 0, sizeof(macKey));
    if(!sealed){
        free(sealedData);
        response->setStatusCode(500);
        response->setStatusText("Failed to encrypt backup");
        return;
    }

    char dispStr[128];
    sprintf(dispStr, " attachment; filename = \"%s\"", "backup.json");
    response->setHeader("Content-Disposition", dispStr);    
    response->setHeader("Content-Type","application/octet-stream");
    response->setStatusCode(200);
    response->write(sealedData, sealedLength);
    free(sealedData);
}

/// @brief Copy a file on the system drive, writing exactly the bytes read.
static bool copySystemFile(const char* from, const char* to){
    auto drive = filesystem.getDisk(SYSTEM_DRIVE);
    File sourceFile = drive->open(from, "r");
    if(!sourceFile) return false;
    File destFile = drive->open(to, FILE_WRITE);
    if(!destFile){ sourceFile.close(); return false; }
    static uint8_t buf[FILESYSTEM_BUFFER_SIZE];
    size_t total = 0, expected = sourceFile.size();
    bool ok = true;
    while(true){
        size_t readLength = sourceFile.read(buf, FILESYSTEM_BUFFER_SIZE);
        if(readLength == 0) break;
        if(destFile.write(buf, readLength) != readLength){ ok = false; break; }
        total += readLength;
    }
    destFile.close();
    sourceFile.close();
    return ok && total == expected;
}

/// @brief A restorable user list must be a non-empty array where every entry has a username and password.
static bool isValidUserList(JsonArray users){
    if(users.isNull() || users.size() == 0) return false;
    for(JsonObject user : users){
        if(user.isNull()) return false;
        if(!user["username"].is<const char*>() || strlen(user["username"].as<const char*>()) == 0) return false;
        if(!user["password"].is<const char*>() || strlen(user["password"].as<const char*>()) == 0) return false;
    }
    return true;
}

void esp32_config_controller::Restore(HTTPRequest *request, HTTPResponse *response)
{
    if(strcmp(request->getHeader(HEADER_GROUP).c_str(),"ADMIN") != 0)
    {
        response->setStatusCode(401);
        return;
    }
    const int length = request->getContentLength();
    // Check the size before sizing anything from it: the documents below are allocated from
    // the client's Content-Length.
    if(length <= 0 || length > BACKUP_MAX_SIZE){
        request->discardRequestBody();
        response->setStatusCode(400);
        response->setStatusText("Invalid backup size");
        return;
    }
    auto drive = filesystem.getDisk(SYSTEM_DRIVE);
    DynamicJsonDocument doc(length * 2 + 8192);
    DynamicJsonDocument docPublic(2048);
    DynamicJsonDocument docSecurity(length * 2 + 2048);
    DynamicJsonDocument docConfig(2048);
    DynamicJsonDocument docDevices(2048);
    unsigned char * backupDataEncrypted  = (unsigned char *)malloc(length);
    unsigned char * decryptedData = (unsigned char *)malloc(length + 1);
    if(backupDataEncrypted == nullptr || decryptedData == nullptr){
        free(backupDataEncrypted);
        free(decryptedData);
        response->setStatusCode(500);
        return;
    }
    memset(decryptedData, 0, length + 1);
    unsigned char * buf = new unsigned char[FILESYSTEM_BUFFER_SIZE];
    size_t fieldLength = 0;
    unsigned long lastData = millis();
    while(fieldLength < (size_t)length && !request->requestComplete()){
        size_t toRead = min((size_t)FILESYSTEM_BUFFER_SIZE, (size_t)length - fieldLength);
        size_t readLength = request->readBytes((byte*)buf, toRead);
        if(readLength == 0){
            if(millis() - lastData > 10000) break; // upload stalled: stop instead of spinning forever
            delay(2);
            continue;
        }
        lastData = millis();
        memcpy(&backupDataEncrypted[fieldLength], buf, readLength);
        fieldLength += readLength;
    }     
    delete[] buf;

    #if defined(DEBUG) && DEBUG > 3
    Serial.printf("Decrypting backup of %d bytes. Read %d bytes\n", length, fieldLength);
    #endif

    auto cert = server.getCertificate();
    size_t decryptedLength = 0;
    bool opened = false;
    if(backup_is_v2(backupDataEncrypted, fieldLength)){
        uint8_t encKey[DERIVED_KEY_SIZE], macKey[DERIVED_KEY_SIZE];
        opened = derive_device_key(cert->getPKData(), cert->getPKLength(), KEY_LABEL_BACKUP_ENC, encKey) &&
                 derive_device_key(cert->getPKData(), cert->getPKLength(), KEY_LABEL_BACKUP_MAC, macKey) &&
                 backup_open(backupDataEncrypted, fieldLength, encKey, macKey, decryptedData, &decryptedLength);
        memset(encKey, 0, sizeof(encKey));
        memset(macKey, 0, sizeof(macKey));
    }
    #if BACKUP_ALLOW_LEGACY_RESTORE
    else if(fieldLength > 0 && fieldLength % 16 == 0){
        // Backups made before v2 used a key/IV taken from the first bytes of the private key.
        // That key is predictable, so these files are not authenticated; allowed only for migration.
        logger.logInfo("Restoring legacy (unauthenticated) backup format");
        char enc_key[32] = {0};
        char enc_iv[16] =  {0};
        memcpy(enc_key,cert->getPKData(),sizeof(enc_key));
        memcpy(enc_iv,cert->getPKData(),sizeof(enc_iv));
        decrypt_string(backupDataEncrypted, fieldLength, (const char*)enc_key,  (const char*)enc_iv, decryptedData);
        decryptedData[fieldLength] = 0;
        decryptedLength = fieldLength;
        opened = true;
    }
    #endif
    free(backupDataEncrypted);

    if(!opened){
        free(decryptedData);
        response->setStatusCode(400);
        response->setStatusText("Backup could not be verified. It was modified or made by another device.");
        return;
    }

    #if defined(DEBUG) && DEBUG > 0
    Serial.printf("Saving %i bytes to config\n", decryptedLength);
    #endif
    auto error = deserializeJson(doc, decryptedData);

      

    if(error.code() == DeserializationError::Ok){
        #if defined(DEBUG) && DEBUG > 0
        Serial.println("Restoring from backup");
        Serial.println();
        #endif
         bool failed = false;
        string secString = "";
        //if we need to apply certs do so and clear it
        if(!doc["security"].isNull()){
            JsonArray users = docSecurity.to<JsonArray>();
            
            for(auto user: doc["security"].as<JsonArray>()){
                #if defined(DEBUG) && DEBUG > 3
                Serial.printf("Restoring user %s\n", user["username"].as<const char*>());                
                #endif
                users.add(user);
            }

            // Validate before touching the existing user database: never write a truncated
            // list, and never write one that would lock every user out.
            if(docSecurity.overflowed() || !isValidUserList(users)){
                failed = true;
                logger.logError("Restore rejected: user list in backup is empty or invalid");
            }
            // Keep usernames, roles and password hashes so everyone can still log in with their
            // password, but give each user a new session id. The backup holds the session ids
            // from when it was made, so restoring them as-is would bring back tokens that had
            // since been revoked by logging out or changing a password.
            if(!failed){
                esp32_authentication::assignNewSessionIds(users);
                if(docSecurity.overflowed()){
                    failed = true;
                    logger.logError("Restore rejected: user list too large");
                }
            }

            bool hadUsers = !failed && drive->exists(PATH_AUTH_FILE);
            if(hadUsers && !copySystemFile(PATH_AUTH_FILE, PATH_AUTH_FILE ".b")){
                failed = true;
                logger.logError("Restore aborted: could not back up current user database");
            }

            if(!failed){
                File security = drive->open(PATH_AUTH_FILE,"w");
                size_t written = serializeJson(docSecurity, security);        
                #if defined(DEBUG) && DEBUG > 3   
                Serial.printf("Serialized security (%d bytes in json) to file with %d bytes...\n", docSecurity.memoryUsage(), written);
                #endif
                security.close();

                //test and rollback if failed           
                File securityVerify = drive->open(PATH_AUTH_FILE,"r");
                DynamicJsonDocument docSec(docSecurity.capacity());
                auto testError = deserializeJson(docSec, securityVerify);
                securityVerify.close();
                if(written == 0 || testError.code() != DeserializationError::Ok || !isValidUserList(docSec.as<JsonArray>())){
                    failed = true;
                    Serial.printf("Error verifying restored user database: %s, rolling back\n", testError.c_str());
                }

                if(failed){ //rollback
                    if(hadUsers)
                        copySystemFile(PATH_AUTH_FILE ".b", PATH_AUTH_FILE);
                    else
                        drive->remove(PATH_AUTH_FILE);
                    #if defined(DEBUG)
                    Serial.println("Quitting restore!");             
                    #endif
                }
            }
            if(hadUsers){
                #if defined(DEBUG) && DEBUG > 1
                Serial.printf("Removing backup file: %s.\n", PATH_AUTH_FILE ".b");
                #endif
                drive->remove(PATH_AUTH_FILE ".b");
            }
            if(failed)
                response->setStatusCode(500);
        }
        if(!failed && !doc["config"].isNull()){
            Serial.println("Restoring config");

            if(drive->exists(PATH_SYSTEM_CONFIG))
                drive->remove(PATH_SYSTEM_CONFIG);
            docConfig = doc["config"].as<JsonObject>();
            File configFile = drive->open(PATH_SYSTEM_CONFIG,"w");
            serializeJson(docConfig, configFile);
            configFile.close();

            #if defined(DEBUG) && DEBUG > 3
            Serial.println("Saved System Configuration:");
            serializeJson(docConfig, Serial);
            Serial.println();
            #endif
        }
        if(!failed && !doc["public"].isNull()){            
            Serial.println("Restoring public pages");

            File publicPagesFile = drive->open(PATH_PUBLIC_PAGES,"w");
            for(auto page: doc["public"].as<JsonArray>())
                publicPagesFile.println(page.as<const char*>());
            publicPagesFile.close();
            
            #if defined(DEBUG) && DEBUG > 3
            Serial.println("Saved Public Pages:");
            serializeJson(doc["public"], Serial);
            Serial.println();
            #endif
        }
        if(!failed && !doc["devices"].isNull()){
            Serial.println("Restoring devices");

            docDevices = doc["devices"];
            File devicesFile = drive->open(PATH_DEVICE_CONFIG,"w");
            serializeJson(docDevices, devicesFile);
            devicesFile.close();

            #if defined(DEBUG) && DEBUG > 3
            Serial.println("Saved Devices:");
            serializeJson(docDevices, Serial);
            Serial.println();
            #endif
        }
        
        response->setStatusCode(failed ? 500 : 200);
        if(!failed)
            logger.logInfo(string_format("%s restored configuration to %s", request->getBasicAuthUser().c_str(), PATH_SYSTEM_CONFIG));
    } else{
        response->setStatusCode(500);
        // String errorText = "Error saving configuration: ";
        // errorText += error.c_str();
        response->setStatusText(error.c_str());
        #if defined(DEBUG)
        Serial.printf(error.c_str());
        #endif
    }    

    free(decryptedData);  

}


void esp32_config_controller::FactoryReset(HTTPRequest *request, HTTPResponse *response){
    if(strcmp(request->getHeader(HEADER_GROUP).c_str(),"ADMIN") != 0)
    {
        response->setStatusCode(401);
        return;
    }
    #if defined(DEBUG) && DEBUG > 2
    Serial.printf("\n\n********************\n** STARTING FACTORY RESET **\n********************\n");
    #endif
    auto drive = filesystem.getDisk(SYSTEM_DRIVE);

    if(drive->exists(PATH_SYSTEM_CONFIG))
        drive->remove(PATH_SYSTEM_CONFIG);

    if(drive->exists(PATH_DEVICE_CONFIG))
        drive->remove(PATH_DEVICE_CONFIG);

    if(drive->exists(PATH_PUBLIC_PAGES))
        drive->remove(PATH_PUBLIC_PAGES);

    
    static uint8_t  buf[FILESYSTEM_BUFFER_SIZE];
    //default system config
    File sourceConfig = drive->open(PATH_FACTORY_SYSTEM_CONFIG, FILE_READ);
    File destConfig = drive->open(PATH_SYSTEM_CONFIG, FILE_WRITE);

    int bytesRead = 0;
    while( true ) {
        bytesRead = sourceConfig.read( buf, FILESYSTEM_BUFFER_SIZE);
        if(bytesRead <= 0) break;
        destConfig.write( buf, bytesRead );
        
    }
    destConfig.close();
    sourceConfig.close(); 

    //default device config
    File sourceDevice = drive->open(PATH_FACTORY_DEVICE_CONFIG, FILE_READ);
    File destDevice = drive->open(PATH_DEVICE_CONFIG, FILE_WRITE);

    
    while( true ) {
        bytesRead = sourceDevice.read( buf, FILESYSTEM_BUFFER_SIZE);
        if(bytesRead <= 0) break;   
        destDevice.write( buf, bytesRead );
        
    }
    destDevice.close();
    sourceDevice.close(); 

    //default public config
    File sourcePublic = drive->open(PATH_FACTORY_PUBLIC_PAGES, FILE_READ);
    File destPublic = drive->open(PATH_PUBLIC_PAGES, FILE_WRITE);

    while( true ) {
        bytesRead = sourcePublic.read( buf, FILESYSTEM_BUFFER_SIZE);
        if(bytesRead <= 0) break;
        destPublic.write( buf, bytesRead );
        
    }
    destPublic.close();
    sourcePublic.close(); 

    if(drive->exists(PATH_AUTH_FILE))
        drive->remove(PATH_AUTH_FILE);

    response->setStatusCode(200);
    response->print("Reset");
    #if defined(DEBUG) && DEBUG > 2
    Serial.println("Reset controller to factory defaults. Resetting...");
    #endif
    ESP.restart();
}