# MQTT Security Architecture

**Version:** 0.2.1
**Date:** 2025-12-22
**Status:** Production Ready

This document describes the secure MQTT architecture for LED Sign Controller communication with a central MQTT broker.

---

## Security Model

### Server-Only TLS + Username/Password Authentication

The LED Sign Controller uses a layered security approach:

| Layer | Mechanism | Protection |
|-------|-----------|------------|
| **Transport** | TLS 1.2 (port 8883) | Encryption in transit, broker identity |
| **Authentication** | Username/Password | Device identity verification |
| **Authorization** | Topic ACLs | Per-device topic permissions |
| **Network** | IP Whitelisting (optional) | Defense-in-depth |

### Why Not Mutual TLS (mTLS)?

While MQTTManager supports mTLS, we chose server-only TLS because:

- **Simpler Operations**: No per-device certificate management
- **Easier Rotation**: Password changes don't require reflashing
- **Less Memory**: ~50% less heap usage (CA cert only vs 3 certs)
- **Faster Debugging**: Username/password errors are clear; TLS handshake failures are cryptic
- **Same Encryption**: TLS 1.2 with AES-256 regardless of auth method

---

## Architecture Overview

```
┌─────────────────────────────────────────────────────────────────┐
│                    MQTT BROKER (Mosquitto)                       │
│  Port 8883 (TLS)                                                 │
│  ┌─────────────────────────────────────────────────────────┐    │
│  │ TLS: Server cert validates broker identity              │    │
│  │      ESP32 validates via CA cert (ca.crt)              │    │
│  └─────────────────────────────────────────────────────────┘    │
│  ┌─────────────────────────────────────────────────────────┐    │
│  │ Auth: Username/password in MQTT CONNECT packet          │    │
│  │       Validated against password_file                   │    │
│  └─────────────────────────────────────────────────────────┘    │
│  ┌─────────────────────────────────────────────────────────┐    │
│  │ ACLs: Topic permissions per user                        │    │
│  │       ledSign/{zone}/* for zone-specific access         │    │
│  └─────────────────────────────────────────────────────────┘    │
└─────────────────────────────────────────────────────────────────┘
                              │
                              │ TLS 1.2 (AES-256-GCM)
                              │
┌─────────────────────────────────────────────────────────────────┐
│                   ESP32 LED SIGN CONTROLLER                      │
│  ┌─────────────────────────────────────────────────────────┐    │
│  │ WiFiClientSecure: CA cert validates broker              │    │
│  │ PubSubClient: username/password in CONNECT              │    │
│  │ Topics: ledSign/{zone}/message (subscribe)              │    │
│  │         ledSign/{device_id}/* (publish telemetry)       │    │
│  └─────────────────────────────────────────────────────────┘    │
└─────────────────────────────────────────────────────────────────┘
```

---

## Server-Side Setup (Mosquitto)

### 1. Generate Certificates

```bash
# Create CA (15-year validity)
openssl genrsa -out ca.key 4096
openssl req -new -x509 -days 5475 -key ca.key -out ca.crt \
  -subj "/CN=MQTT-CA/O=YourOrg"

# Create server key and certificate
openssl genrsa -out server.key 2048
openssl req -new -key server.key -out server.csr \
  -subj "/CN=your-mqtt-server.example.com/O=YourOrg"

# Sign server cert with CA (2-year validity)
openssl x509 -req -in server.csr -CA ca.crt -CAkey ca.key \
  -CAcreateserial -out server.crt -days 730

# Verify certificate chain
openssl verify -CAfile ca.crt server.crt
```

### 2. Configure Mosquitto

**File: /etc/mosquitto/mosquitto.conf**
```
# TLS Listener
listener 8883
protocol mqtt

# Server certificates
cafile /etc/mosquitto/certs/ca.crt
certfile /etc/mosquitto/certs/server.crt
keyfile /etc/mosquitto/certs/server.key

# Client authentication (username/password, not certs)
require_certificate false
use_identity_as_username false

# Password authentication
allow_anonymous false
password_file /etc/mosquitto/passwords

# Topic ACLs
acl_file /etc/mosquitto/acls
```

### 3. Create Users

```bash
# Create password file (first user)
sudo mosquitto_passwd -c /etc/mosquitto/passwords ledsign_kitchen

# Add more users
sudo mosquitto_passwd /etc/mosquitto/passwords ledsign_office
sudo mosquitto_passwd /etc/mosquitto/passwords alert_manager

# Restart mosquitto
sudo systemctl restart mosquitto
```

### 4. Configure ACLs

**File: /etc/mosquitto/acls**
```
# LED Sign devices - zone-specific read access
user ledsign_kitchen
topic read ledSign/kitchen/#
topic write ledSign/+/rssi
topic write ledSign/+/ip
topic write ledSign/+/uptime
topic write ledSign/+/memory

user ledsign_office
topic read ledSign/office/#
topic write ledSign/+/rssi
topic write ledSign/+/ip
topic write ledSign/+/uptime
topic write ledSign/+/memory

# Alert Manager - can publish to all zones
user alert_manager
topic write ledSign/+/message
topic read ledSign/#
```

---

## ESP32 Client Setup

### 1. Deploy CA Certificate

Only the CA certificate is needed (not client cert/key):

```bash
# Copy CA cert to project
cp ca.crt data/certs/ca.crt

# Upload filesystem to ESP32
pio run -t uploadfs
```

### 2. Configure via WiFi Portal

