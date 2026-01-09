/****************************************************************************************************************************
  dynamicParams.h
  For ESP32 boards - tzapu/WiFiManager implementation

  Configuration parameters for MQTT and zone settings.
  Uses WiFiManagerParameter for web portal configuration.
 *****************************************************************************************************************************/

#ifndef dynamicParams_h
#define dynamicParams_h

// Parameter length limits - Primary MQTT (Cloud/Alert Manager)
#define MAX_MQTT_SERVER_LEN      40
#define MAX_MQTT_PORT_LEN        6
#define MAX_MQTT_USER_LEN        32
#define MAX_MQTT_PASS_LEN        32
#define MAX_ZONE_NAME_LEN        16

// Parameter length limits - Secondary MQTT (Home Assistant)
#define MAX_HA_MQTT_SERVER_LEN   40
#define MAX_HA_MQTT_PORT_LEN     6

// Primary MQTT (Cloud/Alert Manager) - TLS with optional auth
char MQTT_Server[MAX_MQTT_SERVER_LEN + 1] = "alert.d-t.pw";
char MQTT_Port[MAX_MQTT_PORT_LEN + 1] = "42690";      // Alert Manager TLS port
char MQTT_User[MAX_MQTT_USER_LEN + 1] = "";           // Optional auth
char MQTT_Pass[MAX_MQTT_PASS_LEN + 1] = "";           // Optional auth
char Zone_Name[MAX_ZONE_NAME_LEN + 1] = "CHANGEME";

// Secondary MQTT (Home Assistant) - Plain, no auth, local LAN
// Empty server = HA integration disabled
char HA_MQTT_Server[MAX_HA_MQTT_SERVER_LEN + 1] = "";     // Empty = disabled
char HA_MQTT_Port[MAX_HA_MQTT_PORT_LEN + 1] = "1883";     // Default plain MQTT

#endif // dynamicParams_h
