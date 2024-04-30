#include "esp32_home_controller.hpp"


DerivedController<esp32_home_controller> esp32_home_controller::reg("esp32_home");

void esp32_home_controller::Index(HTTPRequest* req, HTTPResponse* res) {
    
    title = "ESP32 Web Server Home Page";

    string ctrString = "[";
    int numOfControllers = BaseFactory::getInstanceCount();
    //Serial.printf("Found %i controllers\n", numOfControllers);
    for(int i=0;i< numOfControllers;i++){
        auto controller = BaseFactory::getInstanceAt(i);
        //res->printf("Controller %d of %d: %s\n",i, numOfControllers,controller.first.c_str());
        if(controller.first[0] == '_') continue;

        vector<string> actions = {};
        controller.second()->GetActions(&actions);
        Serial.printf("Controller %d of %d: %s with %i actions\n",i, numOfControllers,controller.first.c_str(), actions.size());
        for(const string& action : actions)     
             ctrString += "{\"" + controller.first + "\": \"" + action + "\"}, ";
    
    }
    if(ctrString.length() > 2) //trim off trailing comma
        ctrString = ctrString.substr(0,ctrString.length() - 2); 

        ctrString += "]";
    
    controllerTemplate.SetTemplateVariable("$_Controllers", ctrString.c_str());
    Base_Controller::Index(req,res);    
}

