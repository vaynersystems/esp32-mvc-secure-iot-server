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
        
    }  else if(_type == dt_SDMMC)    {
        auto *fs = (SDMMCFS*)this->_fileSystem;
        return esp32_drive_info(_type, fs->totalBytes(), fs->usedBytes());
        
    }else {
        log_e("Error occured, %s has an unknown file system type", this->label());
    }
}

void esp32_file_drive::list(
    const char * directory, 
    Print* writeTo,
    esp32_file_format format,
    const char* searchString,
    int maxFilesPerDirectory,
    int maxLevels 
)
{
    _max_files_per_dir = maxFilesPerDirectory;
    _max_listing_levels = maxLevels;
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
                #ifdef DEBUG_FILESYSTEM
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
    
    auto root = _fileSystem->open(_type == esp32_drive_type::dt_SPIFFS ? "/" : directory);
    if(!root){
        Serial.printf("Failed to search for %s in %s. Directory %s not found!\n", searchString, directory, directory);
        return 0;
    } 
    Serial.printf("Searching for %s in %s\n", searchString, directory);
    File pointerFile;
    pointerFile = root.openNextFile();
    do{
        if(pointerFile){
            Serial.printf("\tComparing %s to %s\n", searchString, pointerFile.path());
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
    if(levels > _max_listing_levels) return 0; // if its nested more than 5 levels, escape
    ESP_LOGI("ESP32 FS", "Searching for %s in %s", strlen(searchString) == 0 ? "*" : searchString, directory);
    auto root = _fileSystem->open(_type == esp32_drive_type::dt_SPIFFS ? "/" : directory);
    if(!root){
        return 0;
    }     
    File pointerFile;
    pointerFile = root.openNextFile();
    do{
        if(filesfound > _max_files_per_dir){
            files.push_back(esp32_file_info_extended(" this dir has more files", pointerFile.path(), 0, pointerFile.getLastWrite()));
            break;
        }
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
               
                else{
                    files.push_back(esp32_file_info_extended(this->label(), pointerFile.path(), pointerFile.size(), pointerFile.getLastWrite()));
                    filesfound++;
                }
            
            }
            
            pointerFile = root.openNextFile();
        }
    }while(pointerFile);
    root.close();
    return files.size();
}
