#include "MessageParser.h"
#include <Arduino.h>

// Definition of mapping arrays
const ModeMapping COLOR_MODES[] = {
    {"red", BB_COL_RED},
    {"green", BB_COL_GREEN},
    {"amber", BB_COL_AMBER},
    {"dimred", BB_COL_DIMRED},
    {"dimgreen", BB_COL_DIMGREEN},
    {"brown", BB_COL_BROWN},
    {"orange", BB_COL_ORANGE},
    {"yellow", BB_COL_YELLOW},
    {"rainbow1", BB_COL_RAINBOW1},
    {"rainbow2", BB_COL_RAINBOW2},
    {"colormix", BB_COL_COLORMIX},
    {"autocolor", BB_COL_AUTOCOLOR}
};

const ModeMapping DISPLAY_MODES[] = {
    {"rotate", BB_DM_ROTATE},
    {"hold", BB_DM_HOLD},
    {"flash", BB_DM_FLASH},
    {"rollup", BB_DM_ROLLUP},
    {"rolldown", BB_DM_ROLLDOWN},
    {"rollleft", BB_DM_ROLLLEFT},
    {"rollright", BB_DM_ROLLRIGHT},
    {"wipeup", BB_DM_WIPEUP},
    {"wipedown", BB_DM_WIPEDOWN},
    {"wipeleft", BB_DM_WIPELEFT},
    {"wiperight", BB_DM_WIPERIGHT},
    {"scroll", BB_DM_SCROLL},
    {"special", BB_DM_SPECIAL},
    {"automode", BB_DM_AUTOMODE},
    {"rollin", BB_DM_ROLLIN},
    {"rollout", BB_DM_ROLLOUT},
    {"wipein", BB_DM_WIPEIN},
    {"wipeout", BB_DM_WIPEOUT},
    {"comprotate", BB_DM_COMPROTATE},
    {"explode", BB_DM_EXPLODE},
    {"clock", BB_DM_CLOCK}
};

const ModeMapping SPECIAL_MODES[] = {
    {"twinkle", BB_SDM_TWINKLE},
    {"sparkle", BB_SDM_SPARKLE},
    {"snow", BB_SDM_SNOW},
    {"interlock", BB_SDM_INTERLOCK},
    {"switch", BB_SDM_SWITCH},
    {"slide", BB_SDM_SLIDE},
    {"spray", BB_SDM_SPRAY},
    {"starburst", BB_SDM_STARBURST},
    {"welcome", BB_SDM_WELCOME},
    {"slots", BB_SDM_SLOTS},
    {"newsflash", BB_SDM_NEWSFLASH},
    {"trumpet", BB_SDM_TRUMPET},
    {"cyclecolors", BB_SDM_CYCLECOLORS},
    {"thankyou", BB_SDM_THANKYOU},
    {"nosmoking", BB_SDM_NOSMOKING},
    {"dontdrinkanddrive", BB_SDM_DONTDRINKANDDRIVE},
    {"fishimal", BB_SDM_FISHIMAL},
    {"fireworks", BB_SDM_FIREWORKS},
    {"turballoon", BB_SDM_TURBALLOON},
    {"bomb", BB_SDM_BOMB}
};

const ModeMapping POSITION_MODES[] = {
    {"midline", BB_DP_MIDLINE},
    {"topline", BB_DP_TOPLINE},
    {"botline", BB_DP_BOTLINE},
    {"fill", BB_DP_FILL},
    {"left", BB_DP_LEFT},
    {"right", BB_DP_RIGHT}
};

// Find the code for a mode given its name
char findModeCode(const String& modeName, const ModeMapping* modeMap, int mapSize, char defaultValue) {
    String lowerModeName = modeName;
    lowerModeName.toLowerCase();
    
    for (int i = 0; i < mapSize; i++) {
        if (lowerModeName == modeMap[i].name) {
            return modeMap[i].code;
        }
    }
    
    return defaultValue;
}

// Parse JSON message
ParsedMessage parseJsonMessage(const String& jsonString) {
    ParsedMessage result;
    
    // Set defaults
    result.type = NORMAL_MESSAGE;
    result.color = BB_COL_AUTOCOLOR;
    result.position = BB_DP_TOPLINE;
    result.mode = BB_DM_ROTATE;
    result.special = BB_SDM_TWINKLE;
    result.isValid = false;
    
    // Create JSON document for parsing
    StaticJsonDocument<JSON_DOCUMENT_SIZE> doc;
    DeserializationError error = deserializeJson(doc, jsonString);
    
    // Check for parsing errors
    if (error) {
        result.errorMsg = "JSON parsing failed: ";
        result.errorMsg += error.c_str();
        return result;
    }
    
    // Check for type field
    if (doc.containsKey("type")) {
        String typeStr = doc["type"].as<String>();
        typeStr.toLowerCase();
        
        if (typeStr == "priority") {
            result.type = PRIORITY_MESSAGE;
        } else if (typeStr == "clear") {
            result.type = CLEAR_SIGN;
        } else if (typeStr == "reset") {
            result.type = FACTORY_RESET;
        } else if (typeStr == "options") {
            result.type = SHOW_OPTIONS;
        }
    }
    
    // Check for message text if applicable
    if (result.type == NORMAL_MESSAGE || result.type == PRIORITY_MESSAGE) {
        if (!doc.containsKey("text")) {
            result.errorMsg = "Missing 'text' field in message";
            return result;
        }
        
        result.text = doc["text"].as<String>();
        
        // Get display options if provided
        if (doc.containsKey("color")) {
            String colorStr = doc["color"].as<String>();
            result.color = findModeCode(
                colorStr, 
                COLOR_MODES, 
                sizeof(COLOR_MODES)/sizeof(COLOR_MODES[0]), 
                BB_COL_AUTOCOLOR
            );
        }
        
        if (doc.containsKey("position")) {
            String positionStr = doc["position"].as<String>();
            result.position = findModeCode(
                positionStr, 
                POSITION_MODES, 
                sizeof(POSITION_MODES)/sizeof(POSITION_MODES[0]), 
                BB_DP_TOPLINE
            );
        }
        
        if (doc.containsKey("mode")) {
            String modeStr = doc["mode"].as<String>();
            result.mode = findModeCode(
                modeStr, 
                DISPLAY_MODES, 
                sizeof(DISPLAY_MODES)/sizeof(DISPLAY_MODES[0]), 
                BB_DM_ROTATE
            );
        }
        
        if (doc.containsKey("special")) {
            String specialStr = doc["special"].as<String>();
            result.special = findModeCode(
                specialStr, 
                SPECIAL_MODES, 
                sizeof(SPECIAL_MODES)/sizeof(SPECIAL_MODES[0]), 
                BB_SDM_TWINKLE
            );
            
            // If a special effect is provided, ensure mode is set to special
            if (result.mode != BB_DM_SPECIAL) {
                result.mode = BB_DM_SPECIAL;
            }
        }
    }
      result.isValid = true;
    return result;
}

// Validate message settings
bool validateMessageSettings(ParsedMessage& message) {
    // For now, all settings are considered valid because we're using 
    // the findModeCode function which will default to valid values
    return true;
}
