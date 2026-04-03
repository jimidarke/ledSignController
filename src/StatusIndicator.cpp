#include "StatusIndicator.h"
#include "defines.h"

StatusIndicator::StatusIndicator()
    : _led(RGB_RED_PIN, RGB_GREEN_PIN, RGB_BLUE_PIN,
           LEDC_CH_RED, LEDC_CH_GREEN, LEDC_CH_BLUE),
      _buzzer(BUZZER_PIN, LEDC_CH_BUZZER),
      _basePattern(LEDPattern::OFF),
      _basePriority(PRI_IDLE),
      _transientPriority(PRI_IDLE),
      _transientExpiry(0) {}

void StatusIndicator::begin() {
    _led.begin();
    _buzzer.begin();
    Serial.println("StatusIndicator: Initialized (RGB LED + Buzzer)");
}

void StatusIndicator::loop() {
    // Check if transient pattern expired -> restore base
    if (_transientPriority > PRI_IDLE && millis() >= _transientExpiry) {
        _transientPriority = PRI_IDLE;
        _led.setPattern(_basePattern);
    }

    _led.loop();
    _buzzer.loop();
}

void StatusIndicator::setBase(LEDPattern led, Priority pri) {
    _basePattern = led;
    _basePriority = pri;
    // Only apply if no higher-priority transient is active
    if (_transientPriority <= pri) {
        _led.setPattern(led);
    }
}

void StatusIndicator::setTransient(LEDPattern led, BuzzerPattern buzz,
                                    Priority pri, unsigned long durationMs) {
    // Only override if priority is high enough
    if (pri >= _transientPriority || _transientPriority == PRI_IDLE) {
        _transientPriority = pri;
        _transientExpiry = millis() + durationMs;
        _led.setPattern(led);
    }
    // Buzzer always plays if not muted (short patterns don't need priority gating)
    if (buzz != BuzzerPattern::SILENT) {
        _buzzer.setPattern(buzz);
    }
}

// --- System Events ---

void StatusIndicator::onBoot() {
    setTransient(LEDPattern::RAINBOW_CYCLE, BuzzerPattern::STARTUP_CHIME,
                 PRI_BOOT, 2000);
}

void StatusIndicator::onWiFiConnected() {
    // Clear any sustained WiFi-down pattern
    if (_basePriority <= PRI_SUSTAINED) {
        setBase(LEDPattern::OFF, PRI_IDLE);
    }
    setTransient(LEDPattern::FLASH_GREEN, BuzzerPattern::BEEP_SHORT,
                 PRI_STATUS, 500);
}

void StatusIndicator::onWiFiDisconnected() {
    setBase(LEDPattern::BREATHE_RED, PRI_SUSTAINED);
}

void StatusIndicator::onMQTTConnected() {
    // Clear MQTT-down base if it was set
    if (_basePattern == LEDPattern::FLASH_AMBER) {
        setBase(LEDPattern::OFF, PRI_IDLE);
    }
    setTransient(LEDPattern::SOLID_BLUE, BuzzerPattern::SILENT,
                 PRI_STATUS, 1000);
}

void StatusIndicator::onMQTTDisconnected() {
    // Only set if not already showing a higher-priority sustained pattern
    if (_basePriority <= PRI_SUSTAINED) {
        setBase(LEDPattern::FLASH_AMBER, PRI_SUSTAINED);
    }
}

void StatusIndicator::onMessageReceived() {
    setTransient(LEDPattern::FLASH_GREEN, BuzzerPattern::SILENT,
                 PRI_STATUS, 400);
}

void StatusIndicator::onPriorityAlert() {
    setTransient(LEDPattern::FLASH_RED_RAPID, BuzzerPattern::URGENT_BEEPS,
                 PRI_CRITICAL, 5000);
}

void StatusIndicator::onWarningAlert() {
    setTransient(LEDPattern::FLASH_AMBER_DOUBLE, BuzzerPattern::BEEP_DOUBLE,
                 PRI_WARNING, 1000);
}

void StatusIndicator::onOTAStarted() {
    setBase(LEDPattern::BREATHE_BLUE, PRI_OTA);
}

void StatusIndicator::onOTAComplete() {
    setBase(LEDPattern::OFF, PRI_IDLE);
    setTransient(LEDPattern::SUCCESS_GREEN, BuzzerPattern::SUCCESS_JINGLE,
                 PRI_OTA, 1500);
}

void StatusIndicator::onError() {
    setTransient(LEDPattern::FLASH_RED_SLOW, BuzzerPattern::BEEP_ERROR,
                 PRI_WARNING, 2000);
}

void StatusIndicator::onIdle() {
    setBase(LEDPattern::OFF, PRI_IDLE);
    _transientPriority = PRI_IDLE;
    _led.off();
}

// --- HA Control ---

void StatusIndicator::setLEDPattern(const String& name) {
    if (name == "off")           _led.setPattern(LEDPattern::OFF);
    else if (name == "red")      _led.setPattern(LEDPattern::SOLID_RED);
    else if (name == "green")    _led.setPattern(LEDPattern::SOLID_GREEN);
    else if (name == "blue")     _led.setPattern(LEDPattern::SOLID_BLUE);
    else if (name == "amber")    _led.setPattern(LEDPattern::SOLID_AMBER);
    else if (name == "breathe_blue") _led.setPattern(LEDPattern::BREATHE_BLUE);
    else if (name == "rainbow")  _led.setPattern(LEDPattern::RAINBOW_CYCLE);
    // HA manual control overrides base pattern
    _basePriority = PRI_IDLE;
    _transientPriority = PRI_IDLE;
}

void StatusIndicator::triggerBuzzer(const String& name) {
    if (name == "beep")          _buzzer.setPattern(BuzzerPattern::BEEP_SHORT);
    else if (name == "double")   _buzzer.setPattern(BuzzerPattern::BEEP_DOUBLE);
    else if (name == "error")    _buzzer.setPattern(BuzzerPattern::BEEP_ERROR);
    else if (name == "urgent")   _buzzer.setPattern(BuzzerPattern::URGENT_BEEPS);
    else if (name == "chime")    _buzzer.setPattern(BuzzerPattern::STARTUP_CHIME);
    else if (name == "jingle")   _buzzer.setPattern(BuzzerPattern::SUCCESS_JINGLE);
}
