# Alert Manager Client Integration Guide

**Version 1.0.0**

Quick reference for sending alerts to the Alert Manager system from external clients.

## Quick Start Examples

### Python AlertLogger Usage

```python
from alert_logger import AlertLogger

# Initialize
logger = AlertLogger(
    client_name="my-app",
    username="your-username",
    password="your-password",
    category="application"
)

# Send alerts at different levels
logger.critical("Database connection failed")
logger.warning("API rate limit approaching threshold")
logger.notice("Configuration reloaded successfully")
logger.info("Processed 150 records in 2.3 seconds")
logger.debug("Cache hit for key: user_session_12345")
```

### Manual HTTP Request (cURL)

```bash
# HTTPS (recommended)
curl -X POST https://alert.d-t.pw/alert \
  -u username:password \
  -H "Content-Type: application/json" \
  -d '{
    "timestamp": 1704045600,
    "level": "warning",
    "category": "system",
    "title": "High CPU Usage",
    "message": "CPU usage at 85% for 5 minutes",
    "source": "monitoring-script"
  }'

# HTTP fallback
curl -X POST http://alert.d-t.pw:8080/alert \
  -u username:password \
  -H "Content-Type: application/json" \
  -d '{ ... }'
```

## Connection Endpoints

### HTTPS Gateway (Basic Authentication) - Recommended
- **Host**: `alert.d-t.pw`
- **Port**: `443`
- **Protocol**: TLS 1.3
- **Endpoints**:
  - `POST /alert` - Send alerts (requires auth)
  - `POST /test` - Send test alert (requires auth)
  - `GET /docs` - Documentation (requires auth)
  - `GET /health` - Health check (no auth)
  - `GET /logs` - Log viewer web interface (requires auth)
  - `GET /api/logs/list` - List available log files (requires auth)
  - `GET /api/logs/tail` - Tail log file for streaming (requires auth)
- **Auth**: Basic Auth (username:password) or Bearer Token
- **Use Case**: Web services, simple integrations, one-way alerts, scripted alerts

### HTTP Gateway (Unencrypted Fallback)
- **Host**: `alert.d-t.pw`
- **Port**: `8080`
- **Protocol**: HTTP (unencrypted)
- **Endpoints**: Same as HTTPS gateway above
- **Use Case**: Legacy systems or environments where TLS is not available

### MQTT (Certificate Authentication)
- **Host**: `alert.d-t.pw`
- **Port**: `42690`
- **Protocol**: TLS 1.3 with mutual certificate authentication
- **Topic**: `alerts/input`
- **Use Case**: IoT devices, persistent connections, full pub/sub, high-volume alert streams

### SNMP Gateway (SNMPv3 USM Authentication)
- **Host**: `alert.d-t.pw`
- **Port**: `162` (UDP)
- **Protocol**: SNMPv3 with User-based Security Model (USM)
- **Security Level**: authPriv (authentication + privacy/encryption required)
- **Trap Format**: Standard SNMP v3 traps per RFC 3416
- **Use Case**: Network devices (routers, switches, printers, UPS, servers), hardware monitoring, infrastructure telemetry

## User Types & Access Control

The Alert Manager uses a 3-type user system for authentication and authorization:

| Type | Purpose | Expires | Rate Limit | Clientname | Access |
|------|---------|---------|------------|------------|--------|
| **admin** | Full system access | Never | 1000/hour | null | View all logs, send alerts |
| **node** | Programmatic alert sender | 5 years | 1000/hour | Required | Send alerts (auto-inherit clientname) |
| **client** | Log viewer | 2 years | 100/hour | Required | View only matching client logs |

### Client Name Inheritance

**For node type users**, if your alert payload doesn't include a `client` field (or it's empty), the system automatically inherits the `clientname` from your user profile. This means alerts are automatically logged to `/data/logs/file/{clientname}.{date}.log`.

**Override behavior**: You can specify a different `client` in the payload to route to a different log file.

### Log Viewing Access Control

**Client and node users** can only view log files that match their `clientname`:

