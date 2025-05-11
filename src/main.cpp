#include "defines.h"
#include <SPI.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "OTAupdate.h" // OTA Updates call checkForUpdates()
#include "MessageParser.h"
#include "MQTTManager.h"
#include "RestApiManager.h"

// JSON format examples:
// {"type":"normal", "text":"Hello World", "color":"red", "mode":"rotate"}
// {"type":"priority", "text":"Alert!", "color":"red", "mode":"flash", "special":"starburst"}
// {"type":"clear"}
// {"type":"reset"}
// {"type":"options"}
// {"type":"normal", "text":"Temperature: 21°C", "color":"green", "position":"midline", "mode":"hold"}

const char *currentVersion = "0.0.7";

char wifi_ssid[32] = SIGN_DEFAULT_SSID;
char wifi_pass[32] = SIGN_DEFAULT_PASS;
char mqtt_server[40] = "";
uint16_t mqtt_port = 1883;
char mqtt_user[32] = "";
char mqtt_pass[32] = "";
int sign_max_files = 5; // number of files on the sign (before they get overridden by new ones)

// POSIX time zone string for Mountain Time with DST
char tz[64] = SIGN_TIMEZONE_POSIX;  // Changed from const char* to a char array to allow modification

// NTP server
const char *ntpServer = "pool.ntp.org";

// OTA Update Server details
const char *otaVersionURL = "http://docker02.darketech.ca:8003/version.txt"; // 0.0.1
const char *otaFirmwareURL = "http://docker02.darketech.ca:8003/firmware.bin";

const char *signOptions[] = {
  "Clock",
  "Weather",
  "Jokes",
  "News",
  "Sports",
  "Trivia",
  "Schedule",
  "Status",
  "Alerts",
  "Messages",
  "Settings",
  "About"
}; //iterate through this in the main loop

WiFiClient espClient;
PubSubClient client(espClient);
MQTTManager* mqttManager;
ESP_WiFiManager_Lite *ESP_WiFiManager;
RestApiManager* restApiManager;
BETABRITE bb(1, 16, 17); // SerialID (always 1), RX pin, TX pin

bool inPriority = false;
bool ismqttConfigured = false;
bool isRestApiConfigured = false;
unsigned long clockStart = 0; // Keep track of when the clock started showing

String LEDSIGNID; // will update later using MAC address

char bbTextFileName = 'A';
int numFiles = sign_max_files; // replace this
unsigned long lastUpdate = 0;
unsigned long nextSignAt = 0;

int jokenum = 0;
int newsnum = 0;
int sportnum = 0;
int trivianum = 0;
int schednum = 0;
int statusnum = 0;

// declare functions before using them
void parsePayload(const char *msg);
void processMessage(const char *msg); // Common handler for MQTT and REST API
void reconnectMQTT();
void callback(char *topic, byte *message, unsigned int length);
void restApiMessageHandler(const char *msg); // REST API message callback
unsigned long getUptime();
void smartDelay(int delay_ms);
void cleanup();
void printHomeAssistantDetails();
void initRestApi(); // Initialize REST API

#if USING_CUSTOMS_STYLE
const char NewCustomsStyle[] PROGMEM = "<style>div,input{padding:5px;font-size:1em;}input{width:95%;}body{text-align: center;}"
                                       "button{background-color:blue;color:white;line-height:2.4rem;font-size:1.2rem;width:100%;}fieldset{border-radius:0.3rem;margin:0px;}</style>";
#endif

