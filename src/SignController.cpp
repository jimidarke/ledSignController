/**
 * @file SignController.cpp
 * @brief Implementation of LED Sign control and management
 * 
 * Contains the implementation of all sign control functionality including
 * message display, priority handling, clock management, and system commands.
 */

#include "SignController.h"
#include <time.h>

SignController::SignController(BETABRITE* sign_instance, const String& device_id, int max_files)
    : sign(sign_instance), device_id(device_id), max_files(max_files) {

    // Initialize state variables
    current_file = 'A';
    in_priority_mode = false;
    priority_start_time = 0;
    priority_end_time = 0;
    priority_duration = DEFAULT_PRIORITY_DURATION;
    priority_stage = PRIORITY_NONE;
    in_offline_mode = false;
    offline_sequence_stage = 0;
    offline_stage_start = 0;
    clock_enabled = true;
    clock_start_time = 0;
    clock_display_duration = CLOCK_DISPLAY_DURATION;

    Serial.println("SignController: Initialized");
}

bool SignController::begin() {
    if (!sign) {
        Serial.println("SignController: Error - No sign instance provided");
        return false;
    }
    
    Serial.println("SignController: Initializing LED sign via TTL-RS232 connection");
    Serial.print("SignController: Device ID: ");
    Serial.println(device_id);
    
    // Configure sign memory
    if (!configureMemory()) {
        Serial.println("SignController: Warning - Memory configuration failed");
    }
    
    // Display initial message
    Serial.println("SignController: Displaying initial message");
    sign->WritePriorityTextFile(
        SIGN_INIT_STRING, 
        SIGN_INIT_COLOUR, 
        SIGN_INIT_POSITION, 
        SIGN_INIT_MODE, 
        SIGN_INIT_SPECIAL
    );
    
    // Reset state
    in_priority_mode = false;
    clock_start_time = 0;
    
    Serial.println("SignController: Initialization complete");
    return true;
}

bool SignController::configureMemory(char start_file, int num_files) {
    if (!sign) {
        return false;
    }
    
    current_file = start_file;
    max_files = num_files;
    
    Serial.print("SignController: Configuring memory - Start: ");
    Serial.print(start_file);
    Serial.print(", Files: ");
    Serial.println(num_files);
    
    sign->SetMemoryConfiguration(start_file, num_files);
    delay(500); // Give sign time to process configuration
    Serial.println("SignController: Memory configuration complete");
    return true;
}

bool SignController::displayMessage(const char* message, char color, char position, char mode, char special,
                                   char charset, const char* speed) {
    if (!sign || !message) {
        Serial.println("SignController: Invalid parameters for displayMessage");
        return false;
    }

    // Don't allow normal messages during priority mode
    if (in_priority_mode) {
        Serial.println("SignController: Ignoring message - in priority mode");
        return false;
    }

    Serial.print("SignController: Displaying message on file ");
    Serial.print(current_file);
    Serial.print(": ");
    Serial.println(message);

    // Log display parameters
    Serial.print("  Color: 0x");
    Serial.print(color, HEX);
    Serial.print(", Position: 0x");
    Serial.print(position, HEX);
    Serial.print(", Mode: 0x");
    Serial.print(mode, HEX);
    Serial.print(", Special: 0x");
    Serial.print(special, HEX);
    Serial.print(", Charset: '");
    Serial.print(charset);
    Serial.print("', Speed: 0x");
    for (size_t i = 0; i < strlen(speed); i++) {
        Serial.print((int)(speed[i]), HEX);
    }
    Serial.println();

    // Build formatted message with charset and speed codes
    // Format: \032<charset><speed><message>
    // \032 = BB_FC_SELECTCHARSET
    String formatted_message = "";
    formatted_message += '\032';  // BB_FC_SELECTCHARSET
    formatted_message += charset;
    formatted_message += speed;   // Speed code string
    formatted_message += message;

    // Send message to sign
    sign->WriteTextFile(current_file, formatted_message.c_str(), color, position, mode, special);

    // Advance to next file
    current_file++;

    // Wrap around if we've used all files
    if (current_file > 'A' + max_files - 1) {
        current_file = 'A';
        Serial.println("SignController: File counter wrapped to A");
    }

    return true;
}

