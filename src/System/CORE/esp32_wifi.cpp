#include "esp32_wifi.h"
#include "string_helper.h"
#include <esp_mac.h>
#include "lwip/sockets.h"
#include "esp32_captive_dns.h"
#ifndef WIFI_JOIN_TIMEOUT_MS
#define WIFI_JOIN_TIMEOUT_MS 10000 // override with -DWIFI_JOIN_TIMEOUT_MS=... for a slow router
#endif
#include "esp_netif.h"
#include "dhcpserver/dhcpserver.h"

// extern esp32_wifi wifi;
// void IRAM_ATTR diableWifi(){
//     wifi.end();    
// }

int esp32_wifi::_dnsSocket = -1;
bool esp32_wifi::_captivePortal = false;
bool esp32_wifi::_logDns = false;
std::string esp32_wifi::_apIp = "";
std::string esp32_wifi::_hostname = "";

esp32_wifi::esp32_wifi(){
    
}

void esp32_wifi::dnsTask(void* params){
    // Answers every DNS query with the access point's address (see esp32_captive_dns.h).
    uint8_t ip[4];
    IPAddress apIp = WiFi.softAPIP();
    for(int i = 0; i < 4; i++) ip[i] = apIp[i];
    uint8_t query[512], reply[528];
    char name[128];
    int logged = 0;
    while(_captivePortal){
        struct sockaddr_in client;
        socklen_t clientLength = sizeof(client);
        int length = recvfrom(_dnsSocket, query, sizeof(query), 0, (struct sockaddr*)&client, &clientLength);
        if(length <= 0) continue; // receive timeout: re-check _captivePortal
        uint16_t qtype = 0;
        size_t replyLength = captive_dns_build_reply(query, length, ip, reply, sizeof(reply), name, sizeof(name), &qtype);
        if(replyLength == 0) continue;
        sendto(_dnsSocket, reply, replyLength, 0, (struct sockaddr*)&client, clientLength);
        if(_logDns && logged < 40){ // system.logging.dns; 40 is enough to see what a phone asks for without flooding the console
            Serial.printf("[portal] DNS %s %s -> %s\n", qtype == 1 ? "A" : qtype == 28 ? "AAAA" : qtype == 65 ? "HTTPS" : "type",
                name, qtype == 1 || qtype == 255 ? apIp.toString().c_str() : "(no address)");
            if(++logged == 40) Serial.println("[portal] DNS: further queries not logged");
        }
    }
    close(_dnsSocket);
    _dnsSocket = -1;
    vTaskDelete(NULL);
}

static int openDnsSocket(){
    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if(sock < 0) return -1;
    struct sockaddr_in address;
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_port = htons(53);
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    if(bind(sock, (struct sockaddr*)&address, sizeof(address)) != 0){
        close(sock);
        return -1;
    }
    struct timeval timeout = { 1, 0 }; // lets the task notice when the portal stops
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    return sock;
}

bool esp32_wifi::captivePortalActive(){
    return _captivePortal;
}

std::string esp32_wifi::captivePortalUrl(){
    return "http://" + _apIp + "/";
}

std::string esp32_wifi::captivePortalIp(){
    return _apIp;
}

bool esp32_wifi::isForeignHost(const std::string& hostHeader){
    if(hostHeader.empty()) return false; // HTTP/1.0 client; nothing to compare
    std::string host = hostHeader.substr(0, hostHeader.find(':')); // drop port
    if(host == _apIp) return false;
    if(!_hostname.empty()){
        if(strcasecmp(host.c_str(), _hostname.c_str()) == 0) return false;
        if(strcasecmp(host.c_str(), (_hostname + ".local").c_str()) == 0) return false;
    }
    return true;
}

