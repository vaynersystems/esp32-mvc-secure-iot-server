#include "esp32_wifi.h"

// extern esp32_wifi wifi;
// void IRAM_ATTR diableWifi(){
//     wifi.end();    
// }

esp32_wifi::esp32_wifi(){
    
}

bool esp32_wifi::start(){
    Serial.println("Setting up WiFi");

    IPAddress addr = IPAddress(10, 0, 1, 1);
    StaticJsonDocument<1024> wifiConfig;
    esp32_config::getConfigSection("wifi", &wifiConfig);
    StaticJsonDocument<1024> systemConfig;
    esp32_config::getConfigSection("system", &systemConfig);
    StaticJsonDocument<512> serverConfig;
    esp32_config::getConfigSection("server", &serverConfig);


    bool isAP =  strcmp(wifiConfig["mode"].as<const char *>(), "access-point") == 0;

    string ntpServer = systemConfig["ntp"]["server"].as<string>();
    string timeZone = systemConfig["ntp"]["timezone"].as<string>();    
    
    string network = wifiConfig["network"].as<string>();
    string password = wifiConfig["password"].as<string>();
    

    //if wifi settings not empty and not configured for access point
    bool trySTA = !network.empty() && !password.empty() && !isAP;
    string hostname = systemConfig["hostname"].as<string>();
    bool enableMDNS = systemConfig["enableMDNS"].as<bool>();

    if(serverConfig["disableWifiAfter"].is<int>()){
        // //set up timer to disable wifi
        // timerWifiDisable = timerBegin(2,8000,true); // 48,000 / 80MHz = 0.0006
        // timerAttachInterrupt(timerWifiDisable, &diableWifi, true);
        // timerAlarmWrite(timerWifiDisable, 100000 * 1, true); // 0.0006 x 100,000 = 60 seconds
        // timerAlarmEnable(timerWifiDisable);
        // //set timer to number of minutes specified in config
    }
    
    if(trySTA){
        Serial.printf("Connecting in STA mode to network %S with password %s\n", network.c_str(), password.c_str());        
        bool isDHCP =  wifiConfig["dhcp"].as<bool>();
        
        
        if(!isDHCP){
            string ip = wifiConfig["ip"].as<string>();
            string subnet = wifiConfig["subnet"].as<string>();
            string gateway = wifiConfig["gateway"].as<string>();
            string dns = wifiConfig["dns"].as<string>();

            IPAddress ipAddress, subnetAddress, gatewayAddress, dnsAddress;
            if(!ipAddress.fromString(ip.c_str()) )
                Serial.println("Failed to parse ip address from config file");
            if(!subnetAddress.fromString(subnet.c_str()) )
                Serial.println("Failed to parse subnet address from config file");
            if(!gatewayAddress.fromString(gateway.c_str()) )
                Serial.println("Failed to parse gateway address from config file");
            if(!dnsAddress.fromString(dns.c_str()) )
                Serial.println("Failed to parse dns address from config file");
            
            Serial.println("Configuring custom IP address");
            WiFi.config(ipAddress, gatewayAddress, subnetAddress, dnsAddress);
        }
    
        WiFi.begin(network.c_str(), password.c_str());
        
        unsigned long startConnectTime = millis();
        while (WiFi.status() != WL_CONNECTED) {
            Serial.print(".");
            delay(500);
            if (millis() - startConnectTime > 15000)
            {
                Serial.println("Failed to connect to wifi.");
                
                break;
            }
        }        
    }

    if(WiFi.status() != WL_CONNECTED){
        //failed to start in STA mode. Start in AP mode
        string ip = wifiConfig["ap"]["ip"].as<string>();
        string subnet = wifiConfig["ap"]["subnet"].as<string>();
        string apName = wifiConfig["ap"]["name"].as<string>();
        string apPassword = wifiConfig["ap"]["password"].as<string>();

        IPAddress ipAddress, subnetAddress;
        if(!ipAddress.fromString(ip.c_str()) )
            Serial.println("Failed to parse ip address from config file");
        if(!subnetAddress.fromString(subnet.c_str()) )
            Serial.println("Failed to parse subnet address from config file");

        WiFi.config(ipAddress, ipAddress,subnetAddress, addr, addr);
        WiFi.softAP(apName.c_str(), apPassword.c_str(), 7, 0, 3);
        Serial.println("Started Wifi in AP mode");
    }

    Serial.printf("Setting hostname to %s\n", hostname.c_str());
    WiFi.setHostname(hostname.c_str());
    if(enableMDNS)
        MDNS.begin(hostname.c_str());

  
    
    Serial.print("Connected. IP=");
    Serial.println(WiFi.localIP());

    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, ntpServer.empty() ? "pool.ntp.org" : ntpServer.c_str());
    sntp_init();
Serial.printf("Initialized NTP server %s\n", ntpServer.empty() ? "pool.ntp.org" : ntpServer.c_str());
    return WiFi.status() == WL_CONNECTED;
}

bool esp32_wifi::end(){
    Serial.println("Disabling Wifi...");
    if(WiFi.getMode() == WiFiMode_t::WIFI_MODE_STA)
        WiFi.disconnect(true,true);
    else if(WiFi.getMode() == WiFiMode_t::WIFI_MODE_AP)
        WiFi.disconnect(true,true);
    MDNS.end();
    // timerDetachInterrupt(timerWifiDisable);
    // timerEnd(timerWifiDisable);
}