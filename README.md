# LED Sign Controller

An ESP32/ESP8266-based controller for BetaBrite LED signs that uses MQTT for remote control and integrates with Home Assistant through MQTT discovery.

![LED Sign Controller](https://i.imgur.com/q4QfVyE.png)

## Features

- Control BetaBrite LED signs over a serial connection
- Multiple display modes, colors, and animation effects
- WiFi configuration portal for easy network setup
- MQTT control with multiple message formats
- Home Assistant auto-discovery with multiple entity types
- OTA firmware updates
- Clock display with automatic timezone handling
- Configurable message queue and priority messages
- JSON and simple text message formats
- Customizable display effects and animations

## Hardware Requirements

- ESP32 or ESP8266 board
- BetaBrite LED sign
- TTL to RS-232 level converter circuit (such as MAX232)
- 5V power supply

### Wiring

Connect your ESP32/ESP8266 to the BetaBrite sign using a level converter:

```
ESP32/ESP8266 GPIO16 ----> MAX232 ----> BetaBrite RX
ESP32/ESP8266 GPIO17 <---- MAX232 <---- BetaBrite TX
```

## Software Setup

### Initial Configuration

1. Flash the firmware to your ESP32/ESP8266 board
2. On first boot, the device will create a WiFi access point named "LEDSign"
3. Connect to this AP using password "ledsign0"
4. A configuration portal will open (or navigate to 192.168.50.1)
5. Configure your WiFi network settings and MQTT server

### Configuration Parameters

The web configuration portal allows you to set:

- WiFi SSID and password
- MQTT server address and port
- MQTT username and password
- Timezone (in POSIX format)

## MQTT Control

The LED sign accepts commands through MQTT topics and can use both simple text formats and JSON.

### MQTT Topics

- `ledSign/[DEVICE_ID]/message` - Send JSON-formatted messages
- `ledSign/[DEVICE_ID]/json` - Send JSON-formatted commands (same as above, for compatibility)
- `ledSign/message` - Global JSON messages (all signs)
- `ledSign/json` - Global JSON commands (all signs)

Where `[DEVICE_ID]` is the unique identifier for your sign (based on MAC address).

**Note:** All messages must be in JSON format. Legacy text format is no longer supported.

### Status Topics (published by the controller)

- `ledSign/[DEVICE_ID]/status` - JSON status information (RSSI, IP, uptime, etc.)
- `ledSign/[DEVICE_ID]/state` - Current message on the sign
- `ledSign/[DEVICE_ID]/light_state` - Home Assistant light entity state
- `ledSign/[DEVICE_ID]/availability` - Online/offline status
- `ledSign/[DEVICE_ID]/rssi` - WiFi signal strength
- `ledSign/[DEVICE_ID]/ip` - Device IP address
- `ledSign/[DEVICE_ID]/uptime` - Device uptime in seconds

### Message Format

The LED sign accepts commands in JSON format only.

General structure:
```json
{
  "type": "normal|priority|clear|reset|options",
  "text": "Message to display",
  "color": "red|green|amber|...",
  "mode": "rotate|flash|hold|...",
  "position": "topline|midline|botline|...",
  "special": "twinkle|snow|interlock|..."
}
```

Example normal message:
```json
{
  "type": "normal",
  "text": "Hello World",
  "color": "red",
  "mode": "rotate"
}
```

Example priority message:
```json
{
  "type": "priority",
  "text": "ALERT!",
  "color": "red",
  "mode": "flash",
  "special": "starburst"
}
```

Clear all messages:
```json
{
  "type": "clear"
}
```

## Display Options

### Colors
- `amber` - Amber color
- `autocolor` - Auto cycling colors
- `brown` - Brown color
- `colormix` - Mix of colors
- `dimgreen` - Dim green
- `dimred` - Dim red
- `green` - Green color
- `orange` - Orange color
- `rainbow1` - Rainbow pattern 1
- `rainbow2` - Rainbow pattern 2
- `red` - Red color
- `yellow` - Yellow color

### Display Modes
- `rotate` - Rotates text
- `hold` - Static text
- `flash` - Flashing text
- `rollup` - Roll up animation
- `rolldown` - Roll down animation
- `rollleft` - Roll left animation
- `rollright` - Roll right animation
- `wipeup` - Wipe up animation
- `wipedown` - Wipe down animation
- `wipeleft` - Wipe left animation
- `wiperight` - Wipe right animation
- `scroll` - Scrolling text
- `automode` - Auto cycling modes
- `rollin` - Roll in animation
- `rollout` - Roll out animation
- `wipein` - Wipe in animation
- `wipeout` - Wipe out animation
- `comprotate` - Compress and rotate
- `explode` - Exploding text
- `clock` - Display as clock

### Special Effects
- `twinkle` - Twinkling effect
- `sparkle` - Sparkling effect
- `snow` - Snow falling effect
- `interlock` - Interlocking display
- `switch` - Switching effect
- `slide` - Sliding effect
- `spray` - Spray effect
- `starburst` - Starburst effect
- `welcome` - Welcome animation
- `slots` - Slot machine effect
- `newsflash` - News flash animation
- `trumpet` - Trumpet animation
- `cyclecolors` - Cycling colors
- `thankyou` - Thank you animation
- `nosmoking` - No smoking animation
- `dontdrinkanddrive` - Don't drink and drive animation
- `fishimal` - Fish animation
- `fireworks` - Fireworks animation
- `turballoon` - Turbulent balloon
- `bomb` - Bomb explosion animation

### Text Positions
- `midline` - Middle line
- `topline` - Top line
- `botline` - Bottom line
- `fill` - Fill screen
- `left` - Left aligned
- `right` - Right aligned

## Home Assistant Integration

The LED Sign Controller has built-in MQTT auto-discovery for Home Assistant. When connected to your MQTT broker, it will automatically register as a device with multiple entities.

### Automatically Created Entities

- **LED Sign Message** (sensor) - Shows current message on the sign
- **LED Sign Display Text** (text) - Input to send text messages to the sign
- **LED Sign JSON Command** (text) - Input for advanced JSON commands
- **LED Sign** (light) - Control the sign as a light entity in Home Assistant
  - Turn ON: Displays "ON" in green
  - Turn OFF: Clears the sign
  - Can be used in scenes, automations, and scripts like any light entity
- **LED Sign Clear** (button) - Button to clear all messages
- **LED Sign Signal** (sensor) - WiFi signal strength with dBm unit
- **LED Sign Uptime** (sensor) - Device uptime in seconds

### Using in Home Assistant

1. Ensure your MQTT broker is configured in Home Assistant
2. Enable MQTT discovery in Home Assistant (ON by default)
3. The sign will automatically appear as a device with multiple entities
4. No manual configuration required - entities use the discovery prefix `homeassistant`
5. The device uses Last Will and Testament (LWT) for availability tracking

### Device Information

The LED Sign will appear in Home Assistant with the following device information:
- **Name**: LED Sign (device_id)
- **Identifiers**: Your device's unique ID (based on MAC address)
- **Model**: LED Sign Controller
- **Manufacturer**: Darke Tech Corp
- **SW Version**: Current firmware version

### Example Home Assistant Integration

#### Example Automation
```yaml
automation:
  - alias: "Show Weather on LED Sign"
    trigger:
      - platform: time_pattern
        minutes: "/30"
    action:
      - service: text.set_value
        target:
          entity_id: text.led_sign_display_text
        data:
          value: >-
            {"type":"normal","text":"Weather: {{ states('weather.forecast_home') }}, {{ states('sensor.temperature') }}°C","color":"green","mode":"hold"}
```

#### Example Dashboard Card
```yaml
type: vertical-stack
cards:
  - type: entities
    title: LED Sign Controller
    entities:
      - entity: sensor.led_sign_message
        name: Current Message
      - entity: light.led_sign
        name: Sign Display
      - entity: button.led_sign_clear
        name: Clear Sign
      - entity: text.led_sign_display_text
        name: Send Message
  - type: horizontal-stack
    cards:      - type: button
        tap_action:
          action: call-service
          service: text.set_value
          service_data:
            value: '{"type":"priority","text":"ALERT!","color":"red","mode":"flash"}'
          target:
            entity_id: text.led_sign_display_text
        name: Alert
        icon: mdi:alert
        hold_action:
          action: none
      - type: button
        tap_action:
          action: call-service
          service: text.set_value
          service_data:
            value: '{"type":"normal","text":"Welcome Home","color":"green","mode":"hold"}'
          target:
            entity_id: text.led_sign_display_text
        name: Welcome
        icon: mdi:home
      - type: button
        tap_action:
          action: call-service
          service: text.set_value
          service_data:
            value: '{"type":"normal","text":"Have a great day!","color":"amber","mode":"rotate"}'
          target:
            entity_id: text.led_sign_display_text
        name: Goodbye
        icon: mdi:hand-wave
  - type: entities
    entities:
      - entity: sensor.led_sign_uptime
      - entity: sensor.led_sign_signal
```

#### Example JSON Command
Send a complex message with JSON formatting:
```yaml
service: text.set_value
target:
  entity_id: text.led_sign_json_command
data:
  value: >-
    {"type":"normal","text":"Temperature: {{ states('sensor.temperature') }}°C","color":"green","mode":"hold","position":"midline","special":"twinkle"}
```

## Building the Project

This project uses PlatformIO for building and dependency management, making it easy to compile and flash to your ESP32/ESP8266 board.

### PlatformIO Setup

1. Install [Visual Studio Code](https://code.visualstudio.com/)
2. Install the [PlatformIO extension](https://marketplace.visualstudio.com/items?itemName=platformio.platformio-ide)
3. Clone this repository or download the source code
4. Open the project folder in Visual Studio Code
5. Create the required configuration files:
   - Copy `include/Credentials.h.example` to `include/Credentials.h` and edit as needed
   - Copy `include/dynamicParams.h.example` to `include/dynamicParams.h` if needed

### Building and Uploading

1. Connect your ESP32/ESP8266 board to your computer
2. In VS Code, click the PlatformIO icon in the sidebar
3. Click "Build" to compile the project
4. Click "Upload" to flash the firmware to your board
5. Open the serial monitor to view debug output (115200 baud)

### Customizing the Build

You can modify the `platformio.ini` file to customize the build process:

```ini
[env:esp32dev]
platform = espressif32
board = esp32dev
framework = arduino
monitor_speed = 115200
; Add build flags or library dependencies here
```

## Step-by-Step Setup Guide

### 1. Hardware Assembly
1. Connect your ESP32 to the level converter and LED sign using the wiring diagram above
2. Power up both the ESP32 and the LED sign

### 2. Initial Software Configuration
1. Flash the firmware to your ESP32 (using PlatformIO or Arduino IDE)
2. The device will create a WiFi access point named "LEDSign"
3. Connect to this access point using password "ledsign0"
4. A configuration portal will automatically open (or navigate to 192.168.50.1)
5. Configure your WiFi credentials, MQTT settings, and timezone
6. Save the configuration and the device will restart
7. The LED sign should now connect to your WiFi network

### 3. MQTT Setup
1. Ensure your MQTT broker is running and accessible
2. The sign will automatically connect using the configured credentials
3. Subscribe to `ledSign/#` to monitor all sign-related topics
4. Test the connection by sending a message to `ledSign/[DEVICE_ID]/message`

### 4. Home Assistant Integration
1. Make sure MQTT integration is enabled in Home Assistant
2. The LED Sign entities will appear automatically via MQTT discovery
3. Add the entities to your dashboard or create automations

## Troubleshooting

### Common Issues

1. **WiFi Connection Problems**
   - If the device can't connect to WiFi, it will create an access point named "LEDSign"
   - Connect to this AP and reconfigure WiFi settings
   - Check if your WiFi password is correct
   - Ensure the ESP32 is within range of your WiFi router
   - Try resetting the device by pressing the reset button multiple times

2. **MQTT Connection Issues**
   - Check MQTT broker address and credentials
   - Verify MQTT topics and message format
   - Check firewall settings that might block MQTT traffic
   - Test MQTT connection using another client (e.g., MQTT Explorer)
   - Inspect serial monitor output for MQTT connection errors
   - Verify your MQTT broker accepts the connection

3. **Sign Not Displaying Messages**
   - Verify wiring connections to the sign
   - Check for proper power to both ESP and sign
   - Ensure correct TX/RX pins are used (default GPIO16/17)
   - Verify the LED sign is set to the correct serial mode
   - Check if any JSON formatting errors exist in your messages
   - Try sending a simple text message first before using advanced features

4. **Timezone Issues**
   - Use correct POSIX format for your timezone
   - Example for US Mountain Time: "MST7MDT,M3.2.0/2,M11.1.0/2"
   - For other timezones, check [this timezone database](https://github.com/nayarsystems/posix_tz_db)
   - Look for NTP server errors in the serial monitor

5. **Home Assistant Discovery Not Working**
   - Ensure MQTT integration is set up in Home Assistant
   - Verify MQTT discovery is enabled in Home Assistant
   - Check MQTT broker connectivity
   - Verify the discovery prefix matches (default is "homeassistant")
   - Check if Home Assistant MQTT logs show any errors

### Serial Monitor Output

The controller outputs detailed diagnostic information to the serial monitor at 115200 baud. Connect using a serial terminal to view:

- Boot sequence and initialization
- WiFi connection status
- MQTT connection details
- Home Assistant discovery information
- Incoming and outgoing messages

Example debug output:
```
[WIFI] Connected to MyWiFi
[MQTT] Connecting to broker at 192.168.1.100:1883...
[MQTT] Connected with Client ID: LEDSign_A1B2C3
[HASS] Publishing Home Assistant discovery information...
[HASS] Publishing HASS config for LED Sign Message to homeassistant/sensor/ledsign_A1B2C3_message/config
[HASS] Publishing HASS config for LED Sign Display Text to homeassistant/text/ledsign_A1B2C3_display_text/config
...
[MQTT] Message received on topic: ledSign/A1B2C3/message
[PARSE] Processing message: [green,rotate]Hello World
```

## Advanced Configuration

### Customizing Settings

Edit `defines.h` to customize various aspects of the controller:

```cpp
// MQTT Configuration
#define MQTT_MAX_PACKET_SIZE      512
#define MQTT_KEEPALIVE            60

// Sign Defaults
#define SIGN_DEFAULT_COLOUR       BB_COL_AUTOCOLOR
#define SIGN_DEFAULT_POSITION     BB_DP_TOPLINE
#define SIGN_DEFAULT_MODE         BB_DM_AUTOMODE
#define SIGN_DEFAULT_SPECIAL      BB_SDM_INTERLOCK
#define SIGN_INIT_STRING          "Darke Tech Corp. 2025"

// Clock Configuration
#define SIGN_TIMEZONE_POSIX       "MST7MDT,M3.2.0/2,M11.1.0/2"
#define SIGN_SHOW_CLOCK_DELAY_MS  10000
#define SIGN_CLOCK_COLOUR         SIGN_DEFAULT_COLOUR
#define SIGN_CLOCK_POSITION       SIGN_DEFAULT_POSITION
#define SIGN_CLOCK_MODE           BB_DM_CLOCK
#define SIGN_CLOCK_SPECIAL        BB_SDM_TWINKLE

// Home Assistant MQTT
#define HASS_DISCOVERY_PREFIX     "homeassistant"
#define HASS_DISCOVERY_ENABLED    true
#define HASS_NODE_ID              "ledsign"
```

### WiFi Configuration

The controller uses ESP_WiFiManager_Lite to provide an easy-to-use configuration portal. You can customize the behavior by modifying these settings:

```cpp
#define TIMEOUT_RECONNECT_WIFI                 10000L
#define RESET_IF_CONFIG_TIMEOUT                true
#define CONFIG_TIMEOUT_RETRYTIMES_BEFORE_RESET 5
#define CONFIG_TIMEOUT                         120000L
```

### Multiple Reset Detection

The controller uses MultiResetDetector to enter configuration mode if the reset button is pressed multiple times:

```cpp
#define USING_MRD                     true
#define MRD_TIMEOUT                   10
```

## Firmware Updates

The LED Sign Controller supports Over-The-Air (OTA) updates, making it easy to update firmware without physical access to the device.

### Update Process

1. The controller checks for updates periodically
2. When an update is available, it will download and apply it automatically
3. The device will restart with the new firmware

### Custom Update Server

By default, the controller checks for updates at the following URLs:
```cpp
const char *otaVersionURL = "http://docker02.darketech.ca:8003/version.txt"; 
const char *otaFirmwareURL = "http://docker02.darketech.ca:8003/firmware.bin";
```

To use your own update server:
1. Edit these URLs in `main.cpp`
2. Host a version.txt file that contains only the version number (e.g., "0.0.6")
3. Host the firmware.bin file generated by your build process

### Manual Update

You can also update the firmware manually via USB using PlatformIO or Arduino IDE.

## Project Structure

```
ledSignController/
├── src/
│   ├── defines.h           # Main configuration constants
│   ├── main.cpp            # Main application code
│   ├── MessageParser.cpp   # Message parsing functionality
│   ├── MessageParser.h     # Message parsing header
│   ├── MQTTManager.cpp     # MQTT communication manager
│   └── MQTTManager.h       # MQTT manager header
├── include/
│   ├── Credentials.h       # WiFi and MQTT credentials
│   └── dynamicParams.h     # Dynamic parameters for config portal
├── lib/
│   ├── BETABRITE/          # Library for BetaBrite sign control
│   ├── OTAupdate/          # OTA update functionality
│   └── PubSubClient/       # MQTT client library
└── platformio.ini          # PlatformIO configuration
```

## References

This project builds on the Alpha Sign Communications Protocol (revision F), which can be found by searching online. The protocol hasn't been updated since 2006, indicating stability.

## Credits

- [BETABRITE library](https://github.com/ngsilverman/BETABRITE) for Arduino LED sign control
- [ESP_WiFiManager_Lite](https://github.com/khoih-prog/ESP_WiFiManager_Lite) for WiFi configuration portal
- [PubSubClient](https://github.com/knolleary/pubsubclient) for MQTT functionality
- [ArduinoJson](https://arduinojson.org/) for JSON parsing
- [Darke Tech Corp](https://darketech.ca/) for project development and maintenance

## Security Considerations

When deploying the LED Sign Controller, consider the following security best practices:

### Network Security
- Change the default AP password (`ledsign0`) during configuration
- Place the device on a separate IoT network/VLAN when possible
- Use a strong, unique password for your WiFi network

### MQTT Security
- Enable MQTT authentication and use strong, unique credentials
- Consider using TLS/SSL for MQTT connections in sensitive environments
- Restrict MQTT topic access using ACLs on your broker
- Regularly rotate MQTT passwords

### Physical Security
- Place the ESP32 and wiring in an enclosure to prevent tampering
- Consider the location of your LED sign to prevent unauthorized access

### Updates
- Keep the firmware updated with the latest security patches
- Regularly check for updates to the libraries used in this project

## License

This project is licensed under the MIT License - see the LICENSE file for details.

## Version History

- **0.0.6** - Added Home Assistant MQTT auto-discovery
- **0.0.5** - Added priority message handling
- **0.0.4** - Implemented JSON message format
- **0.0.3** - Added OTA update functionality
- **0.0.2** - Implemented MQTT control
- **0.0.1** - Initial release with basic functionality
