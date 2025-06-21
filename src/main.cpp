/**
 * @file main.cpp
 * @brief LED Sign Controller - Main Application Entry Point
 * 
 * ESP32-based LED sign controller that connects BetaBrite/Alpha Protocol LED signs
 * to WiFi and MQTT for remote message control. This refactored version uses a modular
 * architecture for better maintainability and security.
 * 
 * Key Features:
 * - Secure MQTT communication with exponential backoff
 * - Rich message parsing with colors, animations, and effects
 * - Priority message handling for urgent communications
 * - Automatic clock display with NTP synchronization
 * - Robust error handling and input validation
 * - Comprehensive telemetry and health monitoring
 * - Modular architecture for easy maintenance
 * 
 * Hardware Requirements:
 * - ESP32 development board
 * - BetaBrite LED sign with RS232/TTL interface
 * - Serial connection: RX pin 16, TX pin 17
 * 
 * @author LED Sign Controller Project
 * @version 0.1.4
 * @date 2024
 */

#include "defines.h"
#include <SPI.h>
#include <WiFi.h>
#include <time.h>

// Project modules
#include "MessageParser.h"
#include "MQTTManager.h"
#include "SignController.h"
// #include "SecureOTA.h"  // TODO: Implement SecureOTA.cpp

// Third-party libraries
#include <ArduinoJson.h>
#include "OTAupdate.h" // Legacy OTA - will be replaced with SecureOTA

/**
 * @brief Application version and build information
 */
const char* APP_VERSION = "0.1.4";
const char* BUILD_DATE = __DATE__ " " __TIME__;

/**
 * @brief WiFi and network configuration
 */
char wifi_ssid[32] = SIGN_DEFAULT_SSID;
char wifi_pass[32] = SIGN_DEFAULT_PASS;

/**
 * @brief MQTT configuration parameters
 */
char mqtt_server[40] = "";
uint16_t mqtt_port = 1883;
char mqtt_user[32] = "";
char mqtt_pass[32] = "";

/**
 * @brief Time zone configuration for clock display
 * POSIX time zone string for Mountain Time with DST
 */
const char* timezone_posix = SIGN_TIMEZONE_POSIX;
const char* ntp_server = "pool.ntp.org";

/**
 * @brief OTA update server configuration
 * TODO: Replace with secure HTTPS endpoints
 */
const char* ota_version_url = "http://docker02.darketech.ca:8003/version.txt";
const char* ota_firmware_url = "http://docker02.darketech.ca:8003/firmware.bin";

/**
 * @brief Global object instances
 */
WiFiClient wifi_client;                          ///< WiFi client for network operations
ESP_WiFiManager_Lite* wifi_manager = nullptr;   ///< WiFi configuration manager
BETABRITE led_sign(1, 16, 17);                 ///< BetaBrite sign interface (ID=1, RX=16, TX=17)
MQTTManager* mqtt_manager = nullptr;             ///< MQTT connection manager
SignController* sign_controller = nullptr;       ///< LED sign control interface
// SecureOTA* ota_manager = nullptr;               ///< Secure OTA update manager

/**
 * @brief Application state variables
 */
String device_id;                               ///< Unique device identifier (from MAC)
bool services_initialized = false;             ///< Whether network services are ready
unsigned long last_health_check = 0;           ///< Last system health check timestamp
unsigned long last_time_sync = 0;              ///< Last NTP time synchronization

/**
 * @brief System health monitoring interval (30 seconds)
 */
const unsigned long HEALTH_CHECK_INTERVAL = 30000;

/**
 * @brief Time synchronization interval (1 hour)
 */
const unsigned long TIME_SYNC_INTERVAL = 3600000;

/**
 * @brief WiFi connection monitoring intervals
 */
const unsigned long WIFI_CHECK_INTERVAL = 30000;
const unsigned long MEMORY_REPORT_INTERVAL = 60000;

/**
 * @brief Custom CSS styling for WiFi configuration portal
 */
#if USING_CUSTOMS_STYLE
const char NewCustomsStyle[] PROGMEM = 
    "<style>"
    "div,input{padding:5px;font-size:1em;}"
    "input{width:95%;}"
    "body{text-align: center;font-family: Arial, sans-serif;}"
    "button{background-color:#007bff;color:white;line-height:2.4rem;font-size:1.2rem;width:100%;border:none;border-radius:4px;}"
    "fieldset{border-radius:0.3rem;margin:0px;border: 1px solid #ddd;}"
    ".header{background-color:#343a40;color:white;padding:10px;margin-bottom:20px;}"
    "</style>";
