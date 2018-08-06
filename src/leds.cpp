#include "leds.h"
#define LEDS_CHANNELS 4

void Led::setup() {
  pwm.begin();
  pwm.setPWMFreq(1600);  // This is the maximum PWM frequency
  Wire.setClock(400000); // quick mode
}

void Led::setFull(uint16_t *levels) {
  for (int i = 0; i < LEDS_CHANNELS; i++) {
    levels[i] = 4095;
  }
  force = true;
}

void Led::applyLevels(uint16_t *levels) {
  for (int i = 0; i < LEDS_CHANNELS; i++) {
    pwm.setPWM(i, 0, levels[i]);
  }
}

void Led::setOff(uint16_t *levels) {
  for (int i = 0; i < LEDS_CHANNELS; i++) {
      levels[i] = 0;
  }
  force = true;
}

void Led::setMoonlight(uint16_t *levels) {
  levels[0] = 10;
  for (int i = 1; i < LEDS_CHANNELS; i++) {
      levels[i] = 0;
  }
}

void Led::setAndApply(short channel, uint16_t level, uint16_t *levels) {
  levels[channel] = level;
  pwm.setPWM(channel, 0, level);
}