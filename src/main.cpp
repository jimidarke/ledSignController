
#include "defines.h"
#include <SPI.h> 
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "BETABRITE.h"
#include "Credentials.h" 
#include "dynamicParams.h"

// examples of strings with options
// [red]as of 10/28/12 07:38pm
// [snow,green]Some Text here, it is green with snow effect
// [amber,rotate]I'm amber and I rotate
// [green, rotate]green
// [red,interlock]Monday
// [green,rotate]

char wifi_ssid[32] = "default_ssid";
char wifi_pass[32] = "default_password";
char mqtt_server[40] = "default_mqtt_server";
uint16_t mqtt_port = 1883;
char mqtt_user[32] = "";
char mqtt_pass[32] = "";
int sign_max_files = 5;       // number of files on the sign (before they get overridden by new ones)
int sign_priority_delay = 30; // seconds to display priority messages

// POSIX time zone string for Mountain Time with DST
const char* tz = "MST7MDT,M3.2.0/2,M11.1.0/2";

// NTP server
const char* ntpServer = "pool.ntp.org";

WiFiClient espClient;
PubSubClient client(espClient);
ESP_WiFiManager_Lite* ESP_WiFiManager;

bool inPriority = false;
bool alreadyShowingOfflineMessage = false;
bool ismqttConfigured = false;
String LEDSIGNID; // will update later using MAC address

BETABRITE bb(1, 16, 17); // RX, TX
char bbTextFileName = 'A';
int numFiles = sign_max_files; // replace this
unsigned long lastUpdate = 0;
unsigned long clockStart = 0;

// declare functions before using them
void parsePayload(const char *msg);
void connectWiFi();
void reconnectMQTT();
void callback(char *topic, byte *message, unsigned int length);
unsigned long getUptime();
String getIPAddress();
String getMACAddress();
int getRSSI();


#if USING_CUSTOMS_STYLE
const char NewCustomsStyle[] PROGMEM = "<style>div,input{padding:5px;font-size:1em;}input{width:95%;}body{text-align: center;}"\
                                       "button{background-color:blue;color:white;line-height:2.4rem;font-size:1.2rem;width:100%;}fieldset{border-radius:0.3rem;margin:0px;}</style>";
#endif


void sendSensorUpdates() {
  if (!ismqttConfigured)
    return;
  // RSSI
  client.publish(("ledSign/" + LEDSIGNID + "/rssi").c_str(), String(WiFi.RSSI()).c_str(), true);
  // IP Address
  client.publish(("ledSign/" + LEDSIGNID + "/ip").c_str(), WiFi.localIP().toString().c_str(), true);
  // Uptime
  client.publish(("ledSign/" + LEDSIGNID + "/uptime").c_str(), String(millis() / 1000).c_str(), true);
  Serial.print("RSSI: ");
  Serial.println(WiFi.RSSI());
}

// Callback to handle incoming MQTT messages
void callback(char *topic, byte *message, unsigned int length)
{
  String messageTemp;
  for (int i = 0; i < length; i++)
  {
    messageTemp += (char)message[i];
  }
  parsePayload(messageTemp.c_str());
}

void printDateTime(bool UseMilitaryTime = false) {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return;
  }

  // Print date
  Serial.print("Date: ");
  Serial.print(timeinfo.tm_year + 1900);  // Year
  Serial.print("-");
  Serial.print((timeinfo.tm_mon + 1) < 10 ? "0" : "");  // Leading zero for month
  Serial.print(timeinfo.tm_mon + 1);      // Month
  Serial.print("-");
  Serial.print(timeinfo.tm_mday < 10 ? "0" : "");       // Leading zero for day
  Serial.println(timeinfo.tm_mday);       // Day

  // Determine hour and AM/PM for 12-hour format if needed
  int displayHour = timeinfo.tm_hour;
  bool isPM = false;

  if (!UseMilitaryTime) {
    if (displayHour >= 12) {
      isPM = true;
      if (displayHour > 12) displayHour -= 12;  // Convert to 12-hour format
    } else if (displayHour == 0) {
      displayHour = 12;  // Midnight in 12-hour format
    }
  }

  // Print time
  Serial.print("Time: ");
  Serial.print(displayHour < 10 ? "0" : "");  // Leading zero for hour
  Serial.print(displayHour);
  Serial.print(":");
  Serial.print(timeinfo.tm_min < 10 ? "0" : "");  // Leading zero for minute
  Serial.print(timeinfo.tm_min);
  Serial.print(":");
  Serial.print(timeinfo.tm_sec < 10 ? "0" : "");  // Leading zero for second
  Serial.print(timeinfo.tm_sec);

  if (!UseMilitaryTime) {
    Serial.print(isPM ? " PM" : " AM");
  }
  Serial.println();
}

