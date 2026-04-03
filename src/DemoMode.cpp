#include "DemoMode.h"
#include "BBDEFS.h"

DemoMode::DemoMode(BETABRITE* sign, StatusIndicator* status)
    : _sign(sign), _status(status) {}

void DemoMode::activeDelay(unsigned long ms) {
    unsigned long start = millis();
    while (millis() - start < ms) {
        if (_status) _status->loop();
        delay(10);
    }
}

void DemoMode::showAndWait(const char* text, char color, char mode, char special,
                           unsigned long durationMs, char charset, char position) {
    // Build formatted string with charset prefix
    String formatted;
    formatted += BB_FC_SELECTCHARSET;
    formatted += charset;
    formatted += text;

    _sign->CancelPriorityTextFile();
    _sign->WritePriorityTextFile(formatted.c_str(), color, position, mode, special);
    activeDelay(durationMs);
}

// Phase 1: Cycle through sign colors
void DemoMode::phaseColorShowcase() {
    Serial.println("DemoMode: Phase 1 - Color Showcase");

    struct ColorEntry { const char* name; char code; };
    ColorEntry colors[] = {
        {"RED",       BB_COL_RED},
        {"GREEN",     BB_COL_GREEN},
        {"AMBER",     BB_COL_AMBER},
        {"ORANGE",    BB_COL_ORANGE},
        {"YELLOW",    BB_COL_YELLOW},
        {"RAINBOW",   BB_COL_RAINBOW1},
        {"AUTO",      BB_COL_AUTOCOLOR},
    };

    for (size_t i = 0; i < sizeof(colors) / sizeof(colors[0]); i++) {
        showAndWait(colors[i].name, colors[i].code, BB_DM_HOLD, BB_SDM_TWINKLE,
                    2000, BB_CS_10HIGH);
    }
}

// Phase 2: Flashy special effects
void DemoMode::phaseSpecialEffects() {
    Serial.println("DemoMode: Phase 2 - Special Effects");

    struct EffectEntry { const char* name; char special; };
    EffectEntry effects[] = {
        {"FIREWORKS",  BB_SDM_FIREWORKS},
        {"STARBURST",  BB_SDM_STARBURST},
        {"SPARKLE",    BB_SDM_SPARKLE},
        {"INTERLOCK",  BB_SDM_INTERLOCK},
        {"NEWSFLASH",  BB_SDM_NEWSFLASH},
    };

    for (size_t i = 0; i < sizeof(effects) / sizeof(effects[0]); i++) {
        showAndWait(effects[i].name, BB_COL_AUTOCOLOR, BB_DM_SPECIAL,
                    effects[i].special, 4000, BB_CS_7HIGH);
    }
}

// Phase 3: Inline multi-color text
void DemoMode::phaseRainbowText() {
    Serial.println("DemoMode: Phase 3 - Rainbow Text");

    // Build "RAINBOW" with each letter in a different color using inline color codes
    String rainbow;
    rainbow += BB_FC_SELECTCHARSET;
    rainbow += BB_CS_10HIGH;
    rainbow += BB_FC_SELECTCHARCOLOR;
    rainbow += BB_COL_RED;
    rainbow += 'R';
    rainbow += BB_FC_SELECTCHARCOLOR;
    rainbow += BB_COL_ORANGE;
    rainbow += 'A';
    rainbow += BB_FC_SELECTCHARCOLOR;
    rainbow += BB_COL_YELLOW;
    rainbow += 'I';
    rainbow += BB_FC_SELECTCHARCOLOR;
    rainbow += BB_COL_GREEN;
    rainbow += 'N';
    rainbow += BB_FC_SELECTCHARCOLOR;
    rainbow += BB_COL_AMBER;
    rainbow += 'B';
    rainbow += BB_FC_SELECTCHARCOLOR;
    rainbow += BB_COL_RED;
    rainbow += 'O';
    rainbow += BB_FC_SELECTCHARCOLOR;
    rainbow += BB_COL_GREEN;
    rainbow += 'W';

    _sign->CancelPriorityTextFile();
    _sign->WritePriorityTextFile(rainbow.c_str(), BB_COL_RED, BB_DP_TOPLINE,
                                 BB_DM_HOLD, BB_SDM_TWINKLE);
    activeDelay(5000);
}

// Phase 4: Transition to racing demo
void DemoMode::phaseTransition() {
    Serial.println("DemoMode: Phase 4 - Transition");

    _sign->CancelPriorityTextFile();
    activeDelay(1000);

    showAndWait("SOAP BOX DERBY", BB_COL_YELLOW, BB_DM_SPECIAL,
                BB_SDM_TRUMPET, 6000, BB_CS_10HIGH);
}

