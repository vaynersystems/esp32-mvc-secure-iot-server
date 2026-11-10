#include "CORE/esp32_fileio.hpp"
#include "CORE/esp32_server.hpp"
#include "MODULES/MQTT/esp32_mqtt_client.hpp"
#include "CORE/esp32_wifi.h"
#include "AUTH/esp32_authentication.hpp"
#include "AUTH/esp32_sha256.h"

#include "MODULES/DEVICES/esp32_devices.hpp"
#include "MODULES/SCHEDULING/esp32_scheduling_manager.hpp"
#include "MODULES/LOGGING/esp32_logging.hpp"

#include "MODULES/LCD/esp32_lcd.hpp"
#include <esp_task_wdt.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <esp_heap_caps.h>
#include "mbedtls/platform.h"

esp32_server server;
esp32_wifi wifi;
esp32_fileio disk;
esp32_devices deviceManager;
esp32_scheduling_manager scheduleManager;
esp32_logging logger;
esp32_mqtt_client mqtt;
esp32_pin_manager pinManager;

TaskHandle_t* serverTaskHandle;
TaskHandle_t* lcdTaskHandle;
TaskHandle_t* deviceTaskHandle;
TaskHandle_t* mqttClientTaskHandle;

//QueueHandle_t xMutex;
SemaphoreHandle_t lcdMutex;

void serverTask(void* params);
void lcdTask(void* params);
void deviceTask(void* params);
void mqttClientTask(void* params);
void onShutdown();

#if CONFIG_FREERTOS_UNICORE
    #define ARDUINO_RUNNING_CORE 0
#else
    #define ARDUINO_RUNNING_CORE CONFIG_ARDUINO_RUNNING_CORE
#endif
#define REPORT_FREQUENCY 5000000 // 5 seconds
// DO NOT LOWER THESE. Components will begin to malfunction causing crashes.
// SET_LOOP_TASK_STACK_SIZE(1024*16)
extern const int SERVER_STACK_SIZE = 1024*24; 
extern const int LCD_STACK_SIZE = 1024*8;
extern const int DEVICE_MANAGER_STACK_SIZE = 1024 * 24; 
extern const int MQTT_CLIENT_STACK_SIZE = 1024 * 36;
const TickType_t deviceDelay = 200 / portTICK_PERIOD_MS, serverDelay = 50 / portTICK_PERIOD_MS, lcdDelay = 400 / portTICK_PERIOD_MS;
#ifndef SERVER_LOOP_YIELD_TICKS
#define SERVER_LOOP_YIELD_TICKS 1
#endif

