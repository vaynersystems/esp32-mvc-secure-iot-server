

#ifndef _ESP32_ROUTING_H
#define _ESP32_ROUTING_H
#include "HTTPSServer.hpp"
#include <string>
#include "esp32_filesystem.hpp"
using namespace std;
using namespace httpsserver;
extern esp32_file_system filesystem;
const static vector<string> INTERNAL_PATHS = {PATH_INTERNAL_ROOT, PATH_LOGGING_ROOT, PATH_TEMP_ROOT};
struct esp32_service_route{
    string service;
    string params;
    //string response;

    esp32_service_route() {
		service.clear();
		params.clear();
		//response.clear();
	}
	esp32_service_route(esp32_service_route* rOriginal) {
		service = rOriginal->service;
		params = rOriginal->params;
		//response = rOriginal->response;
	}
};
struct esp32_controller_route {
public:
	string controller;
	string action;
	string params;

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

template<typename T>
struct esp32_route_file_info : public T{
public:    
    esp32_route_file_info(const char *path) : T(path){
        requestPath = path;
        isEditorRequest = false;
        string pathS = path;
        isInternal = find(
            INTERNAL_PATHS.begin(),
            INTERNAL_PATHS.end(), 
            pathS.find_last_of('/') > 1 ? pathS.substr(0,pathS.find_last_of('/')) : "/"
        ) != INTERNAL_PATHS.end(); 
    }
    esp32_route_file_info(HTTPRequest * req) : esp32_route_file_info(urlDecode(req->getRequestString()).c_str())
    {   
        isEditorRequest = strstr(req->getHeader("Refer").c_str(), "edit") != nullptr;          
    }
    /// @brief Full path requested (url)
    string requestPath;
    /// @brief The name of the file requested. This may be with or without .gz
    // string fileName;
    ///@brief File extension. If in zip format, provides extension of raw file
    // string fileExtension;
    /// @brief The file path used to retrieve the file from memory
    //string filePath;
    /// @brief Flag indicating if the request is to download the file
    // bool isDownload;
    /// @brief True if file being retrieved is in Gzip format. False otherwise
    bool isEditorRequest;
    bool isInternal;

    
};
#endif