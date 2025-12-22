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
#include "MQTTManager.h"
#include "SignController.h"
#include "GitHubOTA.h"
#include "HADiscovery.h"

// Third-party libraries
#include <ArduinoJson.h>
#include <LittleFS.h>      // For loading certificates and GitHub token
#include <WiFiManager.h>   // tzapu/WiFiManager for configuration portal

/**
 * @brief Application version and build information
 * Version is defined in defines.h as FIRMWARE_VERSION
 */
const char* APP_VERSION = FIRMWARE_VERSION;
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
 * @brief Global object instances
 */
WiFiClient wifi_client;                          ///< WiFi client for network operations
WiFiManager wifiManager;                         ///< WiFi configuration manager (tzapu/WiFiManager)
BETABRITE led_sign(1, 16, 17);                  ///< BetaBrite sign interface (ID=1, RX=16, TX=17)
MQTTManager* mqtt_manager = nullptr;             ///< MQTT connection manager
SignController* sign_controller = nullptr;       ///< LED sign control interface
GitHubOTA* ota_manager = nullptr;                ///< GitHub-based OTA update manager
HADiscovery* ha_discovery = nullptr;             ///< Home Assistant MQTT Discovery

// WiFiManager custom parameters (linked to dynamicParams.h variables)
WiFiManagerParameter custom_mqtt_server("server", "MQTT Server", MQTT_Server, MAX_MQTT_SERVER_LEN);
WiFiManagerParameter custom_mqtt_port("port", "MQTT Port", MQTT_Port, MAX_MQTT_PORT_LEN);
WiFiManagerParameter custom_mqtt_user("user", "MQTT User (optional)", MQTT_User, MAX_MQTT_USER_LEN);
WiFiManagerParameter custom_mqtt_pass("pass", "MQTT Pass (optional)", MQTT_Pass, MAX_MQTT_PASS_LEN);
WiFiManagerParameter custom_zone_name("zone", "Sign Zone", Zone_Name, MAX_ZONE_NAME_LEN);

/**
 * @brief Application state variables
 */
String device_id;                               ///< Unique device identifier (from MAC)
bool services_initialized = false;             ///< Whether network services are ready
unsigned long last_health_check = 0;           ///< Last system health check timestamp
unsigned long last_time_sync = 0;              ///< Last NTP time synchronization
unsigned long last_offline_log = 0;            ///< Last offline status log message timestamp

/**
 * @brief System health monitoring interval (30 seconds)
 */
const unsigned long HEALTH_CHECK_INTERVAL = 30000;

/**
 * @brief Time synchronization interval (1 hour)
 */
const unsigned long TIME_SYNC_INTERVAL = 3600000;

/**
 * @brief Clock display interval (1 minute)
 */
const unsigned long CLOCK_DISPLAY_INTERVAL = 60000;

/**
 * @brief WiFi connection monitoring intervals
 */
const unsigned long WIFI_CHECK_INTERVAL = 30000;
const unsigned long MEMORY_REPORT_INTERVAL = 60000;

