/**
 * @file MessageParser.cpp
 * @brief Implementation of message parsing utilities
 * 
 * Contains the implementation of all message parsing functionality including
 * option lookup tables, validation, and parsing logic.
 */

#include "MessageParser.h"

// Color option lookup table - maps string names to BetaBrite color codes
const OptionMap MessageParser::colorOptions[] PROGMEM = {
    {"red", BB_COL_RED, false},
    {"amber", BB_COL_AMBER, false},
    {"green", BB_COL_GREEN, false},
    {"yellow", BB_COL_YELLOW, false},
    {"orange", BB_COL_ORANGE, false},
    {"brown", BB_COL_BROWN, false},
    {"dimgreen", BB_COL_DIMGREEN, false},
    {"dimred", BB_COL_DIMRED, false},
    {"rainbow1", BB_COL_RAINBOW1, false},
    {"rainbow2", BB_COL_RAINBOW2, false},
    {"autocolor", BB_COL_AUTOCOLOR, false},
    {"colormix", BB_COL_COLORMIX, false}
};

// Display mode lookup table - maps string names to BetaBrite display modes
const OptionMap MessageParser::modeOptions[] PROGMEM = {
    {"rotate", BB_DM_ROTATE, false},
    {"hold", BB_DM_HOLD, false},
    {"flash", BB_DM_FLASH, false},
    {"rollup", BB_DM_ROLLUP, false},
    {"rolldown", BB_DM_ROLLDOWN, false},
    {"rollleft", BB_DM_ROLLLEFT, false},
    {"rollright", BB_DM_ROLLRIGHT, false},
    {"wipeup", BB_DM_WIPEUP, false},
    {"wipedown", BB_DM_WIPEDOWN, false},
    {"wipeleft", BB_DM_WIPELEFT, false},
    {"wiperight", BB_DM_WIPERIGHT, false},
    {"scroll", BB_DM_SCROLL, false},
    {"automode", BB_DM_AUTOMODE, false},
    {"rollin", BB_DM_ROLLIN, false},
    {"rollout", BB_DM_ROLLOUT, false},
    {"wipein", BB_DM_WIPEIN, false},
    {"wipeout", BB_DM_WIPEOUT, false},
    {"comprotate", BB_DM_COMPROTATE, false},
    {"explode", BB_DM_EXPLODE, false},
    {"clock", BB_DM_CLOCK, false}
};

// Special effect lookup table - maps string names to BetaBrite special effects
const OptionMap MessageParser::specialOptions[] PROGMEM = {
    {"trumpet", BB_SDM_TRUMPET, true},
    {"twinkle", BB_SDM_TWINKLE, true},
    {"sparkle", BB_SDM_SPARKLE, true},
    {"snow", BB_SDM_SNOW, true},
    {"interlock", BB_SDM_INTERLOCK, true},
    {"switch", BB_SDM_SWITCH, true},
    {"slide", BB_SDM_SLIDE, true},
    {"spray", BB_SDM_SPRAY, true},
    {"starburst", BB_SDM_STARBURST, true},
    {"welcome", BB_SDM_WELCOME, true},
    {"slots", BB_SDM_SLOTS, true},
    {"newsflash", BB_SDM_NEWSFLASH, true},
    {"cyclecolors", BB_SDM_CYCLECOLORS, true},
    {"thankyou", BB_SDM_THANKYOU, true},
    {"nosmoking", BB_SDM_NOSMOKING, true},
    {"dontdrinkanddrive", BB_SDM_DONTDRINKANDDRIVE, true},
    {"fish", BB_SDM_FISHIMAL, true},
    {"fireworks", BB_SDM_FIREWORKS, true},
    {"balloon", BB_SDM_TURBALLOON, true},
    {"bomb", BB_SDM_BOMB, true}
};

bool MessageParser::validateMessage(const char* msg) {
    // Null pointer check
    if (msg == NULL) {
        Serial.println("Error: Null message received");
        return false;
    }
    
    // Empty message check
    size_t msg_len = strlen(msg);
    if (msg_len == 0) {
        Serial.println("Error: Empty message received");
        return false;
    }
    
    // Length validation
    if (msg_len > MAX_MESSAGE_LENGTH) {
        Serial.print("Error: Message too long (");
        Serial.print(msg_len);
        Serial.print(" > ");
        Serial.print(MAX_MESSAGE_LENGTH);
        Serial.println("), rejecting");
        return false;
    }
    
    return true;
}

