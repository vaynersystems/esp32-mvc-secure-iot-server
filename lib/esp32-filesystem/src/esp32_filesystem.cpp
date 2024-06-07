#include "esp32_filesystem.hpp"
#include "esp32_filesystem_objects.h"

esp32_file_drive::esp32_file_drive() :
    FS(FSImplPtr(new esp32_fs_impl())), 
    partitionLabel(NULL),
    _fileSystem(&SPIFFS)
{ }

esp32_file_drive::esp32_file_drive(FS &disk, const char* label, int index, esp32_drive_type type):
    FS(FSImplPtr(new esp32_fs_impl())), 
    partitionLabel(label),
    _fileSystem(&disk),
    _index(index),
    _type(type)
{ }

esp32_drive_info esp32_file_drive::info(){    
    if(_type == dt_SPIFFS)
    {
        auto *fs = (SPIFFSFS*)this->_fileSystem;
        return esp32_drive_info(_type, fs->totalBytes(), fs->usedBytes());        
    } else if(_type == dt_SD)    {
        auto *fs = (SDFS*)this->_fileSystem;
        return esp32_drive_info(_type, fs->totalBytes(), fs->usedBytes());
        
    }else {
        log_e("Error occured, %s has an unknown file system type", this->label());
    }
}

void esp32_file_drive::list(
    const char * directory, 
    Print* writeTo,
    esp32_file_format format,
    const char* searchString
)
{
    char time_buf[20];
    //time_t tm;
    vector<esp32_file_info_extended> files;
    int filesFound =  search(files, directory, searchString);

    switch(format){
        case JSON:

            writeTo->print("[");
            for(int idx = 0; idx < files.size(); idx++){            
                time_t tm = files[idx].lastWrite();
                strftime(time_buf, 20, "%Y-%m-%d %H:%M:%S", localtime(&tm));
                string fileInfo = string_format("%s{\"type\": \"file\", \"name\":\"%s\", \"size\": %lu,\"parent_dir\": \"%s\", \"last_modified\":\"%s\"}", 
                    idx == 0 ? "" : ", ",
                    files[idx].name().c_str(), 
                    files[idx].size(),                     
                    files[idx].fullyQualifiedPath().substr(0,files[idx].fullyQualifiedPath().length() - files[idx].name().length() - 1).c_str(), //everything before /filename.ext
                    time_buf
                );
                #ifdef DEBUG
                Serial.printf("%s\n", fileInfo.c_str());
                #endif
                writeTo->printf(  fileInfo.c_str());
                
            }
            writeTo->print("]");     

        break;

        default:
            int totalSize = 0;
            writeTo->printf("Listing directory: %s\r\n", directory);

            for(int idx = 0; idx < files.size(); idx++){            
                time_t tm = files[idx].lastWrite();
                strftime(time_buf, 20, "%Y-%m-%d %H:%M:%S", localtime(&tm));
                writeTo->printf("\n---\nFILE:   %s\nSIZE:   %lu\nCREATED: %s\nPATH:   %s\nFQP:    %s\n", 
                    files[idx].name().c_str(), 
                    files[idx].size(), time_buf, 
                    files[idx].path().c_str(), 
                    files[idx].fullyQualifiedPath().c_str()
                ); 
                totalSize += files[idx].size();           
            }
        break;
    }
    
}

int esp32_file_drive::search(vector<esp32_file_info> &files, const char *directory, const char *searchString)
{
    if(!_fileSystem) return 0;
    int filesfound = 0; 
    auto root = _fileSystem->open(directory);
    if(!root){
        return 0;
    } 
    File pointerFile;
    pointerFile = root.openNextFile();
    do{
        if(pointerFile){
            if(strstr(pointerFile.path(), directory) == pointerFile.path() //begining match
            
                && (strlen(searchString) == 0 ||
                    (strlen(searchString) > 0 && (
                            (searchString[0] == '!' && strstr(pointerFile.name(),string(searchString).substr(1).c_str()) == nullptr) ||
                            (searchString[0] != '!' && strstr(pointerFile.name(),searchString) != nullptr) 
                        )
                    )
                )
            ){
                if(pointerFile.isDirectory())
                    search(files, pointerFile.path(), searchString);
               
                else
                    files.push_back(esp32_file_info(this->label(), pointerFile.path()));
            
            }
            
            pointerFile = root.openNextFile();
        }
    }while(pointerFile);
    root.close();
    return files.size();
}