#include "esp32_diag.h"
#ifdef SERVER_DIAGNOSTICS
volatile const char* g_diagWhere = "idle";
volatile unsigned long g_diagSince = 0;
volatile const char* g_diagLcdWhere = "idle";
volatile unsigned long g_diagLcdSince = 0;
volatile uint32_t diagLcdMaxHoldMs = 0, diagDeviceMaxHoldMs = 0, diagLcdHolds = 0, diagDeviceHolds = 0;
volatile uint32_t diagScheduleMaxMs = 0, diagDevicesMaxMs = 0;
#endif
//for starting and looping server task
void serverTask(void* params) {
    server.start();
    #ifdef SERVER_DIAGNOSTICS
    // Every 10 s: how often the server loop ran, the slowest pass, and how often it could
    // not get the shared lock - shows whether requests are starved or blocked.
    uint32_t diagSteps = 0, diagLockMisses = 0, diagMaxStepMs = 0, diagMaxWaitMs = 0;
    unsigned long diagLast = millis();
    #endif
    while (true) {
        #ifdef SERVER_DIAGNOSTICS
        unsigned long waitStart = millis();
        #endif
        if(xSemaphoreTake( lcdMutex, serverDelay ) == pdTRUE)
        {
            #ifdef SERVER_DIAGNOSTICS
            unsigned long stepStart = millis();
            diagMaxWaitMs = max(diagMaxWaitMs, (uint32_t)(stepStart - waitStart));
            #endif
            server.step();xSemaphoreGive(lcdMutex);            
            #ifdef SERVER_DIAGNOSTICS
            diagSteps++;
            diagMaxStepMs = max(diagMaxStepMs, (uint32_t)(millis() - stepStart));
            #endif
        }
        #ifdef SERVER_DIAGNOSTICS
        else diagLockMisses++;
        if(millis() - diagLast >= 10000){
            Serial.printf("[diag] server loops=%u lock-misses=%u max-lock-wait=%ums max-loop=%ums heap=%u stations=%d\n",
                diagSteps, diagLockMisses, diagMaxWaitMs, diagMaxStepMs, ESP.getFreeHeap(), WiFi.softAPgetStationNum());
            Serial.printf("[diag] device task at '%s' for %lums, lcd task at '%s' for %lums\n",
                (const char*)g_diagWhere, millis() - g_diagSince, (const char*)g_diagLcdWhere, millis() - g_diagLcdSince);
            Serial.printf("[diag] lcd loop ran %u times (max %ums, not under lock), lock held by device %u times (max %ums: schedule %ums, devices %ums)\n",
                diagLcdHolds, diagLcdMaxHoldMs, diagDeviceHolds, diagDeviceMaxHoldMs, diagScheduleMaxMs, diagDevicesMaxMs);
            diagSteps = diagLockMisses = diagMaxStepMs = diagMaxWaitMs = 0;
            diagLcdHolds = diagLcdMaxHoldMs = diagDeviceHolds = diagDeviceMaxHoldMs = diagScheduleMaxMs = diagDevicesMaxMs = 0;
            diagLast = millis();
        }
        #endif
        // Each pass advances every connection by one step (request line, headers, handling,
        // response), so a 50 ms pause here added ~0.2-0.5 s to every request. Yield briefly
        // instead; the lock wait above still caps how long other tasks can be held off.
        vTaskDelay(SERVER_LOOP_YIELD_TICKS);         
               
    }

    //#ifdef DEBUG
    Serial.printf(PROGMEM("[SERVER] Shutting down\n"));
    //#endif
    vTaskDelete( NULL );
}

void lcdTask(void* params){
    lcd.begin(PIN_SDA, PIN_SCL);
    lcd.clear();
    lcd.setTitle(string_format("ESP32 IoT v%s", FIRMWARE_VERSION).c_str());
    lcd.setDetails(string_format("Built %s",FIRMWARE_DATE).c_str());
    while(true){
        // Not under the shared lock: LCD output is slow I2C and a missing or misbehaving
        // display blocked inside Wire, which starved the web server and the device loop.
        // lcd.loop() takes the lock itself only to copy the device snapshot it shows.
        #ifdef SERVER_DIAGNOSTICS
        unsigned long t0 = millis();
        #endif
        DIAG_LCD("loop");
        lcd.loop();       
        DIAG_LCD("idle");
        #ifdef SERVER_DIAGNOSTICS
        uint32_t held = millis() - t0; diagLcdHolds++; if(held > diagLcdMaxHoldMs) diagLcdMaxHoldMs = held;
        #endif
        vTaskDelay(lcdDelay);
    }    
}

