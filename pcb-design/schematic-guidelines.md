# Schematic Design Guidelines

## Overview

This document provides detailed schematic design guidelines for the LED Sign Controller PCB, including circuit topology, component interconnections, and design rules for optimal performance and manufacturability.

## Schematic Organization

### Hierarchical Structure
```
Top Level: System Overview
├── Power Management Sheet
│   ├── USB-C PD Controller
│   ├── 7.5V Buck Converter
│   ├── 3.3V LDO Regulator
│   └── Protection Circuits
├── Microcontroller Sheet
│   ├── ESP32 Dev Board Socket
│   ├── Power Interface (7.5V to dev board)
│   ├── GPIO Connections
│   └── 3.3V Distribution from dev board
├── Serial Interface Sheet
│   ├── MAX3232 Level Converter
│   ├── Charge Pump Capacitors
│   └── Connector Interfaces
└── Interface Connections Sheet
    ├── Serial Terminal Blocks
    ├── Power Input/Output
    └── Dev Board Interface Signals
```

### Sheet Naming Convention
- **Sheet 1**: `Power_Management` - All power generation and distribution
- **Sheet 2**: `Dev_Board_Interface` - ESP32 dev board socket and power interface
- **Sheet 3**: `Serial_Interface` - RS232 level conversion and I/O
- **Sheet 4**: `External_Connections` - All connectors and mechanical interfaces

## Power Management Schematic

### 12V Barrel Jack Input Circuit
```
Design Guidelines:
1. Place barrel jack at board edge for accessibility
2. Implement comprehensive protection: fuse, reverse polarity, surge
3. Include test points for voltage monitoring at each protection stage
4. Minimize power loss while providing robust protection
5. Use standard 2.1mm × 5.5mm barrel jack, center positive

Protection Strategy:
- F1: 3A fuse for overcurrent protection
- D1: Schottky diode for reverse polarity protection
- D2: TVS diode for surge/transient protection
- C1, C2: Input filtering and bulk capacitance
```

#### Barrel Jack Protection Circuit
```
12V Barrel Jack (J1)
     │
     ├─ Center (+) ── F1 (3A Fuse) ── D1 (MBRS340) ── D2 (TVS) ── C1,C2 ── 12V_Protected
     │                                    │            │
     └─ Shell (GND) ──────────────────────┴────────────┴──────── GND

Component Values:
- F1: Littelfuse 0456003.MR (3A fast-blow, 1206)
- D1: MBRS340T3G (3A, 40V Schottky, DO-201AD)
- D2: SMBJ15A (15V TVS, SMB package)
- C1: 100µF, 25V electrolytic (bulk storage)
- C2: 0.1µF, 50V ceramic (HF filtering)

Test Points:
- TP1: 12V input (post-barrel jack, pre-fuse)
- TP2: 12V fused (post-fuse, pre-diodes)
- TP3: 12V protected (final filtered 12V rail)
```

### 7.5V Buck Converter Circuit (TPS54331)
```
Design Guidelines:
1. Minimize switching node area/length
2. Place inductor close to SW pin
3. Use ground plane for heat dissipation
4. Include compensation network
5. Add enable control and power-good output

Feedback Network:
Vout = 0.8V × (1 + R1/R2)
For 7.5V: R1 = 56kΩ (1%), R2 = 12kΩ (1%)
Compensation: RC = 22nF, CC = 470pF
```

#### TPS54331 Reference Circuit
```
12V Protected ── C_in (47µF) ── VIN [TPS54331] SW ── L1 (22µH) ── C_out ── 7.5V Output
                                     │                              (220µF + 22µF)
                                     ├─ BOOT ── C_boot (0.1µF) ──┘
                                     ├─ SS ── C_ss (10nF) ── GND
                                     ├─ COMP ── RC//CC network
                                     ├─ FB ──── R1/R2 divider ──── 7.5V sense
                                     ├─ EN ── R_pullup ── 12V Protected
                                     └─ PGND, AGND ── GND plane

Improved Performance with 12V Input:
- Higher efficiency: >93% vs 89% with 9V input
- Better regulation under load transients
- Lower input current reduces I²R losses
- More headroom for buck converter operation
```