int esp32_file_drive::search(vector<esp32_file_info_extended> &files, const char *directory, const char *searchString)
{
    if(!_fileSystem) return 0;
    int filesfound = 0; 
    byte levels = 0;
    for(int idx=0;idx<strlen(directory);idx++)
        if(directory[idx] == '/') levels++;
    if(levels > 5) return 0; // if its nested more than 5 levels, escape
    auto root = _fileSystem->open(directory);
    if(!root){
        return 0;
    } 
    File pointerFile;
    pointerFile = root.openNextFile();
    do{
        if(pointerFile){            
            if(strstr(pointerFile.path(), directory) == pointerFile.path() //begining match
            
                && (strlen(searchString) == 0 ||
                    (strlen(searchString) > 0 && (
                            (searchString[0] == '!' && strstr(pointerFile.name(),string(searchString).substr(1).c_str()) == nullptr) ||
                            (searchString[0] != '!' && strstr(pointerFile.name(),searchString) != nullptr) 
                        )
                    )
                )
            ){
                if(pointerFile.isDirectory())
                    search(files, pointerFile.path(), searchString);
               
                else
                    files.push_back(esp32_file_info_extended(this->label(), pointerFile.path(), pointerFile.size(), pointerFile.getLastWrite()));
            
            }
            
            pointerFile = root.openNextFile();
        }
    }while(pointerFile);
    root.close();
    return files.size();
}

void esp32_file_system::addDisk(FS &disk, const char* label, esp32_drive_type type)
{
    
    for(int idx = 0; idx < _disks.size();idx++){
        if(strcmp(_disks[idx].label(), label) == 0){
            Serial.printf("Disk %s already added. Skipping.\n", label);
            return;
        }
    }
    
    _disks.push_back(esp32_file_drive(disk, label,_disks.size(),type));
}

int esp32_file_system::driveCount()
{
    return _disks.size();
}

esp32_file_drive* esp32_file_system::getDisk(int index)
{
    return &_disks[index];
}

esp32_file_drive* esp32_file_system::getDisk(const char *driveName)
{
    for(int idx = 0; idx < _disks.size();idx++){
        if(strcmp(_disks[idx].label(), driveName) == 0){            
            return &_disks[idx];
        }
    }
    return NULL;
}
extern esp32_file_system filesystem;

