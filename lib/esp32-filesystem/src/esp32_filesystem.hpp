#ifndef _ESP32_FILESYSTEM_H_
#define _ESP32_FILESYSTEM_H_
#include "esp32_filesystem_objects.h"
#include "string_helper.h"
#include "FS.h"
#include "vfs_api.h"
#include <vector>
#include <SPIFFS.h>
#include "SD.h"


using namespace std;
using namespace fs;



class esp32_fs_impl : public VFSImpl
{
public:
    inline esp32_fs_impl(){};
    virtual ~esp32_fs_impl() { }
    inline bool exists(const char* path)
    {
        File f = open(path, FILE_READ,false);
        bool valid = (f == true);// && !f.isDirectory();
        f.close();
        return valid;
    }
};


class esp32_file_drive: public FS {
public:
    esp32_file_drive();
    esp32_file_drive(FS& disk, const char * label = NULL, int index = 0, esp32_drive_type type = dt_SPIFFS);
    inline const char* label(){
        return partitionLabel;
    }
    inline int index(){ return _index;}

    void list(
        const char * directory = "/",
        Print* writeTo = &Serial,
        esp32_file_format format = esp32_file_format::TEXT,
        const char* searchString = ""
    );

    int search(vector<esp32_file_info> &files, const char* directory = "/", const char * searchString = "");
    int search(vector<esp32_file_info_extended> &files, const char* directory = "/", const char * searchString = "");

    esp32_drive_info info();

    bool busy(){
        return _workingFile;
    }

    virtual bool exists(const char* path){
        string fileSystemPath = getRelativePath(path);
        return _fileSystem->exists(fileSystemPath.c_str());
    }
    virtual bool mkdir(const char * path){
        string fileSystemPath = getRelativePath(path);
        //check if parent exists, if not request to creat parent
        string parentPath = fileSystemPath.substr(0,fileSystemPath.find_last_of('/'));
        if(parentPath.length() > 0 && !_fileSystem->exists(parentPath.c_str())){
            if (!mkdir(parentPath.c_str())) return false; //if failed to create parent, return control reporting failure
        }
        bool created = _fileSystem->mkdir(fileSystemPath.c_str());
        //Serial.printf("Creating directory %s  %s\n", path, created ? "CREATED" : "FAILED");
        return created;
    }
    virtual bool rmdir(const char* path){
        string fileSystemPath = getRelativePath(path);
        return _fileSystem->rmdir(fileSystemPath.c_str());
    }
    virtual bool remove(const char* path){
        string fileSystemPath = getRelativePath(path);
        return _fileSystem->remove(fileSystemPath.c_str());
    }

    virtual bool create(const char* path){
        string fileSystemPath = getRelativePath(path);
        return !_fileSystem->exists(fileSystemPath.c_str()) && _fileSystem->open(fileSystemPath.c_str(),FILE_WRITE,true);
    }

    virtual File open(const char * path, const char* mode = FILE_READ, bool create = false){
        //Serial.printf("Opening %s on filesystem %s\n", path, partitionLabel);
        string fileSystemPath = getRelativePath(path);

        #ifdef DEBUG
        if(!exists(fileSystemPath.c_str())){
            Serial.printf("Path %s not found on drive %s\n", fileSystemPath.c_str(), partitionLabel);
        }
        #endif
        _workingFile = _fileSystem->open(fileSystemPath.c_str(), mode, create);
        #ifdef DEBUG
        Serial.printf("%s %s on filesystem %s with %d bytes\n", _workingFile ? "Opened" : "Failed to open", fileSystemPath.c_str(), partitionLabel, _workingFile.size());
        #endif
        return _workingFile;
    }  

    virtual bool close(){
        if(_workingFile.available()) return false;
        _workingFile.close();
        return true;
    }  
    
    virtual string getRelativePath(const char * path){
        string filePath = path;        
        // if(_workingFile && _workingFile.available()){
        //     Serial.printf("Cannot open file %s. %s is still open.\n", path, _workingFile.path());
        //     return _workingFile;
        // }
        if(strstr(path,string_format("/%s/", partitionLabel).c_str()) != nullptr)
        {
            //Serial.printf("Trimming off drive %s from path %s\n", partitionLabel, path);
            filePath = filePath.substr(strlen(partitionLabel) + 1); //add one for leading /
        }
        return filePath;
    }
    
    virtual string getAbsolutePath(const char* path){
        return string_format("/%s%s", partitionLabel, path);
    }

    
private:
    const char * partitionLabel;
    FS* _fileSystem;
    esp32_drive_type _type;
    int _index;
    File _workingFile;
};


class esp32_file_system
{
public:
    void addDisk(FS &disk, const char* label, esp32_drive_type type = dt_SPIFFS);
    int driveCount();
    esp32_file_drive* getDisk(int index);
    esp32_file_drive* getDisk(const char* driveName);
private:
    vector<esp32_file_drive> _disks;
    esp32_file_drive _selectedDisk;
};
#endif