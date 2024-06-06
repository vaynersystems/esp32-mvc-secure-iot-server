#ifndef _ESP32_TEMPLATE_H
#define _ESP32_TEMPLATE_H
#include "../Config.h"
#include <string>
#include <map>
#include "FS.h"
#include "SPIFFS.h"
#include <HTTPRequest.hpp>
#include <HTTPResponse.hpp>
#include <HTTPConnection.hpp>
#include "../CORE/esp32_fileio.h"
using namespace std;
using namespace httpsserver;
class esp32_template
{
public:
	//static std::string templateFilePath;
	std::string templateContentFilePath;
	//std::map<std::string, std::string> templateVars;
    std::map<std::string, std::string> templateVars;
    ~esp32_template(){
        templateContentFilePath = "";
        templateVars.clear();
        templateVars.~map();
    }
	
	bool RenderTemplate(HTTPRequest* req, HTTPResponse* res);

    void SetTemplateVariable(const __FlashStringHelper * name, const char* value);
	void SetTemplateVariable(std::string name, std::string value);
	void SetGlobalVariables(HTTPRequest* req, HTTPResponse* res);
    void ClearVariables();

	// std::pair<std::string, std::string> GetTemplateVariable(int idx);
	// std::pair<std::string, std::string> GetTemplateVariable(std::string name);


	//void PrintDebugMessage(HTTPRequest* req, HTTPResponse* res);
};

#endif

