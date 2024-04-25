#ifndef _ESP32_ROUTER_H
#define _ESP32_ROUTER_H

#include <string>

//route info


enum HTTPMETHOD {
	HTTPMETHOD_GET = 0,
	HTTPMETHOD_POST = 1,
	HTTPMETHOD_PUT = 2,
	HTTPMETHOD_DELETE = 3
};


#include "FS.h"
#include "SPIFFS.h"
#include "../CORE/esp32_fileio.h"
#include "../Config.h"
#include "../CORE/base_controller.hpp"
#include "../AUTH/esp32_middleware.h"
#include "esp32_template.h"

#include <HTTPSServer.hpp>
#include <HTTPRequest.hpp>
#include <HTTPResponse.hpp>
using namespace httpsserver;
//#include <SSLCert.hpp>
//#include <ArduinoJson.h>
//#include <HTTPBodyParser.hpp>
//#include <HTTPWebkitBodyParser.hpp>
//#include <HTTPURLEncodedBodyParser.hpp>

//route class
class esp32_router
{

public:
	static void InitConfigurableRouting();
	static void RegisterHandler(ResourceNode* resourceNode);
	static void RegisterHandler(String nodeMapPath, HTTPMETHOD method, HTTPSCallbackFunction* handler);
	static void RegisterHandler(String nodeMapPath, String method, HTTPSCallbackFunction* handler);
	static void RegisterHandlers(fs::FS& fs, const char* dirname, uint8_t levels);

	static void handleRoot(HTTPRequest* req, HTTPResponse* res, std::string* content);
	static void handleRoot(HTTPRequest* req, HTTPResponse* res) { esp32_router::handleRoot(req, res, nullptr); }
	static void handleFileUpload(HTTPRequest* req, HTTPResponse* res);
	static void handleInternalPage(HTTPRequest* req, HTTPResponse* res);
	static void handleAdminPage(HTTPRequest* req, HTTPResponse* res);
	static void handlePublicPage(HTTPRequest* req, HTTPResponse* res);
	static void handle404(HTTPRequest* req, HTTPResponse* res);

	static void handleFile(HTTPRequest* req, HTTPResponse* res);
	
	static void dummyPageHandler(HTTPRequest* req, HTTPResponse* res) {};
	static void handleCORS(HTTPRequest* req, HTTPResponse* res);
	static void handleFileList(HTTPRequest* req, HTTPResponse* res);


	static int handlePagePart_Title(HTTPRequest* req, HTTPResponse* res, String line, std::string content);
	static int handlePagePart_Head(HTTPRequest* req, HTTPResponse* res, String line, std::string content );
	static int handlePagePart_Menu(HTTPRequest* req, HTTPResponse* res, String line, std::string content);
	static int handlePagePart_Header(HTTPRequest* req, HTTPResponse* res, String line, std::string content );
	static int handlePagePart_Content(HTTPRequest* req, HTTPResponse* res, String line, std::string content );
	static int handlePagePart_Footer(HTTPRequest* req, HTTPResponse* res, String line, std::string content );


	static int handlePagePart_FromFile(HTTPRequest* req, HTTPResponse* res, String line, const char* searchString, String fileName);
	static int handlePagePart_FromString(HTTPRequest* req, HTTPResponse* res, String line, const char* searchString, std::string content);

	static int handlePagePart_Content(HTTPRequest* req, HTTPResponse* res, String line, Base_Controller* controller);

protected:
	static DerivedController<Base_Controller>* controllerFactory;
    static esp32_route_file_info parseFileRequestInfo(const char * requestPath){
        bool useGZ = false;
        bool download = false;
        esp32_route_file_info fileRequestInfo;

        fileRequestInfo.requestPath = requestPath;
        String fileName = String(requestPath);
        if (!fileName.startsWith(SITE_ROOT))
            fileName = SITE_ROOT + fileName;
        fileName = urlDecode(fileName.c_str()).c_str();
        
        //first we need to trim off any request parameters
        int endIdx = fileName.indexOf("?");
        if (endIdx > 0) {
            if (fileName.substring(endIdx + 1).compareTo("download=true") == 0)
                fileRequestInfo.isDownload = true;
            fileName = fileName.substring(0, endIdx);
        }

        fileRequestInfo.fileName = fileName.c_str();

        //check if file requested is in gzip format
        if(fileName.endsWith(".gz")){
            fileRequestInfo.fileName = fileName.substring(fileName.lastIndexOf('.') + 1).c_str();
            fileRequestInfo.isGZ = true;
        }

        fileRequestInfo.fileExtension = 
            fileName.substring(fileName.lastIndexOf('.') + 1).c_str();

        String zipFileName = fileRequestInfo.isGZ ? fileName : fileName + ".gz";

        if(SPIFFS.exists(zipFileName.c_str())){
            //zip file found
            fileRequestInfo.exists = true;
            fileRequestInfo.filePath = zipFileName.c_str();
            fileRequestInfo.isGZ = true;
        } else if(!fileRequestInfo.isGZ && SPIFFS.exists(fileName.c_str())){
            //regular file found
            fileRequestInfo.exists = true;
            fileRequestInfo.filePath = fileName.c_str();
            fileRequestInfo.isGZ = false;            
        } else{
            //file not found
            fileRequestInfo.exists = false;
        }

        Serial.printf("[3.2] Loading file %s from path %s with extension %s in %s format%s.\n", 
            fileRequestInfo.fileName.c_str() , 
            fileRequestInfo.filePath.c_str(), 
            fileRequestInfo.fileExtension.c_str(),
            fileRequestInfo.isGZ ? "zipped" : "raw",
            download ? " with DOWNLOAD flag" : ""
        );
    }
private:
	static bool GetControllerRoute(HTTPRequest* request, esp32_controller_route& routeObj);

    static void writeFileToResponse(HTTPRequest* req, HTTPResponse* res);
    static bool IsValidRoute(esp32_controller_route & route);
};

#endif