bool SignController::displayPriorityMessage(const char* message, unsigned int duration) {
    if (!sign || !message) {
        Serial.println("SignController: Invalid parameters for displayPriorityMessage");
        return false;
    }

    Serial.println("SignController: ### PRIORITY MESSAGE ###");
    Serial.print("SignController: Content: ");
    Serial.println(message);
    Serial.print("SignController: Duration: ");
    Serial.print(duration);
    Serial.println(" seconds");

    // Store message for later display stages
    priority_message_content = String(message);

    // Initialize priority mode
    in_priority_mode = true;
    priority_start_time = millis();
    priority_duration = duration;
    priority_stage = PRIORITY_WARNING;

    // Calculate when priority should end: warning duration + message duration
    priority_end_time = priority_start_time + PRIORITY_WARNING_DURATION + (duration * 1000UL);

    // Display priority warning (stage 1)
    Serial.println("SignController: Displaying priority warning (non-blocking)");
    sign->CancelPriorityTextFile();
    sign->WritePriorityTextFile(
        "# # # #",
        BB_COL_RED,
        BB_DP_TOPLINE,
        BB_DM_FLASH,
        BB_SDM_TWINKLE
    );

    // Stage transitions will be handled by checkPriorityTimeout() called from main loop
    return true;
}

void SignController::clearAllFiles() {
    if (!sign) {
        Serial.println("SignController: Cannot clear files - no sign instance");
        return;
    }
    
    Serial.println("SignController: Clearing all text files");
    
    // Clear each file from A to max_files
    for (char file = 'A'; file < 'A' + max_files; file++) {
        Serial.print("SignController: Clearing file ");
        Serial.println(file);
        
        sign->WriteTextFile(
            file, 
            "", 
            SIGN_DEFAULT_COLOUR, 
            SIGN_DEFAULT_POSITION, 
            SIGN_DEFAULT_MODE, 
            SIGN_DEFAULT_SPECIAL
        );
    }
    
    // Reset file counter
    current_file = 'A';
    Serial.println("SignController: All files cleared, file counter reset");
}

String SignController::getFormattedDateTime(bool military_time) {
    time_t now = time(nullptr);
    struct tm *timeinfo = localtime(&now);
    
    if (!timeinfo) {
        return "Time Error";
    }
    
    char buffer[80];
    if (military_time) {
        strftime(buffer, sizeof(buffer), "%m/%d %H:%M", timeinfo);
    } else {
        strftime(buffer, sizeof(buffer), "%m/%d %I:%M %p", timeinfo);
    }
    
    return String(buffer);
}

void SignController::displayClock(bool military_time) {
    if (!sign) {
        return;
    }
    
    String time_str = getFormattedDateTime(military_time);
    Serial.print("SignController: Displaying clock: ");
    Serial.println(time_str);
    
    // Only display clock if not in priority mode
    if (!in_priority_mode) {
        sign->CancelPriorityTextFile();
        sign->WritePriorityTextFile(
            time_str.c_str(), 
            SIGN_CLOCK_COLOUR, 
            SIGN_CLOCK_POSITION, 
            SIGN_CLOCK_MODE, 
            SIGN_CLOCK_SPECIAL
        );
        clock_start_time = millis();
    }
}

void SignController::cancelPriorityMessage() {
    if (sign && in_priority_mode) {
        Serial.println("SignController: Canceling priority message");
        sign->CancelPriorityTextFile();
        in_priority_mode = false;
        priority_start_time = 0;
        priority_end_time = 0;
        priority_stage = PRIORITY_NONE;
        priority_message_content = "";
    }
}

