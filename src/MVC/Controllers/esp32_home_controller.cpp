#include "esp32_home_controller.hpp"


DerivedController<esp32_home_controller> esp32_home_controller::reg("esp32_home");

void esp32_home_controller::Index(HTTPRequest* req, HTTPResponse* res) {
    
    title = "ESP32 Web Server Home Page";

    // string ctrString = "[";
    // int numOfControllers = BaseControllerFactory::getInstanceCount();
    // //Serial.printf("Found %i controllers\n", numOfControllers);
    // for(int i=0;i< numOfControllers;i++){
    //     auto controller = BaseControllerFactory::getInstanceAt(i);
    //     if(controller.first[0] == '_') continue;

    //     vector<string> actions = {};
    //     auto genController = controller.second();
    //     genController->GetActions(&actions);
    //     for(const string& action : actions)     
    //          ctrString += "{\"" + controller.first + "\": \"" + action + "\"}, ";

    //     actions.clear();
       
    //     delete genController; 
    // }
    
    // if(ctrString.length() > 2) //trim off trailing comma
    //     ctrString = ctrString.substr(0,ctrString.length() - 2); 

    //     ctrString += "]";
    
    // controllerTemplate.SetTemplateVariable(F("$_Controllers", ctrString.c_str());
    
    esp32_base_controller::Index(req,res);    
}