unsigned long deviceLoopTime = 0;
void deviceTask(void* params) {
    // Devices and schedules stay off until the device has been set up (an administrator
    // exists): nothing should switch outputs on a device nobody has configured yet, and
    // setup should have the processor to itself.
    if(esp32_authentication::setupRequired()){
        Serial.println("Device and schedule processing paused until setup is complete.");
        while(esp32_authentication::setupRequired())
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        Serial.println("Setup complete; starting device and schedule processing.");
    }
    deviceManager.onInit();
    scheduleManager.onInit();

    while(true){
        if(xSemaphoreTake( lcdMutex, deviceDelay) == pdTRUE){
            #ifdef SERVER_DIAGNOSTICS
            unsigned long t0 = millis();
            #endif
            DIAG_AT("schedule");
            scheduleManager.onLoop();        
            #ifdef SERVER_DIAGNOSTICS
            unsigned long t1 = millis();
            #endif
            DIAG_AT("devices");
            deviceManager.onLoop();
            DIAG_AT("idle");
            #ifdef SERVER_DIAGNOSTICS
            unsigned long t2 = millis();
            diagDeviceHolds++;
            if(t2 - t0 > diagDeviceMaxHoldMs) diagDeviceMaxHoldMs = t2 - t0;
            if(t1 - t0 > diagScheduleMaxMs) diagScheduleMaxMs = t1 - t0;
            if(t2 - t1 > diagDevicesMaxMs) diagDevicesMaxMs = t2 - t1;
            #endif
            xSemaphoreGive(lcdMutex);
        }
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


// mbedtls in this core allocates only from internal RAM (CONFIG_MBEDTLS_INTERNAL_MEM_ALLOC).
// Each TLS connection needs ~40 KB of record buffers, so a few open connections exhausted the
// ~170 KB of internal heap ("SSL_new failed"). Put large mbedtls allocations in PSRAM when the
// board has it; small, frequent ones stay internal.
static void* mbedtlsCalloc(size_t count, size_t size){
    size_t bytes = count * size;
    if(bytes >= 1024){
        void* p = heap_caps_calloc(count, size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
        if(p != nullptr) return p;
    }
    return heap_caps_calloc(count, size, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
}
static void mbedtlsFree(void* p){
    heap_caps_free(p);
}

void esp32_system_start(){
    #ifndef MBEDTLS_INTERNAL_RAM_ONLY
    if(psramFound())
        mbedtls_platform_set_calloc_free(mbedtlsCalloc, mbedtlsFree);
    #endif
    lcdMutex = xSemaphoreCreateMutex();
    Serial.printf("Starting esp32-mvc-controller firmware (v %s)\n", FIRMWARE_VERSION);
    // Initialize LCD    
    xTaskCreatePinnedToCore(lcdTask, "lcd", LCD_STACK_SIZE, NULL, 1, lcdTaskHandle, ARDUINO_RUNNING_CORE); 
    //xTaskCreate(lcdTask, "lcd",LCD_STACK_SIZE, NULL, 2, lcdTaskHandle);
    
    //debug logging
    lcd.setDetails(string_format("Serial logging (%d baud)..",115200).c_str());
    Serial.begin(115200);
    
    //esp32 filesystem manager
    lcd.setDetails("Starting disk management..");
    disk.start();    

    //Connect to wifi
    lcd.setDetails("Connecting to Wifi..");
    wifi.start();   
    
    logger.start();    
    
    lcd.setDetails("Init Server");
    //Create Server
    xTaskCreatePinnedToCore(serverTask, "secureserver", SERVER_STACK_SIZE, NULL, 2 | portPRIVILEGE_BIT, serverTaskHandle, ARDUINO_RUNNING_CORE); 
    
    lcd.setDetails("Init Device Manager");
    //Create Device Manager
    xTaskCreatePinnedToCore(deviceTask, "devicemanager",DEVICE_MANAGER_STACK_SIZE, NULL, 2, deviceTaskHandle, ARDUINO_RUNNING_CORE);
    

    lcd.setDetails("Init MQTT");
    //Create MQTT Client
    xTaskCreate(mqttClientTask, "mqttclient",MQTT_CLIENT_STACK_SIZE, NULL, tskIDLE_PRIORITY, mqttClientTaskHandle);
   
    //Register shutdown handler
    esp_register_shutdown_handler(onShutdown);

    //Done
    logger.logInfo(string_format("System started @ %s\n", getCurrentTime().c_str()));    
    lcd.set("System started",getCurrentTime().c_str());

}
