#include "StatusLED.h"

StatusLED::StatusLED(uint8_t pinR, uint8_t pinG, uint8_t pinB,
                     uint8_t chR, uint8_t chG, uint8_t chB)
    : _pinR(pinR), _pinG(pinG), _pinB(pinB),
      _chR(chR), _chG(chG), _chB(chB),
      _currentPattern(LEDPattern::OFF),
      _patternStartTime(0), _lastStepTime(0),
      _stepIndex(0), _patternComplete(true) {}

void StatusLED::begin() {
    ledcSetup(_chR, 5000, 8);
    ledcSetup(_chG, 5000, 8);
    ledcSetup(_chB, 5000, 8);
    ledcAttachPin(_pinR, _chR);
    ledcAttachPin(_pinG, _chG);
    ledcAttachPin(_pinB, _chB);
    setRGB(0, 0, 0);
}

void StatusLED::setRGB(uint8_t r, uint8_t g, uint8_t b) {
    ledcWrite(_chR, r);
    ledcWrite(_chG, g);
    ledcWrite(_chB, b);
}

void StatusLED::off() {
    _currentPattern = LEDPattern::OFF;
    _patternComplete = true;
    setRGB(0, 0, 0);
}

void StatusLED::setPattern(LEDPattern pattern) {
    _currentPattern = pattern;
    _patternStartTime = millis();
    _lastStepTime = millis();
    _stepIndex = 0;
    _patternComplete = (pattern == LEDPattern::OFF);
    // Render first frame immediately so static colors work without loop()
    runPattern();
}

uint8_t StatusLED::breathe(unsigned long elapsed, unsigned long periodMs) {
    // Triangle wave: ramp up then down
    unsigned long phase = elapsed % periodMs;
    unsigned long half = periodMs / 2;
    if (phase < half) {
        return (uint8_t)((phase * 255) / half);
    } else {
        return (uint8_t)(((periodMs - phase) * 255) / half);
    }
}

void StatusLED::loop() {
    if (_patternComplete && _currentPattern == LEDPattern::OFF) return;
    runPattern();
}

void StatusLED::runPattern() {
    unsigned long now = millis();
    unsigned long elapsed = now - _patternStartTime;
    unsigned long stepElapsed = now - _lastStepTime;

    switch (_currentPattern) {
        case LEDPattern::OFF:
            setRGB(0, 0, 0);
            _patternComplete = true;
            break;

        case LEDPattern::SOLID_RED:
            setRGB(255, 0, 0);
            break;

        case LEDPattern::SOLID_GREEN:
            setRGB(0, 255, 0);
            break;

        case LEDPattern::SOLID_BLUE:
            setRGB(0, 0, 255);
            break;

        case LEDPattern::SOLID_AMBER:
            setRGB(255, 140, 0);
            break;

        case LEDPattern::FLASH_GREEN: {
            // 2 quick flashes: ON 80ms, OFF 80ms, ON 80ms, OFF -> done
            const unsigned long t[] = {0, 80, 160, 240, 320};
            if (elapsed < t[1])      setRGB(0, 255, 0);
            else if (elapsed < t[2]) setRGB(0, 0, 0);
            else if (elapsed < t[3]) setRGB(0, 255, 0);
            else if (elapsed < t[4]) setRGB(0, 0, 0);
            else { setRGB(0, 0, 0); _patternComplete = true; }
            break;
        }

        case LEDPattern::FLASH_RED_SLOW:
            // 1Hz: 500ms on, 500ms off (sustained)
            setRGB((elapsed % 1000 < 500) ? 255 : 0, 0, 0);
            break;

        case LEDPattern::FLASH_RED_RAPID:
            // 5Hz: 100ms on, 100ms off (sustained)
            setRGB((elapsed % 200 < 100) ? 255 : 0, 0, 0);
            break;

        case LEDPattern::FLASH_AMBER:
            // 2Hz: 250ms on, 250ms off (sustained)
            if (elapsed % 500 < 250) setRGB(255, 140, 0);
            else setRGB(0, 0, 0);
            break;

        case LEDPattern::FLASH_AMBER_DOUBLE: {
            // Double flash: ON 80ms, OFF 80ms, ON 80ms, OFF 400ms -> done
            if (elapsed < 80)        setRGB(255, 140, 0);
            else if (elapsed < 160)  setRGB(0, 0, 0);
            else if (elapsed < 240)  setRGB(255, 140, 0);
            else if (elapsed < 640)  setRGB(0, 0, 0);
            else { setRGB(0, 0, 0); _patternComplete = true; }
            break;
        }

        case LEDPattern::BREATHE_RED: {
            uint8_t b = breathe(elapsed, 2000);
            setRGB(b, 0, 0);
            break;
        }

        case LEDPattern::BREATHE_BLUE: {
            uint8_t b = breathe(elapsed, 2000);
            setRGB(0, 0, b);
            break;
        }

        case LEDPattern::RAINBOW_CYCLE: {
            // Cycle: R -> G -> B -> White -> off, ~400ms each
            unsigned long phase = elapsed / 400;
            switch (phase) {
                case 0: setRGB(255, 0, 0); break;
                case 1: setRGB(0, 255, 0); break;
                case 2: setRGB(0, 0, 255); break;
                case 3: setRGB(255, 255, 255); break;
                default: setRGB(0, 0, 0); _patternComplete = true; break;
            }
            break;
        }

        case LEDPattern::FLASH_WHITE:
            if (elapsed < 150) setRGB(255, 255, 255);
            else { setRGB(0, 0, 0); _patternComplete = true; }
            break;

        case LEDPattern::SUCCESS_GREEN:
            if (elapsed < 1000) setRGB(0, 255, 0);
            else { setRGB(0, 0, 0); _patternComplete = true; }
            break;
    }
}
