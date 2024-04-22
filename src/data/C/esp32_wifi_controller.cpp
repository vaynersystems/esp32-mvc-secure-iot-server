#include "esp32_wifi_controller.hpp"


DerivedController<esp32_wifi_controller> esp32_wifi_controller::reg("esp32_wifi");

void esp32_wifi_controller::List(HTTPRequest* req, HTTPResponse* res) {
    
    title = "Listing of WIFI";
    controllerTemplate.SetTemplateVariable("$_CUSTOM_MESSAGE", "Hi There friends!");
    
    controllerTemplate.SetTemplateVariable("$_IP_ADDRESS", req->getClientIP().toString().c_str());

    


    //int paramCount = req->getParams()->getQueryParameterCount();
    //res->printf("Wifi Countreer Response from List method from ip [%s] with [%u]parameters\n", req->getClientIP().toString().c_str(),paramCount);
    //if (paramCount > 0) {
    //    auto ittr = req->getParams()->beginQueryParameters();       
    //    for (int i = 0; i < paramCount; i++) {
    //        res->printf("\tParam %d [%s:%s]\n",i+1,ittr->first.c_str(), ittr->second.c_str());
    //        ittr++;
    //    }
    //}
}