#endif

/**
 * @brief Function declarations
 */
void initializeDevice();
void initializeNetworkServices();
void handleMQTTMessage(char* topic, uint8_t* payload, unsigned int length);
void performHealthCheck();
void syncTime();
void smartDelay(unsigned long delay_ms);
void printSystemInfo();
void handleSystemReset();

/**
 * @brief Arduino setup function - runs once at startup
 * 
 * Initializes all hardware and software components:
 * - Serial communication
 * - LED sign interface
 * - WiFi management
 * - Device identification
 */
void setup() {
    // Initialize serial communication
    Serial.begin(115200);
    while (!Serial && millis() < 5000) {
        delay(10); // Wait up to 5 seconds for serial to be ready
    }
    
    Serial.println();
    Serial.println("========================================");
    Serial.println("LED Sign Controller Starting Up");
    Serial.println("Darke Tech Corp. 2024");
    Serial.println("========================================");
    Serial.print("Version: ");
    Serial.println(APP_VERSION);
    Serial.print("Build: ");
    Serial.println(BUILD_DATE);
    Serial.println();
    
    // Initialize device and hardware
    initializeDevice();
    
    // Print system information
    printSystemInfo();
    
    Serial.println("System initialization complete");
    Serial.println("Entering main loop...");
    Serial.println();
}

/**
 * @brief Arduino main loop - runs continuously
 * 
 * Handles the main application logic:
 * - WiFi connection management
 * - Network service initialization
 * - System health monitoring
 * - Service orchestration
 */
void loop() {
    static unsigned long last_wifi_check = 0;
    static unsigned long last_memory_report = 0;
    unsigned long current_time = millis();
    
    // Always run WiFi manager
    if (wifi_manager) {
        wifi_manager->run();
    }
    
    // Periodic WiFi status monitoring
    if (current_time - last_wifi_check > WIFI_CHECK_INTERVAL) {
        Serial.print("WiFi Status: ");
        Serial.print(WiFi.status());
        if (WiFi.status() == WL_CONNECTED) {
            Serial.print(" (Connected to ");
            Serial.print(WiFi.SSID());
            Serial.print(", RSSI: ");
            Serial.print(WiFi.RSSI());
            Serial.print(", IP: ");
            Serial.print(WiFi.localIP().toString());
            Serial.println(")");
        } else {
            Serial.println(" (Disconnected)");
        }
        last_wifi_check = current_time;
    }
    
    // Periodic memory usage reporting
    if (current_time - last_memory_report > MEMORY_REPORT_INTERVAL) {
        Serial.print("Memory - Free: ");
        Serial.print(ESP.getFreeHeap());
        Serial.print(" bytes, Min Free: ");
        Serial.print(ESP.getMinFreeHeap());
        Serial.print(" bytes, Heap Size: ");
        Serial.println(ESP.getHeapSize());
        last_memory_report = current_time;
    }
    
    // Handle WiFi connection state
    if (WiFi.status() == WL_CONNECTED) {
        // Initialize network services on first connection
        if (!services_initialized) {
            Serial.println("WiFi connected - initializing network services");
            initializeNetworkServices();
        }
        
        // Run network services if initialized
        if (services_initialized) {
            // Handle MQTT communication
            if (mqtt_manager) {
                mqtt_manager->loop();
            }
            
            // Handle OTA updates
            // TODO: Implement SecureOTA
            // if (ota_manager) {
            //     ota_manager->loop();
            // }
            
            // Perform periodic health checks
            if (current_time - last_health_check > HEALTH_CHECK_INTERVAL) {
                performHealthCheck();
                last_health_check = current_time;
            }
            
            // Periodic time synchronization
            if (current_time - last_time_sync > TIME_SYNC_INTERVAL) {
                syncTime();
                last_time_sync = current_time;
            }
        }
        
        // Short delay for online operation
        smartDelay(100);
    } else {
        // WiFi disconnected - show offline information
        services_initialized = false;
        
        if (sign_controller) {
            sign_controller->showOfflineMode();
        }
        
        Serial.println("WiFi disconnected - displaying offline information");
    }
    
    // Always run sign controller loop for timing management
    if (sign_controller) {
        sign_controller->loop();
    }
}

