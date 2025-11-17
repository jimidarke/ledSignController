# Power System Design

## Overview

The power system design centers around a 12V DC barrel jack input providing up to 2A (24W), with comprehensive input protection and a single 7.5V power rail feeding both the LED sign and ESP32 dev board. The dev board's onboard regulator provides 3.3V for peripherals.

## Power Budget Analysis

### Detailed Load Analysis

| Component | Operating Voltage | Typical Current | Peak Current | Power (Typical) | Power (Peak) |
|-----------|-------------------|-----------------|--------------|-----------------|--------------|
| **BetaBrite LED Sign** | 7.5V | 2.2A | 2.75A | 16.5W | 20.6W |
| **ESP32 Dev Board** | 7.5V | 200mA | 300mA | 1.5W | 2.25W |
| **MAX3232 + Peripherals** | 3.3V | 150mA | 200mA | 0.5W | 0.66W |
| **Buck Converter Losses** | - | - | - | 1.4W | 1.8W |
| **Total System** | | **1.9A @ 12V** | **2.0A @ 12V** | **19.9W** | **25.3W** |

*ESP32 Dev Board includes onboard regulator losses and 3.3V output for peripherals*

### Power Rail Requirements

#### 7.5V Rail (LED Sign + ESP32 Dev Board)
- **Load**: 3.05A maximum (2.75A LED sign + 0.3A dev board), 2.4A typical
- **Regulation**: ±2% (7.35V to 7.65V)
- **Ripple**: <50mV peak-to-peak
- **Transient Response**: <200mV deviation for 50% load step
- **Protection**: Overcurrent at 3.5A, overvoltage at 8.5V

#### 3.3V Rail (Provided by ESP32 Dev Board)
- **Source**: ESP32-DevKitC onboard AMS1117-3.3V regulator
- **Available Current**: 800mA maximum from dev board
- **Load**: MAX3232 + peripherals = 200mA maximum
- **Margin**: 600mA available for expansion
- **Advantage**: No additional 3.3V regulator required on main PCB

## 12V DC Barrel Jack Input Design

### Input Specifications
- **Nominal Voltage**: 12V DC
- **Input Range**: 10.8V to 13.2V (±10% regulation tolerance)
- **Maximum Current**: 2A continuous
- **Power Rating**: 24W maximum
- **Connector**: Standard 2.1mm ID × 5.5mm OD barrel jack, center positive
- **Wall Adapter**: Standard 12V 2A switching power supply

### Input Protection Circuit Design

#### Complete Protection Strategy
```
Barrel Jack → F1 (Fuse) → D1 (Reverse Polarity) → D2 (TVS Surge) → C1,C2 (Filter) → Buck Converters
```

**Component Selection:**
- **F1**: Littelfuse 0456003.MR (3A, fast-blow, 1206 package) - overcurrent protection
- **D1**: MBRS340 Schottky diode (3A, 40V) - reverse polarity protection
- **D2**: SMBJ15A TVS diode (15V breakdown) - surge/ESD protection
- **C1**: 100µF, 25V electrolytic (bulk input capacitance)
- **C2**: 0.1µF, 50V ceramic (high frequency decoupling)

#### Protection Features
```
Overcurrent Protection:
- 3A fuse provides primary overcurrent protection
- Opens at 150% rated current within 30 seconds
- Protects against short circuits and overload conditions

Reverse Polarity Protection:
- Schottky diode D1 blocks reverse current flow
- Low forward voltage drop (0.4V max) minimizes power loss
- 3A continuous rating matches system requirements

Surge Protection:
- TVS diode D2 clamps voltage spikes above 15V
- Protects against static discharge and power supply transients
- Fast response time (<1ns) for effective protection

EMI Filtering:
- Input capacitors reduce conducted EMI
- Ferrite bead option for additional common-mode filtering
- Differential mode filtering with ceramic capacitors
```

