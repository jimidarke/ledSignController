#!/usr/bin/env python3
"""
BetaBrite Sign Demo - cycles through colors, modes, effects, and charsets.
Usage: python3 tools/sign_demo.py --port /dev/ttyUSB0
"""

import argparse
import time
import sys

try:
    import serial
except ImportError:
    print("ERROR: pip install pyserial")
    sys.exit(1)

# Protocol constants
NUL = 0x00
SOH = 0x01
STX = 0x02
EOT = 0x04
ESC = 0x1B
FC_SELECTCHARCOLOR = 0x1C
FC_SELECTCHARSET = 0x1A

# Sign addressing
SIGN_TYPE_ALL = 0x3F
ADDRESS = b"00"

# Display modes
MODES = {
    "HOLD":      ord('b'),
    "FLASH":     ord('c'),
    "ROLL UP":   ord('e'),
    "ROLL DOWN": ord('f'),
    "ROLL LEFT": ord('g'),
    "WIPE UP":   ord('i'),
    "WIPE DOWN": ord('j'),
    "WIPE IN":   ord('r'),
    "WIPE OUT":  ord('s'),
    "ROTATE":    ord('a'),
    "EXPLODE":   ord('u'),
    "SCROLL":    ord('m'),
}

# Special display modes (used with mode 'n')
SPECIALS = {
    "TWINKLE":    ord('0'),
    "SPARKLE":    ord('1'),
    "SNOW":       ord('2'),
    "INTERLOCK":  ord('3'),
    "SWITCH":     ord('4'),
    "SPRAY":      ord('6'),
    "STARBURST":  ord('7'),
    "WELCOME":    ord('8'),
    "SLOTS":      ord('9'),
    "NEWSFLASH":  ord('A'),
    "TRUMPET":    ord('B'),
    "CYCLE COLS": ord('C'),
    "THANK YOU":  ord('S'),
    "FIREWORKS":  ord('X'),
    "TURBALLOON": ord('Y'),
    "BOMB":       ord('Z'),
}

# Colors
COLORS = {
    "RED":       ord('1'),
    "GREEN":     ord('2'),
    "AMBER":     ord('3'),
    "DIM RED":   ord('4'),
    "DIM GREEN": ord('5'),
    "BROWN":     ord('6'),
    "ORANGE":    ord('7'),
    "YELLOW":    ord('8'),
    "RAINBOW":   ord('9'),
    "RAINBOW2":  ord('A'),
    "COLOR MIX": ord('B'),
}

# Character sets
CHARSETS = {
    "5high":       ord('1'),
    "7high":       ord('3'),
    "7high fancy": ord('5'),
    "10high":      ord('6'),
    "7shadow":     ord('7'),
    "full fancy":  ord('8'),
    "full high":   ord('9'),
}

POSITION_TOP = 0x22
DM_SPECIAL = ord('n')


def build_frame(command_code, payload):
    frame = bytes([NUL] * 5)
    frame += bytes([SOH, SIGN_TYPE_ALL])
    frame += ADDRESS
    frame += bytes([STX, command_code])
    frame += payload
    frame += bytes([EOT])
    return frame


def write_text(ser, message, color=ord('2'), position=POSITION_TOP,
               mode=ord('b'), special=None, charset=None):
    payload = bytes([ord('A')])  # File label A
    payload += bytes([ESC, position, mode])
    if mode == DM_SPECIAL and special is not None:
        payload += bytes([special])
    if charset is not None:
        payload += bytes([FC_SELECTCHARSET, charset])
    if color is not None:
        payload += bytes([FC_SELECTCHARCOLOR, color])
    payload += message.encode('ascii')

    frame = build_frame(ord('A'), payload)
    ser.write(frame)
    ser.flush()


def demo_colors(ser):
    print("\n--- COLOR DEMO ---")
    for name, code in COLORS.items():
        print(f"  {name}")
        write_text(ser, name, color=code, mode=ord('b'))
        time.sleep(2.5)


