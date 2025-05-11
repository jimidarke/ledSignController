#ifndef MESSAGE_PARSER_H
#define MESSAGE_PARSER_H

#include <ArduinoJson.h>
#include "BETABRITE.h"

// Size of the JSON document for message parsing
// Adjust based on the complexity of your expected payloads
#define JSON_DOCUMENT_SIZE 512

// Valid display modes map
struct ModeMapping {
    const char* name;
    char code;
};

// Valid colors map
extern const ModeMapping COLOR_MODES[];
extern const ModeMapping DISPLAY_MODES[];
extern const ModeMapping SPECIAL_MODES[];
extern const ModeMapping POSITION_MODES[];

// Message types
enum MessageType {
    NORMAL_MESSAGE,
    PRIORITY_MESSAGE,
    CLEAR_SIGN,
    FACTORY_RESET,
    SHOW_OPTIONS,
    INVALID_MESSAGE
};

// Structure to hold parsed message details
struct ParsedMessage {
    MessageType type;
    String text;
    char color;
    char position;
    char mode;
    char special;
    bool isValid;
    String errorMsg;
};

// Parse a JSON message
ParsedMessage parseJsonMessage(const String& jsonString);

// Validate a parsed message settings
bool validateMessageSettings(ParsedMessage& message);

// Find a mode code from its name
char findModeCode(const String& modeName, const ModeMapping* modeMap, int mapSize, char defaultValue);

#endif // MESSAGE_PARSER_H
