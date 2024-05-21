#include "esp32_fileio.h"

#include "../ROUTER/esp32_router.h"
#include <esp_task_wdt.h>

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

        //esp32_fileio::listDir(SPIFFS, &Serial,"/", 2);
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

/// @brief List files on the file system. Can be filtered to a path, and search string.
/// @param fs File system to use. (e.g. SPIFFS, LittleFS)
/// @param writeTo any object that inherits from Print class
/// @param dirname name of directory to list. leave empty to list all files
/// @param levels number of levels to traverse
/// @param format text or json format
/// @param searchString substring to match. leading ! will exclude search results matching @ref `searchString`
void esp32_fileio::listDir(fs::FS& fs, Print* writeTo, const char* dirname, uint8_t levels, HTTP_FORMAT format, const char* searchString) {
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
                    esp32_fileio::listDir(fs, writeTo, file.name(), levels - 1,format, searchString);
                }
            }
            else {
                if(strlen(searchString) > 0 && (
                        (searchString[0] != '!' && strstr(file.name(),searchString) == nullptr) ||
                        (searchString[0] == '!' && strstr(file.name(),string(searchString).substr(1).c_str()) != nullptr) 
                    )
                )
                {
                    //Serial.printf("Excluding file %s\n", file.name());
                    file = root.openNextFile();
                    continue; //doesn't match
                }
                    
                if (first) {
                    first = false;
                }
                else writeTo->print(",");

                char buff[20];
                time_t lastWrite = file.getLastWrite();
                strftime(buff, 20, "%Y-%m-%d %H:%M:%S", localtime(&lastWrite));
                writeTo->printf("{\"type\": \"file\", \"name\":\"%s\", \"size\": %d, \"last_modified\":\"%s\"}", file.name(), file.size(), buff);
            }
            file = root.openNextFile();
            //esp_task_wdt_reset();
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
            //esp_task_wdt_reset();
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
void esp32_fileio::buildOrderedFileList(fs::FS& fs, const char* dirname, const char * searchString,uint8_t levels, list<SPIFFS_FileInfo> &list) {
    //std::list<SPIFFS_FileInfo> files;
    std::list<SPIFFS_FileInfo>::iterator it;
    File root = fs.open(dirname);
    if (!root) {
        return;
    }
    
    //Serial.printf("[esp32_fileio::buildOrderedFileList] getting next file after %s %s in %s\n", root.isDirectory() ? "dir" : "file", root.name(), root.path());
    File file = root.openNextFile();
    while (file) {
        //file name shorter than search string or not matching
        if(strlen(searchString) > 0 && ( file.isDirectory() || strlen(file.name()) < strlen(searchString) || strstr(file.name(), searchString) == NULL))
        {
            //...
        }else {
            //add file/dir to list
            //Serial.printf("Adding %s %s to file list\n",file.isDirectory() ? "dir" : "file", file.path());
            SPIFFS_FileInfo f;
            f.filePath = file.path();
            f.isDirectory = file.isDirectory();
            f.parentDir = f.filePath.substr(0,f.filePath.find_last_of('/'));
            f.size = file.size();
            list.push_back(f);
        }
        
        if (file.isDirectory()) {            
            if (levels) { //read deeper
                //Serial.printf("Reading dir %s on level %d\n",file.name(),levels);
                esp32_fileio::buildOrderedFileList(fs, file.path(), searchString, levels - 1, list);
            }
        }
            
        file = root.openNextFile();
        //esp_task_wdt_reset();
    }

    list.sort(SortByPath);

    //iterate over list and add dirs
    string currentDir, parentDir, prevDir;
    auto lastElement = list.end();
    for (it = list.begin(); it != lastElement; ++it)
    {
        SPIFFS_FileInfo file = (*it);
        
        currentDir = file.parentDir;
        char buff[20];
        //check if dir needs to be written
        if (currentDir.compare(prevDir) != 0) {
            if (currentDir.length() > 0) {
                //Serial.printf("Adding dir [%s] after previous dir [%s]\n", currentDir.c_str(), prevDir.c_str());
                SPIFFS_FileInfo dir;
                dir.isDirectory = true;
                dir.size = 0;
                dir.parentDir = currentDir.substr(0,currentDir.find_first_of('/'));
                dir.filePath = currentDir;
                list.push_back(dir);
            }
        }
        prevDir = currentDir;
    }    
    
    //merge files into list
    // for (it = files.begin(); it != files.end(); ++it)    
    //     list->push_back((*it));
}

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
        string filename = entry.filePath.substr(entry.parentDir.length());        
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

