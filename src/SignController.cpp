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

bool SignController::displayMessage(const char* message, char color, char position, char mode, char special) {
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
    Serial.println(special, HEX);
    
    // Send message to sign
    sign->WriteTextFile(current_file, message, color, position, mode, special);
    
    // Advance to next file
    current_file++;
    
    // Wrap around if we've used all files
    if (current_file > 'A' + max_files - 1) {
        current_file = 'A';
        Serial.println("SignController: File counter wrapped to A");
    }
    
    return true;
}

bool SignController::displayPriorityMessage(const char* message) {
    if (!sign || !message) {
        Serial.println("SignController: Invalid parameters for displayPriorityMessage");
        return false;
    }
    
    Serial.println("SignController: ### PRIORITY MESSAGE ###");
    Serial.print("SignController: Content: ");
    Serial.println(message);
    
    in_priority_mode = true;
    priority_start_time = millis();
    
    // Step 1: Display priority warning
    Serial.println("SignController: Displaying priority warning");
    sign->CancelPriorityTextFile();
    sign->WritePriorityTextFile(
        "# # # #", 
        BB_COL_RED, 
        BB_DP_TOPLINE, 
        BB_DM_FLASH, 
        BB_SDM_TWINKLE
    );
    
    // Wait for warning duration (non-blocking delay would be better)
    delay(PRIORITY_WARNING_DURATION);
    
    // Step 2: Display actual priority message
    Serial.println("SignController: Displaying priority message content");
    sign->CancelPriorityTextFile();
    sign->WritePriorityTextFile(
        message, 
        BB_COL_AUTOCOLOR, 
        BB_DP_TOPLINE, 
        BB_DM_ROTATE, 
        BB_SDM_TWINKLE
    );
    
    // Wait for message duration
    delay(PRIORITY_MESSAGE_DURATION);
    
    // Step 3: Cancel priority message and return to normal operation
    Serial.println("SignController: Priority message complete, returning to normal operation");
    sign->CancelPriorityTextFile();
    in_priority_mode = false;
    
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
    
    Serial.println("SignController: Displaying offline connection details");
    displayOfflineDetails();
}

void SignController::displayOfflineDetails() {
    if (!sign) {
        return;
    }
    
    // Cancel any existing priority messages
    sign->CancelPriorityTextFile();
    
    // Define display parameters
    char color = BB_COL_AUTOCOLOR;
    char position = BB_DP_TOPLINE;
    char mode = BB_DM_HOLD;
    char special = BB_SDM_TWINKLE;
    
    // Sequence of messages to display
    struct OfflineMessage {
        const char* text;
        char color;
        char mode;
        unsigned long duration;
    };
    
    OfflineMessage messages[] = {
        {"*Offline*", BB_COL_RED, BB_DM_EXPLODE, 5000},
        {"Connect to:", BB_COL_GREEN, BB_DM_HOLD, 1500},
        {"LEDSign", BB_COL_ORANGE, BB_DM_HOLD, 5000},
        {"Password", BB_COL_GREEN, BB_DM_HOLD, 1500},
        {"ledsign0", BB_COL_ORANGE, BB_DM_HOLD, 5000},
        {"", BB_COL_AUTOCOLOR, BB_DM_SPECIAL, 3500} // Thank you special effect
    };
    
    for (size_t i = 0; i < sizeof(messages) / sizeof(messages[0]); i++) {
        sign->CancelPriorityTextFile();
        
        if (i == sizeof(messages) / sizeof(messages[0]) - 1) {
            // Last message - thank you special effect
            sign->WritePriorityTextFile(messages[i].text, messages[i].color, position, messages[i].mode, BB_SDM_THANKYOU);
        } else {
            sign->WritePriorityTextFile(messages[i].text, messages[i].color, position, messages[i].mode, special);
        }
        
        delay(messages[i].duration);
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
    
    // Handle clock display timeout
    if (clock_start_time > 0 && !in_priority_mode) {
        if (current_time - clock_start_time > clock_display_duration) {
            sign->CancelPriorityTextFile();
            clock_start_time = 0;
        }
    }
    
    // Handle priority message timeout (safety net)
    if (in_priority_mode && priority_start_time > 0) {
        unsigned long total_priority_time = PRIORITY_WARNING_DURATION + PRIORITY_MESSAGE_DURATION + 1000; // +1s buffer
        if (current_time - priority_start_time > total_priority_time) {
            Serial.println("SignController: Priority message timeout - forcing cancel");
            cancelPriorityMessage();
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