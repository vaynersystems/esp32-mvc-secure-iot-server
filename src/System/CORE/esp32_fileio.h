#ifndef _ESP32_FILEIO_H
#define _ESP32_FILEIO_H
#include "FS.h"
#include "SPIFFS.h"
#include <list>
 //Hard(ish) drive
#include "FS.h"
#include "System/ROUTER/esp32_routing.h"
#include "HTTPResponse.hpp"
#include "HTTPMultipartBodyParser.hpp"
#include "string_extensions.h"

using namespace std;


struct SPIFFS_Info{
    int freeBytes;
    int usedBytes;
    int totalBytes;
};

struct SPIFFS_FileInfo{
    string parentDir;
    string filePath;
    bool isDirectory;
    size_t size;
};

enum HTTP_FORMAT {
    TEXT = 0,
    JSON = 1,
    HTML = 2,
    CSS = 3,
    XML = 4,
    JS = 5,
    XLT = 6,
    JPG = 7,
    JPEG = 8,
    PNG = 9,
    BMP = 10,   
};
static int SortByPath(SPIFFS_FileInfo first, SPIFFS_FileInfo second){
    return strcmp(first.filePath.c_str(),second.filePath.c_str());
}
//Class to interact with SPIFFS/LITTLEFS/SD attached to MCU
//TODO: abstract away from SPIFFS
class esp32_fileio
{
public:
    bool start();
    static esp32_route_file_info getFileInfo(fs::FS & fs, const char* filename);
    static esp32_route_file_info_extended getFileInfoExtended(fs::FS & fs, const char* filename);
    
    static int getFilesExtended(fs::FS& fs, vector<esp32_route_file_info_extended>& files,const char * dirName, const char* searchString = "");
    static int getFiles(fs::FS& fs, vector<esp32_route_file_info>& files,const char * dirName, const char* searchString = "");
	
    static int listDir(fs::FS& fs, Print* writeTo, const char* dirname, uint8_t levels, HTTP_FORMAT format = HTTP_FORMAT::TEXT, const char* searchString = "");
    static void buildOrderedFileList(fs::FS& fs, const char* dirname, const char * searchString, uint8_t levels, list<SPIFFS_FileInfo> &list);
    static SPIFFS_Info getMemoryInfo();
    //output data
    static void printFileSearchOrdered(Print* writeTo, list<SPIFFS_FileInfo>* files, string filter);
    
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
    static void writeFileToResponse(esp32_route_file_info  fileInfo, httpsserver::HTTPResponse * response);
protected:
    static bool isQualifiedPath(const char * path);
};

#endif

