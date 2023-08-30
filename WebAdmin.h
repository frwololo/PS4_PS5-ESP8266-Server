#ifndef WEBADMIN_H
#define WEBADMIN_H

//Extended WebAdmin gives access to much more options in the WebAdmin page: File Management, etc...
//But it uses too much heap so I deactivated most of the functionality for now.
//It is mostly untested, made of scavenged parts from Stooged's Server
//TODO: move html to actual files on SPIFF?
//     Compress?
//     Use char* instead of String objects?
//#define EXTENDED_WEBADMIN  

#include <ESP8266WebServer.h>

class WebAdmin {
  
  public:
  WebAdmin(ESP8266WebServer *_webserver);

  private:
  static ESP8266WebServer *webServer;
  static File upFile;


  static void handleConfig();
  static String html_header(const char* title, const char *js = "");
  static String genConfigHtmlInput(String text, String id, String value);
  static void   handleConfigHtml();  
  static void handleRebootMsg(const char * message);

#ifdef EXTENDED_WEBADMIN
  static String formatBytes(size_t bytes);
  static void handleFileUpload();
 static  void handleFormat();
  static void handleDelete();
  static void handleFileMan();
  static void   handleUploadHtml();
  static void   handleFormatHtml();
  static void   handleAdminHtml();
  static void handleRebootHtml();

  static void handleReboot();
  static void handleInfo();
#endif
  
};


#endif
