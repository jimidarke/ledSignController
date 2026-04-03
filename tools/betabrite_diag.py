#!/usr/bin/env python3
"""
BetaBrite LED Sign Diagnostic Tool

Communicates directly with a BetaBrite/Alpha Protocol LED sign over a USB
serial adapter, bypassing the ESP32 entirely. Used for hardware verification,
protocol debugging, and sign interrogation.

Protocol: Alpha Sign Communications Protocol
Serial: 9600 baud, 7 data bits, even parity, 1 stop bit (7E1)

Requires: pyserial (pip install pyserial)

Usage:
    python3 betabrite_diag.py --port /dev/ttyUSB0 <command>

Commands:
    loopback            Loopback test (short TX to RX on adapter first)
    test-write          Send "DIAG OK" to sign file A
    test-write-msg MSG  Send custom message to sign
    read-text LABEL     Read text file (A-Z) from sign
    read-special LABEL  Read special function from sign
    read-string LABEL   Read string file from sign
    discover            Sweep sign types and addresses for responses
    raw HEXBYTES        Send raw hex bytes, log response
    interactive         Interactive REPL for manual commands
    monitor             Passive receive mode - log all incoming bytes
"""

import argparse
import sys
import time
import struct
from datetime import datetime

try:
    import serial
except ImportError:
    print("ERROR: pyserial not installed. Run: pip install pyserial")
    sys.exit(1)


# Alpha Protocol constants (matches BBDEFS.h)
NUL = 0x00
SOH = 0x01
STX = 0x02
ETX = 0x03
EOT = 0x04
ESC = 0x1B

# Sign types
SIGN_TYPES = {
    "ALL":       0x3F,  # BB_ST_ALL '\077'
    "BETABRITE": 0x5E,  # BB_ST_BETABRITE '\136'
    "1LINE":     0x31,  # BB_ST_1LINE '\061'
    "2LINE":     0x32,  # BB_ST_2LINE '\062'
    "RESPONSE":  0x30,  # BB_ST_RESPONSE '\060'
    "ALL2":      0x5A,  # BB_ST_ALL2 '\132'
    "ALLAZ":     0x7A,  # BB_ST_ALLAZ '\172'
}

# Command codes
CC_WTEXT   = ord('A')
CC_RTEXT   = ord('B')
CC_WSPFUNC = ord('E')
CC_RSPFUNC = ord('F')
CC_WSTRING = ord('G')
CC_RSTRING = ord('H')

# Display modes
DM_ROTATE     = ord('a')
DM_HOLD       = ord('b')
DM_FLASH      = ord('c')
DM_SCROLL     = ord('m')
DM_SPECIAL    = ord('n')
DM_COMPROTATE = ord('t')

# Display positions
DP_TOPLINE = 0x22  # '\042'
DP_MIDLINE = 0x20  # '\040'
DP_FILL    = 0x30  # '\060'

# Colors
COL_RED       = ord('1')
COL_GREEN     = ord('2')
COL_AMBER     = ord('3')
COL_AUTOCOLOR = ord('C')

# Special effects
SDM_TWINKLE = ord('0')

# Formatting
FC_SELECTCHARCOLOR = 0x1C  # '\034'
FC_SELECTCHARSET   = 0x1A  # '\032'

# Inter-command delay (ms) - matches BB_BETWEEN_COMMAND_DELAY
COMMAND_DELAY = 0.110

# Verbosity
verbose = False


def log_hex(direction, data, timestamp=True):
    """Log bytes as hex dump with timestamp."""
    ts = datetime.now().strftime("%H:%M:%S.%f")[:-3] if timestamp else ""
    hex_str = " ".join(f"{b:02X}" for b in data)
    ascii_str = "".join(chr(b) if 0x20 <= b < 0x7F else "." for b in data)
    prefix = f"[{ts}] " if ts else ""
    print(f"{prefix}{direction}: {hex_str}  |{ascii_str}|")


