#include "esp32_historic_controller.hpp"
#include "System/ROUTER/esp32_template.h"
#include "string_helper.h"
#include "System/CORE/esp32_config.hpp"
#include <loopback_stream.h>

DerivedController<esp32_historic_controller> esp32_historic_controller::reg("esp32_historic");

void esp32_historic_controller::Index(HTTPRequest* req, HTTPResponse* res) {
    
    auto drive = filesystem.getDisk(SYSTEM_DRIVE);
    File f = drive->open(PATH_DEVICE_CONFIG,"r");
    if(!f){
        Serial.printf("Failed to open config file on spiffs\n");
        return;
    }
    char buff[64];
    string deviceString= "";
    int bytesRead = 0;
    int totalBytesRead = 0;    
    do{
        bytesRead = f.readBytes(buff,sizeof(buff));
        deviceString.append(buff,bytesRead);
        totalBytesRead += bytesRead;
    } while(bytesRead > 0);
    f.close();
    controllerTemplate.SetTemplateVariable(F("$_DEVICES"),deviceString.c_str() );

    vector<esp32_file_info_extended> files;
    auto disk = filesystem.getDisk(logger.location());
    int filesFound = disk->search(files,PATH_LOGGING_ROOT, "SNAPSHOT_");

    if(filesFound > 0)
    {   
        string response;
        
        response = "[";
        int jsonIdx = 0;
        for(int idx = 0; idx < files.size(); idx++){  
            if(files[idx].size() > 0)         
                response += string_format("%s{\"name\": \"%s\"}",jsonIdx++ == 0 ? "": ", ", files[idx].name().c_str()).c_str();
        }
        response += "]";
        Serial.printf("Found %d log files: \n%s\n", filesFound, response.c_str());
        controllerTemplate.SetTemplateVariable(F("$_LOGDAYS"),response.c_str() );
    }
    esp32_base_controller::Index(req,res);      
}

/// @brief Overwrite Action since we have custom action implemented
/// @param req 
/// @param res 
void esp32_historic_controller::Action(HTTPRequest* req, HTTPResponse* res) {
    if (route.action.compare("Logs") == 0) {        
        Logs(req,res);        
    }
    else
        esp32_base_controller::Action(req,res);
}

void esp32_historic_controller::GetActions(vector<string> *actions){
    actions->push_back("Logs");
    esp32_base_controller::GetActions(actions);
}

bool esp32_historic_controller::HasAction(const char* action){
    if (strcmp(action,"Logs") == 0) {
        return true;
    }    
    
    else
        return esp32_base_controller::HasAction(action);

}

void esp32_historic_controller::Logs(HTTPRequest* req, HTTPResponse* res){

    auto params = req->getParams();
    auto disk = filesystem.getDisk(logger.location());
    //auto fs = logger.fs();
    if(disk == NULL){
        Serial.printf("Failed to load %s. Disk could not be mounted\n", req->getRequestString());
        return;
    }

    string filename;
    if(!params->getQueryParameter("file",filename)){
        res->print("Filename not specified in query. [file=]");
        res->setStatusCode(500);
        return;
    }
    filename = PATH_LOGGING_ROOT + filename; //prefix log path

    if(!disk->exists(filename.c_str())){
        res->printf("File %s not found!",filename.c_str());
        res->setStatusCode(404);
        return;
    }
    //Serial.printf("[Historic Controller] Getting log from file %s\n", filename.c_str());
    File f = disk->open(filename.c_str());

    if(!f){
        #ifdef DEBUG_FILESYSTEM
        Serial.printf("Error opening file %s!\n", filename.c_str());
        #endif
        return;
    } else {
        #ifdef DEBUG_FILESYSTEM
        Serial.printf("Opened file %s (%d bytes)\n", filename.c_str(), f.size());
        #endif
    }
    char buf[512];
    while(f.available()){
        int bytesRead = f.readBytes(buf,512);
        res->write((const uint8_t*)buf,bytesRead);
    }
    f.close();
    res->setStatusCode(200);
}

