// basic esp8266 server for PS5/PS4 Exploits. Tested with PS4 5.05 and PS5 3.xx. Supports internet access as a Wifi Repeater (NAT) + FakeDNS
// Code by wololo, based on https://github.com/stooged/PS5-Server, modified DNS Server from https://github.com/rubfi/esphole, NAT example https://github.com/AliBigdeli/Arduino-ESP8266-Repeater

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266WebServerSecure.h>
#include "DNSServer.h"
#include <FS.h>
#include <SPI.h>
#include <lwip/napt.h>

#include "GlobalConfig.h" //Config defaults are in that file
#include "WebAdmin.h" //Tools to handle the device from a web interface

GlobalConfig *conf = GlobalConfig::GetConfig();

//Note: SpoofedDomains and BlockedDomains are *not* regexps, we just do a dumb "string match". So technically, "playstation.net" is equivalent to "*playstation\.net*" regex
const char* SpoofedDomains[] = {"playstation.net"}; //Used only if conf->wifiConnect is true - all these domains will be redirected to the ESP8266 webServer in DNS Queries
const char* BlockedDomains[] = {"playstation.com", "scea.com", "sonyentertainmentnetwork.com"}; //Used only if conf->wifiConnect is true - all these domains will be blocked in DNS queries (IP set to 0)

// number of entries and number of ports in NAT table
// Needs to be reasonably small as it uses the heap, which is also used by SSL 
#define NAPT 100 //27 bytes per entry
#define NAPT_PORT 30 //14 bytes per entry

//Debug 
#define DEBUG_HEAP //The ESP8266 has very little heap memory, and both NAT routing and SSL Handshake use it.
//#define DEBUG_DNS //To debug issues related to Fake DNS
//#define DEBUG_WEBSERVER //Debug messages related to the Web Servers
#define CHRISTMAS_TREE //Builtin LED lights up when DNS replies with a Spoofed or Blocked Domain

#ifdef DEBUG_HEAP
int lastHeap = 0;
#endif

#ifdef CHRISTMAS_TREE
int lastTime = millis();
#endif

//static const char serverCert[] = "-----BEGIN CERTIFICATE-----\r\nMIIC1DCCAj2gAwIBAgIUFQgjEtkNYfmrrpNQKHVNl3+dl08wDQYJKoZIhvcNAQEL\r\nBQAwfDELMAkGA1UEBhMCVVMxEzARBgNVBAgMCkNhbGlmb3JuaWExEDAOBgNVBAcM\r\nB0ZyZW1vbnQxDDAKBgNVBAoMA2VzcDEMMAoGA1UECwwDZXNwMQwwCgYDVQQDDANl\r\nc3AxHDAaBgkqhkiG9w0BCQEWDWVzcEBlc3AubG9jYWwwHhcNMjEwMjIxMDAwMDQ4\r\nWhcNNDMwNzI4MDAwMDQ4WjB8MQswCQYDVQQGEwJVUzETMBEGA1UECAwKQ2FsaWZv\r\ncm5pYTEQMA4GA1UEBwwHRnJlbW9udDEMMAoGA1UECgwDZXNwMQwwCgYDVQQLDANl\r\nc3AxDDAKBgNVBAMMA2VzcDEcMBoGCSqGSIb3DQEJARYNZXNwQGVzcC5sb2NhbDCB\r\nnzANBgkqhkiG9w0BAQEFAAOBjQAwgYkCgYEAsrfFqlV5H0ajdAkkZ51HTOseOjYj\r\nNiaUD4MA5mIRonnph6EKIWb9Yl85vVa6yfVkGn3TFebQ96MMdTfZgLuP4ryCwe6Y\r\n+tZs2g6TjGbR0O6yuA8wQ2Ln7E0T05C8oOl88SGNV4tVL6hz64oMzuVebVDo0J9I\r\nybvL0O/LhMvC4x8CAwEAAaNTMFEwHQYDVR0OBBYEFCMQIU+pZQDVySXejfbIYbLQ\r\ncLXiMB8GA1UdIwQYMBaAFCMQIU+pZQDVySXejfbIYbLQcLXiMA8GA1UdEwEB/wQF\r\nMAMBAf8wDQYJKoZIhvcNAQELBQADgYEAFHPz3YhhXQYiERTGzt8r0LhNWdggr7t0\r\nWEVuAoEukjzv+3DVB2O+56NtDa++566gTXBGGar0pWfCwfWCEu5K6MBkBdm6Ub/A\r\nXDy+sRQTqH/jTFFh5lgxeq246kHWHGRad8664V5PoIh+OSa0G3CEB+BXy7WF82Qq\r\nqx0X6E/mDUU=\r\n-----END CERTIFICATE-----";
//static const char serverKey[] = "-----BEGIN PRIVATE KEY-----\r\nMIICdgIBADANBgkqhkiG9w0BAQEFAASCAmAwggJcAgEAAoGBALK3xapVeR9Go3QJ\r\nJGedR0zrHjo2IzYmlA+DAOZiEaJ56YehCiFm/WJfOb1Wusn1ZBp90xXm0PejDHU3\r\n2YC7j+K8gsHumPrWbNoOk4xm0dDusrgPMENi5+xNE9OQvKDpfPEhjVeLVS+oc+uK\r\nDM7lXm1Q6NCfSMm7y9Dvy4TLwuMfAgMBAAECgYEApKFbSeyQtfnpSlO9oGEmtDmG\r\nT9NdHl3tWFiydId0fTpWoKT9YwWvdnYIB12klbQicbDkyTEl4Gjnafd3ufmNsaH8\r\nZ9twopIdvvWDvGPIqGNjvTYcuczpXmQWiUnG5OTiVWI1XuZa3uZEGSFK9Ra6bE4g\r\nG2xklGZGdaqqcd6AVhECQQDnBXVXwBxExxSFppL8KUtWgyXAvJAEvkzvTOQfcCel\r\naIM5EEUofB7WZeMtDEKgBtoBl+i5PP+GnDF0zsjDFx2nAkEAxgqVQii6zURSVE2T\r\niJDihySXJ2bmLJUjRIi1nCs64I9Oz4fECVvGwZ1XU8Uzhh3ylyBSG2HjhzA5sTSC\r\n1a/tyQJAOgE12EWFE4PE1FXhm+ymXN9q8DyoEHjTilYNBRO88JwQLpi2NJcNixlj\r\n8+CbLeDqhfHlXfVB10OKa2CsKce5CwJAbhaN+DQJ+3dCSOjC3YSk2Dkn6VhTFW9m\r\nJn/UbNa/KPug9M5k1Er3RsO/OqsBxEk7hHUMD3qv74OIXpBxNnZQuQJASlwk5HZT\r\n7rULkr72fK/YYxkS0czBDIpTKqwklxU+xLSGWkSHvSvl7sK4TmQ1w8KVpjKlTCW9\r\nxKbbW0zVmGN6wQ==\r\n-----END PRIVATE KEY-----";

