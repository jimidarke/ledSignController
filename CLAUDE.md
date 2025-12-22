# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Architecture (v0.3.0)

This is an ESP32-based LED sign controller that connects BetaBrite/Alpha Protocol LED signs to WiFi and secure MQTT for **Alert Manager integration**. The system receives JSON-formatted alert messages with automatic display formatting based on severity level and category. Built using PlatformIO with the Arduino framework.

### Core Components

- **Main Controller** (`src/main.cpp`): JSON alert parsing, intelligent display presets, non-blocking priority message handling
- **BETABRITE Library** (`lib/BETABRITE/`): Custom library for BetaBrite Alpha Protocol over RS232/TTL serial
- **MQTTManager** (`src/MQTTManager.h/.cpp`): TLS-enabled MQTT client with optional authentication and zone-based routing
- **SignController** (`src/SignController.h/.cpp`): LED sign control with full protocol access (charset, speed, effects)
- **WiFi Management** (`tzapu/WiFiManager`): WiFi configuration portal with MQTT credentials and zone name
- **Certificate Storage** (`data/certs/`): LittleFS-based CA certificate storage (only ca.crt needed)
- **OTA Updates** (`lib/GitHubOTA/`): GitHub Releases-based over-the-air firmware updates with HTTPS and SHA256 verification

### Key Features (v0.3.0)

- **JSON Message Format**: Alert Manager compatible structured messages with `level`, `category`, `title`, `message`, and optional `display_config`
- **Intelligent Display Presets**: Automatic formatting based on alert level (critical/warning/notice/info) and category (security/weather/automation/system/network/personal)
- **Priority Messages**: Non-blocking critical alerts with configurable duration that interrupt normal operation
- **Full Protocol Control**: Access to all BetaBrite features including charset (font size), speed codes, positioning, colors, modes, and special effects
- **Zone-Based Routing**: Multi-sign deployments with topic `ledSign/{zone}/message` for targeted delivery
- **TLS Encryption**: Server-only TLS with CA certificate validation (port 42690)
- **Flexible Authentication**: Supports anonymous access or optional username/password authentication
- **Persistent MQTT Sessions**: QoS Level 1 with clean session = false for reliable message delivery
- **Graceful Fallback**: Automatic fallback to insecure MQTT (port 1883) if CA cert fails to load
- **Clock Display**: Automatic time display with NTP synchronization
- **Configuration Portal**: Web-based setup for WiFi, MQTT settings, and zone configuration
- **Telemetry**: Publishes RSSI, IP address, uptime, and free memory via MQTT
- **Secure OTA Updates**: GitHub Releases integration with HTTPS downloads, SHA256 checksum verification, semantic versioning, and automatic periodic checks
- **MQTT Test Tool**: `tools/test-mqtt-auth.sh` for validating TLS and connectivity

### Message Format (JSON Only - v0.2.0+)

⚠️ **IMPORTANT**: Bracket notation `[color,effect]message` is **DEPRECATED** as of v0.2.0. The system now **only** accepts JSON format.

**Minimal Alert** (uses intelligent presets):
```json
{
  "title": "Low Disk Space",
  "message": "Server storage at 85% capacity",
  "level": "warning",
  "category": "system"
}
```

**Full Alert** (with explicit display configuration):
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

**Display Presets by Level**:
- **critical**: Red, flash, 10high (large text), bomb effect, priority mode, 60s
- **warning**: Amber, scroll, 7high, 30s
- **notice**: Green, wipein, welcome effect, 20s
- **info**: Green, rotate, twinkle effect, 15s

**Category Effects**: Security=trumpet, Weather=snow, Automation=welcome, System/Network=twinkle, Personal=sparkle

### MQTT Topics (Zone-Based)

- **Subscribe**: `ledSign/{ZONE}/message` - Zone-specific alert messages (JSON, QoS 1)
- **Publish**: `ledSign/{DEVICE_ID}/rssi`, `ledSign/{DEVICE_ID}/ip`, `ledSign/{DEVICE_ID}/uptime` (QoS 0, retained)
- **Client ID Format**: `esp32-betabrite-{zone}-{mac}` (e.g., `esp32-betabrite-kitchen-aabbccddeeff`)
- **Persistent Sessions**: clean_session = false for reliability

## Development Commands

### Build and Upload
```bash
pio run                    # Build the project
pio run -t upload          # Upload to device
pio run -t uploadfs        # Upload filesystem (if using SPIFFS/LittleFS)
```

### Monitoring
```bash
pio device monitor         # Serial monitor at 115200 baud
pio run -t monitor         # Build, upload, and monitor
```

### Environment Management
```bash
pio run -e esp32dev        # Build for specific environment
pio run -t clean           # Clean build files
```

## Configuration

### Hardware Setup
- ESP32 board (esp32dev)
- Serial connection to BetaBrite sign (pins 16 RX, 17 TX)
- WiFi connectivity required for MQTT operations

### Key Configuration Files
- `platformio.ini`: PlatformIO project configuration with LittleFS filesystem and OTA-compatible partition scheme
- `src/defines.h`: Sign controller defines, TLS port (42690), certificate paths, zone defaults, OTA configuration
- `include/Credentials.h`: Placeholder (credentials managed by WiFiManager)
- `include/dynamicParams.h`: MQTT configuration parameters (server, port, user, pass, zone)
- `data/certs/ca.crt`: Certificate Authority root certificate (validates broker identity)
- `data/github_token.txt`: GitHub Personal Access Token for private repo OTA updates (not in repo, **KEEP SECURE**)