### 3.3V LDO Regulator Circuit (TPS73633)
```
Design Guidelines:
1. Input from 7.5V rail for low dropout
2. Adequate input/output capacitance
3. Thermal pad connection to ground
4. Enable control from power sequencing
5. Power-good output for system status

Stability Requirements:
- Input capacitor: 10µF minimum
- Output capacitor: 47µF with low ESR
- ESR range: 5mΩ to 200mΩ for stability
```

#### TPS73633 Reference Circuit
```
7.5V Input ── C_in (10µF) ── IN [TPS73633] OUT ── C_out (47µF + 0.1µF) ── 3.3V Output
                                 │
                                 ├─ EN ── R_pullup ── 7.5V (delayed enable from buck PG)
                                 ├─ NR ── C_nr (10nF) ── GND (noise reduction)
                                 └─ GND, PAD ── Thermal pad ── GND plane
```

## Dev Board Interface Schematic

### ESP32-DevKitC Socket Interface
```
Design Guidelines:
1. Standard 2x15 pin female headers for ESP32-DevKitC compatibility
2. 7.5V power delivery to dev board VIN pin
3. 3.3V power extraction from dev board for peripherals
4. GPIO access via socket pins for serial interface
5. Mechanical stability with proper pin spacing (2.54mm pitch)

Socket Pin Configuration:
Left Side (J6A):     Right Side (J6B):
Pin 1: 3V3           Pin 1: GND
Pin 2: EN            Pin 2: GPIO23
Pin 3: GPIO36        Pin 3: GPIO22 (I2C SCL)
Pin 4: GPIO39        Pin 4: GPIO1 (TX)
Pin 5: GPIO34        Pin 5: GPIO3 (RX)
Pin 6: GPIO35        Pin 6: GPIO21 (I2C SDA)
Pin 7: GPIO32        Pin 7: GND
Pin 8: GPIO33        Pin 8: GPIO19
Pin 9: GPIO25        Pin 9: GPIO18
Pin 10: GPIO26       Pin 10: GPIO5
Pin 11: GPIO27       Pin 11: GPIO17 (TX2) → MAX3232 RX
Pin 12: GPIO14       Pin 12: GPIO16 (RX2) → MAX3232 TX
Pin 13: GPIO12       Pin 13: GPIO4
Pin 14: GND          Pin 14: GPIO0
Pin 15: GPIO13       Pin 15: GPIO2

Additional connections:
- VIN (dev board) ← 7.5V from buck converter
- 3V3 (dev board) → MAX3232 power supply
```

#### Power Interface to Dev Board
```
Power Delivery:
7.5V Buck Output → Dev Board VIN pin
- Current capability: 300mA maximum for dev board
- Voltage range: 6V-12V acceptable, 7.5V optimal
- Dev board regulates to 3.3V internally

Power Extraction:
Dev Board 3.3V → MAX3232 + Peripherals
- Available current: 800mA from dev board AMS1117
- Used current: ~200mA for MAX3232 and support circuits
- Margin: 600mA available for expansion
```

#### GPIO Signal Routing
```
Critical Connections:
- GPIO16 (RX2) → MAX3232 T1IN (ESP32 receives from sign)
- GPIO17 (TX2) → MAX3232 R1OUT (ESP32 transmits to sign)
- 3.3V from dev board → MAX3232 VCC
- GND connections → Common ground plane

Optional Connections (available on socket):
- GPIO18: External status LED (optional)
- GPIO19, GPIO23: External buttons (optional)
- GPIO21, GPIO22: I2C expansion
- Other GPIOs: Available via socket for future expansion
```

### No Additional Circuits Required
```
Eliminated from PCB design:
- ESP32 module and associated passives
- 32.768kHz crystal and load capacitors
- Programming interface (handled by dev board USB)
- User buttons and status LEDs (on dev board)
- 3.3V regulator (provided by dev board)
- ESP32 decoupling capacitors
- Boot/reset circuitry (handled by dev board)

Benefits:
- Reduced PCB complexity
- Lower component cost
- Easier assembly and testing
- Swappable ESP32 hardware
- USB programming via dev board
```

