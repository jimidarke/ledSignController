# Remote Controlled BetaBrite LED Sign Project Plan

## Project Overview

A P2P remote-controlled LED sign system with three main components:
1. **BetaBrite LED Sign**: Classic red/yellow/orange display with RJ45-serial interface
2. **Controller Module**: ESP32 + MAX232 serial interface, vehicle-powered (12V)  
3. **Remote Module**: ESP32 with OLED display, rotary encoder, push buttons, 18650 battery powered

## System Architecture (High-Level)

### Component Communication Flow
```
[Remote ESP32] --ESP-NOW--> [Controller ESP32] --Serial--> [BetaBrite Sign]
      ↑               ↑            ↑               ↑                ↑
  18650 Battery   3dBi Ant.  12V Vehicle    3dBi Ant.      7.5V from Controller
                              Power
```

### Power Distribution
```
Vehicle 12V → [5A Fuse] → [Reverse Protection Diode] → [Input Filter]
                                        ↓
            ┌───────────────────────────┴─────────────────────────────────┐
            │                                                             │
            │  Buck 1                            Buck 2                   │
            │12V→3.3V                    ┌─[P-MOSFET]─→ 12V→7.5V → BetaBrite Sign  
            │  @1A                       │                @6A             │
            │                           │                                │
            └─────┬─────────────────────┘                                │
                  ↓                                                      │
               ESP32 ←───[GPIO Control]──────────────────────────────────┘

Peak Power Analysis:
- BetaBrite: 5A @ 7.5V = 37.5W max
- ESP32: 0.3A @ 3.3V = 1W
- 12V Input: ~4A with converter losses
- 5A fuse provides safe operating margin

Remote Unit:
18650 Battery → [Charge Management IC] → 3.3V ESP32
                        ↑
                   USB Charging
```

## Development Todo List

### Core System Design
- [ ] Design overall system architecture and component interfaces
- [ ] Research ESP-NOW protocol implementation for P2P communication
- [ ] Design power management system (12V vehicle → 7.5V sign + 3.3V ESP32)
- [ ] Plan BetaBrite serial interface integration with MAX232 circuit

### Communication & Protocol
- [ ] Define communication protocol between remote and controller
- [ ] Create message format specification and priority handling

### Hardware Design
- [ ] Design remote control user interface (OLED + rotary encoder + 4 push buttons)
- [ ] Plan 18650 battery management and USB charging for remote

### Development Infrastructure
- [ ] Research Python libraries for development and testing tools
- [ ] Identify potential roadblocks and mitigation strategies

## Technical Specifications (Pseudo-Code)

### ESP-NOW Communication Protocol
```cpp
// Remote Unit Pseudo-Code
struct SignMessage {
    uint8_t message_type;      // PRESET, CUSTOM, PRIORITY, CLEAR
    uint8_t message_id;        // For acknowledgment
    char message_text[256];    // BetaBrite formatted message
    uint8_t priority;          // 0=normal, 1=priority, 2=emergency
    uint32_t checksum;         // Data integrity
};

// Controller Unit Pseudo-Code  
struct AckMessage {
    uint8_t message_id;        // Matching message_id
    uint8_t status;            // SUCCESS, FAILED, BUSY
    char error_message[64];    // If failed
};
```

### Power Management Pseudo-Code
```cpp
// Controller Power Management
class PowerManager {
    const int SIGN_POWER_PIN = 25;    // GPIO to control P-MOSFET
    
    void initPower() {
        pinMode(SIGN_POWER_PIN, OUTPUT);
        digitalWrite(SIGN_POWER_PIN, LOW);  // Sign OFF by default
    }
    
    void enableSign() {
        digitalWrite(SIGN_POWER_PIN, HIGH); // Turn on MOSFET
        delay(100);  // Let 7.5V rail stabilize
    }
    
    void disableSign() {
        digitalWrite(SIGN_POWER_PIN, LOW);  // Complete power cutoff
    }
    
    void checkVehicleStatus() {
        if (getInputVoltage() < 11.0) {
            disableSign();  // Protect vehicle battery
        }
        
        float inputCurrent = getInputCurrent();
        if (inputCurrent > 4.0) {  // 80% of 5A fuse rating
            reducePowerConsumption();
        }
    }
    
    void reducePowerConsumption() {
        // Reduce sign brightness to stay under fuse rating
        betabrite.SetBrightness(50);  // 50% brightness
    }
    
    float getInputVoltage();      // Monitor 12V input
    float getInputCurrent();      // Monitor 12V current draw
    bool isVehiclePowerGood();    // >11V threshold
};

// Remote Power Management  
class RemotePowerManager {
    void enableDeepSleep();       // ESP32 deep sleep between operations
    bool checkBatteryLevel();     // Monitor 18650 charge
    void enableChargingMode();    // USB charging detection
};
```

