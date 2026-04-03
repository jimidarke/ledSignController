#ifndef STATUS_INDICATOR_H
#define STATUS_INDICATOR_H

#include <Arduino.h>
#include "StatusLED.h"
#include "StatusBuzzer.h"

class StatusIndicator {
public:
    StatusIndicator();
    void begin();
    void loop();

    // System event methods
    void onBoot();
    void onWiFiConnected();
    void onWiFiDisconnected();
    void onMQTTConnected();
    void onMQTTDisconnected();
    void onMessageReceived();
    void onPriorityAlert();
    void onWarningAlert();
    void onOTAStarted();
    void onOTAComplete();
    void onError();
    void onIdle();

    // HA control
    void setLEDPattern(const String& patternName);
    void triggerBuzzer(const String& patternName);
    void setMuted(bool muted) { _buzzer.setMuted(muted); }
    bool isMuted() const { return _buzzer.isMuted(); }

private:
    StatusLED _led;
    StatusBuzzer _buzzer;

    // Priority tracking
    enum Priority : uint8_t {
        PRI_IDLE = 0,
        PRI_STATUS = 1,      // brief flashes (message received, connected)
        PRI_SUSTAINED = 2,   // ongoing state (WiFi down, MQTT down)
        PRI_OTA = 3,
        PRI_WARNING = 4,
        PRI_CRITICAL = 5,
        PRI_BOOT = 6
    };

    LEDPattern _basePattern;
    Priority _basePriority;
    Priority _transientPriority;
    unsigned long _transientExpiry;

    void setTransient(LEDPattern led, BuzzerPattern buzz, Priority pri, unsigned long durationMs);
    void setBase(LEDPattern led, Priority pri);
};

#endif
