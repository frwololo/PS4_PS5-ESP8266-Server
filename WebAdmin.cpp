#include "WebAdmin.h"
#include "GlobalConfig.h"

ESPWebServer * WebAdmin::webServer = NULL;
File WebAdmin::upFile;

const static char COMMON_CSS[] PROGMEM = "#loader{z-index:1;width:50px;height:50px;margin:0;border:6px solid #f3f3f3;border-radius:50%;border-top:6px solid #3498db;width:50px;height:50px;-webkit-animation:spin 2s linear infinite;animation:spin 2s linear infinite}@-webkit-keyframes spin{0%{-webkit-transform:rotate(0deg)}100%{-webkit-transform:rotate(360deg)}}@keyframes spin{0%{transform:rotate(0deg)}100%{transform:rotate(360deg)}}body{background-color:#1451ae;color:#fff;font-size:20px;font-weight:700;margin:0 0 0 .0;padding:.4em .4em .4em .6em}";

void WebAdmin::handleConfig()
{

  const char* config_params[] = {"ap_ssid", "ap_pass", "ap_ip", "ap_subnet", "enable_wifi", "wifi_ssid", "wifi_pass"};
  if(webServer->hasArg("ap_ssid")) 
  { 
    File iniFile = SPIFFS.open(CONFIG_FILE, "w");
    if (iniFile) {
      for (const char* element : config_params)
      { 
        String tmp = webServer->arg(element);
        iniFile.print(String(element) + "=" + tmp + "\r\n");
      }
      iniFile.close();
    }
    handleRebootMsg("Config Saved");
  }
  else
  {
  webServer->sendHeader("Location","/config.html");
  webServer->send(302, "text/html", "");
  }
}


void WebAdmin::handleRebootMsg(const char * message)
{
  //Serial.print("Rebooting ESP");
  static const char htmStr[] PROGMEM = "<!DOCTYPE html><html><head><meta http-equiv=\"refresh\" content=\"8; url=/info.html\"><style type=\"text/css\">";
  static const char htmStrBody_tmpl[] PROGMEM = "#status{font-size:16px;font-weight:400}</style></head><center><br><br><br><p id=\"status\"><div id='loader'></div><br>_MSG_<br>Rebooting</p></center></html>";
  String htmStrBody = FPSTR(htmStrBody_tmpl);
  htmStrBody.replace("_MSG_", message);
  webServer->setContentLength(strlen_P(htmStr) + strlen_P(COMMON_CSS) + htmStrBody.length());
  webServer->send(200, "text/html", "");
  webServer->sendContent_P(htmStr);
  webServer->sendContent_P(COMMON_CSS);
  webServer->sendContent(htmStrBody);
  delay(1000);
  ESP.restart();
}

String WebAdmin::html_header(const char* title, const char *js_progmem_ptr ) {
  static const char htmStr_tmpl[] PROGMEM = "<!DOCTYPE html><html><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\"><title>_TIT_</title><style type=\"text/css\">_CSS_</style>_JS_</head>";
  String result = FPSTR(htmStr_tmpl); 
 
  String css =  FPSTR(COMMON_CSS); 
  static const char additional_css[] PROGMEM =".sidenav{width:140px;position:fixed;z-index:1;top:20px;left:10px;background:#6495ed;overflow-x:hidden;padding:8px 0}.sidenav a{padding:6px 8px 6px 16px;text-decoration:none;font-size:14px;color:#fff;display:block}.sidenav a:hover{color:#1451ae}.main{margin-left:150px;padding:10px;position:absolute;top:0;right:0;bottom:0;left:0}input[type=submit]:hover{background:#fff;color:green}input[type=submit]:active{outline-color:green;color:green;background:#fff}input[type=button]:hover{background:#fff;color:#000}input[type=button]:active{outline-color:#000;color:#000;background:#fff}#selfile{font-size:16px;font-weight:400}#status{font-size:16px;font-weight:400}";
  css = css + FPSTR(additional_css);
  
  String js = FPSTR(js_progmem_ptr);

  result.replace("_TIT_", title);
  result.replace("_CSS_", css);
  result.replace("_JS_", js);
  return result;
}

