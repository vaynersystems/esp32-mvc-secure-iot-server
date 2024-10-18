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
#include "System/Config.h"
#include "System/CORE/esp32_fileio.hpp"
#include "System/CORE/esp32_fileio.hpp"
#include "System/CORE/esp32_base_controller.hpp"
#include "System/CORE/esp32_base_service.hpp"
#include "System/AUTH/esp32_middleware.hpp"
#include "esp32_template.h"
#include "esp32_routing.h"
#include "esp32_filesystem.hpp"
#include "string_helper.h"

#include <HTTPSServer.hpp>
#include <HTTPRequest.hpp>
#include <HTTPResponse.hpp>

using namespace httpsserver;

//route class
class esp32_router
{

public:
	//static void InitConfigurableRouting();
	static void RegisterHandler(ResourceNode* resourceNode);
    static void RegisterHandler(String nodeMapPath, HTTPMETHOD method, HTTPSCallbackFunction* handler);
	static void RegisterHandler(String nodeMapPath, String method, HTTPSCallbackFunction* handler);
	//static void RegisterHandlers(fs::FS& fs, const char* dirname, uint8_t levels);
    static void RegisterWebsocket(WebsocketNode* resourceNode);

	static void handleRoot(HTTPRequest* req, HTTPResponse* res);
    static void handleFileUpload(HTTPRequest* req, HTTPResponse* res);	
	static void handleFileUpload(HTTPRequest* req, HTTPResponse* res, const char * overwriteFilePath = NULL);	
	static void handle404(HTTPRequest* req, HTTPResponse* res);
    static void handle401(HTTPRequest* req, HTTPResponse* res);

	static void handleFile(HTTPRequest* req, HTTPResponse* res);
    #if ENABLE_EDITOR
    static void handleEditor(HTTPRequest* req, HTTPResponse* res);
	#endif
	static void dummyPageHandler(HTTPRequest* req, HTTPResponse* res) {};
	static void handleCORS(HTTPRequest* req, HTTPResponse* res);
	static void handleFileList(HTTPRequest* req, HTTPResponse* res);

    /* Consider moving to template*/
	static int handlePagePart_Title(HTTPRequest* req, HTTPResponse* res, String line, std::string content);
	static int handlePagePart_Head(HTTPRequest* req, HTTPResponse* res, String line, std::string content );
	static int handlePagePart_Menu(HTTPRequest* req, HTTPResponse* res, String line, std::string content);
	static int handlePagePart_Header(HTTPRequest* req, HTTPResponse* res, String line, std::string content );
	static int handlePagePart_Content(HTTPRequest* req, HTTPResponse* res, String line, std::string content );
	static int handlePagePart_Footer(HTTPRequest* req, HTTPResponse* res, String line, std::string content );


	static int handlePagePart_FromFile(HTTPRequest* req, HTTPResponse* res, String line, const char* searchString, String fileName);
	static int handlePagePart_FromString(HTTPRequest* req, HTTPResponse* res, String line, const char* searchString, std::string content);

	static int handlePagePart_Content(HTTPRequest* req, HTTPResponse* res, String line, esp32_base_controller* controller);
    static void handleControllerRequest(HTTPRequest* req, HTTPResponse* res, esp32_controller_route route);
    
    static string handleServiceRequest(esp32_service_route route);

protected:
	static DerivedController<esp32_base_controller>* controllerFactory;
    static DerivedService<esp32_base_service>* serviceFactory;
    
private:
	static bool GetControllerRoute(HTTPRequest* request, esp32_controller_route& routeObj);

    static void writeFileToResponse(HTTPRequest* req, HTTPResponse* res);
    static bool IsValidRoute(esp32_controller_route & route);
};


#endif

