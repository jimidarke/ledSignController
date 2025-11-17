# LED Sign Controller - Improvement TODO List

Based on comprehensive codebase analysis. Issues are prioritized by impact on stability, security, and user experience.

## 🔴 CRITICAL PRIORITY - Security & Stability

### 1. Fix Buffer Overflow Vulnerabilities
- **File**: `src/main.cpp:371`
- **Issue**: Variable length array `char options[options_length]` on stack
- **Fix**: Replace with fixed-size buffer and bounds checking
- **Risk**: Stack overflow, crash, potential security exploit

### 2. Secure OTA Update System
- **File**: `lib/OTAupdate/OTAupdate.cpp`
- **Issues**: 
  - HTTP URLs (should be HTTPS)
  - No firmware signature verification
  - No version validation
  - No rollback capability
- **Fix**: Implement secure update protocol with verification

### 3. Input Validation for MQTT Messages
- **File**: `src/main.cpp:320-599` (parsePayload function)
- **Issue**: No sanitization of incoming messages
- **Risk**: Malformed messages can crash device
- **Fix**: Add input validation and length checks

### 4. Replace ESP.restart() with Graceful Recovery
- **File**: `src/main.cpp:648`
- **Issue**: Aggressive restart on MQTT connection failure
- **Fix**: Implement exponential backoff and recovery strategies

## 🟠 HIGH PRIORITY - User Experience & Features

### 5. Add JSON Message Support (Backwards Compatible)
- **New Feature**: Support both legacy string format and JSON
- **JSON Format**:
```json
{
  "message": "Hello World",
  "color": "red",
  "effect": "rotate",
  "priority": false,
  "duration": 30000
}
```
- **Implementation**: Use ArduinoJson library already included
- **Topics**: Add `ledSign/{ID}/json` topic

### 6. Home Assistant Auto-Discovery
- **New Feature**: MQTT Discovery for Home Assistant integration
- **Entities to Create**:
  - Switch: Online/Offline status
  - Text: Manual message input
  - Select: Effect/color selection
  - Button: Reset/clear sign
  - Sensor: RSSI, uptime, IP address
  - Button: Reboot device
- **Discovery Topic**: `homeassistant/text/ledsign_{ID}/config`

### 7. Enhanced MQTT Topics Structure
- **Current**: `ledSign/{ID}/message`
- **Add**:
  - `ledSign/{ID}/json` - JSON formatted messages
  - `ledSign/{ID}/command` - System commands (clear, reset, reboot)
  - `ledSign/{ID}/config` - Runtime configuration changes
  - `ledSign/{ID}/status` - Device status (online/offline/error)
  - `ledSign/{ID}/logs` - Remote logging

### 8. Improved Status Reporting
- **Add Health Monitoring**:
  - WiFi connection quality
  - MQTT connection status
  - Memory usage
  - Error counts
  - Last message timestamp
- **Status Topics**: Publish device health every 30 seconds

### 9. Content Management System
- **Features**:
  - Scheduled messages (show joke at 3pm daily)
  - Message templates library
  - Rotation through multiple data feeds
  - Time-based content (weather in morning, news at lunch)
- **Storage**: Use SPIFFS/LittleFS for message queue

## 🟡 MEDIUM PRIORITY - Code Quality & Maintainability

### 10. Refactor Massive parsePayload() Function
- **File**: `src/main.cpp:320-599` (279 lines!)
- **Current**: Giant if-else chain for option parsing
- **Fix**: 
  - Create separate MessageParser class
  - Use lookup table for options
  - Split into smaller, focused functions

### 11. Implement Proper Logging System
- **Current**: Only Serial.print() statements
- **Add**:
  - Log levels (DEBUG, INFO, WARN, ERROR)
  - Remote logging via MQTT
  - Structured logging with timestamps
  - Configurable verbosity
- **New File**: `lib/Logger/Logger.h`

### 12. Configuration Management Overhaul
- **Current**: Settings scattered across multiple files
- **New System**:
  - Centralized configuration class
  - Runtime parameter changes via MQTT
  - Configuration backup/restore
  - Factory reset with configuration preservation options
