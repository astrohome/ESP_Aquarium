#include "leds.h"
#include "storage.h"
#include "web.h"

#define CHANNELS 4

Led led;
Storage storage;
uint16_t levels[4];
Web web(&led, &storage, levels);

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  
  led.setup();
  web.setup();

  storage.readLevels(levels);
  led.applyLevels(levels);
}

void loop() {
  web.handle();

  tm * t = web.getTime();

  static int lastv = 0;
  if (lastv != t->tm_sec) {
    lastv = t->tm_sec;
    
    if (t->tm_hour == 21 && t->tm_min == 30) {
      led.setMoonlight(levels);
      led.applyLevels(levels);
    }
    else if (t->tm_hour == 12 && t->tm_min == 30) {
      storage.readLevels(levels);
      led.applyLevels(levels);
    }
  }
}