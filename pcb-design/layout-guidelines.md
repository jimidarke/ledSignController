# PCB Layout Guidelines

## Overview

This document provides comprehensive PCB layout guidelines for the 2-layer LED Sign Controller board, focusing on power integrity, signal integrity, thermal management, and manufacturing considerations.

## Board Specifications

### Physical Specifications
```
Target Dimensions: 80mm x 60mm (may adjust based on component placement)
Layer Count: 2 layers (Top + Bottom)
Board Thickness: 1.6mm (standard FR4)
Copper Weight: 1oz base, 2oz for power traces
Minimum Track Width: 0.1mm (4 mil)
Minimum Via Size: 0.2mm (8 mil) drill, 0.45mm (18 mil) pad
Minimum Spacing: 0.1mm (4 mil) track-to-track
Surface Finish: HASL (Hot Air Solder Leveling)
Solder Mask: Green, 0.1mm (4 mil) minimum web
Silkscreen: White, 0.15mm (6 mil) minimum line width
```

### Design Rules Setup
```
Net Classes:
- Power_9V: 1.5mm trace width, 0.3mm via drill
- Power_7V5: 1.0mm trace width, 0.3mm via drill
- Power_3V3: 0.5mm trace width, 0.2mm via drill
- Ground: Maximum copper pour, via stitching every 5mm
- High_Speed: 0.2mm trace width, controlled impedance
- RF_50ohm: 0.35mm trace width, coplanar waveguide
- Standard: 0.15mm trace width, 0.2mm via drill

Via Types:
- Power vias: 0.3mm drill, 0.6mm pad
- Signal vias: 0.2mm drill, 0.45mm pad
- Thermal vias: 0.15mm drill, 0.35mm pad (under ICs)
```

## Layer Stack-up and Copper Distribution

### 2-Layer Stack-up
```
Layer 1 (Top):
- Primary component placement
- High-speed signal routing
- Power distribution (wide traces)
- Local ground pours around sensitive circuits

Layer 2 (Bottom):
- Secondary component placement (passives)
- Ground plane (maximum coverage)
- Return current paths
- Heat spreading copper
```

### Copper Pour Strategy
```
Top Layer Pours:
- 3.3V pour in microcontroller area
- 7.5V pour in power section
- Local ground pours around crystals and sensitive analog

Bottom Layer Pours:
- Solid ground plane (>90% coverage target)
- Split only where absolutely necessary
- Thermal relief connections for through-hole components
- Via stitching to top layer ground every 5mm maximum
```

## Component Placement Strategy

### Power Section Layout (Upper Left Quadrant)
```
Placement Priority:
1. J1 (12V Barrel Jack) - Board edge, mechanical access and strain relief
2. F1 (3A Fuse) - Close to barrel jack input, easy access for replacement
3. D1 (MBRS340) - Adjacent to fuse, adequate heat sinking area
4. D2 (TVS), C1, C2 - Input protection and filtering, compact grouping
5. U1 (TPS54331) - Central power area, thermal considerations
6. L1 (Inductor) - Adjacent to U1 switching pin, minimize loop area
7. C6, C7 (Output caps) - Close to inductor output
8. U2 (TPS73633) - Downstream from 7.5V rail

Layout Constraints:
- Keep switching node (U1 SW pin) copper area <10mm²
- Place input capacitors within 5mm of VIN pins
- MBRS340 requires thermal pad/via for 0.8W heat dissipation
- Route feedback traces away from switching nodes
- Simplified layout with 23 fewer components vs USB-C PD version
```

### Microcontroller Section (Center Right)
```
Placement Priority:
1. U3 (ESP32-WROOM-32UE) - Central location, antenna access
2. J2 (U.FL Connector) - Board edge, antenna routing
3. Y1 (32.768kHz Crystal) - Within 5mm of ESP32, away from power
4. C21, C22 (Crystal caps) - Adjacent to crystal, matched lengths
5. J4 (Programming header) - Accessible but not interference
6. C12-C15 (Decoupling) - Within 2mm of ESP32 power pins

Layout Constraints:
- Maintain antenna keep-out area per ESP32 datasheet
- Crystal guard ring connected to ground, >2mm from power traces
- Programming header accessible but not interfering with enclosure
- GPIO routing from ESP32 to minimize crosstalk
```