- **Web Interface**: Expand beyond basic WiFi settings

### 13. MQTT Connection Reliability
- **Issues**: 
  - No exponential backoff (line 639)
  - No last will and testament
  - No QoS configuration
- **Improvements**:
  - Implement exponential backoff (1s, 2s, 4s, 8s, max 60s)
  - Add LWT for online/offline status
  - Configure QoS 1 for critical messages
  - Add connection state callbacks

### 14. Memory Management
- **Add**: 
  - Heap monitoring and reporting
  - Graceful handling of low memory conditions
  - Memory usage optimization
  - Stack overflow detection

## 🟢 LOW PRIORITY - Nice to Have Features

### 15. Web Dashboard
- **Feature**: Local web interface for device management
- **Capabilities**:
  - Real-time sign preview
  - Message history
  - Configuration management
  - System diagnostics
  - OTA update interface

### 16. Message Scheduling & Automation
- **Features**:
  - Cron-like scheduling for recurring messages
  - Conditional logic (if weather < 32°F, show "Stay warm!")
  - Message priorities and queuing
  - A/B testing for message effectiveness

### 17. Integration Enhancements
- **APIs**: 
  - REST API for external integrations
  - WebSocket for real-time updates
  - GraphQL endpoint for complex queries
- **Protocols**: 
  - CoAP support for IoT integration
  - LoRaWAN for remote locations

### 18. Advanced Display Features
- **Multi-zone Support**: Different content areas
- **Animations**: Custom animation sequences
- **Graphics**: Simple bitmap display support
- **Sound Integration**: Audio alerts with messages

### 19. Analytics & Insights
- **Metrics**:
  - Message display duration
  - Popular message types
  - Device uptime statistics
  - Network performance metrics
- **Reporting**: Daily/weekly summaries via email/MQTT

### 20. User Experience Improvements
- **Mobile App**: Companion app for message control
- **Voice Control**: Integration with Alexa/Google Assistant
- **Template Library**: Pre-built message templates
- **Preview Mode**: Test messages before displaying

## 📋 Implementation Strategy

### Phase 1: Security & Stability (Weeks 1-2)
- Items 1-4: Critical security fixes
- Items 10, 13: Core stability improvements

### Phase 2: Core Features (Weeks 3-4)
- Items 5-6: JSON support and Home Assistant integration
- Items 7-8: Enhanced MQTT and status reporting

### Phase 3: User Experience (Weeks 5-6)
- Items 9, 11-12: Content management and configuration
- Item 15: Basic web dashboard

### Phase 4: Advanced Features (Weeks 7+)
- Items 16-20: Advanced features based on usage feedback

## 🛠️ Technical Recommendations

### File Structure Refactoring
```
src/
├── main.cpp (simplified, orchestration only)
├── network/
│   ├── WiFiManager.cpp
│   ├── MQTTClient.cpp
│   └── WebServer.cpp
├── display/
│   ├── MessageParser.cpp
│   ├── EffectManager.cpp
│   └── ContentManager.cpp
├── config/
│   ├── Configuration.cpp
│   └── HADiscovery.cpp
└── utils/
    ├── Logger.cpp
    └── HealthMonitor.cpp
```

### New Dependencies to Consider
- **AsyncMQTTClient**: Better MQTT reliability
- **ESPAsyncWebServer**: Non-blocking web interface
- **Preferences**: NVS storage for configuration
- **time.h**: Better time handling for scheduling

### Testing Strategy
- Unit tests for message parsing
- Integration tests for MQTT communication
- Load testing for stability
- Security testing for vulnerabilities

### Backwards Compatibility
- Maintain support for existing string-based MQTT messages
- Gradual migration path for integrations
- Configuration migration utilities
- Documentation for both old and new APIs

---

**Total Estimated Effort**: 6-8 weeks for full implementation
**Minimum Viable Improvements**: Items 1-8 (4 weeks)
**Recommended Starting Point**: Items 1-6 for immediate impact