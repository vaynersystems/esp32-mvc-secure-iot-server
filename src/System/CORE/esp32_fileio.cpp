#include "esp32_fileio.h"

#include "../ROUTER/esp32_router.h"
#include <esp_task_wdt.h>

bool esp32_fileio::start(){
    if (!SPIFFS.begin(true)) {
        Serial.println("SPIFFS Mount Failed");
        return false;
    }
    else {
        string sizeText, usedText;
        esp32_fileio::PrettyFormat(SPIFFS.totalBytes(), &sizeText);
        esp32_fileio::PrettyFormat(SPIFFS.usedBytes(), &usedText);
        //Serial.printf("Mounted vol %s. Used %zu bytes of %zu.\n",
        Serial.printf("Mounted vol %s. Used %s bytes of %s.\n",
            "[DEFAULT]", usedText.c_str(), sizeText.c_str());
        #ifdef DEBUG
        //esp32_fileio::listDir(SPIFFS, &Serial,"/", 3);        
        #endif
        Serial.println("-----------------------");
        if(((float)SPIFFS.usedBytes()/(float)SPIFFS.totalBytes()) > 0.8){
            Serial.println("SPIFFS VOLUME IS TOO FULL. REMOVING LOGS");
            logger.removeAllLogs();
        }
    }
    return true;
}

esp32_route_file_info  esp32_fileio::getFileInfo(fs::FS & fs, const char *filename)
{
    esp32_route_file_info fileInfo;
    
    fileInfo.fileName = filename;
    if(!isQualifiedPath(filename)){
        //Serial.printf("Path %s not fully qualified. setting file path to %s\n", fileInfo.fileName.c_str(), (PATH_SITE_ROOT + fileInfo.fileName).c_str());
        fileInfo.fileName = PATH_SITE_ROOT + fileInfo.fileName;
    }

    fileInfo.isInternal = strstr(filename,PATH_INTERNAL_ROOT) == &filename[0] || 
        strstr(filename,PATH_LOGGING_ROOT) == &filename[0] ;// fileName.startsWith(PATH_INTERNAL_ROOT) || fileName.startsWith(PATH_LOGGING_ROOT);    
    
        
    fileInfo.fileName = urlDecode(fileInfo.fileName.c_str()).c_str();
    
    //first we need to trim off any request parameters
    int endIdx = fileInfo.fileName.find("?");
    if (endIdx > 0) {
        if (fileInfo.fileName.substr(endIdx + 1).find("download=true") == 0)
            fileInfo.isDownload = true;
        fileInfo.fileName = fileInfo.fileName.substr(0, endIdx);
    }

    fileInfo.filePath = fileInfo.fileName.substr(0,fileInfo.fileName.find_last_of('/'));

    //Serial.printf("First parse of %s filename %s\n", fileInfo.isInternal ? "internal" : "",  fileInfo.fileName.c_str());

    //check if file requested is in gzip format
    if(ends_with(fileInfo.fileName, ".gz")){
        //Serial.printf("File %s ends with .gz\n", fileInfo.fileName.c_str());
        fileInfo.fileName = fileInfo.fileName.substr(0, fileInfo.fileName.find_last_of('.')).c_str();
        fileInfo.isGZ = true;
    }

    fileInfo.fileExtension = 
        fileInfo.fileName.find_last_of('.') > 0 ?
            fileInfo.fileName.substr(fileInfo.fileName.find_last_of('.') + 1).c_str() : "";

    string zipFileName = fileInfo.fileName + ".gz";

    if(fs.exists(zipFileName.c_str())){
        //zip file found
        fileInfo.exists = true;
        fileInfo.filePath = zipFileName.c_str();
        fileInfo.isGZ = true;
    } else if(!fileInfo.isGZ && fs.exists(fileInfo.fileName.c_str())){
        //regular file found
        fileInfo.exists = true;
        fileInfo.isGZ = false;            
    } else{
        //file not found
        fileInfo.exists = false;
        Serial.printf("File %s NOT FOUND!!!\n", fileInfo.fileName.c_str());
    }    

    return fileInfo;
}

