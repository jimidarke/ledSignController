#!/usr/bin/env python3
"""
Racing Countdown Animation for BetaBrite 213C LED Sign.

Modes:
  full   - Flash entire sign RED -> YELLOW -> GREEN (default)
  thirds - Portrait mode with color bands (sign rotated 90deg CW)

Usage:
    python3 tools/racing_countdown.py --port /dev/ttyUSB0
    python3 tools/racing_countdown.py --port /dev/ttyUSB0 --mode thirds -c 2
    python3 tools/racing_countdown.py --port /dev/ttyUSB0 --test-chars
    python3 tools/racing_countdown.py --port /dev/ttyUSB0 --loop
"""

import argparse
import time
import sys

try:
    import serial
except ImportError:
    print("ERROR: pip install pyserial")
    sys.exit(1)

# Protocol constants (Alpha Sign Communications Protocol)
NUL = 0x00
SOH = 0x01
STX = 0x02
EOT = 0x04
ESC = 0x1B

# Sign addressing
SIGN_TYPE_ALL = 0x3F
ADDRESS = b"00"

# Display modes
DM_HOLD = ord('b')

# Display positions
DP_TOPLINE = 0x22

# Colors
COL_RED = ord('1')
COL_GREEN = ord('2')
COL_YELLOW = ord('8')

# Formatting codes
FC_SELECTCHARSET = 0x1A
FC_SELECTCHARCOLOR = 0x1C

# Character sets
CS_FULL_HIGH = ord('9')

# Verbose logging
verbose = False


def log_hex(direction, data, max_bytes=60):
    """Log bytes as hex dump."""
    if len(data) <= max_bytes:
        hex_str = " ".join(f"{b:02X}" for b in data)
        print(f"  {direction}: {hex_str}")
    else:
        hex_str = " ".join(f"{b:02X}" for b in data[:max_bytes])
        print(f"  {direction}: {hex_str} ... ({len(data)}b)")


def build_frame(command_code, payload):
    """Build a complete Alpha Protocol frame."""
    frame = bytes([NUL] * 5)
    frame += bytes([SOH, SIGN_TYPE_ALL])
    frame += ADDRESS
    frame += bytes([STX, command_code])
    frame += payload
    frame += bytes([EOT])
    return frame


def send_frame(ser, frame):
    """Send a frame to the sign."""
    if verbose:
        log_hex("TX", frame)
    ser.write(frame)
    ser.flush()


def configure_memory(ser):
    """Configure sign memory with a single text file."""
    payload = b"$"
    payload += b"A"      # File A
    payload += b"A"      # Type: Text
    payload += b"L"      # Locked
    payload += b"0100"   # Size: 256 bytes
    payload += b"FF00"   # Always on
    frame = build_frame(ord('E'), payload)
    send_frame(ser, frame)
    time.sleep(0.5)


def write_text(ser, text_payload):
    """Write a text file A with the given pre-built payload content."""
    payload = bytes([ord('A')])  # File A
    payload += bytes([ESC, DP_TOPLINE, DM_HOLD])
    payload += text_payload
    frame = build_frame(ord('A'), payload)
    send_frame(ser, frame)


def clear_display(ser):
    """Clear the display (go black)."""
    write_text(ser, b" ")


def build_solid_fill(color, fill_char='#', count=10, charset=CS_FULL_HIGH):
    """Build payload for a solid color fill of the entire sign."""
    payload = bytes([FC_SELECTCHARSET, charset])
    payload += bytes([FC_SELECTCHARCOLOR, color])
    payload += (fill_char * count).encode('ascii')
    return payload


def build_color_blocks(sections, fill_char='#', charset=CS_FULL_HIGH,
                       total_chars=6):
    """Build payload with colored sections, padded to constant width."""
    payload = bytes([FC_SELECTCHARSET, charset])
    chars_used = 0
    for color, count in sections:
        payload += bytes([FC_SELECTCHARCOLOR, color])
        payload += (fill_char * count).encode('ascii')
        chars_used += count
    remaining = total_chars - chars_used
    if remaining > 0:
        payload += b" " * remaining
    return payload


def run_full_countdown(ser, delay=1.0, fill_char='#', fill_count=10):
    """Full-sign color flash: RED -> YELLOW -> GREEN."""
    print("Racing countdown (full-sign mode)...")
    print(f"  fill='{fill_char}' x{fill_count}, delay={delay}s")

    print("  [BLACK]")
    clear_display(ser)
    time.sleep(delay)

    print("  [RED]")
    write_text(ser, build_solid_fill(COL_RED, fill_char, fill_count))
    time.sleep(delay)

    print("  [YELLOW]")
    write_text(ser, build_solid_fill(COL_YELLOW, fill_char, fill_count))
    time.sleep(delay)

    print("  [GREEN]")
    write_text(ser, build_solid_fill(COL_GREEN, fill_char, fill_count))

    time.sleep(3)
    print("  [BLACK]")
    clear_display(ser)
    print("  Done.")


