#ifndef   STORAGE_H
#define   STORAGE_H

#include <stdint.h>
#include <EEPROM.h>

class Storage {
    public:
      void saveLevels(uint16_t * levels, char channels);
      void readLevels(uint16_t * levels);
      void setup();
};

#endif