#include "StatusBuzzer.h"

StatusBuzzer::StatusBuzzer(uint8_t pin, uint8_t channel)
    : _pin(pin), _channel(channel), _muted(false),
      _currentPattern(BuzzerPattern::SILENT),
      _patternStartTime(0), _stepIndex(0), _stepStartTime(0),
      _patternComplete(true) {}

void StatusBuzzer::begin() {
    ledcSetup(_channel, 2000, 8);
    ledcAttachPin(_pin, _channel);
    ledcWriteTone(_channel, 0);
}

void StatusBuzzer::playTone(uint16_t freq) {
    if (_muted || freq == 0) {
        ledcWriteTone(_channel, 0);
    } else {
        ledcWriteTone(_channel, freq);
    }
}

void StatusBuzzer::stopTone() {
    ledcWriteTone(_channel, 0);
}

void StatusBuzzer::silence() {
    _currentPattern = BuzzerPattern::SILENT;
    _patternComplete = true;
    stopTone();
}

void StatusBuzzer::setPattern(BuzzerPattern pattern) {
    if (pattern == BuzzerPattern::SILENT) {
        silence();
        return;
    }
    _currentPattern = pattern;
    _patternStartTime = millis();
    _stepIndex = 0;
    _stepStartTime = millis();
    _patternComplete = false;
}

void StatusBuzzer::runSequence(const ToneStep* steps, uint8_t count) {
    if (_stepIndex >= count) {
        stopTone();
        _patternComplete = true;
        _currentPattern = BuzzerPattern::SILENT;
        return;
    }

    unsigned long now = millis();

    // Start current step
    if (now - _stepStartTime == 0 || _stepIndex == 0 && now == _patternStartTime) {
        playTone(steps[_stepIndex].frequency);
    }

    // Check if current step is done
    if (now - _stepStartTime >= steps[_stepIndex].durationMs) {
        _stepIndex++;
        _stepStartTime = now;
        if (_stepIndex < count) {
            playTone(steps[_stepIndex].frequency);
        } else {
            stopTone();
            _patternComplete = true;
            _currentPattern = BuzzerPattern::SILENT;
        }
    }
}

void StatusBuzzer::loop() {
    if (_patternComplete) return;

    // Tone sequences — frequencies validated in tools/hw_test
    static const ToneStep beepShort[] = {
        {1047, 100}, {0, 50}
    };
    static const ToneStep beepDouble[] = {
        {1047, 100}, {0, 80}, {1047, 100}, {0, 50}
    };
    static const ToneStep beepError[] = {
        {400, 300}, {0, 50}
    };
    static const ToneStep urgentBeeps[] = {
        {1000, 80}, {0, 40}, {1500, 80}, {0, 40},
        {1000, 80}, {0, 40}, {1500, 80}, {0, 40},
        {1000, 80}, {0, 40}, {1500, 80}, {0, 50}
    };
    static const ToneStep startupChime[] = {
        {1047, 150}, {0, 50}, {1319, 150}, {0, 50}, {1568, 300}, {0, 50}
    };
    static const ToneStep successJingle[] = {
        {1047, 120}, {0, 40}, {1319, 120}, {0, 40},
        {1568, 120}, {0, 40}, {2093, 250}, {0, 50}
    };

    switch (_currentPattern) {
        case BuzzerPattern::BEEP_SHORT:
            runSequence(beepShort, 2);
            break;
        case BuzzerPattern::BEEP_DOUBLE:
            runSequence(beepDouble, 4);
            break;
        case BuzzerPattern::BEEP_ERROR:
            runSequence(beepError, 2);
            break;
        case BuzzerPattern::URGENT_BEEPS:
            runSequence(urgentBeeps, 12);
            break;
        case BuzzerPattern::STARTUP_CHIME:
            runSequence(startupChime, 6);
            break;
        case BuzzerPattern::SUCCESS_JINGLE:
            runSequence(successJingle, 8);
            break;
        default:
            silence();
            break;
    }
}
