#include "_footer.hpp"
#include "System/ROUTER/esp32_template.h"


DerivedController<_footer> _footer::reg("_footer");

void _footer::Index(HTTPRequest* req, HTTPResponse* res) {
    
    controllerTemplate.SetTemplateVariable("$footer_year", "2020");
    
    //Print debug message
    controllerTemplate.PrintDebugMessage(req,res);
}

