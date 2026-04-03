#ifndef STATUS_LED_H
#define STATUS_LED_H

#include <Arduino.h>

enum class LEDPattern : uint8_t {
    OFF = 0,
    SOLID_RED,
    SOLID_GREEN,
    SOLID_BLUE,
    SOLID_AMBER,
    FLASH_GREEN,          // 2 quick flashes then done
    FLASH_RED_SLOW,       // 1Hz blink (sustained)
    FLASH_RED_RAPID,      // 5Hz blink (sustained)
    FLASH_AMBER,          // 2Hz blink (sustained)
    FLASH_AMBER_DOUBLE,   // Double-flash then done
    BREATHE_RED,          // Slow pulse (sustained)
    BREATHE_BLUE,         // Slow pulse (sustained)
    RAINBOW_CYCLE,        // Cycle through colors then done
    FLASH_WHITE,          // Single bright flash then done
    SUCCESS_GREEN,        // Solid green 1s then done
};

class StatusLED {
public:
    StatusLED(uint8_t pinR, uint8_t pinG, uint8_t pinB,
              uint8_t chR, uint8_t chG, uint8_t chB);
    void begin();
    void loop();

    void setPattern(LEDPattern pattern);
    void off();
    LEDPattern getCurrentPattern() const { return _currentPattern; }
    bool isPatternComplete() const { return _patternComplete; }

private:
    uint8_t _pinR, _pinG, _pinB;
    uint8_t _chR, _chG, _chB;

    LEDPattern _currentPattern;
    unsigned long _patternStartTime;
    unsigned long _lastStepTime;
    uint8_t _stepIndex;
    bool _patternComplete;

    void setRGB(uint8_t r, uint8_t g, uint8_t b);
    void runPattern();
    uint8_t breathe(unsigned long elapsed, unsigned long periodMs);
};

#endif
