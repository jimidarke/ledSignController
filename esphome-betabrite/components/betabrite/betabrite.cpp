/**
 * @file betabrite.cpp
 * @brief ESPHome BetaBrite LED Sign Component Implementation
 */

#include "betabrite.h"
#include "esphome/core/application.h"
#ifdef USE_WIFI
#include "esphome/components/wifi/wifi_component.h"
#endif

namespace esphome {
namespace betabrite {

static const char *const TAG = "betabrite";

void BetaBriteComponent::setup() {
  ESP_LOGCONFIG(TAG, "Setting up BetaBrite...");

  // Configure memory on the sign
  // File A = Clock, File B = Messages
  ESP_LOGD(TAG, "Configuring sign memory (A=clock, B=message)...");
  this->set_memory_configuration_('A', 2, 256);  // Only 2 files: A and B

  // Small delay for sign to process
  delay(500);

  this->initialized_ = true;
  this->message_count_ = 0;

  // Display clock on file A
  this->display_clock();

  ESP_LOGCONFIG(TAG, "BetaBrite initialized successfully");
}

void BetaBriteComponent::loop() {
  if (!this->initialized_) {
    return;
  }

  uint32_t now = millis();

  // Check priority message timeout
  this->check_priority_timeout_();

  // Check if we should display the clock
  if (!this->in_priority_mode_ && !this->in_offline_mode_) {
    this->check_clock_display_();
  }

  // Check offline mode (when network disconnected)
  this->check_offline_mode_();
}

void BetaBriteComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "BetaBrite LED Sign:");
  ESP_LOGCONFIG(TAG, "  Sign Type: 0x%02X", this->sign_type_);
  ESP_LOGCONFIG(TAG, "  Address: %c%c", this->address_[0], this->address_[1]);
  ESP_LOGCONFIG(TAG, "  Max Files: %d", this->max_files_);
  ESP_LOGCONFIG(TAG, "  Default Color: %c", this->default_color_);
  ESP_LOGCONFIG(TAG, "  Default Mode: %c", this->default_mode_);
  ESP_LOGCONFIG(TAG, "  Clock Enabled: %s", YESNO(this->clock_enabled_));
  ESP_LOGCONFIG(TAG, "  Clock Interval: %u ms", this->clock_interval_ms_);
  ESP_LOGCONFIG(TAG, "  Offline Messages: %d", this->offline_messages_.size());
}

void BetaBriteComponent::set_address(const std::string &addr) {
  if (addr.length() >= 2) {
    this->address_[0] = addr[0];
    this->address_[1] = addr[1];
  }
}

void BetaBriteComponent::add_offline_message(const std::string &text,
                                              const std::string &color,
                                              const std::string &mode,
                                              uint32_t duration_ms,
                                              const std::string &effect,
                                              const std::string &charset,
                                              const std::string &position,
                                              int speed) {
  OfflineMessage msg;
  msg.text = text;
  msg.color = color_from_string(color);
  msg.mode = mode_from_string(mode);
  msg.effect = effect_from_string(effect);
  msg.charset = charset_from_string(charset);
  msg.position = position_from_string(position);
  msg.speed = speed;
  msg.duration_ms = duration_ms;
  msg.use_effect = !effect.empty();

  this->offline_messages_.push_back(msg);
  ESP_LOGD(TAG, "Added offline message: %s (duration: %u ms)", text.c_str(), duration_ms);
}

// ============================================================================
// Public API Methods
// ============================================================================

void BetaBriteComponent::display_message(const std::string &message) {
  this->display_message_full(message, this->default_color_, this->default_mode_,
                              this->default_charset_, this->default_position_,
                              this->default_speed_, this->default_effect_, false);
}

void BetaBriteComponent::display_message(const std::string &message,
                                          const std::string &color,
                                          const std::string &mode,
                                          const std::string &effect) {
  CharColor col = color.empty() ? this->default_color_ : color_from_string(color);
  DisplayMode mod = mode.empty() ? this->default_mode_ : mode_from_string(mode);
  SpecialMode eff = effect.empty() ? this->default_effect_ : effect_from_string(effect);
  bool use_eff = !effect.empty();

  this->display_message_full(message, col, mod, this->default_charset_,
                              this->default_position_, this->default_speed_,
                              eff, use_eff);
}