### Default Settings (v0.3.0)
- WiFi AP: "LEDSign" / "ledsign0"
- Serial: 115200 baud
- MQTT Server: alert.d-t.pw
- MQTT Port: 42690 (TLS), 1883 (insecure fallback)
- MQTT Auth: Anonymous (optional username/password via WiFiManager portal)
- MQTT QoS: Level 1 (at least once delivery)
- MQTT Packet Size: 2048 bytes (increased for JSON payloads)
- Time Zone: Mountain Time (MST7MDT)
- Sign Files: Maximum 5 text files (A-E)
- Default Zone: "CHANGEME"
- Firmware Version: 0.3.0 (defined in `defines.h`)
- OTA Check Interval: 24 hours (configurable in `defines.h`)
- OTA Auto-Update: Enabled by default

### MQTT Security (v0.3.0)

**Security Model**: Server-only TLS + Optional Authentication
- ESP32 validates broker identity via CA certificate (port 42690)
- Optional username/password authentication (configured via WiFiManager)
- Anonymous access supported when credentials left empty
- Topic ACLs on broker enforce zone-based permissions
- See `docs/MQTT_SECURITY.md` for full architecture and server setup

**Installation**:
```bash
# Extract CA certificate from broker
openssl s_client -connect alert.d-t.pw:42690 -showcerts </dev/null 2>/dev/null | \
  awk '/BEGIN CERT/,/END CERT/ {print}' | tail -n +24 > data/certs/ca.crt

# Or use the test script to validate
./tools/test-mqtt-auth.sh

# Upload filesystem to ESP32
pio run -t uploadfs
```

**Verification** (via serial monitor):
```
MQTTManager: LittleFS mounted successfully
MQTTManager: CA certificate loaded (2048 bytes)
MQTTManager: Server verification ready
MQTTManager: Connected to alert.d-t.pw:42690
```

**Fallback Behavior**:
- If CA cert fails to load, system falls back to insecure MQTT on port 1883
- Warning message: "MQTTManager: Warning - Certificate loading failed"
- Production Alert Manager requires TLS - insecure mode for testing only

### OTA Update Management (v0.2.0)

**GitHub Repository Setup**:
1. Edit `src/defines.h` and set your GitHub repository:
   ```cpp
   #define GITHUB_REPO_OWNER         "yourusername"
   #define GITHUB_REPO_NAME          "ledSignController"
   ```

2. For **private repositories**, create a GitHub Personal Access Token:
   - Go to https://github.com/settings/tokens
   - Create token with `repo` scope
   - Save token to `data/github_token.txt`
   - Upload to device: `pio run -t uploadfs`

**Creating Firmware Releases**:
```bash
# 1. Update version in defines.h
# 2. Build firmware
pio run

# 3. Generate checksum
cd .pio/build/esp32dev/
sha256sum firmware.bin > firmware.sha256

# 4. Create GitHub release with tag (e.g., v0.2.1)
#    Upload both firmware.bin and firmware.sha256 as release assets
```

**Verification** (via serial monitor):
```
OTA: SPIFFS mounted successfully
OTA: GitHub token loaded successfully
OTA: Manager initialized successfully
GitHubOTA: Initialized for username/repo, current version: 0.2.0
```

**Update Process**:
- Device checks for updates every 24 hours (configurable)
- Displays update progress on LED sign: "CHECKING FOR UPDATES" → "DOWNLOADING" → "INSTALLING XX%" → "UPDATE COMPLETE - REBOOTING"
- Uses semantic versioning (won't downgrade)
- Verifies SHA256 checksum before flashing
- Falls back to previous firmware if update fails

**GitHub Actions Automation** (Highly Recommended):
- Automate firmware builds and releases
- Simply push a tag: `git tag v0.2.1 && git push origin v0.2.1`
- GitHub Actions automatically builds, checksums, and releases
- See `docs/GITHUB_ACTIONS_OTA.md` for complete automation guide

See `docs/OTA_DEPLOYMENT.md` for complete deployment guide, troubleshooting, and security considerations.

## Important Notes (v0.3.0)

- ⚠️ **JSON-ONLY**: Bracket notation `[color,effect]message` is **DEPRECATED** - system only accepts JSON format
- ⚠️ **MessageParser is DEPRECATED**: The `MessageParser` class (bracket notation) is no longer used in main.cpp
- The project uses a custom BETABRITE library for Alpha Protocol communication with full protocol access
- WiFi credentials, MQTT settings, and **Zone Name** are configured via web portal
- Device ID is generated from MAC address
- Client ID format: `esp32-betabrite-{zone}-{mac}`
- Zone-based topics enable multi-sign deployments: each sign subscribes to `ledSign/{zone}/message`
- Alert level and category determine display formatting when `display_config` is omitted
- Priority messages use non-blocking state machine (no blocking delays in loop)
- TLS certificates and GitHub tokens are stored in SPIFFS (unencrypted) - consider ESP32 flash encryption for high-security deployments
- **OTA updates**: GitHub Releases-based with HTTPS, SHA256 verification, automatic periodic checks (every 24 hours), and semantic versioning
- **Partition scheme**: Uses `min_spiffs.csv` for OTA support (2x ~896KB app partitions + ~180KB SPIFFS)
- Configure GitHub repo owner/name in `src/defines.h` before building
- **GitHub Actions**: Automate releases with CI/CD pipeline - see `docs/GITHUB_ACTIONS_OTA.md`
- See `docs/OTA_DEPLOYMENT.md` for complete OTA deployment guide
- See `docs/ESP32_BETABRITE_IMPLEMENTATION.md` for complete Alert Manager integration specification
- See `test/sample_alerts.json` for 7 example alert messages covering all levels and categories
- pio commands are run on the host windows pc manually