# ESPHome BetaBrite LED Sign Component

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![ESPHome](https://img.shields.io/badge/ESPHome-2024.1+-blue.svg)](https://esphome.io/)
[![ESP32](https://img.shields.io/badge/ESP-32-green.svg)](https://www.espressif.com/en/products/socs/esp32)

An ESPHome external component for controlling BetaBrite and Alpha Protocol LED signs via ESP32.

## Features

- **Full Protocol Support**: Colors, modes, effects, charsets, positioning, speed control
- **Home Assistant Integration**: Native API with entities for color, mode, effect selection
- **Offline Message Cycling**: Display static messages when WiFi/network is unavailable
- **Priority Messages**: Non-blocking alert system with warning animation
- **Clock Display**: Automatic time display with configurable intervals
- **YAML Configuration**: Compile-time configuration with secrets support
- **ESPHome OTA**: Built-in over-the-air updates via ESPHome dashboard

## Hardware Requirements

- ESP32 development board (ESP32-WROOM-32 or similar)
- BetaBrite LED sign with RS232 interface
- RS232-to-TTL level shifter (MAX3232 or similar)

### Wiring

| ESP32 | RS232 Adapter | BetaBrite |
|-------|---------------|-----------|
| GPIO17 (TX) | TX | RX |
| GPIO16 (RX) | RX | TX |
| 3.3V | VCC | - |
| GND | GND | GND |

> **Warning**: BetaBrite signs use RS232 voltage levels (±12V). You MUST use a level shifter to avoid damaging your ESP32.

## Installation

### Option 1: GitHub Source (Recommended)

Add this to your ESPHome configuration:

```yaml
external_components:
  - source:
      type: git
      url: https://github.com/jimidarke/ledSignController
      ref: main
      path: esphome-betabrite/components
    components: [betabrite]
    refresh: 1d
```

### Option 2: Local Source

Clone or copy the `components/betabrite` folder to your ESPHome config directory:

```
config/
├── components/
│   └── betabrite/
│       ├── __init__.py
│       ├── automation.h
│       ├── bbdefs.h
│       ├── betabrite.cpp
│       └── betabrite.h
├── led_sign.yaml
└── secrets.yaml
```

Then reference as local:

```yaml
external_components:
  - source:
      type: local
      path: components
    components: [betabrite]
```

## Quick Start - Complete Working Example

Copy this entire config to get a working LED sign with Home Assistant text input:

```yaml
substitutions:
  name: "led-sign"
  friendly_name: "LED Sign"

esphome:
  name: ${name}
  friendly_name: ${friendly_name}

esp32:
  board: esp32dev

logger:
api:
  encryption:
    key: !secret api_encryption_key
ota:
  - platform: esphome
    password: !secret ota_password
wifi:
  ssid: !secret wifi_ssid
  password: !secret wifi_password
  ap:
    ssid: "${name}-fallback"
captive_portal:

# === BetaBrite Component ===
external_components:
  - source:
      type: git
      url: https://github.com/jimidarke/ledSignController
      ref: main
      path: esphome-betabrite/components
    components: [betabrite]

uart:
  tx_pin: GPIO17
  rx_pin: GPIO16
  baud_rate: 9600
  data_bits: 7
  parity: EVEN
  stop_bits: 1

betabrite:
  id: led_sign

# === Home Assistant Controls ===
# Text input - type a message in HA, it displays on the sign!
text:
  - platform: template
    name: "Message"
    icon: "mdi:message-text"
    optimistic: true
    max_length: 125
    on_value:
      - if:
          condition:
            lambda: 'return x.length() > 0;'
          then:
            - betabrite.display:
                id: led_sign
                text: !lambda 'return x;'
                color: !lambda 'return id(color_select).state;'

# Color picker dropdown
select:
  - platform: template
    name: "Color"
    id: color_select
    icon: "mdi:palette"
    optimistic: true
    options: ["green", "red", "amber", "orange", "yellow"]
    initial_option: "green"

# Clear button
button:
  - platform: template
    name: "Clear"
    icon: "mdi:eraser"
    on_press:
      - betabrite.clear:
          id: led_sign
```

**In Home Assistant:** Find your device, open the "Message" entity, type text, and watch it appear on your sign!

## Full Configuration

```yaml
betabrite:
  id: led_sign
  uart_id: sign_uart
  sign_type: betabrite
  address: "00"
  max_files: 5

  defaults:
    color: green
    mode: rotate
    charset: 7high
    position: topline
    speed: 3
    effect: twinkle

  clock:
    enabled: true
    interval: 60s
    duration: 4s
    format: 12h
    color: amber

  priority:
    warning_duration: 2500ms
    default_duration: 25s

  offline_messages:
    - text: "Company Name"
      color: green
      mode: hold
      duration: 10s
      effect: welcome

    - text: "123 Main Street"
      color: amber
      mode: scroll
      duration: 15s
```

## Configuration Options

### Main Component

| Option | Type | Default | Description |
|--------|------|---------|-------------|
| `sign_type` | string | `all` | Sign type: `all`, `betabrite`, `1line`, `2line` |
| `address` | string | `"00"` | Two-character sign address |
| `max_files` | int | `5` | Number of text file slots (1-26) |
| `defaults` | object | - | Default display settings |
| `clock` | object | - | Clock display settings |
| `offline_messages` | list | - | Messages for offline mode |

### Defaults

| Option | Values | Default |
|--------|--------|---------|
| `color` | red, green, amber, orange, yellow, rainbow1, rainbow2, autocolor | green |
| `mode` | rotate, hold, flash, scroll, wipein, wipeout, explode, etc. | rotate |
| `charset` | 5high, 7high, 10high, 7highfancy, etc. | 7high |
| `position` | topline, midline, botline, fill, left, right | topline |
| `speed` | 1-5 | 3 |
| `effect` | twinkle, sparkle, snow, welcome, fireworks, bomb, etc. | twinkle |

### Clock

| Option | Type | Default | Description |
|--------|------|---------|-------------|
| `enabled` | bool | `true` | Enable automatic clock display |
| `interval` | time | `60s` | Time between clock displays |
| `duration` | time | `4s` | How long to show clock |
| `format` | string | `12h` | `12h` or `24h` |
| `color` | string | `amber` | Clock text color |

## Actions

Use these actions in ESPHome automations:

### `betabrite.display`

Display a message on the sign:

```yaml
- betabrite.display:
    id: led_sign
    text: "Hello World!"
    color: green       # optional
    mode: rotate       # optional
    effect: twinkle    # optional
```

### `betabrite.priority`

Display a priority alert message with warning animation:

```yaml
- betabrite.priority:
    id: led_sign
    text: "EMERGENCY ALERT!"
    duration: 60s      # optional, default 25s
```

### `betabrite.cancel_priority`

Cancel any active priority message:

```yaml
- betabrite.cancel_priority:
    id: led_sign
```

### `betabrite.clock`

Display the clock immediately:

```yaml
- betabrite.clock:
    id: led_sign
```

### `betabrite.clear`

Clear all messages from the sign:

```yaml
- betabrite.clear:
    id: led_sign
```

### `betabrite.demo`

Run a demonstration of colors and effects:

```yaml
- betabrite.demo:
    id: led_sign
```

## Home Assistant Integration

### Custom Services

Define custom services in your API configuration:

```yaml
api:
  encryption:
    key: !secret api_encryption_key
  services:
    - service: display_message
      variables:
        message: string
        color: string
      then:
        - betabrite.display:
            id: led_sign
            text: !lambda 'return message;'
            color: !lambda 'return color;'
```

Call from Home Assistant:

```yaml
service: esphome.led_sign_display_message
data:
  message: "Hello from HA!"
  color: "amber"
```

### Template Entities

Create input controls in ESPHome that appear in Home Assistant:

```yaml
text:
  - platform: template
    name: "Display Message"
    on_value:
      - betabrite.display:
          id: led_sign
          text: !lambda 'return x;'

select:
  - platform: template
    name: "Display Color"
    options: ["red", "green", "amber", "orange", "yellow"]
    initial_option: "green"
```

## Protocol Reference

### Colors

| Name | Code | Description |
|------|------|-------------|
| red | 1 | Bright red |
| green | 2 | Bright green |
| amber | 3 | Yellow/orange |
| dimred | 4 | Dim red |
| dimgreen | 5 | Dim green |
| brown | 6 | Brown |
| orange | 7 | Orange |
| yellow | 8 | Yellow |
| rainbow1 | 9 | Rainbow pattern 1 |
| rainbow2 | A | Rainbow pattern 2 |
| colormix | B | Color mix |
| autocolor | C | Automatic color |

### Display Modes

| Name | Description |
|------|-------------|
| rotate | Text rotates on screen |
| hold | Static display |
| flash | Flashing text |
| scroll | Scrolling text |
| rollup/rolldown | Roll vertically |
| rollleft/rollright | Roll horizontally |
| wipein/wipeout | Wipe effects |
| explode | Explode effect |

### Special Effects

| Name | Description |
|------|-------------|
| twinkle | Twinkling stars |
| sparkle | Sparkle effect |
| snow | Falling snow |
| welcome | Welcome animation |
| fireworks | Fireworks display |
| bomb | Bomb explosion |
| newsflash | News flash effect |
| trumpet | Trumpet fanfare |

## Troubleshooting

### No Display Output

1. Verify wiring connections (TX/RX may need swapping)
2. Check baud rate is 9600
3. Verify data format is 7E1 (7 data bits, even parity, 1 stop bit)
4. Ensure RS232 level shifter is working

### Garbled Text

1. Check character encoding settings
2. Verify baud rate matches sign configuration
3. Try different charset options

### Messages Not Updating

1. Check max_files setting (sign may be full)
2. Use `betabrite.clear` to reset sign memory
3. Verify UART is not being used by other components

## Resources

- [ESPHome Documentation](https://esphome.io/)
- [ESPHome External Components](https://esphome.io/components/external_components.html)
- [Alpha Protocol Documentation (Official PDF)](https://www.alpha-american.com/alpha-manuals/M-Protocol.pdf)
- [Alpha-American Technical Manuals](https://alpha-american.com/Technical-Manuals.html)
- [Report Issues](https://github.com/jimidarke/ledSignController/issues)

## License

MIT License - See [LICENSE](LICENSE) for details.

This is an independent implementation based on the publicly documented Alpha Sign
Communications Protocol. See [NOTICE.md](NOTICE.md) for protocol documentation
sources, legal basis, and trademark information.

Not affiliated with Alpha-American or Adaptive Micro Systems.

## Contributing

Contributions welcome! Please submit issues and pull requests on [GitHub](https://github.com/jimidarke/ledSignController).
