#ifndef _ESP32_MQTT_CLIENT_H
#define _ESP32_MQTT_CLIENT_H
#include <WiFiClientSecure.h>
#include "PubSubClient.h"
#include <System/MODULES/LOGGING/esp32_logging.hpp>
#include <System/CORE/esp32_config.h>
#include <System/CORE/esp32_server.h>
extern esp32_logging logger;
extern esp32_server server;

class esp32_mqtt_client{
    public:
        void start(); 
        void loop();
        bool publish(const char * topic, const char* data);

        void subscribe(const char * topic, void (*callback)(char* topic, uint8_t* payload, unsigned int length));

        inline bool enabled() { return _enabled;}
    private:
        bool connect();
        void disconnect();

        bool isConnected(){
            return (_port == 8883 ? networkSecure.connected() : networkInsecure.connected());
        }
        WiFiClientSecure networkSecure;
        WiFiClient networkInsecure;

        void callback(char* topic, byte* payload, unsigned int length);

        bool _insecureMode;
        bool _started = false;
        bool _enabled = false;
        bool _subscribeEnabled = false;
        string _brokerUri;
        string _hostname;
        int _port;
        int _timeout;

        vector<pair<string, string>> _publishList;


        PubSubClient client;
        // TimerHandle_t mqttReconnectTimer;
        // TimerHandle_t wifiReconnectTimer;

};


#endif

