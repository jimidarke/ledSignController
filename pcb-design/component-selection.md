# Component Selection and Bill of Materials

## Component Selection Criteria

### Primary Considerations
- **Availability**: Prefer components with multiple suppliers and long-term availability
- **Cost**: Target <$50 total BOM cost for moderate quantities (100-1000 pieces)
- **Assembly**: Favor packages suitable for both hand assembly and automated PCBA
- **Performance**: Meet or exceed specifications with appropriate derating
- **Supply Chain**: Multiple sources to avoid single-supplier dependencies

### Package Preferences
- **Passives**: 0805 size (good balance of cost, availability, and hand assembly)
- **ICs**: SOIC, SOT-223, QFN packages (avoid BGA for cost/complexity)
- **Connectors**: Through-hole for mechanical strength, SMD for space-critical

## Complete Bill of Materials

### Power Management

| Reference | Description | Part Number | Package | Quantity | Unit Cost | Extended |
|-----------|-------------|-------------|---------|----------|-----------|----------|
| **U1** | 7.5V Buck Controller | TPS54331DR | SOIC-8 | 1 | $2.85 | $2.85 |
| **U2** | 3.3V LDO Regulator | TPS73633DRBT | SOT-223 | 1 | $1.45 | $1.45 |
| **L1** | 22µH Power Inductor | XAL1010-223MEC | 10x10mm | 1 | $2.10 | $2.10 |
| **F1** | 3A Input Fuse | 0456003.MR | 1206 | 1 | $0.32 | $0.32 |
| **F2** | 3.5A Output Fuse | MF-MSMF350/24-2 | 1812 | 1 | $0.42 | $0.42 |

### Protection and Filtering

| Reference | Description | Part Number | Package | Quantity | Unit Cost | Extended |
|-----------|-------------|-------------|---------|----------|-----------|----------|
| **D1** | Reverse Polarity Diode | MBRS340T3G | DO-201AD | 1 | $0.45 | $0.45 |
| **D2** | TVS Diode 15V | SMBJ15A | SMB | 1 | $0.22 | $0.22 |
| **FB1** | Ferrite Bead | BLM18PG221SN1D | 0603 | 1 | $0.05 | $0.05 |

### Microcontroller System

| Reference | Description | Part Number | Package | Quantity | Unit Cost | Extended |
|-----------|-------------|-------------|---------|----------|-----------|----------|
| **J6** | ESP32 Dev Board Socket | Female Pin Headers | 2x15 Pin | 2 | $0.45 | $0.90 |
| **U3** | RS232 Transceiver | MAX3232ECPE+ | DIP-16 | 1 | $2.10 | $2.10 |

*ESP32-DevKitC board purchased separately - not included in PCB BOM*

### Connectors

| Reference | Description | Part Number | Package | Quantity | Unit Cost | Extended |
|-----------|-------------|-------------|---------|----------|-----------|----------|
| **J1** | 12V Barrel Jack | PJ-002AH | Through Hole | 1 | $0.25 | $0.25 |
| **J3** | LED Sign Connector | 1935161 | Terminal Block | 1 | $1.20 | $1.20 |
| **J5** | Serial Output | 1935174 | Terminal Block | 1 | $1.85 | $1.85 |

*User interface components (buttons, LEDs) provided by ESP32-DevKitC board*

### Passive Components - Capacitors

| Reference | Description | Value | Voltage | Package | Quantity | Unit Cost | Extended |
|-----------|-------------|-------|---------|---------|----------|-----------|----------|
| **C1** | Input Bulk Cap | 100µF | 25V | Electrolytic | 1 | $0.35 | $0.35 |
| **C2,C3** | Input Decoupling | 0.1µF | 50V | 0805 | 2 | $0.03 | $0.06 |
| **C4** | Input Filter | 0.1µF | 50V | 0805 | 1 | $0.03 | $0.03 |
| **C6** | Buck Output | 220µF | 16V | Electrolytic | 1 | $0.65 | $0.65 |
| **C7** | Buck Output | 22µF | 16V | 1210 | 1 | $0.25 | $0.25 |
| **C8** | Buck Bootstrap | 0.1µF | 50V | 0805 | 1 | $0.03 | $0.03 |
| **C9,C10** | MAX3232 Power | 0.1µF | 10V | 0805 | 2 | $0.03 | $0.06 |
| **C16-C20** | MAX3232 Charge Pump | 0.1µF | 25V | 0805 | 5 | $0.03 | $0.15 |