bool esp32_wifi::start(){
    Serial.println("Setting up WiFi");

    IPAddress addr = IPAddress(10, 0, 1, 1);
    DynamicJsonDocument wifiConfig(2048);
    esp32_config::getConfigSection("wifi", &wifiConfig);
    DynamicJsonDocument systemConfig(2048);
    esp32_config::getConfigSection("system", &systemConfig);
    DynamicJsonDocument serverConfig(1024);
    esp32_config::getConfigSection("server", &serverConfig);


    bool isAP = wifiConfig["mode"].isNull() ? true : strcmp(wifiConfig["mode"].as<const char *>(), "access-point") == 0;

    string ntpServer = systemConfig["ntp"]["server"].as<string>();
    string timeZone = systemConfig["ntp"]["timezone"].as<string>();    
    
    string network = wifiConfig["network"].as<string>();
    string password = wifiConfig["password"].as<string>();
    

    //if wifi settings not empty and not configured for access point
    bool trySTA = !network.empty() && !password.empty() && !isAP;
    string hostname = systemConfig["hostname"].as<string>();
    bool enableMDNS = systemConfig["enableMDNS"].as<bool>();
    _logDns = systemConfig["logging"]["dns"] | false; // captive-portal DNS query log on the serial console

    if(serverConfig["disableWifiAfter"].is<int>()){
        // //set up timer to disable wifi
        // timerWifiDisable = timerBegin(2,8000,true); // 48,000 / 80MHz = 0.0006
        // timerAttachInterrupt(timerWifiDisable, &diableWifi, true);
        // timerAlarmWrite(timerWifiDisable, 100000 * 1, true); // 0.0006 x 100,000 = 60 seconds
        // timerAlarmEnable(timerWifiDisable);
        // //set timer to number of minutes specified in config
    }
    
    if(isAP)
        Serial.println("Wi-Fi mode is access point (or not configured)");
    else if(network.empty())
        Serial.println("Wi-Fi client mode, but no network is configured");
    else if(password.empty())
        Serial.printf("Wi-Fi client mode, but no password is stored for \"%s\"\n", network.c_str());
    if(trySTA){
        Serial.printf("Joining Wi-Fi network \"%s\"...\n", network.c_str());
        #ifndef WIFI_NO_EVENT_LOGGING
        // Say why a join attempt fails (reported by the Wi-Fi driver).
        static wifi_event_id_t joinLogger = 0;
        if(joinLogger == 0)
            joinLogger = WiFi.onEvent([](WiFiEvent_t event, WiFiEventInfo_t info){
                uint8_t reason = info.wifi_sta_disconnected.reason;
                const char* meaning =
                    reason == WIFI_REASON_NO_AP_FOUND ? "network not found (out of range, 5 GHz only, or name mismatch)" :
                    reason == WIFI_REASON_AUTH_FAIL ? "authentication failed" :
                    reason == WIFI_REASON_4WAY_HANDSHAKE_TIMEOUT || reason == WIFI_REASON_HANDSHAKE_TIMEOUT ? "handshake failed (usually a wrong password)" :
                    reason == WIFI_REASON_ASSOC_FAIL ? "association failed" :
                    reason == WIFI_REASON_AUTH_EXPIRE ? "authentication expired" :
                    reason == WIFI_REASON_BEACON_TIMEOUT ? "lost the network's beacon" :
                    reason == WIFI_REASON_CONNECTION_FAIL ? "connection failed" :
                    "see esp_wifi_types.h wifi_err_reason_t";
                Serial.printf("[wifi] join attempt failed: reason %u - %s\n", reason, meaning);
            }, ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
        static wifi_event_id_t stageLogger = 0;
        if(stageLogger == 0)
            stageLogger = WiFi.onEvent([](WiFiEvent_t event, WiFiEventInfo_t info){
                if(event == ARDUINO_EVENT_WIFI_STA_CONNECTED)
                    Serial.printf("[wifi] associated with the access point (channel %u, auth mode %u); waiting for an address\n",
                        info.wifi_sta_connected.channel, info.wifi_sta_connected.authmode);
                else if(event == ARDUINO_EVENT_WIFI_STA_GOT_IP)
                    Serial.printf("[wifi] got address %s\n", IPAddress(info.got_ip.ip_info.ip.addr).toString().c_str());
            });
        #endif
        Serial.printf("[wifi] network name is %u characters, password %u characters\n",
            (unsigned)network.length(), (unsigned)password.length());
        bool isDHCP =  wifiConfig["dhcp"].as<bool>();
        if(isDHCP)
            Serial.println("[wifi] address: DHCP");
        else
            Serial.printf("[wifi] address: fixed ip=%s subnet=%s gateway=%s dns=%s (wifi.dhcp is %s)\n",
                wifiConfig["ip"] | "(none)", wifiConfig["subnet"] | "(none)", wifiConfig["gateway"] | "(none)",
                wifiConfig["dns"] | "(none)", wifiConfig["dhcp"].isNull() ? "missing" : "false");
        
        
        if(!isDHCP){
            string ip = wifiConfig["ip"].as<string>();
            string subnet = wifiConfig["subnet"].as<string>();
            string gateway = wifiConfig["gateway"].as<string>();
            string dns = wifiConfig["dns"].as<string>();

            // The parsing used to sit inside an #if DEBUG block, so without DEBUG every address
            // stayed 0.0.0.0 and WiFi.config() silently fell back to DHCP.
            IPAddress ipAddress, subnetAddress, gatewayAddress, dnsAddress;
            bool ipOk = ipAddress.fromString(ip.c_str());
            bool subnetOk = subnetAddress.fromString(subnet.c_str());
            bool gatewayOk = gatewayAddress.fromString(gateway.c_str());
            bool dnsOk = dnsAddress.fromString(dns.c_str());
            if(!ipOk || !subnetOk || !gatewayOk)
                Serial.printf("[wifi] fixed address settings are incomplete (ip %s, subnet %s, gateway %s); using DHCP instead\n",
                    ipOk ? "ok" : "invalid", subnetOk ? "ok" : "invalid", gatewayOk ? "ok" : "invalid");
            else {
                if(!dnsOk) dnsAddress = gatewayAddress; // a missing DNS server falls back to the gateway
                Serial.println("[wifi] applying fixed address");
                WiFi.config(ipAddress, gatewayAddress, subnetAddress, dnsAddress);
            }
        }
    
        WiFi.begin(network.c_str(), password.c_str());
        
        unsigned long startConnectTime = millis(), lastStatus = 0;
        while (WiFi.status() != WL_CONNECTED) {
            delay(250);
            if (millis() - lastStatus >= 3000) {
                lastStatus = millis();
                Serial.printf("[wifi] %lus: status %d\n", (millis() - startConnectTime) / 1000, (int)WiFi.status());
            }
            if (millis() - startConnectTime > WIFI_JOIN_TIMEOUT_MS)
            {
                Serial.printf("Could not join \"%s\" within %d s (status %d); starting access point\n",
                    network.c_str(), WIFI_JOIN_TIMEOUT_MS / 1000, (int)WiFi.status());
                WiFi.disconnect(true,true);
                break;
            }
        }        
    }

    if(WiFi.status() != WL_CONNECTED){
        //failed to start in STA mode. Start in AP mode
        bool customIP = true;
        string ip = wifiConfig["ap"]["ip"].as<string>();
        string subnet = wifiConfig["ap"]["subnet"].as<string>();
        // Named from the MAC, not a random number: the name stays the same across reboots,
        // so phones recognise the network and the user can find it again.
        uint8_t mac[6];
        esp_read_mac(mac, ESP_MAC_WIFI_SOFTAP);
        string apName = wifiConfig["ap"]["name"].isNull() || strlen(wifiConfig["ap"]["name"] | "") == 0
            ? string_format("ESP32Host-%02X%02X%02X", mac[3], mac[4], mac[5])
            : wifiConfig["ap"]["name"].as<string>();
        string apPassword = wifiConfig["ap"]["password"] | "";

        IPAddress ipAddress, subnetAddress;
        if(!ipAddress.fromString(ip.c_str()) ){
            #ifdef DEBUG
            Serial.println("Failed to parse ip address from config file");
            #endif
            customIP = false;
        }
        if(!subnetAddress.fromString(subnet.c_str()) ){
            #ifdef DEBUG
            Serial.println("Failed to parse subnet address from config file");
            #endif
            customIP = false;
        }       

        WiFi.mode(WIFI_AP);
        // Always configure the AP address explicitly (as the core's CaptivePortal example does),
        // so the DHCP offer carries this device as gateway; phones may not run their portal
        // check on a network that offers no default route.
        if(!customIP){
            ipAddress = IPAddress(192, 168, 4, 1);
            subnetAddress = IPAddress(255, 255, 255, 0);
        }
        WiFi.softAPConfig(ipAddress, ipAddress, subnetAddress);

        // A WPA2 key is used only when one is configured (8-63 characters). Otherwise the
        // access point is open: the network name is broadcast, so using it as the password
        // protected nothing. Every page still requires a device login over HTTPS, and on a
        // new device the first login also needs the setup code from the serial console.
        bool secured = apPassword.length() >= 8 && apPassword.length() <= 63;
        // Setup-mode diagnostics: show each phone joining, getting an address and leaving.
        WiFi.onEvent([](WiFiEvent_t event, WiFiEventInfo_t info){
            switch(event){
                case ARDUINO_EVENT_WIFI_AP_STACONNECTED:
                    Serial.printf("[portal] client %02X:%02X:%02X:%02X:%02X:%02X joined\n",
                        info.wifi_ap_staconnected.mac[0], info.wifi_ap_staconnected.mac[1], info.wifi_ap_staconnected.mac[2],
                        info.wifi_ap_staconnected.mac[3], info.wifi_ap_staconnected.mac[4], info.wifi_ap_staconnected.mac[5]);
                    break;
                case ARDUINO_EVENT_WIFI_AP_STAIPASSIGNED:
                    Serial.printf("[portal] client given address %s\n",
                        IPAddress(info.wifi_ap_staipassigned.ip.addr).toString().c_str());
                    break;
                case ARDUINO_EVENT_WIFI_AP_STADISCONNECTED:
                    Serial.println("[portal] client left");
                    break;
                default: break;
            }
        });
        bool apStarted = WiFi.softAP(apName.c_str(), secured ? apPassword.c_str() : nullptr, 7, 0, 4);

        _apIp = WiFi.softAPIP().toString().c_str();
        if(apStarted){
            // Make the DHCP offer name this device as both router and DNS server explicitly.
            esp_netif_t* apNetif = esp_netif_get_handle_from_ifkey("WIFI_AP_DEF");
            if(apNetif != nullptr){
                esp_netif_dhcps_stop(apNetif);
                dhcps_offer_t routerOffer = OFFER_ROUTER, dnsOffer = OFFER_DNS;
                esp_netif_dhcps_option(apNetif, ESP_NETIF_OP_SET, ESP_NETIF_ROUTER_SOLICITATION_ADDRESS, &routerOffer, sizeof(routerOffer));
                esp_netif_dhcps_option(apNetif, ESP_NETIF_OP_SET, ESP_NETIF_DOMAIN_NAME_SERVER, &dnsOffer, sizeof(dnsOffer));
                esp_netif_dns_info_t dnsInfo;
                memset(&dnsInfo, 0, sizeof(dnsInfo));
                dnsInfo.ip.type = ESP_IPADDR_TYPE_V4;
                dnsInfo.ip.u_addr.ip4.addr = (uint32_t)WiFi.softAPIP();
                esp_netif_set_dns_info(apNetif, ESP_NETIF_DNS_MAIN, &dnsInfo);
                esp_err_t dhcpErr = esp_netif_dhcps_start(apNetif);
                if(dhcpErr != ESP_OK)
                    Serial.printf("[portal] DHCP server restart failed: 0x%x\n", dhcpErr);
            }
            _dnsSocket = openDnsSocket();
            _captivePortal = _dnsSocket >= 0;
            if(_captivePortal)
                xTaskCreate(dnsTask, "captive-dns", 4096, NULL, 1, NULL);
            else
                Serial.println("[portal] could not open DNS port 53; no sign-in prompt will appear");
        }
        Serial.printf("Access point \"%s\" (%s) started at http://%s/ %s\n",
            apName.c_str(), secured ? "WPA2" : "open", _apIp.c_str(),
            _captivePortal ? "with captive portal" : "");
    }

    _hostname = hostname;
    if(hostname.length() > 0){
        #ifdef DEBUG
        Serial.printf("Setting hostname to %s\n", hostname.c_str());
        #endif
        WiFi.setHostname(hostname.c_str());
        if(enableMDNS)
            MDNS.begin(hostname.c_str());        
    }
    
    //#ifdef DEBUG
    Serial.print("Connected. IP=");
    Serial.println(WiFi.getMode() == wifi_mode_t::WIFI_MODE_AP ? WiFi.softAPIP() :  WiFi.localIP());
    //#endif

    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, ntpServer.empty() ? "pool.ntp.org" : ntpServer.c_str());
    sntp_init();    
    
    
    if(WiFi.getMode() == WIFI_MODE_STA){
        setenv("TZ", timeZone.c_str(), 1);
        tzset();
        #ifdef DEBUG
        Serial.printf("Initializing NTP server %s\n", ntpServer.empty() ? "pool.ntp.org" : ntpServer.c_str());
        Serial.printf("Waiting for NTP time..\n");
        #endif
        struct timeval tv = { .tv_sec = 0, .tv_usec = 0 };
        do{
            gettimeofday(&tv, NULL);
            #ifdef DEBUG
            if(tv.tv_usec%500==0) Serial.print(".");
            #endif
            
        } while(tv.tv_sec < 15); //15 seconds to init NTP
    }
    //#ifdef DEBUG
    Serial.println(" done!");
    //#endif

    return WiFi.status() == WL_CONNECTED;
}

bool esp32_wifi::end(){
    #ifdef DEBUG
    Serial.println("Disabling Wifi...");
    #endif
    if(WiFi.getMode() == WiFiMode_t::WIFI_MODE_STA)
        WiFi.disconnect(true,true);
    else if(WiFi.getMode() == WiFiMode_t::WIFI_MODE_AP)
        WiFi.disconnect(true,true);
    if(_captivePortal){
        _captivePortal = false; // dns task closes its socket and exits within a second
    }
    MDNS.end();
    // timerDetachInterrupt(timerWifiDisable);
    // timerEnd(timerWifiDisable);
    return !WiFi.isConnected();
}