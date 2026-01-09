/**
 * @file betabrite.h
 * @brief ESPHome BetaBrite LED Sign Component
 *
 * External component for controlling BetaBrite/Alpha Protocol LED signs
 * via ESPHome. Supports message display, effects, colors, offline message
 * cycling, and Home Assistant integration.
 *
 * Independent implementation based on publicly available Alpha Protocol
 * documentation. MIT License - see LICENSE file.
 */

#pragma once

#include "esphome/core/component.h"
#include "esphome/core/hal.h"
#include "esphome/core/log.h"
#include "esphome/components/uart/uart.h"
#include "bbdefs.h"

#include <vector>
#include <string>

namespace esphome {
namespace betabrite {

/**
 * @brief Structure for offline message configuration
 */
struct OfflineMessage {
  std::string text;
  CharColor color;
  DisplayMode mode;
  SpecialMode effect;
  CharSet charset;
  DisplayPosition position;
  int speed;
  uint32_t duration_ms;
  bool use_effect;
};

/**
 * @brief ESPHome component for BetaBrite LED signs
 *
 * This component provides full control over BetaBrite/Alpha Protocol LED signs
 * including message display, effects, colors, priority messages, and offline
 * message cycling when network is unavailable.
 */
class BetaBriteComponent : public Component, public uart::UARTDevice {
 public:
  BetaBriteComponent() = default;

  // ESPHome Component interface
  void setup() override;
  void loop() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::DATA; }

  // Configuration setters (called from Python codegen)
  void set_sign_type(SignType type) { this->sign_type_ = type; }
  void set_address(const std::string &addr);
  void set_max_files(uint8_t max_files) { this->max_files_ = max_files; }

  // Default display settings
  void set_default_color(CharColor color) { this->default_color_ = color; }
  void set_default_mode(DisplayMode mode) { this->default_mode_ = mode; }
  void set_default_charset(CharSet charset) { this->default_charset_ = charset; }
  void set_default_position(DisplayPosition pos) { this->default_position_ = pos; }
  void set_default_speed(int speed) { this->default_speed_ = speed; }
  void set_default_effect(SpecialMode effect) { this->default_effect_ = effect; }

  // Clock configuration
  void set_clock_enabled(bool enabled) { this->clock_enabled_ = enabled; }
  void set_clock_interval(uint32_t ms) { this->clock_interval_ms_ = ms; }
  void set_clock_duration(uint32_t ms) { this->clock_duration_ms_ = ms; }
  void set_clock_24h(bool is_24h) { this->clock_24h_ = is_24h; }
  void set_clock_color(CharColor color) { this->clock_color_ = color; }

  // Priority message configuration
  void set_priority_warning_duration(uint32_t ms) { this->priority_warning_duration_ms_ = ms; }
  void set_priority_default_duration(uint32_t ms) { this->priority_default_duration_ms_ = ms; }

  // Offline message management
  void add_offline_message(const std::string &text, const std::string &color,
                           const std::string &mode, uint32_t duration_ms,
                           const std::string &effect, const std::string &charset,
                           const std::string &position, int speed);

  // Public API for actions and Home Assistant
  void display_message(const std::string &message);
  void display_message(const std::string &message, const std::string &color,
                       const std::string &mode, const std::string &effect);
  void display_message_full(const std::string &message, CharColor color,
                            DisplayMode mode, CharSet charset,
                            DisplayPosition position, int speed,
                            SpecialMode effect, bool use_effect);
  void display_priority_message(const std::string &message, uint32_t duration_s = 0);
  void cancel_priority_message();
  void display_clock();
  void clear_display();
  void run_demo();

  // State queries
  bool is_in_priority_mode() const { return this->in_priority_mode_; }
  bool is_in_offline_mode() const { return this->in_offline_mode_; }
  char get_current_file() const { return this->current_file_; }
  uint8_t get_message_count() const { return this->message_count_; }

 protected:
  // Alpha Protocol low-level methods
  void sync_();
  void begin_command_();
  void begin_nested_command_();
  void end_command_();
  void end_nested_command_();
  void delay_between_commands_();

  // File operations
  void write_text_file_(char name, const std::string &contents,
                        CharColor color, DisplayPosition position,
                        DisplayMode mode, SpecialMode effect, bool use_effect,
                        CharSet charset, int speed);
  void write_priority_text_file_(const std::string &contents,
                                 CharColor color, DisplayPosition position,
                                 DisplayMode mode, SpecialMode effect, bool use_effect);
  void cancel_priority_text_file_();
  void write_string_file_(char name, const std::string &contents);
  void set_memory_configuration_(char start_file, uint8_t num_files, uint16_t size);

  // State management
  void check_priority_timeout_();
  void check_clock_display_();
  void check_offline_mode_();
  void advance_offline_message_();
  void advance_to_next_file_();
  bool is_network_connected_();

  // Configuration
  SignType sign_type_{ST_ALL};
  char address_[2]{'0', '0'};
  uint8_t max_files_{5};

  // Default display settings
  CharColor default_color_{COL_GREEN};
  DisplayMode default_mode_{DM_ROTATE};
  CharSet default_charset_{CS_7HIGH};
  DisplayPosition default_position_{DP_TOPLINE};
  int default_speed_{3};
  SpecialMode default_effect_{SDM_TWINKLE};

  // Clock settings
  bool clock_enabled_{true};
  uint32_t clock_interval_ms_{60000};
  uint32_t clock_duration_ms_{4000};
  bool clock_24h_{false};
  CharColor clock_color_{COL_AMBER};

  // Priority message settings
  uint32_t priority_warning_duration_ms_{2500};
  uint32_t priority_default_duration_ms_{25000};

  // Offline message list
  std::vector<OfflineMessage> offline_messages_;

  // Runtime state
  bool initialized_{false};
  char current_file_{'A'};
  uint8_t message_count_{0};

  // Priority message state
  bool in_priority_mode_{false};
  uint32_t priority_start_time_{0};
  uint32_t priority_end_time_{0};
  enum PriorityStage { PRIORITY_NONE, PRIORITY_WARNING, PRIORITY_MESSAGE };
  PriorityStage priority_stage_{PRIORITY_NONE};
  std::string priority_message_content_;

  // Clock state
  uint32_t last_clock_display_{0};

  // Offline mode state
  bool in_offline_mode_{false};
  size_t offline_current_index_{0};
  uint32_t offline_stage_start_{0};
  bool was_connected_{false};
};

}  // namespace betabrite
}  // namespace esphome

// Include automation actions after component definition
#include "automation.h"