esp32_route_file_info_extended esp32_fileio::getFileInfoExtended(fs::FS &fs, const char *filename)
{
    esp32_route_file_info_extended fileInfo;
    //Serial.printf("Building file route info for filename %s\n", filename);
            
    fileInfo.fileName = filename;
    if(!isQualifiedPath(filename)){
        //Serial.printf("Path %s not fully qualified. setting file path to %s\n", fileInfo.fileName.c_str(), (PATH_SITE_ROOT + fileInfo.fileName).c_str());
        fileInfo.fileName = PATH_SITE_ROOT + fileInfo.fileName;
    }
        
    fileInfo.fileName = urlDecode(fileInfo.fileName.c_str()).c_str();
    
    //first we need to trim off any request parameters
    int endIdx = fileInfo.fileName.find("?");
    if (endIdx > 0) {
        if (fileInfo.fileName.substr(endIdx + 1).find("download=true") == 0)
            fileInfo.isDownload = true;
        fileInfo.fileName = fileInfo.fileName.substr(0, endIdx);
    }

    fileInfo.filePath = fileInfo.fileName.substr(0,fileInfo.fileName.find_last_of('/'));

    //Serial.printf("First parse of %s filename %s\n", fileInfo.isInternal ? "internal" : "",  fileInfo.fileName.c_str());

    //check if file requested is in gzip format
    if(ends_with(fileInfo.fileName, ".gz")){
        //Serial.printf("File %s ends with .gz\n", fileInfo.fileName.c_str());
        fileInfo.fileName = fileInfo.fileName.substr(0, fileInfo.fileName.find_last_of('.')).c_str();
        fileInfo.isGZ = true;
    }

    fileInfo.fileExtension = 
        fileInfo.fileName.find_last_of('.') > 0 ?
            fileInfo.fileName.substr(fileInfo.fileName.find_last_of('.') + 1).c_str() : "";

    string zipFileName = fileInfo.fileName + ".gz";

    //Serial.printf("Second parse of filename %s\n", fileInfo.fileName.c_str());
    //Serial.printf("         Zipped filename %s\n", zipFileName.c_str());

    if(fs.exists(zipFileName.c_str())){
        //zip file found
        fileInfo.exists = true;
        fileInfo.filePath = zipFileName.c_str();
        fileInfo.isGZ = true;
    } else if(!fileInfo.isGZ && fs.exists(fileInfo.fileName.c_str())){
        //regular file found
        fileInfo.exists = true;
        fileInfo.isGZ = false;            
    } else{
        //file not found
        fileInfo.exists = false;
        Serial.printf("File %s NOT FOUND!!!\n", fileInfo.fileName.c_str());
    }

    //Serial.printf("Third parse of filename %s with path %s\n", fileInfo.fileName.c_str(), fileInfo.filePath.c_str());

    if(fileInfo.exists){
        File f = fs.open(fileInfo.filePath.c_str(),"r");
        fileInfo.fileSize = f.size();    
        fileInfo.lastWrite = f.getLastWrite();    
    }

    return fileInfo;
}

int esp32_fileio::getFilesExtended(fs::FS &fs, vector<esp32_route_file_info_extended> &files, const char *dirName, const char *searchString)
{
    int filesfound = 0; 
    auto root = fs.open("/");
    File filePointer = root.openNextFile();
    do{
        if(!filePointer) continue;

        if(strstr(filePointer.path(),dirName) == filePointer.path() //begining match
            
            && (strlen(searchString) == 0 ||
                (strlen(searchString) > 0 && (
                        (searchString[0] == '!' && strstr(filePointer.name(),string(searchString).substr(1).c_str()) == nullptr) ||
                        (searchString[0] != '!' && strstr(filePointer.name(),searchString) != nullptr) 
                    )
                )
            )
        ){
            files.push_back(getFileInfoExtended(fs,filePointer.path()));
            filesfound++;
        }
        filePointer = root.openNextFile();
    }
    while(filePointer);
    return filesfound;
}