/**
 * @brief Initialize device hardware and core components
 * 
 * Sets up the LED sign, WiFi manager, and device identification.
 * This function handles the fundamental device initialization that
 * must occur regardless of network connectivity.
 */
void initializeDevice() {
    Serial.println("Initializing device hardware...");
    
    // Generate unique device ID from MAC address
    String mac = WiFi.macAddress();
    mac.replace(":", "");
    device_id = mac;
    Serial.print("Device ID: ");
    Serial.println(device_id);
    
    // Initialize LED sign controller
    sign_controller = new SignController(&led_sign, device_id);
    if (!sign_controller->begin()) {
        Serial.println("Warning: LED sign initialization failed");
        // Continue anyway - sign might be temporarily disconnected
    }
    
    // Initialize WiFi manager
    Serial.println("Initializing WiFi manager...");
    wifi_manager = new ESP_WiFiManager_Lite();
    
    if (wifi_manager) {
        // Configure WiFi manager settings
        wifi_manager->setConfigPortalChannel(0);
        wifi_manager->setConfigPortalIP(IPAddress(192, 168, 50, 1));
        wifi_manager->setConfigPortal("LEDSign", "ledsign0");
        
        #if USING_CUSTOMS_STYLE
        wifi_manager->setCustomsStyle(NewCustomsStyle);
        #endif
        
        #if USING_CUSTOMS_HEAD_ELEMENT
        wifi_manager->setCustomsHeadElement(PSTR("<style>html{filter: invert(10%);}</style>"));
        #endif
        
        // Start WiFi manager
        wifi_manager->begin("LEDSign");
        Serial.println("WiFi manager initialized successfully");
    } else {
        Serial.println("Error: Failed to create WiFi manager");
    }
    
    // Initialize random number generator
    randomSeed(analogRead(0) + millis());
    
    Serial.println("Device hardware initialization complete");
}

/**
 * @brief Initialize network-dependent services
 * 
 * Sets up MQTT, OTA, and time synchronization services that require
 * an active WiFi connection. Called once when WiFi connects.
 */
void initializeNetworkServices() {
    Serial.println("Initializing network services...");
    
    try {
        // Initialize time synchronization first
        Serial.println("Configuring NTP time synchronization...");
        configTzTime(timezone_posix, ntp_server);
        delay(2000); // Give NTP time to sync
        
        struct tm timeinfo;
        if (getLocalTime(&timeinfo)) {
            Serial.print("Current time: ");
            Serial.print(asctime(&timeinfo));
            last_time_sync = millis();
        } else {
            Serial.println("Warning: NTP synchronization failed");
        }
        
        // Initialize MQTT manager
        Serial.println("Initializing MQTT manager...");
        mqtt_manager = new MQTTManager(&wifi_client, device_id);
        
        if (mqtt_manager) {
            // Configure MQTT from stored parameters
            if (strlen(myMenuItems[0].pdata) > 0) {
                // Extract MQTT configuration from WiFiManager data
                strcpy(mqtt_server, myMenuItems[0].pdata);
                mqtt_port = atoi(myMenuItems[1].pdata);
                strcpy(mqtt_user, myMenuItems[2].pdata);
                strcpy(mqtt_pass, myMenuItems[3].pdata);
                
                // Configure MQTT manager
                if (mqtt_manager->configure(mqtt_server, mqtt_port, mqtt_user, mqtt_pass)) {
                    mqtt_manager->setMessageCallback(handleMQTTMessage);
                    
                    if (mqtt_manager->begin()) {
                        Serial.println("MQTT manager initialized successfully");
                    } else {
                        Serial.println("Warning: MQTT manager initialization failed");
                    }
                } else {
                    Serial.println("Warning: MQTT configuration invalid");
                }
            } else {
                Serial.println("Info: MQTT not configured - check WiFi portal");
            }
        }
        
        // TODO: Initialize secure OTA manager when implementation is complete
        // Serial.println("Initializing OTA update manager...");
        // ota_manager = new SecureOTA(APP_VERSION, device_id);
        
        // Legacy OTA check (will be removed when SecureOTA is complete)
        Serial.println("Performing legacy OTA check...");
        checkForUpdates(APP_VERSION, ota_version_url, ota_firmware_url);
        
        services_initialized = true;
        Serial.println("All network services initialized successfully");
        
    } catch (const std::exception& e) {
        Serial.print("Error during service initialization: ");
        Serial.println(e.what());
        services_initialized = false;
    } catch (...) {
        Serial.println("Unknown error during service initialization");
        services_initialized = false;
    }
}

