/****************************************************************************************************************************
  defines.h
  ESP32 LED Sign Controller Configuration

  Configuration defines for the LED sign controller project.
  Uses tzapu/WiFiManager for WiFi configuration portal.
 *****************************************************************************************************************************/

#ifndef defines_h
#define defines_h

#if !ESP32
  #error This code is intended to run only on ESP32 boards! Please check your Tools->Board setting.
#endif

/////////////////////////////////////////////
/////// WIFI MANAGER CONFIGURATION //////////
/////////////////////////////////////////////

// Hostname for mDNS and WiFi
#define HOST_NAME                 "LEDSign"

// Configuration portal settings
#define CONFIG_PORTAL_TIMEOUT     180       // Seconds before portal closes
#define WIFI_CONNECT_TIMEOUT      30        // Seconds to wait for WiFi connection

// LED indicator (optional)
#ifdef LED_BUILTIN
  #define LED_PIN     LED_BUILTIN
#else
  #define LED_PIN     2
#endif

/////////////////////////////////////////////
/////// LED SIGN CONTROLLER DEFINES /////////
/////////////////////////////////////////////

#include "BETABRITE.h"
#include "Credentials.h"
#include "dynamicParams.h"

// MQTT QoS and Session Configuration
#define MQTT_QOS_LEVEL            1       // At least once delivery
#define MQTT_CLEAN_SESSION        false   // Persistent session for reliability

// MQTT Port Configuration
// Server-only TLS: ESP32 validates broker with CA cert, authenticates with username/password
#define MQTT_TLS_PORT             8883    // Standard TLS MQTT port
#define MQTT_BASIC_PORT           1883    // Fallback non-TLS port (not recommended)

// Certificate paths for TLS authentication (stored in LittleFS)
#define CERT_PATH_CA              "/certs/ca.crt"
#define CERT_PATH_CLIENT_CERT     "/certs/client.crt"
#define CERT_PATH_CLIENT_KEY      "/certs/client.key"

// Zone Configuration (per ESP32_BETABRITE_IMPLEMENTATION.md)
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
#define SIGN_DEFAULT_COLOUR       BB_COL_GREEN
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

// Current firmware version (semantic versioning)
#define FIRMWARE_VERSION          "0.2.1"

// GitHub Repository Configuration
// TODO: Update these with your actual GitHub username and repository name
#define GITHUB_REPO_OWNER         "yourusername"
#define GITHUB_REPO_NAME          "ledSignController"

// GitHub Personal Access Token (stored in LittleFS for security)
#define GITHUB_TOKEN_PATH         "/github_token.txt"

// OTA Update Check Interval
#define OTA_CHECK_INTERVAL_HOURS  24
#define OTA_CHECK_INTERVAL_MS     (OTA_CHECK_INTERVAL_HOURS * 60 * 60 * 1000UL)

// OTA Update Behavior
#define OTA_AUTO_UPDATE_ENABLED   true
#define OTA_BOOT_CHECK_ENABLED    false

// OTA Security Settings
#define OTA_VERIFY_CHECKSUM       true
#define OTA_ALLOW_DOWNGRADE       false

#endif // defines_h
