# LED Sign Controller

A comprehensive ESP32-based controller for BetaBrite/Alpha Protocol LED signs with WiFi connectivity, MQTT integration, and advanced message management.

![Version](https://img.shields.io/badge/version-0.1.4-blue.svg)
![Platform](https://img.shields.io/badge/platform-ESP32-green.svg)
![License](https://img.shields.io/badge/license-MIT-yellow.svg)
![Build](https://img.shields.io/badge/build-PlatformIO-orange.svg)

## 🌟 Features

### Core Functionality
- **Remote Message Control**: Send messages via MQTT with rich formatting options
- **Priority Messaging**: Urgent messages that interrupt normal operation
- **Automatic Clock Display**: NTP-synchronized time display with timezone support
- **WiFi Portal**: Easy setup via captive portal when offline
- **Over-The-Air Updates**: Secure firmware updates with rollback protection
- **Telemetry**: Real-time monitoring of device health and performance

### Message Features
- **Rich Text Formatting**: Colors, animations, and special effects using bracket notation
- **Message Queuing**: Automatic rotation through multiple message files
- **System Commands**: Clear display, factory reset, and diagnostic functions
- **Input Validation**: Robust message parsing with security checks
- **Multi-Protocol Support**: BetaBrite Alpha protocol with RS232/TTL interface

### Network & Security
- **Secure MQTT**: TLS support with authentication and exponential backoff
- **Network Resilience**: Automatic reconnection with graceful degradation
- **Memory Management**: Heap monitoring and leak detection
- **Error Recovery**: Comprehensive error handling and automatic recovery

## 🚀 Quick Start

### Hardware Requirements

| Component | Specification | Notes |
|-----------|---------------|-------|
| **Microcontroller** | ESP32 Development Board | Tested with ESP32-DevKitC |
| **LED Sign** | BetaBrite or Alpha Protocol | RS232/TTL interface required |
| **Connections** | RX: GPIO 16, TX: GPIO 17 | 3.3V TTL levels |
| **Power** | 5V USB or external | Ensure adequate current for sign |

### Software Requirements

- [PlatformIO](https://platformio.org/) or Arduino IDE
- ESP32 board support package
- Libraries (automatically managed by PlatformIO):
  - ArduinoJson
  - PubSubClient
  - ESP_WiFiManager_Lite

### Installation

1. **Clone the repository**
   ```bash
   git clone https://github.com/your-org/ledSignController.git
   cd ledSignController
   ```

2. **Configure credentials** (create if not exists)
   ```cpp
   // include/Credentials.h
   #define SIGN_DEFAULT_SSID "LEDSign"
   #define SIGN_DEFAULT_PASS "ledsign0"
   // Add your specific settings here
   ```

3. **Build and upload**
   ```bash
   pio run -t upload
   pio device monitor
   ```

4. **Initial setup**
   - Device creates WiFi hotspot "LEDSign" (password: "ledsign0")
   - Connect and configure WiFi + MQTT settings
   - Device automatically connects and starts operation

## 📡 MQTT Integration

### Connection Topics

| Topic | Direction | Purpose | Retained |
|-------|-----------|---------|----------|
| `ledSign/{DEVICE_ID}/message` | Subscribe | Device-specific messages | No |
| `ledSign/message` | Subscribe | Broadcast messages | No |
| `ledSign/{DEVICE_ID}/rssi` | Publish | WiFi signal strength | Yes |
| `ledSign/{DEVICE_ID}/ip` | Publish | Device IP address | Yes |
| `ledSign/{DEVICE_ID}/uptime` | Publish | Device uptime (seconds) | Yes |
| `ledSign/{DEVICE_ID}/memory` | Publish | Free memory (bytes) | Yes |

### Message Format Examples

#### Basic Messages
```
Hello World
```

#### Rich Formatted Messages
```
[red]Emergency Alert
[green,rotate]Status: Online
[amber,snow]Winter Weather Advisory
[rainbow1,explode]Celebration Time!
```

#### Priority Messages (interrupt normal operation)
```
*URGENT: Building Evacuation Required
```

#### System Commands
```
#          Clear all messages
^          Factory reset device
```

### Display Capabilities

The BetaBrite LED sign supports extensive display customization through the Alpha Protocol. This project provides access to:

- **30+ Display Modes**: Including rotate, flash, scroll, explode, wipe effects, and roll animations
- **16+ Special Effects**: Atmospheric effects like snow, sparkle, fireworks, and themed animations
- **10+ Colors**: Standard colors, dimmed variants, rainbow patterns, and automatic color cycling
- **Multiple Character Sets**: From 5-pixel compact fonts to 10-pixel large displays with decorative options
- **Advanced Positioning**: Top, middle, bottom line positioning with fill and alignment controls
- **Priority Messaging**: Interrupt normal display rotation with urgent messages

📖 **Complete Reference**: See [BETABRITE.md](BETABRITE.md) for comprehensive documentation of all 200+ display options.

#### Quick Reference - Most Used Options

**Colors**: `red`, `amber`, `green`, `yellow`, `orange`, `rainbow1`, `autocolor`  
**Display Modes**: `rotate`, `hold`, `flash`, `scroll`, `rollup`, `explode`, `wipeleft`  
**Special Effects**: `twinkle`, `sparkle`, `snow`, `fireworks`, `welcome`, `starburst`, `newsflash`

## 🏠 Home Assistant Integration

### MQTT Discovery Configuration

Add to your Home Assistant `configuration.yaml`:

```yaml
mqtt:
  text:
    - name: "LED Sign Message"
      command_topic: "ledSign/YOUR_DEVICE_ID/message"
      icon: mdi:message-text
      
  sensor:
    - name: "LED Sign RSSI"
      state_topic: "ledSign/YOUR_DEVICE_ID/rssi"
      unit_of_measurement: "dBm"
      device_class: signal_strength
      
    - name: "LED Sign IP Address"
      state_topic: "ledSign/YOUR_DEVICE_ID/ip"
      icon: mdi:ip-network
      
    - name: "LED Sign Uptime"
      state_topic: "ledSign/YOUR_DEVICE_ID/uptime"
      unit_of_measurement: "s"
      device_class: duration

  button:
    - name: "Clear LED Sign"
      command_topic: "ledSign/YOUR_DEVICE_ID/message"
      payload_press: "#"
      icon: mdi:eraser
```

### Automation Examples

#### Weather Alerts
```yaml
automation:
  - alias: "Weather Alert to LED Sign"
    trigger:
      - platform: state
        entity_id: weather.home
        attribute: weather_alerts
    condition:
      - condition: template
        value_template: "{{ trigger.to_state.attributes.weather_alerts | length > 0 }}"
    action:
      - service: mqtt.publish
        data:
          topic: "ledSign/YOUR_DEVICE_ID/message"
          payload: "*[red,flash]Weather Alert: {{ trigger.to_state.attributes.weather_alerts[0].title }}"
```

#### Daily Greetings
```yaml
automation:
  - alias: "Morning Greeting"
    trigger:
      - platform: time
        at: "08:00:00"
    action:
      - service: mqtt.publish
        data:
          topic: "ledSign/YOUR_DEVICE_ID/message"
          payload: "[green,welcome]Good Morning! Have a great day!"
```

## 🔧 Configuration

### WiFi Settings
Configured via captive portal when device is offline:
- SSID: Device's WiFi network name
- Password: Network password
- Static IP (optional): Manual IP configuration

### MQTT Settings
- **Server**: MQTT broker hostname/IP
- **Port**: MQTT port (default: 1883)
- **Username**: MQTT authentication username
- **Password**: MQTT authentication password

### Sign Settings
Configurable in `src/defines.h`:

```cpp
// Sign configuration
#define SIGN_MAX_FILES 5                    // Number of message files
#define SIGN_DEFAULT_COLOUR BB_COL_GREEN    // Default text color
#define SIGN_DEFAULT_MODE BB_DM_ROTATE      // Default display mode
#define SIGN_SHOW_CLOCK_DELAY_MS 10000      // Clock display duration

// Clock settings
#define SIGN_CLOCK_COLOUR BB_COL_AMBER      // Clock text color
#define SIGN_CLOCK_MODE BB_DM_HOLD          // Clock display mode
#define SIGN_TIMEZONE_POSIX "MST7MDT,M3.2.0,M11.1.0"  // Mountain Time

// Network timeouts
#define WIFI_TIMEOUT_MS 30000               // WiFi connection timeout
#define MQTT_RECONNECT_INTERVAL 5000        // MQTT reconnection interval
```

## 🛠️ Development

### Project Structure
```
ledSignController/
├── src/                          # Source code
│   ├── main.cpp                  # Main application entry point
│   ├── MessageParser.h/.cpp      # Message parsing and validation
│   ├── MQTTManager.h/.cpp        # MQTT connection management
│   ├── SignController.h/.cpp     # LED sign control interface
│   ├── SecureOTA.h/.cpp          # Secure OTA update system
│   └── defines.h                 # Configuration constants
├── lib/                          # Project libraries
│   ├── BETABRITE/               # BetaBrite sign protocol
│   ├── OTAupdate/               # Legacy OTA (deprecated)
│   └── ArduinoJson/             # JSON parsing library
├── include/                      # Header files and credentials
├── docs/                         # Documentation and examples
├── platformio.ini               # PlatformIO configuration
└── README.md                    # This file
```

### Building from Source

1. **Install PlatformIO**
   ```bash
   pip install platformio
   ```

2. **Clone and build**
   ```bash
   git clone https://github.com/your-org/ledSignController.git
   cd ledSignController
   pio run
   ```

3. **Upload to device**
   ```bash
   pio run -t upload
   ```

4. **Monitor serial output**
   ```bash
   pio device monitor --baud 115200
   ```

### Code Style and Standards

- **C++ Standard**: C++11 minimum
- **Naming Convention**: snake_case for variables, PascalCase for classes
- **Documentation**: Doxygen-style comments for all public methods
- **Error Handling**: Comprehensive error checking with graceful degradation
- **Memory Management**: RAII principles, avoid dynamic allocation where possible

### Testing

#### Unit Testing
```bash
pio test
```

#### Integration Testing
```bash
# Test MQTT connectivity
mosquitto_pub -h localhost -t "ledSign/test/message" -m "[green]Test Message"

# Test system commands
mosquitto_pub -h localhost -t "ledSign/test/message" -m "#"
```

### Contributing

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'Add amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

## 🔒 Security Considerations

### Current Security Features
- Input validation and sanitization
- Buffer overflow protection
- Exponential backoff for connection attempts
- Memory usage monitoring

### Security Recommendations
1. **Use TLS/SSL** for MQTT connections in production
2. **Change default passwords** for WiFi portal
3. **Implement MQTT authentication** with unique credentials
4. **Regular updates** via secure OTA mechanism
5. **Network segmentation** for IoT devices

### Planned Security Enhancements
- Cryptographic firmware signature verification
- Certificate-based authentication
- Encrypted configuration storage
- Intrusion detection and response

## 🔄 OTA Updates

### Current Implementation
- HTTP-based version checking
- Automatic download and installation
- Basic error handling and recovery

### Secure OTA Roadmap
- HTTPS-only downloads with certificate validation
- Cryptographic signature verification
- Rollback capability for failed updates
- User consent and scheduling options
- Progress reporting and status updates

### Manual OTA Update
```bash
# Check current version
mosquitto_pub -h broker -t "ledSign/DEVICE_ID/command" -m "version"

# Trigger update check
mosquitto_pub -h broker -t "ledSign/DEVICE_ID/command" -m "update_check"
```

## 📊 Monitoring and Diagnostics

### Telemetry Data
The device publishes telemetry data every 60 seconds:

| Metric | Topic | Description |
|--------|-------|-------------|
| Signal Strength | `ledSign/{ID}/rssi` | WiFi RSSI in dBm |
| IP Address | `ledSign/{ID}/ip` | Current IP address |
| Uptime | `ledSign/{ID}/uptime` | Seconds since boot |
| Memory | `ledSign/{ID}/memory` | Free heap memory |

### Health Monitoring
```bash
# Subscribe to all telemetry
mosquitto_sub -h broker -t "ledSign/+/+" -v

# Monitor specific device
mosquitto_sub -h broker -t "ledSign/YOUR_DEVICE_ID/#" -v
```

### Troubleshooting

#### Common Issues

**Problem**: Device not connecting to WiFi
```
Solution: 
1. Check WiFi credentials in portal
2. Verify 2.4GHz network (ESP32 doesn't support 5GHz)
3. Check signal strength and interference
```

**Problem**: MQTT messages not received
```
Solution:
1. Verify MQTT broker accessibility
2. Check topic names and device ID
3. Validate MQTT credentials
4. Monitor broker logs for connection attempts
```

**Problem**: Sign not displaying messages
```
Solution:
1. Check serial connection (RX/TX pins)
2. Verify baud rate (115200)
3. Test with simple message: "Hello"
4. Check sign power and protocol compatibility
```

#### Debug Commands
```bash
# Enable verbose logging
mosquitto_pub -h broker -t "ledSign/DEVICE_ID/command" -m "debug_on"

# Get system status
mosquitto_pub -h broker -t "ledSign/DEVICE_ID/command" -m "status"

# Test sign connectivity
mosquitto_pub -h broker -t "ledSign/DEVICE_ID/message" -m "[green,flash]TEST"
```

## 📋 API Reference

### MessageParser Class

#### Methods
```cpp
// Validate message for safety and correctness
static bool validateMessage(const char* msg);

// Check if message is a system command (# or ^)
static bool isSystemCommand(const char* msg);

// Check if message is a priority message (*)
static bool isPriorityMessage(const char* msg);

// Parse message and extract display parameters
static bool parseMessage(const char* msg, char* color, char* position, 
                        char* mode, char* special, String* messageContent);
```

### MQTTManager Class

#### Methods
```cpp
// Configure MQTT connection parameters
bool configure(const char* server, uint16_t port = 1883, 
               const char* username = "", const char* password = "");

// Set message callback function
void setMessageCallback(std::function<void(char*, uint8_t*, unsigned int)> callback);

// Check if MQTT client is connected
bool isConnected() const;

// Publish a message to specified topic
bool publish(const char* topic, const char* message, bool retain = false);

// Publish telemetry data (RSSI, IP, uptime)
void publishTelemetry();

// Main loop function - call regularly to maintain connection
void loop();
```

### SignController Class

#### Methods
```cpp
// Initialize the LED sign with default settings
bool begin();

// Display a message with specified parameters
bool displayMessage(const char* message, char color, char position, char mode, char special);

// Display a priority message that interrupts normal operation
bool displayPriorityMessage(const char* message);

// Clear all text files on the sign
void clearAllFiles();

// Display current time on the sign
void displayClock(bool military_time = false);

// Handle system commands (clear, factory reset)
bool handleSystemCommand(char command);

// Check if currently displaying a priority message
bool isInPriorityMode() const;
```

## 📄 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## 👥 Authors and Acknowledgments

- **Primary Developer**: Darke Tech Corp
- **Contributors**: Open source community
- **Special Thanks**: 
  - BetaBrite protocol documentation contributors
  - ESP32 Arduino core developers
  - PlatformIO team

## 🔗 Related Projects

- [Home Assistant](https://www.home-assistant.io/) - Home automation platform
- [Node-RED](https://nodered.org/) - Visual programming for IoT
- [MQTT Explorer](https://mqtt-explorer.com/) - MQTT client and visualization
- [ESP32 Arduino Core](https://github.com/espressif/arduino-esp32) - Arduino support for ESP32

## 📞 Support

- **Issues**: [GitHub Issues](https://github.com/your-org/ledSignController/issues)
- **Discussions**: [GitHub Discussions](https://github.com/your-org/ledSignController/discussions)
- **Wiki**: [Project Wiki](https://github.com/your-org/ledSignController/wiki)

---

**Made with ❤️ for the IoT community**