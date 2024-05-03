#include "System/Config.h"
#include "System/AUTH/cert.h"
#include "System/AUTH/key.h"

#include "System/CORE/esp32_fileio.h"
#include "System/CORE/esp32_server.h"
#include "System/CORE/esp32_wifi.h"

//TODO: store cert in SPIFFS
//#define CERT_FILE_CER   "/PRI/CERT.DER"
//#define CERT_FILE_KEY   "/PRI/CERM.KEY.DER"

public_cert pubCert = public_cert();
private_cert priCert = private_cert();
static SSLCert  cert = 
SSLCert((unsigned char*)pubCert.esp32_vaynersystems_com_der, pubCert.esp32_vaynersystems_com_der_len,
(unsigned char*)priCert.esp32_vaynersystems_com_key, priCert.esp32_vaynersystems_com_key_der_len);
esp32_server server(&cert);
esp32_wifi wifi;
esp32_fileio disk;
void serverTask(void* params);

#if CONFIG_FREERTOS_UNICORE
    #define ARDUINO_RUNNING_CORE 0
#else
    #define ARDUINO_RUNNING_CORE 1
#endif
extern const int STACK_SIZE = 1024*32;
unsigned long lastreport = millis();
String freeBytesHEAPSPretty("");


//for starting and looping server task
void serverTask(void* params) {
    server.start();
    while (true)         
        server.step();
}
#ifdef DEBUG
void heapMonitor(void* params){
    while(true)
        if(millis() - lastreport > 1000){
            esp32_fileio::PrettyFormat((size_t)esp_get_free_heap_size(), &freeBytesHEAPSPretty);
            Serial.printf("Free heap: %s\n", freeBytesHEAPSPretty.c_str());
            lastreport = millis(); 
        }
}
#endif
void setup() {
    // For logging
    Serial.begin(115200);
    //Get Spiffs Online
    disk.start();    
    // Connect to WiFi
    wifi.start();       
       
    //Create Server
    xTaskCreatePinnedToCore(serverTask, "secureserver", STACK_SIZE, NULL, 1, NULL, ARDUINO_RUNNING_CORE);
    #ifdef DEBUG
    //xTaskCreatePinnedToCore(heapMonitor, "heapmonitor", 256, NULL, 1, NULL, ARDUINO_RUNNING_CORE);
    #endif
    
}



void loop() {   

   
    
}
