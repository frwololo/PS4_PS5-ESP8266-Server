// basic esp8266/ESP32 server for PS5/PS4 Exploits. Tested with PS4 5.05 and PS5 3.xx. Supports internet access as a Wifi Repeater (NAT) + FakeDNS
// Code by wololo, sources scavenged from:
//    https://github.com/stooged/PS5-Server
//    https://github.com/stooged/PS5-Server32
//    modified DNS Server from https://github.com/rubfi/esphole
//    NAT example https://github.com/AliBigdeli/Arduino-ESP8266-Repeater


#include <FS.h>
#include <SPI.h>

#if defined(ESP8266)
  #include <ESP8266WiFi.h>
  #include <ESP8266WebServer.h>
  using ESPWebServer = ESP8266WebServer;
  #include <ESP8266WebServerSecure.h>
  #include <lwip/napt.h>
#elif defined(ESP32)
  #include <WiFi.h>
  #include <WebServer.h>
  using ESPWebServer = WebServer;
  #include <SSLCert.hpp>
  #include <HTTPSServer.hpp>
  #include <HTTPRequest.hpp>
  #include <HTTPResponse.hpp>
  using namespace httpsserver;
  #include "lwip/lwip_napt.h"
  #include <SPIFFS.h>
#else
  #error "Supported devices are ESP8266 and ESP32"
#endif

#include "DNSServer.h" //Modified DNS Server - Replaces the default SDK one
#include "GlobalConfig.h" //Note: Config defaults are in that file
#include "WebAdmin.h" //Pages to manage the device from a web interface (10.1.1.1/admin.html)

GlobalConfig *conf = GlobalConfig::GetConfig();

//Note: SpoofedDomains and BlockedDomains are *not* regexps, we just do a dumb "string match". So technically, "playstation.net" is equivalent to "*playstation\.net*" regex
const char* SpoofedDomains[] = {"playstation.net"}; //Used only if conf->wifiConnect is true - all these domains will be redirected to the ESP webServer in DNS Queries
const char* BlockedDomains[] = {"playstation.com", "scea.com", "sonyentertainmentnetwork.com"}; //Used only if conf->wifiConnect is true - all these domains will be blocked in DNS queries (IP set to 0)

#if defined(ESP8266)
  // number of entries and number of ports in NAT table
  // Needs to be reasonably small as it uses the heap, which is also used by SSL. But big enough to actually handle queries
  // if https server fails SSL handshake, it might be out of memory. Try to reduce these numbers
  #define NAPT 100 //27 bytes per entry
  #define NAPT_PORT 30 //14 bytes per entry
#endif  

//Debug
#define DEBUG_HEAP //The ESP8266 has very little heap memory, and both NAT routing and SSL Handshake use it.
                   // In my tests, the https Server needs around 25'000 bytes free heap to succesfully load a page. 30'000 is better, 20'000 not enough in my tests
                   // Because of this, it is essential to move a lot of static content (e.g. Strings) to Flash rather than RAM (hence the heavy use of PROGMEM macro in WebAdmin.cpp in particular)
//#define DEBUG_DNS //To debug issues related to Fake DNS
//#define DEBUG_WEBSERVER //Debug messages related to the Web Servers

#ifdef ESP8266
  #define CHRISTMAS_TREE //Builtin LED lights up when DNS replies with a Spoofed or Blocked Domain
#endif

#ifdef DEBUG_HEAP
int lastHeap = 0;
#endif

#ifdef CHRISTMAS_TREE
int lastTime = millis();
#endif

