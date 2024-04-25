#include "esp32_fileio.h"

#include "../ROUTER/esp32_router.h"

bool esp32_fileio::start(){
    if (!SPIFFS.begin(true)) {
        Serial.println("SPIFFS Mount Failed");
        return false;
    }
    else {
        String sizeText, usedText;
        esp32_fileio::PrettyFormat(SPIFFS.totalBytes(), &sizeText);
        esp32_fileio::PrettyFormat(SPIFFS.usedBytes(), &usedText);
        //Serial.printf("Mounted vol %s. Used %zu bytes of %zu.\n",
        Serial.printf("Mounted vol %s. Used %s bytes of %s.\n",
            "[DEFAULT]", usedText, sizeText);

        esp32_fileio::listDir(SPIFFS, &Serial,"/", 2);
        Serial.println("-----------------------");
    }
    return true;
}

SPIFFS_Info esp32_fileio::getMemoryInfo(){
    SPIFFS_Info info;
    info.totalBytes = SPIFFS.totalBytes();
    info.usedBytes = SPIFFS.usedBytes();
    info.freeBytes = info.totalBytes - info.usedBytes;
    return info;
}

void esp32_fileio::listDir(fs::FS& fs, Print* writeTo, const char* dirname, uint8_t levels, HTTP_FORMAT format) {
    if (format == HTTP_FORMAT::JSON) {
        bool first = true;
        File root = fs.open(dirname);
        if (!root) {
            return;
        }
        if (!root.isDirectory()) {
            return;
        }
        writeTo->print("[");
        File file = root.openNextFile();
        while (file) {
            if (file.isDirectory()) {
                writeTo->print(file.name());
                if (levels) {
                    esp32_fileio::listDir(fs, writeTo, file.name(), levels - 1,format);
                }
            }
            else {
                if (first) {
                    first = false;
                }
                else writeTo->print(",");

                char buff[20];
                time_t lastWrite = file.getLastWrite();
                strftime(buff, 20, "%Y-%m-%d %H:%M:%S", localtime(&lastWrite));
                writeTo->printf("{\"type\": \"file\", \"name\":\"%s\", \"size\": %d, \"last_modified\":\"%s\"}", file.name() + 1, file.size(), buff);
            }
            file = root.openNextFile();
        }
        writeTo->print("]");
    }
    else
    {
        size_t totalSize = 0;
        writeTo->printf("Listing directory: %s\r\n", dirname);

        File root = fs.open(dirname);
        if (!root) {
            writeTo->println("- failed to open directory");
            return;
        }
        if (!root.isDirectory()) {
            writeTo->println(" - not a directory");
            return;
        }

        File file = root.openNextFile();
        uint8_t fileLenMax = 31;
        while (file) {
            if (file.isDirectory()) {
                writeTo->print("  DIR : ");
                writeTo->println(file.name());
                if (levels) {
                    listDir(fs, writeTo, file.name(), levels - 1,format);
                }
            }
            else {
                writeTo->print("  FILE: ");
                writeTo->print(file.name());
                for (int i = 0; i < fileLenMax - strlen(file.name()); i++)
                    writeTo->print(' ');
                writeTo->print("SIZE: ");
                writeTo->println(file.size());
                totalSize += file.size();
            }
            file = root.openNextFile();
        }
        file.close();
        String sizeStr;
        PrettyFormat(totalSize, &sizeStr);
        writeTo->printf("Total of %s bytes\n", sizeStr);
    }

    
}
/// @brief Support whildcard search.
/// @param fs File System Object
/// @param dirname Path to search
/// @param levels Levels of directories to traverse
/// @param list Object to be filled with file information
/// @param returnDirs set to true if you want to include directories in the list
void esp32_fileio::buildOrderedFileList(fs::FS& fs, const char* dirname, const char * searchString,uint8_t levels, list<SPIFFS_FileInfo>* list, bool returnDirs) {
    std::list<SPIFFS_FileInfo> files;
    std::list<SPIFFS_FileInfo>::iterator it;
    File root = fs.open(dirname);
    if (!root) {
        //Serial.printf("Failed to open requested path %s. Quitting...", dirname);
        return;
    }
    if (!root.isDirectory() && returnDirs) {
        //Serial.printf("Expected path [ %s] to be a directory. Quitting...", dirname);    
        return;
    }
    Serial.printf("[esp32_fileio::buildOrderedFileList] getting next file after %s %s in %s\n", root.isDirectory() ? "dir" : "file", root.name(), root.path());
    File file = root.openNextFile();
    while (file) {
        //file name shorter than search string or not matching
        if(strlen(searchString) > 0 && ( file.isDirectory() || strlen(file.name()) < strlen(searchString) || strstr(file.name(), searchString) == NULL))
        {
            //...
        }else {
            //add file/dir to list
            Serial.printf("Adding %s %s to file list\n",file.isDirectory() ? "dir" : "file", file.path());
            SPIFFS_FileInfo f;
            f.filePath = file.path();
            f.isDirectory = file.isDirectory();
            f.parentDir = f.filePath.substr(0,f.filePath.find_last_of('/'));
            f.size = file.size();
            files.push_back(f);
        }
        
        if (file.isDirectory()) {            
            if (levels) { //read deeper
                //Serial.printf("Reading dir %s on level %d\n",file.name(),levels);
                esp32_fileio::buildOrderedFileList(fs, file.path(), searchString, levels - 1, &files);
            }
        }
                  
            
        file = root.openNextFile();
    }

    files.sort(SortByPath);

    //roll through files, check if dir record exists in list for *each* of its parents, if not, create.

    //iterate over list and add dirs
    
    string currentDir, parentDir, prevDir;
    for (it = files.begin(); it != files.end(); ++it)
    {
        SPIFFS_FileInfo file = (*it);
        
        currentDir = file.parentDir;
        char buff[20];
        //check if dir needs to be written
        if (currentDir.compare(prevDir) != 0) {
            if (currentDir.length() > 0) {
                Serial.printf("Adding dir [%s] after previous dir [%s]\n", currentDir.c_str(), prevDir.c_str());
                SPIFFS_FileInfo dir;
                dir.isDirectory = true;
                dir.size = 0;
                dir.parentDir = currentDir.substr(0,currentDir.find_first_of('/'));
                dir.filePath = currentDir;
                list->push_back(dir);
            }
        }
        prevDir = currentDir;
    }
    
    
    //merge files into list
    for (it = files.begin(); it != files.end(); ++it)    
        list->push_back((*it));
}