## Development Framework Decision

### Arduino/C++ Single Stack Approach
**Decision**: Use Arduino/C++ framework for all components to maintain simplicity and reliability.

**Benefits:**
- Single development environment and build system
- Leverages existing BETABRITE library without modification
- Proven ESP-NOW implementation with reliable timing
- Direct hardware access for optimal power management
- Simplified debugging and maintenance

### Remote Control User Interface Design

#### Hardware Components
- **0.96" OLED Display**: 128x64 SSD1306 (I2C interface)
- **Rotary Encoder**: KY-040 style with push button for navigation
- **4 Push Buttons**: 3 quick-access presets + 1 priority/emergency
- **Pin Usage**: 9 GPIO pins total (2 I2C + 3 encoder + 4 buttons)

#### Physical Layout
```
┌─────────────────┐
│   OLED Display  │  Status, menus, message preview
│    128 x 64     │  
├─────────────────┤
│    [ENCODER]    │  Navigation and text entry
├─────────────────┤
│ [1] [2] [3] [⚡] │  Quick presets + priority mode
└─────────────────┘
```

#### User Interface Flow
```cpp
class RemoteUI {
    enum MenuState {
        MAIN_MENU, PRESET_MENU, CUSTOM_MESSAGE, 
        PRIORITY_SELECT, SETTINGS, CONFIRM_SEND
    };
    
    // Quick access buttons - instant transmission
    void button1_press() {
        sendPresetMessage(1);  // "Good Morning"
        showTransmissionStatus();
    }
    
    void button4_press() {
        enterPriorityMode();   // Emergency/priority presets
        showPriorityMenu();
    }
    
    // Advanced functions via encoder navigation
    void encoder_longPress() {
        enterSettingsMode();   // Time setting, configuration
    }
    
    void handleRotaryTurn(int direction) {
        if (currentState == PRESET_MENU) {
            selectedPreset += direction;
            displayPresetMenu();
        }
        // Navigation through menus and text entry
    }
};
```

#### Display States Examples
```
Normal Mode:                Settings Mode:
┌──────────────────┐       ┌──────────────────┐
│ BetaBrite Remote │       │ ⚙ SETTINGS       │
│ Battery: 78% ●●●○│       │ > Set Time       │
│ Signal:  ●●●●○   │       │   Configure [1]  │
│ [1]Morning [2]Hi │       │   Configure [2]  │
│ [3]Thanks  [4]⚡  │       │ Press to select  │
└──────────────────┘       └──────────────────┘
```

#### Advanced Configuration Features
```cpp
class AdvancedSettings {
    void setCurrentTime() {
        // Encoder navigation for hours/minutes
        display.println("Set Hours: " + String(hours));
    }
    
    void configureFavorites() {
        // Program custom messages for buttons 1-3
        display.println("Program Button 1:");
        // Character selection via encoder
    }
    
    void testCommunication() {
        // Range testing and signal strength
        sendTestMessage();
        displaySignalQuality();
    }
};
```

### Arduino Development Tools
```cpp
// Serial configuration interface for development
void handleSerialCommands() {
    if (Serial.available()) {
        String cmd = Serial.readStringUntil('\n');
        
        if (cmd.startsWith("preset:")) {
            // Configure preset messages: "preset:1:Hello World"
            configurePresetMessage(cmd);
        }
        
        if (cmd.startsWith("test:")) {
            // Send test messages: "test:[red,flash]Test Message"
            sendTestMessage(cmd.substring(5));
        }
        
        if (cmd.startsWith("battery")) {
            // Report battery status
            Serial.println("Battery: " + String(getBatteryVoltage()) + "V");
        }
    }
}
```

