#include "System/CORE/esp32_fileio.h"
#include "System/CORE/esp32_server.h"
#include "System/MODULES/MQTT/esp32_mqtt_client.hpp"
#include "System/CORE/esp32_wifi.h"
#include "System/AUTH/esp32_authentication.h"
#include "System/AUTH/esp32_sha256.h"

#include "System/MODULES/DEVICES/esp32_devices.hpp"
#include "System/MODULES/LOGGING/esp32_logging.hpp"
#include <esp_task_wdt.h>

esp32_server server;
esp32_wifi wifi;
esp32_fileio disk;
esp32_devices deviceManager;
esp32_logging logger;
esp32_mqtt_client mqtt;
DallasTemperature sensors;

TaskHandle_t* serverTaskHandle;
TaskHandle_t* deviceTaskHandle;
TaskHandle_t* mqttClientTaskHandle;

void serverTask(void* params);
void deviceTask(void* params);
void mqttClientTask(void* params);
void onShutdown();

#if CONFIG_FREERTOS_UNICORE
    #define ARDUINO_RUNNING_CORE 0
#else
    #define ARDUINO_RUNNING_CORE 1
#endif
#define REPORT_FREQUENCY 5000000 // 5 seconds
// DO NOT LOWER THESE. Components will begin to malfunction causing crashes.
extern const int SERVER_STACK_SIZE = 1024*32; 
extern const int DEVICE_MANAGER_STACK_SIZE = 1024 * 24; 
extern const int MQTT_CLIENT_STACK_SIZE = 1024 * 42;
const TickType_t deviceDelay = 600 / portTICK_PERIOD_MS, serverDelay = 50 / portTICK_PERIOD_MS;
#ifdef DEBUG
int64_t lastreportServer = 0;
string freeBytesHEAPSPretty("");
string freeBytesSTACKPretty("");
#endif
//for starting and looping server task
void serverTask(void* params) {
    server.start();
    while (true) {
        server.step();
        vTaskDelay(serverDelay);
        #ifdef DEBUG
        if(esp_timer_get_time() - lastreportServer > REPORT_FREQUENCY){
            auto stackFreeBytes = uxTaskGetStackHighWaterMark(NULL); 
            esp32_fileio::PrettyFormat((size_t)esp_get_free_heap_size(), &freeBytesHEAPSPretty);
            esp32_fileio::PrettyFormat(stackFreeBytes, &freeBytesSTACKPretty);
            
            Serial.printf("[SERVER] Free heap: %s\t stack: %s\n", freeBytesHEAPSPretty.c_str(), freeBytesSTACKPretty.c_str());
            lastreportServer = esp_timer_get_time(); 
        }
        #endif
    }

    //#ifdef DEBUG
    Serial.printf(PROGMEM("[SERVER] Shutting down\n"));
    //#endif
    vTaskDelete( NULL );
}

unsigned long deviceLoopTime = 0;
void deviceTask(void* params) {
    
    deviceManager.onInit();

    while(true){
        // #ifdef DEBUG
        // deviceLoopTime = millis();
        // #endif
        deviceManager.onLoop();

        // #ifdef DEBUG
        // Serial.printf("Device Loop took %lu ms.\n", millis() - deviceLoopTime);
        // #endif
        vTaskDelay(deviceDelay);
    }   
    vTaskDelete( NULL );
}

void mqttClientTask(void * params){
    //initialize mqtt    
    mqtt.start();
    if(mqtt.enabled()) // do not use all this RAM if its disabled     
        while(true){
            mqtt.loop();        
            vTaskDelay(deviceDelay);
        }
    vTaskDelete( NULL );
}

void onShutdown(){
    logger.logInfo("Shutting down controller");
}

void setup() {
    
    //debug logging
    Serial.begin(115200);
    //esp32 filesystem manager
    disk.start();    
   
    // //Connect to wifi
    wifi.start();   

    // //start logger
    logger.start();

    //Create Server
    xTaskCreatePinnedToCore(serverTask, "secureserver", SERVER_STACK_SIZE, NULL, 2, serverTaskHandle, ARDUINO_RUNNING_CORE); 

    //Create Device Manager
    xTaskCreatePinnedToCore(deviceTask, "devicemanager",DEVICE_MANAGER_STACK_SIZE, NULL, 2, deviceTaskHandle, ARDUINO_RUNNING_CORE);

    //Create MQTT Client
    xTaskCreate(mqttClientTask, "mqttclient",MQTT_CLIENT_STACK_SIZE, NULL, tskIDLE_PRIORITY, mqttClientTaskHandle);
    
    esp_register_shutdown_handler(onShutdown);
   
    logger.logInfo("System started");
}
int64_t lastReportMain = 0;
string freeBytesHEAPPretty = "", freeBytesSTACKmPretty = "";//, freeBytesSTACKServerPretty="", freeBytesSTACKDevicePretty="", freeBytesSTACKMQTTPretty="";
void loop() {   
    #ifdef DEBUG
    if(esp_timer_get_time() - lastReportMain > REPORT_FREQUENCY){        

       
        auto stackFreeBytes = uxTaskGetStackHighWaterMark(NULL); 
        // auto stackServerFreeBytes = uxTaskGetStackHighWaterMark(serverTaskHandle); 
        // auto stackDeviceFreeBytes = uxTaskGetStackHighWaterMark(deviceTaskHandle); 
        // auto stackMQTTFreeBytes = uxTaskGetStackHighWaterMark(mqttClientTaskHandle); 
        esp32_fileio::PrettyFormat((size_t)esp_get_free_heap_size(), &freeBytesHEAPPretty);
        esp32_fileio::PrettyFormat(stackFreeBytes, &freeBytesSTACKmPretty);
        // esp32_fileio::PrettyFormat(stackServerFreeBytes, &freeBytesSTACKServerPretty);
        // esp32_fileio::PrettyFormat(stackDeviceFreeBytes, &freeBytesSTACKDevicePretty);
        // esp32_fileio::PrettyFormat(stackMQTTFreeBytes, &freeBytesSTACKMQTTPretty);
        
        Serial.printf("[MAIN] Free heap: %s\tmain stack: %s\n", // \n\tserver stack: %s\n\tdevice stack: %s\n\tmqtt stack: %s
            freeBytesHEAPPretty.c_str(), 
            freeBytesSTACKmPretty.c_str()
            // freeBytesSTACKServerPretty.c_str(),
            // freeBytesSTACKDevicePretty.c_str(),
            // freeBytesSTACKMQTTPretty.c_str()
        );
        lastReportMain = esp_timer_get_time(); 
    }
    delay(50);
    #endif
}