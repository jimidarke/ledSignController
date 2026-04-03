#!/usr/bin/env python3
"""
BetaBrite Serial Protocol Sniffer

Passively monitors serial traffic between an ESP32 and BetaBrite sign.
Connect an FTDI adapter's RX pin to the line you want to monitor (TX only,
do NOT connect FTDI TX).

Wiring for single-channel (ESP32 -> Sign):
  - Tap BEFORE MAX3232 (TTL level): connect FTDI RX to GPIO17 (ESP32 TX2)
  - Tap AFTER MAX3232 (RS232 level): connect RS232 FTDI RX to the RS232 TX line
  - Connect GND

Wiring for dual-channel (bidirectional):
  - FTDI-1 RX -> ESP32 TX line (outgoing commands)
  - FTDI-2 RX -> Sign TX line (incoming responses)
  - Both GND connected

Requires: pyserial (pip install pyserial)

Usage:
    # Single channel - monitor ESP32 -> Sign
    python3 betabrite_sniffer.py --port /dev/ttyUSB1

    # Dual channel - monitor both directions
    python3 betabrite_sniffer.py --tx-port /dev/ttyUSB1 --rx-port /dev/ttyUSB2

    # Log to file
    python3 betabrite_sniffer.py --port /dev/ttyUSB1 --output capture.log
"""

import argparse
import sys
import time
import threading
from datetime import datetime

try:
    import serial
except ImportError:
    print("ERROR: pyserial not installed. Run: pip install pyserial")
    sys.exit(1)


# Alpha Protocol constants
NUL = 0x00
SOH = 0x01
STX = 0x02
ETX = 0x03
EOT = 0x04
ESC = 0x1B

# Command code names
CMD_NAMES = {
    ord('A'): "WriteText",
    ord('B'): "ReadText",
    ord('E'): "WriteSpecFunc",
    ord('F'): "ReadSpecFunc",
    ord('G'): "WriteString",
    ord('H'): "ReadString",
    ord('I'): "WriteSmallDots",
    ord('J'): "ReadSmallDots",
    ord('K'): "WriteRGBDots",
    ord('L'): "ReadRGBDots",
    ord('M'): "WriteLargeDots",
    ord('N'): "ReadLargeDots",
    ord('O'): "WriteBulletin",
    ord('T'): "SetTimeout",
}

# Display mode names
MODE_NAMES = {
    ord('a'): "Rotate", ord('b'): "Hold", ord('c'): "Flash",
    ord('e'): "RollUp", ord('f'): "RollDown", ord('g'): "RollLeft",
    ord('h'): "RollRight", ord('i'): "WipeUp", ord('j'): "WipeDown",
    ord('k'): "WipeLeft", ord('l'): "WipeRight", ord('m'): "Scroll",
    ord('n'): "Special", ord('o'): "Auto", ord('p'): "RollIn",
    ord('q'): "RollOut", ord('r'): "WipeIn", ord('s'): "WipeOut",
    ord('t'): "CompRotate", ord('u'): "Explode", ord('v'): "Clock",
}

# Color names
COLOR_NAMES = {
    ord('1'): "Red", ord('2'): "Green", ord('3'): "Amber",
    ord('4'): "DimRed", ord('5'): "DimGreen", ord('6'): "Brown",
    ord('7'): "Orange", ord('8'): "Yellow", ord('9'): "Rainbow1",
    ord('A'): "Rainbow2", ord('B'): "ColorMix", ord('C'): "Auto",
}

# Special effect names
SPECIAL_NAMES = {
    ord('0'): "Twinkle", ord('1'): "Sparkle", ord('2'): "Snow",
    ord('3'): "Interlock", ord('4'): "Switch", ord('5'): "Slide",
    ord('6'): "Spray", ord('7'): "Starburst", ord('8'): "Welcome",
    ord('9'): "Slots", ord('A'): "Newsflash", ord('B'): "Trumpet",
    ord('C'): "CycleColors", ord('S'): "ThankYou", ord('U'): "NoSmoking",
    ord('V'): "DontDrinkDrive", ord('W'): "Fishimal", ord('X'): "Fireworks",
    ord('Y'): "TurBalloon", ord('Z'): "Bomb",
}

# Sign type names
SIGN_TYPE_NAMES = {
    0x3F: "ALL", 0x5E: "BetaBrite", 0x31: "1Line", 0x32: "2Line",
    0x30: "Response", 0x5A: "ALL2", 0x7A: "AllAZ",
}

# Format codes
FC_SELECTCHARCOLOR = 0x1C
FC_SELECTCHARSET = 0x1A

