# Alert Manager Client Integration Reference

**Version:** 1.1.0
**Last Updated:** 2025-11-30

Technical reference for sending alerts to the Alert Manager system. Primary audience: LLM code assistants and senior developers.

## Connection Endpoints

| Protocol | Host | Port | Auth | Use Case |
|----------|------|------|------|----------|
| **HTTPS** | `alert.d-t.pw` | `443` | Basic Auth / Bearer | Web services, scripts, one-way alerts |
| **HTTP** | `alert.d-t.pw` | `8080` | Basic Auth / Bearer | Legacy/non-TLS environments |
| **MQTT** | `alert.d-t.pw` | `42690` | TLS + username/password | IoT, persistent connections, high-volume |
| **SNMPv3** | `alert.d-t.pw` | `162/UDP` | USM (noAuthNoPriv/authPriv) | Network devices, hardware monitoring |

### HTTP Endpoints

| Method | Path | Auth | Description |
|--------|------|------|-------------|
| `POST` | `/alert` | Required | Send alert payload |
| `POST` | `/test` | Required | Send test alert |
| `GET` | `/health` | None | Health check |
| `GET` | `/logs` | Required | Log viewer web interface |
| `GET` | `/api/logs/list` | Required | List available log files |
| `GET` | `/api/logs/tail` | Required | Tail log file for streaming |
| `GET` | `/docs` | Required | Documentation |

## Authentication & Access Control

| User Type | Rate Limit | Can Send | Log Access | Expires |
|-----------|------------|----------|------------|---------|
| `admin` | 1000/hr | Yes (all clients) | All logs (ops/, file/) | Never |
| `node` | 1000/hr | Yes (auto-inherit clientname) | file/{clientname}*.log | 5 years |
| `client` | 100/hr | No | file/{clientname}*.log | 2 years |

**Client Name Inheritance**: For `node` users, if `client` field is empty/missing in payload, the user's configured `clientname` is automatically used.

## Alert Payload Schema

### Required Fields

| Field | Type | Constraints |
|-------|------|-------------|
| `timestamp` | integer | Unix epoch seconds (no decimals, not ISO string) |
| `level` | string | One of: `critical`, `warning`, `notice`, `info`, `debug` |
| `category` | string | One of: `security`, `system`, `weather`, `automation`, `personal`, `network`, `application`, `news` |
| `title` | string | Short alert title |
| `message` | string | Detailed message (supports `<a href>` tags in log viewer) |
| `source` | string | Source identifier (must be string, not object) |

### Optional Fields

| Field | Type | Description |
|-------|------|-------------|
| `client` | string | Client identifier for log routing (auto-inherited from user if empty) |
| `metadata` | object | Arbitrary key-value data |
| `display_config` | object | LED sign display settings |
| `routing` | object | Override routing rules |

### Level Aliases

| Canonical | Aliases |
|-----------|---------|
| `critical` | `error`, `err`, `crit` |
| `warning` | `warn` |
| `info` | `inf` |
| `debug` | `dbg` |

### String Normalization

All string fields (`level`, `category`, `client`) are automatically:
- Whitespace trimmed
- Lowercased
- Alias-resolved (for `level`)

### Minimal Payload

```json
{
  "timestamp": 1732982400,
  "level": "info",
  "category": "application",
  "title": "Status Update",
  "message": "Operation completed successfully",
  "source": "my-service"
}
```

### Full Payload

```json
{
  "timestamp": 1732982400,
  "level": "critical",
  "category": "security",
  "title": "Security Alert",
  "message": "Unauthorized access detected",
  "source": "security-monitor",
  "client": "prod-security",
  "metadata": {
    "ip_address": "192.168.1.100",
    "led_zones": ["office", "security_desk"]
  },
  "display_config": {
    "mode": "flash",
    "color": "red",
    "special_effect": "bomb",
    "priority": true,
    "duration": 30
  },
  "routing": {
    "override": true,
    "pathways": ["email_critical", "led_sign", "file_log"],
    "exclude": ["console_alerts"]
  }
}
```

### LED Display Config Options

| Field | Values |
|-------|--------|
| `mode` | `rotate`, `hold`, `flash`, `scroll`, `rollup`, `rolldown`, `rollright`, `rollleft`, `explode`, `newsflash`, `welcome`, `compressed` |
| `color` | `red`, `green`, `amber`, `rainbow1`, `rainbow2`, `autocolor`, `color_mix` |
| `special_effect` | `twinkle`, `sparkle`, `snow`, `starburst`, `bomb`, `welcome`, `cyclecolors` |
| `character_set` | `5high`, `7high`, `10high`, `7shadow`, `7highfancy`, `7widestroke` |
| `speed` | 1-5 (slowest to fastest) |
| `priority` | boolean (interrupt normal rotation) |
| `duration` | seconds |