/// @brief Gets files matching `searchString` in absolute path `dirName` on filesystem `fs`
/// @param fs File System (e.g. SPIFFS, LitteFS)
/// @param files vector of `esp32_route_file_info_extended` to populate
/// @param dirName absolute path to search. leave blank or use '/' for root.
/// @param searchString
/// @return number of files found
int esp32_fileio::getFiles(fs::FS &fs, vector<esp32_route_file_info> &files, const char *dirName, const char *searchString )
{
    int filesfound = 0; 
    auto root = fs.open("/");
    File filePointer = root.openNextFile();
    do{
        if(!filePointer) continue;

        if(strstr(filePointer.path(),dirName) == filePointer.path() //begining match
            
            && (strlen(searchString) == 0 ||
                (strlen(searchString) > 0 && (
                        (searchString[0] == '!' && strstr(filePointer.name(),string(searchString).substr(1).c_str()) == nullptr) ||
                        (searchString[0] != '!' && strstr(filePointer.name(),searchString) != nullptr) 
                    )
                )
            )
        ){
            files.push_back(getFileInfo(fs,filePointer.path()));
            filesfound++;
        }
        filePointer = root.openNextFile();
    }
    while(filePointer);
    return filesfound;
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
int esp32_fileio::listDir(fs::FS& fs, Print* writeTo, const char* dirname, uint8_t levels, HTTP_FORMAT format, const char* searchString) {
    vector<esp32_route_file_info_extended> files;
    int filesfound = getFilesExtended(fs, files, dirname, searchString);
    if(filesfound <= 0) return filesfound;
    char buff[20];

    if (format == HTTP_FORMAT::JSON) {
        
        writeTo->print("[");
        for(int idx = 0; idx < files.size(); idx++){            
            
            strftime(buff, 20, "%Y-%m-%d %H:%M:%S", localtime(&files[idx].lastWrite));
            writeTo->printf("%s{\"type\": \"file\", \"name\":\"%s\", \"size\": %d, \"last_modified\":\"%s\"}", 
                idx == 0 ? "" : ", ",
                files[idx].fileName.c_str(), 
                files[idx].fileSize, 
                buff
            );
            
        }
        writeTo->print("]");        
    }
    else
    {
        int totalSize = 0;
        writeTo->printf("Listing directory: %s\r\n", dirname);

        for(int idx = 0; idx < files.size(); idx++){            
            strftime(buff, 20, "%Y-%m-%d %H:%M:%S", localtime(&files[idx].lastWrite));
            writeTo->printf("FILE: %s\t SIZE: %lf, CREATED: %s\n", files[idx].fileName.c_str(), files[idx].fileSize, buff); 
            totalSize += files[idx].fileSize;           
        }
        // string sizeStr;
        // PrettyFormat(totalSize, &sizeStr);
        writeTo->printf("File count: %d. File Size: %s%d bytes\033[39m. Size on disk: %s%d bytes\033[39m\n", 
            filesfound,
            totalSize > 900000 ? "\033[91m" : totalSize > 400000 ? "\033[93m" : "\033[92m", 
            totalSize, 
            totalSize > 900000 ? "\033[91m" : totalSize > 400000 ? "\033[93m" : "\033[92m",  
            SPIFFS.usedBytes());
    }
    return filesfound;
    
}

bool esp32_fileio::CreateFile(const char * filename){
    string name = filename;
    if(!isQualifiedPath(filename)){
    //if(strstr(filename,PATH_SITE_ROOT) == nullptr && strstr(filename, PATH_LOGGING_ROOT) == nullptr && strstr(filename, PATH_INTERNAL_ROOT)){
        //Serial.printf("Prefixing %s to path %s\n", PATH_SITE_ROOT, filename);
        name = PATH_SITE_ROOT + name;
    }
    if(SPIFFS.exists(name.c_str())) {
        Serial.printf("File %s already exists!\n", name.c_str());
        return false;
    }
    //check if directory exists
    auto dirPath = name.substr(0,name.find_last_of('/')).c_str();
    if(SPIFFS.exists(dirPath))
        SPIFFS.mkdir(dirPath);
    File f = SPIFFS.open(name.c_str(),"w",true);
    f.close();
    return true;
}
/// @brief Update file on SPIFFS.
/// @brief Will make sure file in is the PATH_SITE_ROOT path
/// @param filename path of file to save
/// @param HTTPMultipartBodyParser object from the request
/// @param createIfNotFound create file if no found
/// @return number of bytes saved. -1 if failed
size_t esp32_fileio::UpdateFile(const char * filename, httpsserver::HTTPMultipartBodyParser* parser, bool createIfNotFound){
    string name = filename;
    if(name[0] != '/') name = "/" + name;
    if(!isQualifiedPath(filename)){
    //if(!iequals(PATH_SITE_ROOT, name.c_str(),strlen(PATH_SITE_ROOT)) &&
    //    !iequals(PATH_LOGGING_ROOT, name.c_str(),strlen(PATH_LOGGING_ROOT))){
        name = PATH_SITE_ROOT + name;
    }

    size_t fieldLength = 0;
    if(!SPIFFS.exists(name.c_str())){        
        if(!createIfNotFound){
            Serial.printf("File %s not found \n", name.c_str());
            return -1;
        }
            
        CreateFile(name.c_str());
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

/// @brief adds message to file.
/// @param filename name of file
/// @param message message to append to file
/// @param createIfNotFound create file if it does not exist
/// @param seek position relative to end of file. e.g. To seek 2 chars back from end, use -2, to see to begining use -file.size()
/// @return 
size_t esp32_fileio::UpdateFile(const char *filename, const char *message, bool createIfNotFound, int seek )
{
    string name = filename;
    if(name[0] != '/') name = "/" + name;
    if(!iequals(PATH_SITE_ROOT, name.c_str(),strlen(PATH_SITE_ROOT)) &&
        !iequals(PATH_LOGGING_ROOT, name.c_str(),strlen(PATH_LOGGING_ROOT))){
        name = PATH_SITE_ROOT + name;
    }

    size_t fieldLength = 0;
    if(!SPIFFS.exists(name.c_str())){        
        if(!createIfNotFound || seek != 0){
            Serial.printf("File %s not found \n", name.c_str());
            return 0;
        }
            
        CreateFile(filename);
    }
    File file = SPIFFS.open(name.c_str(), "r+w",createIfNotFound);
    if(!file) return 0;
    if(seek != 0){
        int fileSize = file.size();
        int seekPos = fileSize  + seek;
        bool seekWorked = file.seek(seekPos, SeekMode::SeekSet);
        if(!seekWorked){            
            return 0;
        }
    }
    file.print(message);
    file.close();  
    return fieldLength;  
}

bool esp32_fileio::DeleteFile(const char * filename){
    if (filename == "") {
       return false;
    }
    string name = filename;
    if(!iequals(PATH_SITE_ROOT, filename, strlen(PATH_SITE_ROOT)) &&
        !iequals(PATH_LOGGING_ROOT, name.c_str(),strlen(PATH_LOGGING_ROOT))
    ){
        name = PATH_SITE_ROOT + name;
    }
    if (!SPIFFS.exists(name.c_str())) {
        return false;
    }
    return SPIFFS.remove(name.c_str());
}

void esp32_fileio::writeFileToResponse(esp32_route_file_info fileInfo, httpsserver::HTTPResponse *response)
{
    //Serial.printf("Writing %s located at %s to HTTPResponse\n", fileInfo.fileName.c_str(), fileInfo.filePath.c_str());
    File f = SPIFFS.open(fileInfo.isGZ ? (fileInfo.fileName + ".gz").c_str() : fileInfo.fileName.c_str(), "r");

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
/// @brief Checks if path is fully qualified to match system paths
/// @param path path of file or directory to test
/// @return true if path starts with valid directory. false otherwise
bool esp32_fileio::isQualifiedPath(const char *path)
{
    return strstr(path,PATH_SITE_ROOT) != nullptr || strstr(path, PATH_LOGGING_ROOT) != nullptr || strstr(path, PATH_INTERNAL_ROOT) != nullptr;
}





#pragma region LEGACY

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
    Serial.printf("Printing %d files\n", files->size());
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

#pragma endregion