### Serial Interface Section (Lower Center)
```
Placement Priority:
1. U4 (MAX3232) - Between ESP32 and output connector
2. C16-C20 (Charge pump caps) - Within 3mm of MAX3232, star configuration
3. J5 (Serial connector) - Board edge for external access
4. R13, R14 (Termination) - Close to connector pins

Layout Constraints:
- Keep charge pump capacitors equal length from MAX3232
- Route RS232 signals away from power switching
- Provide ESD protection path to ground at connector
- Minimize crosstalk between TX and RX signals
```

### User Interface Section (Lower Right)
```
Placement Priority:
1. U5 (WS2812B RGB LED) - Visible through enclosure
2. SW1, SW2 (User buttons) - Accessible through enclosure
3. R4, R5, R8 (Pull-ups, current limit) - Adjacent to driven components

Layout Constraints:
- LED positioned for optimal light pipe coupling
- Button placement for ergonomic access
- Signal routing to minimize noise coupling into power supplies
```

## Routing Guidelines

### Power Distribution Network (PDN)

#### High Current Paths (>1A)
```
12V Barrel Jack Input Protection:
- Route: J1 → F1 → D1 → D2 → C1,C2 → Buck converter
- Trace width: 1.2mm minimum for 2A capability
- Keep protection components close together
- MBRS340 diode requires thermal via array

7.5V Distribution:
- Route: U1 → L1 → C6,C7 → J3 (LED Sign)
- Trace width: 1.5mm for 3A at 2oz copper
- Polygon pour with thermal relief at connectors
- Multiple vias (0.3mm drill) for layer transitions

3.3V Distribution:
- Route: U2 → C10,C11 → ESP32, MAX3232, LED
- Trace width: 0.8mm for 1A capability
- Star configuration from output capacitors
- Separate digital and analog 3.3V where beneficial
```

#### Current Carrying Capacity
```
Trace Width Calculations (2oz copper, 10°C rise):
- 3A: 1.5mm trace width minimum
- 2A: 1.0mm trace width minimum
- 1A: 0.5mm trace width minimum
- 500mA: 0.3mm trace width minimum
- 250mA: 0.15mm trace width minimum

Via Current Calculations:
- 0.3mm drill via: 1.5A continuous
- 0.2mm drill via: 0.8A continuous
- Use multiple vias in parallel for higher current
- Thermal vias: 0.15mm drill, primarily for heat transfer
```

### Ground System Design

#### Ground Plane Implementation
```
Bottom Layer Ground Plane:
- Maximum copper coverage (>90% target)
- Continuous plane, avoid splits where possible
- Thermal relief for through-hole components
- Direct connection for SMD component thermal pads

Top Layer Ground Distribution:
- Local ground pours around sensitive circuits
- Via stitching to bottom plane every 5mm
- Star connection points for analog references
- Isolated ground islands for crystal circuits
```

#### Ground Current Return Paths
```
High-Speed Signals:
- Route over continuous ground plane
- Minimize return path loops
- Via stitching where ground plane is interrupted
- Keep signal and return path impedance controlled

Power Supply Returns:
- Low impedance connection to main ground plane
- Multiple parallel paths for high current returns
- Avoid forcing return current through narrow necks
- Kelvin connections for current sense applications
```

### Signal Integrity Routing

#### High-Speed Digital Signals
```
I2C Bus (SDA, SCL):
- Route length: <50mm to avoid transmission line effects
- Trace width: 0.2mm for 100Ω differential impedance
- Parallel routing with 0.3mm spacing
- Terminate at farthest device, pull-up resistors
- Via placement: minimize stubs, use buried vias if available

SPI Signals (if used):
- Length matching: ±1mm between clock and data
- Trace width: 0.15mm for 50Ω single-ended
- Route over solid ground plane
- Clock signal isolation from other signals
- Series termination at source if ringing observed
```