void SignController::checkPriorityTimeout() {
    if (!sign || !in_priority_mode) {
        return;
    }

    unsigned long current_time = millis();

    // Handle stage transitions
    switch (priority_stage) {
        case PRIORITY_WARNING:
            // Check if warning duration has elapsed
            if (current_time - priority_start_time >= PRIORITY_WARNING_DURATION) {
                // Transition to message stage
                Serial.println("SignController: Transitioning to priority message display");
                priority_stage = PRIORITY_MESSAGE;

                // Display the actual priority message
                sign->CancelPriorityTextFile();
                sign->WritePriorityTextFile(
                    priority_message_content.c_str(),
                    BB_COL_AUTOCOLOR,
                    BB_DP_TOPLINE,
                    BB_DM_ROTATE,
                    BB_SDM_TWINKLE
                );
            }
            break;

        case PRIORITY_MESSAGE:
            // Check if total priority time has elapsed
            if (current_time >= priority_end_time) {
                // Priority message duration complete - return to normal operation
                Serial.println("SignController: Priority message duration complete, returning to normal operation");
                cancelPriorityMessage();
            }
            break;

        case PRIORITY_NONE:
        default:
            // Should not reach here, but handle gracefully
            break;
    }
}

bool SignController::handleSystemCommand(char command) {
    switch (command) {
        case '#':
            Serial.println("SignController: System command - Clear all files");
            clearAllFiles();
            begin(); // Reinitialize
            return true;
            
        case '^':
            Serial.println("SignController: System command - Factory reset requested");
            clearAllFiles();
            // Note: Actual factory reset (WiFi config clear) should be handled by main application
            return true;
            
        default:
            Serial.print("SignController: Unknown system command: ");
            Serial.println(command);
            return false;
    }
}

void SignController::showOfflineMode() {
    if (!sign) {
        return;
    }

    // Start non-blocking offline mode sequence if not already running
    if (!in_offline_mode) {
        Serial.println("SignController: Starting offline mode sequence (non-blocking)");
        in_offline_mode = true;
        offline_sequence_stage = 0;
        offline_stage_start = millis();

        // Display first stage immediately
        sign->CancelPriorityTextFile();
        sign->WritePriorityTextFile("*Offline*", BB_COL_RED, BB_DP_TOPLINE, BB_DM_EXPLODE, BB_SDM_TWINKLE);
    }
}

void SignController::cancelOfflineMode() {
    if (in_offline_mode) {
        Serial.println("SignController: Canceling offline mode sequence");
        in_offline_mode = false;
        offline_sequence_stage = 0;
        offline_stage_start = 0;

        // Clear the display
        if (sign) {
            sign->CancelPriorityTextFile();
        }
    }
}

void SignController::displayError(const char* error_message, unsigned int duration_seconds) {
    if (!sign || !error_message) {
        return;
    }

    Serial.print("SignController: Displaying error message: ");
    Serial.println(error_message);

    // Display error as a priority message (non-blocking)
    // Use red color, flash mode for high visibility
    displayPriorityMessage(error_message, duration_seconds);
}

void SignController::checkOfflineTimeout() {
    if (!sign || !in_offline_mode) {
        return;
    }

    unsigned long current_time = millis();

    // Define offline message sequence
    struct OfflineStage {
        const char* text;
        char color;
        char mode;
        char special;
        unsigned long duration;
    };

    const OfflineStage stages[] = {
        {"*Offline*", BB_COL_RED, BB_DM_EXPLODE, BB_SDM_TWINKLE, 5000},
        {"Connect to:", BB_COL_GREEN, BB_DM_HOLD, BB_SDM_TWINKLE, 1500},
        {"LEDSign", BB_COL_ORANGE, BB_DM_HOLD, BB_SDM_TWINKLE, 5000},
        {"Password", BB_COL_GREEN, BB_DM_HOLD, BB_SDM_TWINKLE, 1500},
        {"ledsign0", BB_COL_ORANGE, BB_DM_HOLD, BB_SDM_TWINKLE, 5000},
        {"", BB_COL_AUTOCOLOR, BB_DM_SPECIAL, BB_SDM_THANKYOU, 3500}
    };

    const int num_stages = sizeof(stages) / sizeof(stages[0]);

    // Check if current stage duration has elapsed
    if (current_time - offline_stage_start >= stages[offline_sequence_stage].duration) {
        // Move to next stage
        offline_sequence_stage++;

        if (offline_sequence_stage >= num_stages) {
            // Sequence complete - restart from beginning
            offline_sequence_stage = 0;
        }

        // Display current stage
        const OfflineStage& stage = stages[offline_sequence_stage];
        sign->CancelPriorityTextFile();
        sign->WritePriorityTextFile(stage.text, stage.color, BB_DP_TOPLINE, stage.mode, stage.special);

        offline_stage_start = current_time;

        Serial.print("SignController: Offline mode stage ");
        Serial.print(offline_sequence_stage);
        Serial.print(": ");
        Serial.println(stage.text);
    }
}