| User Type | User Clientname | Can View |
|-----------|-----------------|----------|
| admin | null | ALL log files (ops/, file/, everything) |
| client | prod-web | file/prod-web*.log |
| node | prod-api | file/prod-api*.log |

**Access Rules:**
- `ops/` logs are **admin-only** (operational/system logs)
- `file/` logs require **clientname matching** (alert content logs)
- Admin users see **everything** (all directories and files)

**Web Interface**: Visit `http://alert.d-t.pw:8080/logs` to view logs filtered by your access level.

## Alert Payload Structure

### Alert Levels

The Alert Manager supports **five log levels**:

| Level | Description | Use Case | Aliases |
|-------|-------------|----------|---------|
| **critical** | Emergency conditions requiring immediate attention | System failures, security breaches, critical errors | `error`, `err`, `crit` |
| **warning** | Important conditions that may require investigation | Performance degradation, potential issues, warnings | `warn` |
| **notice** | Normal but significant operational events | Scheduled tasks, reboots, successful completions | *(none)* |
| **info** | General informational messages | Status updates, routine events, confirmations | `inf` |
| **debug** | Debug-level troubleshooting messages | Detailed diagnostic information, verbose logging | `dbg` |

**Normalization:**
- All levels are **case-insensitive** and **whitespace-trimmed** (e.g., `" ERROR "` → `"critical"`)
- Aliases are automatically converted to their standard level (e.g., `"error"` → `"critical"`)

### Alert Categories

The Alert Manager supports **eight alert categories**:

| Category | Description | Use Cases |
|----------|-------------|-----------|
| **security** | Security and access control events | Unauthorized access, intrusion detection, authentication failures, security breaches |
| **system** | System and infrastructure events | Server status, resource usage, service restarts, hardware issues |
| **weather** | Weather and environmental alerts | Severe weather warnings, temperature alerts, environmental conditions |
| **automation** | Home/building automation events | Smart home triggers, automated task completions, sensor events |
| **personal** | Personal notifications and reminders | Calendar events, reminders, personal tasks |
| **network** | Network and connectivity events | Network outages, bandwidth issues, connectivity problems |
| **application** | Application-specific events | App errors, deployment notifications, application status |
| **news** | News and information updates | RSS feeds, news alerts, information broadcasts |

**Normalization:**
- Categories are **case-insensitive** and **whitespace-trimmed** (e.g., `"SECURITY"` → `"security"`)
- Categories are used for routing rules and display filtering

### Field Specifications

#### Required Fields
- **`timestamp`** (integer): Unix epoch seconds (not ISO string, no decimals)
- **`level`** (string): One of: `critical`, `warning`, `notice`, `info`, `debug` (or aliases)
- **`category`** (string): One of: `security`, `system`, `weather`, `automation`, `personal`, `network`, `application`, `news`
- **`title`** (string): Short alert title
- **`message`** (string): Detailed alert message
- **`source`** (string): Source identifier (must be string, not object)

#### Optional Fields
- **`client`** (string): Client name/identifier for filtering and logging
- **`metadata`** (object): Any additional data (use for extra source details, LED zones, etc.)
- **`display_config`** (object): LED sign display configuration (mode, color, effects, etc.)
- **`routing`** (object): Override default routing rules
  - `override` (boolean): Force specific pathways
  - `pathways` (array): Pathway names to use
  - `exclude` (array): Pathways to skip

#### String Normalization
All string fields (`level`, `category`, `client`) are automatically normalized:
- Whitespace trimmed from both ends
- Converted to lowercase
- Aliases applied to `level` field

#### Field Type Validation
- **timestamp**: Must be Unix epoch integer (not ISO 8601 string, no decimals)
- **source**: Must be string (not object)
- **level**: Validated against allowed values (case-insensitive, trimmed, aliases supported)
- **category**: Validated against allowed values (case-insensitive, trimmed)
- **client**: Optional string field (case-insensitive, trimmed)

### HTML Link Support in Messages

The HTTP gateway log viewer (`/logs`) supports **clickable links** in alert messages. You can embed `<a href>` tags in the `message` or `title` fields, and they will render as clickable links in the web interface.