## Serial Interface Schematic

### MAX3232 Level Converter Circuit
```
Design Guidelines:
1. Charge pump capacitors close to IC
2. All capacitors same voltage rating (25V minimum)
3. Series termination resistors on RS232 lines
4. ESD protection on exposed connector pins
5. Status LEDs for TX/RX activity (optional)

Charge Pump Configuration:
C1+, C1-: 0.1µF (charge pump 1)
C2+, C2-: 0.1µF (charge pump 2)
V+, V-: ±12V rails generated internally
VCC: 3.3V supply, decoupled with 0.1µF
```

#### MAX3232 Connection Diagram
```
ESP32 Side (TTL Levels):
GPIO16 (RX) ── R1IN [MAX3232] T1OUT ── R_term (120Ω) ── RS232 RX
GPIO17 (TX) ── T1IN [MAX3232] R1OUT ── R_term (120Ω) ── RS232 TX

Control Signals:
RTS_OUT ── T2IN [MAX3232] R2OUT ── RS232 RTS (optional)
CTS_IN  ── R2IN [MAX3232] T2OUT ── RS232 CTS (optional)

RS232 Connector (J5):
Pin 2: TX (T1OUT)
Pin 3: RX (R1OUT)
Pin 5: GND
Pin 7: RTS (T2OUT) - optional
Pin 8: CTS (R2IN) - optional
```

## User Interface Schematic

### RGB Status LED Circuit (WS2812B)
```
Design Guidelines:
1. Single data line control from ESP32
2. Adequate power supply decoupling
3. Current limiting not required (internal control)
4. Short data line with series resistor for noise immunity
5. Ground plane return path

Circuit Configuration:
3.3V ── VDD [WS2812B] DOUT ── (future expansion)
GPIO18 ── R_series (470Ω) ── DIN
GND ── VSS, GND [WS2812B]
Decoupling: 0.1µF ceramic capacitor close to VDD pin
```

### User Button Circuits
```
Design Guidelines:
1. Pull-up resistors to prevent floating inputs
2. Debouncing handled in software
3. ESD protection with series resistors
4. Mechanical switch with good tactile feedback
5. PCB footprint compatible with standard 6x6mm switches

Button Configuration:
SW1 (Boot/User1):
3.3V ── R_pullup (10kΩ) ── GPIO19 ── R_series (100Ω) ── [Switch] ── GND

SW2 (User2):
3.3V ── R_pullup (10kΩ) ── GPIO23 ── R_series (100Ω) ── [Switch] ── GND

Special Boot Button:
GPIO0 ── R_pullup (10kΩ) ── 3.3V
GPIO0 ── R_series (100Ω) ── [Boot Switch] ── GND
```

## Design Rules and Constraints

### Electrical Design Rules

#### Power Supply Design
```
1. Power Rail Sequencing:
   - USB-C PD establishes 9V
   - 7.5V buck converter starts with soft-start
   - 3.3V LDO enables after 7.5V reaches 90%
   - ESP32 reset released after 3.3V stable

2. Power Supply Decoupling:
   - Bulk capacitors: 47µF minimum per supply
   - HF decoupling: 0.1µF within 5mm of IC power pins
   - Ground plane: continuous, minimize via count in return paths
   - Power planes: separate analog/digital where possible
```

#### Signal Integrity Guidelines
```
1. High-Speed Signals:
   - ESP32 crystal: guard rings, short traces (<10mm)
   - SPI/I2C: series termination if trace length >25mm
   - USB-C data: differential impedance 90Ω ±10%
   - Clock signals: point-to-point routing, no stubs

2. Analog Sensitive Circuits:
   - ADC references: separate analog ground return
   - Power supply feedback: route away from digital switching
   - Crystal oscillators: isolate from power switching nodes
```