void showOfflineConnectionDetails() //displays on the sign the wifi info and such when its offline
{
  if (!alreadyShowingOfflineMessage) {
    // default options
    char color = BB_COL_AUTOCOLOR;
    char position = BB_DP_TOPLINE;
    char mode = BB_DM_ROTATE;
    char special = BB_SDM_TWINKLE;
    String msg = "Connect to WiFi LEDSign password ledsign0 then access http://192.168.50.1 to configure sign";
    bb.CancelPriorityTextFile();
    bb.WritePriorityTextFile(msg.c_str(), color, position, mode, special);
    Serial.println("Displaying Offline Message");
    alreadyShowingOfflineMessage = true;
  }
}

String getFriendlyDateTime() {
  time_t now = time(nullptr);
  struct tm *timeinfo;
  timeinfo = localtime(&now);
  char buffer[80];
  strftime(buffer, 80, "%m/%d %I:%M %p", timeinfo);
  return String(buffer);
}
void clearTextFiles() { //clears all values i.e. wipes the screen
  //iterates from A to numFiles and clears the text files
  for (char i = 'A'; i < 'A' + numFiles + 1; i++)
  {
    Serial.print("Clearing Text File: " ); Serial.println(i);
    bb.WriteTextFile(i, "", BB_COL_AUTOCOLOR, BB_DP_TOPLINE, BB_DM_COMPROTATE, BB_SDM_TWINKLE);
  }
  bbTextFileName = 'A';
}

void showClock() { //renders the time
  String dtime = getFriendlyDateTime();
  Serial.println(dtime);
  char color = BB_COL_AUTOCOLOR;
  char position = BB_DP_TOPLINE;
  char mode = BB_DM_CLOCK;
  char special = BB_SDM_TWINKLE;
  if (!inPriority) {
    bb.CancelPriorityTextFile();
    bb.WritePriorityTextFile(dtime.c_str(), color, position, mode, special);
    clockStart = millis();
  }
}
void initSign() {
  //init sign
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
  char color = BB_COL_AUTOCOLOR;
  char position = BB_DP_TOPLINE;
  char mode = BB_DM_AUTOMODE;
  char special = BB_SDM_INTERLOCK;
  Serial.println("Sending Default Message");
  bb.WritePriorityTextFile("Darke Tech", color, position, mode, special);
  inPriority = false;
  clockStart = 0;
  //delay(5000);
  //bb.CancelPriorityTextFile();
}

//smart delay to allow background processes to still occur
void smartDelay(int delay_ms) {
  int stoptime = millis() + delay_ms;
  while (millis() < stoptime) {
    if (ismqttConfigured) {
      reconnectMQTT();
      client.loop();
    }
    //turn off clock if times up
    if (millis() - clockStart > 10000 && clockStart > 0 && !inPriority) { //hide after ten seconds
      bb.CancelPriorityTextFile();
    }
    if (millis() - lastUpdate > 60000) { //send telemetry and show clock
      sendSensorUpdates();
      showClock();
      lastUpdate = millis();
    }
    delay(5);
  }
}



