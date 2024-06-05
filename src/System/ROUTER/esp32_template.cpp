#include "esp32_template.h"
#include <iterator>
#include "WiFi.h"
#include "string_helper.h"

//TODO: rework to "Set Model Variable". Need to be able to specify model path
void esp32_template::SetTemplateVariable(std::string name, std::string value)
{
	//auto itr = templateVars.begin();
	std::map<std::string, std::string>::iterator itr = templateVars.begin();
	for (std::pair<std::string, std::string> element : templateVars) {
		if (element.first != name) continue;
		//if found
		element.second = value;
		return;
	}

	templateVars.emplace(name, value);
}

void esp32_template::ClearVariables(){
    //roll through and destruct
    // std::map<std::string, std::string>::iterator itr = templateVars.begin();
	// for (std::pair<std::string, std::string> element : templateVars) {
	// 	element.first.~basic_string();
    //     element.second.~basic_string(); 		
	// }
    templateVars.clear();
}

void esp32_template::SetGlobalVariables(HTTPRequest* req, HTTPResponse* res) {
	time_t now;
	char strftime_buf[64];
	struct tm timeinfo;

	time(&now);

	localtime_r(&now, &timeinfo);
	strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
	SetTemplateVariable("$_TIME", strftime_buf);
	SetTemplateVariable("$_CLIENT_IP", req->getClientIP().toString().c_str());

	
	SetTemplateVariable("$_UPTIME", String((uint32_t)(esp_timer_get_time() / 1000000)).c_str());
	SetTemplateVariable("$_SERVER_IP", WiFi.localIP().toString().c_str());
	SetTemplateVariable("$_REQUEST_URL", req->getRequestString().c_str());
    SetTemplateVariable("$_USERNAME", req->getHeader(HEADER_USERNAME).c_str());
    SetTemplateVariable("$_USERROLE", req->getHeader(HEADER_GROUP).c_str());
}

bool esp32_template:: RenderTemplate(HTTPRequest* req, HTTPResponse* res)
{
    unsigned long timer = millis();
	//update globals
	SetGlobalVariables(req,res);
    #ifdef DEBUG
    Serial.printf("Rendering template %s for request %s\n", templateContentFilePath.c_str(), req->getRequestString().c_str());
    #endif
	//open file
	
    //try gz
    // if(SPIFFS.exists(string_format("%s.gz", templateContentFilePath.c_str()).c_str())){
    //     templateContentFilePath += ".gz";
    //     res->setHeader("Content-Encoding", "gzip");
    // } else if (!SPIFFS.exists(templateContentFilePath.c_str())) {
    //     res->printf("Template %s not found! \n", templateContentFilePath.c_str());            
    //     return false;
    // }
	size_t lastPos = 0;
	File templateFile = SPIFFS.open(templateContentFilePath.c_str());
    //Serial.printf("Opened file %s\n", templateContentFilePath.c_str());
	if (templateFile.size() > 0) {
        
        // Serial.printf("Reading %d bytes from %s\n", templateFile.size(), templateFile.path());
		while (templateFile.position() < templateFile.size()) {
			//have data to read
            if(lastPos == templateFile.position() && lastPos > 0){ //move forward one
                Serial.printf("Failed to seek at position %d. Quitting abnornally\n", lastPos);
                break;
            }
            //Serial.printf("Reading at position %d\n", templateFile.position());
            lastPos =   templateFile.position();       
			//for each line, seach for each template variable.
			// if found, replace just the template variable part of the string
			String line = templateFile.readStringUntil('\n');
			std::map<std::string, std::string>::iterator itr = templateVars.begin();
			for (std::pair<std::string, std::string> element : templateVars) {
				int idxStart = line.indexOf(element.first.c_str()); //check for variable
				if (idxStart < 0) continue; //not found; check for next variable                
				//if found
				line.replace(element.first.c_str(), element.second.c_str());
			}
			res->println(line.c_str());               
		}
	}
	templateFile.close();
    Serial.printf("Completed rendering template in %d ms\n", millis() - timer);
	return true;
}

std::pair<std::string, std::string> esp32_template::GetTemplateVariable(int idx)
{
	std::map<std::string, std::string>::iterator itr = templateVars.begin();
	for (int i = 0; i < idx; i++, itr++);
	return (*itr);
	
}

std::pair<std::string, std::string> esp32_template::GetTemplateVariable(std::string name)
{
	std::map<std::string, std::string>::iterator itr = templateVars.begin();
	for (std::pair<std::string, std::string> element : templateVars) {
		if (element.first != name) continue;
		//if found
		return element;
	}
    return std::pair<string,string>();
}

void esp32_template::PrintDebugMessage(HTTPRequest* req, HTTPResponse* res)
{
	int paramCount = req->getParams()->getQueryParameterCount();
	string freeBytesPretty(""), totalBytesPretty("");
	esp32_fileio::PrettyFormat((size_t)esp_get_free_heap_size(), &freeBytesPretty);
	esp32_fileio::PrettyFormat((size_t)ESP.getHeapSize(), &totalBytesPretty);
	res->printf("<p class=\"debug-message\">VS Web Server v%s.  %s of %s  FREE.</p>\n",
		FIRMWARE_VERSION, freeBytesPretty.c_str(),totalBytesPretty.c_str());	
}
