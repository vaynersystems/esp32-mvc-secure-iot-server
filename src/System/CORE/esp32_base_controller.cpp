#include "esp32_base_controller.hpp"
#include "../ROUTER/esp32_router.h"
BaseControllerFactory::map_type* BaseControllerFactory::controllerMap = NULL;

inline void esp32_base_controller::GenericIndex(HTTPRequest* req, HTTPResponse* res) {  
    //serve get request using template engine
    auto info = esp32_file_info(this->controllerTemplate.templateContentFilePath.c_str());
    auto file = filesystem.getDisk(info.drive())->open(string_format("%s%s", PATH_SITE_ROOT, "/T/_template.html").c_str());
    #ifdef DEBUG
    Serial.printf("Template file %s result %s\n", info.path().c_str(), file ? " Open " : "Failed");
    #endif
    //File f = SPIFFS.open(string_format("%s%s", PATH_SITE_ROOT, "/T/_template.html").c_str());
    res->setStatusCode(200);
    res->setStatusText("OK");
    res->setHeader("Content-Type", "text/html");
    String line ;
    int titleidx = 0,
        headidx = 0,
        menuidx = 0,
        headeridx = 0,
        contentidx = 0,
        footeridx = 0;
    #ifdef DEBUG
    Serial.printf("[esp32_base_controller::handleRoot] Opening file %s\n", file.path());
    #endif
    while (file.available()) {
        line = file.readStringUntil('\n');
        //if we already processed a section, skip trying to reprocess
        

        titleidx = esp32_router::handlePagePart_Title(req, res, line, title);
        if (titleidx > 0) {
            continue;
        }
        headidx = esp32_router::handlePagePart_Head(req, res, line, head);
        if (headidx > 0) {
            continue;
        }
        menuidx = esp32_router::handlePagePart_Menu(req, res, line, menu);
        if (menuidx > 0) {
        continue;
        }
        headeridx = esp32_router::handlePagePart_Header(req, res, line,"");//TODO: oops .. missed that one
        if (headeridx > 0) {
            continue;
        }
        
        contentidx = line.indexOf(HTML_REF_CONST_CONTENT);
        if (contentidx >= 0){ 
            esp32_router::handlePagePart_Content(req, res, line, this);
            // res->print(line.substring(0, contentidx));
            // if (!this->controllerTemplate.RenderTemplate(req,res))
            //     esp32_router::handle404(req, res);//if not able to render template, handle with 404
            // res->println(line.substring(contentidx + sizeof(HTML_REF_CONST_CONTENT) - 1));

        }
        if (contentidx > 0) {
            continue;
        }
        footeridx = esp32_router::handlePagePart_Footer(req, res, line,footer);
        if (footeridx > 0) {
            continue;
        }


        bool anyReplaced = titleidx > 0 || headidx > 0 || headeridx > 0 || menuidx > 0 || contentidx > 0 || footeridx > 0;
        if (!anyReplaced) {
        /* Serial.printf("[CONTENT WRITER] %s\n", line.c_str());*/
            res->println(line);
        }

    }
    file.close();
    
}

inline std::string esp32_base_controller::GetControllersJSON(){
    string ctrString = "[";
    int numOfControllers = BaseControllerFactory::getInstanceCount();
    //Serial.printf("Found %i controllers\n", numOfControllers);
    for(int i=0;i< numOfControllers;i++){
        auto controller = BaseControllerFactory::getInstanceAt(i);
        if(controller.first[0] == '_') continue;
        auto genController = controller.second();    

        //check if current role has access
        //genController.    
        string categoryStr = "";

        switch(genController->GetCategory()){
            case esp32_controller_category::Devices:
                categoryStr = "Devices";
            break;
            case esp32_controller_category::Extras:
                categoryStr = "Extras";
            break;            
            case esp32_controller_category::Site:
                categoryStr = "Site";
            break;
            case esp32_controller_category::Tools:
                categoryStr = "Tools";
            break;
            case esp32_controller_category::Users:
                categoryStr = "Users";
            break;
            case esp32_controller_category::Account:
                categoryStr = "Account";
            break;
            case esp32_controller_category::_Internal:
            default:
                categoryStr = "Internal";
            break;
        }
        ctrString += "{\"group\": \"" + categoryStr + "\", \"sort\": \"" + string_format("%d", genController->GetCategory()).c_str() + "\", \"name\": \"" + genController->GetName() + "\", \"controller\": \"" + controller.first + "\"}, ";

        // vector<string> actions = {};
        
        // //ctrString+= "{\"" + genController->category}";
        // genController->GetActions(&actions);
        // for(const string& action : actions)     
        //      ctrString += "{\"" + controller.first + "\": \"" + action + "\"}, ";

        // actions.clear();
    
        delete genController; 
    }
    
    if(ctrString.length() > 2) //trim off trailing comma
        ctrString = ctrString.substr(0,ctrString.length() - 2); 

        ctrString += "]";

    return ctrString;
}