void parsePayload(const char *msg) {
  // check if first character is # this will clear the sign
  if (msg[0] == '#') {
    Serial.println("Clearing Internal Data");
    clearTextFiles();
    initSign();
    return;
  }

  if (msg[0] == '^') { //factory reset
    clearTextFiles();
    Serial.println("");
    Serial.println("Clearing All Data and Resetting");
    Serial.println("UGHhh Imm Dying DIEING");
    ESP_WiFiManager->clearConfigData();
    ESP_WiFiManager->resetAndEnterConfigPortal();
  }

  // default options
  char color = BB_COL_AUTOCOLOR;
  char position = BB_DP_TOPLINE;
  char mode = BB_DM_ROTATE;
  char special = BB_SDM_TWINKLE;

  // check if first character is a * to denote a priority message
  if (msg[0] == '*') {
    inPriority = true; //global var
    msg++;
    Serial.println("### Priority Message ###");
    Serial.println(msg);
    bb.CancelPriorityTextFile();
    bb.WritePriorityTextFile("# # # #", BB_COL_RED, BB_DP_TOPLINE, BB_DM_FLASH, BB_SDM_TWINKLE);
    smartDelay(2500); // 5 seconds delay to display priority message warning
    bb.CancelPriorityTextFile();
    bb.WritePriorityTextFile(msg, color, position, mode, special);
    smartDelay(25000); // 20 seconds delay to display actual priority message
    bb.CancelPriorityTextFile();
    inPriority = false;
    return;
  }

  // handle options
  char *open_delim = strchr(msg, '[');
  char *close_delim = strchr(msg, ']');
  if (open_delim == msg && close_delim != NULL && !inPriority)  {
    int options_length = close_delim - open_delim;
    char options[options_length];
    strncpy(options, msg + 1, options_length - 1);
    options[options_length - 1] = '\0';
    Serial.print("options: ");
    Serial.println(options);
    char *option = strtok(options, ",");
    while (option != NULL)
    {
      if (strstr(option, "trumpet") == option)
      {
        mode = BB_DM_SPECIAL;
        special = BB_SDM_TRUMPET;
      }
      else if (strstr(option, "red") == option)
      {
        color = BB_COL_RED;
      }
      else if (strstr(option, "amber") == option)
      {
        color = BB_COL_AMBER;
      }
      else if (strstr(option, "green") == option)
      {
        color = BB_COL_GREEN;
      }
      else if (strstr(option, "rotate") == option)
      {
        mode = BB_DM_ROTATE;
      }
      else if (strstr(option, "hold") == option)
      {
        mode = BB_DM_HOLD;
      }
      else if (strstr(option, "flash") == option)
      {
        mode = BB_DM_FLASH;
      }
      else if (strstr(option, "rollup") == option)
      {
        mode = BB_DM_ROLLUP;
      }
      else if (strstr(option, "rolldown") == option)
      {
        mode = BB_DM_ROLLDOWN;
      }
      else if (strstr(option, "rollleft") == option)
      {
        mode = BB_DM_ROLLLEFT;
      }
      else if (strstr(option, "rollright") == option)
      {
        mode = BB_DM_ROLLRIGHT;
      }
      else if (strstr(option, "wipeup") == option)
      {
        mode = BB_DM_WIPEUP;
      }
      else if (strstr(option, "wipedown") == option)
      {
        mode = BB_DM_WIPEDOWN;
      }
      else if (strstr(option, "wipeleft") == option)
      {
        mode = BB_DM_WIPELEFT;
      }
      else if (strstr(option, "wiperight") == option)
      {
        mode = BB_DM_WIPERIGHT;
      }
      else if (strstr(option, "scroll") == option)
      {
        mode = BB_DM_SCROLL;
      }
      else if (strstr(option, "automode") == option)
      {
        mode = BB_DM_AUTOMODE;
      }
      else if (strstr(option, "rollin") == option)
      {
        mode = BB_DM_ROLLIN;
      }
      else if (strstr(option, "rollout") == option)
      {
        mode = BB_DM_ROLLOUT;
      }
      else if (strstr(option, "wipein") == option)
      {
        mode = BB_DM_WIPEIN;
      }
      else if (strstr(option, "wipeout") == option)
      {
        mode = BB_DM_WIPEOUT;
      }
      else if (strstr(option, "comprotate") == option)
      {
        mode = BB_DM_COMPROTATE;
      }
      else if (strstr(option, "explode") == option)
      {
        mode = BB_DM_EXPLODE;
      }
      else if (strstr(option, "clock") == option)
      {
        mode = BB_DM_CLOCK;
      }
      else if (strstr(option, "twinkle") == option)
      {
        mode = BB_DM_SPECIAL;
        special = BB_SDM_TWINKLE;
      }
      else if (strstr(option, "sparkle") == option)
      {
        mode = BB_DM_SPECIAL;
        special = BB_SDM_SPARKLE;
      }
      else if (strstr(option, "snow") == option)
      {
        mode = BB_DM_SPECIAL;
        special = BB_SDM_SNOW;
      }
      else if (strstr(option, "interlock") == option)
      {
        mode = BB_DM_SPECIAL;
        special = BB_SDM_INTERLOCK;
      }
      else if (strstr(option, "switch") == option)
      {
        mode = BB_DM_SPECIAL;
        special = BB_SDM_SWITCH;
      }
      else if (strstr(option, "slide") == option)
      {
        mode = BB_DM_SPECIAL;
        special = BB_SDM_SLIDE;
      }
      else if (strstr(option, "spray") == option)
      {
        mode = BB_DM_SPECIAL;
        special = BB_SDM_SPRAY;
      }
      else if (strstr(option, "starburst") == option)
      {
        mode = BB_DM_SPECIAL;
        special = BB_SDM_STARBURST;
      }
      else if (strstr(option, "welcome") == option)
      {
        mode = BB_DM_SPECIAL;
        special = BB_SDM_WELCOME;
      }
      else if (strstr(option, "slots") == option)
      {
        mode = BB_DM_SPECIAL;
        special = BB_SDM_SLOTS;
      }
      else if (strstr(option, "newsflash") == option)
      {
        mode = BB_DM_SPECIAL;
        special = BB_SDM_NEWSFLASH;
      }
      else if (strstr(option, "cyclecolors") == option)
      {
        mode = BB_DM_SPECIAL;
        special = BB_SDM_CYCLECOLORS;
      }
      else if (strstr(option, "thankyou") == option)
      {
        mode = BB_DM_SPECIAL;
        special = BB_SDM_THANKYOU;
      }
      else if (strstr(option, "nosmoking") == option)
      {
        mode = BB_DM_SPECIAL;
        special = BB_SDM_NOSMOKING;
      }
      else if (strstr(option, "dontdrinkanddrive") == option)
      {
        mode = BB_DM_SPECIAL;
        special = BB_SDM_DONTDRINKANDDRIVE;
      }
      else if (strstr(option, "fish") == option)
      {
        mode = BB_DM_SPECIAL;
        special = BB_SDM_FISHIMAL;
      }
      else if (strstr(option, "fireworks") == option)
      {
        mode = BB_DM_SPECIAL;
        special = BB_SDM_FIREWORKS;
      }
      else if (strstr(option, "balloon") == option)
      {
        mode = BB_DM_SPECIAL;
        special = BB_SDM_TURBALLOON;
      }
      else if (strstr(option, "bomb") == option)
      {
        mode = BB_DM_SPECIAL;
        special = BB_SDM_BOMB;
      }
      else
      {
        Serial.print("Unknown option:");
        Serial.println(option);
      }
      option = strtok(NULL, ",");
    }
    msg = close_delim + 1;
  }
  Serial.println("");
  Serial.print("Color: ");
  Serial.print(color);
  Serial.print(" Pos: ");
  Serial.print(position);
  Serial.print(" Mode: ");
  Serial.print(mode);
  Serial.print(" Special: ");
  Serial.print(special);
  Serial.print(" File: ");
  Serial.println(bbTextFileName);
  Serial.println(msg);
  //bbTextFileName++;
  bb.WriteTextFile(bbTextFileName++, msg, color, position, mode, special);
  // check if more than numfiles files have been created and reset counter
  if (bbTextFileName > 'A' + numFiles + 1)
  {
    bbTextFileName = 'A';
  }
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


// Reconnect to MQTT if disconnected
void reconnectMQTT()
{
  int c = 0;
  while (!client.connected()) {
    if (client.connect(("LEDSign_" + LEDSIGNID).c_str(), mqtt_user, mqtt_pass)) {
      String message_topic = "ledSign/" + LEDSIGNID + "/message";
      client.subscribe(message_topic.c_str());
      client.subscribe("ledSign/message");
    }
    else
    {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" trying again in 1 second");
      delay(1000);
      c++;
      if (c > 5) {
        Serial.println("Can't seem to connect? Maybe the server info is wrong?");
        Serial.print("Server: " ); Serial.print(mqtt_server); Serial.print(":"); Serial.println(mqtt_port);
        Serial.println("Going to reboot just for the fuck of it");
        ESP.restart();
      }
    }
  }
}