static const char serverCert[]= "-----BEGIN CERTIFICATE-----\r\nMIIBszCCAV2gAwIBAgIUa8NoeDqaT6pFP5PHokRpKMXrT6swDQYJKoZIhvcNAQEL\r\nBQAwLjELMAkGA1UEBhMCSlAxDjAMBgNVBAgMBVRva3lvMQ8wDQYDVQQKDAZXb2xv\r\nbG8wHhcNMjMwODMwMDE0NDIxWhcNMzQxMTE2MDE0NDIxWjAuMQswCQYDVQQGEwJK\r\nUDEOMAwGA1UECAwFVG9reW8xDzANBgNVBAoMBldvbG9sbzBcMA0GCSqGSIb3DQEB\r\nAQUAA0sAMEgCQQDFBUICgP5wFMcRCs8VRqrY3QZr72FtzvLtPTCMcgPsaBE60QJt\r\n+YAj+e2mAolbRPRk4FNNcrg5/XsURdfuAPfvAgMBAAGjUzBRMB0GA1UdDgQWBBSv\r\nywhrMAdpJLbVx4kF9hmuTpHlLjAfBgNVHSMEGDAWgBSvywhrMAdpJLbVx4kF9hmu\r\nTpHlLjAPBgNVHRMBAf8EBTADAQH/MA0GCSqGSIb3DQEBCwUAA0EAjH+jaG47Qngl\r\nPTGjPpFmJp+LK6YHPzhqrJrAN18WziGJiaeCsuWV7j8cgGgB0U0AOybcdYMrjMOR\r\n+u+B12nV2w==\r\n-----END CERTIFICATE-----";
static const char serverKey[] = "-----BEGIN PRIVATE KEY-----\r\nMIIBVAIBADANBgkqhkiG9w0BAQEFAASCAT4wggE6AgEAAkEAxQVCAoD+cBTHEQrP\r\nFUaq2N0Ga+9hbc7y7T0wjHID7GgROtECbfmAI/ntpgKJW0T0ZOBTTXK4Of17FEXX\r\n7gD37wIDAQABAkAcuYaFPO9bwfvABVZp4LK6CYeNJwqKavjKE/jHETx3M/cot1rM\r\nA39OZ/wCuNrqr+k4Pb2qQy2rzXJoa37lMG7ZAiEA+itu8WdfuzdOUAfkoHgsjUdz\r\nBmfaWy37q58A0druQEMCIQDJnLmp1sTDIcqoIMlIXDNwTPUc4pJj/JJSeczTQg7U\r\n5QIhAJzSu2JzNgzLd7ktqYF6tBsAbjfWxIgiBEEqlL346x+3AiB54WAwN2C94jjE\r\nSQXF089Y7X0kmCgNgAvpBi7367BRrQIgDH+nVOtPl0Y4Yqop/7hCT3vEsvmMWgx4\r\n9InegA0Cz68=\r\n-----END PRIVATE KEY-----";