#### Crystal Oscillator Layout
```
32.768kHz Crystal Circuit:
- Trace length: <5mm from ESP32 to crystal
- Trace width: 0.15mm minimum, wider acceptable
- Ground guard ring: 0.5mm wide, connected via to ground plane
- Component placement: Crystal and load caps form equilateral triangle
- Isolation: >2mm from power switching circuits, >5mm from inductors

High-Speed Crystal (if present):
- Similar guidelines but more critical
- Differential routing if crystal has differential output
- Avoid crossover with other high-speed signals
```

### RF and Antenna Considerations

#### WiFi Antenna Routing
```
ESP32 to U.FL Connector:
- Impedance: 50Ω coplanar waveguide
- Trace width: 0.35mm with 0.2mm ground clearance
- Ground reference: Continuous ground plane on bottom layer
- Via stitching: Ground vias every 2mm along trace edges
- Length: Minimize, typically <20mm acceptable

Keep-Out Areas:
- Antenna connector: 5mm radius clear of copper (except ground)
- ESP32 module: Follow manufacturer's recommended keep-out
- No high-current switching circuits within 10mm of RF path
```

#### EMC Considerations
```
Noise Source Isolation:
- Buck converter switching node: minimize copper area, shield with ground
- Crystal circuits: guard rings, isolated ground connections
- High-speed digital: route over solid ground, minimize loop area
- Power supply traces: parallel return paths, avoid crossing sensitive signals

Filtering and Suppression:
- Ferrite beads on power supply inputs/outputs
- Common-mode chokes on USB data lines
- TVS diodes for ESD protection at connectors
- Ground plane stitching for high-frequency return paths
```

## Thermal Management

### Heat Source Identification
```
Primary Heat Sources:
1. U2 (TPS54331): 1.8W maximum dissipation
2. U3 (TPS73633): 1.4W maximum dissipation
3. U4 (ESP32): 0.5W typical, 1.0W peak during TX
4. L1 (Inductor): 0.3W copper losses at full load

Secondary Heat Sources:
5. U1 (STUSB4500): 0.2W typical
6. U5 (MAX3232): 0.1W typical
7. Current-carrying traces: I²R losses
```

### Thermal Design Strategy
```
Copper Pour Heat Spreading:
- Maximum copper coverage on both layers
- Thermal vias under high-power components
- Via array: 0.15mm drill, 0.7mm pitch grid pattern
- Thermal interface to enclosure mounting points

Component Thermal Management:
- Power ICs: Thermal pad connection to ground plane
- ESP32 module: Thermal pad via connection
- Buck converter: Keep switching components together for shared heat sink
- Power resistors: Route to thermal vias if significant power
```

#### Thermal Via Calculations
```
Thermal Resistance Calculations:
- Single 0.15mm via: ~200°C/W thermal resistance
- Via array 5x5 (25 vias): ~8°C/W thermal resistance
- Copper plane area: ~0.1°C/W per cm² at 1oz thickness
- Component to ambient: Sum of all thermal resistances

Via Array Design:
- TPS54331: 3x3 array under thermal pad
- TPS73633: 2x3 array under thermal pad
- ESP32: 4x4 array under module thermal pad
- Via spacing: 0.7mm pitch maximum for effective heat transfer
```

### Temperature Management
```
Operating Temperature Targets:
- Ambient: -10°C to +60°C specification
- Component junction: <100°C maximum for all ICs
- Power converter efficiency: Maintain >85% at high temperature
- Crystal stability: ±20ppm over temperature range

Thermal Monitoring:
- ESP32 internal temperature sensor for system monitoring
- Optional external temperature sensor (DS18B20) for enclosure monitoring
- Thermal shutdown protection in power management ICs
- Software thermal management (frequency scaling, load reduction)
```

## Manufacturing and Assembly Considerations

### Design for Manufacturing (DFM)

#### PCB Fabrication Constraints
```
Minimum Feature Sizes:
- Track width: 0.1mm (4 mil) - standard capability
- Track spacing: 0.1mm (4 mil) - standard capability
- Via size: 0.2mm (8 mil) drill minimum
- Solder mask expansion: 0.05mm (2 mil)
- Silkscreen minimum: 0.15mm (6 mil) line width

Layer Registration:
- Via-in-pad: Avoid for cost reasons, use thermal relief
- Drill-to-copper: 0.15mm (6 mil) minimum annular ring
- Layer-to-layer alignment: ±0.1mm typical
```