def run_thirds_countdown(ser, chars_per_section=2, delay=1.0, fill_char='#'):
    """Portrait mode: red top -> yellow middle -> green bottom."""
    total = chars_per_section * 3
    print("Racing countdown (thirds mode)...")
    print(f"  {chars_per_section} chars/section ({total} total), "
          f"fill='{fill_char}', delay={delay}s")

    print("  [BLACK]")
    clear_display(ser)
    time.sleep(delay)

    print("  [RED]")
    write_text(ser, build_color_blocks(
        [(COL_RED, chars_per_section)],
        fill_char, total_chars=total))
    time.sleep(delay)

    print("  [RED + YELLOW]")
    write_text(ser, build_color_blocks(
        [(COL_RED, chars_per_section), (COL_YELLOW, chars_per_section)],
        fill_char, total_chars=total))
    time.sleep(delay)

    print("  [RED + YELLOW + GREEN]")
    write_text(ser, build_color_blocks(
        [(COL_RED, chars_per_section), (COL_YELLOW, chars_per_section),
         (COL_GREEN, chars_per_section)],
        fill_char, total_chars=total))

    time.sleep(3)
    print("  [BLACK]")
    clear_display(ser)
    print("  Done.")


def run_test_chars(ser):
    """Cycle through candidate fill characters to find the densest one."""
    candidates = ['#', 'M', 'W', '@', '8', '0', '%', '&', 'X', 'H', 'N',
                  'B', 'Q', 'D', 'E']
    print("=== FILL CHARACTER TEST ===")
    print("Each character shown in GREEN, full_high font, 6 wide.")
    print("Watch for which fills the most pixels. Press Ctrl+C to stop.\n")

    for ch in candidates:
        print(f"  '{ch}'")
        write_text(ser, build_solid_fill(COL_GREEN, ch, 6))
        try:
            time.sleep(3)
        except KeyboardInterrupt:
            print(f"\n  Stopped at '{ch}'")
            clear_display(ser)
            return

    print("\nDone. Use --fill-char <char> with your best pick.")
    clear_display(ser)


def open_port(port_path, timeout=2.0):
    """Open serial port with Alpha Protocol settings (9600 7E1)."""
    try:
        ser = serial.Serial(
            port=port_path,
            baudrate=9600,
            bytesize=serial.SEVENBITS,
            parity=serial.PARITY_EVEN,
            stopbits=serial.STOPBITS_ONE,
            timeout=timeout,
            dsrdtr=False,
            rtscts=False,
        )
        ser.dtr = False
        ser.rts = False
        print(f"Opened {port_path} at 9600 7E1")
        return ser
    except serial.SerialException as e:
        print(f"ERROR: Cannot open {port_path}: {e}")
        print("  Check: ls -la /dev/ttyUSB*")
        print("  Permissions: sudo usermod -aG dialout $USER")
        sys.exit(1)


def main():
    global verbose

    parser = argparse.ArgumentParser(
        description="Racing Countdown for BetaBrite 213C LED Sign")
    parser.add_argument("--port", "-p", default="/dev/ttyUSB0")
    parser.add_argument("--mode", choices=["full", "thirds"], default="full",
                        help="full = whole sign flashes each color; "
                             "thirds = portrait bands (default: full)")
    parser.add_argument("--delay", "-d", type=float, default=1.0,
                        help="Seconds between stages (default: 1.0)")
    parser.add_argument("--fill-char", default="#",
                        help="Fill character (default: #)")
    parser.add_argument("--fill-count", type=int, default=10,
                        help="Chars to send in full mode (default: 10, overflow clipped)")
    parser.add_argument("--chars", "-c", type=int, default=2,
                        help="Chars per section in thirds mode (default: 2)")
    parser.add_argument("--loop", "-l", action="store_true",
                        help="Loop the animation")
    parser.add_argument("--loop-delay", type=float, default=3.0,
                        help="Seconds between loops (default: 3.0)")
    parser.add_argument("--test-chars", action="store_true",
                        help="Cycle through fill characters to find densest")
    parser.add_argument("--verbose", "-v", action="store_true")
    args = parser.parse_args()

    verbose = args.verbose
    ser = open_port(args.port)

    try:
        print("Configuring sign memory...")
        configure_memory(ser)

        if args.test_chars:
            run_test_chars(ser)
        elif args.loop:
            print(f"Looping {args.mode} countdown (Ctrl+C to stop)...")
            while True:
                if args.mode == "full":
                    run_full_countdown(ser, args.delay, args.fill_char,
                                       args.fill_count)
                else:
                    run_thirds_countdown(ser, args.chars, args.delay,
                                         args.fill_char)
                time.sleep(args.loop_delay)
        else:
            if args.mode == "full":
                run_full_countdown(ser, args.delay, args.fill_char,
                                   args.fill_count)
            else:
                run_thirds_countdown(ser, args.chars, args.delay,
                                     args.fill_char)
    except KeyboardInterrupt:
        print("\nStopped.")
        clear_display(ser)
    finally:
        ser.close()
        print("Port closed.")


if __name__ == "__main__":
    main()