def decode_frame(data):
    """Attempt to decode an Alpha Protocol frame for human-readable output."""
    if len(data) < 7:
        return "Frame too short"

    parts = []

    # Find SOH
    soh_idx = None
    for i, b in enumerate(data):
        if b == SOH:
            soh_idx = i
            break

    if soh_idx is None:
        return "No SOH found"

    if soh_idx > 0:
        parts.append(f"Preamble: {soh_idx} NUL bytes")

    parts.append(f"SOH at byte {soh_idx}")

    idx = soh_idx + 1
    if idx < len(data):
        sign_type = data[idx]
        type_name = next((k for k, v in SIGN_TYPES.items() if v == sign_type), f"0x{sign_type:02X}")
        parts.append(f"Type: {type_name} (0x{sign_type:02X})")
        idx += 1

    if idx + 1 < len(data):
        addr = chr(data[idx]) + chr(data[idx + 1])
        parts.append(f"Address: {addr}")
        idx += 2

    if idx < len(data) and data[idx] == STX:
        parts.append("STX")
        idx += 1

        if idx < len(data):
            cmd = data[idx]
            cmd_names = {
                CC_WTEXT: "WriteText", CC_RTEXT: "ReadText",
                CC_WSPFUNC: "WriteSpecFunc", CC_RSPFUNC: "ReadSpecFunc",
                CC_WSTRING: "WriteString", CC_RSTRING: "ReadString",
            }
            cmd_name = cmd_names.get(cmd, f"0x{cmd:02X}")
            parts.append(f"Command: {cmd_name} ({chr(cmd)})")
            idx += 1

        # Payload until ETX/EOT
        payload = []
        while idx < len(data) and data[idx] not in (ETX, EOT):
            payload.append(data[idx])
            idx += 1

        if payload:
            payload_ascii = "".join(chr(b) if 0x20 <= b < 0x7F else f"\\x{b:02X}" for b in payload)
            parts.append(f"Payload: {payload_ascii}")

        if idx < len(data):
            if data[idx] == ETX:
                parts.append("ETX")
            elif data[idx] == EOT:
                parts.append("EOT")

    return " | ".join(parts)


def build_frame(sign_type, address, command_code, payload=b""):
    """Build a complete Alpha Protocol frame.

    Args:
        sign_type: Sign type byte (e.g., 0x3F for ALL)
        address: 2-byte address string (e.g., "00")
        command_code: Command code byte (e.g., CC_WTEXT)
        payload: Additional payload bytes
    Returns:
        bytes: Complete frame ready to send
    """
    frame = bytes([NUL] * 5)           # 5 NUL preamble
    frame += bytes([SOH])              # Start of header
    frame += bytes([sign_type])        # Sign type
    frame += address.encode("ascii")   # 2-byte address
    frame += bytes([STX])              # Start of text
    frame += bytes([command_code])     # Command code
    frame += payload                   # Command payload
    frame += bytes([EOT])              # End of transmission
    return frame


def build_write_text_frame(sign_type, address, file_label, message,
                           color=COL_GREEN, position=DP_TOPLINE,
                           mode=DM_HOLD, special=SDM_TWINKLE):
    """Build a Write Text File frame."""
    payload = bytes([ord(file_label)])
    payload += bytes([ESC, position, mode])
    if mode == DM_SPECIAL:
        payload += bytes([special])
    if color != COL_AUTOCOLOR:
        payload += bytes([FC_SELECTCHARCOLOR, color])
    payload += message.encode("ascii")
    return build_frame(sign_type, address, CC_WTEXT, payload)


def build_read_text_frame(sign_type, address, file_label):
    """Build a Read Text File frame."""
    payload = bytes([ord(file_label)])
    return build_frame(sign_type, address, CC_RTEXT, payload)


def build_read_special_frame(sign_type, address, func_label):
    """Build a Read Special Function frame."""
    payload = bytes([ord(func_label)])
    return build_frame(sign_type, address, CC_RSPFUNC, payload)


def build_read_string_frame(sign_type, address, file_label):
    """Build a Read String File frame."""
    payload = bytes([ord(file_label)])
    return build_frame(sign_type, address, CC_RSTRING, payload)


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
            write_timeout=timeout,
        )
        print(f"Opened {port_path} at 9600 7E1 (timeout={timeout}s)")
        return ser
    except serial.SerialException as e:
        print(f"ERROR: Cannot open {port_path}: {e}")
        print("\nTroubleshooting:")
        print("  - Check the device exists: ls -la /dev/ttyUSB*")
        print("  - Check permissions: sudo usermod -aG dialout $USER")
        print("  - Identify adapters: ls -la /dev/serial/by-id/")
        sys.exit(1)


def send_frame(ser, frame):
    """Send a frame and log it."""
    log_hex("TX", frame)
    ser.write(frame)
    ser.flush()


