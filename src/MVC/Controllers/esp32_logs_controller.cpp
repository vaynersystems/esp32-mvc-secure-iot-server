#include "esp32_logs_controller.hpp"
#include "System/ROUTER/esp32_template.h"
#include "string_helper.h"
#include "System/CORE/esp32_config.hpp"
#include "loopback_stream.h"

DerivedController<esp32_logs_controller> esp32_logs_controller::reg("esp32_logs");

void esp32_logs_controller::Index(HTTPRequest* req, HTTPResponse* res) {
    
    vector<esp32_file_info_extended> files;
    auto drive = filesystem.getDisk(logger.location());
    #if defined(DEBUG) && DEBUG > 0
    Serial.printf("Searching %s for logs\n", drive->label());
    #endif

    drive->search(files,PATH_LOGGING_ROOT, "!SNAPSHOT_");
    
    string response;
    int jsonIdx = 0;
    response = "[";
    for(int idx = 0; idx < files.size(); idx++){
        if(files[idx].size() > 0)         
            response += string_format("%s{\"name\": \"%s\"}",jsonIdx++ == 0 ? "" : ", ", files[idx].name().c_str()).c_str();
    }
    response += "]";
    #if defined(DEBUG) && DEBUG > 0
    Serial.printf("Found the following log files \n%s\n", response.c_str());
    #endif
    controllerTemplate.SetTemplateVariable(F("$_LOGFILES"),response.c_str() );

    esp32_base_controller::Index(req,res);      
}

/// @brief Overwrite Action since we have custom action implemented
/// @param req 
/// @param res 
void esp32_logs_controller::Action(HTTPRequest* req, HTTPResponse* res) {
    if (route.action.compare("Logs") == 0) {        
        Logs(req,res);        
    }
    else
        esp32_base_controller::Action(req,res);
}

void esp32_logs_controller::GetActions(vector<string> *actions){
    actions->push_back("Logs");
    esp32_base_controller::GetActions(actions);
}

bool esp32_logs_controller::HasAction(const char* action){
    if (strcmp(action,"Logs") == 0) {
        return true;
    }    
    
    else
        return esp32_base_controller::HasAction(action);

}

void esp32_logs_controller::Logs(HTTPRequest* req, HTTPResponse* res){

    auto params = req->getParams();
    auto drive = filesystem.getDisk(logger.location());
    

    string filename;
    if(!params->getQueryParameter("file",filename)){
        res->print("Filename not specified in query. [file=]");
        res->setStatusCode(500);
        return;
    }
    filename = string_format("/%s%s%s", drive->label(), PATH_LOGGING_ROOT, filename.c_str()); //prefix log path

    //Serial.printf("Getting log file %s from drive %s\n",filename.c_str(), drive->label());

    if(!drive->exists(filename.c_str())){
        res->printf("File %s not found!",filename.c_str());
        res->setStatusCode(404);
        return;
    }
    esp32_fileio::writeFileToResponse(filename.c_str(),res);
    // File f = fs.open(filename.c_str());
    // char buf[512];
    // while(f.available()){
    //     int bytesRead = f.readBytes(buf,512);
    //     res->write((const uint8_t*)buf,bytesRead);
    // }
    // f.close();
    res->setStatusCode(200);
}

