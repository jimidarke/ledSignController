"""
ESPHome BetaBrite LED Sign Component

External component for controlling BetaBrite/Alpha Protocol LED signs.
Supports message display, effects, colors, offline message cycling,
and Home Assistant integration.

Usage in YAML:
  external_components:
    - source: github://username/esphome-betabrite
      components: [betabrite]

  uart:
    tx_pin: GPIO17
    rx_pin: GPIO16
    baud_rate: 9600

  betabrite:
    id: led_sign
    # ... configuration options
"""

import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import automation
from esphome.components import uart
from esphome.const import (
    CONF_ID,
    CONF_TRIGGER_ID,
)

DEPENDENCIES = ["uart", "wifi"]
AUTO_LOAD = []
MULTI_CONF = False

CODEOWNERS = ["@jimidarke"]

# Namespace
betabrite_ns = cg.esphome_ns.namespace("betabrite")
BetaBriteComponent = betabrite_ns.class_(
    "BetaBriteComponent", cg.Component, uart.UARTDevice
)

# Configuration keys
CONF_SIGN_TYPE = "sign_type"
CONF_ADDRESS = "address"
CONF_MAX_FILES = "max_files"
CONF_DEFAULTS = "defaults"
CONF_OFFLINE_MESSAGES = "offline_messages"
CONF_CLOCK = "clock"
CONF_PRIORITY = "priority"

# Defaults sub-keys
CONF_COLOR = "color"
CONF_MODE = "mode"
CONF_CHARSET = "charset"
CONF_POSITION = "position"
CONF_SPEED = "speed"
CONF_EFFECT = "effect"

# Offline message keys
CONF_TEXT = "text"
CONF_DURATION = "duration"

# Clock keys
CONF_ENABLED = "enabled"
CONF_INTERVAL = "interval"
CONF_FORMAT = "format"

# Priority keys
CONF_WARNING_DURATION = "warning_duration"
CONF_DEFAULT_DURATION = "default_duration"

# Sign type options
SIGN_TYPES = {
    "all": "ST_ALL",
    "betabrite": "ST_BETABRITE",
    "1line": "ST_1LINE",
    "2line": "ST_2LINE",
    "alphavision": "ST_ALPHA_VISION",
    "alphaeclipse": "ST_ALPHA_ECLIPSE_3600",
    "alphapremiere": "ST_ALPHA_PREMIERE",
}

# Color options
COLORS = [
    "red", "green", "amber", "dimred", "dimgreen", "brown",
    "orange", "yellow", "rainbow1", "rainbow2", "colormix", "autocolor"
]

# Display mode options
MODES = [
    "rotate", "hold", "flash", "rollup", "rolldown", "rollleft",
    "rollright", "wipeup", "wipedown", "wipeleft", "wiperight",
    "scroll", "special", "automode", "rollin", "rollout",
    "wipein", "wipeout", "comprotate", "explode", "clock"
]

# Special effect options
EFFECTS = [
    "twinkle", "sparkle", "snow", "interlock", "switch", "slide",
    "spray", "starburst", "welcome", "slots", "newsflash", "trumpet",
    "cyclecolors", "thankyou", "nosmoking", "dontdrinkanddrive",
    "fishimal", "fireworks", "turballoon", "bomb"
]

# Character set options
CHARSETS = [
    "5high", "5stroke", "7high", "7stroke", "7highfancy",
    "10high", "7shadow", "fhighfancy", "fhigh", "7shadowfancy",
    "5wide", "7wide", "7widefancy", "5widestroke"
]

# Position options
POSITIONS = ["midline", "topline", "botline", "fill", "left", "right"]

# Clock format options
CLOCK_FORMATS = ["12h", "24h"]


def validate_address(value):
    """Validate sign address (2 characters)."""
    value = cv.string_strict(value)
    if len(value) != 2:
        raise cv.Invalid("Address must be exactly 2 characters")
    return value


# Offline message schema
OFFLINE_MESSAGE_SCHEMA = cv.Schema({
    cv.Required(CONF_TEXT): cv.string,
    cv.Optional(CONF_COLOR, default="green"): cv.one_of(*COLORS, lower=True),
    cv.Optional(CONF_MODE, default="rotate"): cv.one_of(*MODES, lower=True),
    cv.Optional(CONF_DURATION, default="10s"): cv.positive_time_period_milliseconds,
    cv.Optional(CONF_EFFECT): cv.one_of(*EFFECTS, lower=True),
    cv.Optional(CONF_CHARSET, default="7high"): cv.one_of(*CHARSETS, lower=True),
    cv.Optional(CONF_POSITION, default="topline"): cv.one_of(*POSITIONS, lower=True),
    cv.Optional(CONF_SPEED, default=3): cv.int_range(min=1, max=5),
})