#### Simplified Input Circuit
```
12V Barrel Jack (J1):
Pin 1 (Center): +12V
Pin 2 (Shell): Ground

Protection Path:
+12V → F1 (3A Fuse) → D1 (MBRS340 Schottky) → D2 (SMBJ15A TVS) → C1 (100µF) || C2 (0.1µF) → 12V_Protected

Power Loss Analysis:
- Fuse: Negligible resistance (<50mΩ)
- Schottky diode: 0.4V × 2A = 0.8W heat dissipation
- TVS diode: Negligible in normal operation
- Total protection overhead: ~3% power loss
```

## Buck Converter Design

### 7.5V Buck Converter (Primary Rail)

#### IC Selection: TPS54331
```
Key Specifications:
- Input Range: 3.5V to 28V (12V input well within range)
- Output Current: 3A continuous
- Switching Frequency: 570kHz (adjustable)
- Efficiency: >93% at 2A load with 12V input (improved from 9V)
- Package: SOIC-8 (easy assembly)
- Integrated Features: Soft-start, thermal shutdown
```

#### Feedback Network
```
Vout = Vref × (1 + R1/R2)
For 7.5V output: R1 = 56kΩ, R2 = 12kΩ
Tolerance: 1% resistors for ±2% regulation
```

#### Inductor Selection
```
L1: 22µH, 4A rated
Part: Coilcraft XAL1010-223MEC
- DCR: 42mΩ (low power loss)
- Saturation Current: 4.2A
- Package: 10x10mm shielded
```

#### Output Capacitors
```
Primary: 220µF, 16V electrolytic (bulk storage)
Secondary: 22µF, 16V ceramic (fast transient response)
ESR: <50mΩ combined for stability
```

### ESP32 Dev Board Power Interface

#### Dev Board Power Input (VIN)
```
ESP32-DevKitC Power Specifications:
- VIN Range: 6V to 12V (7.5V input is optimal)
- Onboard Regulator: AMS1117-3.3V (800mA capability)
- Input Current: 300mA maximum at 7.5V input
- Efficiency: ~75% (AMS1117 is linear regulator)
- Dropout: ~1.2V (minimum 4.5V required for 3.3V output)

Power Distribution from Dev Board:
- 3.3V Output: Powers MAX3232 and other peripherals
- Available Current: 800mA total, 200mA used, 600mA margin
- Connection: Direct connection to PCB 3.3V rail via socket pins
```

#### Simplified Power Tree
```
12V Input → Protection → 7.5V Buck → [LED Sign 7.5V]
                                   → [ESP32 Dev Board VIN]
                                       ↓
                                   [Dev Board 3.3V] → MAX3232 + Peripherals

Advantages of This Approach:
- Eliminates need for separate 3.3V regulator on PCB
- Uses proven, certified dev board power supply
- Reduces component count by ~8 components
- Simplified PCB layout and reduced cost
```

## Power Sequencing and Control

### Startup Sequence
1. **12V Connection**: Barrel jack inserted, protection circuit validates (~1ms)
2. **Protection Active**: Fuse, reverse polarity, and surge protection enabled
3. **7.5V Rail**: Buck converter soft-start begins (~2ms)
4. **Dev Board Power**: ESP32 dev board VIN powered, onboard regulator starts (~1ms)
5. **3.3V Available**: Dev board provides 3.3V to peripherals (~2ms)
6. **ESP32 Boot**: Power-on reset released, system ready (~100ms)
7. **System Ready**: All rails stable and regulated (~106ms total)

### Power-Good Signals
```
7.5V Rail: PG1 - indicates output within ±5% (from TPS54331)
Dev Board Status: Power LED and status indicators on ESP32-DevKitC
System Ready: Monitored via software through dev board status
Simplified Monitoring: Fewer power rails to monitor vs discrete design
```

### Shutdown Sequence
```
Controlled Shutdown (via ESP32):
1. ESP32 signals shutdown intent
2. Sign controller saves state
3. MQTT disconnect gracefully
4. Buck converters disable in reverse order

Emergency Shutdown:
1. Input fuse protection
2. Thermal shutdown (>85°C junction)
3. Overcurrent protection per rail
```

## Efficiency Analysis

### Overall System Efficiency