def demo_modes(ser):
    print("\n--- MODE DEMO ---")
    for name, code in MODES.items():
        print(f"  {name}")
        write_text(ser, name, color=ord('2'), mode=code)
        time.sleep(4)


def demo_specials(ser):
    print("\n--- SPECIAL EFFECTS DEMO ---")
    for name, code in SPECIALS.items():
        print(f"  {name}")
        write_text(ser, name, color=ord('9'), mode=DM_SPECIAL, special=code)
        time.sleep(5)


def demo_charsets(ser):
    print("\n--- CHARSET / FONT DEMO ---")
    for name, code in CHARSETS.items():
        print(f"  {name}")
        write_text(ser, name, color=ord('3'), mode=ord('b'), charset=code)
        time.sleep(3)


def demo_multicolor(ser):
    print("\n--- MULTI-COLOR MESSAGE ---")
    # Build a message with inline color changes
    payload = bytes([ord('A')])  # File A
    payload += bytes([ESC, POSITION_TOP, ord('b')])  # Hold mode
    payload += bytes([FC_SELECTCHARSET, ord('6')])  # 10high
    payload += bytes([FC_SELECTCHARCOLOR, ord('1')]) + b"R"
    payload += bytes([FC_SELECTCHARCOLOR, ord('3')]) + b"A"
    payload += bytes([FC_SELECTCHARCOLOR, ord('2')]) + b"I"
    payload += bytes([FC_SELECTCHARCOLOR, ord('8')]) + b"N"
    payload += bytes([FC_SELECTCHARCOLOR, ord('7')]) + b"B"
    payload += bytes([FC_SELECTCHARCOLOR, ord('5')]) + b"O"
    payload += bytes([FC_SELECTCHARCOLOR, ord('1')]) + b"W"
    frame = build_frame(ord('A'), payload)
    ser.write(frame)
    ser.flush()
    time.sleep(4)


def demo_priority(ser):
    print("\n--- PRIORITY MESSAGE (overwrites normal) ---")
    # Priority file is label '0'
    payload = bytes([ord('0')])  # Priority file
    payload += bytes([ESC, POSITION_TOP, ord('c')])  # Flash mode
    payload += bytes([FC_SELECTCHARSET, ord('6')])  # 10high (big)
    payload += bytes([FC_SELECTCHARCOLOR, ord('1')])  # Red
    payload += b"!! ALERT !!"
    frame = build_frame(ord('A'), payload)
    ser.write(frame)
    ser.flush()
    time.sleep(5)

    # Cancel priority
    print("  Cancelling priority...")
    payload = bytes([ord('0')])  # Priority file
    frame = build_frame(ord('A'), payload)
    ser.write(frame)
    ser.flush()


def main():
    parser = argparse.ArgumentParser(description="BetaBrite Sign Feature Demo")
    parser.add_argument("--port", "-p", default="/dev/ttyUSB0")
    parser.add_argument("--demo", choices=["all", "colors", "modes", "specials", "charsets", "multicolor", "priority"],
                        default="all", help="Which demo to run (default: all)")
    args = parser.parse_args()

    ser = serial.Serial(
        port=args.port, baudrate=9600,
        bytesize=serial.SEVENBITS, parity=serial.PARITY_EVEN,
        stopbits=serial.STOPBITS_ONE, timeout=2.0
    )
    print(f"Opened {args.port} at 9600 7E1")

    demos = {
        "colors": demo_colors,
        "modes": demo_modes,
        "specials": demo_specials,
        "charsets": demo_charsets,
        "multicolor": demo_multicolor,
        "priority": demo_priority,
    }

    try:
        if args.demo == "all":
            for name, func in demos.items():
                func(ser)
                time.sleep(1)
            # Finish with something nice
            print("\n--- DONE ---")
            write_text(ser, "SIGN TEST COMPLETE", color=ord('2'),
                       mode=DM_SPECIAL, special=ord('8'), charset=ord('6'))
        else:
            demos[args.demo](ser)
    except KeyboardInterrupt:
        print("\nStopped.")
    finally:
        ser.close()
        print(f"Port closed.")


if __name__ == "__main__":
    main()
