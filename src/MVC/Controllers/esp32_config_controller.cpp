#include "esp32_config_controller.hpp"



DerivedController<esp32_config_controller> esp32_config_controller::reg("esp32_config");

void esp32_config_controller::Index(HTTPRequest* request, HTTPResponse* response) {
    
    title = "Module Configuration Page";
    auto drive = filesystem.getDisk(SYSTEM_DRIVE);

    File f = drive->open(PATH_SYSTEM_CONFIG,"r+w");
    if(!f){
        #if defined(DEBUG) && DEBUG > 0
        Serial.printf("Failed to open config file on spiffs\n");
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
    // Serial.printf("Uploading with action %s and parameter %s\n",
    //     route.action.c_str(), route.params.c_str());
    //req has post of file.
    if(strcmp(route.params.c_str(), "Public") == 0){
        esp32_router::handleFileUpload(request, response, PUBLIC_TEMP_PATH);
    } else if(strcmp(route.params.c_str(), "Private") == 0){
        esp32_router::handleFileUpload(request, response, PRIVATE_TEMP_PATH);
    }
    
}

void esp32_config_controller::GenerateCertificate(HTTPRequest *request, HTTPResponse *response)
{
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

    serializeJson(doc,*response);

    char dispStr[128];
    sprintf(dispStr, " attachment; filename = \"%s\"", "backup.json");
    response->setHeader("Content-Disposition", dispStr);    
    response->setHeader("Content-Type","application/octet-stream");
    response->setStatusCode(200);
}

void esp32_config_controller::Restore(HTTPRequest *request, HTTPResponse *response)
{
    const int length = request->getContentLength();
    auto drive = filesystem.getDisk(SYSTEM_DRIVE);
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
        if(!doc["security"].isNull()){
            File security = drive->open(PATH_AUTH_FILE,"w");
            serializeJson(doc["security"], security);
            security.close();
        }
        if(!doc["config"].isNull()){
            File config = drive->open(PATH_SYSTEM_CONFIG,"w");
            serializeJson(doc["config"], config);
            config.close();
        }
        if(!doc["public"].isNull()){
            File publicPages = drive->open(PATH_PUBLIC_PAGES,"w");
            for(auto page: doc["public"].as<JsonArray>())
                publicPages.println(page.as<const char*>());
            publicPages.close();
        }
        if(!doc["devices"].isNull()){
            File devices = drive->open(PATH_DEVICE_CONFIG,"w");
            for(auto device: doc["devices"].as<JsonArray>())
                devices.println(device.as<const char*>());
            devices.close();
        }


        response->setStatusCode(200);
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

}
