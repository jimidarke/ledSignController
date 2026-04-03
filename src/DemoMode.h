#ifndef DEMO_MODE_H
#define DEMO_MODE_H

#include <Arduino.h>
#include "BETABRITE.h"
#include "StatusIndicator.h"

class DemoMode {
public:
    DemoMode(BETABRITE* sign, StatusIndicator* status);

    // Runs the demo sequence forever. Never returns.
    void run();

private:
    BETABRITE* _sign;
    StatusIndicator* _status;

    void phaseColorShowcase();
    void phaseSpecialEffects();
    void phaseRainbowText();
    void phaseTransition();
    void phaseRacingDemo();

    // Display text on sign and wait, keeping status LED alive
    void showAndWait(const char* text, char color, char mode, char special,
                     unsigned long durationMs, char charset = BB_CS_7HIGH,
                     char position = BB_DP_TOPLINE);

    // Pump StatusIndicator loop during delays
    void activeDelay(unsigned long ms);
};

#endif