/**
 * @brief Handle incoming MQTT messages
 * 
 * Processes MQTT messages received from subscribed topics, validates them,
 * and routes them to appropriate handlers (message display, system commands).
 * 
 * @param topic MQTT topic the message was received on
 * @param payload Message payload bytes
 * @param length Length of payload in bytes
 */
void handleMQTTMessage(char* topic, uint8_t* payload, unsigned int length) {
    // Validate input parameters
    if (!topic || !payload || length == 0) {
        Serial.println("MQTT: Invalid message parameters");
        return;
    }
    
    // Log received message
    Serial.print("MQTT Message [");
    Serial.print(topic);
    Serial.print("]: ");
    
    // Convert payload to string
    String message;
    message.reserve(length + 1);
    for (unsigned int i = 0; i < length; i++) {
        message += (char)payload[i];
    }
    Serial.println(message);
    
    // Validate message using MessageParser
    if (!MessageParser::validateMessage(message.c_str())) {
        Serial.println("MQTT: Message validation failed");
        return;
    }
    
    // Handle system commands
    if (MessageParser::isSystemCommand(message.c_str())) {
        if (sign_controller) {
            char command = message.charAt(0);
            if (sign_controller->handleSystemCommand(command)) {
                Serial.print("MQTT: System command ");
                Serial.print(command);
                Serial.println(" executed");
                
                // Handle factory reset command
                if (command == '^') {
                    handleSystemReset();
                }
            }
        }
        return;
    }
    
    // Handle priority messages
    if (MessageParser::isPriorityMessage(message.c_str())) {
        if (sign_controller) {
            String priority_content = message.substring(1); // Remove * prefix
            sign_controller->displayPriorityMessage(priority_content.c_str());
        }
        return;
    }
    
    // Handle normal messages with option parsing
    char color, position, mode, special;
    String message_content;
    
    if (MessageParser::parseMessage(message.c_str(), &color, &position, &mode, &special, &message_content)) {
        if (sign_controller) {
            sign_controller->displayMessage(message_content.c_str(), color, position, mode, special);
        }
    } else {
        Serial.println("MQTT: Message parsing failed");
    }
}

/**
 * @brief Perform system health checks
 * 
 * Monitors system health including memory usage, WiFi signal strength,
 * MQTT connectivity, and component status. Reports issues and attempts
 * automatic recovery where possible.
 */
void performHealthCheck() {
    Serial.println("Performing system health check...");
    
    // Check memory health
    size_t free_heap = ESP.getFreeHeap();
    size_t min_free_heap = ESP.getMinFreeHeap();
    
    if (free_heap < 10000) { // Less than 10KB free
        Serial.print("Warning: Low memory - Free: ");
        Serial.print(free_heap);
        Serial.println(" bytes");
    }
    
    if (min_free_heap < 5000) { // Minimum free dropped below 5KB
        Serial.print("Warning: Memory fragmentation detected - Min free: ");
        Serial.print(min_free_heap);
        Serial.println(" bytes");
    }
    
    // Check WiFi signal strength
    if (WiFi.status() == WL_CONNECTED) {
        int rssi = WiFi.RSSI();
        if (rssi < -80) {
            Serial.print("Warning: Weak WiFi signal - RSSI: ");
            Serial.print(rssi);
            Serial.println(" dBm");
        }
    }
    
    // Check MQTT connectivity
    if (mqtt_manager && mqtt_manager->isConfigured()) {
        if (!mqtt_manager->isConnected()) {
            Serial.println("Warning: MQTT disconnected");
        }
    }
    
    // Check sign controller status
    if (sign_controller) {
        String sign_status = sign_controller->getStatus();
        // Status is mainly for debugging, not printed regularly to avoid spam
    }
    
    // Display health indicator on sign occasionally
    static int health_counter = 0;
    if (health_counter++ % 10 == 0 && sign_controller && !sign_controller->isInPriorityMode()) {
        // Show a quick health indicator every 10th health check (5 minutes)
        String health_msg = "OK " + WiFi.localIP().toString();
        sign_controller->displayMessage(health_msg.c_str(), BB_COL_GREEN, BB_DP_TOPLINE, BB_DM_HOLD, BB_SDM_TWINKLE);
    }
}

