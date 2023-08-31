#ifndef WEBADMIN_H
#define WEBADMIN_H

//Extended WebAdmin gives access to optional menus in the WebAdmin pages, but uses additional heap
//#define EXTENDED_WEBADMIN  

#include <ESP8266WebServer.h>

class WebAdmin {
  
  public:
  WebAdmin(ESP8266WebServer *_webserver);

  private:
  static ESP8266WebServer *webServer;
  static File upFile;


  static void handleConfig();
  static String html_header(const char* title, const char *js_progmem_ptr);
  static String genConfigHtmlInput(String text, String id, String value);
  static void   handleConfigHtml();  
  static void handleRebootMsg(const char * message);

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
#ifdef EXTENDED_WEBADMIN  
  static void handleInfo();
#endif
  
};


#endif
