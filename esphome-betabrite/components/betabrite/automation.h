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

}  // namespace betabrite
}  // namespace esphome