**Security:**
- Only `<a href>` tags are permitted
- Only safe protocols are allowed: `http://`, `https://`, `//`
- All other HTML is escaped for security
- Links automatically open in new tabs with security attributes

**Example Usage:**
```json
{
  "timestamp": 1704045600,
  "level": "notice",
  "category": "application",
  "title": "Image Update",
  "message": "Latest image updated to <a href='https://example.com/image.jpg'>2025-11-12 13:30</a>",
  "source": "image-processor"
}
```

**Rendered Output in Log Viewer:**
```
Nov 14 1:30pm 1704045600 NOTICE [application] image-processor: Image Update - Latest image updated to [2025-11-12 13:30] ← clickable link
```

**Supported Link Formats:**
- Single quotes: `<a href='https://example.com'>Link Text</a>`
- Double quotes: `<a href="https://example.com">Link Text</a>`
- Protocol-relative: `<a href='//example.com/path'>Link Text</a>`

**Note:** This feature only affects the web log viewer display. Links in file logs, email alerts, and LED signs remain as plain text.

### Minimum Required Payload Example
```json
{
  "timestamp": 1704045600,
  "level": "info",
  "category": "system",
  "title": "Alert Title",
  "message": "Alert message content",
  "source": "client-identifier"
}
```

### Full Payload with Options Example
```json
{
  "timestamp": 1704045600,
  "level": "critical",
  "category": "security",
  "title": "Security Alert",
  "message": "Unauthorized access detected",
  "source": "security-system",
  "client": "prod-security-monitor",
  "metadata": {
    "device_id": "motion_01",
    "location": "front_door",
    "ip_address": "192.168.1.100",
    "priority": true,
    "led_zones": ["office", "security_desk"]
  },
  "display_config": {
    "mode": "flash",
    "special_effect": "bomb",
    "color": "red",
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

### LED Sign Display Configuration Options

**display_config fields:**
- `mode`: rotate, hold, flash, scroll, rollup, rolldown, rollright, rollleft, explode, newsflash, welcome, compressed, etc.
- `color`: red, green, amber, rainbow1, rainbow2, autocolor, color_mix, etc.
- `special_effect`: twinkle, sparkle, snow, starburst, bomb, welcome, cyclecolors, etc.
- `character_set`: 5high, 7high, 10high, 7shadow, 7highfancy, 7widestroke, etc.
- `speed`: 1-5 (slowest to fastest)
- `priority`: boolean (interrupt normal display rotation)
- `duration`: seconds (for priority messages)

**LED zone targeting** uses `metadata.led_zones` array to specify which zones should display the alert.

## Validation Summary

**Valid Examples:**
- `"level": "error"` → `"critical"`
- `"level": "ERROR"` → `"critical"`
- `"level": " warn "` → `"warning"`
- `"category": "SYSTEM"` → `"system"`
- `"client": " PROD-01 "` → `"prod-01"`
- `"timestamp": 1704045600` → valid (integer)

**Invalid Examples:**
- `"timestamp": "2024-01-15T10:30:00Z"` → Must be integer
- `"timestamp": 1704045600.123` → No decimals
- `"level": "severe"` → Not a valid level
- `"source": {"name": "server"}` → Must be string, not object

## User Management

User accounts are managed by administrator using `tools/http-auth-manager.sh`:
- `create-user <username> <password> <type> [clientname]` - Create user account
- `set-clientname <username> <clientname>` - Update user's clientname
- `delete-user <username>` - Delete user account
- `list-users` - List all user accounts

## Certificate Management (MQTT)

MQTT certificates are managed by administrator using `tools/cert-manager.sh`:
- `client <name> <type>` - Generate client certificate bundle
- Receive `{name}-bundle.tar.gz` containing: `ca.crt`, `client.crt`, `client.key`

## Service Monitoring Topics

- `alerts/input` - Publish alerts to this topic
- `alerts/service/status` - Service health and statistics (published every 60 seconds)
- `ledSign/{zone}/message` - LED sign messages for specific zones

## SNMP Trap Configuration

### SNMPv3 User Requirements

SNMP traps must be sent using SNMPv3 with authPriv security level:

**Required Credentials:**
- **Username**: Configured SNMPv3 user (contact administrator)
- **Authentication Protocol**: SHA, SHA-256, or stronger
- **Authentication Key**: Minimum 8 characters (16+ recommended)
- **Privacy Protocol**: AES-128 or stronger
- **Privacy Key**: Minimum 8 characters (16+ recommended)

### Device Configuration Example

**Cisco Switch/Router:**
```
snmp-server group v3group v3 priv
snmp-server user monitor v3group v3 auth sha darketechsnmp priv aes 128 dawntodarke
snmp-server host alert.d-t.pw version 3 priv monitor udp-port 162
snmp-server enable traps snmp linkdown linkup coldstart
```

**Linux Server (Net-SNMP):**
```bash
# /etc/snmp/snmpd.conf
createUser monitor SHA darketechsnmp AES dawntodarke
rouser monitor priv
trapsink alert.d-t.pw:162 monitor
```

### SNMP Trap to Alert Mapping

SNMP traps are automatically transformed into Alert Manager JSON payloads:

**Input (SNMP Trap):**
- Trap OID: `1.3.6.1.6.3.1.1.5.3` (linkDown)
- Varbinds:
  - `IF-MIB::ifIndex.2` = `2`
  - `IF-MIB::ifDescr.2` = `"eth0"`

**Output (Alert JSON):**
```json
{
  "timestamp": 1704045600,
  "level": "warning",
  "category": "network",
  "title": "Network Link Down",
  "message": "Interface eth0 (index 2) is down on 192.168.1.100",
  "source": "snmp/192.168.1.100",
  "metadata": {
    "snmp_trap_oid": "1.3.6.1.6.3.1.1.5.3",
    "snmp_trap_oid_symbolic": "IF-MIB::linkDown",
    "snmp_agent_addr": "192.168.1.100",
    "snmp_uptime": 123456789,
    "varbinds": [
      {"oid": "1.3.6.1.2.1.2.2.1.1.2", "symbolic": "IF-MIB::ifIndex.2", "value": "2"},
      {"oid": "1.3.6.1.2.1.2.2.1.2.2", "symbolic": "IF-MIB::ifDescr.2", "value": "eth0"}
    ]
  }
}
```

### Testing SNMP Traps

Use `snmptrap` command to test connectivity:

```bash
snmptrap -v 3 -l authPriv \
  -u monitor \
  -a SHA -A darketechsnmp \
  -x AES -X dawntodarke \
  alert.d-t.pw:162 \
  '' \
  1.3.6.1.6.3.1.1.5.3 \
  ifIndex i 2 \
  ifDescr s "eth0"
