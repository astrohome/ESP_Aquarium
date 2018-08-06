#include "web.h"

#define TZ              -7       // (utc+) TZ in hours
#define DST_MN          60      // use 60mn for summer time in some countries
#define TZ_MN           ((TZ)*60)
#define TZ_SEC          ((TZ)*3600)
#define DST_SEC         ((DST_MN)*60)

using namespace std::placeholders;

timeval cbtime;			// time set in callback
bool cbtime_set = false;
ESP8266WiFiMulti wifiMulti;        // Create an instance of the ESP8266WiFiMulti class, called 'wifiMulti'
ESP8266WebServer server(80);       // Create a webserver object that listens for HTTP request on port 80
WebSocketsServer wss(81);
const char* mdnsName = "aquarium"; // Domain name for the mDNS responder
File fsUploadFile;                 // a File variable to temporarily store the received file
time_t now;
Ticker ticker;

Web::Web(Led * _led, Storage * _storage, uint16_t * _levels) {
    this->led = _led;
    this->storage = _storage;
    this->levels = _levels;
}

tm* Web::getTime() {
    now = time(nullptr);
    return localtime(&now);
}

//gets called when WiFiManager enters configuration mode
static void configModeCallback(WiFiManager * wifiManager) {
  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());
  Serial.println(wifiManager->getConfigPortalSSID());
}

void Web::setup() {
    WiFiManager wifiManager;
    //reset settings - for testing
    //wifiManager.resetSettings();
    wifiManager.setAPCallback(configModeCallback);

    if (!wifiManager.autoConnect()) {
        Serial.println("failed to connect and hit timeout");
        //reset and try again, or maybe put it to deep sleep
        ESP.reset();
        delay(1000);
    }

    ArduinoOTA.onStart([]() {
        String type;
        if (ArduinoOTA.getCommand() == U_FLASH) {
        type = "sketch";
        } else { // U_SPIFFS
        type = "filesystem";
        }

        // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
        Serial.println("Start updating " + type);
    });
    ArduinoOTA.onEnd([]() {
        Serial.println("\nEnd");
    });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    });
    ArduinoOTA.onError([](ota_error_t error) {
        Serial.printf("Error[%u]: ", error);
        if (error == OTA_AUTH_ERROR) {
        Serial.println("Auth Failed");
        } else if (error == OTA_BEGIN_ERROR) {
        Serial.println("Begin Failed");
        } else if (error == OTA_CONNECT_ERROR) {
        Serial.println("Connect Failed");
        } else if (error == OTA_RECEIVE_ERROR) {
        Serial.println("Receive Failed");
        } else if (error == OTA_END_ERROR) {
        Serial.println("End Failed");
        }
    });
    ArduinoOTA.begin();

    configTime(TZ_SEC, DST_SEC, "pool.ntp.org", "time.nist.gov", "time.windows.com");

    startSPIFFS();               // Start the SPIFFS and list all contents
    startWebSocket();
    startMDNS();                 // Start the mDNS responder
    startServer();               // Start a HTTP server with a file read handler and an upload handler
}

void Web::startWebSocket() {
    wss.onEvent(std::bind(&Web::webSocketEvent, this, _1, _2, _3, _4));
}

void Web::startMDNS() { // Start the mDNS responder
  MDNS.begin(mdnsName);                        // start the multicast domain name server
  Serial.print("mDNS responder started: http://");
  Serial.print(mdnsName);
  Serial.println(".local");
}

void Web::startServer() { // Start a HTTP server with a file read handler and an upload handler
  server.on("/edit.html",  HTTP_POST, []() {  // If a POST request is sent to the /edit.html address,
    server.send(200, "text/plain", ""); 
  }, std::bind(&Web::handleFileUpload, this));                       // go to 'handleFileUpload'

  server.onNotFound(std::bind(&Web::handleNotFound, this));          // if someone requests any other file or page, go to function 'handleNotFound'
                                              // and check if the file exists

  server.begin();                             // start the HTTP server
  Serial.println("HTTP server started.");
}

void Web::startSPIFFS() { // Start the SPIFFS and list all contents
  SPIFFS.begin();                             // Start the SPI Flash File System (SPIFFS)
  Serial.println("SPIFFS started. Contents:");
  {
    Dir dir = SPIFFS.openDir("/");
    while (dir.next()) {                      // List the file system contents
      String fileName = dir.fileName();
      size_t fileSize = dir.fileSize();
      Serial.printf("\tFS File: %s, size: %s\r\n", fileName.c_str(), formatBytes(fileSize).c_str());
    }
    Serial.printf("\n");
  }
}