#### Component Placement for Assembly
```
Component Orientation:
- Consistent orientation for similar components
- Polarized components: Clear polarity marking
- Reference designators: Visible after component placement
- Test points: Accessible with standard probe pitches

Assembly Considerations:
- No components under tall components (accessibility)
- Minimum 0.5mm spacing between components
- Fiducial markers: 3 points minimum, 1mm diameter
- Tool access: Programming headers, test points accessible
- Rework access: Space around high-failure-rate components
```

### Design for Test (DFT)

#### Test Point Strategy
```
Essential Test Points:
- TP1: 9V input (post-fuse, pre-regulation)
- TP2: 7.5V output (buck converter validation)
- TP3: 3.3V output (LDO validation)
- TP4: Ground reference (multiple locations)
- TP5: ESP32 reset signal (programming/debug)
- TP6: I2C SDA (bus monitoring)
- TP7: I2C SCL (bus monitoring)
- TP8: Serial TX (protocol monitoring)
- TP9: Serial RX (protocol monitoring)

Test Point Specifications:
- Diameter: 1mm minimum for standard test probes
- Spacing: 2.5mm minimum center-to-center
- Location: Away from components, accessible from one side
- Marking: Clear silkscreen identification
```

#### In-Circuit Test (ICT) Considerations
```
ICT Access Points:
- All power rails accessible
- Critical control signals accessible
- Component isolation points for fault diagnosis
- Ground reference points distributed across board

Boundary Scan (if implemented):
- JTAG interface on ESP32 (if accessible)
- Boundary scan chain for manufacturing test
- IEEE 1149.1 compliance for automated test
```

### Design for Repair and Maintenance

#### Component Accessibility
```
High-Failure Components:
- Electrolytic capacitors: Accessible for replacement
- Connectors: Mechanically robust, standard parts
- Power management ICs: Reworkable packages preferred
- ESP32 module: Socket option for development, direct mount for production

Rework Considerations:
- Component spacing for hot air rework
- Ground plane thermal relief for hand soldering
- Alternative component footprints for supply chain flexibility
```

## Quality and Validation

### Design Rule Check (DRC) Setup
```
Electrical Rules:
- Net connectivity verification
- Power and ground network analysis
- Component pin assignment verification
- Unconnected pin reporting (intentional vs. error)

Physical Rules:
- Minimum trace width and spacing compliance
- Via size and drill constraints
- Component courtyard violations
- Silk screen and solder mask conflicts
- Manufacturing rule compliance (DFM check)
```

### Pre-Production Validation
```
Simulation Verification:
- Power integrity analysis (DC drop, AC impedance)
- Signal integrity simulation (high-speed nets)
- Thermal simulation (steady-state and transient)
- EMC pre-compliance (radiated and conducted emissions)

Physical Validation:
- 3D mechanical fit check with enclosure
- Component placement and orientation review
- Assembly process verification (pick-and-place compatibility)
- Test and programming fixture design validation
```

### Documentation Requirements

#### Manufacturing Documentation
```
Fabrication Files:
- Gerber files (RS-274X format)
- Excellon drill files
- Pick and place files (XY coordinates)
- Bill of materials (BOM) with manufacturer part numbers
- Assembly drawings (top and bottom views)
- Fabrication notes and special requirements

Quality Documents:
- Test procedures and acceptance criteria
- Inspection checklists for incoming inspection
- Functional test specifications
- Burn-in and reliability test procedures
```

#### Design Documentation
```
Design Review Package:
- Complete schematic with all sheets
- PCB layout plots (all layers)
- Cross-reference between schematic and layout
- Design review checklist and sign-offs
- Component stress analysis and derating
- Thermal analysis and cooling requirements
- EMC compliance strategy and test plan
```

This comprehensive layout guideline ensures the PCB design will meet performance, manufacturability, and reliability requirements while maintaining cost-effectiveness for the LED Sign Controller application.