void BetaBriteComponent::display_message_full(const std::string &message,
                                               CharColor color,
                                               DisplayMode mode,
                                               CharSet charset,
                                               DisplayPosition position,
                                               int speed,
                                               SpecialMode effect,
                                               bool use_effect) {
  if (this->in_priority_mode_) {
    ESP_LOGD(TAG, "Ignoring message - priority mode active");
    return;
  }

  // Cancel offline mode when displaying manual message
  this->in_offline_mode_ = false;

  ESP_LOGD(TAG, "Displaying message on file B: %s", message.c_str());

  // Use special mode if effect is requested
  DisplayMode actual_mode = use_effect ? DM_SPECIAL : mode;

  // Messages always go to file B (file A is reserved for clock)
  this->write_text_file_('B', message, color, position,
                         actual_mode, effect, use_effect, charset, speed);

  this->message_count_++;
}

void BetaBriteComponent::display_priority_message(const std::string &message,
                                                   uint32_t duration_s) {
  ESP_LOGD(TAG, "Displaying priority message: %s", message.c_str());

  // Store message for display after warning
  this->priority_message_content_ = message;

  // Calculate duration
  uint32_t duration_ms = (duration_s > 0) ? (duration_s * 1000) : this->priority_default_duration_ms_;

  // Start with warning display
  this->write_priority_text_file_("!! ALERT !!", COL_RED, DP_FILL, DM_FLASH,
                                  SDM_BOMB, true);

  this->in_priority_mode_ = true;
  this->priority_stage_ = PRIORITY_WARNING;
  this->priority_start_time_ = millis();
  this->priority_end_time_ = this->priority_start_time_ + this->priority_warning_duration_ms_ + duration_ms;
}

void BetaBriteComponent::cancel_priority_message() {
  if (!this->in_priority_mode_) {
    return;
  }

  ESP_LOGD(TAG, "Cancelling priority message");
  this->cancel_priority_text_file_();
  this->in_priority_mode_ = false;
  this->priority_stage_ = PRIORITY_NONE;
}

void BetaBriteComponent::display_clock() {
  if (this->in_priority_mode_) {
    return;
  }

  ESP_LOGD(TAG, "Displaying clock on file A");

  // Build clock display string with call-time format code
  std::string clock_str;
  clock_str += FC_CALLTIME;  // This tells the sign to display its internal time

  // Clock always goes to file A
  this->write_text_file_('A', clock_str, this->clock_color_,
                         DP_TOPLINE, DM_HOLD, SDM_TWINKLE, false, CS_7HIGH, 3);
}

void BetaBriteComponent::clear_display() {
  ESP_LOGD(TAG, "Clearing display");

  // Cancel any priority message
  this->cancel_priority_message();

  // Reset memory configuration (2 files: A=clock, B=message)
  this->set_memory_configuration_('A', 2, 256);

  this->message_count_ = 0;

  // Show clock after clearing
  delay(200);
  this->display_clock();
}

void BetaBriteComponent::run_demo() {
  ESP_LOGD(TAG, "Running demo sequence");

  // Disable clock display during demo
  bool clock_was_enabled = this->clock_enabled_;
  this->clock_enabled_ = false;

  // Demo sequence showing various colors and effects
  const char *demo_colors[] = {"red", "green", "amber", "orange", "yellow"};
  const char *demo_modes[] = {"rotate", "scroll", "flash", "wipein", "explode"};
  const char *demo_effects[] = {"twinkle", "sparkle", "welcome", "fireworks", "bomb"};

  for (int i = 0; i < 5; i++) {
    std::string msg = "Demo Mode " + std::to_string(i + 1);
    this->display_message(msg, demo_colors[i], demo_modes[i], demo_effects[i]);
    delay(4000);
  }

  // Re-enable clock and show it
  this->clock_enabled_ = clock_was_enabled;
  this->display_clock();
  ESP_LOGD(TAG, "Demo complete");
}

