#!/bin/bash
# Test MQTT connection with certificates

echo "Testing MQTT connection to alert.d-t.pw:42690..."

# Test publish
mosquitto_pub \
    --cafile ca.crt \
    --cert client.crt \
    --key client.key \
    -h alert.d-t.pw \
    -p 42690 \
    -t "alerts/test" \
    -m "{\"test\": true, \"timestamp\": \"$(date -u +%Y-%m-%dT%H:%M:%SZ)\", \"client\": \"$(basename $(pwd))\"}"

if [ $? -eq 0 ]; then
    echo "✓ Connection test successful!"
else
    echo "✗ Connection test failed!"
    exit 1
fi