**Zone targeting**: Use `metadata.led_zones` array to specify display zones.

## Output Pathways

| Pathway | Description |
|---------|-------------|
| **file** | Date-rotated log files with per-client routing (`/data/logs/file/{client}/{date}.log`) |
| **email** | Microsoft Graph API (O365) with MSAL auth, HTML templates |
| **led_sign** | BetaBrite LED signs with multi-zone support via MQTT |
| **console** | stdout logging |
| **database** | PostgreSQL with weekly partitions, 1-year retention |

Routing is configuration-driven with first-match semantics. Payload-level overrides via `routing` field.

## Quick Start Examples

### cURL

```bash
curl -X POST https://alert.d-t.pw/alert \
  -u username:password \
  -H "Content-Type: application/json" \
  -d '{"timestamp":1732982400,"level":"warning","category":"system","title":"High CPU","message":"CPU at 85%","source":"monitor"}'
```

### Python (requests)

```python
import requests, time

requests.post(
    "https://alert.d-t.pw/alert",
    auth=("username", "password"),
    json={
        "timestamp": int(time.time()),
        "level": "info",
        "category": "application",
        "title": "Status",
        "message": "Task completed",
        "source": "my-app"
    }
)
```

## Additional Features

| Feature | Description | Documentation |
|---------|-------------|---------------|
| **Analytics Engine** | Scheduled log analysis with HTML email reports | `ANALYTICS.md`, `ANALYTICS_QUICKSTART.md` |
| **Database Backend** | PostgreSQL storage with SQL-based analytics | `DATABASE_INTEGRATION.md` |
| **SNMPv3 Gateway** | Trap reception with OID-to-alert mapping | `SNMP_INTEGRATION.md`, `SNMPV3_SPECIFICATION.md` |
| **Feed Aggregator** | RSS/API polling (news, jokes, facts, trivia) | Config: `feed_aggregator` section |
| **HTML Links** | `<a href>` tags rendered clickable in log viewer | HTTP/HTTPS protocols only |

## Management Tools

| Tool | Purpose |
|------|---------|
| `tools/http-auth-manager.sh` | HTTP user management (create-user, delete-user, list-users, set-clientname) |
| `tools/snmp-auth-manager.sh` | SNMPv3 user management (create-user, delete-user, list-users) |
| `tools/cert-manager.sh` | TLS certificate management |
| `tools/mqtt-auth-manager.sh` | MQTT user management |

## Validation Rules

**Valid:**
- `"level": "ERROR"` → `"critical"` (alias + case normalization)
- `"level": " warn "` → `"warning"` (trim + alias)
- `"timestamp": 1732982400` → valid integer

**Invalid:**
- `"timestamp": "2024-01-15T10:30:00Z"` → Must be integer
- `"timestamp": 1732982400.123` → No decimals
- `"source": {"name": "server"}` → Must be string
- `"level": "severe"` → Unknown level

## Service Monitoring

**Status Topic**: `alerts/service/status` (published every 60s)

```json
{
  "status": "running",
  "timestamp": "2024-01-15T10:30:00Z",
  "stats": {
    "alerts_processed": 42,
    "alerts_routed": 38,
    "errors": 0,
    "uptime_seconds": 3600,
    "loaded_pathways": ["console_alerts", "file_log", "led_sign", "email_critical"],
    "enabled_pathways": ["console_alerts", "file_log", "led_sign"]
  }
}
```

## Documentation Index

| Document | Content |
|----------|---------|
| `CLAUDE.md` | Technical reference (authoritative) |
| `ALERT_PAYLOAD_SPEC.md` | Complete payload specification |
| `HTTP_GATEWAY.md` | HTTP API reference |
| `MQTT_AUTH.md` | MQTT authentication |
| `SNMP_INTEGRATION.md` | SNMP device configuration |
| `SNMP_CLIENT_MAPPING.md` | SNMP user-to-client mapping |
| `ANALYTICS.md` | Analytics engine setup |
| `DATABASE_INTEGRATION.md` | PostgreSQL backend |
| `BETABRITE.md` | LED sign capabilities |
