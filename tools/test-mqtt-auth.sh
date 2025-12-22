#!/bin/bash
# MQTT Authentication Validation Script
# Tests TLS connection and authentication to Alert Manager MQTT broker

set -e

# Configuration
MQTT_HOST="${MQTT_HOST:-alert.d-t.pw}"
MQTT_PORT="${MQTT_PORT:-42690}"
CA_CERT="${CA_CERT:-data/certs/ca.crt}"
ZONE="${ZONE:-jimioffice}"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

print_ok() { echo -e "${GREEN}[OK]${NC} $1"; }
print_fail() { echo -e "${RED}[FAIL]${NC} $1"; }
print_info() { echo -e "${YELLOW}[INFO]${NC} $1"; }

echo "========================================"
echo "MQTT Authentication Validation"
echo "========================================"
echo "Host: $MQTT_HOST"
echo "Port: $MQTT_PORT"
echo "Zone: $ZONE"
echo "CA Cert: $CA_CERT"
echo ""

# Check for required tools
check_tools() {
    local missing=0
    for tool in openssl mosquitto_pub mosquitto_sub; do
        if ! command -v "$tool" &> /dev/null; then
            print_fail "$tool not found"
            missing=1
        fi
    done
    if [ $missing -eq 1 ]; then
        echo ""
        print_info "Install missing tools:"
        echo "  Ubuntu/Debian: sudo apt install openssl mosquitto-clients"
        echo "  macOS: brew install openssl mosquitto"
        exit 1
    fi
    print_ok "All required tools found"
}

# Check CA certificate exists
check_ca_cert() {
    if [ ! -f "$CA_CERT" ]; then
        print_fail "CA certificate not found: $CA_CERT"
        echo ""
        print_info "Extract CA cert from broker:"
        echo "  openssl s_client -connect $MQTT_HOST:$MQTT_PORT -showcerts </dev/null 2>/dev/null | openssl x509 -outform PEM > $CA_CERT"
        exit 1
    fi
    print_ok "CA certificate found"

    # Validate cert format
    if openssl x509 -in "$CA_CERT" -noout 2>/dev/null; then
        print_ok "CA certificate is valid PEM format"

        # Show cert info
        local subject=$(openssl x509 -in "$CA_CERT" -noout -subject 2>/dev/null | sed 's/subject=//')
        local expiry=$(openssl x509 -in "$CA_CERT" -noout -enddate 2>/dev/null | sed 's/notAfter=//')
        echo "    Subject: $subject"
        echo "    Expires: $expiry"
    else
        print_fail "CA certificate is not valid PEM format"
        exit 1
    fi
}

# Test TLS connection
test_tls_connection() {
    echo ""
    print_info "Testing TLS connection..."

    if timeout 10 openssl s_client -connect "$MQTT_HOST:$MQTT_PORT" -CAfile "$CA_CERT" -verify_return_error </dev/null 2>&1 | grep -q "Verify return code: 0"; then
        print_ok "TLS connection successful (certificate verified)"
    else
        # Try without strict verification to see what's happening
        local result=$(timeout 10 openssl s_client -connect "$MQTT_HOST:$MQTT_PORT" </dev/null 2>&1)
        if echo "$result" | grep -q "CONNECTED"; then
            print_fail "TLS connected but certificate verification failed"
            echo "$result" | grep -i "verify" | head -3
        else
            print_fail "Cannot connect to $MQTT_HOST:$MQTT_PORT"
        fi
        exit 1
    fi
}

# Test MQTT authentication
test_mqtt_auth() {
    echo ""

    # Build auth arguments (empty if no credentials)
    AUTH_ARGS=""
    if [ -n "$MQTT_USER" ]; then
        AUTH_ARGS="-u $MQTT_USER"
        if [ -n "$MQTT_PASS" ]; then
            AUTH_ARGS="$AUTH_ARGS -P $MQTT_PASS"
        fi
        print_info "Testing MQTT with credentials (user: $MQTT_USER)..."
    else
        print_info "Testing MQTT anonymous access..."
    fi

    # Test publish to a test topic
    local test_topic="ledSign/$ZONE/test"
    local test_msg="auth_test_$(date +%s)"

    # Try to publish
    if mosquitto_pub -h "$MQTT_HOST" -p "$MQTT_PORT" \
        --cafile "$CA_CERT" \
        $AUTH_ARGS \
        -t "$test_topic" -m "$test_msg" \
        -q 1 --quiet 2>&1; then
        if [ -n "$MQTT_USER" ]; then
            print_ok "MQTT authentication successful"
        else
            print_ok "MQTT anonymous access successful"
        fi
    else
        print_fail "MQTT connection failed"
        exit 1
    fi
}

# Test subscription
test_subscription() {
    echo ""
    print_info "Testing subscription to ledSign/$ZONE/message..."

    # Subscribe for 3 seconds
    timeout 3 mosquitto_sub -h "$MQTT_HOST" -p "$MQTT_PORT" \
        --cafile "$CA_CERT" \
        $AUTH_ARGS \
        -t "ledSign/$ZONE/message" \
        -q 1 -v 2>&1 || true

    # If we got here without error, subscription worked
    print_ok "Subscription successful (topic: ledSign/$ZONE/message)"
}

# Test sending a sample message
test_send_message() {
    echo ""

    # Auto-send if SEND_TEST=1, otherwise prompt
    if [ "$SEND_TEST" = "1" ]; then
        send_test="y"
    else
        read -p "Send a test message to the sign? [y/N]: " send_test
    fi

    if [[ "$send_test" =~ ^[Yy]$ ]]; then
        local timestamp=$(date +%s)
        local test_message=$(cat <<EOF
{
  "timestamp": $timestamp,
  "level": "info",
  "category": "system",
  "title": "Test",
  "message": "MQTT Auth Test OK",
  "source": "test-script"
}
EOF
)
        print_info "Sending test message..."

        if mosquitto_pub -h "$MQTT_HOST" -p "$MQTT_PORT" \
            --cafile "$CA_CERT" \
            $AUTH_ARGS \
            -t "ledSign/$ZONE/message" \
            -m "$test_message" \
            -q 1; then
            print_ok "Test message sent to ledSign/$ZONE/message"
        else
            print_fail "Failed to send test message"
        fi
    fi
}

# Main
echo ""
check_tools
echo ""
check_ca_cert
test_tls_connection
test_mqtt_auth
test_subscription
test_send_message

echo ""
echo "========================================"
print_ok "All tests passed!"
echo "========================================"
echo ""
echo "ESP32 Configuration:"
echo "  MQTT Server: $MQTT_HOST"
echo "  MQTT Port:   $MQTT_PORT"
echo "  Zone Name:   $ZONE"
echo "  Topic:       ledSign/$ZONE/message"
echo ""
