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
	std::map<std::string, std::string> templateVars;
	
	bool RenderTemplate(HTTPRequest* req, HTTPResponse* res);


	void SetTemplateVariable(std::string name, std::string value);
	void SetGlobalVariables(HTTPRequest* req, HTTPResponse* res);

	std::pair<std::string, std::string> GetTemplateVariable(int idx);
	std::pair<std::string, std::string> GetTemplateVariable(std::string name);


	void PrintDebugMessage(HTTPRequest* req, HTTPResponse* res);
};

#endif