### Passive Components - Resistors

| Reference | Description | Value | Power | Package | Quantity | Unit Cost | Extended |
|-----------|-------------|-------|-------|---------|----------|-----------|----------|
| **R1** | Buck Feedback Upper | 56kΩ | 1/8W | 0805 | 1 | $0.02 | $0.02 |
| **R2** | Buck Feedback Lower | 12kΩ | 1/8W | 0805 | 1 | $0.02 | $0.02 |
| **R3** | Buck Soft Start | 100kΩ | 1/8W | 0805 | 1 | $0.02 | $0.02 |
| **R9-R12** | I2C Pull-ups | 4.7kΩ | 1/8W | 0805 | 4 | $0.02 | $0.08 |
| **R13,R14** | Serial Termination | 120Ω | 1/8W | 0805 | 2 | $0.02 | $0.04 |

## Component Specifications Detail

### Critical Components Analysis

#### ESP32-WROOM-32UE Module
```
Key Features:
+ External antenna support via U.FL connector
+ 4MB flash, 520KB RAM
+ 802.11 b/g/n WiFi + Bluetooth 4.2
+ -40°C to +85°C operating range
+ CE, FCC, IC certified
+ Pin-compatible with existing firmware

Considerations:
- Requires external antenna and proper RF layout
- Built-in PCB antenna disabled in U variant
- Pre-certified module simplifies compliance
```

#### STUSB4500 PD Controller
```
Programming Configuration (via I2C):
- PDO1: 5V/3A (15W fallback)
- PDO2: 9V/3A (27W primary)
- PDO3: Disabled
- VBUS discharge: Enabled
- GPIO configuration: Power good indication

Pin Configuration:
- I2C address: 0x28 (7-bit)
- RESET pin: Connected to ESP32 GPIO for control
- ATTACH pin: Power good signal to ESP32
- VBUS_SENSE: Voltage monitoring
```

#### TPS54331 Buck Controller
```
Design Calculations:
Vout = 0.8V × (1 + R1/R2)
For 7.5V: R1 = 56kΩ, R2 = 12kΩ
Inductor: L = (Vin-Vout) × Vout / (ΔIL × f × Vin)
For 22µH: ΔIL ≈ 0.8A (30% ripple)
Cout: ESR < 50mΩ for stability
```

### Alternative Component Options

#### Cost-Reduced Alternatives

| Primary Component | Alternative | Cost Saving | Trade-offs |
|------------------|-------------|-------------|------------|
| STUSB4500 | FUSB302 + resistors | 40% | Fixed 9V only, no I2C control |
| TPS54331 | LM2596S-ADJ | 25% | Lower efficiency, larger package |
| ESP32-WROOM-32UE | ESP32-WROOM-32E | 15% | No external antenna support |
| WS2812B | Standard RGB + drivers | 20% | More complex control circuit |

#### Performance Enhanced Alternatives

| Primary Component | Enhancement | Cost Increase | Benefits |
|------------------|-------------|---------------|----------|
| TPS54331 | TPS543620 | 30% | Higher efficiency, smaller |
| TPS73633 | TPS62160 | 20% | Switching regulator efficiency |
| MAX3232 | MAX3232E | 10% | Enhanced ESD protection |

## Supply Chain and Sourcing

### Primary Suppliers

