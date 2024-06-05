#include "esp32_fileio.h"

#include "../ROUTER/esp32_router.h"
#include <esp_task_wdt.h>

#include "SD.h"
#include "SPI.h"
#include <esp32_filesystem_objects.h>
const int CS = 5;
esp32_file_system filesystem;
const char* infoTypes[] = {"SPIFFS","SD"};
bool esp32_fileio::start(){
    int retries = 0;
    if (!SPIFFS.begin(true)) {
        Serial.println("SPIFFS Mount Failed");
        return false;
    }
    else filesystem.addDisk(SPIFFS,"spiffs");

    bool sdConnected = false;
    while(retries++ < 3 && !sdConnected)
        sdConnected = SD.begin(CS);
        //return;
    if(!sdConnected) {
        Serial.println("SD Initialization failed!");
    } else{
        filesystem.addDisk(SD, "sd",dt_SD);
    }
    Serial.printf("Loaded %d drives.\n",  filesystem.driveCount());

    for(int idx = 0; idx <  filesystem.driveCount(); idx++){
        auto drive = filesystem.getDisk(idx);
        
        auto info = drive->info();
        Serial.printf("Drive #%d %s. Type: %s. Size: %d bytes. Used: %d bytes.\n",
            drive->index(), drive->label(), infoTypes[info.type()], info.size(), info.used()
        );
        //drive->list();
    }

    return true;
}



bool esp32_fileio::CreateFile( const char * filename){
    auto routeInfo = esp32_route_file_info<esp32_file_info>(filename);
    auto drive = filesystem.getDisk(routeInfo.drive());
    if(drive->exists(routeInfo.fullyQualifiedPath().c_str()))
    {
        Serial.printf("File %s already exists on volume %s!\n", routeInfo.fullyQualifiedPath().c_str(), drive->label());
        return false;
    }
    if(!drive->exists(routeInfo.path().c_str())){
        bool created = drive->mkdir(routeInfo.path().c_str()); 
        Serial.printf("Creating directory %s %s\n", routeInfo.path(), created ? "SUCESSFULL" : "FAILED" );       
        if(!created) return false;
    }
        

    return drive->create(routeInfo.fullyQualifiedPath().c_str()); 
}

/// @brief Update file on fs.
/// @brief /* Depricated */Will make sure file in is the PATH_SITE_ROOT path
/// @param filename path of file to save
/// @param HTTPMultipartBodyParser object from the request
/// @param createIfNotFound create file if no found
/// @return number of bytes saved. -1 if failed
size_t esp32_fileio::UpdateFile(const char * filename, httpsserver::HTTPMultipartBodyParser* parser, bool createIfNotFound){

    auto routeInfo = esp32_route_file_info<esp32_file_info>(filename);
    auto drive = filesystem.getDisk(routeInfo.drive());
    if(!drive->exists(routeInfo.path().c_str()) && !createIfNotFound)
    {
        Serial.printf("File %s does not exist, and asked not to create. Terminating!\n", routeInfo.name().c_str());
        return false;
    }else
        drive->mkdir(routeInfo.path().c_str());

    File openedFile = drive->open(routeInfo.fullyQualifiedPath().c_str(),FILE_WRITE,createIfNotFound); 
    if(!openedFile) return 0;

    byte* buf = new byte[512]; //must be at least 72 chars to detect boundary
    size_t readLength = 0;
    size_t fieldLength = 0;
    while (!parser->endOfField()) {
        
        readLength = parser->read(buf, 512);
        openedFile.write(buf, readLength);
        //Serial.write(buf,readLength);
        fieldLength += readLength;
    }
    delete[] buf;
    openedFile.close();  
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
    //TODO: consider testing for internal, otherwise prefix spiffs public path if uploading to spiffs
    auto routeInfo = esp32_route_file_info<esp32_file_info>(filename);
    auto drive = filesystem.getDisk(routeInfo.drive());
    if(!drive->exists(routeInfo.path().c_str()) && !createIfNotFound)
    {
        Serial.printf("File %s does not exist, and asked not to create. Terminating!\n", routeInfo.name().c_str());
        return false;
    }
    if(!drive->exists(routeInfo.path().c_str()))
        drive->mkdir(routeInfo.path().c_str());

    File openedFile = drive->open(routeInfo.fullyQualifiedPath().c_str(),FILE_WRITE,createIfNotFound); 
    if(!openedFile) return 0;

    size_t fieldLength = 0;
    if(seek != 0){
        int fileSize = openedFile.size();
        int seekPos = fileSize  + seek;
        bool seekWorked = openedFile.seek(seekPos, SeekMode::SeekSet);
        if(!seekWorked){            
            return 0;
        }
    }
    openedFile.print(message);
    openedFile.close();  
    return fieldLength;      
}