String SignController::generateRandomString(int length) {
    const char characters[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    const int char_count = sizeof(characters) - 1;
    String result = "";
    
    for (int i = 0; i < length; i++) {
        result += characters[random(0, char_count)];
    }
    
    return result;
}

void SignController::runDemo() {
    if (!sign) {
        Serial.println("SignController: Cannot run demo - no sign instance");
        return;
    }
    
    Serial.println("SignController: Running sign capabilities demo");
    
    // Demo special effects
    const char specials[] = {
        BB_SDM_TWINKLE, BB_SDM_SPARKLE, BB_SDM_SNOW, BB_SDM_INTERLOCK,
        BB_SDM_SWITCH, BB_SDM_SLIDE, BB_SDM_SPRAY, BB_SDM_STARBURST,
        BB_SDM_WELCOME, BB_SDM_SLOTS, BB_SDM_NEWSFLASH, BB_SDM_TRUMPET,
        BB_SDM_CYCLECOLORS, BB_SDM_THANKYOU, BB_SDM_NOSMOKING,
        BB_SDM_DONTDRINKANDDRIVE, BB_SDM_FISHIMAL, BB_SDM_FIREWORKS,
        BB_SDM_TURBALLOON, BB_SDM_BOMB
    };
    
    char default_color = BB_COL_AUTOCOLOR;
    char default_position = BB_DP_TOPLINE;
    char default_mode = BB_DM_ROTATE;
    
    for (size_t i = 0; i < sizeof(specials) / sizeof(specials[0]); i++) {
        String demo_message = "Demo " + generateRandomString(4);
        
        Serial.print("SignController: Demo effect 0x");
        Serial.print(specials[i], HEX);
        Serial.print(" - ");
        Serial.println(demo_message);
        
        sign->CancelPriorityTextFile();
        sign->WritePriorityTextFile(demo_message.c_str(), default_color, default_position, BB_DM_SPECIAL, specials[i]);
        
        delay(3000); // Show each effect for 3 seconds
    }
    
    Serial.println("SignController: Demo complete");
}

void SignController::loop() {
    if (!sign) {
        return;
    }

    unsigned long current_time = millis();

    // Handle priority message stage transitions and timeout
    checkPriorityTimeout();

    // Handle offline mode sequence progression
    checkOfflineTimeout();

    // Handle clock display timeout (only when not in priority mode)
    if (clock_start_time > 0 && !in_priority_mode) {
        if (current_time - clock_start_time > clock_display_duration) {
            sign->CancelPriorityTextFile();
            clock_start_time = 0;
        }
    }
}

bool SignController::isInPriorityMode() const {
    return in_priority_mode;
}

char SignController::getCurrentFile() const {
    return current_file;
}

int SignController::getMaxFiles() const {
    return max_files;
}

void SignController::setClockEnabled(bool enabled, unsigned long duration) {
    clock_enabled = enabled;
    clock_display_duration = duration;
    
    Serial.print("SignController: Clock ");
    Serial.print(enabled ? "enabled" : "disabled");
    if (enabled) {
        Serial.print(" (duration: ");
        Serial.print(duration / 1000);
        Serial.println(" seconds)");
    } else {
        Serial.println();
    }
}

String SignController::getStatus() const {
    String status = "SignController Status:\n";
    status += "  Current File: " + String(current_file) + "\n";
    status += "  Max Files: " + String(max_files) + "\n";
    status += "  Priority Mode: " + String(in_priority_mode ? "Yes" : "No") + "\n";
    status += "  Clock Enabled: " + String(clock_enabled ? "Yes" : "No") + "\n";
    
    if (clock_start_time > 0) {
        unsigned long remaining = clock_display_duration - (millis() - clock_start_time);
        status += "  Clock Remaining: " + String(remaining / 1000) + "s\n";
    }
    
    return status;
}