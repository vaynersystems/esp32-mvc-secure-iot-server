#include "esp32_config_controller.hpp"
#include "esp32_crypt.h"


DerivedController<esp32_config_controller> esp32_config_controller::reg("esp32_config");

void esp32_config_controller::Index(HTTPRequest* request, HTTPResponse* response) {
    
    title = "Module Configuration Page";
    auto drive = filesystem.getDisk(SYSTEM_DRIVE);

    File f = drive->open(PATH_SYSTEM_CONFIG,"r+w");
    if(!f){
        #if defined(DEBUG) && DEBUG > 0
        Serial.printf("Failed to open config file on %s\n", drive.label());
        #endif
        return;
    }
    char buff[64];
    string configData= "";
    int bytesRead = 0;
    do{
        bytesRead = f.readBytes(buff,sizeof(buff));
        configData.append(buff,bytesRead);
    } while(bytesRead > 0);

    
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
    byte buff[32];
    int bytesToRead = 0;
    while(true){
        bytesToRead = f.available() > sizeof(buff) ? sizeof(buff) : f.available();
        if(bytesToRead <= 0) break;
        f.readBytes((char*)buff,bytesToRead);
        response->write(buff,bytesToRead);
    }
    f.close();    

    response->setStatusCode(200);
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
    
    DynamicJsonDocument doc(length * 2);
    string content;
    char * buf = new char[32];
    while(true){
        
        int bytesRead = request->readBytes((byte*)buf,32); 
        if(bytesRead <= 0) break;       
        content.append(buf,bytesRead);
    }
    delete[] buf;

    #if defined(DEBUG) && DEBUG > 0
    Serial.printf("Saving %i bytes to config\n%s\n", content.length(),content.c_str());
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
    char * buf = new char[32];
    while(true){
        
        int bytesRead = request->readBytes((byte*)buf,32); 
        if(bytesRead <= 0) break;       
        content.append(buf,bytesRead);
    }
    delete[] buf;

    auto error = deserializeJson(doc, content);

    if(error.code() == DeserializationError::Ok){
        string deviceName = "", companyName = "", validFrom="", validTo="";
        //get certificate parameters
        if(doc["device"].isNull() || doc["company"].isNull()){
            response->println("Missing device or company. Please fill in fields and retry");
            return;
        }

        deviceName = doc["device"].as<const char*>();
        companyName = doc["company"].as<const char*>();
        validFrom = doc["from"].as<const char*>();
        validTo = doc["to"].as<const char*>();
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
    DynamicJsonDocument doc(8192);
    DynamicJsonDocument docPublic(2048);
    DynamicJsonDocument docSecurity(1024);
    DynamicJsonDocument docConfig(2048);
    DynamicJsonDocument docDevices(2048);

    auto drive = filesystem.getDisk(SYSTEM_DRIVE);

    File authorize = drive->open(PATH_AUTH_FILE, "r");
    deserializeJson(docSecurity,authorize);
    authorize.close();

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

    

    char enc_key[32] = {0}; //null terminated
    char enc_iv[16] =  {0};
    memcpy(enc_key,server.getCertificate()->getPKData(),sizeof(enc_key));
    memcpy(enc_iv,server.getCertificate()->getPKData(),sizeof(enc_iv));

    string backupData = "", backupDataEncrypted = "";
    unsigned char * encryptedData;
    serializeJson(doc,backupData);
    size_t dataLength = ecrypted_string_length(backupData.c_str());

    encryptedData = (unsigned char *)malloc(dataLength);

    encrypt_string(backupData.c_str(), (uint8_t*)enc_key,  (uint8_t*)enc_iv, encryptedData);
    Serial.printf("Encypted string of %d bytes to %d bytes.\n",
        backupData.length(), dataLength
    );
    
    response->write(encryptedData, dataLength);
    delete[] encryptedData;

    char dispStr[128];
    sprintf(dispStr, " attachment; filename = \"%s\"", "backup.json");
    response->setHeader("Content-Disposition", dispStr);    
    response->setHeader("Content-Type","application/octet-stream");
    response->setStatusCode(200);
}

void esp32_config_controller::Restore(HTTPRequest *request, HTTPResponse *response)
{
    if(strcmp(request->getHeader(HEADER_GROUP).c_str(),"ADMIN") != 0)
    {
        response->setStatusCode(401);
        return;
    }
    const int length = request->getContentLength();
    auto drive = filesystem.getDisk(SYSTEM_DRIVE);
    //DynamicJsonDocument doc(length * 2);
    DynamicJsonDocument doc(8192);
    DynamicJsonDocument docPublic(2048);
    DynamicJsonDocument docSecurity(2048);
    DynamicJsonDocument docConfig(2048);
    DynamicJsonDocument docDevices(2048);
    
    unsigned char * backupDataEncrypted  = (unsigned char *)malloc(length);
    memset((void *)backupDataEncrypted, 0, length + 1);
    unsigned char * buf = new unsigned char[FILESYSTEM_BUFFER_SIZE];
    size_t readLength = 0;
    size_t fieldLength = 0;
    while(true){            
        readLength = request->readBytes((byte*)buf,FILESYSTEM_BUFFER_SIZE); 
        if(readLength <= 0) continue;

        memcpy(&backupDataEncrypted[fieldLength], buf, readLength);
        fieldLength += readLength;
        
        if(request->requestComplete()) break;
    }     
    delete[] buf;
    
    #if defined(DEBUG) && DEBUG > 3
    #endif
    Serial.printf("Decrypting string of %d bytes. Read %d bytes\n", length, fieldLength);

    char enc_key[32] = {0}; //null terminated
    char enc_iv[16] =  {0};
    memcpy(enc_key,server.getCertificate()->getPKData(),sizeof(enc_key));
    memcpy(enc_iv,server.getCertificate()->getPKData(),sizeof(enc_iv));
    
    unsigned char * decryptedData = (unsigned char *)malloc(length + 1);
    memset(decryptedData, 0, length + 1);

    auto decryptedLength = decrypt_string(backupDataEncrypted, length, (const char*)enc_key,  (const char*)enc_iv, decryptedData);
    #if defined(DEBUG) && DEBUG > 3
    Serial.printf("Decrypted string of %d bytes\n", decryptedLength);
    #endif

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
                docSecurity.add(user);
            }
            File sourceFile = drive->open(PATH_AUTH_FILE);
            File destFile = drive->open(PATH_AUTH_FILE ".b", FILE_WRITE);
            static uint8_t buf[FILESYSTEM_BUFFER_SIZE];
            while( sourceFile.read( buf, FILESYSTEM_BUFFER_SIZE) ) {
                destFile.write( buf, FILESYSTEM_BUFFER_SIZE );
            }
            destFile.close();
            sourceFile.close(); 

            drive->remove(PATH_AUTH_FILE);
            
            File security = drive->open(PATH_AUTH_FILE,"w");
            //security.print(userStr.c_str());
            serializeJson(docSecurity, security);        
            #if defined(DEBUG) && DEBUG > 3   
            Serial.printf("Serialized security (%d bytes in json) to file with %d bytes...\n", docSecurity.memoryUsage(), security.position());
            #endif
            security.close();
            
            //test and rollback if failed           
            File securityVerify = drive->open(PATH_AUTH_FILE,"r");
            DynamicJsonDocument docSec(2048);
            auto testError = deserializeJson(docSec, securityVerify);
            securityVerify.close();
            if(testError.code() != DeserializationError::Ok || docSec.isNull()){
                failed = true;
                Serial.printf("Error occured deserializing updated security: %s, rolling back\n", testError.c_str());
                
            } else{
                Serial.printf("Sucessfully deserialized updated security: %s.\n", testError.c_str());
                for(auto userAccount : docSec.to<JsonArray>()){
                    if(userAccount["username"].isNull()){
                        Serial.printf("Error, username field not found");
                        failed = true;
                        break;
                    }
                }
            }
            if(failed){ //rollback
                File sourceFile = drive->open(PATH_AUTH_FILE ".b");
                File destFile = drive->open(PATH_AUTH_FILE , FILE_WRITE);
                static uint8_t buf[FILESYSTEM_BUFFER_SIZE];
                while( sourceFile.read( buf, FILESYSTEM_BUFFER_SIZE) ) {
                    destFile.write( buf, FILESYSTEM_BUFFER_SIZE );
                }
                destFile.close();
                sourceFile.close(); 
                response->setStatusCode(500);   
                #if defined(DEBUG)
                Serial.println("Quitting restore!");             
                #endif
                
            } else // commit / remove backup
            {
                #if defined(DEBUG) && DEBUG > 1
                Serial.println();
                Serial.printf("Removing backup file: %s.\n", PATH_AUTH_FILE ".b");
                #endif
                drive->remove(PATH_AUTH_FILE ".b");
            
            }
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