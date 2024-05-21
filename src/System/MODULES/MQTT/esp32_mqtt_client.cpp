#include "esp32_mqtt_client.hpp"
#include <string_extensions.h>

void esp32_mqtt_client::start()
{
    StaticJsonDocument<2048> systemConfig;
    esp32_config::getConfigSection("system", &systemConfig);
    if(systemConfig["mqtt"].isNull()) return; //no init, not configured

    _insecureMode = systemConfig["mqtt"]["insecure"].isNull() || systemConfig["mqtt"]["insecure"].as<bool>();

    if(_insecureMode)
        networkSecure.setInsecure();

    _brokerUri = systemConfig["mqtt"]["broker"].isNull() ? "test.mosquitto.org" :  systemConfig["mqtt"]["broker"].as<const char*>();
    _port = systemConfig["mqtt"]["port"].isNull() ? 8883 : systemConfig["mqtt"]["port"].as<int>();
    _hostname = systemConfig["hostname"].as<const char *>();

    
    Serial.printf("Setting MQTT broker to %s on port %d.\n", _brokerUri.c_str(), _port);
    if(_port == 8883){ //load certs for encrypted traffic
        StaticJsonDocument<1024> serverConfig;
        esp32_config::getConfigSection("server", &serverConfig);        
        esp32_cert_base* certManager;
        if(serverConfig["certificates"]["source"].isNull() || 
            iequals(serverConfig["certificates"]["source"].as<const char*>(),"nvs",3))
                certManager = new esp32_cert_nvs();        
        
        else
            certManager = new esp32_cert_spiffs();        
        
        //initialize certificates
        certManager->loadCertificates(); 
        networkSecure.setCertificate((const char*) certManager->getCert()->getCertData());
        networkSecure.setPrivateKey((const char*) certManager->getCert()->getPKData());
        client = PubSubClient(networkSecure);
    } else{ //unecrypted    
        client = PubSubClient(networkInsecure);
    }

    client.setKeepAlive(60);

    
    client.setServer(_brokerUri.c_str(),_port);
    //client.setServer("test.mosquitto.org",8883);    
    _started = true;
    logger.logDebug("MQTT Client started.");
    Serial.println("MQTT Client started. Connecting..");
    //client.begin(_brokerUri.c_str(), network);
    connect();
   Serial.println("MQTT Client initialization completed");
}

bool esp32_mqtt_client::connect()
{
    if(!_started) start();
    if(_port == 8883){
        if(networkSecure.connected()) networkSecure.stop();
            networkSecure.connect(_brokerUri.c_str(),_port);
    } else{
        if(networkInsecure.connected()) networkInsecure.stop();
        Serial.printf("Connecting to INSECURE network %s on port %d\n", _brokerUri.c_str(),_port);
            if(networkInsecure.connect(_brokerUri.c_str(),_port))
                Serial.println("MQTT Connected!");
    }
    
    
    // network.print("Test string");
    // unsigned long startAttempt = millis();
    // //retry connect for 10 seconds
    // //while(!client.connect("ESP32_CLIENT#1", "username", "password") && (millis() - startAttempt < 10000)){
    // //client.connect("ESP32_CLIENT#1", "username", "password");
    // lwmqtt_connect_options_t options = lwmqtt_default_connect_options;
    // options.keep_alive = true;
    // options.clean_session = true;
    // options.client_id = lwmqtt_string("ESP32_CLIENT#1");
    // auto error = lwmqtt_connect(&lwmqtt_client, &options, will, _timeout);
    // if(error != lwmqtt_err_t::LWMQTT_SUCCESS)
    // if(!lwmqtt_client.network){
    //     Serial.printf("Failed to connect to broker %s on port %d, %d\n", _brokerUri.c_str(), _port, error);
    //     return false;
    // }
    // logger.logDebug(string_format("Connecting to MQTT broker %s on port %d: %s",
    //     _brokerUri.c_str(),
    //     _port,
    //     isConnected() ? "Connected" : "Disconnected"));
    return true;
}

void esp32_mqtt_client::loop(){
    //if there is anything to piublish, do so
     if(!isConnected())
        connect();
    //Serial.printf("Processing %d publications\n", _publishList.size());
    int qos = 0;
    for(int idx=0; idx< _publishList.size(); idx++){
        //Serial.printf("Publishing to topic %s %s", _publishList[idx].first, _publishList[idx].second);
        client.publish(_publishList[idx].first,_publishList[idx].second);
        int writeError = client.getWriteError();
        logger.logDebug(string_format("Publishing. Topic: [%s] Value [%s] - Result %d", _publishList[idx].first, _publishList[idx].second ? "suceeded" : "failed", writeError));

    }
    _publishList.clear();
    client.loop();
}



void esp32_mqtt_client::disconnect()
{
    _port == 8883 ? networkSecure.stop() : networkInsecure.stop();
}

void esp32_mqtt_client::subscribe(const char *topic, void (*callback)(char* topic, uint8_t* payload, unsigned int size))
{
    // client.subscribe(topic);
    // client.onMessage(callback);
    logger.logDebug(string_format("Subscribed to %s\n", topic));
    client.subscribe(topic,2);
    client.setCallback(callback);
}


bool esp32_mqtt_client::publish(const char *topic, const char* data)
{
    _publishList.push_back(pair<const char *,const char *>(topic, data));
   
//   // prepare message
//   lwmqtt_message_t message = lwmqtt_default_message;
//   message.payload = (uint8_t *)data;
//   message.payload_len = strlen(data);
//   message.retained = false;
//   message.qos = lwmqtt_qos_t(qos);

//   // prepare options
//   lwmqtt_publish_options_t options = lwmqtt_default_publish_options;

  

//   // publish message
//   auto error = lwmqtt_publish(&lwmqtt_client, &options, lwmqtt_string(topic), message, _timeout);
//   logger.logDebug(string_format("%s publishing %s to topic %s\n", error == LWMQTT_SUCCESS ? "Suceeded" : "Failed", data, topic));
//   if (error != LWMQTT_SUCCESS) {
//     // close connection
//     disconnect();

    return true;
  //}

 // return true;
    
   //bool result = client.publish(topic,data);
   
}

void esp32_mqtt_client::callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i=0;i<length;i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}





//