String defaultIndex = "<!DOCTYPE html><title>hi</title><center>ESP</center>";

//Global variables
DNSServer dnsServer;
ESP8266WebServer webServer(80);
ESP8266WebServerSecure sWebServer(443);

String getContentType(String filename){
  if(filename.endsWith(".htm")) return "text/html";
  else if(filename.endsWith(".html")) return "text/html";
  else if(filename.endsWith(".css")) return "text/css";
  else if(filename.endsWith(".js")) return "application/javascript";
  else if(filename.endsWith(".png")) return "image/png";
  else if(filename.endsWith(".gif")) return "image/gif";
  else if(filename.endsWith(".jpg")) return "image/jpeg";
  else if(filename.endsWith(".ico")) return "image/x-icon";
  else if(filename.endsWith(".xml")) return "text/xml";
  else if(filename.endsWith(".pdf")) return "application/x-pdf";
  else if(filename.endsWith(".zip")) return "application/x-zip";
  else if(filename.endsWith(".gz")) return "application/x-gzip";
  return "text/plain";
}


void handleHtml() {

  if (loadFromSpiffs(webServer.uri())) {
    return;
  }
  
  webServer.send(404, "text/plain", "Not Found");

#ifdef DEBUG_WEBSERVER
  String message = "\n\n";
  message += "URI: ";
  message += webServer.uri();
  message += "\nMethod: ";
  message += (webServer.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += webServer.args();
  message += "\n";
  for (uint8_t i = 0; i < webServer.args(); i++) {
    message += " NAME:" + webServer.argName(i) + "\n VALUE:" + webServer.arg(i) + "\n";
  }  
  Serial.print(message);
#endif  
}


bool loadFromSpiffs(String path) {

  // PS4 User Guide url are all redirected to the root (index.html initially)
  if ((path.indexOf("/document/") >=0) && (path.indexOf("/ps4") >= 0))
  {
     path = path.substring(path.lastIndexOf("/"));
  }  

  if (path.equals("/")) {
    path += "index.html";
  }

  if (path.endsWith("index.html") && !SPIFFS.exists(path)) {
    webServer.setContentLength(defaultIndex.length());
    webServer.send(200, "text/html", defaultIndex);
    return true;
  }

  if (SPIFFS.exists(path)){
    String dataType = getContentType(path);
    File dataFile = SPIFFS.open(path, "r");
    if (!dataFile) {
      return false;
    }

    //Download file request from manager
    if (webServer.hasArg("dl")) {
      dataType = "application/octet-stream";
      String dlFile = path;
      if (dlFile.startsWith("/"))
      {
        dlFile = dlFile.substring(1);
      }
      webServer.sendHeader("Content-Disposition", "attachment; filename=\"" + dlFile + "\"");
      webServer.sendHeader("Content-Transfer-Encoding", "binary");
    }

    if (webServer.streamFile(dataFile, dataType) != dataFile.size()) {
      Serial.println("Sent less data than expected!");
    }
    dataFile.close();
    return true;
  }else{
    return false;
  }
}


void setup(void) 
{
  Serial.begin(115200);
  //Serial.setDebugOutput(true);


  SPIFFS.begin();

  //Load config values from SPIFFS to replace default ones
  conf->loadConfigFromStorage();
  
  //Serial.println("SSID: " + conf->AP_SSID);
  //Serial.println("Password: " + conf->AP_PASS);
  //Serial.println("WEB Server IP: " + conf->Server_IP.toString());


#ifdef CHRISTMAS_TREE
    pinMode(LED_BUILTIN, OUTPUT);     // Initialize the LED_BUILTIN pin as an output
    digitalWrite(LED_BUILTIN, HIGH); //set to off initially
#endif   


  //Connect to Wifi Router if needed
  if (conf->connectWifi)
  {
    WiFi.mode(WIFI_AP_STA);
    WiFi.setAutoConnect(true); 
    WiFi.setAutoReconnect(true);
    WiFi.begin(conf->WIFI_SSID, conf->WIFI_PASS);

    //Serial.println("WIFI connecting");
    if (WiFi.waitForConnectResult() != WL_CONNECTED) {
      Serial.println("Wifi failed to connect");
    } else {
       Serial.println("Connected to " + conf->WIFI_SSID);
    }
  } 
  else 
  {
    Serial.println("Offline mode");
    WiFi.mode(WIFI_AP);
  }

  //Set up Wifi Access Point
  WiFi.softAPConfig(conf->Server_IP, conf->Server_IP, conf->Subnet_Mask); 
  WiFi.softAP(conf->AP_SSID.c_str(), conf->AP_PASS.c_str());
  Serial.println("WIFI AP started");


  //Start DNS Server
  dnsServer.setTTL(30);
  dnsServer.setErrorReplyCode(DNSReplyCode::ServerFailure);
  dnsServer.start(53, "*", conf->Server_IP);
  Serial.println("DNS server started");


  //HTTP Web Server Setup
  webServer.onNotFound(handleHtml);
  //Add handles for internal Admin pages
  WebAdmin webAdmin (&webServer);

  //HTTPS Web Server Set up: only thing it does is instantly redirect the browser to the HTTP index page
  sWebServer.getServer().setRSACert(new X509List(serverCert), new PrivateKey(serverKey));
  sWebServer.onNotFound([]() {
    sWebServer.sendHeader("Location", String("http://" + conf->Server_IP.toString() + "/index.html" ), true);
    sWebServer.send(301, "text/plain", "");
  });


  //NAT Routing - this enables Internet access for clients (The ESP8266 acts as a Wifi Repeater)
  if (conf->connectWifi)
  {
    Serial.printf("Heap before: %d\n", ESP.getFreeHeap());
    err_t ret = ip_napt_init(NAPT, NAPT_PORT);
    //Serial.printf("ip_napt_init(%d,%d): ret=%d (OK=%d)\n", NAPT, NAPT_PORT, (int)ret, (int)ERR_OK);
    if (ret == ERR_OK)
    {
      ret = ip_napt_enable_no(SOFTAP_IF, 1);
      //Serial.printf("ip_napt_enable_no(SOFTAP_IF): ret=%d (OK=%d)\n", (int)ret, (int)ERR_OK);
      if (ret == ERR_OK)
      {
        Serial.println("NAT init OK");
      }
    }
    Serial.printf("Heap after napt init: %d\n", ESP.getFreeHeap());
    if (ret != ERR_OK)
    {
      Serial.println("NAPT initialization failed");
    }
  }

  //Start HTTP and HTTPS Web Servers
  sWebServer.begin();
  webServer.begin();
  Serial.println("HTTP servers started");

}

//DNS Blocking and redirecting to local server
void dnsProcess () {
  int dnsOK = dnsServer.processNextRequest();

  //Note: we skip this whole section if we're not connected to the Wifi Router, as DNS would simply not work anyway
  if (!conf->connectWifi) 
  {
    dnsServer.replyWithIP(conf->Server_IP);
    return;
  }
  if ((dnsOK == 0))
  {
    String dom = dnsServer.getQueryDomainName();
      
    if ((dom != ""))
    {

#ifdef DEBUG_DNS    
      Serial.printf("Domain: %s", dom.c_str());
#endif


      //check for domains we want to redirect to the ESP
      for (const char* element : SpoofedDomains)
      { 
        if (dom.indexOf(element) >= 0) 
        { 
#ifdef DEBUG_DNS    
          Serial.printf(" redirected to local server \n");
#endif                      

#ifdef CHRISTMAS_TREE
  digitalWrite(LED_BUILTIN, LOW);   // Turn the LED on
#endif               
          dnsServer.replyWithIP(conf->Server_IP);
          return;
        }
      }


      //Check for Domains we want to block
      for (const char* element : BlockedDomains)
      { 
        if (dom.indexOf(element) >= 0) 
        {  
                
#ifdef DEBUG_DNS    
          Serial.printf(" Blocked \n");
#endif    

#ifdef CHRISTMAS_TREE
  digitalWrite(LED_BUILTIN, LOW);   // Turn the LED on
#endif 
          dnsServer.replyWithIP(IPAddress(0, 0, 0, 0));
          return;
        }
      }

      //Default case, pass through
      IPAddress ip;
      WiFi.hostByName(dom.c_str(), ip);
      dnsServer.replyWithIP(ip); 
      
#ifdef DEBUG_DNS    
      Serial.println(" | IP:" + ip.toString());
#endif             
   
    }
  } 
}

void loop(void) {
  dnsProcess();
  webServer.handleClient();
  sWebServer.handleClient();


#ifdef DEBUG_HEAP  
  //occasionally display free Heap
  int freeHeap = ESP.getFreeHeap();
  if (((freeHeap - lastHeap) > 2000) || ((freeHeap - lastHeap) < -2000)) {
    Serial.printf("Heap available: %d\n", freeHeap);
    lastHeap = freeHeap;
  }
#endif  

#ifdef CHRISTMAS_TREE
  //Code to blink the built-in led when accessing specific urls in DNS Server
  int currentTime = millis();
  if ((currentTime - lastTime) > 300) {
    lastTime = currentTime;
    digitalWrite(LED_BUILTIN, HIGH);  // Turn the LED off by making the voltage HIGH
  }
#endif
}
