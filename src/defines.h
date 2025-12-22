/****************************************************************************************************************************
  defines.h
  For ESP8266 / ESP32 boards

  ESP_WiFiManager_Lite (https://github.com/khoih-prog/ESP_WiFiManager_Lite) is a library 
  for the ESP32/ESP8266 boards to enable store Credentials in EEPROM/SPIFFS/LittleFS for easy 
  configuration/reconfiguration and autoconnect/autoreconnect of WiFi and other services without Hardcoding.

  Built by Khoi Hoang https://github.com/khoih-prog/ESP_WiFiManager_Lite
  Licensed under MIT license      
 *****************************************************************************************************************************/

#ifndef defines_h
#define defines_h

#if !( ESP8266 || ESP32)
  #error This code is intended to run only on the ESP8266/ESP32 boards ! Please check your Tools->Board setting.
#endif

/* Comment this out to disable prints and save space */
#define ESP_WM_LITE_DEBUG_OUTPUT      Serial

#define _ESP_WM_LITE_LOGLEVEL_        2

// use builtin LED to show configuration mode
#define USE_LED_BUILTIN               true

#define USING_MRD                     true

#if USING_MRD
  #define MULTIRESETDETECTOR_DEBUG      true
  
  // Number of seconds after reset during which a
  // subseqent reset will be considered a double reset.
  #define MRD_TIMEOUT                   10
  
  // RTC Memory Address for the DoubleResetDetector to use
  #define MRD_ADDRESS                   0

  #if (_ESP_WM_LITE_LOGLEVEL_ > 3)
    #warning Using MULTI_RESETDETECTOR
  #endif
#else
  #define DOUBLERESETDETECTOR_DEBUG     true
  
  // Number of seconds after reset during which a
  // subseqent reset will be considered a double reset.
  #define DRD_TIMEOUT                   10
  
  // RTC Memory Address for the DoubleResetDetector to use
  #define DRD_ADDRESS                   0

  #if (_ESP_WM_LITE_LOGLEVEL_ > 3)
    #warning Using DOUBLE_RESETDETECTOR
  #endif
#endif

/////////////////////////////////////////////

// Filesystem selection - Check if already defined by build flags first
#if !defined(USE_LITTLEFS) && !defined(USE_SPIFFS)
  // LittleFS has higher priority than SPIFFS (but can be overridden by build flags)
  #if ( defined(ESP_ARDUINO_VERSION_MAJOR) && (ESP_ARDUINO_VERSION_MAJOR >= 2) )
    #define USE_LITTLEFS    true
    #define USE_SPIFFS      false
  #elif defined(ARDUINO_ESP32C3_DEV)
    // For core v1.0.6-, ESP32-C3 only supporting SPIFFS and EEPROM. To use v2.0.0+ for LittleFS
    #define USE_LITTLEFS          false
    #define USE_SPIFFS            true
  #else
    // For ESP8266, and other boards
    #define USE_LITTLEFS    true
    #define USE_SPIFFS      false
  #endif
#endif

/////////////////////////////////////////////

// Add customs headers from v1.2.0
#define USING_CUSTOMS_STYLE           true
#define USING_CUSTOMS_HEAD_ELEMENT    true
#define USING_CORS_FEATURE            true

/////////////////////////////////////////////

// Force some params
#define TIMEOUT_RECONNECT_WIFI                    10000L

// Permit running CONFIG_TIMEOUT_RETRYTIMES_BEFORE_RESET times before reset hardware
// to permit user another chance to config. Only if Config Data is valid.
// If Config Data is invalid, this has no effect as Config Portal will persist
#define RESET_IF_CONFIG_TIMEOUT                   true

// Permitted range of user-defined CONFIG_TIMEOUT_RETRYTIMES_BEFORE_RESET between 2-100
#define CONFIG_TIMEOUT_RETRYTIMES_BEFORE_RESET    5

// Config Timeout 120s (default 60s). Applicable only if Config Data is Valid
#define CONFIG_TIMEOUT                            120000L

/////////////////////////////////////////////

// Permit input only one set of WiFi SSID/PWD. The other can be "NULL or "blank"
// Default is false (if not defined) => must input 2 sets of SSID/PWD
#define REQUIRE_ONE_SET_SSID_PW               true    //false

// Max times to try WiFi per loop() iteration. To avoid blocking issue in loop()
// Default 1 if not defined, and minimum 1.
#define MAX_NUM_WIFI_RECON_TRIES_PER_LOOP     2

// Default no interval between recon WiFi if lost
// Max permitted interval will be 10mins
// Uncomment to use. Be careful, WiFi reconnect will be delayed if using this method
// Only use whenever urgent tasks in loop() can't be delayed. But if so, it's better you have to rewrite your code, e.g. using higher priority tasks.
//#define WIFI_RECON_INTERVAL                   30000

/////////////////////////////////////////////

// Permit reset hardware if no WiFi to permit user another chance to access Config Portal.
#define RESET_IF_NO_WIFI              false

/////////////////////////////////////////////

#define USE_DYNAMIC_PARAMETERS        true

/////////////////////////////////////////////

#define SCAN_WIFI_NETWORKS                  true

// To be able to manually input SSID, not from a scanned SSID lists
#define MANUAL_SSID_INPUT_ALLOWED           true

// From 2-15
  #define MAX_SSID_IN_LIST                  8
  
/////////////////////////////////////////////