// void esp32_fileio::printDirOrdered(Print* writeTo, list<SPIFFS_FileInfo>* files)
// {
//     list<SPIFFS_FileInfo>::iterator it;
//     files->sort(Sort);
//     string currentDir;
//     bool first = true;
//     writeTo->print("[");
//     for (it = files->begin(); it != files->end(); ++it)
//     {
//         File file = SPIFFS.open((*it).c_str());

//         //get current dir
//         currentDir = file.path();
//         byte idx = currentDir.find_last_of('/');
//         if (0xFF > idx > 0)
//             currentDir.erase(idx);
//         else
//             currentDir = "/";//defaul

//         //prefix comma if not first
//         if (first) {
//             first = false; 
//         }
//         else writeTo->print(",");
        
//         if (file.isDirectory()) {
//             writeTo->printf("{\"type\": \"dir\", \"name\":\"%s\", \"size\": %d, \"last_modified\":\"%s\", \"parent_dir\":\"%s\"}", file.name(), 0, "N/A", currentDir.c_str());
//         }
//         else
//         {
//             char buff[20];
//             time_t lastWrite = file.getLastWrite();
//             strftime(buff, 20, "%Y-%m-%d %H:%M:%S", localtime(&lastWrite));
//             writeTo->printf("{\"type\": \"file\", \"name\":\"%s\", \"size\": %d, \"last_modified\":\"%s\", \"parent_dir\":\"%s\"}", file.name(), file.size(), buff, currentDir.c_str());
//         }
        
//     }
//     writeTo->print("]");
// }
void esp32_fileio::printFileSearchOrdered(Print* writeTo, list<SPIFFS_FileInfo>* files,string filter = "")
{
    list<SPIFFS_FileInfo>::iterator it;
    files->sort(SortByPath);
    //string currentDir;
    bool first = true;
    writeTo->print("[");
    for (it = files->begin(); it != files->end(); ++it)
    {
        auto entry = *it;
        
        // File file = SPIFFS.open((*it).c_str());

        string filename = entry.filePath.substr(entry.parentDir.length());
        // int idx = currentDir.find_last_of('/');
        // if (idx > 0)
        //     currentDir.erase(idx);
        // else
        //     currentDir = "/";//defaul
        
        int idx = strncasecmp(filter.c_str(), filename.c_str(), filter.length());

        if (filter != "" && idx != 0)
            //filter applied, but file is not in filter
            continue;
        //prefix comma if not first
        if (first) {
            first = false;
        }
        else writeTo->print(",");

       writeTo->printf("{\"type\": \"%s\", \"name\":\"%s\", \"size\": %d, \"parent_dir\":\"%s\"}", 
            entry.isDirectory ? "dir" : "file", filename.c_str() , entry.size, entry.parentDir.c_str());
        

    }
    writeTo->print("]");
}