# Charset names
CHARSET_NAMES = {
    ord('1'): "5high", ord('2'): "5stroke", ord('3'): "7high",
    ord('4'): "7stroke", ord('5'): "7fancy", ord('6'): "10high",
    ord('7'): "7shadow", ord('8'): "FullFancy", ord('9'): "FullHigh",
}

# Output lock for thread safety
output_lock = threading.Lock()
log_file = None


def output(text):
    """Thread-safe output to console and optional log file."""
    with output_lock:
        print(text)
        if log_file:
            log_file.write(text + "\n")
            log_file.flush()


def hex_dump(data):
    """Format bytes as hex string."""
    return " ".join(f"{b:02X}" for b in data)


def ascii_safe(data):
    """Format bytes as safe ASCII."""
    return "".join(chr(b) if 0x20 <= b < 0x7F else "." for b in data)


def decode_write_text_payload(payload):
    """Decode the payload of a WriteText command."""
    parts = []
    if len(payload) < 1:
        return "empty"

    file_label = chr(payload[0])
    parts.append(f"File={file_label}")

    idx = 1
    # Look for ESC + position + mode
    if idx < len(payload) and payload[idx] == ESC:
        idx += 1
        if idx < len(payload):
            pos = payload[idx]
            pos_names = {0x20: "Mid", 0x22: "Top", 0x26: "Bot", 0x30: "Fill", 0x31: "Left", 0x32: "Right"}
            parts.append(f"Pos={pos_names.get(pos, f'0x{pos:02X}')}")
            idx += 1
        if idx < len(payload):
            mode = payload[idx]
            mode_name = MODE_NAMES.get(mode, f"0x{mode:02X}")
            parts.append(f"Mode={mode_name}")
            idx += 1

            # If special mode, next byte is the effect
            if mode == ord('n') and idx < len(payload):
                effect = payload[idx]
                parts.append(f"Effect={SPECIAL_NAMES.get(effect, f'0x{effect:02X}')}")
                idx += 1

    # Look for color and charset codes in remaining payload
    text_start = idx
    while idx < len(payload):
        if payload[idx] == FC_SELECTCHARCOLOR and idx + 1 < len(payload):
            color = payload[idx + 1]
            parts.append(f"Color={COLOR_NAMES.get(color, f'0x{color:02X}')}")
            idx += 2
            text_start = idx
        elif payload[idx] == FC_SELECTCHARSET and idx + 1 < len(payload):
            cs = payload[idx + 1]
            parts.append(f"Charset={CHARSET_NAMES.get(cs, f'0x{cs:02X}')}")
            idx += 2
            text_start = idx
        else:
            break

    # Remaining is text content
    text = payload[text_start:]
    if text:
        text_str = "".join(chr(b) if 0x20 <= b < 0x7F else f"\\x{b:02X}" for b in text)
        parts.append(f'Text="{text_str}"')

    return ", ".join(parts)


def decode_frame(data, direction=""):
    """Decode and pretty-print an Alpha Protocol frame."""
    ts = datetime.now().strftime("%H:%M:%S.%f")[:-3]
    prefix = f"[{ts}] {direction}" if direction else f"[{ts}]"

    # Raw hex
    lines = [f"{prefix} Raw: {hex_dump(data)}  |{ascii_safe(data)}|"]

    # Find SOH
    soh_idx = None
    for i, b in enumerate(data):
        if b == SOH:
            soh_idx = i
            break

    if soh_idx is None:
        lines.append(f"{prefix} (No SOH found - not a valid frame)")
        return "\n".join(lines)

    idx = soh_idx + 1

    # Type
    sign_type = data[idx] if idx < len(data) else None
    type_name = SIGN_TYPE_NAMES.get(sign_type, f"0x{sign_type:02X}") if sign_type else "?"
    idx += 1

    # Address
    addr = ""
    if idx + 1 < len(data):
        addr = chr(data[idx]) + chr(data[idx + 1])
        idx += 2

    # STX
    if idx < len(data) and data[idx] == STX:
        idx += 1

    # Command
    cmd = data[idx] if idx < len(data) else None
    cmd_name = CMD_NAMES.get(cmd, f"0x{cmd:02X}") if cmd else "?"
    idx += 1

    # Payload (until ETX/EOT)
    payload = bytearray()
    while idx < len(data) and data[idx] not in (ETX, EOT):
        payload.append(data[idx])
        idx += 1

    summary = f"{prefix} {type_name}:{addr} {cmd_name}"

    # Decode specific commands
    if cmd == ord('A'):  # WriteText
        detail = decode_write_text_payload(payload)
        summary += f" [{detail}]"
    elif cmd == ord('E'):  # WriteSpecFunc
        if payload:
            summary += f" Label='{chr(payload[0])}'"
    elif cmd == ord('G'):  # WriteString
        if payload:
            summary += f" File={chr(payload[0])}"
            if len(payload) > 1:
                text = payload[1:].decode("ascii", errors="replace")
                summary += f' Content="{text}"'

    lines.append(summary)
    return "\n".join(lines)


