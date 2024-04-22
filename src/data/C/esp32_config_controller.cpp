#include "esp32_config_controller.hpp"


DerivedController<esp32_config_controller> esp32_config_controller::reg("esp32_config");

void esp32_config_controller::List(HTTPRequest* req, HTTPResponse* res) {
    
    title = "Module Configuration Page";
    controllerTemplate.SetTemplateVariable("$_CUSTOM_MESSAGE", "<B>[Dynamic - Template Driven Data]</B>");

    int paramCount = req->getParams()->getQueryParameterCount();
    res->printf("<p class='debug-small'>Page from ip [%s] with [%u]parameters\n", req->getClientIP().toString().c_str(), paramCount);
    if (paramCount > 0) {
        auto ittr = req->getParams()->beginQueryParameters();
        for (int i = 0; i < paramCount; i++) {
            res->printf("\tParam [%s]: [%s]\n", ittr->first.c_str(), ittr->second.c_str());
            ittr++;
        }
    }
    res->printf("</p>");
}

