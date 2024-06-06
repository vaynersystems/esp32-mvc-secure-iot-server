#ifndef _ESP32_FILEIO_H
#define _ESP32_FILEIO_H
#include "FS.h"
#include "SPIFFS.h"
//#include "SD.h"

#include <list>

#include "System/Config.h"
#include "System/ROUTER/esp32_routing.h"
#include "HTTPResponse.hpp"
#include "HTTPMultipartBodyParser.hpp"
#include "string_helper.h"
#include <esp32_filesystem_objects.h>
#include <esp32_filesystem.hpp>

using namespace std;
// enum HTTP_FORMAT {
//     TEXT = 0,
//     JSON = 1,
//     HTML = 2,
//     CSS = 3,
//     XML = 4,
//     JS = 5,
//     XLT = 6,
//     JPG = 7,
//     JPEG = 8,
//     PNG = 9,
//     BMP = 10,   
// };
// static int SortByPath(FS_FileInfo first, FS_FileInfo second){
//     return strcmp(first.filePath.c_str(),second.filePath.c_str());
// }
//Class to interact with SPIFFS/LITTLEFS/SD attached to MCU
//TODO: abstract away from SPIFFS
class esp32_fileio
{
public:
    bool start();
    //static esp32_route_file_info getFileRouteInfo(const char* filename);
    // static esp32_route_file_info_extended getFileInfoExtended(fs::FS & fs, const char* filename);
    
    //output data
    //static void printFileSearchOrdered(Print* writeTo, list<FS_FileInfo>* files, string filter);
    
    static bool CreateFile(const char * filename);
    static size_t UpdateFile(const char * filename, httpsserver::HTTPMultipartBodyParser* parser, bool createIfNotFound = false);    
    static size_t UpdateFile(const char * filename, const char* message, bool createIfNotFound = false, int seek = 0);          
    static bool DeleteFile(const char * filename);
//helper methods
	static void PrettyFormat(size_t size, string* output) {
        int order = 0;
        //Serial.printf("Formatting %u\n",size);
        while (true) {
            if (pow(1024, order + 1) > size)
                break;
            order++;
        }

        *output = string_format("%6.2lf", size / pow(1024, order));
        *output += " ";
        switch (order) {
        case 0:
            *output += "bytes";
            break;
        case 1:
            *output += "KB";
            break;
        case 2:
            *output += "MB";
            break;
        case 3:
            *output += "GB";
            break;
        default:
            *output += "WTFB";
            break;

        }
    }
    static void writeFileToResponse(const char * filename, httpsserver::HTTPResponse * response);
    static void writeFileToResponse(esp32_route_file_info<esp32_file_info_extended> routeInfo, httpsserver::HTTPResponse *response);
protected:
    static bool isQualifiedPath(const char * path);

private:

    
    
};

#endif