void BetaBriteComponent::set_time(uint8_t hour, uint8_t minute, uint8_t month,
                                   uint8_t day, uint16_t year, uint8_t day_of_week,
                                   bool use_24h) {
  ESP_LOGD(TAG, "Setting time: %02d:%02d %02d/%02d/%04d (dow=%d, 24h=%s)",
           hour, minute, month, day, year, day_of_week, use_24h ? "yes" : "no");

  char buf[8];

  this->begin_command_();

  // Set time (HHMM)
  this->begin_nested_command_();
  this->delay_between_commands_();
  this->write_byte(CC_WSPFUNC);
  this->write_byte(' ');  // Set Time command
  snprintf(buf, sizeof(buf), "%02d%02d", hour, minute);
  this->write_str(buf);
  this->end_nested_command_();

  // Set time format (M=24h military, S=12h standard)
  this->begin_nested_command_();
  this->delay_between_commands_();
  this->write_byte(CC_WSPFUNC);
  this->write_byte('\'');  // Time format command (0x27)
  this->write_byte(use_24h ? 'M' : 'S');
  this->end_nested_command_();

  // Set day of week (1=Sunday through 7=Saturday)
  this->begin_nested_command_();
  this->delay_between_commands_();
  this->write_byte(CC_WSPFUNC);
  this->write_byte('&');  // Day of week command
  this->write_byte('1' + (day_of_week % 7));
  this->end_nested_command_();

  // Set date (MMDDYY)
  this->begin_nested_command_();
  this->delay_between_commands_();
  this->write_byte(CC_WSPFUNC);
  this->write_byte(';');  // Set Date command
  uint8_t yy = (year >= 2000) ? (year - 2000) : 0;
  snprintf(buf, sizeof(buf), "%02d%02d%02d", month, day, yy);
  this->write_str(buf);
  this->end_nested_command_();

  this->end_command_();

  ESP_LOGD(TAG, "Time set complete");
}

// ============================================================================
// Alpha Protocol Low-Level Methods
// ============================================================================

void BetaBriteComponent::sync_() {
  for (int i = 0; i < 5; i++) {
    this->write_byte(BB_NUL);
  }
}

void BetaBriteComponent::begin_command_() {
  this->sync_();
  this->write_byte(BB_SOH);
  this->write_byte(this->sign_type_);
  this->write_byte(this->address_[0]);
  this->write_byte(this->address_[1]);
}

void BetaBriteComponent::begin_nested_command_() {
  this->write_byte(BB_STX);
}

void BetaBriteComponent::end_command_() {
  this->write_byte(BB_EOT);
}

void BetaBriteComponent::end_nested_command_() {
  this->write_byte(BB_ETX);
}

void BetaBriteComponent::delay_between_commands_() {
  delay(BETWEEN_COMMAND_DELAY_MS);
}

void BetaBriteComponent::write_text_file_(char name, const std::string &contents,
                                           CharColor color, DisplayPosition position,
                                           DisplayMode mode, SpecialMode effect,
                                           bool use_effect, CharSet charset, int speed) {
  this->begin_command_();
  this->begin_nested_command_();

  // Write text command
  this->write_byte(CC_WTEXT);
  this->write_byte(name);

  // Position and mode
  this->write_byte(BB_ESC);
  this->write_byte(position);
  this->write_byte(mode);

  // Special effect if using special mode
  if (mode == DM_SPECIAL && use_effect) {
    this->write_byte(effect);
  }

  // Character set selection
  this->write_byte(FC_SELECTCHARSET);
  this->write_byte(charset);

  // Speed control
  this->write_byte(speed_code_from_int(speed));

  // Color (if not autocolor)
  if (color != COL_AUTOCOLOR) {
    this->write_byte(FC_SELECTCHARCOLOR);
    this->write_byte(color);
  }

  // Write the actual content
  this->write_str(contents.c_str());

  // Note: Do NOT send ETX for text files - just EOT
  this->end_command_();
}

void BetaBriteComponent::write_priority_text_file_(const std::string &contents,
                                                    CharColor color,
                                                    DisplayPosition position,
                                                    DisplayMode mode,
                                                    SpecialMode effect,
                                                    bool use_effect) {
  this->write_text_file_(PRIORITY_FILE_LABEL, contents, color, position,
                         mode, effect, use_effect, CS_10HIGH, 5);
}

void BetaBriteComponent::cancel_priority_text_file_() {
  this->begin_command_();
  this->begin_nested_command_();
  this->write_byte(CC_WTEXT);
  this->write_byte(PRIORITY_FILE_LABEL);
  this->end_command_();
}

void BetaBriteComponent::write_string_file_(char name, const std::string &contents) {
  this->begin_command_();
  this->begin_nested_command_();
  this->write_byte(CC_WSTRING);
  this->write_byte(name);
  this->write_str(contents.c_str());
  this->end_command_();
}

