# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Architecture

This is an ESP32-based LED sign controller that connects BetaBrite/Alpha Protocol LED signs to WiFi and MQTT for remote message control. The system is built using PlatformIO with the Arduino framework.

### Core Components

- **Main Controller** (`src/main.cpp`): The primary application logic handling WiFi connectivity, MQTT communication, and sign control
- **BETABRITE Library** (`lib/BETABRITE/`): Custom library for communicating with BetaBrite LED signs over serial (RS232/TTL)
- **WiFi Management** (`ESP_WiFiManager_Lite`): Handles WiFi configuration portal and credential storage
- **MQTT Client**: Subscribes to topics for receiving display messages and publishes telemetry data
- **OTA Updates** (`lib/OTAupdate/`): Over-the-air firmware update capability

### Key Features

- **Message Parsing**: Supports rich text formatting with colors, animations, and effects using bracket notation `[color,effect]message`
- **Priority Messages**: Special handling for urgent messages using `*` prefix
- **Clock Display**: Automatic time display with NTP synchronization
- **Configuration Portal**: Web-based setup accessible when device is offline
- **Telemetry**: Publishes RSSI, IP address, and uptime via MQTT
- **Multi-Reset Detection**: Factory reset capability via multiple device resets

### Message Format Examples

```
[red]Emergency Message
[green,rotate]Normal rotating text
[amber,snow]Weather text with snow effect
*Priority alert message
#Clear all text files
^Factory reset
```

### MQTT Topics

- Subscribe: `ledSign/{DEVICE_ID}/message` and `ledSign/message`
- Publish: `ledSign/{DEVICE_ID}/rssi`, `ledSign/{DEVICE_ID}/ip`, `ledSign/{DEVICE_ID}/uptime`

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
- `platformio.ini`: PlatformIO project configuration
- `src/defines.h`: Sign controller specific defines and settings
- `include/Credentials.h`: WiFi and server credentials (not in repo)
- `include/dynamicParams.h`: MQTT configuration parameters

### Default Settings
- WiFi AP: "LEDSign" / "ledsign0"
- Serial: 115200 baud
- MQTT Port: 1883
- Time Zone: Mountain Time (MST7MDT)
- Sign Files: Maximum 5 text files (A-E)

## Important Notes

- The project uses a custom BETABRITE library for Alpha Protocol communication
- WiFi credentials and MQTT settings are configured via web portal
- Device ID is generated from MAC address
- OTA updates check version from remote server on startup
- Factory reset available via `^` command or multi-reset detection