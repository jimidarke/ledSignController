# LED Sign Controller - Custom PCB Design

## Project Overview

This directory contains the complete design documentation for a custom PCB with ESP32 dev board socket, MAX3232 serial interface, and 12V DC barrel jack power input for the BetaBrite LED Sign Controller project.

## Design Objectives

- **Modularity**: Socket-based design for ESP32-DevKitC board with easy replacement and upgrades
- **Power**: Support LED sign (7.5V @ 2.75A) and ESP32 dev board from 12V DC barrel jack input
- **Connectivity**: Compatible with ESP32-DevKitC variants (built-in or external antenna options)
- **User Interface**: Utilize dev board's built-in buttons and LED for operation and programming
- **Cost-Effective**: 2-layer PCB design with minimal components for low manufacturing cost
- **Robust**: Comprehensive protection circuits including reverse polarity and overcurrent protection
- **Serviceability**: Swappable ESP32 hardware for field service and development flexibility

## Power Requirements

| Component | Voltage | Current | Power | Notes |
|-----------|---------|---------|-------|-------|
| LED Sign | 7.5V | 2.75A | 20.6W | Actual measured draw vs 3.25A max |
| ESP32 Dev Board | 7.5V | 300mA | 2.25W | Including onboard regulator losses |
| MAX3232 + Misc | 3.3V | 200mA | 0.66W | Powered from dev board 3.3V output |
| **Total System** | | | **23.5W** | **12V DC @ 2A = 24W provides 2% headroom** |

## Key Features

### Power System
- 12V DC barrel jack input with reverse polarity protection
- 7.5V @ 3A buck converter for LED sign and ESP32 dev board
- Comprehensive input protection: fuse, reverse polarity diode, TVS surge protection
- ESP32 dev board provides regulated 3.3V for peripherals

### Microcontroller Interface
- Female pin socket for ESP32-DevKitC-32E/32UE boards
- Compatible with built-in PCB antenna or external antenna variants
- Direct access to dev board USB programming port
- All GPIO pins accessible via socket connection

### Serial Interface
- MAX3232 for RS232 level conversion
- Compatible with existing BetaBrite library
- Powered from dev board 3.3V output
- Standard terminal block output for LED sign connection

### User Interface
- Utilizes dev board's built-in boot button and reset button
- Dev board provides onboard power and status LEDs
- USB programming and serial monitor via dev board
- Optional external status indicators can be added via GPIO

### Protection & Reliability
- Input fuse and TVS diode protection
- Reverse polarity protection via Schottky diode
- ESD protection on exposed connectors
- Swappable ESP32 hardware for field service
- Thermal management via copper pours

## Document Organization

- **[power-system.md](power-system.md)** - Detailed power system design and calculations
- **[component-selection.md](component-selection.md)** - Component specifications and BOM
- **[schematic-guidelines.md](schematic-guidelines.md)** - Schematic design rules and blocks
- **[layout-guidelines.md](layout-guidelines.md)** - PCB layout constraints and routing
- **[mechanical-thermal.md](mechanical-thermal.md)** - Physical design and thermal management

## Target Specifications

| Parameter | Specification |
|-----------|---------------|
| **PCB Size** | 70mm x 50mm (target) |
| **Layer Count** | 2 layers (FR4, 1.6mm) |
| **Component Height** | <15mm for compact enclosure |
| **Operating Temperature** | -10°C to +60°C |
| **Input Voltage** | 12V ±10% via barrel jack |
| **Output Regulation** | 7.5V ±2%, 3.3V ±3% |
| **WiFi Range** | Extended via external antenna |
| **Serial Interface** | RS232 ±12V levels |

## Development Status

- [x] Requirements analysis and power calculations
- [x] Component research and selection
- [ ] Schematic capture (KiCad recommended)
- [ ] PCB layout and routing
- [ ] 3D mechanical modeling
- [ ] Design review and validation
- [ ] Prototype fabrication and testing
- [ ] Production optimization

## Manufacturing Approach

### Recommended PCB Specifications
- **Fabrication**: JLCPCB or similar low-cost manufacturer
- **Material**: Standard FR4, 1.6mm thickness
- **Copper**: 1oz base, 2oz for power traces
- **Surface Finish**: HASL (cost-effective)
- **Solder Mask**: Green (standard)
- **Silkscreen**: White for good contrast

### Assembly Options
- **Prototype**: Hand assembly or small-batch PCBA service
- **Production**: Automated PCBA with standard components
- **Component Sourcing**: LCSC basic parts library optimization

## Integration with Existing Project

This PCB design is fully compatible with the existing LED sign controller firmware:
- Uses standard ESP32-DevKitC boards with identical pin assignments
- BetaBrite library interfaces remain unchanged
- MQTT and WiFi functionality preserved
- OTA update capability maintained via dev board USB
- All existing features supported
- Easy migration from development breadboard setup

## Next Steps

1. Review and approve component selections
2. Begin schematic capture in KiCad
3. Create initial PCB layout
4. Generate manufacturing files
5. Order prototype PCBs for testing
6. Validate functionality and iterate design

## Contact & Contributions

This design documentation is part of the LED Sign Controller project. Refer to the main project README for contribution guidelines and project status.