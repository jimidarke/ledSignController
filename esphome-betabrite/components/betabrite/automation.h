/**
 * @file automation.h
 * @brief ESPHome BetaBrite Automation Actions
 *
 * Defines action classes for use in ESPHome automations.
 * These allow betabrite.display, betabrite.clear, etc. in YAML.
 */

#pragma once

#include "betabrite.h"
#include "esphome/core/automation.h"

namespace esphome {
namespace betabrite {

/**
 * @brief Action to display a message on the LED sign
 */
template<typename... Ts>
class DisplayMessageAction : public Action<Ts...> {
 public:
  explicit DisplayMessageAction(BetaBriteComponent *parent) : parent_(parent) {}

  TEMPLATABLE_VALUE(std::string, message)
  TEMPLATABLE_VALUE(std::string, color)
  TEMPLATABLE_VALUE(std::string, mode)
  TEMPLATABLE_VALUE(std::string, effect)

  void play(const Ts &...x) override {
    std::string msg = this->message_.value(x...);
    std::string col = this->color_.has_value() ? this->color_.value(x...) : "";
    std::string mod = this->mode_.has_value() ? this->mode_.value(x...) : "";
    std::string eff = this->effect_.has_value() ? this->effect_.value(x...) : "";

    if (col.empty() && mod.empty() && eff.empty()) {
      this->parent_->display_message(msg);
    } else {
      this->parent_->display_message(msg, col, mod, eff);
    }
  }

 protected:
  BetaBriteComponent *parent_;
};

/**
 * @brief Action to clear the LED sign display
 */
template<typename... Ts>
class ClearAction : public Action<Ts...> {
 public:
  explicit ClearAction(BetaBriteComponent *parent) : parent_(parent) {}

  void play(const Ts &...x) override {
    this->parent_->clear_display();
  }

 protected:
  BetaBriteComponent *parent_;
};

/**
 * @brief Action to display a priority message
 */
template<typename... Ts>
class PriorityMessageAction : public Action<Ts...> {
 public:
  explicit PriorityMessageAction(BetaBriteComponent *parent) : parent_(parent) {}

  TEMPLATABLE_VALUE(std::string, message)
  TEMPLATABLE_VALUE(uint32_t, duration)

  void play(const Ts &...x) override {
    std::string msg = this->message_.value(x...);
    uint32_t dur = this->duration_.has_value() ? this->duration_.value(x...) : 0;
    this->parent_->display_priority_message(msg, dur);
  }

 protected:
  BetaBriteComponent *parent_;
};

/**
 * @brief Action to run the demo sequence
 */
template<typename... Ts>
class DemoAction : public Action<Ts...> {
 public:
  explicit DemoAction(BetaBriteComponent *parent) : parent_(parent) {}

  void play(const Ts &...x) override {
    this->parent_->run_demo();
  }

 protected:
  BetaBriteComponent *parent_;
};

/**
 * @brief Action to cancel any priority message
 */
template<typename... Ts>
class CancelPriorityAction : public Action<Ts...> {
 public:
  explicit CancelPriorityAction(BetaBriteComponent *parent) : parent_(parent) {}

  void play(const Ts &...x) override {
    this->parent_->cancel_priority_message();
  }

 protected:
  BetaBriteComponent *parent_;
};

/**
 * @brief Action to display the clock
 */
template<typename... Ts>
class DisplayClockAction : public Action<Ts...> {
 public:
  explicit DisplayClockAction(BetaBriteComponent *parent) : parent_(parent) {}

  void play(const Ts &...x) override {
    this->parent_->display_clock();
  }

 protected:
  BetaBriteComponent *parent_;
};

/**
 * @brief Action to set the sign's internal clock
 */
template<typename... Ts>
class SetTimeAction : public Action<Ts...> {
 public:
  explicit SetTimeAction(BetaBriteComponent *parent) : parent_(parent) {}

  TEMPLATABLE_VALUE(uint8_t, hour)
  TEMPLATABLE_VALUE(uint8_t, minute)
  TEMPLATABLE_VALUE(uint8_t, month)
  TEMPLATABLE_VALUE(uint8_t, day)
  TEMPLATABLE_VALUE(uint16_t, year)
  TEMPLATABLE_VALUE(uint8_t, day_of_week)
  TEMPLATABLE_VALUE(bool, use_24h)

  void play(const Ts &...x) override {
    uint8_t h = this->hour_.has_value() ? this->hour_.value(x...) : 0;
    uint8_t m = this->minute_.has_value() ? this->minute_.value(x...) : 0;
    uint8_t mo = this->month_.has_value() ? this->month_.value(x...) : 1;
    uint8_t d = this->day_.has_value() ? this->day_.value(x...) : 1;
    uint16_t y = this->year_.has_value() ? this->year_.value(x...) : 2025;
    uint8_t dow = this->day_of_week_.has_value() ? this->day_of_week_.value(x...) : 0;
    bool use24 = this->use_24h_.has_value() ? this->use_24h_.value(x...) : false;

    this->parent_->set_time(h, m, mo, d, y, dow, use24);
  }

 protected:
  BetaBriteComponent *parent_;
};

}  // namespace betabrite
}  // namespace esphome
