#ifndef   WEB_H
#define   WEB_H

#include <ESP8266WiFi.h>          
#include <DNSServer.h>
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager
#include <Ticker.h>
#include <FS.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <WebSocketsServer.h>
#include <ArduinoOTA.h>
#include <time.h>                       // time() ctime()
#include <sys/time.h>                   // struct timeval
#include <coredecls.h>                  // settimeofday_cb()
#include "leds.h"
#include "storage.h"

using namespace std::placeholders;

class Web {
    public:
        Web(Led * _led, Storage * _storage, uint16_t * _levels);
        void setup();
        tm* getTime();
        void handle();
    private:
        void startSPIFFS();               // Start the SPIFFS and list all contents
        void startWebSocket();            // Start a WebSocket server
        void startMDNS();                 // Start the mDNS responder
        void startServer();               // Start a HTTP server with a file read handler and an upload handler
        void time_is_set(void);

        void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t lenght);

        void handleNotFound();
        void handleFileUpload();
        bool handleFileRead(String path);
        String formatBytes(size_t bytes);
        String getContentType(String filename);
        Led * led;
        Storage * storage;
        uint16_t * levels;
};

#endif