# Defaults schema
DEFAULTS_SCHEMA = cv.Schema({
    cv.Optional(CONF_COLOR, default="green"): cv.one_of(*COLORS, lower=True),
    cv.Optional(CONF_MODE, default="rotate"): cv.one_of(*MODES, lower=True),
    cv.Optional(CONF_CHARSET, default="7high"): cv.one_of(*CHARSETS, lower=True),
    cv.Optional(CONF_POSITION, default="topline"): cv.one_of(*POSITIONS, lower=True),
    cv.Optional(CONF_SPEED, default=3): cv.int_range(min=1, max=5),
    cv.Optional(CONF_EFFECT, default="twinkle"): cv.one_of(*EFFECTS, lower=True),
})

# Clock schema
CLOCK_SCHEMA = cv.Schema({
    cv.Optional(CONF_ENABLED, default=True): cv.boolean,
    cv.Optional(CONF_INTERVAL, default="60s"): cv.positive_time_period_milliseconds,
    cv.Optional(CONF_DURATION, default="4s"): cv.positive_time_period_milliseconds,
    cv.Optional(CONF_FORMAT, default="12h"): cv.one_of(*CLOCK_FORMATS, lower=True),
    cv.Optional(CONF_COLOR, default="amber"): cv.one_of(*COLORS, lower=True),
})

# Priority schema
PRIORITY_SCHEMA = cv.Schema({
    cv.Optional(CONF_WARNING_DURATION, default="2500ms"): cv.positive_time_period_milliseconds,
    cv.Optional(CONF_DEFAULT_DURATION, default="25s"): cv.positive_time_period_milliseconds,
})

# Main component schema
CONFIG_SCHEMA = (
    cv.Schema({
        cv.GenerateID(): cv.declare_id(BetaBriteComponent),
        cv.Optional(CONF_SIGN_TYPE, default="all"): cv.one_of(*SIGN_TYPES, lower=True),
        cv.Optional(CONF_ADDRESS, default="00"): validate_address,
        cv.Optional(CONF_MAX_FILES, default=5): cv.int_range(min=1, max=26),
        cv.Optional(CONF_DEFAULTS): DEFAULTS_SCHEMA,
        cv.Optional(CONF_OFFLINE_MESSAGES): cv.ensure_list(OFFLINE_MESSAGE_SCHEMA),
        cv.Optional(CONF_CLOCK): CLOCK_SCHEMA,
        cv.Optional(CONF_PRIORITY): PRIORITY_SCHEMA,
    })
    .extend(cv.COMPONENT_SCHEMA)
    .extend(uart.UART_DEVICE_SCHEMA)
)


async def to_code(config):
    """Generate C++ code from YAML configuration."""
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await uart.register_uart_device(var, config)

    # Sign type
    sign_type = SIGN_TYPES[config[CONF_SIGN_TYPE]]
    cg.add(var.set_sign_type(getattr(betabrite_ns, sign_type)))

    # Address
    cg.add(var.set_address(config[CONF_ADDRESS]))

    # Max files
    cg.add(var.set_max_files(config[CONF_MAX_FILES]))

    # Default settings
    if CONF_DEFAULTS in config:
        defaults = config[CONF_DEFAULTS]
        cg.add(var.set_default_color(
            cg.RawExpression(f"esphome::betabrite::color_from_string(\"{defaults[CONF_COLOR]}\")")
        ))
        cg.add(var.set_default_mode(
            cg.RawExpression(f"esphome::betabrite::mode_from_string(\"{defaults[CONF_MODE]}\")")
        ))
        cg.add(var.set_default_charset(
            cg.RawExpression(f"esphome::betabrite::charset_from_string(\"{defaults[CONF_CHARSET]}\")")
        ))
        cg.add(var.set_default_position(
            cg.RawExpression(f"esphome::betabrite::position_from_string(\"{defaults[CONF_POSITION]}\")")
        ))
        cg.add(var.set_default_speed(defaults[CONF_SPEED]))
        cg.add(var.set_default_effect(
            cg.RawExpression(f"esphome::betabrite::effect_from_string(\"{defaults[CONF_EFFECT]}\")")
        ))

    # Clock settings
    if CONF_CLOCK in config:
        clock = config[CONF_CLOCK]
        cg.add(var.set_clock_enabled(clock[CONF_ENABLED]))
        cg.add(var.set_clock_interval(clock[CONF_INTERVAL]))
        cg.add(var.set_clock_duration(clock[CONF_DURATION]))
        cg.add(var.set_clock_24h(clock[CONF_FORMAT] == "24h"))
        cg.add(var.set_clock_color(
            cg.RawExpression(f"esphome::betabrite::color_from_string(\"{clock[CONF_COLOR]}\")")
        ))

    # Priority settings
    if CONF_PRIORITY in config:
        priority = config[CONF_PRIORITY]
        cg.add(var.set_priority_warning_duration(priority[CONF_WARNING_DURATION]))
        cg.add(var.set_priority_default_duration(priority[CONF_DEFAULT_DURATION]))

    # Offline messages
    if CONF_OFFLINE_MESSAGES in config:
        for msg in config[CONF_OFFLINE_MESSAGES]:
            effect = msg.get(CONF_EFFECT, "")
            cg.add(var.add_offline_message(
                msg[CONF_TEXT],
                msg[CONF_COLOR],
                msg[CONF_MODE],
                msg[CONF_DURATION],
                effect if effect else "",
                msg[CONF_CHARSET],
                msg[CONF_POSITION],
                msg[CONF_SPEED],
            ))