#ifdef ESP8266
static const char serverCert[] PROGMEM = "-----BEGIN CERTIFICATE-----\r\nMIIBszCCAV2gAwIBAgIUa8NoeDqaT6pFP5PHokRpKMXrT6swDQYJKoZIhvcNAQEL\r\nBQAwLjELMAkGA1UEBhMCSlAxDjAMBgNVBAgMBVRva3lvMQ8wDQYDVQQKDAZXb2xv\r\nbG8wHhcNMjMwODMwMDE0NDIxWhcNMzQxMTE2MDE0NDIxWjAuMQswCQYDVQQGEwJK\r\nUDEOMAwGA1UECAwFVG9reW8xDzANBgNVBAoMBldvbG9sbzBcMA0GCSqGSIb3DQEB\r\nAQUAA0sAMEgCQQDFBUICgP5wFMcRCs8VRqrY3QZr72FtzvLtPTCMcgPsaBE60QJt\r\n+YAj+e2mAolbRPRk4FNNcrg5/XsURdfuAPfvAgMBAAGjUzBRMB0GA1UdDgQWBBSv\r\nywhrMAdpJLbVx4kF9hmuTpHlLjAfBgNVHSMEGDAWgBSvywhrMAdpJLbVx4kF9hmu\r\nTpHlLjAPBgNVHRMBAf8EBTADAQH/MA0GCSqGSIb3DQEBCwUAA0EAjH+jaG47Qngl\r\nPTGjPpFmJp+LK6YHPzhqrJrAN18WziGJiaeCsuWV7j8cgGgB0U0AOybcdYMrjMOR\r\n+u+B12nV2w==\r\n-----END CERTIFICATE-----";
static const char serverKey[]  PROGMEM = "-----BEGIN PRIVATE KEY-----\r\nMIIBVAIBADANBgkqhkiG9w0BAQEFAASCAT4wggE6AgEAAkEAxQVCAoD+cBTHEQrP\r\nFUaq2N0Ga+9hbc7y7T0wjHID7GgROtECbfmAI/ntpgKJW0T0ZOBTTXK4Of17FEXX\r\n7gD37wIDAQABAkAcuYaFPO9bwfvABVZp4LK6CYeNJwqKavjKE/jHETx3M/cot1rM\r\nA39OZ/wCuNrqr+k4Pb2qQy2rzXJoa37lMG7ZAiEA+itu8WdfuzdOUAfkoHgsjUdz\r\nBmfaWy37q58A0druQEMCIQDJnLmp1sTDIcqoIMlIXDNwTPUc4pJj/JJSeczTQg7U\r\n5QIhAJzSu2JzNgzLd7ktqYF6tBsAbjfWxIgiBEEqlL346x+3AiB54WAwN2C94jjE\r\nSQXF089Y7X0kmCgNgAvpBi7367BRrQIgDH+nVOtPl0Y4Yqop/7hCT3vEsvmMWgx4\r\n9InegA0Cz68=\r\n-----END PRIVATE KEY-----";
#endif
static const char defaultIndex[] PROGMEM = "<!DOCTYPE html><title>hi</title><center>ESP</center>";

//Global variables
DNSServer dnsServer;
ESPWebServer webServer(80);
#if defined(ESP8266)
ESP8266WebServerSecure sWebServer(443);
#else
HTTPSServer * secureServer;
#endif

String getContentType(String filename) {
  if (filename.endsWith(".htm")) return "text/html";
  else if (filename.endsWith(".html")) return "text/html";
  else if (filename.endsWith(".css")) return "text/css";
  else if (filename.endsWith(".js")) return "application/javascript";
  else if (filename.endsWith(".png")) return "image/png";
  else if (filename.endsWith(".gif")) return "image/gif";
  else if (filename.endsWith(".jpg")) return "image/jpeg";
  else if (filename.endsWith(".ico")) return "image/x-icon";
  else if (filename.endsWith(".xml")) return "text/xml";
  else if (filename.endsWith(".pdf")) return "application/x-pdf";
  else if (filename.endsWith(".zip")) return "application/x-zip";
  else if (filename.endsWith(".gz")) return "application/x-gzip";
  return "text/plain";
}