String WebAdmin::genConfigHtmlInput(String text, String id, String value){
  static const char htmStr_tmpl[] PROGMEM = "<tr><td>_TXT_:</td><td><input name=\"_ID_\" value=\"_VAL_\"></td></tr>";
  String result = FPSTR(htmStr_tmpl);
  result.replace("_TXT_", text);
  result.replace("_ID_", id);
  result.replace("_VAL_", value);
  return result;
}

void WebAdmin::handleConfigHtml()
{

  GlobalConfig * conf = GlobalConfig::GetConfig();
  static const char js[] PROGMEM ="";
  static const char open_body[] PROGMEM = "<body><form action=\"/config.html\" method=\"post\"><center><table>";
  static const char close_body[] PROGMEM = "</table><br><input id=\"savecfg\" type=\"submit\" value=\"Save Config\"></center></form></body></html>";
  
  String htmStr = html_header("Config Editor", js) + FPSTR(open_body);
  htmStr = htmStr + genConfigHtmlInput("SSID", "ap_ssid",conf->AP_SSID); 
  htmStr = htmStr + genConfigHtmlInput("PASSWORD", "ap_pass",conf->AP_PASS);
  htmStr = htmStr + genConfigHtmlInput("WEBSERVER", "ap_ip",conf->Server_IP.toString());
  htmStr = htmStr + genConfigHtmlInput("SUBNET MASK", "ap_subnet",conf->Subnet_Mask.toString());
  htmStr = htmStr + genConfigHtmlInput("ENABLE WIFI", "enable_wifi",String(conf->connectWifi));
  htmStr = htmStr + genConfigHtmlInput("WIFI SSID", "wifi_ssid",conf->WIFI_SSID);
  htmStr = htmStr + genConfigHtmlInput("WIFI PASSWORD", "wifi_pass",conf->WIFI_PASS);
  htmStr = htmStr + FPSTR(close_body);
  webServer->setContentLength(htmStr.length());
  webServer->send(200, "text/html", htmStr);
}




WebAdmin::WebAdmin(ESPWebServer *_webserver) {

  webServer = _webserver;

  webServer->on("/config.html", HTTP_GET, handleConfigHtml);
  webServer->on("/config.html", HTTP_POST, handleConfig);

   webServer->on("/upload.html", HTTP_POST, []() {
    webServer->sendHeader("Location","/fileman.html");
    webServer->send(302, "text/html", "");
  }, handleFileUpload);
  webServer->on("/upload.html", HTTP_GET, handleUploadHtml);
  webServer->on("/format.html", HTTP_GET, handleFormatHtml);
  webServer->on("/format.html", HTTP_POST, handleFormat);
  webServer->on("/fileman.html", HTTP_GET, handleFileMan);
  webServer->on("/delete", HTTP_POST, handleDelete);

  webServer->on("/admin.html", HTTP_GET, handleAdminHtml);
  webServer->on("/reboot.html", HTTP_GET, handleRebootHtml);
  webServer->on("/reboot.html", HTTP_POST, handleReboot);
#ifdef EXTENDED_WEBADMIN  
  webServer->on("/info.html", HTTP_GET, handleInfo);  
#else
  //in basic mode, non existing paths lead to config file
  webServer->on("/info.html", HTTP_GET, handleConfigHtml);
#endif  

}

String WebAdmin::formatBytes(size_t bytes){
  if (bytes < 1024){
    return String(bytes)+" B";
  } else if(bytes < (1024 * 1024)){
    return String(bytes/1024.0)+" KB";
  } else if(bytes < (1024 * 1024 * 1024)){
    return String(bytes/1024.0/1024.0)+" MB";
  } else {
    return String(bytes/1024.0/1024.0/1024.0)+" GB";
  }
}


void WebAdmin::handleFileUpload() {
  if (webServer->uri() != "/upload.html") {
    webServer->send(500, "text/plain", "Internal Server Error");
    return;
  }
  HTTPUpload& upload = webServer->upload();
  if (upload.status == UPLOAD_FILE_START) {
    String filename = upload.filename;
    if (!filename.startsWith("/")) {
      filename = "/" + filename;
    }
    if (filename.equals(CONFIG_FILE))
    {return;}
    WebAdmin::upFile = SPIFFS.open(filename, "w");
    filename = String();
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (WebAdmin::upFile) {
      WebAdmin::upFile.write(upload.buf, upload.currentSize);
    }
  } else if (upload.status == UPLOAD_FILE_END) {
    if (WebAdmin::upFile) {
      WebAdmin::upFile.close();
    }
  }
}

