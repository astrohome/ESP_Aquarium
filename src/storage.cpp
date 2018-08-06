#include "storage.h"

void Storage::saveLevels(uint16_t * levels, char channels) {
    for (int i = 0; i < channels; i++) {
        uint16_t level = levels[i];
        uint8_t low = (uint8_t) level;
        uint8_t high = (uint8_t)(level >> 8);
        EEPROM.write(i * 2, low);
        EEPROM.write(i * 2 + 1, high);
    }
    EEPROM.commit();
}

void Storage::setup() {
    EEPROM.begin(512);
}

void Storage::readLevels(uint16_t * levels)
{
    for (int i = 0; i < 4; i++) {
        uint8_t high = EEPROM.read(i * 2 + 1);
        uint8_t low = EEPROM.read(i * 2);
        uint16_t level = (high << 8) | low;
        levels[i] = level;
    }
}