#include <Arduino.h>
#include "esp32_filesystem.hpp"
#include <SPIFFS.h>
#include <SD.h>

const int CS = 5;
esp32_file_system filesystem;
File workingFile;

void commandList(int driveIdx);
bool commandGet(const char* path);
void commandRead(const char* path);
bool commandOpen(const char* path, const char* mode, bool create = false, int seek = 0);
int commandWrite(const char * buffer, size_t length);
bool commandClose();

void printMenu();



void setup() {
    Serial.begin(115200);
    pinMode(CS,PULLUP);
    if (!SPIFFS.begin(true)) {
        Serial.println("SPIFFS Mount Failed");        
    }
    else filesystem.addDisk(SPIFFS,"spiffs");

    //retry loop needed, sometimes SD does not start at first
    bool sdConnected = false;
    int retries = 0;
    while(retries++ < 3 && !sdConnected)
        sdConnected = SD.begin(CS);
      
    if(!sdConnected) {
        Serial.println("SD Initialization failed!");
    } else{
        filesystem.addDisk(SD, "sd");
    }
    int driveCount = filesystem.driveCount();

    Serial.printf("Loaded %d drives.\n", driveCount);

    for(int idx = 0; idx < driveCount; idx++){
        auto drive = filesystem.getDisk(idx);
        Serial.printf("Drive %d: %s\n", idx, drive->label());
        drive->list();
    }
    
    printMenu();
   
}

void loop() {
  //if request for file comes in over serial, process it through both regular and extended
  if(Serial.available()){
    auto line = Serial.readStringUntil('\n');
    auto params = explode(line.c_str(),":");
    string commandName = "", parameter = "";
    if(params.size() < 2){
        // commandName = "get";
        // parameter = line.c_str();
        Serial.println("Error: Invalid usage!");
        printMenu();
    } else{
        commandName = params[0];
        parameter = params[1];
    }
    if(strcmp(commandName.c_str(), "?") == 0 || strcmp(commandName.c_str(), "help") == 0){
        printMenu();
    }

    else if(strcmp(commandName.c_str(), "read") == 0){
        commandRead(parameter.c_str());
    }
    else if(strcmp(commandName.c_str(), "get") == 0){
        commandGet(parameter.c_str());
    }
    else if(strcmp(commandName.c_str(), "open") == 0){
        commandOpen(parameter.c_str(),FILE_WRITE);
    }
    else if(strcmp(commandName.c_str(), "append") == 0){
        commandOpen(parameter.c_str(), FILE_APPEND);
    }
    else if(strcmp(commandName.c_str(), "write") == 0){
        commandWrite((parameter + '\n').c_str(), parameter.length() + 1);
    }    
    else if(strcmp(commandName.c_str(), "close") == 0){
        commandClose();
    }
    else if(strcmp(commandName.c_str(), "list") == 0){
        commandList(atoi(parameter.c_str()));
    } else{
         Serial.println("Error: Invalid usage!");
        printMenu();
    }
    Serial.flush();
  }
}

void commandList(int driveIdx){
    auto drive = filesystem.getDisk(driveIdx);
    drive->list();
}

bool commandGet(const char* path){
    
    auto fileExtended = esp32_file_info_extended(path);
    Serial.printf("File %s in path %s on disk %d. Size %d %s\n",
        fileExtended.name().c_str(),
        fileExtended.path().c_str(),
        fileExtended.drive(),
        fileExtended.size(),
        fileExtended.exists() ? "found" : "not found"
    );

    return fileExtended.exists();
}
void commandRead(const char* path){
    auto fileInfo = esp32_file_info(path);
    auto drive = filesystem.getDisk(fileInfo.drive());
    auto file = drive->open(fileInfo.fullyQualifiedPath().c_str(),FILE_READ);

    if(!file){
        Serial.println("Failed to open file");
        return;
    }
    Serial.printf("Can read: %d,  write: %d\n", file.available(), file.availableForWrite());

    Serial.printf("Reading file %s (%d bytes) from drive [%d]%s\n",
        fileInfo.fullyQualifiedPath().c_str(),
        file.size(),
        fileInfo.drive(),
        drive->label()
    );
    
    static uint8_t  buff[512];
    int bytesRead = 0;
    do{
        bytesRead = file.read((uint8_t*)buff,sizeof(buff));
        if(bytesRead > 0) Serial.write((const char *)buff,bytesRead);
    }
    while(bytesRead > 0);  
    file.close();
}
bool commandOpen(const char* path, const char* mode, bool create, int seek){
    auto fileInfo = esp32_file_info(path);
    auto drive = filesystem.getDisk(fileInfo.drive());
    workingFile = drive->open(fileInfo.fullyQualifiedPath().c_str(), mode, create);
    Serial.printf("Opened working file %s\n", path);
    // if(seek != 0)
    //     workingFile.seek(workingFile.size() - seek);
    return true;
}

int commandWrite(const char * buffer, size_t length){
    if(!workingFile) return 0;
    int bytesWritten = workingFile.write((const uint8_t *)buffer,length);
    Serial.printf("Wrote %d bytes\n", bytesWritten);
    return length;
}

bool commandClose(){
    if(workingFile) {
        //workingFile.flush();
        workingFile.close();
        Serial.println("Closed file\n");
        return true;
    }
    return false;
}

void printMenu(){
    Serial.println("+---------------------------------------------------+");
    Serial.println("|           File System Interface                   |");
    Serial.println("+---------------------------------------------------+");
    Serial.println("|   Usage:  command:parameter                       |");
    Serial.println("|   Example: get:/sd/Log/test.log                   |");    
    Serial.println("+---------------------------------------------------+");
    Serial.println("| Commands:                                         |");
    Serial.println("|   list:drive_idx   - gets file information        |");
    Serial.println("|   get:file_path    - gets file information        |");
    Serial.println("|   open:file_apth   - opens the file for edit      |");
    Serial.println("|   write:string     - write string to file         |");
    Serial.println("|   append:string    - append string to file        |");
    Serial.println("|   close:[any]      - close file (param ignored)   |");
    Serial.println("|   read:file_path   - read file, print to serial   |");
    Serial.println("+---------------------------------------------------+");
    Serial.println("| File path conventions:                            |");
    Serial.println("|   /DRIVE/PATH/filename.ext                        |");
    Serial.println("|   /PATH/filename.ext?drive=1                      |");
    Serial.println("|   /PATH/filename.ext (default drive 0)            |");
    Serial.println("+---------------------------------------------------+");
    Serial.println("|  'help' or '?' to view this message               |");
    Serial.println("+---------------------------------------------------+");
}