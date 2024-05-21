#include "esp32_historic_controller.hpp"
#include "System/ROUTER/esp32_template.h"
#include "string_extensions.h"
#include <System/CORE/esp32_config.h>
#include <loopback_stream.h>

DerivedController<esp32_historic_controller> esp32_historic_controller::reg("esp32_historic");

void esp32_historic_controller::Index(HTTPRequest* req, HTTPResponse* res) {
    
    //this page will render charts for devices.
    // set device config to template variable
    StaticJsonDocument<2048> doc;
    esp32_config::getConfigSection("devices",&doc);
    string deviceString = "";
    serializeJson(doc, deviceString);
    controllerTemplate.SetTemplateVariable("$_DEVICES",deviceString );

    loopback_stream buffer(512);
    esp32_fileio::listDir(SPIFFS, &buffer, PATH_LOGGING_ROOT, 1, HTTP_FORMAT::JSON,"SNAPSHOT_");   
    
    ostringstream oss;
    string response;
    char buf[512];
    while(buffer.available()){
        int bytesRead = buffer.readBytes(buf,512);
        oss.write(buf,bytesRead);
    }
    response = oss.str();
    //Serial.printf("Found the following log files \n%s\n", response.c_str());
    controllerTemplate.SetTemplateVariable("$_LOGDAYS",response.c_str() );

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
    string filename;
    if(!params->getQueryParameter("file",filename)){
        res->print("Filename not specified in query. [file=]");
        res->setStatusCode(500);
        return;
    }
    filename = PATH_LOGGING_ROOT + filename; //prefix log path

    if(!SPIFFS.exists(filename.c_str())){
        res->printf("File %s not found!",filename.c_str());
        res->setStatusCode(404);
        return;
    }
    File f = SPIFFS.open(filename.c_str());
    char buf[512];
    while(f.available()){
        int bytesRead = f.readBytes(buf,512);
        res->write((const uint8_t*)buf,bytesRead);
    }
    f.close();
    res->setStatusCode(200);
}