// WiFiManager uses built-in styling - no custom CSS needed

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
    static unsigned long last_clock_display = 0;
    unsigned long current_time = millis();
    
    // WiFiManager handles reconnection automatically - no run() needed
    // Check if WiFi disconnected and attempt reconnection
    if (WiFi.status() != WL_CONNECTED) {
        static unsigned long last_reconnect_attempt = 0;
        if (current_time - last_reconnect_attempt > 30000) {
            Serial.println("WiFi disconnected, attempting reconnection...");
            WiFi.reconnect();
            last_reconnect_attempt = current_time;
        }
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

            // Cancel offline mode if it was running
            if (sign_controller) {
                sign_controller->cancelOfflineMode();
            }

            initializeNetworkServices();
        }
        
        // Run network services if initialized
        if (services_initialized) {
            // Handle MQTT communication
            if (mqtt_manager) {
                mqtt_manager->loop();

                // Home Assistant Discovery - publish when MQTT connects
                static bool ha_discovery_published = false;
                static unsigned long last_ha_sensor_update = 0;

                if (mqtt_manager->isConnected()) {
                    // Publish discovery on first connection
                    if (!ha_discovery_published && ha_discovery) {
                        Serial.println("MQTT connected - publishing HA Discovery...");

                        // Update availability to online
                        ha_discovery->updateAvailability(true);

                        // Publish discovery messages
                        if (ha_discovery->publishDiscovery()) {
                            // Subscribe to command topics
                            ha_discovery->subscribeToCommands();
                            ha_discovery_published = true;
                            Serial.println("HA Discovery published successfully");
                        }
                    }

                    // Update HA sensors periodically (every 60 seconds)
                    if (ha_discovery && (current_time - last_ha_sensor_update > 60000)) {
                        ha_discovery->updateSensors(
                            WiFi.RSSI(),
                            millis() / 1000,
                            WiFi.localIP().toString(),
                            ESP.getFreeHeap()
                        );
                        last_ha_sensor_update = current_time;
                    }
                } else {
                    // Reset discovery flag when disconnected so it republishes on reconnect
                    ha_discovery_published = false;
                }
            }

            // Handle OTA updates (periodic checks for new firmware)
            if (ota_manager) {
                ota_manager->loop();
            }

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

            // Periodic clock display (every 60 seconds for 4 seconds, unless priority message active)
            if (current_time - last_clock_display > CLOCK_DISPLAY_INTERVAL) {
                if (sign_controller && !sign_controller->isInPriorityMode()) {
                    sign_controller->displayClock();
                    last_clock_display = current_time;
                }
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

        // Throttle log message to once every 10 seconds
        unsigned long current_time = millis();
        if (current_time - last_offline_log > 10000) {
            Serial.println("WiFi disconnected - displaying offline information");
            last_offline_log = current_time;
        }
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
    
    // Initialize WiFi manager (tzapu/WiFiManager)
    Serial.println("Initializing WiFi manager...");

    // Add custom parameters
    wifiManager.addParameter(&custom_mqtt_server);
    wifiManager.addParameter(&custom_mqtt_port);
    wifiManager.addParameter(&custom_mqtt_user);
    wifiManager.addParameter(&custom_mqtt_pass);
    wifiManager.addParameter(&custom_zone_name);

    // Configure WiFi manager
    wifiManager.setConfigPortalTimeout(CONFIG_PORTAL_TIMEOUT);
    wifiManager.setConnectTimeout(WIFI_CONNECT_TIMEOUT);
    wifiManager.setDebugOutput(true);

    // Set hostname
    WiFi.setHostname(HOST_NAME);

    // Auto-connect - will start config portal if no saved credentials
    // This blocks until WiFi is connected or portal times out
    Serial.println("Connecting to WiFi (or starting config portal)...");
    if (!wifiManager.autoConnect(SIGN_DEFAULT_SSID, SIGN_DEFAULT_PASS)) {
        Serial.println("WiFi connection failed - restarting in 3 seconds...");
        delay(3000);
        ESP.restart();
    }

    // Copy parameter values after portal (user may have updated them)
    strcpy(MQTT_Server, custom_mqtt_server.getValue());
    strcpy(MQTT_Port, custom_mqtt_port.getValue());
    strcpy(MQTT_User, custom_mqtt_user.getValue());
    strcpy(MQTT_Pass, custom_mqtt_pass.getValue());
    strcpy(Zone_Name, custom_zone_name.getValue());

    Serial.println("WiFi connected successfully!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
    
    // Initialize random number generator
    // Note: Cannot use analogRead(0) - GPIO0 is on ADC2 which conflicts with WiFi
    // Use MAC address + millis() for entropy instead
    uint8_t mac_bytes[6];
    esp_read_mac(mac_bytes, ESP_MAC_WIFI_STA);
    uint32_t seed = 0;
    for (int i = 0; i < 6; i++) {
        seed ^= (mac_bytes[i] << (i * 4));
    }
    randomSeed(seed ^ millis());
    
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
            if (sign_controller) {
                sign_controller->displayError("NTP Sync Failed", 5);
            }
        }
        
        // Initialize MQTT manager with zone name (per ESP32_BETABRITE_IMPLEMENTATION.md)
        Serial.println("Initializing MQTT manager...");

        // Get zone name from configuration
        String zone_name = String(Zone_Name);
        if (zone_name.length() == 0) {
            zone_name = SIGN_DEFAULT_ZONE;
            Serial.print("Using default zone: ");
        } else {
            Serial.print("Using configured zone: ");
        }
        Serial.println(zone_name);

        mqtt_manager = new MQTTManager(&wifi_client, device_id, zone_name);

        if (mqtt_manager) {
            // Configure MQTT from stored parameters
            if (strlen(MQTT_Server) > 0) {
                // Use global configuration variables
                strcpy(mqtt_server, MQTT_Server);
                mqtt_port = atoi(MQTT_Port);
                strcpy(mqtt_user, MQTT_User);
                strcpy(mqtt_pass, MQTT_Pass);

                // Determine if TLS should be used based on port
                // Port 8883 = standard TLS MQTT (server-only TLS + username/password)
                // Port 1883 = basic MQTT (not recommended, fallback only)
                bool use_tls = (mqtt_port != MQTT_BASIC_PORT);

                Serial.print("MQTT Configuration - Server: ");
                Serial.print(mqtt_server);
                Serial.print(", Port: ");
                Serial.print(mqtt_port);
                Serial.print(", TLS: ");
                Serial.println(use_tls ? "YES" : "NO");

                // Configure MQTT manager with TLS option
                if (mqtt_manager->configure(mqtt_server, mqtt_port, mqtt_user, mqtt_pass, use_tls)) {
                    mqtt_manager->setMessageCallback(handleMQTTMessage);

                    if (mqtt_manager->begin()) {
                        Serial.println("MQTT manager initialized successfully");

                        // Initialize Home Assistant Discovery
                        Serial.println("Initializing Home Assistant Discovery...");
                        ha_discovery = new HADiscovery(
                            mqtt_manager->getClient(),
                            device_id,
                            "LED Sign",
                            Zone_Name
                        );

                        if (ha_discovery) {
                            // Set up callbacks for HA commands
                            ha_discovery->setMessageCallback([](const String& message) {
                                Serial.print("HA: Display message: ");
                                Serial.println(message);
                                if (sign_controller) {
                                    // Use priority message for HA commands (30 second display)
                                    sign_controller->displayPriorityMessage(message.c_str(), 30);
                                }
                            });

                            ha_discovery->setEffectCallback([](const String& effect) {
                                Serial.print("HA: Effect changed to: ");
                                Serial.println(effect);
                                // Effect will be applied on next message
                            });

                            ha_discovery->setColorCallback([](const String& color) {
                                Serial.print("HA: Color changed to: ");
                                Serial.println(color);
                                // Color will be applied on next message
                            });

                            ha_discovery->setClearCallback([]() {
                                Serial.println("HA: Clear display requested");
                                if (sign_controller) {
                                    sign_controller->clearAllFiles();
                                }
                            });

                            ha_discovery->setRebootCallback([]() {
                                Serial.println("HA: Reboot requested");
                                delay(1000);
                                ESP.restart();
                            });

                            Serial.println("Home Assistant Discovery initialized");
                        }
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

        // Initialize GitHub OTA manager
        Serial.println("Initializing OTA update manager...");
        ota_manager = new GitHubOTA(GITHUB_REPO_OWNER, GITHUB_REPO_NAME, &led_sign);

        if (ota_manager) {
            ota_manager->begin(APP_VERSION);

            // Load GitHub token from LittleFS (if available)
            if (LittleFS.begin(true)) {
                Serial.println("OTA: LittleFS mounted successfully");

                File tokenFile = LittleFS.open(GITHUB_TOKEN_PATH, "r");
                if (tokenFile) {
                    String token = tokenFile.readStringUntil('\n');
                    token.trim();  // Remove whitespace/newlines
                    tokenFile.close();

                    if (token.length() > 0) {
                        ota_manager->setGitHubToken(token.c_str());
                        Serial.println("OTA: GitHub token loaded successfully");
                    } else {
                        Serial.println("OTA: Warning - GitHub token file is empty");
                    }
                } else {
                    Serial.println("OTA: Info - No GitHub token found (public repo or token not uploaded)");
                    Serial.println("OTA: To use private repos, upload token to SPIFFS at: " GITHUB_TOKEN_PATH);
                }
            } else {
                Serial.println("OTA: Warning - LittleFS mount failed, cannot load GitHub token");
            }

            // Configure OTA settings from defines.h
            ota_manager->setCheckInterval(OTA_CHECK_INTERVAL_MS);
            ota_manager->setAutoUpdate(OTA_AUTO_UPDATE_ENABLED);

            // Optionally perform boot-time update check
            if (OTA_BOOT_CHECK_ENABLED) {
                Serial.println("OTA: Performing boot-time update check...");
                if (ota_manager->checkForUpdate()) {
                    if (ota_manager->isUpdateAvailable()) {
                        Serial.printf("OTA: Update available - %s\n", ota_manager->getLatestVersion().c_str());
                        // The periodic check will handle the update, or it can be triggered manually
                    } else {
                        Serial.println("OTA: Firmware is up to date");
                    }
                }
            }

            Serial.println("OTA: Manager initialized successfully");
        } else {
            Serial.println("OTA: Warning - Failed to create OTA manager");
        }

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
 * @brief Structure for display configuration presets
 *
 * Contains all display parameters for an alert level/category combination
 */
struct DisplayPreset {
    char color_code;
    char mode_code;
    char charset_code;
    char position_code;
    const char* speed_code;
    char effect_code;
    bool priority;
    unsigned int duration;
};

/**
 * @brief Get display preset based on alert level and category
 *
 * Returns appropriate display configuration based on alert severity and type.
 * Falls back to safe defaults if level/category not recognized.
 *
 * Alert Levels (by severity):
 * - critical: Red, flash/newsflash, large text, priority, 60s
 * - warning:  Amber, scroll, normal text, 30s
 * - notice:   Green, wipein, normal text, 20s
 * - info:     Green, rotate, normal text, 15s
 *
 * Categories influence special effects:
 * - security: Bomb effect for visual urgency
 * - weather:  Snow/weather-appropriate effects
 * - automation: Welcome/completion effects
 * - system/network: Subtle twinkle effects
 *
 * @param level Alert severity level ("critical", "warning", "notice", "info")
 * @param category Alert category ("security", "weather", "automation", "system", etc.)
 * @return DisplayPreset struct with appropriate display parameters
 */
DisplayPreset getDisplayPreset(const char* level, const char* category) {
    DisplayPreset preset;

    // Default safe values
    preset.color_code = '2';        // Green
    preset.mode_code = 'a';         // Rotate
    preset.charset_code = '3';      // 7high
    preset.position_code = ' ';     // Midline
    preset.speed_code = "\027";     // Medium (3)
    preset.effect_code = '0';       // Twinkle
    preset.priority = false;
    preset.duration = 15;

    // Determine base preset by level
    if (strcmp(level, "critical") == 0) {
        preset.color_code = '1';     // Red
        preset.mode_code = 'c';      // Flash
        preset.charset_code = '6';   // 10high (large)
        preset.position_code = '0';  // Fill
        preset.speed_code = "\031";  // Fast (5)
        preset.effect_code = 'Z';    // Bomb (urgent)
        preset.priority = true;
        preset.duration = 60;
    } else if (strcmp(level, "warning") == 0) {
        preset.color_code = '3';     // Amber
        preset.mode_code = 'm';      // Scroll
        preset.charset_code = '3';   // 7high
        preset.position_code = '\"'; // Topline
        preset.speed_code = "\027";  // Medium (3)
        preset.effect_code = '0';    // Twinkle
        preset.priority = false;
        preset.duration = 30;
    } else if (strcmp(level, "notice") == 0) {
        preset.color_code = '2';     // Green
        preset.mode_code = 'r';      // Wipein
        preset.charset_code = '3';   // 7high
        preset.position_code = ' ';  // Midline
        preset.speed_code = "\027";  // Medium (3)
        preset.effect_code = '8';    // Welcome
        preset.priority = false;
        preset.duration = 20;
    }
    // else: info or unknown - use defaults set above

    // Modify effect based on category (overrides level-based effect for non-critical)
    if (strcmp(level, "critical") != 0) {  // Don't override critical bomb effect
        if (strcmp(category, "security") == 0) {
            preset.effect_code = 'B';    // Trumpet (attention-getting)
        } else if (strcmp(category, "weather") == 0) {
            preset.effect_code = '2';    // Snow
        } else if (strcmp(category, "automation") == 0) {
            preset.effect_code = '8';    // Welcome/completion
        } else if (strcmp(category, "system") == 0 || strcmp(category, "network") == 0) {
            preset.effect_code = '0';    // Twinkle (subtle)
        } else if (strcmp(category, "personal") == 0) {
            preset.effect_code = '1';    // Sparkle (friendly)
        }
    }

    return preset;
}

/**
 * @brief Handle incoming MQTT messages
 *
 * Processes MQTT messages received from subscribed topics, validates them,
 * and routes them to appropriate handlers (message display, system commands).
 * Supports JSON format with optional display_config. When display_config is
 * missing, intelligent presets are applied based on alert level and category.
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

    // Check if HADiscovery should handle this message (command topics)
    if (ha_discovery && ha_discovery->handleMessage(topic, payload, length)) {
        return;  // Message handled by HADiscovery
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

    // Try to parse as JSON (Alert Manager format)
    // Use MQTT_MAX_PACKET_SIZE from defines.h for JSON parsing buffer
    DynamicJsonDocument doc(MQTT_MAX_PACKET_SIZE);
    DeserializationError error = deserializeJson(doc, payload, length);

    if (!error) {
        // Successfully parsed as JSON - Extract alert fields
        Serial.println("MQTT: Parsing JSON alert message");

        // Extract core fields
        const char* title = doc["title"] | "Alert";
        const char* msg = doc["message"] | "";
        const char* level = doc["level"] | "info";
        const char* category = doc["category"] | "application";

        // Build display text: "Title: Message"
        String display_text = String(title) + ": " + String(msg);

        Serial.print("  Level: ");
        Serial.println(level);
        Serial.print("  Category: ");
        Serial.println(category);
        Serial.print("  Display Text: ");
        Serial.println(display_text);

        // Extract display_config object
        JsonObject displayConfig = doc["display_config"];

        if (displayConfig && !displayConfig.isNull()) {
            // Extract protocol codes from display_config
            const char* mode_code = displayConfig["mode_code"] | "a";       // Default: rotate
            const char* color_code = displayConfig["color_code"] | "2";     // Default: green
            const char* charset_code = displayConfig["charset_code"] | "3"; // Default: 7high
            const char* position_code = displayConfig["position_code"] | " "; // Default: midline
            const char* speed_code = displayConfig["speed_code"] | "\027";  // Default: medium (3)
            const char* effect_code = displayConfig["effect_code"] | "";     // Default: no effect (empty string)
            bool priority = displayConfig["priority"] | false;
            unsigned int duration = displayConfig["duration"] | 15;

            Serial.println("  Display Config:");
            Serial.print("    Mode: ");
            Serial.println(mode_code);
            Serial.print("    Color: ");
            Serial.println(color_code);
            Serial.print("    Priority: ");
            Serial.println(priority ? "YES" : "NO");
            Serial.print("    Duration: ");
            Serial.print(duration);
            Serial.println(" seconds");

            // Convert JSON codes to BetaBrite constants
            char mode = mode_code[0];
            char color = color_code[0];
            char charset = charset_code[0];
            char position = position_code[0];
            char special = (effect_code && strlen(effect_code) > 0) ? effect_code[0] : 0;

            // Display message based on priority
            if (sign_controller) {
                if (priority) {
                    // Priority message with dynamic duration
                    sign_controller->displayPriorityMessage(display_text.c_str(), duration);
                } else {
                    // Normal message with display config (including charset and speed)
                    sign_controller->displayMessage(display_text.c_str(), color, position, mode, special,
                                                   charset, speed_code);
                }
            }
        } else {
            // No display_config - apply intelligent preset based on level/category
            Serial.println("  No display_config found - applying preset based on level/category");

            DisplayPreset preset = getDisplayPreset(level, category);

            Serial.println("  Applied Preset:");
            Serial.print("    Level: ");
            Serial.println(level);
            Serial.print("    Category: ");
            Serial.println(category);
            Serial.print("    Color: ");
            Serial.println(preset.color_code);
            Serial.print("    Mode: ");
            Serial.println(preset.mode_code);
            Serial.print("    Effect: ");
            Serial.println(preset.effect_code);
            Serial.print("    Priority: ");
            Serial.println(preset.priority ? "YES" : "NO");
            Serial.print("    Duration: ");
            Serial.print(preset.duration);
            Serial.println(" seconds");

            if (sign_controller) {
                if (preset.priority) {
                    // Critical alert - use priority message
                    sign_controller->displayPriorityMessage(display_text.c_str(), preset.duration);
                } else {
                    // Normal alert - display with preset configuration
                    sign_controller->displayMessage(
                        display_text.c_str(),
                        preset.color_code,
                        preset.position_code,
                        preset.mode_code,
                        preset.effect_code,
                        preset.charset_code,
                        preset.speed_code
                    );
                }
            }
        }
        return;
    }

    // JSON parsing failed - message might be legacy format or invalid
    Serial.print("MQTT: JSON parse failed - ");
    Serial.println(error.c_str());
    Serial.println("MQTT: Treating as invalid message (bracket notation no longer supported)");

    // Log the rejection
    Serial.println("MQTT: Message rejected - only JSON format supported");
    Serial.println("MQTT: Expected format: {\"title\":\"...\", \"message\":\"...\", \"display_config\":{...}}");
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
    static int mqtt_fail_count = 0;
    if (mqtt_manager && mqtt_manager->isConfigured()) {
        if (!mqtt_manager->isConnected()) {
            Serial.println("Warning: MQTT disconnected");
            mqtt_fail_count++;

            // After 3 consecutive failed health checks (90 seconds), show error on sign
            if (mqtt_fail_count >= 3 && sign_controller) {
                String status = mqtt_manager->getConnectionStatus();
                String error_msg = "MQTT: " + status;
                sign_controller->displayError(error_msg.c_str(), 10);
                mqtt_fail_count = 0; // Reset counter after displaying error
            }
        } else {
            mqtt_fail_count = 0; // Reset counter when connected
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
        String health_msg = "System OK";

        // Add MQTT status indicator
        if (mqtt_manager && mqtt_manager->isConfigured()) {
            if (mqtt_manager->isConnected()) {
                health_msg += " [MQTT OK]";
            }
        }

        // Add IP address
        health_msg += " " + WiFi.localIP().toString();

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
        if (sign_controller) {
            sign_controller->displayError("NTP Sync Failed", 5);
        }
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
    
    // Clear WiFi configuration (tzapu/WiFiManager)
    wifiManager.resetSettings();

    // Display reset message on sign
    if (sign_controller) {
        sign_controller->displayPriorityMessage("Factory Reset");
    }

    delay(3000); // Give user time to see message

    Serial.println("Restarting device...");
    ESP.restart();
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
        // tzapu/WiFiManager handles WiFi internally - no run() needed

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