bool esp32_fileio::CreateFile(const char * filename){
    string name = filename;
    if(!iequals(SITE_ROOT, name.c_str(),strlen(SITE_ROOT))){
        Serial.printf("Prefixing %s to path %s\n", SITE_ROOT, filename);
        name = SITE_ROOT + name;
    }
    if(SPIFFS.exists(name.c_str())) {
        Serial.printf("File %s already exists!", name.c_str());
        return false;
    }
    File f = SPIFFS.open(name.c_str(),"w");
    f.close();
    return true;
}
/// @brief Update file on SPIFFS.
/// @brief Will make sure file in is the SITE_ROOT path
/// @param filename path of file to save
/// @param HTTPMultipartBodyParser object from the request
/// @param createIfNotFound create file if no found
/// @return number of bytes saved. -1 if failed
size_t esp32_fileio::UpdateFile(const char * filename, httpsserver::HTTPMultipartBodyParser* parser, bool createIfNotFound){
    string name = filename;
    if(name[0] != '/') name = "/" + name;
    if(!iequals(SITE_ROOT, name.c_str(),strlen(SITE_ROOT)) &&
        !iequals(PATH_LOGGING_ROOT, name.c_str(),strlen(PATH_LOGGING_ROOT))){
        name = SITE_ROOT + name;
    }

    size_t fieldLength = 0;
    if(!SPIFFS.exists(name.c_str())){
        Serial.printf("File %s not found \n", name.c_str());
        if(!createIfNotFound)
            return -1;
    }
    File file = SPIFFS.open(name.c_str(), "w");
    byte* buf = new byte[512]; //must be at least 72 chars to detect boundary
    size_t readLength = 0;
    while (!parser->endOfField()) {
        
        readLength = parser->read(buf, 512);
        file.write(buf, readLength);
        //Serial.write(buf,readLength);
        fieldLength += readLength;
    }
    delete[] buf;
    file.close();  
    return fieldLength;  
}

bool esp32_fileio::DeleteFile(const char * filename){
    if (filename == "") {
       return false;
    }
    string name = filename;
    if(!iequals(SITE_ROOT, filename, strlen(SITE_ROOT)) &&
        !iequals(PATH_LOGGING_ROOT, name.c_str(),strlen(PATH_LOGGING_ROOT))
    ){
        name = SITE_ROOT + name;
    }
    if (!SPIFFS.exists(name.c_str())) {
        return false;
    }
    return SPIFFS.remove(name.c_str());
}

void esp32_fileio::writeFileToResponse(esp32_route_file_info fileInfo, httpsserver::HTTPResponse *response)
{
    File f = SPIFFS.open(fileInfo.filePath.c_str(), "r");

    if (fileInfo.isGZ)
        response->setHeader("Content-Encoding", "gzip");

    response->setHeader("Cache-Control", strstr(f.name(), "list?") != nullptr || fileInfo.isEditorRequest ? "no-store" : "private, max-age=604800");

    if (fileInfo.isDownload)
    {
        char dispStr[128];
        sprintf(dispStr, " attachment; filename = \"%s\"", fileInfo.fileName);
        response->setHeader("Content - Disposition", dispStr);
        fileInfo.fileExtension = "application/octet-stream";
    }
    else
    {
        if (fileInfo.fileExtension == "htm")
            fileInfo.fileExtension = "html"; // workaround for encoding
        else if (fileInfo.fileExtension == "js")
            fileInfo.fileExtension = "javascript"; // workaround for encoding
        fileInfo.fileExtension = "text/" + fileInfo.fileExtension;
    }
    response->setHeader("Content-Type", fileInfo.fileExtension.c_str());
    response->setStatusCode(200);
    char buff[32];
    while (true)
    {

        uint16_t bytestoRead = f.available() < sizeof(buff) ? f.available() : sizeof(buff);
        if (bytestoRead == 0)
            break;
        f.readBytes(buff, bytestoRead);
        response->write((uint8_t *)buff, bytestoRead);
        // Serial.printf("Wrote %u bytes to %s\n", bytestoRead, f.name());
    }
}
