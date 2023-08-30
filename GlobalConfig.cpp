#include "GlobalConfig.h"

const char* CONFIG_FILE = "/config.ini";

GlobalConfig* GlobalConfig::mInstance = NULL;

GlobalConfig* GlobalConfig::GetConfig()
{
  if (mInstance == NULL) mInstance = new GlobalConfig();
    return mInstance;
}

GlobalConfig::GlobalConfig()
{
  
}

GlobalConfig::~GlobalConfig()
{
  
}

void GlobalConfig::writeConfig()
{
  File iniFile = SPIFFS.open(CONFIG_FILE, "w");
  if (iniFile) {
      iniFile.print("ap_ssid=" + AP_SSID + "\r\nap_pass=" + AP_PASS + "\r\nap_ip=" + Server_IP.toString() + "\r\nap_subnet=" + Subnet_Mask.toString() +  "\r\nenable_wifi=" + connectWifi + "\r\nwifi_ssid=" + WIFI_SSID + "\r\nwifi_pass=" + WIFI_PASS + "\r\n");
      iniFile.close();
  }
}

String GlobalConfig::getConfigValue(String configString, String key) {
	String sKey = key + "=";
	String delimiter = "\r\n";
	int position = configString.indexOf(sKey);
	if (position < 0) 
	{
		return "";
	}
	
	int start = position + sKey.length();
	int end = configString.indexOf(delimiter, start);

  String result = configString.substring(start, configString.length());
  if (end >= 0) 
  {
    result = configString.substring(start, end);
  }
  result.trim();
  return result;
}

void GlobalConfig::loadConfigFromStorage(void) 
{
  if (SPIFFS.exists(CONFIG_FILE)) {
    File iniFile = SPIFFS.open(CONFIG_FILE, "r");
    if (iniFile) {
      String iniData;
      while (iniFile.available()) {
        char chnk = iniFile.read();
        iniData += chnk;
      }
      iniFile.close();
      AP_SSID = getConfigValue(iniData, "ap_ssid");
      AP_PASS = getConfigValue(iniData, "ap_pass");
      Server_IP.fromString(getConfigValue(iniData, "ap_ip"));
      Subnet_Mask.fromString(getConfigValue(iniData, "ap_subnet"));
      connectWifi = getConfigValue(iniData, "enable_wifi").toInt() ;
      WIFI_SSID = getConfigValue(iniData, "wifi_ssid");
      WIFI_PASS = getConfigValue(iniData, "wifi_pass");
    }
  }
  else
  {
   writeConfig(); 
  }
}

void GlobalConfig::DebugOutput(void) {
#ifdef GLOBALCONFIG_DEBUG 
  Serial.println("***Config****");
  
  Serial.println("SSID: " + AP_SSID);
  Serial.println("Password: " + AP_PASS);
  Serial.println("WEB Server IP: " + Server_IP.toString());
  Serial.println("Subnet Mask: " + Subnet_Mask.toString());
  Serial.println("Connect Wifi: " + connectWifi);
  Serial.println("WIFI_SSID: " + WIFI_SSID);  
  Serial.println("WIFI_PASS: " + WIFI_PASS);    
  Serial.println("***END Config****");
#endif
}
