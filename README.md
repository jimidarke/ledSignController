# LED Sign Controller

A comprehensive ESP32-based controller for BetaBrite/Alpha Protocol LED signs with WiFi connectivity, secure MQTT integration, and Alert Manager support for centralized notification management.

![Version](https://img.shields.io/badge/version-0.3.0-blue.svg)
![Platform](https://img.shields.io/badge/platform-ESP32-green.svg)
![License](https://img.shields.io/badge/license-MIT-yellow.svg)
![Build](https://img.shields.io/badge/build-PlatformIO-orange.svg)

## 🌟 Features

### Core Functionality
- **Alert Manager Integration**: JSON-based alert messages with centralized notification management
- **Priority Messaging**: Critical alerts that interrupt normal operation with configurable durations
- **Intelligent Display Presets**: Automatic formatting based on alert level and category
- **Zone-Based Routing**: Multi-sign deployments with independent zone control
- **Automatic Clock Display**: NTP-synchronized time display every 60 seconds (4-second duration)
- **Visual Error Reporting**: System errors displayed on sign for immediate user feedback
- **System Health Status**: Periodic health indicators with MQTT connectivity status
- **WiFi Portal**: Easy setup via captive portal when offline (non-blocking display sequence)
- **Over-The-Air Updates**: GitHub Releases-based firmware updates with HTTPS, SHA256 verification, and automatic rollback
- **Telemetry**: Real-time monitoring of device health and performance

### Message Features
- **JSON Message Format**: Structured alert messages with full protocol code access
- **Alert Levels**: Critical, warning, notice, and info severity levels
- **Alert Categories**: Security, weather, automation, system, network, and personal
- **Full BetaBrite Protocol**: Complete access to colors, modes, effects, charset, speed, and positioning
- **Display Presets**: Intelligent defaults when display_config is omitted
- **Message Queuing**: Automatic rotation through multiple message files

### Network & Security
- **Mutual TLS Authentication**: Certificate-based client authentication with CA verification
- **Secure MQTT (TLS 1.3)**: Encrypted communication on port 42690 (production) or 46942 (development)
- **Certificate Management**: SPIFFS-based certificate storage with graceful fallback
- **Zone-Based Topics**: Subscribe to `ledSign/{zone}/message` for targeted delivery
- **Persistent Sessions**: QoS Level 1 with clean session = false for reliable delivery
- **Network Resilience**: Automatic reconnection with exponential backoff and visual status
- **Memory Management**: Heap monitoring and leak detection
- **Error Recovery**: Comprehensive error handling with automatic recovery and sign display
- **Non-Blocking Architecture**: All operations use state machines to prevent system freezes

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

3. **Configure GitHub OTA Updates** (in `src/defines.h`)
   ```cpp
   // Update these with your GitHub repository
   #define GITHUB_REPO_OWNER         "yourusername"
   #define GITHUB_REPO_NAME          "ledSignController"
   ```

4. **Install TLS certificate** (required for production)
   ```bash
   # Extract CA certificate from broker (or copy your ca.crt)
   openssl s_client -connect alert.d-t.pw:42690 -showcerts </dev/null 2>/dev/null | \
     awk '/BEGIN CERT/,/END CERT/ {print}' | tail -n +24 > data/certs/ca.crt

   # Validate with test script
   ./tools/test-mqtt-auth.sh

   # Upload filesystem to ESP32
   pio run -t uploadfs
   ```
   See `data/certs/README.md` for detailed certificate instructions.

5. **Install GitHub token** (optional, for private repositories)
   ```bash
   # Create GitHub Personal Access Token at https://github.com/settings/tokens
   # Save token to data/github_token.txt
   echo "ghp_your_token_here" > data/github_token.txt

   # Upload filesystem to ESP32
   pio run -t uploadfs
   ```
   See `docs/OTA_DEPLOYMENT.md` for detailed OTA setup instructions.

6. **Build and upload firmware**
   ```bash
   pio run -t upload
   pio device monitor
   ```

7. **Initial setup**
   - Device creates WiFi hotspot "LEDSign" (password: "ledsign0")
   - Connect and configure: WiFi, MQTT server (alert.d-t.pw:42690), and Zone name
   - Device automatically connects and starts operation

## 📡 MQTT Integration

### Zone-Based Topics

The controller uses zone-based MQTT topics for targeted message delivery in multi-sign deployments.

| Topic | Direction | Purpose | QoS | Retained |
|-------|-----------|---------|-----|----------|
| `ledSign/{ZONE}/message` | Subscribe | Zone-specific alert messages (JSON) | 1 | No |
| `ledSign/{DEVICE_ID}/rssi` | Publish | WiFi signal strength | 0 | Yes |
| `ledSign/{DEVICE_ID}/ip` | Publish | Device IP address | 0 | Yes |
| `ledSign/{DEVICE_ID}/uptime` | Publish | Device uptime (seconds) | 0 | Yes |
| `ledSign/{DEVICE_ID}/memory` | Publish | Free memory (bytes) | 0 | Yes |

**Client ID Format**: `esp32-betabrite-{zone}-{mac}`
**Persistent Sessions**: Enabled (clean session = false)
**TLS Ports**: 42690 (production), 46942 (development)

### JSON Message Format

All messages must be in JSON format per the Alert Manager specification. See `test/sample_alerts.json` for comprehensive examples.

#### Minimal Alert (uses intelligent presets)
```json
{
  "title": "Low Disk Space",
  "message": "Server storage at 85% capacity",
  "level": "warning",
  "category": "system"
}
```

The system will automatically apply display presets based on level:
- **critical**: Red, flash, large text (10high), priority mode, 60s duration
- **warning**: Amber, scroll, normal text, 30s duration
- **notice**: Green, wipein, 20s duration
- **info**: Green, rotate, 15s duration

Category influences special effects (security=trumpet, weather=snow, etc.)

#### Full Alert with Display Configuration
```json
{
  "timestamp": 1704045600,
  "level": "critical",
  "category": "security",
  "title": "Intruder Alert",
  "message": "Motion detected at front door",
  "display_config": {
    "mode_code": "c",
    "color_code": "1",
    "charset_code": "6",
    "position_code": "0",
    "speed_code": "\\031",
    "effect_code": "Z",
    "priority": true,
    "duration": 60
  },
  "source": "security-system-01",
  "zone": "kitchen"
}
```

#### Protocol Code Reference
| Parameter | Options | Examples |
|-----------|---------|----------|
| **mode_code** | Display animation | `a`=rotate, `m`=scroll, `c`=flash, `r`=wipein, `A`=newsflash |
| **color_code** | Text color | `1`=red, `2`=green, `3`=amber, `9`=rainbow1 |
| **charset_code** | Font size | `3`=7high, `6`=10high (large), `5`=7highfancy |
| **position_code** | Vertical position | `0`=fill, `"`=topline, ` `=midline |
| **speed_code** | Animation speed | `\026`=slow (2), `\027`=medium (3), `\031`=fast (5) |
| **effect_code** | Special effect | `Z`=bomb, `0`=twinkle, `2`=snow, `B`=trumpet |

See `docs/BETABRITE.md` for complete protocol documentation.

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

#### Weather Alerts (JSON Format)
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
          topic: "ledSign/kitchen/message"  # Replace with your zone
          payload: >
            {
              "level": "warning",
              "category": "weather",
              "title": "Weather Alert",
              "message": "{{ trigger.to_state.attributes.weather_alerts[0].title }}"
            }
```

#### Daily Greetings (with Display Config)
```yaml
automation:
  - alias: "Morning Greeting"
    trigger:
      - platform: time
        at: "08:00:00"
    action:
      - service: mqtt.publish
        data:
          topic: "ledSign/kitchen/message"  # Replace with your zone
          payload: >
            {
              "level": "info",
              "category": "personal",
              "title": "Good Morning",
              "message": "Have a great day!",
              "display_config": {
                "mode_code": "r",
                "color_code": "2",
                "effect_code": "8",
                "duration": 20
              }
            }
```

#### Critical Security Alert
```yaml
automation:
  - alias: "Security Breach Alert"
    trigger:
      - platform: state
        entity_id: binary_sensor.front_door
        to: "on"
    action:
      - service: mqtt.publish
        data:
          topic: "ledSign/office/message"
          payload: >
            {
              "level": "critical",
              "category": "security",
              "title": "Intruder Alert",
              "message": "Motion detected at front door"
            }
```

## 🔧 Configuration

### WiFi Settings
Configured via captive portal when device is offline:
- SSID: Device's WiFi network name
- Password: Network password
- Static IP (optional): Manual IP configuration

### MQTT Settings
- **Server**: MQTT broker hostname/IP (default: alert.d-t.pw)
- **Port**: MQTT port (42690 for TLS production, 46942 for TLS development, 1883 for insecure fallback)
- **Username**: MQTT authentication username (optional with certificates)
- **Password**: MQTT authentication password (optional with certificates)
- **Zone**: Sign zone name for topic routing (default: "default")

### TLS Certificate Setup

For production Alert Manager integration, TLS certificates are required:

1. **Obtain certificates from Alert Manager administrator**
   - CA certificate (`ca.crt`) - Verifies broker identity
   - Client certificate (`client.crt`) - Authenticates this device
   - Client private key (`client.key`) - Private key for client cert

2. **Install certificates**
   ```bash
   # Place in data/certs/ directory
   cp ca.crt data/certs/
   cp client.crt data/certs/
   cp client.key data/certs/

   # Upload filesystem to ESP32
   pio run -t uploadfs
   ```

3. **Verify installation**
   - Watch serial monitor for: "MQTTManager: All certificates configured successfully"
   - If certificates fail to load, system falls back to insecure MQTT (port 1883)

See `data/certs/README.md` for detailed certificate management instructions.

### Sign Settings
Configurable in `src/defines.h`:

```cpp
// Sign configuration
#define SIGN_MAX_FILES 5                    // Number of message files
#define SIGN_DEFAULT_COLOUR BB_COL_GREEN    // Default text color
#define SIGN_DEFAULT_MODE BB_DM_ROTATE      // Default display mode

// Clock settings
#define SIGN_CLOCK_COLOUR BB_COL_AMBER      // Clock text color
#define SIGN_CLOCK_MODE BB_DM_HOLD          // Clock display mode
#define SIGN_TIMEZONE_POSIX "MST7MDT,M3.2.0,M11.1.0"  // Mountain Time

// Display timing (in main.cpp)
const unsigned long CLOCK_DISPLAY_INTERVAL = 60000;      // Show clock every 60 seconds
const unsigned long CLOCK_DISPLAY_DURATION = 4000;       // Clock shows for 4 seconds
const unsigned long HEALTH_CHECK_INTERVAL = 30000;       // System health check every 30 seconds

// Network timeouts
#define WIFI_TIMEOUT_MS 30000               // WiFi connection timeout
#define MQTT_RECONNECT_INTERVAL 5000        // MQTT reconnection interval
```

## 🛠️ Development

### Project Structure
```
ledSignController/
├── src/                          # Source code
│   ├── main.cpp                  # Main application with JSON alert handling
│   ├── MessageParser.h/.cpp      # DEPRECATED: Legacy bracket notation (v0.1.x)
│   ├── MQTTManager.h/.cpp        # MQTT with TLS and zone routing
│   ├── SignController.h/.cpp     # LED sign control with full protocol support
│   ├── SecureOTA.h               # PLANNED: Advanced OTA with signature verification
│   └── defines.h                 # Configuration constants (version, GitHub repo, OTA settings)
├── lib/                          # Project libraries
│   ├── BETABRITE/               # BetaBrite Alpha protocol implementation
│   ├── GitHubOTA/               # GitHub Releases-based OTA with HTTPS and SHA256 verification
│   ├── OTAupdate/               # DEPRECATED: Legacy OTA (v0.1.x)
│   └── README                   # Library documentation
├── include/                      # Header files and credentials
│   ├── dynamicParams.h          # WiFi portal parameters (MQTT, Zone)
│   └── Credentials.h            # WiFi credentials (not in repo)
├── data/                         # Filesystem data (uploaded via uploadfs)
│   ├── certs/                   # TLS certificates
│   │   ├── README.md            # Certificate installation guide
│   │   ├── ca.crt               # CA certificate (not in repo)
│   │   ├── client.crt           # Client certificate (not in repo)
│   │   └── client.key           # Private key (not in repo)
│   └── github_token.txt         # GitHub Personal Access Token for OTA (not in repo)
├── test/                         # Testing resources
│   └── sample_alerts.json       # Example alert messages for testing
├── docs/                         # Comprehensive documentation
│   ├── ESP32_BETABRITE_IMPLEMENTATION.md  # Alert Manager integration spec
│   ├── OTA_DEPLOYMENT.md        # GitHub-based OTA update deployment guide
│   ├── GITHUB_ACTIONS_OTA.md    # Automated releases with GitHub Actions
│   ├── CLIENTNODE.md            # Client node architecture
│   └── BETABRITE.md             # Complete protocol reference
├── platformio.ini               # PlatformIO configuration with SPIFFS
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

**Testing without TLS** (development only):
```bash
# Test with minimal JSON alert
mosquitto_pub -h localhost -p 1883 -t "ledSign/kitchen/message" \
  -m '{"level":"info","category":"system","title":"Test","message":"System operational"}'

# Test critical alert (triggers priority mode)
mosquitto_pub -h localhost -p 1883 -t "ledSign/kitchen/message" \
  -m '{"level":"critical","category":"security","title":"Test Alert","message":"Emergency test"}'
```

**Testing with TLS** (production):
```bash
# Test with TLS certificates
mosquitto_pub -h alert.d-t.pw -p 42690 \
  --cafile data/certs/ca.crt \
  --cert data/certs/client.crt \
  --key data/certs/client.key \
  -t "ledSign/kitchen/message" \
  -m @test/sample_alerts.json

# Validate from sample alerts file
jq '.alerts.info_system' test/sample_alerts.json | \
  mosquitto_pub -h alert.d-t.pw -p 42690 --cafile ca.crt --cert client.crt --key client.key \
  -t "ledSign/office/message" -l
```

### Contributing

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'Add amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

## 🔒 Security Considerations

### Current Security Features ✅
- **Mutual TLS Authentication**: Certificate-based client authentication (v0.2.0)
- **Encrypted Communication**: TLS 1.3 for MQTT on port 42690/46942
- **Certificate Management**: SPIFFS-based cert storage with validation
- **JSON Input Validation**: Strict message parsing with type checking
- **Buffer Overflow Protection**: Safe string handling with length limits
- **Exponential Backoff**: Connection retry with increasing delays
- **Memory Monitoring**: Heap usage tracking and leak detection
- **Secure Fallback**: Graceful degradation on certificate failure

### Security Recommendations
1. ✅ **Use TLS/SSL** for MQTT connections in production (implemented v0.2.0)
2. ⚠️ **Change default passwords** for WiFi portal
3. ✅ **Certificate-based auth** with unique device certificates (implemented v0.2.0)
4. ✅ **Secure OTA updates** via GitHub Releases with HTTPS and checksums (implemented v0.2.0)
5. ✅ **Network segmentation** for IoT devices (use VLANs/firewall rules)
6. ⚠️ **Certificate rotation** - Monitor expiration and renew before 5-year limit
7. ⚠️ **Physical security** - SPIFFS is not encrypted, physical access = certificate/token access
8. ⚠️ **GitHub token management** - Use minimal scope, rotate periodically, enable 2FA

### Certificate and Token Security
- **CA Certificate**: Shared across all devices, validates broker identity
- **Client Certificate**: Unique per device, identifies this ESP32 to broker
- **Private Key**: Unique per device, MUST be kept secret
- **GitHub Token**: Repository access token for OTA updates (private repos only)
- **Storage**: All stored in SPIFFS (unencrypted) - consider ESP32 flash encryption for high-security deployments
- **Expiration**: Certificates have typical 5-year validity - implement monitoring for renewal
- **Token Scope**: Use minimal scope (`repo` for private, `public_repo` for public)

### Planned Security Enhancements
- Cryptographic firmware signature verification
- ESP32 flash encryption for certificate protection
- Secure element integration (ATECC608A)
- Intrusion detection and tamper response

## 🔄 OTA Updates

### GitHub Releases-Based OTA System

The controller uses GitHub Releases for secure, reliable firmware distribution with automatic updates.

**Features**:
- ✅ **HTTPS-only downloads** with certificate validation
- ✅ **SHA256 checksum verification** to prevent corrupted firmware
- ✅ **Semantic versioning** (0.2.0 → 0.2.1) with automatic downgrade prevention
- ✅ **Automatic periodic checks** (default: every 24 hours, configurable)
- ✅ **LED sign feedback** during update process
- ✅ **Rollback protection** - previous firmware retained until new firmware validates
- ✅ **Public or private repository** support

### Configuration

1. **Set your GitHub repository** in `src/defines.h`:
   ```cpp
   #define GITHUB_REPO_OWNER         "yourusername"
   #define GITHUB_REPO_NAME          "ledSignController"
   #define FIRMWARE_VERSION          "0.2.0"
   ```

2. **For private repositories**, create a GitHub Personal Access Token:
   - Visit https://github.com/settings/tokens
   - Create token with `repo` scope
   - Save to `data/github_token.txt`
   - Upload: `pio run -t uploadfs`

3. **Configure update behavior** (optional):
   ```cpp
   #define OTA_CHECK_INTERVAL_HOURS  24      // Check frequency
   #define OTA_AUTO_UPDATE_ENABLED   true    // Auto-install updates
   #define OTA_BOOT_CHECK_ENABLED    false   // Check on boot
   ```

### Creating a Release

When you're ready to deploy a new firmware version:

```bash
# 1. Update version in src/defines.h
#define FIRMWARE_VERSION          "0.2.1"

# 2. Build firmware
pio run -t clean
pio run

# 3. Generate SHA256 checksum
cd .pio/build/esp32dev/
sha256sum firmware.bin > firmware.sha256

# 4. Create GitHub release with tag v0.2.1
#    Upload both firmware.bin and firmware.sha256 as release assets
```

**Using GitHub CLI**:
```bash
gh release create v0.2.1 \
  --title "Version 0.2.1" \
  --notes "Bug fixes and improvements" \
  .pio/build/esp32dev/firmware.bin \
  .pio/build/esp32dev/firmware.sha256
```

### Update Process

1. Device checks GitHub Releases API every 24 hours (configurable)
2. Compares latest release version with current firmware version
3. If newer version available:
   - Displays "CHECKING FOR UPDATES" on sign
   - Downloads `firmware.bin` over HTTPS
   - Downloads `firmware.sha256` checksum file
   - Displays "DOWNLOADING" on sign
   - Verifies checksum matches (prevents corruption)
   - Displays "INSTALLING XX%" with progress
   - Displays "VERIFYING CHECKSUM"
   - Flashes new firmware to ESP32
   - Displays "UPDATE COMPLETE - REBOOTING"
   - Reboots device with new firmware
4. If update fails at any stage, device continues running current firmware

### Serial Monitor Output

Successful update:
```
GitHubOTA: Periodic update check triggered
GitHubOTA: Latest release: 0.2.1
GitHubOTA: Update available: 0.2.0 -> 0.2.1
GitHubOTA: Downloading firmware from https://github.com/...
GitHubOTA: Firmware size: 896.5 KB
GitHubOTA: Progress: 50%
GitHubOTA: Download complete
GitHubOTA: Checksum verified OK
GitHubOTA: Firmware flashed successfully
GitHubOTA: Update successful, rebooting...
```

### Troubleshooting

See `docs/OTA_DEPLOYMENT.md` for comprehensive troubleshooting, including:
- GitHub API configuration issues
- Token authentication problems
- Checksum verification failures
- Network connectivity issues
- Partition scheme problems

### Security Notes

**Current Security Level: BASIC (Sufficient for most deployments)**

✅ Implemented:
- HTTPS downloads (prevents man-in-the-middle attacks)
- SHA256 checksums (prevents corrupted downloads)
- Semantic versioning (prevents accidental downgrades)
- GitHub token stored in SPIFFS (not hardcoded)

❌ Not Implemented (advanced security):
- Cryptographic signature verification (see `SecureOTA.h` for planned design)
- Would require private key management

**Best Practices**:
- Keep your GitHub account secure (enable 2FA)
- Use a private repository for production
- Rotate GitHub tokens periodically
- Consider ESP32 flash encryption for high-security deployments

### Automated Releases with GitHub Actions

**Highly Recommended**: Automate the entire release process!

Instead of manually building firmware, GitHub Actions can automatically:
- Build firmware when you push a version tag
- Generate checksums
- Create GitHub releases
- Upload files
- Validate versions and firmware size

**Simple workflow**:
```bash
# Update version in defines.h, then:
git tag v0.2.1 && git push origin v0.2.1
# Everything else happens automatically!
```

📖 See **[docs/GITHUB_ACTIONS_OTA.md](docs/GITHUB_ACTIONS_OTA.md)** for:
- Complete production-ready workflow
- Automated testing and validation
- Team collaboration features
- Advanced deployment strategies

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

#### Visual Status Indicators

The system now displays status and errors directly on the sign for immediate user feedback:

- **Clock Display**: Time shows every 60 seconds (4 seconds duration) when system is operational
- **System Health**: "System OK [MQTT OK] {IP}" displays every 5 minutes when healthy
- **MQTT Errors**: "MQTT: {status}" displays after 90 seconds of connection failure
- **NTP Errors**: "NTP Sync Failed" displays immediately when time sync fails
- **Offline Mode**: Non-blocking sequence shows WiFi credentials when disconnected

#### Common Issues

**Problem**: Device not connecting to WiFi
```
Solution:
1. Watch sign display - it will show "Connect to: LEDSign" with password
2. Check WiFi credentials in portal (192.168.50.1)
3. Verify 2.4GHz network (ESP32 doesn't support 5GHz)
4. Check signal strength and interference
```

**Problem**: MQTT messages not received
```
Solution:
1. Watch for error display on sign after 90 seconds
2. Check sign display for specific error: "MQTT: Bad Credentials", "MQTT: Connection Timeout", etc.
3. Verify MQTT broker accessibility and port (42690 for TLS)
4. Check topic names match your zone configuration
5. Monitor broker logs for connection attempts
6. Verify certificates are installed (pio run -t uploadfs)
```

**Problem**: Sign not displaying messages
```
Solution:
1. Check serial connection (RX/TX pins: GPIO 16/17)
2. Verify baud rate (115200)
3. Watch for clock display every 60 seconds (indicates sign is working)
4. Test with simple message via MQTT
5. Check sign power and protocol compatibility
```

**Problem**: Clock not displaying
```
Solution:
1. Verify NTP synchronization (watch for "NTP Sync Failed" error on sign)
2. Check internet connectivity and firewall rules (pool.ntp.org access)
3. Monitor serial output for NTP sync status
4. Clock displays every 60 seconds unless priority message is active
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

### handleMQTTMessage Function

Main message processing function - parses JSON alerts and displays them.

```cpp
/**
 * @brief Process incoming MQTT alert messages
 * @param topic MQTT topic (e.g., "ledSign/kitchen/message")
 * @param payload JSON message payload
 * @param length Payload length in bytes
 *
 * Supports:
 * - Full display_config for custom formatting
 * - Intelligent presets based on level/category
 * - Priority interrupts for critical alerts
 */
void handleMQTTMessage(char* topic, uint8_t* payload, unsigned int length);
```

### MQTTManager Class

Manages secure MQTT connection with TLS support and zone-based routing.

#### Constructor
```cpp
// Create MQTT manager with zone routing
MQTTManager(WiFiClient* wifi_client, const String& device_id, const String& zone_name = "default");
```

#### Methods
```cpp
// Configure MQTT connection with optional TLS
bool configure(const char* server, uint16_t port = 42690,
               const char* username = "", const char* password = "",
               bool use_tls = true);

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

// Get current zone name
String getZoneName() const;
```

### SignController Class

Controls BetaBrite LED sign with full protocol support.

#### Methods
```cpp
// Initialize the LED sign with default settings
bool begin();

// Display a message with full protocol control (v0.2.0)
bool displayMessage(const char* message, char color, char position, char mode, char special,
                   char charset = '3', const char* speed = "\027");

// Display a priority message with configurable duration (v0.2.0)
bool displayPriorityMessage(const char* message, unsigned int duration = 30);

// Check priority message timeout (non-blocking, v0.2.0)
void checkPriorityTimeout();

// Clear all text files on the sign
void clearAllFiles();

// Display current time on the sign
void displayClock(bool military_time = false);

// Handle system commands (clear, factory reset)
bool handleSystemCommand(char command);

// Check if currently displaying a priority message
bool isInPriorityMode() const;

// Cancel priority message and return to normal operation
void cancelPriorityMessage();
```

### MessageParser Class ⚠️ DEPRECATED

> **Note**: MessageParser is deprecated as of v0.2.0. The system now uses JSON-only format with ArduinoJson for parsing. This class is retained for reference only.

Previous bracket notation format `[color,mode]message` is no longer supported. Use JSON format instead.

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