void BetaBriteComponent::set_memory_configuration_(char start_file, uint8_t num_files,
                                                    uint16_t size) {
  this->begin_command_();
  this->begin_nested_command_();

  // Write special function - clear memory
  this->write_byte(CC_WSPFUNC);
  this->write_byte(SFL_CLEARMEM);

  // Format size as 4-digit hex (lowercase to match protocol)
  char size_buf[5];
  snprintf(size_buf, sizeof(size_buf), "%04x", size);

  // Configure each file
  for (char c = start_file; c < start_file + num_files && c <= 'Z'; c++) {
    this->write_byte(c);
    this->write_byte(SFFT_TEXT);
    this->write_byte(SFKPS_LOCKED);
    this->write_str(size_buf);
    this->write_str("FF00");  // AlwaysOn for text file
  }

  this->end_command_();

  // Give the sign time to reconfigure memory
  delay(500);
}

// ============================================================================
// State Management Methods
// ============================================================================

void BetaBriteComponent::check_priority_timeout_() {
  if (!this->in_priority_mode_) {
    return;
  }

  uint32_t now = millis();

  if (this->priority_stage_ == PRIORITY_WARNING) {
    // Check if warning period is over
    if (now - this->priority_start_time_ >= this->priority_warning_duration_ms_) {
      // Switch to actual message
      ESP_LOGD(TAG, "Priority warning complete, showing message");
      this->write_priority_text_file_(this->priority_message_content_, COL_RED,
                                      DP_FILL, DM_FLASH, SDM_NEWSFLASH, true);
      this->priority_stage_ = PRIORITY_MESSAGE;
    }
  } else if (this->priority_stage_ == PRIORITY_MESSAGE) {
    // Check if message duration is over
    if (now >= this->priority_end_time_) {
      ESP_LOGD(TAG, "Priority message timeout, cancelling");
      this->cancel_priority_message();
    }
  }
}

void BetaBriteComponent::check_clock_display_() {
  if (!this->clock_enabled_) {
    return;
  }

  uint32_t now = millis();
  if (now - this->last_clock_display_ >= this->clock_interval_ms_) {
    this->display_clock();
    this->last_clock_display_ = now;
  }
}

void BetaBriteComponent::check_offline_mode_() {
  if (this->offline_messages_.empty()) {
    return;
  }

  bool connected = this->is_network_connected_();

  // Detect transition from connected to disconnected
  if (this->was_connected_ && !connected) {
    ESP_LOGD(TAG, "Network disconnected, entering offline mode");
    this->in_offline_mode_ = true;
    this->offline_current_index_ = 0;
    this->offline_stage_start_ = millis();

    // Display first offline message
    this->advance_offline_message_();
  }

  // Detect transition from disconnected to connected
  if (!this->was_connected_ && connected) {
    ESP_LOGD(TAG, "Network reconnected, exiting offline mode");
    this->in_offline_mode_ = false;
  }

  this->was_connected_ = connected;

  // Cycle through offline messages
  if (this->in_offline_mode_) {
    uint32_t now = millis();
    const OfflineMessage &current = this->offline_messages_[this->offline_current_index_];

    if (now - this->offline_stage_start_ >= current.duration_ms) {
      // Move to next message
      this->offline_current_index_ = (this->offline_current_index_ + 1) % this->offline_messages_.size();
      this->offline_stage_start_ = now;
      this->advance_offline_message_();
    }
  }
}

void BetaBriteComponent::advance_offline_message_() {
  if (this->offline_messages_.empty()) {
    return;
  }

  const OfflineMessage &msg = this->offline_messages_[this->offline_current_index_];
  ESP_LOGD(TAG, "Showing offline message %d: %s", this->offline_current_index_, msg.text.c_str());

  DisplayMode mode = msg.use_effect ? DM_SPECIAL : msg.mode;

  // Offline messages go to file B (same as regular messages)
  this->write_text_file_('B', msg.text, msg.color, msg.position,
                         mode, msg.effect, msg.use_effect, msg.charset, msg.speed);
}

void BetaBriteComponent::advance_to_next_file_() {
  this->current_file_++;
  if (this->current_file_ >= 'A' + this->max_files_) {
    this->current_file_ = 'A';
  }
}

bool BetaBriteComponent::is_network_connected_() {
  // Check if WiFi has active network connectivity
#ifdef USE_WIFI
  return wifi::global_wifi_component != nullptr && wifi::global_wifi_component->is_connected();
#else
  return true;  // Assume connected if no WiFi
#endif
}

}  // namespace betabrite
}  // namespace esphome
