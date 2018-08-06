#ifndef   LED_H
#define   LED_H

#include <Adafruit_PWMServoDriver.h>

class Led {
    public:
       void setFull(uint16_t *levels);
       void setOff(uint16_t *levels);
       void setMoonlight(uint16_t *levels);
       void applyLevels(uint16_t *levels);
       void setAndApply(short channel, uint16_t level, uint16_t *levels);
       void setup();
    private:
       bool force = false;
       Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();
};

#endif