void sendSensorUpdates()
{
  if (!ismqttConfigured || !mqttManager->checkConnection())
    return;
    
  // Create JSON document for telemetry data
  StaticJsonDocument<256> doc;
  doc["rssi"] = WiFi.RSSI();
  doc["ip"] = WiFi.localIP().toString();
  doc["uptime"] = millis() / 1000;
  doc["version"] = currentVersion;
  doc["freeHeap"] = ESP.getFreeHeap();
  doc["inPriority"] = inPriority;
  
  // Serialize to JSON string
  String jsonStr;
  serializeJson(doc, jsonStr);
  
  // Publish telemetry data as JSON
  mqttManager->publish(("ledSign/" + LEDSIGNID + "/status").c_str(), jsonStr.c_str(), true);
  
  // Also publish individual values for compatibility and Home Assistant
  mqttManager->publish(("ledSign/" + LEDSIGNID + "/rssi").c_str(), String(WiFi.RSSI()).c_str(), true);
  mqttManager->publish(("ledSign/" + LEDSIGNID + "/ip").c_str(), WiFi.localIP().toString().c_str(), true);
  mqttManager->publish(("ledSign/" + LEDSIGNID + "/uptime").c_str(), String(millis() / 1000).c_str(), true);
  
  // Publish current sign status for Home Assistant
  String statusMsg = inPriority ? "Priority message active" : "Normal operation";
  mqttManager->publish(("ledSign/" + LEDSIGNID + "/state").c_str(), statusMsg.c_str(), true);
  
  Serial.print("Telemetry sent - RSSI: ");
  Serial.println(WiFi.RSSI());
}

void printDateTime(bool UseMilitaryTime = false)
{
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo))
  {
    Serial.println("Failed to obtain time");
    return;
  }

  // Print date
  Serial.print("Date: ");
  Serial.print(timeinfo.tm_year + 1900); // Year
  Serial.print("-");
  Serial.print((timeinfo.tm_mon + 1) < 10 ? "0" : ""); // Leading zero for month
  Serial.print(timeinfo.tm_mon + 1);                   // Month
  Serial.print("-");
  Serial.print(timeinfo.tm_mday < 10 ? "0" : ""); // Leading zero for day
  Serial.println(timeinfo.tm_mday);               // Day

  // Determine hour and AM/PM for 12-hour format if needed
  int displayHour = timeinfo.tm_hour;
  bool isPM = false;

  if (!UseMilitaryTime)
  {
    if (displayHour >= 12)
    {
      isPM = true;
      if (displayHour > 12)
        displayHour -= 12; // Convert to 12-hour format
    }
    else if (displayHour == 0)
    {
      displayHour = 12; // Midnight in 12-hour format
    }
  }

  // Print time
  Serial.print("Time: ");
  Serial.print(displayHour < 10 ? "0" : ""); // Leading zero for hour
  Serial.print(displayHour);
  Serial.print(":");
  Serial.print(timeinfo.tm_min < 10 ? "0" : ""); // Leading zero for minute
  Serial.print(timeinfo.tm_min);
  Serial.print(":");
  Serial.print(timeinfo.tm_sec < 10 ? "0" : ""); // Leading zero for second
  Serial.print(timeinfo.tm_sec);

  if (!UseMilitaryTime)
  {
    Serial.print(isPM ? " PM" : " AM");
  }
  Serial.println();
}

