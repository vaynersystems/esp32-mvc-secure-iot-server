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