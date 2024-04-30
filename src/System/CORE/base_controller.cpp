#include "base_controller.hpp"
#include "../ROUTER/esp32_router.h"
BaseFactory::map_type* BaseFactory::map = NULL;

inline void Base_Controller::GenericIndex(HTTPRequest* req, HTTPResponse* res) {  
            //serve get request using template engine
            String path = SITE_ROOT;
            path += "/T/_template.html";
            File f = SPIFFS.open(path);
            res->setStatusCode(200);
            res->setStatusText("OK");
            res->setHeader("Content-Type", "text/html");
            Serial.printf("[esp32_router::handleRoot] Opening file %s\n", f.path());
            while (f.available()) {
                String line = f.readStringUntil('\n');
                //if we already processed a section, skip trying to reprocess
                int titleidx = 0,
                    headidx = 0,
                    menuidx = 0,
                    headeridx = 0,
                    contentidx = 0,
                    footeridx = 0;

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
                
                int idx = line.indexOf(HTML_REF_CONST_CONTENT);
                if (idx >= 0){ 
                    res->print(line.substring(0, idx));
                    if (!this->controllerTemplate.RenderTemplate(req,res))
                        esp32_router::handle404(req, res);//if not able to render template, handle with 404
                    res->println(line.substring(idx + sizeof(HTML_REF_CONST_CONTENT) - 1));

                }
                // esp32_router::handlePagePart_Content(req, res, line, controller);
                if (contentidx > 0) {
                continue;
                }
                footeridx = esp32_router::handlePagePart_Footer(req, res, line,footer);
                if (footeridx > 0) {
                    continue;
                }


                bool anyReplaced = titleidx >= 0 || headidx >= 0 || headeridx >= 0 || menuidx >= 0 || contentidx >= 0 || footeridx >= 0;
                if (!anyReplaced) {
                /* Serial.printf("[CONTENT WRITER] %s\n", line.c_str());*/
                    res->println(line);
                }

            }
            f.close();
        }