def read_response(ser, timeout=2.0):
    """Read response from sign with timeout.

    Returns:
        bytes: Raw response data, or empty bytes if timeout
    """
    ser.timeout = timeout
    response = bytearray()

    # Wait for first byte
    start = time.time()
    while time.time() - start < timeout:
        if ser.in_waiting > 0:
            break
        time.sleep(0.01)
    else:
        if verbose:
            print(f"  No response within {timeout}s")
        return bytes()

    # Read all available bytes with short inter-byte timeout
    ser.timeout = 0.1
    while True:
        chunk = ser.read(256)
        if not chunk:
            break
        response.extend(chunk)

    if response:
        log_hex("RX", response)

    ser.timeout = timeout
    return bytes(response)


def parse_response_payload(response):
    """Extract payload from a response frame (between STX and ETX/EOT)."""
    stx_idx = None
    for i, b in enumerate(response):
        if b == STX:
            stx_idx = i
            break

    if stx_idx is None:
        return None

    payload = bytearray()
    for i in range(stx_idx + 1, len(response)):
        if response[i] in (ETX, EOT):
            break
        payload.append(response[i])

    return bytes(payload)


# ---- Commands ----

def cmd_loopback(ser):
    """Loopback test: send known pattern, verify echo.

    BEFORE RUNNING: physically short TX to RX on the serial adapter.
    - DB9: connect pin 2 (RX) to pin 3 (TX)
    - TTL header: connect TX to RX
    """
    print("\n=== LOOPBACK TEST ===")
    print("IMPORTANT: Short TX to RX on the adapter before running this test.")
    print("  DB9: connect pin 2 to pin 3")
    print("  TTL: connect TX to RX\n")

    test_patterns = [
        bytes([0xAA, 0x55, 0x00, 0xFF]),
        bytes([0x01, 0x02, 0x03, 0x04]),
        bytes([0x00] * 5 + [0x01, 0x3F, 0x30, 0x30, 0x02, 0x41, 0x04]),  # Looks like a frame
    ]

    # Flush any stale data
    ser.reset_input_buffer()
    ser.reset_output_buffer()

    all_passed = True
    for i, pattern in enumerate(test_patterns):
        print(f"\nPattern {i + 1}/{len(test_patterns)}:")
        log_hex("TX", pattern)

        ser.write(pattern)
        ser.flush()
        time.sleep(0.05)  # Short delay for loopback

        received = ser.read(len(pattern) + 16)  # Read a bit extra
        if received:
            log_hex("RX", received)
        else:
            print("  RX: (nothing)")

        # Note: 7E1 strips the high bit, so transmitted 0xAA becomes 0x2A etc.
        # For 7-bit serial, we compare only low 7 bits
        expected_7bit = bytes([b & 0x7F for b in pattern])
        received_7bit = bytes([b & 0x7F for b in received[:len(pattern)]])

        if received_7bit == expected_7bit:
            print("  PASS (7-bit match)")
        elif received == pattern:
            print("  PASS (exact match)")
        elif len(received) == 0:
            print("  FAIL: No data received")
            print("  -> Check that TX is shorted to RX on the adapter")
            all_passed = False
        else:
            print(f"  FAIL: Mismatch")
            print(f"    Expected (7-bit): {' '.join(f'{b:02X}' for b in expected_7bit)}")
            print(f"    Got      (7-bit): {' '.join(f'{b:02X}' for b in received_7bit)}")
            all_passed = False

    print(f"\n{'ALL TESTS PASSED' if all_passed else 'SOME TESTS FAILED'}")
    print("\nNote: 7E1 serial config strips the high bit (bit 7).")
    print("This is expected - the BetaBrite protocol uses 7-bit ASCII.")
    return all_passed


def cmd_test_write(ser, message="DIAG OK", sign_type=0x3F, address="00"):
    """Send a test message to the sign."""
    print(f"\n=== WRITE TEST ===")
    print(f"Message: '{message}'")
    print(f"Sign type: 0x{sign_type:02X}, Address: {address}")
    print(f"Target: File A, Green, Hold mode\n")

    # Flush receive buffer
    ser.reset_input_buffer()

    frame = build_write_text_frame(sign_type, address, 'A', message,
                                   color=COL_GREEN, position=DP_TOPLINE,
                                   mode=DM_HOLD)
    send_frame(ser, frame)

    print(f"\nDecoded: {decode_frame(frame)}")
    print(f"\nIf the sign displays '{message}', the hardware path is working.")
    print("If nothing happens, check:")
    print("  1. RS232 voltage levels (need +/-12V, not 3.3V TTL)")
    print("  2. TX/RX are not swapped")
    print("  3. Ground is connected")
    print("  4. Sign is powered on and in 'auto' mode")

    # Check for any response
    time.sleep(0.5)
    response = read_response(ser, timeout=1.0)
    if response:
        print(f"\nSign responded! Decoded: {decode_frame(response)}")