void initMQTT() { //reads the stored values
  // 0 - mqtt_server 1 - mqtt_port 2 - mqtt_user 3 - mqtt_pass
  // take from myMenuItem
  strcpy(mqtt_server, myMenuItems[0].pdata);
  mqtt_port = atoi(myMenuItems[1].pdata);
  strcpy(mqtt_user, myMenuItems[2].pdata);
  strcpy(mqtt_pass, myMenuItems[3].pdata);

  Serial.print("Configuring MQTT Server:");
  Serial.print(mqtt_server);
  Serial.print(":");
  Serial.println(mqtt_port);
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
  ismqttConfigured = true;
  reconnectMQTT();
  Serial.println("Getting Date Time");
  configTzTime(tz, ntpServer);
  printDateTime();
}

void setup() {
  Serial.begin(115200);
  while (!Serial);
  Serial.println("Starting LED Sign Controller. Darke Tech Corp. 2024");
  initSign();
  Serial.println("Checking WiFi Connectivity through WiFiManager_Lite");
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
  ESP_WiFiManager->begin("LEDSign");
  Serial.println("Starting main loop");
}



void loop() {
  ESP_WiFiManager->run(); //manages the wifi connections
  if (WiFi.status() == WL_CONNECTED) {
    if (!ismqttConfigured) {
      initMQTT(); //also gets/updates time
      delay(200);
    }
    smartDelay(1000); //main "online" loop
  }
  else {
    showOfflineConnectionDetails();
    if (ismqttConfigured) {
      Serial.println("ERROR");
      Serial.println("Lost WiFi or other general error. Rebooting");
      Serial.println("BYEEEE");
      ESP.restart();
    }
  }
}
