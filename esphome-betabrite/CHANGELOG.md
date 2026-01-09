# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added
- Initial public release of ESPHome BetaBrite component
- Full Alpha Protocol support for BetaBrite LED signs
- Message display with configurable color, mode, charset, position, speed, and effects
- Priority message system with two-stage warning animation
- Offline message cycling when network is disconnected
- Automatic clock display with configurable intervals
- Home Assistant integration via ESPHome API
- Custom services for display_message and priority_alert
- Template entities for Home Assistant UI controls
- YAML automation actions: `betabrite.display`, `betabrite.priority`, `betabrite.clear`, `betabrite.demo`
- Optional MQTT integration for Alert Manager compatibility
- Comprehensive protocol definitions for 50+ sign types
- Support for 12 colors, 20+ display modes, 19 special effects, 14 character sets

### Hardware Support
- ESP32 development boards (ESP32-WROOM-32, ESP32-DevKitC, etc.)
- BetaBrite LED signs with RS232 interface
- Alpha Protocol compatible signs (AlphaVision, AlphaPremiere, AlphaEclipse, etc.)

## [0.1.0] - 2025-01-02

### Added
- Initial component structure following ESPHome external component standards
- Python configuration validation and code generation (`__init__.py`)
- C++ component implementation (`betabrite.h`, `betabrite.cpp`)
- Protocol definitions (`bbdefs.h`)
- Automation actions (`automation.h`)
- Example configuration (`example_led_sign.yaml`)
- Documentation (`README.md`)