def cmd_read_text(ser, file_label, sign_type=0x3F, address="00"):
    """Read a text file from the sign."""
    print(f"\n=== READ TEXT FILE '{file_label}' ===")
    print(f"Sign type: 0x{sign_type:02X}, Address: {address}\n")

    ser.reset_input_buffer()

    frame = build_read_text_frame(sign_type, address, file_label)
    send_frame(ser, frame)
    print(f"Decoded: {decode_frame(frame)}")

    time.sleep(COMMAND_DELAY)
    response = read_response(ser, timeout=2.0)

    if response:
        print(f"\nResponse decoded: {decode_frame(response)}")
        payload = parse_response_payload(response)
        if payload:
            # Skip command echo byte and file label
            text = payload[2:] if len(payload) > 2 else payload
            print(f"Text content: {text.decode('ascii', errors='replace')}")
    else:
        print("\nNo response from sign.")
        print("This may be normal - some BetaBrite models don't support read commands,")
        print("or they may only respond to specific sign type codes (try: discover)")


def cmd_read_special(ser, func_label, sign_type=0x3F, address="00"):
    """Read a special function from the sign."""
    print(f"\n=== READ SPECIAL FUNCTION '{func_label}' ===")

    func_labels = {
        ' ': "Time of Day",
        '\"': "Speaker Status",
        '#': "General Information",
        '&': "Day of Week",
        ';': "Date",
    }
    desc = func_labels.get(func_label, "Unknown")
    print(f"Function: {desc}")
    print(f"Sign type: 0x{sign_type:02X}, Address: {address}\n")

    ser.reset_input_buffer()

    frame = build_read_special_frame(sign_type, address, func_label)
    send_frame(ser, frame)
    print(f"Decoded: {decode_frame(frame)}")

    time.sleep(COMMAND_DELAY)
    response = read_response(ser, timeout=2.0)

    if response:
        print(f"\nResponse decoded: {decode_frame(response)}")
        payload = parse_response_payload(response)
        if payload:
            print(f"Data: {payload.decode('ascii', errors='replace')}")
    else:
        print("\nNo response from sign.")


def cmd_read_string(ser, file_label, sign_type=0x3F, address="00"):
    """Read a string file from the sign."""
    print(f"\n=== READ STRING FILE '{file_label}' ===")
    print(f"Sign type: 0x{sign_type:02X}, Address: {address}\n")

    ser.reset_input_buffer()

    frame = build_read_string_frame(sign_type, address, file_label)
    send_frame(ser, frame)
    print(f"Decoded: {decode_frame(frame)}")

    time.sleep(COMMAND_DELAY)
    response = read_response(ser, timeout=2.0)

    if response:
        print(f"\nResponse decoded: {decode_frame(response)}")
        payload = parse_response_payload(response)
        if payload:
            print(f"String content: {payload[2:].decode('ascii', errors='replace')}")
    else:
        print("\nNo response from sign.")


def cmd_discover(ser):
    """Sweep sign types and addresses, looking for responses."""
    print("\n=== SIGN DISCOVERY ===")
    print("Trying combinations of sign type + address...")
    print("Sending Read Special Function (time of day) to each.\n")

    found = []
    types_to_try = ["ALL", "BETABRITE", "1LINE", "2LINE", "RESPONSE", "ALL2", "ALLAZ"]
    addresses_to_try = ["00", "01", "02", "03", "04", "05"]

    total = len(types_to_try) * len(addresses_to_try)
    count = 0

    for type_name in types_to_try:
        sign_type = SIGN_TYPES[type_name]
        for addr in addresses_to_try:
            count += 1
            sys.stdout.write(f"\r  [{count}/{total}] Type={type_name} (0x{sign_type:02X}), Addr={addr}  ")
            sys.stdout.flush()

            ser.reset_input_buffer()

            frame = build_read_special_frame(sign_type, addr, ' ')
            ser.write(frame)
            ser.flush()

            time.sleep(COMMAND_DELAY)
            response = read_response(ser, timeout=0.5)

            if response:
                print(f"\n  >>> RESPONSE from Type={type_name}, Addr={addr}!")
                log_hex("RX", response)
                found.append((type_name, sign_type, addr, response))

    print(f"\n\nDiscovery complete. Found {len(found)} responding combination(s).")

    if found:
        print("\nResponding signs:")
        for type_name, sign_type, addr, resp in found:
            print(f"  Type: {type_name} (0x{sign_type:02X}), Address: {addr}")
            payload = parse_response_payload(resp)
            if payload:
                print(f"  Data: {payload.decode('ascii', errors='replace')}")
    else:
        print("\nNo signs responded to read commands.")
        print("This doesn't necessarily mean the sign is broken - try 'test-write' first.")
        print("Many BetaBrite signs accept write commands but don't respond to reads.")
        print("\nIf test-write also fails, check:")
        print("  1. RS232 adapter outputs +/-12V levels (not TTL)")
        print("  2. TX from adapter connects to sign RX")
        print("  3. Ground is connected")
        print("  4. Sign is powered on")


