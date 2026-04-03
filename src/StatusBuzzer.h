#ifndef STATUS_BUZZER_H
#define STATUS_BUZZER_H

#include <Arduino.h>

enum class BuzzerPattern : uint8_t {
    SILENT = 0,
    BEEP_SHORT,       // Single 100ms beep
    BEEP_DOUBLE,      // Two 100ms beeps
    BEEP_ERROR,       // Low 400Hz 300ms
    URGENT_BEEPS,     // Rapid alternating tones
    STARTUP_CHIME,    // Ascending C5-E5-G5
    SUCCESS_JINGLE,   // Ascending C5-E5-G5-C6
};

class StatusBuzzer {
public:
    StatusBuzzer(uint8_t pin, uint8_t channel);
    void begin();
    void loop();

    void setPattern(BuzzerPattern pattern);
    void silence();
    void setMuted(bool muted) { _muted = muted; }
    bool isMuted() const { return _muted; }
    bool isPatternComplete() const { return _patternComplete; }

private:
    uint8_t _pin, _channel;
    bool _muted;

    BuzzerPattern _currentPattern;
    unsigned long _patternStartTime;
    uint8_t _stepIndex;
    unsigned long _stepStartTime;
    bool _patternComplete;

    struct ToneStep {
        uint16_t frequency;  // 0 = silence
        uint16_t durationMs;
    };

    void playTone(uint16_t freq);
    void stopTone();
    void runSequence(const ToneStep* steps, uint8_t count);
};

#endif
