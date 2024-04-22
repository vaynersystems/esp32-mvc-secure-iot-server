#include "_footer_controller.hpp"
#include "System/ROUTER/esp32_template.h"


DerivedController<_footer_controller> _footer_controller::reg("_footer");

void _footer_controller::Index(HTTPRequest* req, HTTPResponse* res) {
    controllerTemplate.SetTemplateVariable("$footer_year", "2024");    
    //Print debug message
    //controllerTemplate.PrintDebugMessage(req,res);
}

