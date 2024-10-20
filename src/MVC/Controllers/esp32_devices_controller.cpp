#include "esp32_devices_controller.hpp"
extern esp32_pin_manager pinManager;

DerivedController<esp32_devices_controller> esp32_devices_controller::reg("esp32_devices");

void esp32_devices_controller::Index(HTTPRequest * request, HTTPResponse * response){

    title = "Device Configuration Page";

    auto drive = filesystem.getDisk(SYSTEM_DRIVE);
    File f = drive->open(PATH_DEVICE_CONFIG,"r");
    if(!f){
        #if defined(DEBUG) && DEBUG > 0
        Serial.printf("Failed to open config file on spiffs\n");
        #endif
        return;
    }
    char buff[64];
    string configData= "";
    int bytesRead = 0;
    int totalBytesRead = 0;    
    do{
        bytesRead = f.readBytes(buff,sizeof(buff));
        configData.append(buff,bytesRead);
        totalBytesRead += bytesRead;
    } while(bytesRead > 0);
    f.close();

    controllerTemplate.SetTemplateVariable(F("$_DEVICE_DATA"), configData.c_str());

    auto pins = pinManager.getControllerPins();
    string pinData = "[";

    for(auto pin : pins){
        pinData.append(string_format(
            "{\"pin\": %d, \"analog\": %d, \"mode\": %d}, ",
            pin.gpioPin,
            pin.isAnalog?"":" not",
            pin.pinMode == GPIO_MODE_INPUT?" not":""
        ));
    }
    if(pinData.length() > 2) pinData = pinData.substr(0,pinData.length() - 2);
    pinData += "]";
    controllerTemplate.SetTemplateVariable(F("$_PIN_DATA"), pinData.c_str());
    
    esp32_base_controller::Index(request, response);    
    
}

void esp32_devices_controller::Post(HTTPRequest* request, HTTPResponse* response) {
    SaveDeviceData(request, response);   
}

void esp32_devices_controller::Action(HTTPRequest* request, HTTPResponse* response) {    
    if (route.action.compare("LoadDeviceData") == 0) {
        LoadDeviceData(request, response);
    }
    else if (route.action.compare("SaveDeviceData") == 0) {
        SaveDeviceData(request, response);
    }
    else if (route.action.compare("ResetDevice") == 0) {
        ResetDevice(request, response);
    }    
    else
        esp32_base_controller::Action(request, response);
}


bool esp32_devices_controller::HasAction(const char * action){
    if (strcmp(action, "LoadDeviceData") == 0) {
        return true;
    }
    
    else if (strcmp(action, "SaveDeviceData") == 0) {
        return true;
    }
    
    else if (strcmp(action, "ResetDevice") == 0) {
        return true;
    }

    else
        return esp32_base_controller::HasAction(action);
}



/// @brief Load configuration information from json file on disk
/// @param req 
/// @param res 
void esp32_devices_controller::LoadDeviceData(HTTPRequest* request, HTTPResponse* response) {
    auto drive = filesystem.getDisk(SYSTEM_DRIVE);
    File f = drive->open(PATH_DEVICE_CONFIG,"r");
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
bool esp32_devices_controller::SaveDeviceData(HTTPRequest* request, HTTPResponse* response){
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

    #ifdef DEBUG
    Serial.printf("Saving %i bytes to config\n%s\n", content.length(),content.c_str());
    #endif
    auto error = deserializeJson(doc, content);

    if(error.code() == DeserializationError::Ok){
       
        auto drive = filesystem.getDisk(SYSTEM_DRIVE);
        File f = drive->open(PATH_DEVICE_CONFIG, FILE_WRITE);
        if(f.available()){
            serializeJson(doc,f);
            f.close();
            response->setStatusCode(200);
            logger.logInfo(string_format("%s saved configuration to %s", request->getBasicAuthUser().c_str(), PATH_SYSTEM_CONFIG));
        } else{
            response->setStatusCode(500);
            // String errorText = "Error saving configuration: ";
            // errorText += error.c_str();
            response->setStatusText("Failed to open configuration file");
            #ifdef DEBUG
            Serial.printf(error.c_str());
            #endif
            return false;
        }
    } else{
        response->setStatusCode(500);
        // String errorText = "Error saving configuration: ";
        // errorText += error.c_str();
        response->setStatusText(error.c_str());
        #ifdef DEBUG
        Serial.printf(error.c_str());
        #endif
        return false;
    }    
    
    return true;
}

void esp32_devices_controller::ResetDevice(HTTPRequest* request, HTTPResponse* response){
    if(strcmp(request->getHeader(HEADER_GROUP).c_str(),"ADMIN") == 0){
        response->setStatusCode(200);
        response->finalize();
        esp_restart();
    }
    else response->setStatusCode(401);
}