bool esp32_fileio::DeleteFile(const char * filename){
    if (filename == "") {
       return false;
    }

    auto routeInfo = esp32_route_file_info<esp32_file_info>(filename);
    auto drive = filesystem.getDisk(routeInfo.drive());
    if(!drive->exists(routeInfo.fullyQualifiedPath().c_str())){
        Serial.printf("Attempted to delete a non-existent file %s on disk %s\n", routeInfo.fullyQualifiedPath().c_str(), drive->label());
        return false;
    }

    return drive->remove(routeInfo.fullyQualifiedPath().c_str());   
}

void esp32_fileio::writeFileToResponse(esp32_route_file_info<esp32_file_info_extended> routeInfo, httpsserver::HTTPResponse *response){
    auto drive = filesystem.getDisk(routeInfo.drive());    
    auto file = drive->open(routeInfo.fullyQualifiedPath().c_str(), "r");
    //Serial.printf("Writing %d bytes from  %s located at %s in %s format on drive %d-%s to HTTPResponse\n", file.size(), routeInfo.name().c_str(), routeInfo.fullyQualifiedPath().c_str(), routeInfo.isGZ() ? "GZ" : "RAW", drive->index(), drive->label());
   
    if(!file) {
    auto file = drive->open(routeInfo.fullyQualifiedPath().c_str(), "r");
        Serial.printf("Error, file not valid read: %d write: %d \n", file.available(), file.availableForWrite());
        return;
    }
    //f.seek(0);

    if (routeInfo.isGZ())
        response->setHeader("Content-Encoding", "gzip");

    response->setHeader("Cache-Control", strstr(file.name(), "list?") != nullptr || routeInfo.isEditorRequest ? "no-store" : "private, max-age=604800");
    string extension = routeInfo.extension();
    if (routeInfo.isDownload())
    {
        char dispStr[128];
        sprintf(dispStr, " attachment; filename = \"%s\"", routeInfo.name().c_str());
        response->setHeader("Content - Disposition", dispStr);
        extension = "application/octet-stream";
    }
    else
    {
        if (strcmp(routeInfo.extension(), "htm") == 0)
            extension = "html"; // workaround for encoding
        else if (strcmp(routeInfo.extension(), "js") == 0)
            extension = "javascript"; // workaround for encoding
        extension = "text/" + extension;
    }
    response->setHeader("Content-Type", routeInfo.extension());
    response->setStatusCode(200);
    char buff[32];
    while (true)
    {

        uint16_t bytestoRead = file.available() < sizeof(buff) ? file.available() : sizeof(buff);
        if (bytestoRead == 0)
            break;
        file.readBytes(buff, bytestoRead);
        response->write((uint8_t *)buff, bytestoRead);
        //Serial.printf("Wrote %u bytes at %d of %d from %s\n", bytestoRead, f.position(), f.size(), f.name());
    }
}

void esp32_fileio::writeFileToResponse(const char * filename, httpsserver::HTTPResponse *response)
{
    auto routeInfo = esp32_route_file_info<esp32_file_info_extended>(filename);
    writeFileToResponse(routeInfo,response);
    
}
/// @brief Checks if path is fully qualified to match system paths
/// @param path path of file or directory to test
/// @return true if path starts with valid directory. false otherwise
bool esp32_fileio::isQualifiedPath(const char *path)
{
    return strstr(path,PATH_SITE_ROOT) != nullptr || strstr(path, PATH_LOGGING_ROOT) != nullptr || strstr(path, PATH_INTERNAL_ROOT) != nullptr;
}