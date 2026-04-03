/**
 * @file demo_only_main.cpp
 * @brief Standalone demo mode - runs demo immediately on boot, nothing else.
 *
 * To use: rename main.cpp -> main.cpp.disabled, rename this -> main.cpp
 * Upload with: pio run -t upload
 */

#include <Arduino.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "BETABRITE.h"
#include "BBDEFS.h"

BETABRITE sign(1, 17, 16);

// Pump delay without blocking
void activeDelay(unsigned long ms) {
    unsigned long start = millis();
    while (millis() - start < ms) {
        delay(10);
    }
}

void blankSign() {
    // Exactly what racing_countdown.py does: file A, ESC+TOPLINE+HOLD, space
    // Using AUTOCOLOR skips the color select byte, matching the python script
    sign.CancelPriorityTextFile();
    sign.WriteTextFile('A', " ", BB_COL_AUTOCOLOR, BB_DP_TOPLINE, BB_DM_HOLD, BB_SDM_TWINKLE);
    activeDelay(500);
}

void showText(const char* text, char color, char mode, char special,
              unsigned long durationMs, char charset = BB_CS_7HIGH) {
    String formatted;
    formatted += BB_FC_SELECTCHARSET;
    formatted += charset;
    formatted += text;

    sign.CancelPriorityTextFile();
    sign.WritePriorityTextFile(formatted.c_str(), color, BB_DP_TOPLINE, mode, special);
    activeDelay(durationMs);
}

// Phase 1: Color showcase
void phaseColors() {
    Serial.println("Demo: Colors");
    struct { const char* name; char code; } colors[] = {
        {"RED",     BB_COL_RED},
        {"GREEN",   BB_COL_GREEN},
        {"AMBER",   BB_COL_AMBER},
        {"ORANGE",  BB_COL_ORANGE},
        {"YELLOW",  BB_COL_YELLOW},
        {"RAINBOW", BB_COL_RAINBOW1},
        {"AUTO",    BB_COL_AUTOCOLOR},
    };
    for (size_t i = 0; i < 7; i++) {
        showText(colors[i].name, colors[i].code, BB_DM_HOLD, BB_SDM_TWINKLE, 2000, BB_CS_10HIGH);
    }
}

// Phase 2: Effects
void phaseEffects() {
    Serial.println("Demo: Effects");
    struct { const char* name; char special; } effects[] = {
        {"FIREWORKS",  BB_SDM_FIREWORKS},
        {"STARBURST",  BB_SDM_STARBURST},
        {"SPARKLE",    BB_SDM_SPARKLE},
        {"INTERLOCK",  BB_SDM_INTERLOCK},
        {"NEWSFLASH",  BB_SDM_NEWSFLASH},
    };
    for (size_t i = 0; i < 5; i++) {
        showText(effects[i].name, BB_COL_AUTOCOLOR, BB_DM_SPECIAL, effects[i].special, 4000);
    }
}

// Phase 3: Rainbow text
void phaseRainbow() {
    Serial.println("Demo: Rainbow");
    String rainbow;
    rainbow += BB_FC_SELECTCHARSET;
    rainbow += BB_CS_10HIGH;
    rainbow += BB_FC_SELECTCHARCOLOR; rainbow += BB_COL_RED;    rainbow += 'R';
    rainbow += BB_FC_SELECTCHARCOLOR; rainbow += BB_COL_ORANGE; rainbow += 'A';
    rainbow += BB_FC_SELECTCHARCOLOR; rainbow += BB_COL_YELLOW; rainbow += 'I';
    rainbow += BB_FC_SELECTCHARCOLOR; rainbow += BB_COL_GREEN;  rainbow += 'N';
    rainbow += BB_FC_SELECTCHARCOLOR; rainbow += BB_COL_AMBER;  rainbow += 'B';
    rainbow += BB_FC_SELECTCHARCOLOR; rainbow += BB_COL_RED;    rainbow += 'O';
    rainbow += BB_FC_SELECTCHARCOLOR; rainbow += BB_COL_GREEN;  rainbow += 'W';

    sign.CancelPriorityTextFile();
    sign.WritePriorityTextFile(rainbow.c_str(), BB_COL_RED, BB_DP_TOPLINE, BB_DM_HOLD, BB_SDM_TWINKLE);
    activeDelay(5000);
}

// Phase 4: Transition
void phaseTransition() {
    Serial.println("Demo: Transition");
    blankSign();
    activeDelay(1000);
    showText("SOAP BOX DERBY", BB_COL_YELLOW, BB_DM_SPECIAL, BB_SDM_TRUMPET, 6000, BB_CS_10HIGH);
}