def cmd_raw(ser, hex_string):
    """Send raw hex bytes and log any response."""
    print(f"\n=== RAW SEND ===")

    try:
        data = bytes.fromhex(hex_string.replace(" ", ""))
    except ValueError as e:
        print(f"ERROR: Invalid hex string: {e}")
        return

    ser.reset_input_buffer()

    print(f"Sending {len(data)} bytes:")
    send_frame(ser, data)

    time.sleep(COMMAND_DELAY)
    response = read_response(ser, timeout=2.0)

    if response:
        print(f"\nResponse ({len(response)} bytes):")
        print(f"Decoded: {decode_frame(response)}")
    else:
        print("\nNo response.")


def cmd_interactive(ser):
    """Interactive REPL for manual sign communication."""
    print("\n=== INTERACTIVE MODE ===")
    print("Commands:")
    print("  w <file> <message>     Write text to file (e.g., w A Hello)")
    print("  rt <file>              Read text file (e.g., rt A)")
    print("  rs <file>              Read string file")
    print("  rf <label>             Read special function")
    print("  raw <hex>              Send raw hex bytes")
    print("  type <name>            Set sign type (ALL, BETABRITE, etc)")
    print("  addr <NN>              Set address (e.g., 00)")
    print("  ping                   Ping sign (read time of day)")
    print("  q                      Quit\n")

    current_type = 0x3F
    current_addr = "00"

    while True:
        try:
            line = input(f"[type=0x{current_type:02X} addr={current_addr}]> ").strip()
        except (EOFError, KeyboardInterrupt):
            print()
            break

        if not line:
            continue

        parts = line.split(maxsplit=2)
        cmd = parts[0].lower()

        if cmd == "q":
            break
        elif cmd == "type":
            if len(parts) < 2:
                print(f"Available: {', '.join(SIGN_TYPES.keys())}")
                continue
            name = parts[1].upper()
            if name in SIGN_TYPES:
                current_type = SIGN_TYPES[name]
                print(f"Sign type set to {name} (0x{current_type:02X})")
            else:
                print(f"Unknown type. Available: {', '.join(SIGN_TYPES.keys())}")
        elif cmd == "addr":
            if len(parts) < 2:
                print(f"Current: {current_addr}")
            else:
                current_addr = parts[1][:2].zfill(2)
                print(f"Address set to {current_addr}")
        elif cmd == "w":
            if len(parts) < 3:
                print("Usage: w <file_label> <message>")
                continue
            file_label = parts[1][0]
            message = parts[2]
            cmd_test_write(ser, message=message, sign_type=current_type, address=current_addr)
        elif cmd == "rt":
            label = parts[1][0] if len(parts) > 1 else "A"
            cmd_read_text(ser, label, sign_type=current_type, address=current_addr)
        elif cmd == "rs":
            label = parts[1][0] if len(parts) > 1 else "1"
            cmd_read_string(ser, label, sign_type=current_type, address=current_addr)
        elif cmd == "rf":
            label = parts[1][0] if len(parts) > 1 else " "
            cmd_read_special(ser, label, sign_type=current_type, address=current_addr)
        elif cmd == "raw":
            if len(parts) < 2:
                print("Usage: raw <hex bytes>")
                continue
            cmd_raw(ser, " ".join(parts[1:]))
        elif cmd == "ping":
            cmd_read_special(ser, " ", sign_type=current_type, address=current_addr)
        else:
            print(f"Unknown command: {cmd}")


