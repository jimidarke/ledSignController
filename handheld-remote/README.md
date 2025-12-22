# BetaBrite Remote LED Sign Controller

Remote controlled BetaBrite LED sign system using ESP32 modules with P2P communication for vehicle and stationary applications.

## System Components

**Controller Module** (Vehicle-mounted)
- ESP32 with external 3dBi antenna
- MAX232 serial interface for BetaBrite communication
- 12V vehicle power with dual buck converters and P-MOSFET power control
- Buck 1: 12V→3.3V @1A (ESP32), Buck 2: 12V→7.5V @6A (BetaBrite sign)
- Zero parasitic drain with complete sign power isolation

**Remote Control** (Handheld) 
- ESP32 with external 3dBi antenna  
- 0.96" OLED display (128x64) for status and menus
- Rotary encoder with push button for navigation
- 4 dedicated push buttons (3 presets + 1 priority mode)
- 18650 battery with USB charging

**BetaBrite LED Sign**
- Classic red/yellow/orange display
- RJ45-serial interface connection
- Full Alpha Protocol support with animations and effects

## Features

- **P2P Communication**: Direct ESP-NOW protocol between remote and controller
- **Quick Access**: 3 programmable preset buttons for instant messaging
- **Visual Interface**: OLED display with rotary encoder navigation
- **Custom Messages**: Text entry via encoder character selection
- **Priority System**: Dedicated emergency button with priority message handling
- **Power Management**: Deep sleep optimization for 24+ hour operation
- **Intelligent Power Control**: P-MOSFET switching with vehicle battery protection
- **Status Display**: Real-time battery level, signal strength, transmission status
- **Zero Drain**: Complete sign power isolation when controller is off

## Quick Start

### Hardware Requirements
- ESP32 modules with external antenna connectors (ESP32-WROOM-32U)
- 0.96" OLED display (SSD1306, I2C interface)
- Rotary encoder with push button (KY-040 style)
- 4x tactile push buttons for quick access
- Dual buck converters: 12V→3.3V @1A and 12V→7.5V @6A
- P-channel MOSFET for power switching control  
- 5A automotive fuse and 14 AWG wiring for vehicle integration
- Reverse protection diode (10A Schottky) and input filtering
- Anderson Powerpole connectors for weatherproof connections
- [TBD] PCB designs for controller and remote
- 3dBi external antennas and mounting hardware

### Software Setup
**Framework**: Arduino/C++ with PlatformIO for simplified development and maximum reliability.

```bash
# Build and upload to controller
pio run -e controller -t upload

# Build and upload to remote  
pio run -e remote -t upload

# Monitor serial output for debugging
pio device monitor
```

## Message Format

### Basic Messages
```
[TBD] Simple text format
[TBD] Color and effect formatting  
[TBD] Priority message syntax
```

### Advanced Features  
```
[TBD] BetaBrite Alpha Protocol integration
[TBD] Animation and special effects
[TBD] Multi-line message support
```

## Configuration

### Controller Setup
**Power Connection:**
- Install 5A automotive fuse in vehicle fuse box or inline holder
- Run 14 AWG wire from fuse to controller mounting location
- Use Anderson Powerpole connectors for weatherproof connection
- Connect to switched 12V (ignition controlled) for automatic operation

**Sign Connection:**
- [TBD] BetaBrite serial cable connection via MAX232 interface
- [TBD] RJ45 to serial adapter requirements

**Mounting:**
- [TBD] Controller enclosure mounting to vehicle
- [TBD] External 3dBi antenna positioning for optimal range

### Remote Setup  
- [TBD] Battery installation and charging
- [TBD] Button configuration
- [TBD] Pairing with controller

## Development

### Build Environment
- PlatformIO with Arduino framework
- [TBD] Library dependencies (ESP-NOW, existing BETABRITE library)
- Serial interface for development and testing tools

### Testing
- [TBD] Range and interference testing
- [TBD] Battery life validation  
- [TBD] Protocol reliability testing

## Documentation

- [PROJECT_PLAN.md](PROJECT_PLAN.md) - Detailed technical specifications
- [BETABRITE.md](../BETABRITE.md) - BetaBrite display capabilities reference
- [TBD] Hardware assembly guide
- [TBD] User manual

## License

[TBD]

## Contributing

[TBD]