#### Tier 1 (Preferred)
- **Digi-Key**: Excellent availability, fast shipping, no minimum orders
- **Mouser**: Comparable to Digi-Key, good for design verification
- **Arrow**: Good for production quantities, competitive pricing

#### Tier 2 (Alternative)
- **LCSC**: Low-cost Asian supplier, good for JLCPCB assembly
- **Newark/Farnell**: European/global alternative
- **TME**: European specialist with good component selection

#### Direct from Manufacturer
- **Espressif**: ESP32 modules (for production quantities >1000)
- **STMicroelectronics**: STUSB4500 (direct ordering available)

### Lead Time Analysis

| Component Category | Lead Time (Typical) | Lead Time (Extended) | Stock Risk |
|-------------------|---------------------|---------------------|------------|
| **Passive Components** | 1-2 weeks | 8-16 weeks | Low |
| **Standard ICs** | 2-4 weeks | 16-26 weeks | Medium |
| **ESP32 Modules** | 4-8 weeks | 26-52 weeks | High |
| **USB-C Connectors** | 2-4 weeks | 12-20 weeks | Medium |
| **Power ICs** | 4-12 weeks | 20-40 weeks | High |

### Risk Mitigation Strategies
1. **Multi-sourcing**: Identify 2+ suppliers for critical components
2. **Alternate parts**: Maintain pin-compatible alternatives for high-risk parts
3. **Inventory buffer**: Stock long-lead-time components for production
4. **Design flexibility**: Use standard values and packages where possible

## Cost Analysis by Category

### BOM Cost Breakdown (100 pieces)

| Category | Components | Cost | Percentage |
|----------|------------|------|------------|
| **Dev Board Interface** | Female Pin Headers | $0.90 | 5.1% |
| **Power Management** | Buck, Magnetics | $5.42 | 30.9% |
| **Protection** | Fuses, Diodes, TVS Protection | $1.04 | 5.9% |
| **Connectors** | Barrel Jack, Terminal Blocks | $3.30 | 18.8% |
| **Interface** | MAX3232, RS232 Caps | $2.25 | 12.8% |
| **Passives** | Resistors, Capacitors | $1.35 | 7.7% |
| **Miscellaneous** | PCB, Assembly | $3.30 | 18.8% |
| **PCB Subtotal** | | **$17.56** | **100%** |
| **ESP32-DevKitC Board** | Purchased separately | **$7.00** | |
| **Complete System** | | **$24.56** | |

**Major Savings Achieved:**
- **vs USB-C PD approach**: $7.09 reduction (22% savings)
- **vs integrated ESP32 approach**: $8.87 reduction (27% savings)
- **Component count reduction**: ~35 fewer components vs integrated design

### Cost Optimization Roadmap

#### Phase 1: Prototype (10-50 units)
- Use preferred components for functionality validation
- Accept higher costs for proven performance
- Hand assembly to minimize setup costs

#### Phase 2: Small Production (100-500 units)
- Negotiate volume pricing with suppliers
- Consider basic cost reductions where validated
- Implement automated assembly for consistent quality

#### Phase 3: Volume Production (1000+ units)
- Implement all cost optimizations
- Direct manufacturer relationships
- Custom packaging and logistics optimization

## Component Lifecycle and Obsolescence

### High-Risk Components (Monitor for EOL)
- **ESP32-WROOM-32UE**: Monitor Espressif roadmap
- **MAX3232**: Mature product, stable but watch for package changes
- **TPS54331**: TI has newer alternatives, plan migration path

### Future-Proof Selections
- **USB-C Connector**: Industry standard, long-term availability
- **Standard Passives**: 0805 resistors/capacitors have decades of availability
- **Protection Devices**: Basic fuses and TVS diodes are commodity items

### Obsolescence Mitigation
1. **Design Reviews**: Annual component availability assessment
2. **Alternative Validation**: Test backup components before needed
3. **Lifecycle Monitoring**: Subscribe to manufacturer PCN notifications
4. **Inventory Management**: Last-time-buy planning for EOL components