1. Power on ESP32 (first boot or after reset)
2. Connect to WiFi AP: `LEDSign` / `ledsign0`
3. Navigate to `192.168.4.1`
4. Enter:
   - **MQTT Server**: `your-mqtt-server.example.com`
   - **MQTT Port**: `8883`
   - **MQTT User**: `ledsign_kitchen`
   - **MQTT Pass**: (your password)
   - **Zone**: `kitchen`

### 3. Verify Connection

Serial output on successful connection:
```
MQTTManager: CA certificate loaded (2048 bytes)
MQTTManager: Server verification ready (username/password auth)
MQTTManager: Connecting to your-mqtt-server.example.com:8883 (TLS)
MQTTManager: Connected as ledsign_kitchen
MQTTManager: Subscribed to ledSign/kitchen/message
```

---

## Testing

### Command-Line Test

```bash
# Subscribe to zone messages
mosquitto_sub -h your-mqtt-server.example.com -p 8883 \
  --cafile ca.crt \
  -u ledsign_kitchen -P "your_password" \
  -t 'ledSign/kitchen/#'

# Publish test alert (as alert_manager)
mosquitto_pub -h your-mqtt-server.example.com -p 8883 \
  --cafile ca.crt \
  -u alert_manager -P "manager_password" \
  -t 'ledSign/kitchen/message' \
  -m '{"title":"Test","message":"Security test","level":"info","category":"system"}'
```

### Verify ACL Enforcement

```bash
# This should FAIL (kitchen can't read office)
mosquitto_sub -h your-mqtt-server.example.com -p 8883 \
  --cafile ca.crt \
  -u ledsign_kitchen -P "your_password" \
  -t 'ledSign/office/#'
# Expected: Not authorized

# This should FAIL (kitchen can't publish messages)
mosquitto_pub -h your-mqtt-server.example.com -p 8883 \
  --cafile ca.crt \
  -u ledsign_kitchen -P "your_password" \
  -t 'ledSign/kitchen/message' \
  -m '{"title":"Hack","message":"Unauthorized"}'
# Expected: Not authorized
```

---

## Troubleshooting

| Symptom | Cause | Fix |
|---------|-------|-----|
| `Certificate verify failed` | Wrong CA cert | Ensure ca.crt matches server cert issuer |
| `Connection refused` | Firewall/port | Check port 8883 is open |
| `Not authorized` | Wrong credentials | Check username/password in password_file |
| `TLS handshake timeout` | NTP not synced | ESP32 needs valid time for cert validation |
| Connection works then fails | Heap exhaustion | Monitor ESP.getFreeHeap(), keep >100KB |
| `Bad protocol` | TLS to non-TLS port | Ensure server has TLS on 8883 |

### Enable Debug Logging

**Mosquitto:**
```
log_type all
log_dest file /var/log/mosquitto/mosquitto.log
```

**ESP32:** (in platformio.ini)
```ini
build_flags =
    -D CORE_DEBUG_LEVEL=4
```

---

## Security Considerations

### Credential Rotation

1. Generate new password on server: `mosquitto_passwd /etc/mosquitto/passwords ledsign_kitchen`
2. Restart mosquitto: `systemctl restart mosquitto`
3. Update ESP32 via WiFi portal (reset device to trigger portal)

### IP Whitelisting (Optional)

Add to mosquitto.conf for additional security:
```
# Only allow connections from specific IPs
listener 8883 192.168.1.0/24
```

### Certificate Renewal

- CA cert: 15-year validity, rarely needs renewal
- Server cert: 2-year validity, renew before expiration
- No client certs to manage (using username/password)

### Flash Encryption (High Security)

For deployments requiring protection against physical device access:
1. Enable ESP32 flash encryption
2. Enable NVS encryption
3. Credentials stored in encrypted flash

---

## MQTT Topics Reference

### Subscriptions (ESP32 → Broker)

| Topic | QoS | Description |
|-------|-----|-------------|
| `ledSign/{zone}/message` | 1 | Alert messages (JSON) |

### Publications (ESP32 → Broker)

| Topic | QoS | Retained | Description |
|-------|-----|----------|-------------|
| `ledSign/{device_id}/rssi` | 0 | Yes | WiFi signal strength |
| `ledSign/{device_id}/ip` | 0 | Yes | Device IP address |
| `ledSign/{device_id}/uptime` | 0 | Yes | Seconds since boot |
| `ledSign/{device_id}/memory` | 0 | Yes | Free heap bytes |

### Alert Message Format

```json
{
  "level": "warning",
  "category": "system",
  "title": "Alert Title",
  "message": "Alert details",
  "display_config": {
    "mode_code": "c",
    "color_code": "1",
    "priority": true,
    "duration": 30
  }
}
```

---

## For Alert Manager Integration

The Alert Manager server should:

1. **Connect as `alert_manager` user** with write access to all zones
2. **Publish to `ledSign/{zone}/message`** with JSON alerts
3. **Monitor `ledSign/+/rssi`** etc. for device health
4. **Use QoS 1** for reliable delivery

Example Alert Manager configuration:
```yaml
mqtt:
  host: your-mqtt-server.example.com
  port: 8883
  tls:
    ca_cert: /path/to/ca.crt
  auth:
    username: alert_manager
    password: ${ALERT_MANAGER_MQTT_PASSWORD}
  topics:
    publish: "ledSign/{zone}/message"
    subscribe: "ledSign/+/+"
```

---

## References

- [Mosquitto TLS Configuration](https://mosquitto.org/man/mosquitto-conf-5.html)
- [ESP32 WiFiClientSecure](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/protocols/esp_tls.html)
- [MQTT Security Best Practices](https://www.hivemq.com/mqtt/mqtt-security-fundamentals/)
