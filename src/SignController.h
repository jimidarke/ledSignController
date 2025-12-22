/**
 * @file SignController.h
 * @brief LED Sign control and management for BetaBrite displays
 * 
 * This module handles all LED sign-related functionality including:
 * - Sign initialization and configuration
 * - Message display with effects and colors
 * - Priority message handling
 * - Clock display management
 * - System commands (clear, reset)
 * - File management on the sign
 * 
 * @author LED Sign Controller Project
 * @version 0.1.4
 * @date 2024
 */

#ifndef SIGN_CONTROLLER_H
#define SIGN_CONTROLLER_H

#include <Arduino.h>
#include "BETABRITE.h"

// Sign configuration constants (from defines.h)
#ifndef SIGN_DEFAULT_COLOUR
#define SIGN_DEFAULT_COLOUR BB_COL_GREEN
#endif
#ifndef SIGN_DEFAULT_POSITION  
#define SIGN_DEFAULT_POSITION BB_DP_TOPLINE
#endif
#ifndef SIGN_DEFAULT_MODE
#define SIGN_DEFAULT_MODE BB_DM_ROTATE
#endif
#ifndef SIGN_DEFAULT_SPECIAL
#define SIGN_DEFAULT_SPECIAL BB_SDM_TWINKLE
#endif
#ifndef SIGN_CLOCK_COLOUR
#define SIGN_CLOCK_COLOUR BB_COL_AMBER
#endif
#ifndef SIGN_CLOCK_POSITION
#define SIGN_CLOCK_POSITION BB_DP_TOPLINE
#endif
#ifndef SIGN_CLOCK_MODE
#define SIGN_CLOCK_MODE BB_DM_HOLD
#endif
#ifndef SIGN_CLOCK_SPECIAL
#define SIGN_CLOCK_SPECIAL BB_SDM_TWINKLE
#endif
#ifndef SIGN_INIT_STRING
#define SIGN_INIT_STRING "Starting..."
#endif
#ifndef SIGN_INIT_COLOUR
#define SIGN_INIT_COLOUR BB_COL_GREEN
#endif
#ifndef SIGN_INIT_POSITION
#define SIGN_INIT_POSITION BB_DP_TOPLINE
#endif
#ifndef SIGN_INIT_MODE
#define SIGN_INIT_MODE BB_DM_ROTATE
#endif
#ifndef SIGN_INIT_SPECIAL
#define SIGN_INIT_SPECIAL BB_SDM_TWINKLE
#endif

/**
 * @brief LED Sign control and management class
 * 
 * Manages all interactions with the BetaBrite LED sign including message
 * display, effects, priority handling, and system functions.
 */
class SignController {
private:
    BETABRITE* sign;                    ///< BetaBrite sign interface
    String device_id;                   ///< Unique device identifier
    
    // File management
    char current_file;                  ///< Current text file letter (A-E)
    int max_files;                      ///< Maximum number of files on sign
    
    // Priority message management
    bool in_priority_mode;              ///< Whether priority message is active
    unsigned long priority_start_time;  ///< When priority message started
    unsigned long priority_end_time;    ///< When priority message should end
    unsigned int priority_duration;     ///< Duration in seconds for priority message
    String priority_message_content;    ///< Stored content for priority message display

    // Priority message stages
    enum PriorityStage {
        PRIORITY_NONE,
        PRIORITY_WARNING,
        PRIORITY_MESSAGE
    };
    PriorityStage priority_stage;       ///< Current stage of priority message

    // Offline mode sequence management
    bool in_offline_mode;               ///< Whether offline mode sequence is active
    int offline_sequence_stage;         ///< Current stage in offline sequence (0-5)
    unsigned long offline_stage_start;  ///< When current offline stage started

    // Clock display management
    bool clock_enabled;                 ///< Whether clock display is enabled
    unsigned long clock_start_time;     ///< When clock was last displayed
    unsigned long clock_display_duration; ///< How long to show clock (ms)

    // Timing constants
    static const unsigned long PRIORITY_WARNING_DURATION = 2500;  ///< Priority warning display time (ms)
    static const unsigned long DEFAULT_PRIORITY_DURATION = 25;    ///< Default priority message duration (seconds)
    static const unsigned long CLOCK_DISPLAY_DURATION = 4000;     ///< Default clock display time (ms) - 4 seconds
    
    /**
     * @brief Display connection details when offline
     * Shows WiFi credentials and setup instructions on the sign
     */
    void displayOfflineDetails();
    