/**
 * @brief Synchronize system time with NTP servers
 * 
 * Performs NTP time synchronization and updates the system clock.
 * Called periodically to maintain accurate time for clock display
 * and logging timestamps.
 */
void syncTime() {
    Serial.println("Synchronizing time with NTP server...");
    
    configTzTime(timezone_posix, ntp_server);
    delay(1000); // Give NTP time to sync
    
    struct tm timeinfo;
    if (getLocalTime(&timeinfo)) {
        Serial.print("Time synchronized: ");
        Serial.print(asctime(&timeinfo));
        
        // Display current time on sign
        if (sign_controller && !sign_controller->isInPriorityMode()) {
            sign_controller->displayClock();
        }
    } else {
        Serial.println("Warning: NTP synchronization failed");
    }
}

/**
 * @brief Handle factory reset command
 * 
 * Performs a complete factory reset including WiFi configuration
 * clearing and device restart. This is a destructive operation
 * that returns the device to initial setup state.
 */
void handleSystemReset() {
    Serial.println("========================================");
    Serial.println("FACTORY RESET INITIATED");
    Serial.println("Clearing all configuration data...");
    Serial.println("========================================");
    
    // Clear WiFi configuration
    if (wifi_manager) {
        wifi_manager->clearConfigData();
    }
    
    // Clear any saved preferences
    // TODO: Add preferences clearing when implemented
    
    // Display reset message on sign
    if (sign_controller) {
        sign_controller->displayPriorityMessage("Factory Reset");
    }
    
    delay(3000); // Give user time to see message
    
    Serial.println("Restarting device...");
    
    // Reset and enter configuration portal
    if (wifi_manager) {
        wifi_manager->resetAndEnterConfigPortal();
    } else {
        ESP.restart();
    }
}

/**
 * @brief Smart delay function that maintains background operations
 * 
 * Provides a non-blocking delay that continues to service WiFi management,
 * MQTT communication, and other background tasks during the delay period.
 * 
 * @param delay_ms Delay duration in milliseconds
 */
void smartDelay(unsigned long delay_ms) {
    unsigned long start_time = millis();
    
    while (millis() - start_time < delay_ms) {
        // Service WiFi manager
        if (wifi_manager) {
            wifi_manager->run();
        }
        
        // Service MQTT if connected
        if (mqtt_manager && services_initialized) {
            mqtt_manager->loop();
        }
        
        // Service sign controller
        if (sign_controller) {
            sign_controller->loop();
        }
        
        // Small delay to prevent excessive CPU usage
        delay(1);
        
        // Yield to other tasks
        yield();
    }
}

/**
 * @brief Print comprehensive system information
 * 
 * Displays detailed system information including hardware details,
 * network configuration, memory status, and component versions.
 * Useful for debugging and system monitoring.
 */
void printSystemInfo() {
    Serial.println("========================================");
    Serial.println("SYSTEM INFORMATION");
    Serial.println("========================================");
    
    // Hardware information
    Serial.print("Chip Model: ");
    Serial.println(ESP.getChipModel());
    Serial.print("Chip Revision: ");
    Serial.println(ESP.getChipRevision());
    Serial.print("CPU Frequency: ");
    Serial.print(ESP.getCpuFreqMHz());
    Serial.println(" MHz");
    Serial.print("Flash Size: ");
    Serial.print(ESP.getFlashChipSize() / 1024 / 1024);
    Serial.println(" MB");
    
    // Memory information
    Serial.print("Heap Size: ");
    Serial.print(ESP.getHeapSize());
    Serial.println(" bytes");
    Serial.print("Free Heap: ");
    Serial.print(ESP.getFreeHeap());
    Serial.println(" bytes");
    Serial.print("PSRAM Size: ");
    Serial.print(ESP.getPsramSize());
    Serial.println(" bytes");
    
    // Network information
    Serial.print("MAC Address: ");
    Serial.println(WiFi.macAddress());
    Serial.print("Device ID: ");
    Serial.println(device_id);
    
    // Software versions
    Serial.print("Arduino Core: ");
    Serial.println(ESP.getSdkVersion());
    Serial.print("Application: ");
    Serial.println(APP_VERSION);
    Serial.print("Build Date: ");
    Serial.println(BUILD_DATE);
    
    Serial.println("========================================");
}