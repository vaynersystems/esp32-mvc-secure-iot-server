#include "esp32_config_controller.hpp"
#include "string_extensions.h"
#include <WiFi.h>
#include <System/AUTH/CERT/esp32_cert_base.hpp>
#include <System/CORE/esp32_server.h>

extern esp32_server server;
extern const char* PUBLIC_TEMP_PATH;
extern const char* PRIVATE_TEMP_PATH;
DerivedController<esp32_config_controller> esp32_config_controller::reg("esp32_config");

void esp32_config_controller::Index(HTTPRequest* req, HTTPResponse* res) {
    
    title = "Module Configuration Page";

    File f = SPIFFS.open(PATH_SYSTEM_CONFIG,"r");
    StaticJsonDocument<1024> configDoc;
    auto error = deserializeJson(configDoc, f);
    string configData;
    serializeJson(configDoc, configData);

    controllerTemplate.SetTemplateVariable("$_CONFIGURATION_DATA", configData.c_str());
    controllerTemplate.SetTemplateVariable("$_CONFIG_FILE", PATH_SYSTEM_CONFIG);
    
    esp32_base_controller::Index(req,res);    
}

void esp32_config_controller::Post(HTTPRequest* req, HTTPResponse* res) {
    SaveConfigData(req,res);
    Serial.println("Completed POST method");
}

/// @brief Overwrite Action since we have custom action implemented
/// @param req 
/// @param res 
void esp32_config_controller::Action(HTTPRequest* req, HTTPResponse* res) {
    if (route.action.compare("GetAvailableWifi") == 0) {
        GetAvailableWifi(req,res);
    }
    else if (route.action.compare("LoadConfigData") == 0) {
        LoadConfigData(req,res);
    }
    else if (route.action.compare("SaveConfigData") == 0) {
        SaveConfigData(req,res);
    }
    else if (route.action.compare("ResetDevice") == 0) {
        ResetDevice(req,res);
    }
    else if (route.action.compare("UploadCertificate") == 0) {
        UploadCertificate(req,res);
    }
    else if (route.action.compare("GenerateCertificate") == 0) {
        GenerateCertificate(req,res);
    }
    else
        esp32_base_controller::Action(req,res);
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
    
    else
        return esp32_base_controller::HasAction(action);
}




//custom action to get wifi list, or other list
void esp32_config_controller::GetAvailableWifi(HTTPRequest* req, HTTPResponse* res) {
     int n = WiFi.scanNetworks();
     StaticJsonDocument<1024> doc;
     JsonArray networks = doc.to<JsonArray>();

     for (int i = 0; i < n; ++i) {
        auto network = networks.createNestedObject();
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
        network["ssid"]= WiFi.SSID(i);
        network["rssi"]= WiFi.RSSI(i);
        network["channel"]= WiFi.channel(i);
        network["encryption"]= encryption;   
    }
    String outputstring;
    serializeJson(doc,outputstring);   
    res->print(outputstring.c_str());    
    res->setStatusCode(200);
}

/// @brief Load configuration information from json file on disk
/// @param req 
/// @param res 
void esp32_config_controller::LoadConfigData(HTTPRequest* req, HTTPResponse* res) {
    File f = SPIFFS.open(PATH_SYSTEM_CONFIG,"r");
    byte buff[32];
    int bytesToRead = 0;
    while(true){
        bytesToRead = f.available() > sizeof(buff) ? sizeof(buff) : f.available();
        if(bytesToRead <= 0) break;
        f.readBytes((char*)buff,bytesToRead);
        res->write(buff,bytesToRead);
    }
    f.close();    

    res->setStatusCode(200);
}