def sniff_port(ser, direction_label, stop_event):
    """Read from a serial port and decode frames."""
    buffer = bytearray()
    idle_start = None

    while not stop_event.is_set():
        try:
            data = ser.read(256)
        except serial.SerialException:
            output(f"[{direction_label}] Serial error - port disconnected?")
            break

        if data:
            buffer.extend(data)
            idle_start = time.time()

            # Check for complete frame (ends with EOT)
            while EOT in buffer:
                eot_idx = buffer.index(EOT)
                frame = bytes(buffer[: eot_idx + 1])
                buffer = buffer[eot_idx + 1 :]
                output(decode_frame(frame, direction_label))
                output("")  # Blank line between frames
        else:
            # Flush partial buffer after idle
            if buffer and idle_start and (time.time() - idle_start > 0.3):
                output(decode_frame(bytes(buffer), f"{direction_label}(partial)"))
                output("")
                buffer.clear()
                idle_start = None


def main():
    global log_file

    parser = argparse.ArgumentParser(
        description="BetaBrite Serial Protocol Sniffer",
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )

    # Single or dual channel
    parser.add_argument("--port", "-p", help="Serial port for single-channel sniffing")
    parser.add_argument("--tx-port", help="TX direction port (ESP32 -> Sign) for dual-channel")
    parser.add_argument("--rx-port", help="RX direction port (Sign -> ESP32) for dual-channel")
    parser.add_argument("--output", "-o", help="Log output to file")
    parser.add_argument("--raw", action="store_true", help="Show raw hex only (no decode)")

    args = parser.parse_args()

    if not args.port and not (args.tx_port or args.rx_port):
        parser.error("Specify --port for single-channel or --tx-port/--rx-port for dual-channel")

    if args.output:
        log_file = open(args.output, "w")
        print(f"Logging to {args.output}")

    print("=" * 60)
    print("  BetaBrite Serial Protocol Sniffer")
    print("  Protocol: Alpha Sign Communications (9600 7E1)")
    print("  Press Ctrl+C to stop")
    print("=" * 60)

    stop_event = threading.Event()
    threads = []

    try:
        if args.port:
            # Single channel
            ser = serial.Serial(
                port=args.port, baudrate=9600,
                bytesize=serial.SEVENBITS, parity=serial.PARITY_EVEN,
                stopbits=serial.STOPBITS_ONE, timeout=0.1,
            )
            print(f"Monitoring {args.port} (single-channel)")
            print()
            t = threading.Thread(target=sniff_port, args=(ser, "SNIFF", stop_event), daemon=True)
            t.start()
            threads.append(t)
        else:
            # Dual channel
            if args.tx_port:
                ser_tx = serial.Serial(
                    port=args.tx_port, baudrate=9600,
                    bytesize=serial.SEVENBITS, parity=serial.PARITY_EVEN,
                    stopbits=serial.STOPBITS_ONE, timeout=0.1,
                )
                print(f"Monitoring TX (ESP32->Sign): {args.tx_port}")
                t = threading.Thread(target=sniff_port, args=(ser_tx, "ESP32->SIGN", stop_event), daemon=True)
                t.start()
                threads.append(t)

            if args.rx_port:
                ser_rx = serial.Serial(
                    port=args.rx_port, baudrate=9600,
                    bytesize=serial.SEVENBITS, parity=serial.PARITY_EVEN,
                    stopbits=serial.STOPBITS_ONE, timeout=0.1,
                )
                print(f"Monitoring RX (Sign->ESP32): {args.rx_port}")
                t = threading.Thread(target=sniff_port, args=(ser_rx, "SIGN->ESP32", stop_event), daemon=True)
                t.start()
                threads.append(t)

            print()

        # Wait for Ctrl+C
        while True:
            time.sleep(0.5)

    except serial.SerialException as e:
        print(f"ERROR: {e}")
    except KeyboardInterrupt:
        print("\nStopping sniffer...")
    finally:
        stop_event.set()
        for t in threads:
            t.join(timeout=1)
        if log_file:
            log_file.close()
        print("Done.")


if __name__ == "__main__":
    main()
