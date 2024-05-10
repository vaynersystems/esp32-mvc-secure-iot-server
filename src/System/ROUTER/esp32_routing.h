

#ifndef _ESP32_ROUTING_H
#define _ESP32_ROUTING_H
#include <string>
using namespace std;

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

struct esp32_route_file_info{
public:
    /// @brief Full path requested (url)
    string requestPath;
    /// @brief The name of the file requested. This may be with or without .gz
    string fileName;
    ///@brief File extension. If in zip format, provides extension of raw file
    string fileExtension;
    /// @brief The file path used to retrieve the file from memory
    string filePath;
    /// @brief True if file exists
    bool exists;
    /// @brief Flag indicating if the request is to download the file
    bool isDownload;
    /// @brief True if file being retrieved is in Gzip format. False otherwise
    bool isGZ;

    esp32_route_file_info(){
        requestPath = "";
        fileName = "";
        fileExtension = "";
        filePath = "";
        exists = false;
        isDownload = false;
        isGZ = false;
    }
};

#endif