/// @brief Save configuration json to disk
/// @param req 
/// @param res 
/// @return 
bool esp32_config_controller::SaveConfigData(HTTPRequest* req, HTTPResponse* res){
    const int length = req->getContentLength();
    
    DynamicJsonDocument doc(length * 2);
    string content;
    Serial.printf("Saving configuration to %s...\n", PATH_SYSTEM_CONFIG);
    char * buf = new char[32];
    while(true){
        
        int bytesRead = req->readBytes((byte*)buf,32); 
        if(bytesRead <= 0) break;       
        content.append(buf,bytesRead);
    }
    delete[] buf;

    #ifdef DEBUG
    Serial.printf("Saving %i bytes to config\n%s\n", content.length(),content.c_str());
    #endif
    auto error = deserializeJson(doc, content);

    if(error.code() == DeserializationError::Ok){
        //if we need to apply certs do so and clear it
        if(!doc["server"]["certificates"].isNull() && !doc["server"]["certificates"]["uploaded"].isNull()){
            if(doc["server"]["certificates"]["uploaded"].as<bool>() == true){
                server.importCertFromTemporaryStorage();
                doc["server"]["certificates"]["uploaded"] = NULL;
            }
        }
        File f = SPIFFS.open(PATH_SYSTEM_CONFIG, "w");
        serializeJson(doc,f);
        f.close();
        res->setStatusCode(200);
        Serial.printf("Saved configuration to %s\n", PATH_SYSTEM_CONFIG);
    } else{
        res->setStatusCode(500);
        // String errorText = "Error saving configuration: ";
        // errorText += error.c_str();
        res->setStatusText(error.c_str());
        Serial.printf(error.c_str());
        return false;
    }    
    Serial.printf("Completed SaveConfigData, returning control\n");    
    
    return true;
}

void esp32_config_controller::ResetDevice(HTTPRequest* req, HTTPResponse* res){
    if(strcmp(req->getHeader(HEADER_GROUP).c_str(),"ADMIN") == 0)
        esp_restart();
    else res->setStatusCode(401);
}

void esp32_config_controller::UploadCertificate(HTTPRequest *req, HTTPResponse *res)
{
    Serial.printf("Uploading with action %s and parameter %s\n",
        route.action.c_str(), route.params.c_str());
    //req has post of file.
    if(strcmp(route.params.c_str(), "Public") == 0){
        esp32_router::handleFileUpload(req,res, PUBLIC_TEMP_PATH);
    } else if(strcmp(route.params.c_str(), "Private") == 0){
        esp32_router::handleFileUpload(req,res, PRIVATE_TEMP_PATH);
    }
    
}

void esp32_config_controller::GenerateCertificate(HTTPRequest *req, HTTPResponse *res)
{
    const int length = req->getContentLength();
    
    DynamicJsonDocument doc(length * 2);
    string content;
    Serial.printf("Generating certificate...\n");
    char * buf = new char[32];
    while(true){
        
        int bytesRead = req->readBytes((byte*)buf,32); 
        if(bytesRead <= 0) break;       
        content.append(buf,bytesRead);
    }
    delete[] buf;

    auto error = deserializeJson(doc, content);

    if(error.code() == DeserializationError::Ok){
        string deviceName = "", companyName = "", validFrom="", validTo="";
        //get certificate parameters
        if(doc["device"].isNull() || doc["company"].isNull()){
            res->println("Missing device or company. Please fill in fields and retry");
            return;
        }

        deviceName = doc["device"].as<const char*>();
        companyName = doc["company"].as<const char*>();
        validFrom = doc["from"].as<const char*>();
        validTo = doc["to"].as<const char*>();

        Serial.printf(".. parsed \n\tdevice: %s\n\t company %s\n\t valid-from: %s\n\t valid-to: %s\n",
            deviceName.c_str(), companyName.c_str(), validFrom.c_str(), validTo.c_str()
        );

        if(deviceName.length() > 0 && deviceName.length() < 32 &&
            companyName.length() > 0 && companyName.length() < 32 && 
            validFrom.length() == 14 && validTo.length() == 14)
            server.generateCertificate(deviceName.c_str(), companyName.c_str(), validFrom.c_str(), validTo.c_str());
        
    } else {
        Serial.printf("Error deserializing input: [%d]%s\n", error.code(), error.c_str());
    }
}
