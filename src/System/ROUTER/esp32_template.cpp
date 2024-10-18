#include "esp32_template.h"
#include <iterator>
#include "WiFi.h"
#include "string_helper.h"

void esp32_template::SetTemplateVariable(const __FlashStringHelper *name, const char *value)
{
    auto nameString = string((const char*)(pgm_read_ptr(&name)));
	auto itr = templateVars.begin();
	for (auto element : templateVars) {
		if (element.first.c_str() != pgm_read_ptr(name)) continue;
		//if found
		element.second = value;
		return;
	}
    //Serial.printf("Adding parameter %s\n", nameString.c_str());
	templateVars.emplace(nameString, value);
}

// TODO: rework to "Set Model Variable". Need to be able to specify model path
void esp32_template::SetTemplateVariable(std::string name, std::string value)
{
	//auto itr = templateVars.begin();
	std::map<string,string>::iterator itr = templateVars.begin();
	for (pair<string,string> element : templateVars) {
		if (strcmp(element.first.c_str() , name.c_str()) != 0)  continue;
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
	SetTemplateVariable(F("$_TIME"), strftime_buf);
	SetTemplateVariable(F("$_CLIENT_IP"), req->getClientIP().toString().c_str());

	
	SetTemplateVariable(F("$_UPTIME"), String((uint32_t)(esp_timer_get_time() / 1000000)).c_str());
	SetTemplateVariable(F("$_SERVER_IP"), WiFi.localIP().toString().c_str());
	SetTemplateVariable(F("$_REQUEST_URL"), req->getRequestString().c_str());
    SetTemplateVariable(F("$_USERNAME"), req->getHeader(HEADER_USERNAME).c_str());
    SetTemplateVariable(F("$_USERROLE"), req->getHeader(HEADER_GROUP).c_str());
    SetTemplateVariable(F("$_FIRMWARE_VERSION"), FIRMWARE_VERSION);
    SetTemplateVariable(F("$_FIRMWARE_DATE"), FIRMWARE_DATE);
}

bool esp32_template:: RenderTemplate(HTTPRequest* req, HTTPResponse* res)
{
    unsigned long timer = millis();
    auto drive = filesystem.getDisk(SYSTEM_DRIVE);
	//update globals
	SetGlobalVariables(req,res);
    #ifdef DEBUG
    Serial.printf("Rendering template %s for request %s\n", templateContentFilePath.c_str(), req->getRequestString().c_str());
    #endif
	//open file
	
	size_t lastPos = 0;
	File templateFile = drive->open(templateContentFilePath.c_str());
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
			auto itr = templateVars.begin();
            while(itr != templateVars.end()){
            //for(int idx = 0; idx < templateVars.size();idx++){
			//for (auto element : templateVars) {
//                auto element = ;
				int idxStart = line.indexOf((*(itr)).first.c_str()); //check for variable
				if (idxStart > 0) //if found
				    line.replace((*(itr)).first.c_str(), (*(itr)).second.c_str());
                itr++;
			}
			res->println(line.c_str());               
		}
	}
	templateFile.close();
    #ifdef DEBUG
    Serial.printf("Completed rendering template in %d ms\n", millis() - timer);
    #endif
	return true;
}

// std::pair<std::string, std::string> esp32_template::GetTemplateVariable(int idx)
// {
// 	auto itr = templateVars.begin();
// 	for (int i = 0; i < idx; i++, itr++);
// 	return (*itr);
	
// }

// std::pair<std::string, std::string> esp32_template::GetTemplateVariable(std::string name)
// {
// 	std::map<std::string, std::string>::iterator itr = templateVars.begin();
// 	for (std::pair<std::string, std::string> element : templateVars) {
// 		if (element.first != name) continue;
// 		//if found
// 		return element;
// 	}
//     return std::pair<string,string>();
// }

// void esp32_template::PrintDebugMessage(HTTPRequest* req, HTTPResponse* res)
// {
// 	int paramCount = req->getParams()->getQueryParameterCount();
// 	string freeBytesPretty(""), totalBytesPretty("");
// 	esp32_fileio::PrettyFormat((size_t)esp_get_free_heap_size(), &freeBytesPretty);
// 	esp32_fileio::PrettyFormat((size_t)ESP.getHeapSize(), &totalBytesPretty);
// 	res->printf("<p class=\"debug-message\">VS Web Server v%s.  %s of %s  FREE.</p>\n",
// 		FIRMWARE_VERSION, freeBytesPretty.c_str(),totalBytesPretty.c_str());	
// }