String generateRandomString(int length)
{
  const char characters[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
  const int charactersLength = sizeof(characters) - 1;
  String result = "";
  for (int i = 0; i < length; i++)
  {
    result += characters[random(0, charactersLength)];
  }
  return result;
}

void showAllSignOptions() // runs through all color, special, mode, and position options for the sign
{
  // Define the four lists
  const char colors[] = {
      BB_COL_AMBER, BB_COL_AUTOCOLOR, BB_COL_BROWN, BB_COL_COLORMIX,
      BB_COL_DIMGREEN, BB_COL_DIMRED, BB_COL_GREEN, BB_COL_ORANGE,
      BB_COL_RAINBOW1, BB_COL_RAINBOW2, BB_COL_RED, BB_COL_YELLOW};

  const char specials[] = {
      BB_SDM_TWINKLE, BB_SDM_SPARKLE, BB_SDM_SNOW, BB_SDM_INTERLOCK,
      BB_SDM_SWITCH, BB_SDM_SLIDE, BB_SDM_SPRAY, BB_SDM_STARBURST,
      BB_SDM_WELCOME, BB_SDM_SLOTS, BB_SDM_NEWSFLASH, BB_SDM_TRUMPET,
      BB_SDM_CYCLECOLORS, BB_SDM_THANKYOU, BB_SDM_NOSMOKING,
      BB_SDM_DONTDRINKANDDRIVE, BB_SDM_FISHIMAL, BB_SDM_FIREWORKS,
      BB_SDM_TURBALLOON, BB_SDM_BOMB};

  const char modes[] = {
      BB_DM_ROTATE, BB_DM_HOLD, BB_DM_FLASH, BB_DM_ROLLUP,
      BB_DM_ROLLDOWN, BB_DM_ROLLLEFT, BB_DM_ROLLRIGHT, BB_DM_WIPEUP,
      BB_DM_WIPEDOWN, BB_DM_WIPELEFT, BB_DM_WIPERIGHT, BB_DM_SCROLL,
      BB_DM_SPECIAL, BB_DM_AUTOMODE, BB_DM_ROLLIN, BB_DM_ROLLOUT,
      BB_DM_WIPEIN, BB_DM_WIPEOUT, BB_DM_COMPROTATE, BB_DM_EXPLODE,
      BB_DM_CLOCK};

  const char positions[] = {BB_DP_MIDLINE, BB_DP_TOPLINE, BB_DP_BOTLINE, BB_DP_FILL, BB_DP_LEFT, BB_DP_RIGHT};

  // default options
  char default_color = BB_COL_AUTOCOLOR;
  char default_position = BB_DP_TOPLINE;
  char default_mode = BB_DM_ROTATE;
  char default_special = BB_SDM_TWINKLE;
  // toShowClock = false;
  // Iterate Specials first, then iterate through the rest
  for (int i = 0; i < sizeof(specials) / sizeof(specials[0]); i++)
  {
    char special = specials[i];
    String randomMsg = String(i) + generateRandomString(4);
    Serial.print(special);
    Serial.print(": ");
    Serial.println(randomMsg);
    bb.CancelPriorityTextFile();
    bb.WritePriorityTextFile(randomMsg.c_str(), BB_COL_RED, default_position, BB_DM_SPECIAL, special);
    smartDelay(10000); // 10 seconds
  }
  // toShowClock = true;
}

void showOfflineConnectionDetails() // displays on the sign the wifi info and such when its offline
{
  bb.CancelPriorityTextFile(); // turns off init screens. blocking.
  /*
  You are offline
  Connect to WiFi
  LEDSign
  Password
  ledsign0
  Have a great day
  [thank you]
  */
  // default options
  char color = BB_COL_AUTOCOLOR;
  char position = BB_DP_TOPLINE;
  char mode = BB_DM_HOLD;
  char special = BB_SDM_TWINKLE;
  String msg;
  msg = "*Offline*";
  bb.WritePriorityTextFile(msg.c_str(), BB_COL_RED, position, BB_DM_EXPLODE, special);
  smartDelay(5000);
  bb.CancelPriorityTextFile();
  msg = "Connect to:";
  bb.WritePriorityTextFile(msg.c_str(), BB_COL_GREEN, position, BB_DM_HOLD, special);
  smartDelay(1500);
  bb.CancelPriorityTextFile();
  msg = "LEDSign";
  bb.WritePriorityTextFile(msg.c_str(), BB_COL_ORANGE, position, mode, special);
  smartDelay(5000);
  bb.CancelPriorityTextFile();
  msg = "Password";
  bb.WritePriorityTextFile(msg.c_str(), BB_COL_GREEN, position, mode, special);
  smartDelay(1500);
  bb.CancelPriorityTextFile();
  msg = "ledsign0";
  bb.WritePriorityTextFile(msg.c_str(), BB_COL_ORANGE, position, mode, special);
  smartDelay(5000);
  bb.CancelPriorityTextFile();
  msg = "";
  bb.WritePriorityTextFile(msg.c_str(), color, position, BB_DM_SPECIAL, BB_SDM_THANKYOU);
  smartDelay(3500);
}

String getFriendlyDateTime()
{
  time_t now = time(nullptr);
  struct tm *timeinfo;
  timeinfo = localtime(&now);
  char buffer[80];
  strftime(buffer, 80, "%m/%d %I:%M %p", timeinfo);
  return String(buffer);
}

void clearTextFiles()
{ // clears all values i.e. wipes the screen
  // iterates from A to numFiles and clears the text files
  for (char i = 'A'; i < 'A' + numFiles + 1; i++)
  {
    Serial.print("Clearing Text File: ");
    Serial.println(i);
    bb.WriteTextFile(i, "", SIGN_DEFAULT_COLOUR, SIGN_DEFAULT_POSITION, SIGN_DEFAULT_MODE, SIGN_DEFAULT_SPECIAL);
  }
  bbTextFileName = 'A';
}

void showClock()
{ // renders the time
  String dtime = getFriendlyDateTime();
  Serial.println(dtime);
  char color = SIGN_CLOCK_COLOUR;
  char position = SIGN_CLOCK_POSITION;
  char mode = SIGN_CLOCK_MODE;
  char special = SIGN_CLOCK_SPECIAL;
  if (!inPriority && clockStart == 0)
  {
    bb.CancelPriorityTextFile();
    bb.WritePriorityTextFile(dtime.c_str(), color, position, mode, special);
    clockStart = millis();
  }
}

void initSign()
{
  // init sign
  Serial.println("Initializing LED sign through TTL - RS232 Serial connection");
  String mac = WiFi.macAddress();
  mac.replace(":", "");
  LEDSIGNID = mac;
  Serial.print("Sign ID: ");
  Serial.println(LEDSIGNID);
  bbTextFileName = 'A';
  Serial.println("Setting Sign Memory Config...");
  bb.SetMemoryConfiguration(bbTextFileName, numFiles);
  delay(500);
  Serial.println("Sending Default Message");
  // msg = SIGN_INIT_STRING + currentVersion;
  String msg = String(SIGN_INIT_STRING) + " " + currentVersion;
  bb.WritePriorityTextFile(msg.c_str(), SIGN_INIT_COLOUR, SIGN_INIT_POSITION, SIGN_INIT_MODE, SIGN_INIT_SPECIAL);
  inPriority = false;
  clockStart = 0;
}

// smart delay to allow background processes to still occur
void smartDelay(int delay_ms)
{
  unsigned long stoptime = millis() + delay_ms;
  unsigned long lastYield = millis();
  
  while (millis() < stoptime)
  {
    // Handle WiFi management
    ESP_WiFiManager->run();
    
    // Handle MQTT if configured
    if (ismqttConfigured && mqttManager) {
      mqttManager->loop();
      
      // Handle clock display timeout
      if (millis() - clockStart > SIGN_SHOW_CLOCK_DELAY_MS && clockStart > 0 && !inPriority) {
        bb.CancelPriorityTextFile();
        clockStart = 0;
      }
      
      // Send telemetry periodically
      if (millis() - lastUpdate > 60000) {
        sendSensorUpdates();
        lastUpdate = millis();
      }
    }
    
    // Yield to prevent watchdog trigger - at least every 100ms
    if (millis() - lastYield > 100) {
      delay(1);
      lastYield = millis();
    }
  }
}

void parsePayload(const char *msg)
{
  // Parse as JSON only
  String messageStr = String(msg);
  ParsedMessage parsedMsg = parseJsonMessage(messageStr);
  
  if (!parsedMsg.isValid) {
    Serial.print("JSON parsing error: ");
    Serial.println(parsedMsg.errorMsg);
    Serial.println("Message must be in JSON format");
    return;
  }
  
  // Process based on message type
  switch (parsedMsg.type) {    case CLEAR_SIGN:
      Serial.println("Clearing Internal Data");
      clearTextFiles();
      initSign();
      
      // Update Home Assistant light state
      if (ismqttConfigured && mqttManager) {
        mqttManager->publish(("ledSign/" + LEDSIGNID + "/light_state").c_str(), "{\"state\":\"OFF\"}", true);
        mqttManager->publish(("ledSign/" + LEDSIGNID + "/state").c_str(), "Sign cleared", true);
      }
      return;
      
    case FACTORY_RESET:
      clearTextFiles();
      Serial.println("");
      Serial.println("Clearing All Data and Resetting");
      Serial.println("Factory reset requested");
      ESP_WiFiManager->clearConfigData();
      ESP_WiFiManager->resetAndEnterConfigPortal();
      return;
      
    case SHOW_OPTIONS:
      Serial.println("Showing all sign display options");
      showAllSignOptions();
      return;
        case PRIORITY_MESSAGE:
      inPriority = true; // global var
      Serial.println("### Priority Message ###");
      Serial.println(parsedMsg.text);
      
      // Publish state update
      if (ismqttConfigured && mqttManager) {
        mqttManager->publish(("ledSign/" + LEDSIGNID + "/state").c_str(), 
                            ("PRIORITY: " + parsedMsg.text).c_str(), true);
      }
      
      // Show warning
      bb.CancelPriorityTextFile();
      bb.WritePriorityTextFile("# # # #", BB_COL_RED, BB_DP_TOPLINE, BB_DM_FLASH, BB_SDM_TWINKLE);
      smartDelay(2500); // 2.5 seconds delay to display priority message warning
      
      // Show actual message
      bb.CancelPriorityTextFile();
      bb.WritePriorityTextFile(parsedMsg.text.c_str(), parsedMsg.color, parsedMsg.position, 
                             parsedMsg.mode, parsedMsg.special);
      smartDelay(25000); // 25 seconds delay to display actual priority message
      
      // Cancel priority message
      bb.CancelPriorityTextFile();
      inPriority = false;
      
      // Update state now that priority is over
      if (ismqttConfigured && mqttManager) {
        mqttManager->publish(("ledSign/" + LEDSIGNID + "/state").c_str(), "Normal operation", true);
      }
      return;
        case NORMAL_MESSAGE:
      // Log message details
      Serial.println("");
      Serial.print("Color: ");
      Serial.print(parsedMsg.color);
      Serial.print(" Pos: ");
      Serial.print(parsedMsg.position);
      Serial.print(" Mode: ");
      Serial.print(parsedMsg.mode);
      Serial.print(" Special: ");
      Serial.print(parsedMsg.special);
      Serial.print(" File: ");
      Serial.println(bbTextFileName);
      Serial.println(parsedMsg.text);
      
      // Write message to sign
      bb.WriteTextFile(bbTextFileName++, parsedMsg.text.c_str(), parsedMsg.color, 
                     parsedMsg.position, parsedMsg.mode, parsedMsg.special);
        // Publish the message to the state topic for Home Assistant
      if (ismqttConfigured && mqttManager) {
        String truncatedMsg = parsedMsg.text;
        if (truncatedMsg.length() > 60) {
          truncatedMsg = truncatedMsg.substring(0, 57) + "...";
        }
        mqttManager->publish(("ledSign/" + LEDSIGNID + "/state").c_str(), truncatedMsg.c_str(), true);
        
        // Update light state for Home Assistant
        mqttManager->publish(("ledSign/" + LEDSIGNID + "/light_state").c_str(), "{\"state\":\"ON\"}", true);
      }
      
      // Cycle through file names
      if (bbTextFileName > 'A' + numFiles) {
        bbTextFileName = 'A';
      }
      return;
      
    case INVALID_MESSAGE:
    default:
      Serial.println("Invalid or unrecognized message format");
      return;  }
}

unsigned long getUptime()
{
  return millis() / 1000; // Uptime in seconds
}

String getIPAddress()
{
  return WiFi.localIP().toString();
}

String getMACAddress()
{
  return WiFi.macAddress();
}

int getRSSI()
{
  return WiFi.RSSI();
}

// Common message handler for both MQTT and REST API
void processMessage(const char *msg)
{
  // Log the message
  Serial.print("Processing message: ");
  if (strlen(msg) > 100) {
    char truncated[101];
    strncpy(truncated, msg, 100);
    truncated[100] = '\0';
    Serial.print(truncated);
    Serial.println("...[truncated]");
  } else {
    Serial.println(msg);
  }
  
  // Process the message
  parsePayload(msg);
}

// REST API message handler
void restApiMessageHandler(const char *msg)
{
  processMessage(msg);
}

// Callback to handle incoming MQTT messages
void callback(char *topic, byte *message, unsigned int length)
{
  // Create a null-terminated copy of the message
  char messageBuffer[length + 1];
  memcpy(messageBuffer, message, length);
  messageBuffer[length] = '\0';
  
  Serial.print("Message received on MQTT topic: ");
  Serial.println(topic);
  
  // Process the message using the common handler
  processMessage(messageBuffer);
}

void initMQTT()
{ 
  // Get values from configuration
  strcpy(mqtt_server, myMenuItems[0].pdata);
  mqtt_port = atoi(myMenuItems[1].pdata);
  strcpy(mqtt_user, myMenuItems[2].pdata);
  strcpy(mqtt_pass, myMenuItems[3].pdata);
  strncpy(tz, myMenuItems[4].pdata, sizeof(tz) - 1);  // Copy timezone from config safely
  tz[sizeof(tz) - 1] = '\0';  // Ensure null termination

  Serial.print("Configuring MQTT Server: ");
  Serial.print(mqtt_server);
  Serial.print(":");
  Serial.println(mqtt_port);
    // Initialize the MQTT manager
  mqttManager = new MQTTManager(&espClient);
  mqttManager->configure(
    mqtt_server, 
    mqtt_port, 
    mqtt_user, 
    mqtt_pass, 
    ("LEDSign_" + LEDSIGNID).c_str()
  );
  
  // Set device details for Home Assistant discovery
  mqttManager->setDeviceDetails(LEDSIGNID.c_str(), currentVersion);
  
  // Set up the message callback
  mqttManager->setCallback(callback);
    // Connect and subscribe to relevant topics
  if (mqttManager->connect()) {
    String message_topic = "ledSign/" + LEDSIGNID + "/message";
    mqttManager->subscribe(message_topic.c_str());
    mqttManager->subscribe("ledSign/message");
    
    // Subscribe to additional topic for JSON messages
    mqttManager->subscribe(("ledSign/" + LEDSIGNID + "/json").c_str());
    mqttManager->subscribe("ledSign/json");
    
    ismqttConfigured = true;
      // Publish Home Assistant MQTT discovery information
    if (mqttManager->isHassDiscoveryEnabled()) {
      Serial.println("Publishing Home Assistant discovery information");
      mqttManager->publishHassDiscovery();
      
      // Print detailed integration information
      printHomeAssistantDetails();
    }
    
    // Publish initial status
    sendSensorUpdates();
  } else {
    Serial.println("Failed to connect to MQTT broker");
  }
  
  // Configure timezone and get time
  Serial.println("Setting timezone and getting date/time");
  configTzTime(tz, ntpServer);
  printDateTime();
}

// Print Home Assistant integration details
void printHomeAssistantDetails() {
  Serial.println("Home Assistant MQTT Integration Details:");
  Serial.println("--------------------------------------");
  Serial.println("Device ID: " + LEDSIGNID);
  Serial.println("Base Topic: ledSign/" + LEDSIGNID);
  
  String nodeId = String(HASS_NODE_ID) + "_" + LEDSIGNID;
  Serial.println("Home Assistant Node ID: " + nodeId);
  
  Serial.println("\nAvailable Entities:");
  Serial.println("- Sensor: LED Sign Message");
  Serial.println("- Text: LED Sign Display Text");
  Serial.println("- Text: LED Sign JSON Command");
  Serial.println("- Light: LED Sign");
  Serial.println("- Button: LED Sign Clear");
  Serial.println("- Sensor: LED Sign Signal");
  Serial.println("- Sensor: LED Sign Uptime");
  
  Serial.println("\nTo use in Home Assistant:");
  Serial.println("1. Make sure your MQTT broker is configured in Home Assistant");
  Serial.println("2. Enable MQTT discovery in Home Assistant");
  Serial.println("3. The sign should appear automatically in your devices");
  Serial.println("4. You can use the entities in automations and scripts");
  Serial.println("--------------------------------------");
}

// Initialize the REST API
void initRestApi() {
  if (REST_API_ENABLED && WiFi.status() == WL_CONNECTED) {
    Serial.println("Setting up REST API...");
    
    // Create REST API manager if not already created
    if (!restApiManager) {
      restApiManager = new RestApiManager();
    }
    
    // Initialize with default or configured credentials
    restApiManager->begin(REST_API_USERNAME, REST_API_PASSWORD);
    
    // Set the device ID
    restApiManager->setDeviceId(LEDSIGNID.c_str());
    
    // Set message handler callback
    restApiManager->setMessageHandler(restApiMessageHandler);
    
    isRestApiConfigured = true;
    Serial.println("REST API initialized successfully");
    
    // Print API usage information
    Serial.println("\nREST API Usage:");
    Serial.println("--------------------------------------");
    Serial.println("Base URL: http://" + WiFi.localIP().toString() + "/api");
    Serial.println("Authentication: Basic (Username: " + String(REST_API_USERNAME) + ")");
    Serial.println("\nEndpoints:");
    Serial.println("- GET /api/info - Returns device information");
    Serial.println("- POST /api/message - Send message to sign (JSON format)");
    Serial.println("--------------------------------------");
    Serial.println("Example POST to /api/message:");
    Serial.println("{\"type\":\"normal\",\"text\":\"Hello World\",\"color\":\"red\",\"mode\":\"rotate\"}");
    Serial.println("--------------------------------------");
  }
}

// Clean up resources when shutting down or resetting
void cleanup() {
  if (mqttManager) {
    // Remove Home Assistant discovery entries if enabled
    if (mqttManager->isHassDiscoveryEnabled()) {
      mqttManager->removeHassDiscovery();
    }
    
    // Clean up MQTT manager
    delete mqttManager;
    mqttManager = nullptr;
  }
  
  if (restApiManager) {
    // Stop and clean up REST API manager
    restApiManager->stop();
    delete restApiManager;
    restApiManager = nullptr;
  }
  
  if (ESP_WiFiManager) {
    // WiFi manager cleanup handled by the library
  }
  
  ismqttConfigured = false;
  isRestApiConfigured = false;
}

void setup()
{
  // Initialize Serial
  Serial.begin(115200);
  while (!Serial && millis() < 5000) // Wait but timeout after 5 seconds
    delay(10);
      Serial.println("\n\n=============================================");
  Serial.println("Starting LED Sign Controller. Darke Tech Corp. 2024");
  Serial.print("Version: ");
  Serial.println(currentVersion);
  Serial.println("Home Assistant MQTT Discovery: " + String(HASS_DISCOVERY_ENABLED ? "Enabled" : "Disabled"));
  Serial.println("REST API: " + String(REST_API_ENABLED ? "Enabled" : "Disabled"));
  Serial.println("=============================================");
  
  // Initialize the sign
  initSign();
  
  // Initialize WiFi Manager
  Serial.println("Configuring WiFi connection manager");
  ESP_WiFiManager = new ESP_WiFiManager_Lite();
  ESP_WiFiManager->setConfigPortalChannel(0);
  ESP_WiFiManager->setConfigPortalIP(IPAddress(192, 168, 50, 1));
  ESP_WiFiManager->setConfigPortal("LEDSign", "ledsign0");
  
#if USING_CUSTOMS_STYLE
  ESP_WiFiManager->setCustomsStyle(NewCustomsStyle);
#endif

#if USING_CUSTOMS_HEAD_ELEMENT
  ESP_WiFiManager->setCustomsHeadElement(PSTR("<style>html{filter: invert(10%);}</style>"));
#endif

  // Start WiFi Manager
  ESP_WiFiManager->begin("LEDSign");
    // Initialize MQTT Manager (but don't connect yet - will connect in loop)
  mqttManager = new MQTTManager(&espClient);
  
  // Initialize REST API Manager (will be configured after WiFi connects)
  restApiManager = new RestApiManager();
  
  // Set random seed
  randomSeed(analogRead(0));
  
  Serial.println("Starting main loop");
}

void loop()
{
  // Always run WiFi manager first
  ESP_WiFiManager->run();
  
  // Check WiFi connection
  if (WiFi.status() == WL_CONNECTED)
  {
    if (!ismqttConfigured || !isRestApiConfigured) {
      // First-time setup when connected
      checkForUpdates(currentVersion, otaVersionURL, otaFirmwareURL);
      
      if (!ismqttConfigured) {
        initMQTT();
      }
      
      if (REST_API_ENABLED && !isRestApiConfigured) {
        initRestApi();
      }
      
      delay(50);
    }
    else {
      // Regular operation when online
      
      // Process MQTT messages
      if (mqttManager) {
        mqttManager->loop();
      }
      
      // Periodic tasks
      unsigned long currentMillis = millis();
      if (currentMillis - lastUpdate > 60000) {
        // Every minute tasks
        sendSensorUpdates();
        showClock();
        lastUpdate = currentMillis;
      }
    }
  }
  else
  {
    // WiFi disconnected - show offline message
    static unsigned long lastOfflineMessage = 0;
    if (millis() - lastOfflineMessage > 60000) { // Show message once per minute
      showOfflineConnectionDetails();
      lastOfflineMessage = millis();
    }
  }
  
  // Yield to the CPU to prevent watchdog issues
  delay(1);
}
