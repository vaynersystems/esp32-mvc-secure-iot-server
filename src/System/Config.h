#pragma once
#include <string>
#define FIRMWARE_VERSION "0.03"
#define SITE_ROOT "/W"
#define TEMPLE_VAR_COUNT 50

#define HTML_REF_CONST_TITLE "$title"
#define HTML_REF_CONST_HEAD "$doc_head"
#define HTML_REF_CONST_HEADER "$header"
#define HTML_REF_CONST_MENU "$menu"
#define HTML_REF_CONST_CONTENT "$content"
#define HTML_REF_CONST_FOOTER "$footer"

#define HEADER_USERNAME "X-USERNAME"
#define HEADER_GROUP    "X-GROUP"
#define HEADER_AUTH "Authorization"


struct esp32_controller_route {
public:
	std::string controller;
	std::string action;
	std::string params;

	esp32_controller_route() {
		controller.clear();
		action.clear();
		params.clear();
	}
	esp32_controller_route(esp32_controller_route* rOriginal) {
		controller = rOriginal->controller;
		action = rOriginal->action;
		params = rOriginal->params;
	}
};