// Phase 5: Racing demo
void phaseRacing() {
    Serial.println("Demo: Racing");
    const char* pinnies[] = {"0042", "0117", "0235", "0008", "0164", "0091"};

    for (int race = 0; race < 6; race += 2) {
        int r1 = race, r2 = race + 1;

        // Lane assignments
        String lanes;
        lanes += BB_FC_SELECTCHARSET; lanes += BB_CS_7HIGH;
        lanes += "NOW: #"; lanes += pinnies[r1];
        lanes += "  NEXT: #"; lanes += pinnies[r2];
        sign.CancelPriorityTextFile();
        sign.WritePriorityTextFile(lanes.c_str(), BB_COL_AMBER, BB_DP_TOPLINE, BB_DM_HOLD, BB_SDM_TWINKLE);
        activeDelay(5000);

        // Current racer
        String racing;
        racing += BB_FC_SELECTCHARSET; racing += BB_CS_10HIGH;
        racing += "RACING: #"; racing += pinnies[r1];
        sign.CancelPriorityTextFile();
        sign.WritePriorityTextFile(racing.c_str(), BB_COL_GREEN, BB_DP_TOPLINE, BB_DM_HOLD, BB_SDM_TWINKLE);
        activeDelay(4000);

        // Countdown
        String s;

        s = ""; s += BB_FC_SELECTCHARSET; s += BB_CS_10HIGH; s += '3';
        sign.CancelPriorityTextFile();
        sign.WritePriorityTextFile(s.c_str(), BB_COL_RED, BB_DP_TOPLINE, BB_DM_HOLD, BB_SDM_TWINKLE);
        activeDelay(1000);

        s = ""; s += BB_FC_SELECTCHARSET; s += BB_CS_10HIGH; s += '2';
        sign.CancelPriorityTextFile();
        sign.WritePriorityTextFile(s.c_str(), BB_COL_RED, BB_DP_TOPLINE, BB_DM_HOLD, BB_SDM_TWINKLE);
        activeDelay(1000);

        s = ""; s += BB_FC_SELECTCHARSET; s += BB_CS_10HIGH; s += '1';
        sign.CancelPriorityTextFile();
        sign.WritePriorityTextFile(s.c_str(), BB_COL_AMBER, BB_DP_TOPLINE, BB_DM_HOLD, BB_SDM_TWINKLE);
        activeDelay(1000);

        showText("GO!", BB_COL_GREEN, BB_DM_FLASH, BB_SDM_TWINKLE, 2000, BB_CS_10HIGH);
        showText("RACE IN PROGRESS", BB_COL_GREEN, BB_DM_SCROLL, BB_SDM_TWINKLE, 4000);

        // Winner
        String winner;
        winner += BB_FC_SELECTCHARSET; winner += BB_CS_10HIGH;
        winner += "WINNER! #"; winner += pinnies[r1];
        sign.CancelPriorityTextFile();
        sign.WritePriorityTextFile(winner.c_str(), BB_COL_RAINBOW1, BB_DP_TOPLINE, BB_DM_SPECIAL, BB_SDM_STARBURST);
        activeDelay(6000);

        blankSign();
        activeDelay(1500);
    }
}

void setup() {
    WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
    Serial.begin(115200);
    delay(500);
    Serial.println("=== DEMO ONLY MODE ===");

    // Send raw clear command on UART1 (the sign's serial port) manually
    // This is exactly what racing_countdown.py sends:
    // 5x NUL, SOH, '~' (BetaBrite type), "00" (address), STX, 'A' (write text),
    // 'A' (file A), ESC, '"' (topline), 'b' (hold), ' ' (space), EOT
    Serial.println("Sending raw clear to sign UART...");

    // First: wait for sign to be ready
    delay(3000);

    // Send the exact same bytes as the Python script
    uint8_t clear_frame[] = {
        0x00, 0x00, 0x00, 0x00, 0x00,  // 5x NUL sync
        0x01,                            // SOH
        0x3F,                            // Sign type: ALL (same as Python script)
        0x30, 0x30,                      // Address "00"
        0x02,                            // STX
        0x41,                            // 'A' = write text command
        0x41,                            // 'A' = file A
        0x1B,                            // ESC
        0x22,                            // '"' = topline position
        0x62,                            // 'b' = hold mode
        0x20,                            // ' ' = space (blank)
        0x04                             // EOT
    };

    // The BETABRITE library uses HardwareSerial(1) on GPIO 16/17
    // sign is that object — write directly to it
    sign.write(clear_frame, sizeof(clear_frame));
    sign.flush();
    Serial.println("Raw frame sent");
    Serial.printf("Frame bytes: ");
    for (size_t i = 0; i < sizeof(clear_frame); i++) {
        Serial.printf("%02X ", clear_frame[i]);
    }
    Serial.println();

    delay(2000);

    // Also try via the library
    Serial.println("Also trying via library...");
    sign.WriteTextFile('A', " ", BB_COL_AUTOCOLOR, BB_DP_TOPLINE, BB_DM_HOLD, BB_SDM_TWINKLE);
    delay(1000);

    Serial.println("Waiting 10 seconds - IS THE SIGN BLACK?");
    delay(10000);

    // Loop forever
    while (true) {
        phaseColors();
        phaseEffects();
        phaseRainbow();
        phaseTransition();
        phaseRacing();
        Serial.println("--- Looping ---");
    }
}

void loop() {}