void WebAdmin::handleFormat()
{
  //Serial.print("Formatting SPIFFS");
  SPIFFS.end();
  SPIFFS.format();
  SPIFFS.begin();

  GlobalConfig * conf = GlobalConfig::GetConfig();
  conf->writeConfig();
  webServer->sendHeader("Location","/fileman.html");
  webServer->send(302, "text/html", "");
}


void WebAdmin::handleDelete(){
  if(!webServer->hasArg("file")) 
  {
    webServer->sendHeader("Location","/fileman.html");
    webServer->send(302, "text/html", "");
    return;
  }
 String path = webServer->arg("file");
 if (SPIFFS.exists("/" + path) && path != "/" && !path.equals("config.ini")) {
    SPIFFS.remove("/" + path);
 }
   webServer->sendHeader("Location","/fileman.html");
   webServer->send(302, "text/html", "");
}


void WebAdmin::handleFileMan() {
#ifdef ESP32
  File dir = SPIFFS.open("/");
#else
  Dir dir = SPIFFS.openDir("/");
#endif
  
  const static char header_html[] PROGMEM =  "<!DOCTYPE html><html><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\"><title>File Manager</title><style type=\"text/css\">a:link {color: #ffffff; text-decoration: none;} a:visited {color: #ffffff; text-decoration: none;} a:hover {color: #ffffff; text-decoration: underline;} a:active {color: #ffffff; text-decoration: underline;} table {font-family: arial, sans-serif; border-collapse: collapse; width: 100%;} td, th {border: 1px solid #dddddd; text-align: left; padding: 8px;} button {display: inline-block; padding: 1px; margin-right: 6px; vertical-align: top; float:left;} body {background-color: #1451AE;color: #ffffff; font-size: 14px; padding: 0.4em 0.4em 0.4em 0.6em; margin: 0 0 0 0.0;}</style><script>function statusDel(fname) {var answer = confirm(\"Are you sure you want to delete \" + fname + \" ?\");if (answer) {return true;} else { return false; }}</script></head><body><br><table>"; 
  const static char template_html[] PROGMEM = "<tr><td><a href=\"_FNAME_\">_FNAME_</a></td><td>_SIZE_</td><td><form action=\"/_FNAME_?dl=1\" method=\"post\"><button type=\"submit\" name=\"file\" value=\"_FNAME_\">Download</button></form></td><td><form action=\"/delete\" method=\"post\"><button type=\"submit\" name=\"file\" value=\"_FNAME_\" onClick=\"return statusDel('_FNAME_');\">Delete</button></form></td></tr>";
  String output = "";

#ifdef ESP32
  File entry = dir.openNextFile();
  while(entry){
    String fname = String(entry.name());
#else
  while(dir.next()){
    File entry = dir.openFile("r");
    String fname = String(entry.name()).substring(1);
#endif
    
    if (fname.length() > 0 && !fname.equals("config.ini"))
    {
      String tmpl = FPSTR(template_html);
      tmpl.replace("_FNAME_", fname);
      tmpl.replace("_SIZE_", formatBytes(entry.size()));
      output += tmpl;
    }
#ifdef ESP32
  entry = dir.openNextFile();
#else
    entry.close();
#endif    
  }
#ifdef ESP32
      dir.close();
#endif  
  output += "</table></body></html>";
  webServer->setContentLength( strlen_P(header_html) + output.length());
  webServer->send(200, "text/html", "");
  webServer->sendContent_P(header_html);
  webServer->sendContent(output);
}


void WebAdmin::handleReboot()
{
  return handleRebootMsg("");
}


void WebAdmin::handleUploadHtml()
{
  const static char js[] PROGMEM = "<script>function formatBytes(bytes) {  if(bytes == 0) return '0 Bytes';  var k = 1024,  dm = 2,  sizes = ['Bytes', 'KB', 'MB', 'GB', 'TB', 'PB', 'EB', 'ZB', 'YB'],  i = Math.floor(Math.log(bytes) / Math.log(k));  return parseFloat((bytes / Math.pow(k, i)).toFixed(dm)) + ' ' + sizes[i];}function statusUpl() {  document.getElementById(\"upload\").style.display=\"none\";  document.getElementById(\"btnsel\").style.display=\"none\";  document.getElementById(\"status\").innerHTML = \"<div id='loader'></div><br>Uploading files\";}function FileSelected(e){  var strdisp = \"\";  var file = document.getElementById(\"upfiles\").files;  for (var i = 0; i < file.length; i++)  {   strdisp = strdisp + file[i].name + \" - \" + formatBytes(file[i].size) + \"<br>\";  }  document.getElementById(\"selfile\").innerHTML = strdisp;  document.getElementById(\"upload\").style.display=\"block\";}</script>";
  const static char htmlBody[] PROGMEM = "<body><center><form action=\"/upload.html\" enctype=\"multipart/form-data\" method=\"post\"><p>File Uploader<br><br></p><p><input id=\"btnsel\" type=\"button\" onclick=\"document.getElementById('upfiles').click()\" value=\"Select files\" style=\"display: block;\"><p id=\"selfile\"></p><input id=\"upfiles\" type=\"file\" name=\"fwupdate\" size=\"0\" onChange=\"FileSelected();\" style=\"width:0; height:0;\" multiple></p><div><p id=\"status\"></p><input id=\"upload\" type=\"submit\" value=\"Upload Files\" onClick=\"statusUpl();\" style=\"display: none;\"></div></form><center></body></html>";
  String htmStr = html_header("File Upload", js);
  htmStr = htmStr + FPSTR(htmlBody);
  webServer->setContentLength(htmStr.length());
  webServer->send(200, "text/html", htmStr);
}


void WebAdmin::handleFormatHtml()
{

  const static char js[] PROGMEM = "<script>function statusFmt() { var answer = confirm(\"Are you sure you want to format?\");  if (answer) {   document.getElementById(\"format\").style.display=\"none\";   document.getElementById(\"status\").innerHTML = \"<div id='loader'></div><br>Formatting Storage\";   return true;  }  else {   return false;  }}</script>";
  const static char htmlBody[] PROGMEM = "<body><center><form action=\"/format.html\" method=\"post\"><p>Storage Format<br><br></p><p id=\"msgfmt\">This will delete all the files on the server</p><div><p id=\"status\"></p><input id=\"format\" type=\"submit\" value=\"Format Storage\" onClick=\"return statusFmt();\" style=\"display: block;\"></div></form><center></body></html>";
  String htmStr = html_header("Format Disk", js);
  htmStr = htmStr + FPSTR(htmlBody);
  webServer->setContentLength(htmStr.length());
  webServer->send(200, "text/html", htmStr);
}


void WebAdmin::handleAdminHtml()
{
  const static  char js[]  PROGMEM = "";
  String htmStr = html_header("Admin Panel", js);
#ifdef EXTENDED_WEBADMIN
  const static char body[] PROGMEM = "<body><div class=\"sidenav\"><a href=\"/index.html\" target=\"mframe\">Main Page</a><a href=\"/info.html\" target=\"mframe\">ESP Information</a><a href=\"/fileman.html\" target=\"mframe\">File Manager</a><a href=\"/upload.html\" target=\"mframe\">File Uploader</a><a href=\"/config.html\" target=\"mframe\">Config Editor</a><a href=\"/format.html\" target=\"mframe\">Storage Format</a><a href=\"/reboot.html\" target=\"mframe\">Reboot ESP</a></div><div class=\"main\"><iframe src=\"info.html\" name=\"mframe\" height=\"100%\" width=\"100%\" frameborder=\"0\"></iframe></div></table></body></html> ";
#else
  const static char body[] PROGMEM = "<body><div class=\"sidenav\"><a href=\"/index.html\" target=\"mframe\">Main Page</a><a href=\"/fileman.html\" target=\"mframe\">File Manager</a><a href=\"/upload.html\" target=\"mframe\">File Uploader</a><a href=\"/config.html\" target=\"mframe\">Config Editor</a><a href=\"/format.html\" target=\"mframe\">Storage Format</a><a href=\"/reboot.html\" target=\"mframe\">Reboot ESP</a></div><div class=\"main\"><iframe src=\"info.html\" name=\"mframe\" height=\"100%\" width=\"100%\" frameborder=\"0\"></iframe></div></table></body></html> ";
#endif
  
  htmStr = htmStr + FPSTR(body);
  webServer->setContentLength(htmStr.length());
  webServer->send(200, "text/html", htmStr);
}

void WebAdmin::handleRebootHtml()
{
  const static char js[] PROGMEM ="<script>function statusRbt() { var answer = confirm(\"Are you sure you want to reboot?\");  if (answer) {document.getElementById(\"reboot\").style.display=\"none\";   document.getElementById(\"status\").innerHTML = \"<div id='loader'></div><br>Rebooting ESP Board\"; return true;  }else {   return false;  }}</script>";
  const static char htmlBody[] PROGMEM ="<body><center><form action=\"/reboot.html\" method=\"post\"><p>ESP Reboot<br><br></p><p id=\"msgrbt\">This will reboot the esp board</p><div><p id=\"status\"></p><input id=\"reboot\" type=\"submit\" value=\"Reboot ESP\" onClick=\"return statusRbt();\" style=\"display: block;\"></div></form><center></body></html>";
  String htmStr = html_header("ESP Reboot", js);
  htmStr = htmStr + FPSTR(htmlBody);
  webServer->setContentLength(htmStr.length());
  webServer->send(200, "text/html", htmStr);
}

//Stuff below this line doesn't really work: USes too much heap, which prevents https webserver from working

#ifdef EXTENDED_WEBADMIN

void WebAdmin::handleInfo()
{
  const static char js[] PROGMEM ="";
  FSInfo fs_info;
  SPIFFS.info(fs_info);
  float flashFreq = (float)ESP.getFlashChipSpeed() / 1000.0 / 1000.0;
  FlashMode_t ideMode = ESP.getFlashChipMode();
  float supplyVoltage = (float)ESP.getVcc()/ 1000.0 ;
  String output = html_header("System Information", js);
  output += "<hr>###### Software ######<br><br>";
  output += "Core version: " + String(ESP.getCoreVersion()) + "<br>";
  output += "SDK version: " + String(ESP.getSdkVersion()) + "<br>";
  output += "Boot version: " + String(ESP.getBootVersion()) + "<br>";
  output += "Boot mode: " + String(ESP.getBootMode()) + "<br>";
  output += "Chip Id: " + String(ESP.getChipId()) + "<br><hr>";
  output += "###### CPU ######<br><br>";
  output += "CPU frequency: " + String(ESP.getCpuFreqMHz()) + "MHz<br><hr>";
  output += "###### Flash chip information ######<br><br>";
  output += "Flash chip Id: " +  String(ESP.getFlashChipId()) + "<br>";
  output += "Estimated Flash size: " + formatBytes(ESP.getFlashChipSize()) + "<br>";
  output += "Actual Flash size based on chip Id: " + formatBytes(ESP.getFlashChipRealSize()) + "<br>";
  output += "Flash frequency: " + String(flashFreq) + " MHz<br>";
  output += "Flash write mode: " + String((ideMode == FM_QIO ? "QIO" : ideMode == FM_QOUT ? "QOUT" : ideMode == FM_DIO ? "DIO" : ideMode == FM_DOUT ? "DOUT" : "UNKNOWN")) + "<br><hr>";
  output += "###### File system (SPIFFS) ######<br><br>"; 
  output += "Total space: " + formatBytes(fs_info.totalBytes) + "<br>";
  output += "Used space: " + formatBytes(fs_info.usedBytes) + "<br>";
  output += "Block size: " + String(fs_info.blockSize) + "<br>";
  output += "Page size: " + String(fs_info.pageSize) + "<br>";
  output += "Maximum open files: " + String(fs_info.maxOpenFiles) + "<br>";
  output += "Maximum path length: " +  String(fs_info.maxPathLength) + "<br><hr>";
  output += "###### Sketch information ######<br><br>";
  output += "Sketch hash: " + ESP.getSketchMD5() + "<br>";
  output += "Sketch size: " +  formatBytes(ESP.getSketchSize()) + "<br>";
  output += "Free space available: " +  formatBytes(ESP.getFreeSketchSpace()) + "<br><hr>";
  output += "###### power ######<br><br>";
  output += "Supply voltage: " +  String(supplyVoltage) + " V<br><hr>";
  output += "</html>";
  webServer->setContentLength(output.length());
  webServer->send(200, "text/html", output);
}


#endif  //EXTENDED_WEBADMIN