    /**
     * @brief Generate a random string for testing
     * @param length Length of string to generate
     * @return Random alphanumeric string
     */
    String generateRandomString(int length);
    
public:
    /**
     * @brief Constructor - initializes sign controller
     * @param sign_instance Pointer to BetaBrite sign instance
     * @param device_id Unique device identifier
     * @param max_files Maximum number of files to use (default: 5)
     */
    SignController(BETABRITE* sign_instance, const String& device_id, int max_files = 5);
    
    /**
     * @brief Initialize the LED sign with default settings
     * Sets up memory configuration and displays initial message
     * @return true if initialization successful, false otherwise
     */
    bool begin();
    
    /**
     * @brief Display a message with specified parameters
     * @param message Text content to display
     * @param color Color code for the message
     * @param position Position code for the message
     * @param mode Display mode/animation
     * @param special Special effect code
     * @param charset Character set code (default: 7high)
     * @param speed Speed code string (default: medium)
     * @return true if message sent successfully, false otherwise
     */
    bool displayMessage(const char* message, char color, char position, char mode, char special,
                       char charset = '3', const char* speed = "\027");
    
    /**
     * @brief Display a priority message that interrupts normal operation
     * Priority messages show a warning, then the message, then resume normal operation
     * @param message Priority message content
     * @param duration Duration in seconds to display message (default: 25 seconds)
     * @return true if priority message initiated, false otherwise
     */
    bool displayPriorityMessage(const char* message, unsigned int duration = DEFAULT_PRIORITY_DURATION);
    
    /**
     * @brief Clear all text files on the sign
     * Wipes all stored messages and resets file counter
     */
    void clearAllFiles();
    
    /**
     * @brief Display current time on the sign
     * Shows formatted time with AM/PM or 24-hour format
     * @param military_time Whether to use 24-hour format (default: false)
     */
    void displayClock(bool military_time = false);
    
    /**
     * @brief Cancel any currently displayed priority message
     * Returns sign to normal operation
     */
    void cancelPriorityMessage();

    /**
     * @brief Check and handle priority message timeout
     * Called from main loop to manage non-blocking priority message transitions
     */
    void checkPriorityTimeout();

    /**
     * @brief Check and handle offline mode sequence progression
     * Called from main loop to manage non-blocking offline mode display
     */
    void checkOfflineTimeout();
    
    /**
     * @brief Handle system commands (clear, factory reset)
     * @param command Command character (# for clear, ^ for factory reset)
     * @return true if command was handled, false if unknown
     */
    bool handleSystemCommand(char command);
    
    /**
     * @brief Display offline connection instructions
     * Shows WiFi credentials and setup information when device is offline
     */
    void showOfflineMode();

    /**
     * @brief Cancel offline mode sequence
     * Stops the offline mode display and returns to normal operation
     */
    void cancelOfflineMode();

    /**
     * @brief Display error message on sign
     * Shows error message with red color and flash mode for visibility
     * @param error_message Error message to display
     * @param duration_seconds How long to show error (default: 5 seconds)
     */
    void displayError(const char* error_message, unsigned int duration_seconds = 5);
    
    /**
     * @brief Run demonstration of all sign capabilities
     * Cycles through colors, modes, and special effects for testing
     */
    void runDemo();
    
    /**
     * @brief Main loop function - call regularly to manage timers
     * Handles clock display timing and priority message timeouts
     */
    void loop();
    
    /**
     * @brief Check if currently displaying a priority message
     * @return true if in priority mode, false otherwise
     */
    bool isInPriorityMode() const;
    
    /**
     * @brief Get current file letter being used
     * @return Current file character (A-E)
     */
    char getCurrentFile() const;
    
    /**
     * @brief Get maximum number of files configured
     * @return Maximum file count
     */
    int getMaxFiles() const;
    
    /**
     * @brief Enable or disable automatic clock display
     * @param enabled Whether clock should be automatically displayed
     * @param duration How long to show clock in milliseconds
     */
    void setClockEnabled(bool enabled, unsigned long duration = CLOCK_DISPLAY_DURATION);
    
    /**
     * @brief Get formatted date/time string
     * @param military_time Whether to use 24-hour format
     * @return Formatted date/time string
     */
    String getFormattedDateTime(bool military_time = false);
    
    /**
     * @brief Get sign health status
     * @return Status string indicating sign health
     */
    String getStatus() const;
    
    /**
     * @brief Set sign memory configuration
     * @param start_file Starting file letter
     * @param num_files Number of files to configure
     * @return true if configuration successful, false otherwise
     */
    bool configureMemory(char start_file = 'A', int num_files = 5);
};

#endif // SIGN_CONTROLLER_H