// Optional, to use Board Name in Menu
#define USING_BOARD_NAME                    false

/////////////////////////////////////////////

#include <ESP_WiFiManager_Lite.h>

#if ESP8266 
  #define HOST_NAME   "ESP8266-Controller"
#else
  #define HOST_NAME   "ESP32-Controller"
#endif

#ifdef LED_BUILTIN
  #define LED_PIN     LED_BUILTIN
#else
  #define LED_PIN     13
#endif

/////////////////////////////////////////////
/////// LED SIGN CONTROLLER DEFINES /////////
/////////////////////////////////////////////
#include "BETABRITE.h"
#include "Credentials.h"
#include "dynamicParams.h"

// MQTT Configuration (Alert Manager Integration)
// Note: PubSubClient.h defines these first, so we need to undefine and redefine
#ifdef MQTT_MAX_PACKET_SIZE
#undef MQTT_MAX_PACKET_SIZE
#endif
#define MQTT_MAX_PACKET_SIZE      2048    // Increased for JSON payloads

#ifdef MQTT_KEEPALIVE
#undef MQTT_KEEPALIVE
#endif
#define MQTT_KEEPALIVE            60      // Per ESP32_BETABRITE_IMPLEMENTATION.md

#define MQTT_QOS_LEVEL            1       // At least once delivery
#define MQTT_CLEAN_SESSION        false   // Persistent session for reliability

// TLS MQTT Ports (per ESP32_BETABRITE_IMPLEMENTATION.md)
#define MQTT_TLS_PORT_PRODUCTION  42690   // Production TLS port
#define MQTT_TLS_PORT_DEVELOPMENT 46942   // Development TLS port
#define MQTT_BASIC_PORT           1883    // Fallback non-TLS port

// Certificate paths for TLS authentication (stored in SPIFFS)
#define CERT_PATH_CA              "/certs/ca.crt"
#define CERT_PATH_CLIENT_CERT     "/certs/client.crt"
#define CERT_PATH_CLIENT_KEY      "/certs/client.key"

// Zone Configuration (per ESP32_BETABRITE_IMPLEMENTATION.md)
// Default zone name - should be configured via WiFi portal
#define SIGN_DEFAULT_ZONE         "default"

// Time Configuration
#define SIGN_TIMEZONE_POSIX       "MST7MDT,M3.2.0/2,M11.1.0/2"

// Display Timing
#define SIGN_SHOW_CLOCK_DELAY_MS  10000
#define SIGN_SHOW_ALERT_DELAY_MS  30000

// WiFi Portal Configuration
#define SIGN_DEFAULT_SSID         "LEDSign"
#define SIGN_DEFAULT_PASS         "ledsign0"

// Display Defaults (Fallback when display_config missing)
#define SIGN_DEFAULT_COLOUR       BB_COL_GREEN      // Green for normal messages
#define SIGN_DEFAULT_POSITION     BB_DP_TOPLINE
#define SIGN_DEFAULT_MODE         BB_DM_ROTATE
#define SIGN_DEFAULT_SPECIAL      BB_SDM_TWINKLE
#define SIGN_DEFAULT_CHARSET      '3'               // 7high
#define SIGN_DEFAULT_SPEED        "\027"            // Medium (speed 3)

// Clock Display Configuration
#define SIGN_CLOCK_COLOUR         BB_COL_AMBER
#define SIGN_CLOCK_POSITION       BB_DP_TOPLINE
#define SIGN_CLOCK_MODE           BB_DM_HOLD
#define SIGN_CLOCK_SPECIAL        BB_SDM_TWINKLE

// Initialization Message
#define SIGN_INIT_COLOUR          BB_COL_GREEN
#define SIGN_INIT_POSITION        BB_DP_TOPLINE
#define SIGN_INIT_MODE            BB_DM_ROTATE
#define SIGN_INIT_SPECIAL         BB_SDM_WELCOME
#define SIGN_INIT_STRING          "Alert Manager Ready"

/////////////////////////////////////////////
/////// OTA UPDATE CONFIGURATION ////////////
/////////////////////////////////////////////

// Current firmware version (semantic versioning: major.minor.patch)
// Update this with each release
#define FIRMWARE_VERSION          "0.2.0"

// GitHub Repository Configuration
// TODO: Update these with your actual GitHub username and repository name
#define GITHUB_REPO_OWNER         "yourusername"     // Your GitHub username/organization
#define GITHUB_REPO_NAME          "ledSignController"  // Your repository name

// GitHub Personal Access Token (stored in SPIFFS for security)
// Create token at: https://github.com/settings/tokens
// Required scopes: repo (for private repos) or public_repo (for public repos)
#define GITHUB_TOKEN_PATH         "/github_token.txt"

// OTA Update Check Interval
#define OTA_CHECK_INTERVAL_HOURS  24              // Check for updates every 24 hours
#define OTA_CHECK_INTERVAL_MS     (OTA_CHECK_INTERVAL_HOURS * 60 * 60 * 1000UL)

// OTA Update Behavior
#define OTA_AUTO_UPDATE_ENABLED   true            // Automatically download and install updates
#define OTA_BOOT_CHECK_ENABLED    false           // Also check for updates on boot (in addition to periodic)

// OTA Security Settings
#define OTA_VERIFY_CHECKSUM       true            // Require SHA256 checksum verification
#define OTA_ALLOW_DOWNGRADE       false           // Prevent downgrading to older versions



#endif      //defines_h