| Load Condition | 7.5V Rail (Total) | Dev Board 3.3V Out | Input Power (12V) | Efficiency |
|----------------|--------------------|---------------------|-------------------|------------|
| **Idle** | 0.6A | 0.1A | 5.2W | 89% |
| **Typical** | 2.4A | 0.15A | 19.1W | 91% |
| **Peak** | 3.05A | 0.2A | 24.1W | 90% |

*Efficiency includes both buck converter and dev board regulator losses*

### Heat Dissipation Estimates

| Component | Power Loss (Typical) | Power Loss (Peak) | Thermal Resistance |
|-----------|---------------------|-------------------|-------------------|
| **TPS54331** | 1.1W | 1.6W | 45°C/W (with thermal pad) |
| **MBRS340** | 0.6W | 0.8W | 65°C/W (DO-201AD) |
| **ESP32 Dev Board** | 0.6W | 0.8W | 25°C/W (distributed across board) |
| **Total** | 2.3W | 3.2W | Requires thermal management |

*Note: Dev board regulator heat is distributed across the dev board PCB*

## Circuit Protection Features

### Overcurrent Protection
- **Input**: 3A fuse provides primary overcurrent protection
- **Reverse Polarity**: MBRS340 Schottky diode blocks reverse current
- **7.5V Rail**: TPS54331 internal current limit (4.2A)
- **3.3V Rail**: ESP32 dev board AMS1117 internal current limit (800mA)
- **Output**: 3.5A resettable fuse on LED sign connection

### Overvoltage Protection
- **Input**: SMBJ15A TVS diode clamps transients above 15V
- **7.5V Rail**: Internal OVP shuts down at 8.5V
- **3.3V Rail**: Input from 7.5V rail limits maximum voltage
- **Reverse Polarity**: Schottky diode prevents damage from reversed connections

### Thermal Protection
- **Buck Controllers**: Internal thermal shutdown at 150°C
- **System**: Temperature monitoring via ESP32 ADC (optional)

### ESD Protection
- **USB-C Data Lines**: Integrated in PD controller
- **Exposed Connectors**: TVS diode arrays
- **GPIO Lines**: Series resistors + internal ESP32 protection

## Power Supply Testing and Validation

### Performance Tests
1. **Load Regulation**: 0% to 100% load sweep
2. **Line Regulation**: 8V to 10V input variation
3. **Transient Response**: 10% to 90% load steps
4. **Ripple Measurement**: 20MHz bandwidth scope
5. **Efficiency Mapping**: Multiple load points
6. **Thermal Performance**: Steady-state temperature rise

### Protection Tests
1. **Overcurrent**: Verify trip points and recovery
2. **Overvoltage**: Input surge testing
3. **Thermal**: Temperature chamber validation
4. **ESD**: IEC 61000-4-2 compliance
5. **EMC**: Conducted and radiated emissions

### Reliability Testing
1. **Burn-in**: 168 hours at maximum load
2. **Thermal Cycling**: -10°C to +60°C, 100 cycles
3. **Vibration**: Per IEC 60068-2-6
4. **MTBF Analysis**: Component stress analysis

## Design Validation Checklist

- [ ] Power budget verified with actual component specifications
- [ ] Thermal analysis completed with worst-case ambient
- [ ] All protection circuits validated through simulation
- [ ] Component derating applied (voltage, current, temperature)
- [ ] Backup component selections identified for supply chain
- [ ] EMC pre-compliance analysis completed
- [ ] Safety analysis per relevant standards (IEC 60950-1)
- [ ] Design review completed by independent engineer

## Cost Analysis

### Component Costs (Estimated, 100 piece quantities)

| Component Category | Cost Range | Notes |
|-------------------|------------|--------|
| **PD Controller** | $3-5 | STUSB4500 or equivalent |
| **Buck Controllers** | $4-6 | TPS54331 + TPS73633 |
| **Inductors/Caps** | $3-5 | Quality components for reliability |
| **Protection** | $2-3 | Fuses, TVS diodes, misc |
| **Total Power System** | **$12-19** | ~30-40% of total PCB cost |

### Cost Optimization Opportunities
- Use integrated PD modules vs discrete controller
- Standard inductor values vs optimized custom values
- Aluminum vs tantalum capacitors where acceptable
- Generic protection devices vs premium brands