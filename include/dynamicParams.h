/****************************************************************************************************************************
  dynamicParams.h
  For ESP32 boards - tzapu/WiFiManager implementation

  Configuration parameters for MQTT and zone settings.
  Uses WiFiManagerParameter for web portal configuration.
 *****************************************************************************************************************************/

#ifndef dynamicParams_h
#define dynamicParams_h

// Parameter length limits
#define MAX_MQTT_SERVER_LEN      40
#define MAX_MQTT_PORT_LEN        6
#define MAX_MQTT_USER_LEN        32
#define MAX_MQTT_PASS_LEN        32
#define MAX_ZONE_NAME_LEN        16

// Global parameter storage (persists across reboots via WiFiManager)
char MQTT_Server[MAX_MQTT_SERVER_LEN + 1] = "alert.d-t.pw";
char MQTT_Port[MAX_MQTT_PORT_LEN + 1] = "42690";
char MQTT_User[MAX_MQTT_USER_LEN + 1] = "";
char MQTT_Pass[MAX_MQTT_PASS_LEN + 1] = "";
char Zone_Name[MAX_ZONE_NAME_LEN + 1] = "default";

#endif // dynamicParams_h