// Phase 5: Soap box derby racing simulation
void DemoMode::phaseRacingDemo() {
    Serial.println("DemoMode: Phase 5 - Racing Demo");

    struct Racer { const char* pinny; };
    Racer racers[] = {
        {"0042"},
        {"0117"},
        {"0235"},
        {"0008"},
        {"0164"},
        {"0091"},
    };
    const int numRacers = sizeof(racers) / sizeof(racers[0]);

    // Run 3 races, pairing sequential racers
    for (int race = 0; race < numRacers; race += 2) {
        int r1 = race;
        int r2 = race + 1;
        if (r2 >= numRacers) break;

        Serial.printf("DemoMode: Race %d - #%s vs #%s\n",
                       (race / 2) + 1, racers[r1].pinny, racers[r2].pinny);

        // Show lane assignments
        String lanes;
        lanes += BB_FC_SELECTCHARSET;
        lanes += BB_CS_7HIGH;
        lanes += "NOW: #";
        lanes += racers[r1].pinny;
        lanes += "  NEXT: #";
        lanes += racers[r2].pinny;

        _sign->CancelPriorityTextFile();
        _sign->WritePriorityTextFile(lanes.c_str(), BB_COL_AMBER, BB_DP_TOPLINE,
                                     BB_DM_HOLD, BB_SDM_TWINKLE);
        activeDelay(5000);

        // Show current racer large
        String racing;
        racing += BB_FC_SELECTCHARSET;
        racing += BB_CS_10HIGH;
        racing += "RACING: #";
        racing += racers[r1].pinny;

        _sign->CancelPriorityTextFile();
        _sign->WritePriorityTextFile(racing.c_str(), BB_COL_GREEN, BB_DP_TOPLINE,
                                     BB_DM_HOLD, BB_SDM_TWINKLE);
        activeDelay(4000);

        // Countdown: 3... 2... 1... GO!
        String three;
        three += BB_FC_SELECTCHARSET;
        three += BB_CS_10HIGH;
        three += '3';
        _sign->CancelPriorityTextFile();
        _sign->WritePriorityTextFile(three.c_str(), BB_COL_RED, BB_DP_TOPLINE,
                                     BB_DM_HOLD, BB_SDM_TWINKLE);
        activeDelay(1000);

        String two;
        two += BB_FC_SELECTCHARSET;
        two += BB_CS_10HIGH;
        two += '2';
        _sign->CancelPriorityTextFile();
        _sign->WritePriorityTextFile(two.c_str(), BB_COL_RED, BB_DP_TOPLINE,
                                     BB_DM_HOLD, BB_SDM_TWINKLE);
        activeDelay(1000);

        String one;
        one += BB_FC_SELECTCHARSET;
        one += BB_CS_10HIGH;
        one += '1';
        _sign->CancelPriorityTextFile();
        _sign->WritePriorityTextFile(one.c_str(), BB_COL_AMBER, BB_DP_TOPLINE,
                                     BB_DM_HOLD, BB_SDM_TWINKLE);
        activeDelay(1000);

        showAndWait("GO!", BB_COL_GREEN, BB_DM_FLASH, BB_SDM_TWINKLE,
                    2000, BB_CS_10HIGH);

        // Simulate race in progress
        showAndWait("RACE IN PROGRESS", BB_COL_GREEN, BB_DM_SCROLL,
                    BB_SDM_TWINKLE, 4000, BB_CS_7HIGH);

        // Winner announcement
        String winner;
        winner += BB_FC_SELECTCHARSET;
        winner += BB_CS_10HIGH;
        winner += "WINNER! #";
        winner += racers[r1].pinny;

        _sign->CancelPriorityTextFile();
        _sign->WritePriorityTextFile(winner.c_str(), BB_COL_RAINBOW1, BB_DP_TOPLINE,
                                     BB_DM_SPECIAL, BB_SDM_STARBURST);
        activeDelay(6000);

        // Brief pause between races
        _sign->CancelPriorityTextFile();
        activeDelay(1500);
    }
}

void DemoMode::run() {
    Serial.println("DemoMode: === DEMO SEQUENCE STARTED ===");
    Serial.println("DemoMode: Power cycle to exit");

    // Blank the sign immediately
    _sign->CancelPriorityTextFile();
    _sign->WritePriorityTextFile(" ", BB_COL_GREEN, BB_DP_TOPLINE, BB_DM_HOLD, BB_SDM_TWINKLE);
    activeDelay(500);

    if (_status) {
        _status->onBoot();
        _status->setLEDPattern("rainbow");
    }

    while (true) {
        phaseColorShowcase();
        phaseSpecialEffects();
        phaseRainbowText();
        phaseTransition();
        phaseRacingDemo();
        Serial.println("DemoMode: --- Looping demo sequence ---");
    }
}