#### EMC and Noise Considerations
```
1. Switching Power Supply Layout:
   - Minimize switching node copper area
   - Ground plane under switching components
   - Input/output filtering with ferrite beads
   - Separate power and signal ground planes

2. RF Considerations:
   - ESP32 antenna: keep clear area per module datasheet
   - U.FL connector: 50Ω controlled impedance trace
   - WiFi antenna: isolate from power switching circuits
```

### Component Placement Guidelines

#### Power Section
```
1. USB-C Connector Placement:
   - Board edge for accessibility
   - Away from antenna keep-out area
   - Mechanical stress considerations

2. Buck Converter Layout:
   - Input capacitor close to VIN pin
   - Switching node (SW pin) minimized copper area
   - Inductor placement adjacent to SW pin
   - Output capacitors close to inductor
   - Feedback divider close to FB pin
```

#### Microcontroller Section
```
1. ESP32 Module Placement:
   - Central location for signal routing
   - Access to external antenna connector
   - Programming header nearby
   - Heat dissipation consideration

2. Crystal Placement:
   - Within 5mm of ESP32 crystal pins
   - Guard ring connected to ground
   - Away from power switching circuits
```

### Net Classes and Constraints

#### Critical Net Classifications
```
Power Rails:
- USB_9V: 1.5mm trace width minimum, 2oz copper
- 7V5: 1.0mm trace width minimum, 2oz copper
- 3V3: 0.5mm trace width minimum, 1oz copper
- GND: Maximum copper pour coverage

High-Speed Digital:
- I2C_SDA, I2C_SCL: 0.2mm trace width, 50Ω impedance
- ESP32_CLK: 0.2mm trace width, point-to-point routing
- SPI signals: 0.15mm trace width, length matching ±1mm

RF/Antenna:
- ANT_RF: 0.35mm trace width, 50Ω controlled impedance
- U.FL connection: coplanar waveguide with ground

Serial Interface:
- RS232_TX, RS232_RX: 0.15mm trace width, 120Ω termination
- UART_TX, UART_RX: 0.15mm trace width, point-to-point
```

## Verification and Validation

### Design Review Checklist

#### Schematic Review
- [ ] All power rails properly decoupled
- [ ] Boot configuration resistors correct values
- [ ] I2C pull-ups sized for bus loading
- [ ] Crystal load capacitors match specification
- [ ] MAX3232 charge pump capacitors correct values
- [ ] GPIO assignments don't conflict with boot requirements
- [ ] All connectors pinout verified against mechanical drawings

#### Component Selection Review
- [ ] All components have acceptable temperature ratings
- [ ] Power dissipation calculations completed for all ICs
- [ ] Passive component tolerances adequate for circuit function
- [ ] Alternative components identified for supply chain risks
- [ ] Component packages suitable for intended assembly method

#### Interface Compatibility
- [ ] ESP32 GPIO assignments compatible with existing firmware
- [ ] Programming interface matches development tools
- [ ] Serial interface compatible with BetaBrite protocol requirements
- [ ] Power supply outputs within LED sign specifications
- [ ] USB-C connector meets mechanical requirements

### Simulation and Analysis Requirements

#### Power Supply Analysis
```
Required Simulations:
1. Buck converter stability analysis (SPICE)
2. Load transient response modeling
3. Power supply rejection ratio (PSRR) analysis
4. Thermal analysis under worst-case loading
5. Efficiency calculations across load range
```

#### Signal Integrity Analysis
```
Required Analysis:
1. I2C signal timing and loading
2. Crystal oscillator startup simulation
3. ESP32 boot timing verification
4. RS232 signal levels and timing
5. EMC pre-compliance modeling
```

## Documentation and Deliverables

### Schematic Documentation Requirements
- Complete schematic with all sheets properly titled and dated
- Component reference designators following IPC standards
- Net names clear and consistent across sheets
- Test points identified and labeled
- Assembly notes and special instructions
- Bill of materials with manufacturer part numbers
- Component placement drawing (top and bottom views)
- Drill drawing and fabrication notes

### Change Control Process
1. All schematic changes require engineering review
2. Version control with clear revision history
3. Design review sign-off before PCB layout
4. ECO (Engineering Change Order) process for post-layout changes
5. Final design freeze before prototype fabrication