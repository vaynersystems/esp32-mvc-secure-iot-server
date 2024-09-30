#ifndef _ESP32_FILE_DRIVE_H_
#define _ESP32_FILE_DRIVE_H_
//#define DEBUG_FILESYSTEM
#include "esp32_filesystem_objects.h"
#include "FS.h"
#include "vfs_api.h"

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
        return _openFiles > 0;
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
        auto fileInfo = esp32_file_info(path);

        if(fileInfo.drive() != _index){
            Serial.printf("Incorrect drive %s chosen to open file %s\n", partitionLabel, path);
        }
        //Serial.printf("Opening %s on filesystem %s\n", path, partitionLabel);
        string fileSystemPath = getRelativePath(path);

        

        #ifdef DEBUG_FILESYSTEM
        if(!exists(fileSystemPath.c_str())){
            Serial.printf("Path %s not found on drive %s\n", fileSystemPath.c_str(), partitionLabel);
        }
        #endif
        _openFiles++;
        auto file = _fileSystem->open(fileSystemPath.c_str(), mode, create);
        
        #ifdef DEBUG_FILESYSTEM
        Serial.printf("%s %s on filesystem %s with %d bytes\n", file ? "Opened" : "Failed to open", fileSystemPath.c_str(), partitionLabel, file.size());
        #endif
        return file;
        //return _workingFile;
    }  

    virtual bool close(File file){
        _openFiles--;
        if(file.available()) return false;
        file.close();
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
    int _openFiles = 0;
    //File _workingFile;
};
#endif