#ifdef ESP32
//Redirect all https requests to http index.html
void handleHTTPS(HTTPRequest *req, HTTPResponse *res)
{
  std::string serverHost = conf->Server_IP.toString().c_str();
  res->setStatusCode(301);
  res->setStatusText("Moved Permanently");
  res->setHeader("Location", "http://" + serverHost + "/index.html");
  res->println("Moved Permanently");
}
#endif

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
  if ((path.indexOf("/document/") >= 0) && (path.indexOf("/ps4") >= 0))
  {
    path = path.substring(path.lastIndexOf("/"));
  }

  if (path.equals("/")) {
    path += "index.html";
  }

  if (path.endsWith("index.html") && !SPIFFS.exists(path)) {
    webServer.setContentLength(strlen_P(defaultIndex));
    webServer.send_P(200, "text/html", defaultIndex);
    return true;
  }

  if (SPIFFS.exists(path)) {
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
      Serial.println(F("Sent less data than expected!"));
    }
    dataFile.close();
    return true;
  } else {
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
    WiFi.begin(conf->WIFI_SSID.c_str(), conf->WIFI_PASS.c_str());

    //Serial.println(F("WIFI connecting"));
    if (WiFi.waitForConnectResult() != WL_CONNECTED) {
      Serial.println(F("Wifi failed to connect"));
    } else {
      Serial.println("Connected to " + conf->WIFI_SSID);
    }
  }
  else
  {
    Serial.println(F("Offline mode"));
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
  Serial.println(F("DNS server started"));


  //HTTP Web Server Setup
  webServer.onNotFound(handleHtml);
  //Add handles for internal Admin pages
  WebAdmin webAdmin (&webServer);

  //HTTPS Web Server Set up: only thing it does is instantly redirect the browser to the HTTP index page

#ifdef ESP8266
  //ESP8266 ssl certificate
  char bufCert[1000];            // -------------------
  char bufKey[1000];             //PROGMEM shenanigans
  strcpy_P(bufCert, serverCert); //
  strcpy_P(bufKey, serverKey);   //--------------------
  sWebServer.getServer().setRSACert(new X509List(bufCert), new PrivateKey(bufKey));

  //ESP8266 Redirect https to http
  sWebServer.onNotFound([]() {
    sWebServer.sendHeader("Location", String("http://" + conf->Server_IP.toString() + "/index.html" ), true);
    sWebServer.send(301, "text/plain", "");
  });
#else
  SSLCert * cert;
  //ESP32 SSL Certificate
  if (SPIFFS.exists("/pk.pem") && SPIFFS.exists("/cert.der"))
  {
    uint8_t* cBuffer;
    uint8_t* kBuffer;
    unsigned int clen = 0;
    unsigned int klen = 0;
    File certFile = SPIFFS.open("/cert.der", "r");
    clen = certFile.size();
    cBuffer = (uint8_t*)malloc(clen);
    certFile.read(cBuffer, clen);
    certFile.close();
    File pkFile = SPIFFS.open("/pk.pem", "r");
    klen = pkFile.size();
    kBuffer = (uint8_t*)malloc(klen);
    pkFile.read(kBuffer, klen);
    pkFile.close();
    cert = new SSLCert(cBuffer, clen, kBuffer, klen);
  }else{
    cert = new SSLCert();
    String keyInf = "CN=" + conf->Server_IP.toString() + ",O=Esp32_Server,C=US";
    int createCertResult = createSelfSignedCert(*cert, KEYSIZE_1024, (std::string)keyInf.c_str(), "20190101000000", "20300101000000");
    if (createCertResult != 0) {
      Serial.printf("Certificate failed, Error Code = 0x%02X\n", createCertResult);
    }else{
      Serial.println("Certificate created");
      File pkFile = SPIFFS.open("/pk.pem", "w");
      pkFile.write( cert->getPKData(), cert->getPKLength());
      pkFile.close();
      File certFile = SPIFFS.open("/cert.der", "w");
      certFile.write(cert->getCertData(), cert->getCertLength());
      certFile.close();
    }
  }
  
  secureServer = new HTTPSServer(cert);
  //ESP32 Redirect https tp http
  ResourceNode *nhttps = new ResourceNode("", "ANY", &handleHTTPS);
  secureServer->setDefaultNode(nhttps);
#endif

  //NAT Routing - this enables Internet access for clients (The ESP8266/ESP32 acts as a Wifi Repeater)
  if (conf->connectWifi)
  {
    Serial.printf("Heap before: %d\n", ESP.getFreeHeap());
#ifdef ESP8266    
    err_t ret = ip_napt_init(NAPT, NAPT_PORT);
    if (ret == ERR_OK)
    {
      ret = ip_napt_enable_no(SOFTAP_IF, 1); 
    }   
#else
    #if !IP_NAPT
      #error "IP_NAPT must be defined" //Some ESP32 SDKs don't have NAPT support enabled, you might need a forked build. See: https://github.com/espressif/arduino-esp32/issues/6421
    #endif // !IP_NAPT
    ip_napt_enable(conf->Server_IP, 1);
    err_t ret = ERR_OK; 
#endif    

    if (ret == ERR_OK)
    {
      Serial.println(F("NAT init OK"));
    }else {
      Serial.println(F("NAPT initialization failed"));
    }
    Serial.printf("Heap after napt init: %d\n", ESP.getFreeHeap());
  }

  //Start HTTP and HTTPS Web Servers

#ifdef ESP32
  secureServer->start();
  if (secureServer->isRunning())
  {
    Serial.println("HTTPs Server started");
  }
#else
  sWebServer.begin();
#endif

  webServer.begin();
  Serial.println(F("HTTP servers started"));

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
          Serial.printf(F(" redirected to local server \n"));
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
          Serial.printf(F(" Blocked \n"));
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
#ifdef ESP32
  secureServer->loop();
#else
  sWebServer.handleClient();
#endif

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
