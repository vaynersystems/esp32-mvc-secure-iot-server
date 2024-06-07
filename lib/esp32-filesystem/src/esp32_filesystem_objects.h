#ifndef _ESP32_FILESYSTEM_OBJECTS_H_
#define _ESP32_FILESYSTEM_OBJECTS_H_

/*  EXAMPLE #1: GZ PATH passed in request
        REQUEST PATH: /LOG/DIR1/SNAPSHOT_2025.log.gz?download=true&drive=1
                     |---PATH--|-----filename-------|--download---|-disk:sd-|
                     |---- FULLY QUALIFIED PATH ----|
        DISK PATH: /LOG/DIR1/SNAPSHOT_2025.log.gz
        Disk: SD 1

    // EXAMPLE #2: GZ PATH not passed in request, file stored on disk is of type gz
        // REQUEST PATH: /LOG/DIR1/SNAPSHOT_2025.log?download=true&drive=1
        //              |---PATH--|-----filename----|--|--download---|-disk:sd-|
        //              |---- FULLY QUALIFIED PATH ----|
        // DISK PATH: /LOG/DIR1/SNAPSHOT_2025.log.gz
        // Disk: SD 1
        // INSTEAD, USING GZ FILE DETERMINED A TIME OF REQUEST/RENDER FROM FILE
*/


#include "string_helper.h"
#include "FS.h"
using namespace std;
using namespace fs;

enum esp32_file_format {
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

enum esp32_drive_type{
    dt_Invalid = -1,
    dt_SPIFFS = 0,
    dt_SD = 1
};

struct esp32_drive_info{
    public:
    esp32_drive_info(){
        this->_size = 0;
        this->_used = 0;
        this->_type = dt_Invalid;
    }
    esp32_drive_info(esp32_drive_type type, size_t size, size_t used){
        this->_size = size;
        this->_type = type;
        this->_used = used;
    }
    inline esp32_drive_type type(){ return _type;}
    /// @brief Get size of drive in bytes
    /// @return Number of bytes
    inline size_t size(){ return _size;}
    /// @brief Get used bytes
    /// @return Number of used bytes
    inline size_t used(){ return _used;}

    private:
    esp32_drive_type _type;
    size_t _size;
    size_t _used;    
};

struct esp32_file_info{
    public:
    esp32_file_info(const char * path);
    esp32_file_info(string path) : esp32_file_info(path.c_str()){

    };
    esp32_file_info(const char* drive, const char* path);
    ~esp32_file_info(){
    }
    
    const string path(){ 
        //Serial.printf("Printing path %s. from fqn %s\n", _fullyQualifiedPath.substr(_pathIdx,_nameIdx - _pathIdx - 1).c_str(), _fullyQualifiedPath.c_str());
        return _fullyQualifiedPath.substr(_pathIdx,_nameIdx - _pathIdx - 1).c_str();
    };
    inline string name(){
        return _fullyQualifiedPath.substr(_nameIdx).c_str();
    };
    inline string fullyQualifiedPath() {
        return _fullyQualifiedPath;
    };
    inline const char* extension(){
        return _fullyQualifiedPath.substr(_extensionIdx).c_str();
    }
    inline int drive(){
        return _driveIdx;
    }
    inline bool isGZ(){
        return _isGZ;
    }
    inline bool isDownload(){
        return _isDownload;
    }
    
    
   
protected:
    string _fullyQualifiedPath ;
    int _pathIdx;
    int _nameIdx ;
    int _extensionIdx;
    bool _isGZ = false;
    bool _isDownload = false;
    int _driveIdx = 0;
};

struct esp32_file_info_extended: public esp32_file_info{
    public:
        esp32_file_info_extended(const char * path);
        esp32_file_info_extended(const char* drive, const char * path);
        esp32_file_info_extended(const char* drive, const char * path, size_t size, time_t lastUpdate);
        inline size_t size(){ return _size;};
        inline bool exists(){ return _exists;}
        inline time_t lastWrite(){ return _lastWrite;}
    private:
        size_t _size;
        bool _exists = false;
        time_t _lastWrite;
};

#endif