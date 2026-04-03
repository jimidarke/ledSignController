#!/usr/bin/env python3
"""
Racing Countdown Animation for BetaBrite Sign in Portrait Mode.

Simulates a racing start-light sequence: RED -> YELLOW -> GREEN,
designed for the sign rotated 90 degrees clockwise (portrait orientation).

The left side of the sign becomes the top in portrait mode, so the first
characters in the text string appear at the top of the display.

Usage:
    python3 tools/racing_countdown.py --port /dev/ttyUSB0
    python3 tools/racing_countdown.py --port /dev/ttyUSB0 --test-fill
    python3 tools/racing_countdown.py --port /dev/ttyUSB0 -c 3 --loop
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
DM_FLASH = ord('c')

# Display positions
DP_FILL = 0x30
DP_TOPLINE = 0x22
DP_LEFT = 0x31

# Colors
COL_RED = ord('1')
COL_GREEN = ord('2')
COL_YELLOW = ord('8')

# Formatting codes
FC_SELECTCHARSET = 0x1A
FC_SELECTCHARCOLOR = 0x1C

# Character sets
CS_FULL_HIGH = ord('9')
CS_10HIGH = ord('6')
CS_7HIGH = ord('3')

# Verbose logging
verbose = False


def log_hex(direction, data):
    """Log bytes as hex dump."""
    hex_str = " ".join(f"{b:02X}" for b in data)
    ascii_str = "".join(chr(b) if 0x20 <= b < 0x7F else "." for b in data)
    print(f"  {direction}: {hex_str}  |{ascii_str}|")


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
    """Configure sign memory with a single text file.

    Some BetaBrite signs need memory configured before repeated writes,
    otherwise rapid writes can cause the sign to reset.
    """
    # Write Special Function: Clear memory and allocate file A
    # Format: E $ [file_label] [type] [lock] [size_hex] [time_hex]
    payload = b"$"  # Clear memory label
    payload += b"A"  # File A
    payload += b"A"  # Type: Text
    payload += b"L"  # Locked
    payload += b"0100"  # Size: 256 bytes
    payload += b"FF00"  # Always on
    frame = build_frame(ord('E'), payload)
    send_frame(ser, frame)
    time.sleep(0.5)  # Give sign time to reconfigure memory


def clear_display(ser):
    """Clear the display (go black)."""
    # Write an empty/space message to file A with HOLD mode
    payload = bytes([ord('A')])  # File A
    payload += bytes([ESC, DP_LEFT, DM_HOLD])
    payload += b" "
    frame = build_frame(ord('A'), payload)
    send_frame(ser, frame)


def write_color_blocks(ser, sections, fill_char='#', charset=CS_FULL_HIGH,
                       total_chars=6):
    """Write colored block sections to the display.

    Pads with spaces to total_chars so the sign doesn't re-center
    as new sections are added.

    Args:
        ser: Serial port
        sections: List of (color_code, char_count) tuples
        fill_char: ASCII character to repeat for fill blocks
        charset: Character set code
        total_chars: Total display width in chars (pad to this)
    """
    payload = bytes([ord('A')])  # File A
    payload += bytes([ESC, DP_LEFT, DM_HOLD])
    payload += bytes([FC_SELECTCHARSET, charset])

    chars_used = 0
    for color_code, count in sections:
        payload += bytes([FC_SELECTCHARCOLOR, color_code])
        payload += (fill_char * count).encode('ascii')
        chars_used += count

    # Pad remaining space so text width is constant (prevents re-centering)
    remaining = total_chars - chars_used
    if remaining > 0:
        payload += b" " * remaining

    frame = build_frame(ord('A'), payload)
    send_frame(ser, frame)


def run_countdown(ser, chars_per_section=3, fill_char='#', delay=1.0,
                  charset=CS_FULL_HIGH):
    """Run the racing countdown animation.

    Sequence: black -> red top -> +yellow middle -> +green bottom (hold)
    """
    total = chars_per_section * 3
    print("Starting racing countdown...")
    print(f"  {chars_per_section} chars/section ({total} total), fill='{fill_char}', "
          f"delay={delay}s, charset=0x{charset:02X}")

    # Frame 0: Clear (black)
    print("  [BLACK]")
    clear_display(ser)
    time.sleep(delay)

    # Frame 1: Red top third (padded to full width)
    print("  [RED]")
    write_color_blocks(ser, [(COL_RED, chars_per_section)],
                       fill_char, charset, total_chars=total)
    time.sleep(delay)

    # Frame 2: Red + Yellow (padded to full width)
    print("  [RED + YELLOW]")
    write_color_blocks(ser, [
        (COL_RED, chars_per_section),
        (COL_YELLOW, chars_per_section),
    ], fill_char, charset, total_chars=total)
    time.sleep(delay)

    # Frame 3: Red + Yellow + Green (full width, no padding needed)
    print("  [RED + YELLOW + GREEN]")
    write_color_blocks(ser, [
        (COL_RED, chars_per_section),
        (COL_YELLOW, chars_per_section),
        (COL_GREEN, chars_per_section),
    ], fill_char, charset, total_chars=total)

    # Hold the full countdown for a few seconds, then go black
    time.sleep(3)
    print("  [BLACK]")
    clear_display(ser)
    print("  Done.")


def run_test_fill(ser, fill_char='#', charset=CS_FULL_HIGH):
    """Calibration mode: send increasing char counts to find display width.

    Watch the sign and note when characters stop appearing (overflow).
    That tells you the total character capacity, divide by 3 for --chars.
    """
    print("=== TEST FILL MODE ===")
    print(f"Fill char: '{fill_char}', charset: 0x{charset:02X}")
    print("Watch the sign. Press Ctrl+C when done.\n")

    for count in range(1, 21):
        print(f"  Sending {count} green chars: '{fill_char * count}'")
        write_color_blocks(ser, [(COL_GREEN, count)], fill_char, charset)
        try:
            time.sleep(2)
        except KeyboardInterrupt:
            print(f"\n  Stopped at {count} chars.")
            print(f"  If all {count} were visible, try: --chars {count // 3}")
            return

    print("\n  Done. Divide the max visible count by 3 for --chars.")


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
        print(f"Opened {port_path} at 9600 7E1 (DTR/RTS disabled)")
        return ser
    except serial.SerialException as e:
        print(f"ERROR: Cannot open {port_path}: {e}")
        print("  Check: ls -la /dev/ttyUSB*")
        print("  Permissions: sudo usermod -aG dialout $USER")
        sys.exit(1)


def main():
    global verbose

    parser = argparse.ArgumentParser(
        description="Racing Countdown Animation for Portrait BetaBrite Sign")
    parser.add_argument("--port", "-p", default="/dev/ttyUSB0",
                        help="Serial port (default: /dev/ttyUSB0)")
    parser.add_argument("--chars", "-c", type=int, default=3,
                        help="Characters per color section (default: 3, tune with --test-fill)")
    parser.add_argument("--fill-char", default="#",
                        help="Fill character (default: #)")
    parser.add_argument("--delay", "-d", type=float, default=1.0,
                        help="Delay between stages in seconds (default: 1.0)")
    parser.add_argument("--charset", default="9",
                        choices=["3", "5", "6", "9"],
                        help="Charset: 3=7high, 5=7fancy, 6=10high, 9=full_high (default: 9)")
    parser.add_argument("--loop", "-l", action="store_true",
                        help="Loop the animation")
    parser.add_argument("--loop-delay", type=float, default=3.0,
                        help="Seconds to hold before restarting loop (default: 3.0)")
    parser.add_argument("--test-fill", action="store_true",
                        help="Calibration mode: send increasing chars to find display width")
    parser.add_argument("--verbose", "-v", action="store_true",
                        help="Print hex dumps of frames")
    args = parser.parse_args()

    verbose = args.verbose
    charset = ord(args.charset)

    ser = open_port(args.port)

    try:
        print("Configuring sign memory...")
        configure_memory(ser)

        if args.test_fill:
            run_test_fill(ser, fill_char=args.fill_char, charset=charset)
        elif args.loop:
            print("Looping countdown (Ctrl+C to stop)...")
            while True:
                run_countdown(ser, chars_per_section=args.chars,
                              fill_char=args.fill_char, delay=args.delay,
                              charset=charset)
                time.sleep(args.loop_delay)
        else:
            run_countdown(ser, chars_per_section=args.chars,
                          fill_char=args.fill_char, delay=args.delay,
                          charset=charset)
            # Hold port open briefly to avoid sign reset from DTR toggle
            time.sleep(5)
    except KeyboardInterrupt:
        print("\nStopped.")
    finally:
        ser.close()
        print(f"Port closed.")


if __name__ == "__main__":
    main()