/// @brief Parses file information from path without verifying file information on disk
/// @param path Path to file
esp32_file_info::esp32_file_info(const char *path)
{    
    //Serial.printf("ESP32 file info: hydrate for path: %s\n", path);
    if(path == NULL) return;
    string fullPath = string(path);
    string driveLabel = "spiffs";
    bool drivePassed = false;
    int drivePartLength = driveLabel.length();
    _driveIdx = 0;
    _isGZ = false;
    //parse and apply query parameters
    int queryIdx = fullPath.find_first_of('?');
    if(queryIdx > 0){           
        auto params = explode(fullPath.substr(queryIdx + 1),"&",true);
        for(int idx = 0; idx < params.size(); idx++){
            auto keyValue = explode(params[idx], "=",true);
            if(keyValue.size() == 1){
                if(strcmp(keyValue[0].c_str(),"download") == 0)
                    _isDownload = true;
            }
            else if(keyValue.size() == 2){
                if(strcmp(keyValue[0].c_str(),"download") == 0)
                    _isDownload = strcmp(keyValue[1].c_str(),"true") == 0;
                else if(strcmp(keyValue[0].c_str(),"drive") == 0){
                    drivePassed = true;
                    _driveIdx = atoi(keyValue[1].c_str());
                    auto drive = filesystem.getDisk(_driveIdx);
                    driveLabel = drive->label();       
                    //Serial.printf("Request passed drive as query parameter %s - %s\n", keyValue[1].c_str(), driveLabel.c_str());
                }
            }
        }
        //update working path
        fullPath = fullPath.substr(0,queryIdx).c_str();
    }

    //if drive not explicitly passed in parameter, see if its first part of path
    //if(!drivePassed){ //!!!could be both
        auto parts= explode(fullPath.c_str(),"/", true);
        if(parts.size() > 1){
            auto drive = filesystem.getDisk(parts[1].c_str());
            if(drive != NULL){ //drive prefix in PATH
                //Serial.printf("Found matching drive %s at index %d\n", drive->label(),  drive->index());
                _driveIdx = drive->index();
                driveLabel = drive->label();            
                fullPath = fullPath.substr(parts[1].length() + 1); //trim filesystem from path            
            } else{
                //Serial.printf("Did not find matching drive %s\n", parts[1].c_str());
            }        
        }
    //}

    //specify length of drive including '/' prefix
    drivePartLength = driveLabel.length() + 1;
    //requested gz file. mark as such
    if(fullPath.length() > 3 && strcmp(fullPath.substr(fullPath.length() - 3).c_str(), ".gz") == 0){
        //Serial.printf("Requesting GZ file %s\n",fullPath.c_str());
        _isGZ = true;      
    }
    

    //set path and filename
    auto lastSplitIdx = fullPath.find_last_of("/");
    _pathIdx = drivePartLength;
    if(lastSplitIdx == -1){
        fullPath = "/" + fullPath;        
        _nameIdx = drivePartLength;
        
    }
    else if(lastSplitIdx == 0){                   
       
       _nameIdx = drivePartLength + 1;

    } else{        
        _nameIdx = drivePartLength + lastSplitIdx + 1;

    }

    if(fullPath.find_last_of('.') > 0){
        //Serial.printf("Found extension %s at index %d in %s\n", fullPath.substr(fullPath.find_last_of('.') + 1).c_str(), fullPath.find_last_of('.') + 1 + drivePartLength, fullPath.c_str()); 
        _extensionIdx = fullPath.find_last_of('.') + 1 + drivePartLength;
    }

   
    _fullyQualifiedPath = string_format("/%s%s", driveLabel.c_str(), fullPath.c_str() );
    #ifdef DEBUG
    Serial.printf("Parsed Path %s file %s on disk %d. FQP: %s\n", 
        fullPath.c_str(), 
        fullPath.substr(fullPath.find_last_of('/')).c_str(),
        _driveIdx,
        _fullyQualifiedPath.c_str()
    );
    #endif
    //Serial.printf("Setting FQP to %s\n.", _fullyQualifiedPath.c_str());
    
}

esp32_file_info::esp32_file_info(const char *drive, const char *path): esp32_file_info(("/" + string(drive) + string(path)).c_str())
{
} /// @brief Gets file information from path and details from file on disk
/// @param path Path to file
esp32_file_info_extended::esp32_file_info_extended(const char *path): esp32_file_info(path){
    
    //check if file requested is not gz, but stored on disk as GZ
    auto drive = filesystem.getDisk(_driveIdx);

    if(!_isGZ){
    //check if file on disk is GZ file     
        string gzPath = string_format("%s.gz", _fullyQualifiedPath.c_str());
        //Serial.printf("Checking if GZ file %s exists on disk...\n", gzPath.c_str());  
        
        bool foundGzVersion = drive->exists(gzPath.c_str());
        if(foundGzVersion){
            //Serial.printf("Found GZ version of file %s\n", gzPath.c_str());
            //update fully qualified path and gz flag
            _isGZ = true;
            _fullyQualifiedPath = gzPath;
        }
    }
    // #ifdef DEBUG
    // Serial.printf("Getting extended info for file %s from drive %d: %s\n",
    //     _fullyQualifiedPath.c_str(),
    //     _driveIdx, 
    //     drive->label()
    // );
    // #endif
    //verify file and get size
    _exists = drive->exists(_fullyQualifiedPath.c_str());
    if(_exists){
        auto file = drive->open(_fullyQualifiedPath.c_str(), "r",false);
        if(!file){
            _exists = false;
            return;
        }
        _size = file.size();
        _lastWrite = file.getLastWrite();    
        file.close();
    }
    
    
}

esp32_file_info_extended::esp32_file_info_extended(const char *drive, const char *path)
    : esp32_file_info_extended(("/" + string(drive) + string(path)).c_str())

{
}

esp32_file_info_extended::esp32_file_info_extended(
    const char *drive, const char *path, size_t size, time_t lastUpdate
) :  esp32_file_info(drive, path)
{
    _size = size;
    _lastWrite = lastUpdate;
}
