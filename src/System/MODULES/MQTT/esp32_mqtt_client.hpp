#ifndef _ESP32_MQTT_CLIENT_H
#define _ESP32_MQTT_CLIENT_H
#include <WiFiClientSecure.h>
//#include <MQTT.h>
//#include "PubSubClient.h"
#include <System/MODULES/LOGGING/esp32_logging.hpp>
#include <System/CORE/esp32_config.h>
#include <System/CORE/esp32_server.h>
extern esp32_logging logger;
extern esp32_server server;

#define ASYNC_TCP_SSL_ENABLED       true
#include <AsyncMQTT_ESP32.h>

extern "C"
{
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
}
#if ASYNC_TCP_SSL_ENABLED

#define MQTT_SECURE     true

const uint8_t MQTT_SERVER_FINGERPRINT[] = {0x7e, 0x36, 0x22, 0x01, 0xf9, 0x7e, 0x99, 0x2f, 0xc5, 0xdb, 0x3d, 0xbe, 0xac, 0x48, 0x67, 0x5b, 0x5d, 0x47, 0x94, 0xd2};
const char *PubTopic  = "async-mqtt/ESP32_SSL_Pub";               // Topic to publish

#define MQTT_PORT       8883

#else

const char *PubTopic  = "async-mqtt/ESP32_Pub";                   // Topic to publish

#define MQTT_PORT       1883

#endif

class esp32_mqtt_client{
    public:
        void start(); 
        void loop();
        bool publish(const char * topic, const char* data);

        void subscribe(const char * topic, void (*callback)(char* topic, uint8_t* payload, unsigned int length));
    private:
        bool connect();
        void disconnect();

        bool isConnected(){
            _port == 8883 ? networkSecure.connected() : networkInsecure.connected();
        }
        WiFiClientSecure networkSecure;
        WiFiClient networkInsecure;
        //PubSubClient client;
        //MQTTClient client;

        void callback(char* topic, byte* payload, unsigned int length);

        bool _insecureMode;
        bool _started = false;
        string _brokerUri;
        string _hostname;
        int _port;
        int _timeout;

        // lwmqtt_client_t lwmqtt_client = lwmqtt_client_t();
        // lwmqtt_will_t *will = new lwmqtt_will_t();
        vector<pair<const char *,const char *>> _publishList;


        AsyncMqttClient mqttClient;
        TimerHandle_t mqttReconnectTimer;
        TimerHandle_t wifiReconnectTimer;

};


#endif

