#include "esp32_home.hpp"


DerivedController<esp32_home> esp32_home::reg("esp32_home");

void esp32_home::Index(HTTPRequest* req, HTTPResponse* res) {
    title = "ESP32 Web Server Home Page";

    DynamicJsonDocument doc(200);
    JsonArray arr;
    //link to controllers
    //res->printf("<p class='debug-small'>");
    int numOfControllers = BaseFactory::getInstanceCount();
    for(int i=0;i< numOfControllers;i++){
        auto controller = BaseFactory::getInstanceAt(i);
        //res->printf("Controller %d of %d: %s\n",i, numOfControllers,controller.first.c_str());
        arr.add(controller.first.c_str());
     } 

    doc["controllers"] = arr;
    String ctrString;
    serializeJsonPretty(doc, ctrString);
    controllerTemplate.SetTemplateVariable("$_Controllers", ctrString.c_str());
    //res->printf("</p>");
}

