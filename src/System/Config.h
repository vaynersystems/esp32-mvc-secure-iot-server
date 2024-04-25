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

using namespace std;
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
};