```

### Discovery Mode

The SNMP Gateway supports a **discovery mode** to capture and analyze all incoming traps for a period of time. This helps you understand what data your devices are sending and build custom OID mappings.

**Use Cases:**
- Identify available trap types from your devices
- Discover numeric metrics for threshold monitoring
- Understand varbind structure before creating mappings
- Capture real-world trap examples for analysis

Contact your administrator to enable discovery mode temporarily.

## Additional Resources

- **SNMP_INTEGRATION.md** - SNMP device configuration and integration guide
- **SNMP_CLIENT_MAPPING.md** - Client mapping and access control for SNMP
- **SNMPV3_SPECIFICATION.md** - Technical specification for SNMPv3 protocol
- **ALERT_PAYLOAD_SPEC.md** - Complete payload specification with validation rules
- **BETABRITE.md** - Complete LED sign capabilities
- **HTTP_GATEWAY.md** - HTTP API reference
- **AUTH.md** - Authentication and certificate management

---

## Python AlertLogger Implementation

### Features
- **Automatic source tracking**: Uses `inspect` module to capture function name and file:line
- **Graceful degradation**: Falls back to console logging on HTTP failures with backoff logic (5 consecutive failures)
- **Debug strategy**: Debug messages written to local file only (`tmp/debug.log`) with 10MB rotation, never sent to HTTP gateway
- **Configuration**: All settings via constructor, supports enable/disable toggle, configurable timeouts

### Alert Level Usage

| Level | Usage | Example |
|-------|-------|---------|
| **critical** | Unexpected failures, unhandled exceptions | `"Unclean exit detected from previous run"` |
| **warning** | Expected failures impacting operations | `"Cannot upload file - Connection timeout"` |
| **notice** | Operational events, state changes | `"Feature disabled by configuration"` |
| **info** | Completion summaries, counts | `"Processed 5 items successfully"` |
| **debug** | Local troubleshooting only (not sent to HTTP) | `"Processing frame 15 with dimensions 2560x1440"` |

### Best Practices
- Use **node type user** for programmatic clients (higher rate limits, client name auto-inheritance, 5-year expiration)
- Store credentials in configuration (not hardcoded), keep out of version control
- Set reasonable HTTP timeout (5 seconds default) to prevent hanging
- Always log to console (stdout) for real-time visibility during testing and cron job output capture
- Track consecutive failures and disable HTTP after threshold to prevent log spam

### AlertLogger Class

```python
import requests
import inspect
import time
import os
from datetime import datetime
from typing import Optional, Dict, Any