## Identified Roadblocks & Mitigation Strategies

### Communication Challenges
**Roadblock**: RF interference in vehicle environment
- **Mitigation**: External 3dBi antennas with proper mounting and 50-ohm impedance
- **Strategy**: Reduced power transmission to extend battery life while maintaining reliability

**Roadblock**: Message delivery guarantees and acknowledgments  
- **Mitigation**: Implement retry logic with exponential backoff
- **Monitoring**: LED feedback on remote for transmission status

### Power Management Issues
**Roadblock**: Remote battery life with frequent transmissions
- **Mitigation**: Deep sleep modes, optimized transmission intervals
- **Strategy**: Implement message queuing to batch transmissions

**Roadblock**: Vehicle power fluctuations affecting sign stability
- **Mitigation**: P-MOSFET controlled power switching with complete sign isolation
- **Protection**: Reverse protection diode and under-voltage cutoff (11V threshold)
- **Efficiency**: Dual buck converter approach with zero parasitic drain when sign off

### Environmental Challenges  
**Roadblock**: Temperature extremes in vehicle mounting
- **Mitigation**: Temperature monitoring and thermal protection
- **Design**: Conformal coating for moisture protection

**Roadblock**: RF interference in vehicle environment
- **Mitigation**: Frequency hopping, error correction codes
- **Testing**: Comprehensive vehicle testing with various electronics

### Development Complexity
**Roadblock**: BetaBrite protocol timing requirements
- **Mitigation**: Hardware flow control, buffering strategies
- **Reference**: Existing BETABRITE library as foundation

## Next Steps

### Phase 1: Foundation (Week 1-2)
1. Set up development environment with PlatformIO
2. Select ESP32 modules with external antenna connectors (ESP32-WROOM-32U)
3. Test ESP-NOW basic communication between two ESP32s with external antennas
4. Validate BetaBrite serial interface with existing library
5. Design and test power conversion circuits with protection and filtering

### Vehicle Integration Specifications
#### Electrical Requirements
- **Input Voltage**: 10-16V (automotive range)
- **Fuse Rating**: 5A automotive blade fuse
- **Wire Gauge**: 14 AWG minimum for 5A continuous current
- **Peak Current**: 4A at 12V input (with converter losses)
- **Protection**: Reverse polarity, over-current, under-voltage

#### Wiring Recommendations
```
Vehicle Battery/Fuse Box
    ↓
[5A Automotive Fuse] ← Close to battery connection
    ↓
[14 AWG Wire] ← Red positive lead
    ↓
[Anderson Powerpole Connector] ← Weatherproof disconnect
    ↓
[Reverse Protection Diode] ← 10A Schottky (low voltage drop)
    ↓
[Input Filter Capacitors] ← 1000µF + 0.1µF for noise filtering
    ↓
Dual Buck Converters
```

#### Installation Options
- **Switched 12V**: Connect to ignition-controlled circuit (recommended)
- **Always-On 12V**: Connect to battery with voltage monitoring
- **Fuse Location**: Vehicle fuse box or inline fuse holder
- **Connector Type**: Anderson Powerpole or automotive blade connectors
- **Mounting**: Secure to vehicle chassis with proper strain relief

### Phase 2: Core Implementation (Week 3-4)  
1. Implement P2P message protocol with acknowledgments
2. Develop remote control interface with button handling
3. Integrate power management and battery monitoring
4. Create Python testing tools for protocol validation

### Phase 3: Integration & Testing (Week 5-6)
1. Full system integration testing
2. Vehicle environment testing (range, interference, temperature)
3. Battery life optimization and power management tuning
4. Create preset message library and configuration tools

### Phase 4: Enhancement & Deployment (Week 7-8)
1. OTA update capability for both ESP32 modules
2. Advanced message scheduling and priority handling
3. Mobile app integration planning (Bluetooth bridge)
4. Documentation and user manual creation

## Success Criteria

- [ ] 100+ meter range between remote and controller in open area
- [ ] 24+ hour battery life on remote with normal usage
- [ ] <1 second message transmission and display time
- [ ] Reliable operation in -20°C to +70°C temperature range
- [ ] 10+ preset messages with custom message capability
- [ ] Visual feedback on remote for all operations
- [ ] Graceful degradation when vehicle power is disconnected