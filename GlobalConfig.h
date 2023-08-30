#ifndef GLOBALCONFIG_H
#define GLOBALCONFIG_H

#include <ESP8266WiFi.h>

//#define GLOBALCONFIG_DEBUG
extern const char* CONFIG_FILE;

class GlobalConfig{
  public:

//
// Default configuration (these values are overwritten if a config.ini file exists on the device, see GlobalConfig.c)
//

// Access point Settings (how your clients will connect to the ESP8266 Access Point)
    String AP_SSID = "PS5_WEB_AP";
    String AP_PASS = "password";
    IPAddress Server_IP = IPAddress(10,1,1,1);
    IPAddress Subnet_Mask = IPAddress(255,255,255,0); 

//Wifi Connection (how your ESP8266 will connect to your Wifi Router)

// set connectWifi to true to connect to your Wifi Router (set WIFI_SSID and WIFI_PASS in wifisettings.h)
// if connectWifi is true (internet access + FakeDNS mode):
// - The ESP8266 will act as a FakeDNS Server with redirected and Blocked addresses (see SpoofedDomains and BlockedDomains below). All other domains will go through normally
// - The ESP8266 will act as a Wifi Repeater for Internet traffic
// - http access to SpoofedDomains will be redirected to the ESP8266 webserver
// Please note that the client devices (e.g. playstation) could choose to use another DNS Server (either in the Network settings or unknown to the user), in which case this whole blocking mechanism would be bypassed, so tread carefully
// If connectWifi is false (no internet access, local only):
// - All http requests are redirected to the ESP8266 webserver
// - There is no internet access for the PS4/PS5    
    boolean connectWifi = false; 
    String WIFI_SSID = ""; 
    String WIFI_PASS = "";

    static GlobalConfig* GetConfig();
    void loadConfigFromStorage(void);
    void writeConfig();   
    void DebugOutput();  

  private:
    static GlobalConfig * mInstance;
    String getConfigValue(String configString, String key);

  protected:
    GlobalConfig();
    ~GlobalConfig();
      
};




#endif