# ============================================================================
# Actions for automations
# ============================================================================

# Action: betabrite.display
DisplayMessageAction = betabrite_ns.class_("DisplayMessageAction", automation.Action)

DISPLAY_MESSAGE_ACTION_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.use_id(BetaBriteComponent),
    cv.Required(CONF_TEXT): cv.templatable(cv.string),
    cv.Optional(CONF_COLOR): cv.templatable(cv.one_of(*COLORS, lower=True)),
    cv.Optional(CONF_MODE): cv.templatable(cv.one_of(*MODES, lower=True)),
    cv.Optional(CONF_EFFECT): cv.templatable(cv.one_of(*EFFECTS, lower=True)),
})


@automation.register_action(
    "betabrite.display",
    DisplayMessageAction,
    DISPLAY_MESSAGE_ACTION_SCHEMA,
)
async def betabrite_display_action_to_code(config, action_id, template_arg, args):
    parent = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, parent)

    text_template = await cg.templatable(config[CONF_TEXT], args, cg.std_string)
    cg.add(var.set_message(text_template))

    if CONF_COLOR in config:
        color_template = await cg.templatable(config[CONF_COLOR], args, cg.std_string)
        cg.add(var.set_color(color_template))

    if CONF_MODE in config:
        mode_template = await cg.templatable(config[CONF_MODE], args, cg.std_string)
        cg.add(var.set_mode(mode_template))

    if CONF_EFFECT in config:
        effect_template = await cg.templatable(config[CONF_EFFECT], args, cg.std_string)
        cg.add(var.set_effect(effect_template))

    return var


# Action: betabrite.clear
ClearAction = betabrite_ns.class_("ClearAction", automation.Action)

CLEAR_ACTION_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.use_id(BetaBriteComponent),
})


@automation.register_action(
    "betabrite.clear",
    ClearAction,
    CLEAR_ACTION_SCHEMA,
)
async def betabrite_clear_action_to_code(config, action_id, template_arg, args):
    parent = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, parent)
    return var


# Action: betabrite.priority
PriorityMessageAction = betabrite_ns.class_("PriorityMessageAction", automation.Action)

PRIORITY_MESSAGE_ACTION_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.use_id(BetaBriteComponent),
    cv.Required(CONF_TEXT): cv.templatable(cv.string),
    cv.Optional(CONF_DURATION, default="25s"): cv.templatable(cv.positive_time_period_seconds),
})


@automation.register_action(
    "betabrite.priority",
    PriorityMessageAction,
    PRIORITY_MESSAGE_ACTION_SCHEMA,
)
async def betabrite_priority_action_to_code(config, action_id, template_arg, args):
    parent = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, parent)

    text_template = await cg.templatable(config[CONF_TEXT], args, cg.std_string)
    cg.add(var.set_message(text_template))

    duration_template = await cg.templatable(config[CONF_DURATION], args, cg.uint32)
    cg.add(var.set_duration(duration_template))

    return var


# Action: betabrite.demo
DemoAction = betabrite_ns.class_("DemoAction", automation.Action)

DEMO_ACTION_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.use_id(BetaBriteComponent),
})


@automation.register_action(
    "betabrite.demo",
    DemoAction,
    DEMO_ACTION_SCHEMA,
)
async def betabrite_demo_action_to_code(config, action_id, template_arg, args):
    parent = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, parent)
    return var


# Action: betabrite.cancel_priority
CancelPriorityAction = betabrite_ns.class_("CancelPriorityAction", automation.Action)

CANCEL_PRIORITY_ACTION_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.use_id(BetaBriteComponent),
})


@automation.register_action(
    "betabrite.cancel_priority",
    CancelPriorityAction,
    CANCEL_PRIORITY_ACTION_SCHEMA,
)
async def betabrite_cancel_priority_action_to_code(config, action_id, template_arg, args):
    parent = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, parent)
    return var


# Action: betabrite.clock
DisplayClockAction = betabrite_ns.class_("DisplayClockAction", automation.Action)

DISPLAY_CLOCK_ACTION_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.use_id(BetaBriteComponent),
})


@automation.register_action(
    "betabrite.clock",
    DisplayClockAction,
    DISPLAY_CLOCK_ACTION_SCHEMA,
)
async def betabrite_clock_action_to_code(config, action_id, template_arg, args):
    parent = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, parent)
    return var
