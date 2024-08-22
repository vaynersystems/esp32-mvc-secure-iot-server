#include "esp32_home_controller.hpp"


DerivedController<esp32_home_controller> esp32_home_controller::reg("esp32_home");

void esp32_home_controller::Index(HTTPRequest* req, HTTPResponse* res) {
    
    title = "ESP32 Web Server Home Page";

    string ctrString = "[";
    int numOfControllers = BaseControllerFactory::getInstanceCount();
    //Serial.printf("Found %i controllers\n", numOfControllers);
    for(int i=0;i< numOfControllers;i++){
        auto controller = BaseControllerFactory::getInstanceAt(i);
        if(controller.first[0] == '_') continue;
        auto genController = controller.second();        
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
            case esp32_controller_category::_Internal:
            default:
                categoryStr = "Internal";
            break;
        }
        ctrString += "{\"group\": \"" + categoryStr + "\", \"name\": \"" + genController->GetName() + "\", \"controller\": \"" + controller.first + "\"}, ";

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
    
     controllerTemplate.SetTemplateVariable("$_Controllers", ctrString.c_str());
    
    esp32_base_controller::Index(req,res);    
}