class AlertLogger:
    VALID_LEVELS = {'critical', 'warning', 'notice', 'info', 'debug'}
    LEVEL_ALIASES = {'error': 'critical', 'err': 'critical', 'crit': 'critical',
                     'warn': 'warning', 'inf': 'info', 'dbg': 'debug'}
    VALID_CATEGORIES = {'security', 'system', 'weather', 'automation',
                        'personal', 'network', 'application', 'news'}

    def __init__(
        self,
        client_name: str,
        username: str,
        password: str,
        host: str = 'alert.d-t.pw',
        port: int = 443,
        use_https: bool = True,
        category: str = 'application',
        enabled: bool = True,
        timeout: int = 5,
        fallback_to_console: bool = True,
        debug_log_enabled: bool = True,
        debug_log_path: str = 'tmp/debug.log',
        debug_log_max_size_mb: int = 10,
        max_failures_before_disable: int = 5
    ):
        if not client_name or not isinstance(client_name, str):
            raise ValueError("client_name is required and must be a non-empty string")
        if not username or not isinstance(username, str):
            raise ValueError("username is required and must be a non-empty string")
        if not password or not isinstance(password, str):
            raise ValueError("password is required and must be a non-empty string")

        category_normalized = category.strip().lower()
        if category_normalized not in self.VALID_CATEGORIES:
            raise ValueError(f"Invalid category: {category}. Must be one of {self.VALID_CATEGORIES}")

        self.client = client_name.strip().lower()
        self.username = username
        self.password = password
        self.host = host
        self.port = port
        self.use_https = use_https
        self.category = category_normalized
        self.enabled = enabled
        self.timeout = timeout
        self.fallback_to_console = fallback_to_console
        self.debug_log_enabled = debug_log_enabled
        self.debug_log_max_size_mb = debug_log_max_size_mb

        protocol = "https" if use_https else "http"
        self.endpoint = f"{protocol}://{self.host}:{self.port}/alert"

        if os.path.isabs(debug_log_path):
            self.debug_log_path = debug_log_path
        else:
            module_dir = os.path.dirname(os.path.abspath(__file__))
            project_root = os.path.dirname(module_dir)
            self.debug_log_path = os.path.join(project_root, debug_log_path)

        self.consecutive_failures = 0
        self.max_failures_before_disable = max_failures_before_disable

    def _get_caller_info(self) -> tuple[str, str]:
        frame = inspect.currentframe()
        try:
            caller_frame = frame.f_back.f_back.f_back
            function_name = caller_frame.f_code.co_name
            filename = os.path.basename(caller_frame.f_code.co_filename)
            line_number = caller_frame.f_lineno
            return function_name, f"{filename}:{line_number}"
        finally:
            del frame

    def _normalize_level(self, level: str) -> str:
        normalized = level.strip().lower()
        if normalized in self.LEVEL_ALIASES:
            normalized = self.LEVEL_ALIASES[normalized]
        if normalized not in self.VALID_LEVELS:
            raise ValueError(f"Invalid level: {level}. Must be one of {self.VALID_LEVELS} or aliases {list(self.LEVEL_ALIASES.keys())}")
        return normalized

    def _send_alert(self, level: str, message: str, title_override: Optional[str] = None) -> bool:
        level = self._normalize_level(level)
        function_name, source = self._get_caller_info()
        title = title_override if title_override else function_name

        timestamp = datetime.now().strftime('%Y-%m-%d %H:%M:%S')
        print(f"[{timestamp}] [{level.upper()}] [{title}] {message}")

        payload: Dict[str, Any] = {
            "timestamp": int(time.time()),
            "level": level,
            "category": self.category,
            "title": title,
            "message": message,
            "source": source,
            "client": self.client
        }

        if self.enabled and self.consecutive_failures < self.max_failures_before_disable:
            try:
                response = requests.post(
                    self.endpoint,
                    json=payload,
                    auth=(self.username, self.password),
                    timeout=self.timeout
                )

                if response.status_code in (200, 201, 202):
                    self.consecutive_failures = 0
                    return True
                else:
                    self.consecutive_failures += 1
                    if self.fallback_to_console:
                        self._log_error(level, title, message, source, f"HTTP {response.status_code}")
                    return False

            except requests.exceptions.RequestException as e:
                self.consecutive_failures += 1
                if self.fallback_to_console:
                    self._log_error(level, title, message, source, str(e))
                return False
        else:
            if self.fallback_to_console and self.consecutive_failures >= self.max_failures_before_disable:
                print(f"[ALERT MANAGER DISABLED] Too many consecutive failures ({self.consecutive_failures})")
            return False

    def _log_error(self, level: str, title: str, message: str, source: str, error: str):
        timestamp = datetime.now().strftime('%Y-%m-%d %H:%M:%S')
        print(f"[{timestamp}] [ALERT ERROR] [{level.upper()}] [{title}] {message} "
              f"(source: {source}) [Error: {error}]")

    def _debug_file_log(self, level: str, title: str, message: str, source: str):
        timestamp = datetime.now().strftime('%Y-%m-%d %H:%M:%S')
        log_message = f"[{timestamp}] [{level.upper()}] [{title}] {message} (source: {source})\n"

        try:
            log_dir = os.path.dirname(self.debug_log_path)
            if log_dir and not os.path.exists(log_dir):
                os.makedirs(log_dir, exist_ok=True)

            if os.path.exists(self.debug_log_path):
                file_size_mb = os.path.getsize(self.debug_log_path) / (1024 * 1024)
                if file_size_mb > self.debug_log_max_size_mb:
                    old_log = self.debug_log_path + '.old'
                    if os.path.exists(old_log):
                        os.remove(old_log)
                    os.rename(self.debug_log_path, old_log)

            with open(self.debug_log_path, 'a') as f:
                f.write(log_message)

        except Exception as e:
            print(f"[DEBUG LOG ERROR] Failed to write to {self.debug_log_path}: {e}")
            print(log_message.rstrip())

    def critical(self, message: str, title: Optional[str] = None) -> bool:
        return self._send_alert("critical", message, title)

    def warning(self, message: str, title: Optional[str] = None) -> bool:
        return self._send_alert("warning", message, title)

    def notice(self, message: str, title: Optional[str] = None) -> bool:
        return self._send_alert("notice", message, title)

    def info(self, message: str, title: Optional[str] = None) -> bool:
        return self._send_alert("info", message, title)

    def debug(self, message: str, title: Optional[str] = None):
        if not self.debug_log_enabled:
            return

        function_name, source = self._get_caller_info()
        title = title if title else function_name

        timestamp = datetime.now().strftime('%Y-%m-%d %H:%M:%S')
        print(f"[{timestamp}] [DEBUG] [{title}] {message}")

        self._debug_file_log("debug", title, message, source)
```
