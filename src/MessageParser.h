/**
 * @file MessageParser.h
 * @brief Message parsing utilities for LED Sign Controller
 *
 * ⚠️  DEPRECATED - This module is no longer used ⚠️
 *
 * As of version 0.2.0, the LED Sign Controller uses JSON-only message format
 * for Alert Manager integration. This bracket notation parser is kept for
 * reference purposes only.
 *
 * Previously handled:
 * - Option parsing from bracket notation [color,effect]
 * - Priority message handling (*message)
 * - System command processing (#clear, ^reset)
 * - Input validation and sanitization
 *
 * @deprecated Use JSON format with handleMQTTMessage() in main.cpp
 * @see docs/ESP32_BETABRITE_IMPLEMENTATION.md for current message format
 * @author LED Sign Controller Project
 * @version 0.1.4 (deprecated)
 * @date 2024
 */

#ifndef MESSAGE_PARSER_H
#define MESSAGE_PARSER_H

#include <Arduino.h>
#include "BETABRITE.h"

/**
 * @brief Structure for mapping option names to LED sign values
 * 
 * Used for efficient lookup of color, mode, and special effect options
 * from string-based commands.
 */
struct OptionMap {
    const char* name;    ///< Option name as string (e.g., "red", "rotate")
    char value;          ///< Corresponding LED sign command value
    bool isSpecial;      ///< Whether this option requires special mode
};

/**
 * @brief Message parsing and command processing class
 * 
 * Handles all aspects of message parsing including option extraction,
 * validation, and conversion to LED sign commands.
 */
class MessageParser {
private:
    static const int MAX_MESSAGE_LENGTH = 1024;     ///< Maximum allowed message length
    static const int MAX_OPTIONS_LENGTH = 256;      ///< Maximum options string length
    
    // Option lookup tables for efficient parsing
    static const OptionMap colorOptions[];          ///< Color option mappings
    static const OptionMap modeOptions[];           ///< Display mode mappings  
    static const OptionMap specialOptions[];        ///< Special effect mappings
    
    /**
     * @brief Parse options string using lookup tables
     * @param options Comma-separated options string
     * @param color Pointer to color value to update
     * @param position Pointer to position value to update
     * @param mode Pointer to mode value to update
     * @param special Pointer to special effect value to update
     */
    static void parseOptions(const char* options, char* color, char* position, char* mode, char* special);
    
public:
    /**
     * @brief Validate incoming message for safety and correctness
     * @param msg Message to validate
     * @return true if message is valid, false otherwise
     */
    static bool validateMessage(const char* msg);
    
    /**
     * @brief Check if message is a system command
     * @param msg Message to check
     * @return true if message starts with # or ^
     */
    static bool isSystemCommand(const char* msg);
    
    /**
     * @brief Check if message is a priority message
     * @param msg Message to check
     * @return true if message starts with *
     */
    static bool isPriorityMessage(const char* msg);
    
    /**
     * @brief Parse message and extract display parameters
     * @param msg Input message with optional [options] prefix
     * @param color Output: parsed color value
     * @param position Output: parsed position value
     * @param mode Output: parsed display mode value
     * @param special Output: parsed special effect value
     * @param messageContent Output: message content without options
     * @return true if parsing successful, false on error
     */
    static bool parseMessage(const char* msg, char* color, char* position, 
                           char* mode, char* special, String* messageContent);
};

#endif // MESSAGE_PARSER_H