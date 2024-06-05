

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
        string pathS = this->path();
        isInternal = find(
            INTERNAL_PATHS.begin(),
            INTERNAL_PATHS.end(), 
            pathS.find_last_of('/') > 1 ? pathS.substr(0,pathS.find_last_of('/')) : "/"
        ) != INTERNAL_PATHS.end();   
        // auto drive = filesystem.getDisk(((esp32_file_info)this)._driveIdx);
        // if(!isInternal && strcmp(drive->label(),"spiffs") == 0 && strcmp(_path.substr(0,3).c_str(), string_format("%s/",PATH_SITE_ROOT).c_str()) != 0){
        // //prefix site root to route path
        //      _path = _path.length() > 1 ? string_format("%s%s", PATH_SITE_ROOT, _path.c_str()) : PATH_SITE_ROOT;
        //     _fullyQualifiedPath = string_format("/%s%s/%s",
        //         drive->label(), _path.c_str(), _name.c_str()
        //     );   
        // }
    }
    esp32_route_file_info(HTTPRequest * req) : esp32_route_file_info(req->getRequestString().c_str())
    {   
        isEditorRequest = strstr(req->getHeader("Refer").c_str(), "edit.html") != nullptr;          
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

// struct esp32_route_file_info_extended: public esp32_file_info_extended{
// public:    
//     esp32_route_file_info_extended(const char *path) : esp32_file_info_extended(path){
//         requestPath = path;
//         isEditorRequest = false;
//         isInternal = find(
//             INTERNAL_PATHS.begin(),
//             INTERNAL_PATHS.end(), 
//             this->path().find_last_of('/') > 1 ? this->path().substr(0,this->path().find_last_of('/')) : "/"
//         ) != INTERNAL_PATHS.end();    

//         auto drive = filesystem.getDisk(_driveIdx);
//         // Serial.printf("Checking if request is not internal: %s, volume %s is equal to spiffs and %s matches site root %s\n",
//         //     isInternal ? "Internal" : "Not internal",
//         //     drive->label(), 
//         //     _path.substr(0,3).c_str(),
//         //     string_format("%s/",PATH_SITE_ROOT).c_str()
//         // );
//         if(!isInternal && strcmp(drive->label(),"spiffs") == 0 && strcmp(_path.substr(0,3).c_str(), string_format("%s/",PATH_SITE_ROOT).c_str()) != 0){
//         //prefix site root to route path
//             _path = _path.length() > 1 ? string_format("%s%s", PATH_SITE_ROOT, _path.c_str()) : PATH_SITE_ROOT;
//             _fullyQualifiedPath = string_format("/%s%s/%s",
//                 drive->label(), _path.c_str(), _name.c_str()
//             );   
//         }
//     }
//     esp32_route_file_info_extended(HTTPRequest * req) : esp32_route_file_info_extended(req->getRequestString().c_str())
//     {   
//         isEditorRequest = strstr(req->getHeader("Refer").c_str(), "edit.html") != nullptr;          
//     }

//     /// @brief Full path requested (url)
//     string requestPath;
//     /// @brief The name of the file requested. This may be with or without .gz
//     // string fileName;
//     ///@brief File extension. If in zip format, provides extension of raw file
//     // string fileExtension;
//     /// @brief The file path used to retrieve the file from memory
//     //string filePath;
//     /// @brief Flag indicating if the request is to download the file
//     // bool isDownload;
//     /// @brief True if file being retrieved is in Gzip format. False otherwise
//     bool isEditorRequest;
//     bool isInternal;
    
// };

#endif