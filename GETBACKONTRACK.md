# LED Sign Controller - Get Back On Track Summary

**Date:** 2025-12-22
**Version:** 0.2.0
**Status:** Reorganization Complete - Ready for Hardware Testing

---

## What Was Done

### Phase 1: Git Consolidation
- **Backed up** all branches with tags (`backup-master-pre-consolidation`, `backup-fromprod-pre-consolidation`)
- **Committed** v0.2.0 release with 53 files (+13,661/-10,166 lines)
- **Preserved** REST API work in `feature/rest-api-homeassistant` branch
- **Merged** fromprod (v0.2.0) into master
- **Cleaned up** old branches (fromprod, rest-api-documentation deleted)
- **Created** v0.2.0 tag

**Note:** Remote push pending - repository URL needs verification.

### Phase 2: Library Cleanup
| Before | After |
|--------|-------|
| Duplicate ArduinoJson (lib + include + managed) | Managed only (v6.21.3) |
| Local PubSubClient (2020) | Managed (v2.8) |
| ESP_WiFiManager_Lite (embedded header) | tzapu/WiFiManager (managed) |
| SPIFFS filesystem | LittleFS filesystem |

**Build Result:** SUCCESS
- RAM: 15.1% (49KB)
- Flash: 57.5% (1.1MB)

### Phase 3: Documentation Reorganization
| Action | File |
|--------|------|
| MOVED | `ALERTING_SYSTEM_PLAN.md` → `docs/external-services/ALERTING_SYSTEM_ARCHITECTURE.md` |
| MOVED | `CLIENTNODE.md` → `docs/external-services/ALERT_MANAGER_CLIENT_REFERENCE.md` |
| DELETED | `docs/DEV_GUIDE.md` (redundant with README.md) |
| DELETED | `src/README.md` (minimal, redundant with BETABRITE.md) |

---

## Current Architecture

### Files Structure
```
ledSignController/
├── src/
│   ├── main.cpp           # Main application (tzapu/WiFiManager)
│   ├── MQTTManager.cpp/h  # TLS MQTT with LittleFS certificates
│   ├── SignController.cpp/h  # BetaBrite control
│   └── defines.h          # Configuration
├── lib/
│   ├── BETABRITE/         # Custom Alpha Protocol library
│   └── GitHubOTA/         # Secure OTA updates
├── include/
│   ├── dynamicParams.h    # MQTT parameter storage
│   └── Credentials.h      # Placeholder (WiFiManager manages)
├── data/
│   └── certs/             # TLS certificates (LittleFS)
├── docs/
│   ├── external-services/ # Alert Manager references
│   └── *.md               # Core documentation
├── pcb-design/            # Hardware documentation
└── handheld-remote/       # Sub-project
```

### Dependencies (platformio.ini)
```ini
lib_deps =
    bblanchon/ArduinoJson@^6.21.3
    knolleary/PubSubClient@^2.8
    https://github.com/tzapu/WiFiManager.git
```

### Key Changes from v0.1
- **WiFi Configuration**: tzapu/WiFiManager (blocks until connected, auto-reconnect)
- **Filesystem**: LittleFS (ESP32 Core 3.x compatible)
- **Message Format**: JSON-only (bracket notation removed)
- **MQTT**: TLS on ports 42690/46942, certificates from LittleFS

---

## Remaining Tasks

### Phase 4: Hardware Testing (User Required)
1. Upload firmware: `pio run -t upload`
2. Upload filesystem: `pio run -t uploadfs` (with certs in `data/certs/`)
3. Test WiFi portal at 192.168.4.1 (SSID: LEDSign, Pass: ledsign0)
4. Test MQTT connection to alert.d-t.pw:42690
5. Test alert message display on BetaBrite sign

### Phase 5: Home Assistant Integration (Future)
- Create HADiscovery class for MQTT Discovery
- 10 entities: text, selectors, buttons, sensors
- See plan file for detailed specification

### Phase 6: Final Cleanup
- Commit all changes with v0.2.1 tag
- Push to remote (after verifying repo URL)
- Create GitHub Release

---

## Quick Reference

### Build Commands
```bash
pio run                    # Build firmware
pio run -t upload          # Upload to ESP32
pio run -t uploadfs        # Upload filesystem (certs)
pio device monitor         # Serial monitor
```

### WiFi Portal
- **SSID:** LEDSign
- **Password:** ledsign0
- **Portal IP:** 192.168.4.1 (auto-starts if no saved WiFi)

### MQTT Topics
- **Subscribe:** `ledSign/{zone}/message` (JSON alerts)
- **Publish:** `ledSign/{device_id}/rssi`, `/ip`, `/uptime`, `/memory`

### Alert JSON Format
```json
{
  "level": "warning",
  "category": "system",
  "title": "Alert Title",
  "message": "Alert details",
  "display_config": {
    "mode_code": "c",
    "color_code": "1",
    "priority": true,
    "duration": 30
  }
}
```

---

## Files Changed Summary

### Added
- `data/certs/` - Certificate storage structure
- `lib/GitHubOTA/` - Secure OTA library
- `docs/external-services/` - External service references
- `test/sample_alerts.json` - Test fixtures

### Removed
- `lib/ArduinoJson/` - Use managed
- `lib/PubSubClient/` - Use managed
- `lib/OTAupdate/` - Replaced by GitHubOTA
- `include/ESP_WiFiManager_Lite.h` - Use managed tzapu
- `include/ESP_MultiResetDetector.h` - Not needed
- `docs/DEV_GUIDE.md` - Redundant
- `docs/examples/` - Outdated

### Modified
- `platformio.ini` - New deps, LittleFS, build flags
- `src/main.cpp` - WiFiManager migration
- `src/MQTTManager.*` - SPIFFS→LittleFS
- `src/defines.h` - Simplified configuration
- `include/dynamicParams.h` - New format
- `include/Credentials.h` - Simplified

---

## Troubleshooting

### WiFi portal doesn't appear
- Reset device 3-5 times rapidly to trigger factory reset
- Or call `wifiManager.resetSettings()` via serial command

### MQTT connection fails
- Check certificates in `data/certs/` (ca.crt, client.crt, client.key)
- Verify port 42690 is reachable
- Check NTP time sync (TLS requires valid time)

### Build errors after changes
```bash
pio run --target clean
pio run
```

---

## Sources & References

- [tzapu/WiFiManager](https://github.com/tzapu/WiFiManager) - WiFi configuration
- [PlatformIO ESP32](https://docs.platformio.org/en/latest/platforms/espressif32.html) - Build system
- [ArduinoJson](https://arduinojson.org/) - JSON parsing
- [Home Assistant MQTT Discovery](https://www.home-assistant.io/docs/mqtt/discovery/) - HA integration (planned)

---

*Generated with Claude Code - 2025-12-22*