void Web::webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t lenght) { // When a WebSocket message is received
  switch (type) {
    case WStype_DISCONNECTED:             // if the websocket is disconnected
      Serial.printf("[%u] Disconnected!\n", num);
      break;
    case WStype_CONNECTED: {              // if a new websocket connection is established
        IPAddress ip = wss.remoteIP(num);
        Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);
        now = time(nullptr);
        tm* t = localtime(&now);
        wss.sendTXT(num, "{\"time\":{\"hour\":"+ String(t->tm_hour) + ",\"min\":" + String(t->tm_min) + "}}");
      }
      break;
    case WStype_TEXT:                     // if new text data is received
        Serial.printf("[%u] get Text: %s\n", num, payload);
        if (payload[0] == 'a' || payload[0] == 'b' || payload[0] == 'c' || payload[0] == 'd') {            // we get RGB data
            int channel = payload[0] - 'a';
            led->setAndApply(channel, (uint16_t) strtol((const char *) &payload[1], NULL, 10), levels);
            Serial.printf("Setting PWM %u: %u\n", channel, levels[channel]);
        }
        else if (payload[0] == 'S') {
            storage->saveLevels(levels, 4);
        }
        else if (payload[0] == 'L') {
            storage->readLevels(levels);
            String result = "[";
            for (int i = 0; i < 4; i++) {
            result += String(levels[i]);
            if (i != 3) result += ",";
            }
            result += "]";
            wss.sendTXT(num, result);
        }
        else if (payload[0] == 'M') {
            led->setMoonlight(levels);
            led->applyLevels(levels);
        }
        else if (payload[0] == '1') {
            storage->readLevels(levels);
            led->applyLevels(levels);
        } else if (payload[0] == '0') {
            led->setOff(levels);
            led->applyLevels(levels);
        }
        break;
  }
}

String Web::formatBytes(size_t bytes) { // convert sizes in bytes to KB and MB
  if (bytes < 1024) {
    return String(bytes) + "B";
  } else if (bytes < (1024 * 1024)) {
    return String(bytes / 1024.0) + "KB";
  } else if (bytes < (1024 * 1024 * 1024)) {
    return String(bytes / 1024.0 / 1024.0) + "MB";
  }
}

String Web::getContentType(String filename) { // determine the filetype of a given filename, based on the extension
  if (filename.endsWith(".html")) return "text/html";
  else if (filename.endsWith(".css")) return "text/css";
  else if (filename.endsWith(".js")) return "application/javascript";
  else if (filename.endsWith(".ico")) return "image/x-icon";
  else if (filename.endsWith(".gz")) return "application/x-gzip";
  else if (filename.endsWith(".json")) return "application/json";
  return "text/plain";
}

bool Web::handleFileRead(String path) { // send the right file to the client (if it exists)
  Serial.println("handleFileRead: " + path);
  if (path.endsWith("/")) path += "index.html";          // If a folder is requested, send the index file
  String contentType = getContentType(path);             // Get the MIME type
  String pathWithGz = path + ".gz";
  if (SPIFFS.exists(pathWithGz) || SPIFFS.exists(path)) { // If the file exists, either as a compressed archive, or normal
    if (SPIFFS.exists(pathWithGz))                         // If there's a compressed version available
      path += ".gz";                                         // Use the compressed verion
    File file = SPIFFS.open(path, "r");                    // Open the file
    size_t sent = server.streamFile(file, contentType);    // Send it to the client
    file.close();                                          // Close the file again
    Serial.println(String("\tSent file: ") + path);
    return true;
  }
  Serial.println(String("\tFile Not Found: ") + path);   // If the file doesn't exist, return false
  return false;
}

void Web::handleNotFound() { // if the requested file or page doesn't exist, return a 404 not found error
  if(!handleFileRead(server.uri())) {          // check if the file exists in the flash memory (SPIFFS), if so, send it
    server.send(404, "text/plain", "404: File Not Found");
  }
}

void Web::handleFileUpload() { // upload a new file to the SPIFFS
  HTTPUpload& upload = server.upload();
  String path;
  if(upload.status == UPLOAD_FILE_START){
    path = upload.filename;
    if(!path.startsWith("/")) path = "/"+path;
    if(!path.endsWith(".gz")) {                          // The file server always prefers a compressed version of a file 
      String pathWithGz = path+".gz";                    // So if an uploaded file is not compressed, the existing compressed
      if(SPIFFS.exists(pathWithGz))                      // version of that file must be deleted (if it exists)
         SPIFFS.remove(pathWithGz);
    }
    Serial.print("handleFileUpload Name: "); Serial.println(path);
    fsUploadFile = SPIFFS.open(path, "w");            // Open the file for writing in SPIFFS (create if it doesn't exist)
    path = String();
  } else if(upload.status == UPLOAD_FILE_WRITE){
    if(fsUploadFile)
      fsUploadFile.write(upload.buf, upload.currentSize); // Write the received bytes to the file
  } else if(upload.status == UPLOAD_FILE_END){
    if(fsUploadFile) {                                    // If the file was successfully created
      fsUploadFile.close();                               // Close the file again
      Serial.print("handleFileUpload Size: "); Serial.println(upload.totalSize);
      server.sendHeader("Location","/success.html");      // Redirect the client to the success page
      server.send(303);
    } else {
      server.send(500, "text/plain", "500: couldn't create file");
    }
  }
}

void Web::time_is_set(void) {
  gettimeofday(&cbtime, NULL);
  cbtime_set = true;
  Serial.println("------------------ settimeofday() was called ------------------");
}

void Web::handle() {
  wss.loop();
  server.handleClient();
  ArduinoOTA.handle();
}