def cmd_monitor(ser):
    """Passive receive mode - log all incoming bytes."""
    print("\n=== MONITOR MODE ===")
    print("Listening for incoming data. Press Ctrl+C to stop.\n")

    ser.timeout = 0.1
    buffer = bytearray()

    try:
        while True:
            data = ser.read(256)
            if data:
                buffer.extend(data)
                log_hex("RX", data)

                # Try to decode if we see a complete frame (SOH ... EOT)
                if EOT in buffer:
                    print(f"  Frame: {decode_frame(buffer)}")
                    buffer.clear()
            else:
                # Flush partial buffer after idle period
                if buffer:
                    time.sleep(0.2)
                    if ser.in_waiting == 0:
                        print(f"  Partial: {decode_frame(buffer)}")
                        buffer.clear()
    except KeyboardInterrupt:
        print("\nMonitor stopped.")


def main():
    global verbose

    parser = argparse.ArgumentParser(
        description="BetaBrite LED Sign Diagnostic Tool",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Wiring Guide:
  RS232 adapter (DB9) to BetaBrite:
    Adapter TX (pin 3) -> Sign RX
    Adapter RX (pin 2) -> Sign TX
    Adapter GND (pin 5) -> Sign GND

  TTL adapter + MAX3232 breakout:
    FTDI TX -> MAX3232 TTL-in -> MAX3232 RS232-out -> Sign RX
    Sign TX -> MAX3232 RS232-in -> MAX3232 TTL-out -> FTDI RX
    GND connected throughout

  Port identification:
    ls -la /dev/serial/by-id/
    dmesg | tail   (after plugging in adapter)
""",
    )
    parser.add_argument("--port", "-p", required=True, help="Serial port (e.g., /dev/ttyUSB0)")
    parser.add_argument("--timeout", "-t", type=float, default=2.0, help="Read timeout in seconds (default: 2.0)")
    parser.add_argument("--verbose", "-v", action="store_true", help="Verbose output")
    parser.add_argument("--sign-type", default="ALL", choices=SIGN_TYPES.keys(),
                        help="Sign type code (default: ALL)")
    parser.add_argument("--address", default="00", help="Sign address (default: 00)")

    subparsers = parser.add_subparsers(dest="command", help="Command to execute")

    subparsers.add_parser("loopback", help="Loopback test (short TX to RX first)")

    p_tw = subparsers.add_parser("test-write", help="Send test message to sign")
    p_tw.add_argument("message", nargs="?", default="DIAG OK", help="Message to send (default: DIAG OK)")

    p_rt = subparsers.add_parser("read-text", help="Read text file from sign")
    p_rt.add_argument("label", default="A", nargs="?", help="File label A-Z (default: A)")

    p_rs = subparsers.add_parser("read-special", help="Read special function")
    p_rs.add_argument("label", default=" ", nargs="?",
                      help="Function label: ' '=time, '\"'=speaker, '#'=info, '&'=dow, ';'=date")

    p_rst = subparsers.add_parser("read-string", help="Read string file from sign")
    p_rst.add_argument("label", default="1", nargs="?", help="File label (default: 1)")

    subparsers.add_parser("discover", help="Sweep sign types and addresses")

    p_raw = subparsers.add_parser("raw", help="Send raw hex bytes")
    p_raw.add_argument("hexbytes", help="Hex bytes to send (e.g., '00 00 00 00 00 01 3F')")

    subparsers.add_parser("interactive", help="Interactive REPL")
    subparsers.add_parser("monitor", help="Passive receive mode")

    args = parser.parse_args()
    verbose = args.verbose

    if not args.command:
        parser.print_help()
        sys.exit(1)

    print("=" * 60)
    print("  BetaBrite LED Sign Diagnostic Tool")
    print("  Protocol: Alpha Sign Communications (9600 7E1)")
    print("=" * 60)

    ser = open_port(args.port, timeout=args.timeout)
    sign_type = SIGN_TYPES[args.sign_type]
    address = args.address

    try:
        if args.command == "loopback":
            cmd_loopback(ser)
        elif args.command == "test-write":
            cmd_test_write(ser, message=args.message, sign_type=sign_type, address=address)
        elif args.command == "read-text":
            cmd_read_text(ser, args.label, sign_type=sign_type, address=address)
        elif args.command == "read-special":
            cmd_read_special(ser, args.label, sign_type=sign_type, address=address)
        elif args.command == "read-string":
            cmd_read_string(ser, args.label, sign_type=sign_type, address=address)
        elif args.command == "discover":
            cmd_discover(ser)
        elif args.command == "raw":
            cmd_raw(ser, args.hexbytes)
        elif args.command == "interactive":
            cmd_interactive(ser)
        elif args.command == "monitor":
            cmd_monitor(ser)
    finally:
        ser.close()
        print(f"\nPort {args.port} closed.")


if __name__ == "__main__":
    main()
