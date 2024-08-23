#include "_menu_int_controller.hpp"
#include "System/ROUTER/esp32_template.h"


DerivedController<_menu_int_controller> _menu_int_controller::reg("_menu_int");

void _menu_int_controller::Index(HTTPRequest* req, HTTPResponse* res) {
    controllerTemplate.SetTemplateVariable("$_Controllers", GetControllersJSON().c_str());
}