bool MessageParser::isSystemCommand(const char* msg) {
    return (msg != NULL && (msg[0] == '#' || msg[0] == '^'));
}

bool MessageParser::isPriorityMessage(const char* msg) {
    return (msg != NULL && msg[0] == '*');
}

void MessageParser::parseOptions(const char* options, char* color, char* position, char* mode, char* special) {
    // Create a copy of the options string since strtok modifies it
    char optionsCopy[MAX_OPTIONS_LENGTH];
    strncpy(optionsCopy, options, MAX_OPTIONS_LENGTH - 1);
    optionsCopy[MAX_OPTIONS_LENGTH - 1] = '\0';
    
    char* option = strtok(optionsCopy, ",");
    while (option != NULL) {
        // Trim leading whitespace
        while (*option == ' ' || *option == '\t') {
            option++;
        }
        
        bool found = false;
        
        // Check color options
        for (size_t i = 0; i < sizeof(colorOptions) / sizeof(colorOptions[0]); i++) {
            if (strstr(option, colorOptions[i].name) == option) {
                *color = colorOptions[i].value;
                found = true;
                Serial.print("Color: ");
                Serial.println(colorOptions[i].name);
                break;
            }
        }
        
        // Check mode options (only if not found in colors)
        if (!found) {
            for (size_t i = 0; i < sizeof(modeOptions) / sizeof(modeOptions[0]); i++) {
                if (strstr(option, modeOptions[i].name) == option) {
                    *mode = modeOptions[i].value;
                    found = true;
                    Serial.print("Mode: ");
                    Serial.println(modeOptions[i].name);
                    break;
                }
            }
        }
        
        // Check special options (only if not found in colors or modes)
        if (!found) {
            for (size_t i = 0; i < sizeof(specialOptions) / sizeof(specialOptions[0]); i++) {
                if (strstr(option, specialOptions[i].name) == option) {
                    *mode = BB_DM_SPECIAL;  // Special effects require special mode
                    *special = specialOptions[i].value;
                    found = true;
                    Serial.print("Special: ");
                    Serial.println(specialOptions[i].name);
                    break;
                }
            }
        }
        
        if (!found) {
            Serial.print("Warning: Unknown option: ");
            Serial.println(option);
        }
        
        option = strtok(NULL, ",");
    }
}

bool MessageParser::parseMessage(const char* msg, char* color, char* position, 
                                char* mode, char* special, String* messageContent) {
    
    // Validate input message
    if (!validateMessage(msg)) {
        return false;
    }
    
    // Set default values
    *color = BB_COL_AUTOCOLOR;
    *position = BB_DP_TOPLINE;
    *mode = BB_DM_ROTATE;
    *special = BB_SDM_TWINKLE;
    
    // Look for option brackets [option1,option2]
    char *open_delim = strchr(msg, '[');
    char *close_delim = strchr(msg, ']');
    
    if (open_delim == msg && close_delim != NULL) {
        // Options found at start of message
        int options_length = close_delim - open_delim;
        
        // Validate options string length
        if (options_length >= MAX_OPTIONS_LENGTH) {
            Serial.println("Error: Options string too long, using defaults");
        } else {
            // Extract options string (without brackets)
            char options[MAX_OPTIONS_LENGTH];
            strncpy(options, msg + 1, options_length - 1);
            options[options_length - 1] = '\0';
            
            Serial.print("Parsing options: ");
            Serial.println(options);
            
            // Parse the options
            parseOptions(options, color, position, mode, special);
        }
        
        // Message content is everything after the closing bracket
        *messageContent = String(close_delim + 1);
    } else {
        // No options, entire string is the message content
        *messageContent = String(msg);
    }
    
    // Log final parsed values
    Serial.println("Parsed message parameters:");
    Serial.print("  Color: 0x");
    Serial.println(*color, HEX);
    Serial.print("  Position: 0x");
    Serial.println(*position, HEX);
    Serial.print("  Mode: 0x");
    Serial.println(*mode, HEX);
    Serial.print("  Special: 0x");
    Serial.println(*special, HEX);
    Serial.print("  Content: ");
    Serial.println(*messageContent);
    
    return true;
}