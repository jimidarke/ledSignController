/**
 * @file MQTTManager.cpp
 * @brief Implementation of MQTT connection and message handling
 *
 * Contains the implementation of robust MQTT management with exponential
 * backoff, health monitoring, and telemetry publishing.
 */

#include "MQTTManager.h"
#include <time.h>

// Static instance pointer for callback routing
MQTTManager* MQTTManager::instance = nullptr;

MQTTManager::MQTTManager(WiFiClient* wifi_client, const String& device_id, const String& zone_name)
    : wifi_client(wifi_client), device_id(device_id), zone_name(zone_name) {

    // Initialize connection parameters
    memset(mqtt_server, 0, sizeof(mqtt_server));
    mqtt_port = 42690;  // Default TLS port per ESP32_BETABRITE_IMPLEMENTATION.md
    memset(mqtt_user, 0, sizeof(mqtt_user));
    memset(mqtt_pass, 0, sizeof(mqtt_pass));

    // Initialize TLS parameters
    use_tls = true;  // Default to TLS per documentation
    certificates_loaded = false;
    wifi_client_secure = nullptr;
    ca_cert_data = nullptr;
    client_cert_data = nullptr;
    client_key_data = nullptr;

    // Initialize state variables
    is_configured = false;
    last_attempt_time = 0;
    reconnect_attempts = 0;
    backoff_delay = INITIAL_BACKOFF;
    last_telemetry_time = 0;

    // Create MQTT client (will set actual client in configure())
    mqtt_client = nullptr;

    // Set static instance for callback routing
    instance = this;

    Serial.println("MQTTManager: Initialized");
    Serial.print("MQTTManager: Zone: ");
    Serial.println(zone_name);
    Serial.print("MQTTManager: Device ID: ");
    Serial.println(device_id);
}

MQTTManager::~MQTTManager() {
    if (mqtt_client) {
        mqtt_client->disconnect();
        delete mqtt_client;
    }
    if (wifi_client_secure) {
        delete wifi_client_secure;
    }
    // Free certificate memory
    if (ca_cert_data) {
        free(ca_cert_data);
    }
    if (client_cert_data) {
        free(client_cert_data);
    }
    if (client_key_data) {
        free(client_key_data);
    }
    instance = nullptr;
}

bool MQTTManager::configure(const char* server, uint16_t port,
                           const char* username, const char* password,
                           bool use_tls) {

    // Validate server parameter
    if (server == NULL || strlen(server) == 0) {
        Serial.println("MQTTManager: Error - Server cannot be empty");
        return false;
    }

    // Validate port range
    if (port <= 0 || port > 65535) {
        Serial.println("MQTTManager: Error - Invalid port number");
        return false;
    }

    // Store configuration
    strncpy(mqtt_server, server, sizeof(mqtt_server) - 1);
    mqtt_server[sizeof(mqtt_server) - 1] = '\0';

    mqtt_port = port;
    this->use_tls = use_tls;

    // Store credentials (can be empty)
    if (username != NULL) {
        strncpy(mqtt_user, username, sizeof(mqtt_user) - 1);
        mqtt_user[sizeof(mqtt_user) - 1] = '\0';
    }

    if (password != NULL) {
        strncpy(mqtt_pass, password, sizeof(mqtt_pass) - 1);
        mqtt_pass[sizeof(mqtt_pass) - 1] = '\0';
    }

    // Setup TLS if requested
    if (use_tls) {
        Serial.println("MQTTManager: TLS enabled - loading certificates");

        // Create secure client
        if (!wifi_client_secure) {
            wifi_client_secure = new WiFiClientSecure();
        }

        // Load certificates from LittleFS
        if (loadCertificates()) {
            Serial.println("MQTTManager: Certificates loaded successfully");
            certificates_loaded = true;

            // Create MQTT client with secure client
            if (mqtt_client) {
                delete mqtt_client;
            }
            mqtt_client = new PubSubClient(*wifi_client_secure);
        } else {
            Serial.println("MQTTManager: Warning - Certificate loading failed");
            Serial.println("MQTTManager: Falling back to insecure mode");
            certificates_loaded = false;
            this->use_tls = false;

            // Fallback to basic client
            if (mqtt_client) {
                delete mqtt_client;
            }
            mqtt_client = new PubSubClient(*wifi_client);
        }
    } else {
        Serial.println("MQTTManager: TLS disabled - using basic connection");

        // Create MQTT client with basic client
        if (mqtt_client) {
            delete mqtt_client;
        }
        mqtt_client = new PubSubClient(*wifi_client);
    }

    is_configured = true;

    Serial.print("MQTTManager: Configured - Server: ");
    Serial.print(mqtt_server);
    Serial.print(":");
    Serial.print(mqtt_port);
    Serial.print(", TLS: ");
    Serial.print(this->use_tls ? "YES" : "NO");
    Serial.print(", User: ");
    Serial.println(strlen(mqtt_user) > 0 ? mqtt_user : "(none)");

    return true;
}

void MQTTManager::setMessageCallback(std::function<void(char*, uint8_t*, unsigned int)> callback) {
    message_callback = callback;
}

bool MQTTManager::begin() {
    if (!is_configured) {
        Serial.println("MQTTManager: Error - Not configured");
        return false;
    }
    
    // Configure MQTT client
    mqtt_client->setServer(mqtt_server, mqtt_port);
    mqtt_client->setCallback(staticCallback);
    
    Serial.println("MQTTManager: Ready for connections");
    return true;
}

void MQTTManager::staticCallback(char* topic, byte* payload, unsigned int length) {
    if (instance && instance->message_callback) {
        instance->message_callback(topic, payload, length);
    }
}

void MQTTManager::resetConnectionState() {
    reconnect_attempts = 0;
    backoff_delay = INITIAL_BACKOFF;
    last_attempt_time = 0;
}

String MQTTManager::loadCertificateFile(const char* path) {
    File file = LittleFS.open(path, "r");
    if (!file) {
        Serial.print("MQTTManager: Warning - File not found: ");
        Serial.println(path);
        return "";
    }

    size_t size = file.size();
    if (size == 0) {
        Serial.print("MQTTManager: Warning - Empty file: ");
        Serial.println(path);
        file.close();
        return "";
    }

    String content = file.readString();
    file.close();

    // Trim whitespace and normalize line endings (Windows CRLF -> Unix LF)
    content.trim();
    content.replace("\r\n", "\n");
    content.replace("\r", "\n");

    return content;
}

bool MQTTManager::loadCertificates() {
    // Initialize LittleFS if not already initialized
    // Try mounting WITHOUT auto-format first to preserve uploaded files
    if (!LittleFS.begin(false)) {
        Serial.println("MQTTManager: Error - Failed to mount LittleFS (no auto-format)");
        Serial.println("MQTTManager: This usually means filesystem wasn't uploaded or is corrupted");
        Serial.println("MQTTManager: Attempting format and creating empty filesystem...");

        // Only format if explicitly needed
        if (!LittleFS.begin(true)) {
            Serial.println("MQTTManager: Error - Failed to mount LittleFS even after format");
            Serial.println("MQTTManager: Check partition table configuration");
            return false;
        }
        Serial.println("MQTTManager: LittleFS formatted - please run 'pio run -t uploadfs' and reboot");
        return false;  // Don't continue without certificates
    }

    Serial.println("MQTTManager: LittleFS mounted successfully");

    // Load CA certificate (required for server verification)
    String caCertStr = loadCertificateFile(CERT_PATH_CA);
    if (caCertStr.length() == 0) {
        Serial.println("MQTTManager: Error - CA certificate not found or empty");
        Serial.println("MQTTManager: Please upload certificates: pio run -t uploadfs");
        return false;
    }

    // Allocate heap memory and copy (add 1 for null terminator)
    ca_cert_data = (char*)malloc(caCertStr.length() + 1);
    if (!ca_cert_data) {
        Serial.println("MQTTManager: Error - Failed to allocate memory for CA cert");
        return false;
    }
    strcpy(ca_cert_data, caCertStr.c_str());
    Serial.print("MQTTManager: CA certificate loaded (");
    Serial.print(strlen(ca_cert_data));
    Serial.println(" bytes)");

    // Load client certificate (optional - only needed for mutual TLS)
    String clientCertStr = loadCertificateFile(CERT_PATH_CLIENT_CERT);
    if (clientCertStr.length() > 0) {
        // Mutual TLS mode: CA cert + client cert + private key
        Serial.println("MQTTManager: Client certificate found - mutual TLS mode");

        // Allocate heap memory and copy
        client_cert_data = (char*)malloc(clientCertStr.length() + 1);
        if (!client_cert_data) {
            Serial.println("MQTTManager: Error - Failed to allocate memory for client cert");
            return false;
        }
        strcpy(client_cert_data, clientCertStr.c_str());
        Serial.print("MQTTManager: Client certificate loaded (");
        Serial.print(strlen(client_cert_data));
        Serial.println(" bytes)");

        // Load client private key (required if client cert exists)
        String clientKeyStr = loadCertificateFile(CERT_PATH_CLIENT_KEY);
        if (clientKeyStr.length() == 0) {
            Serial.println("MQTTManager: Error - Client private key not found but cert exists");
            Serial.println("MQTTManager: For mutual TLS, both client.crt and client.key are required");
            return false;
        }

        // Allocate heap memory and copy
        client_key_data = (char*)malloc(clientKeyStr.length() + 1);
        if (!client_key_data) {
            Serial.println("MQTTManager: Error - Failed to allocate memory for private key");
            return false;
        }
        strcpy(client_key_data, clientKeyStr.c_str());
        Serial.print("MQTTManager: Private key loaded (");
        Serial.print(strlen(client_key_data));
        Serial.println(" bytes)");

        Serial.println("MQTTManager: Certificate-based authentication enabled");
    } else {
        // Server-only TLS mode: CA cert only
        Serial.println("MQTTManager: No client certificate - server verification mode");
        Serial.println("MQTTManager: Authentication will use username/password");
        client_cert_data = nullptr;
        client_key_data = nullptr;
    }

    // Configure WiFiClientSecure with certificates
    if (!wifi_client_secure) {
        Serial.println("MQTTManager: Error - Secure client not initialized");
        return false;
    }

    Serial.println("MQTTManager: Configuring WiFiClientSecure...");

    // Set longer handshake timeout (default is 10s, increase to 30s for reliability)
    wifi_client_secure->setHandshakeTimeout(30000);

    // Load CA certificate using Stream method (preferred)
    File caFile = LittleFS.open(CERT_PATH_CA, "r");
    if (caFile && wifi_client_secure->loadCACert(caFile, caFile.size())) {
        Serial.println("MQTTManager: CA certificate configured via Stream");
        caFile.close();
    } else {
        if (caFile) caFile.close();
        wifi_client_secure->setCACert(ca_cert_data);
        Serial.println("MQTTManager: CA certificate configured via setCACert");
    }

    // Load client certificate and key (only for mutual TLS)
    if (client_cert_data != nullptr && client_key_data != nullptr) {
        // Load client certificate
        File certFile = LittleFS.open(CERT_PATH_CLIENT_CERT, "r");
        if (certFile && wifi_client_secure->loadCertificate(certFile, certFile.size())) {
            Serial.println("MQTTManager: Client certificate configured via Stream");
            certFile.close();
        } else {
            if (certFile) certFile.close();
            wifi_client_secure->setCertificate(client_cert_data);
            Serial.println("MQTTManager: Client certificate configured via setCertificate");
        }

        // Load private key
        File keyFile = LittleFS.open(CERT_PATH_CLIENT_KEY, "r");
        if (keyFile && wifi_client_secure->loadPrivateKey(keyFile, keyFile.size())) {
            Serial.println("MQTTManager: Private key configured via Stream");
            keyFile.close();
        } else {
            if (keyFile) keyFile.close();
            wifi_client_secure->setPrivateKey(client_key_data);
            Serial.println("MQTTManager: Private key configured via setPrivateKey");
        }
        Serial.println("MQTTManager: Mutual TLS ready (certificate-based auth)");
    } else {
        Serial.println("MQTTManager: Server verification ready (username/password auth)");
    }

    Serial.println("MQTTManager: All certificates configured successfully");
    return true;
}

void MQTTManager::loop() {
    if (!is_configured) {
        return;
    }
    
    unsigned long current_time = millis();
    
    // Handle MQTT client loop if connected
    if (mqtt_client->connected()) {
        mqtt_client->loop();
        
        // Reset connection state on successful connection
        if (reconnect_attempts > 0) {
            Serial.println("MQTTManager: Connection restored");
            resetConnectionState();
        }
        
        // Publish telemetry at regular intervals
        if (current_time - last_telemetry_time > TELEMETRY_INTERVAL) {
            publishTelemetry();
            last_telemetry_time = current_time;
        }
        
        return;
    }
    
    // Handle reconnection attempts with exponential backoff
    if (current_time - last_attempt_time < backoff_delay) {
        return; // Not time to retry yet
    }
    
    last_attempt_time = current_time;
    
    // Check if we've exceeded maximum attempts
    if (reconnect_attempts >= MAX_ATTEMPTS) {
        Serial.println("MQTTManager: Max reconnection attempts reached, waiting longer...");
        last_attempt_time = current_time + LONG_DELAY;
        resetConnectionState();
        return;
    }
    
    // Check if system time is valid (required for TLS certificate validation)
    if (use_tls && certificates_loaded) {
        time_t now = time(nullptr);
        if (now < 1609459200) {  // January 1, 2021 - if time is before this, NTP hasn't synced
            Serial.println("MQTTManager: Waiting for NTP time sync (required for TLS)...");
            return;  // Don't attempt connection until time is synced
        }
        Serial.print("MQTTManager: System time is valid: ");
        Serial.println(ctime(&now));
    }

    // Attempt MQTT connection
    Serial.print("MQTTManager: Attempting MQTT connection to ");
    Serial.print(mqtt_server);
    Serial.print(":");
    Serial.println(mqtt_port);

    // Client ID format per ESP32_BETABRITE_IMPLEMENTATION.md:
    // esp32-betabrite-{zone}-{mac_address}
    String client_id = "esp32-betabrite-" + zone_name + "-" + device_id;
    bool connected = false;

    // Set buffer size for larger JSON payloads
    mqtt_client->setBufferSize(MQTT_MAX_PACKET_SIZE);

    Serial.print("MQTTManager: Client ID: ");
    Serial.println(client_id);
    Serial.print("MQTTManager: Username: ");
    Serial.println(strlen(mqtt_user) > 0 ? mqtt_user : "(none)");

    if (strlen(mqtt_user) > 0) {
        // Connect with credentials and clean session = false (persistent session)
        connected = mqtt_client->connect(client_id.c_str(), mqtt_user, mqtt_pass,
                                        NULL, 0, false, NULL, !MQTT_CLEAN_SESSION);
    } else {
        // Connect without credentials (certificate-based auth only)
        connected = mqtt_client->connect(client_id.c_str(), NULL, NULL,
                                        NULL, 0, false, NULL, !MQTT_CLEAN_SESSION);
    }
    
    if (connected) {
        Serial.println("connected");
        
        // Subscribe to topics
        if (subscribeToTopics()) {
            Serial.println("MQTTManager: Subscribed to topics successfully");
        } else {
            Serial.println("MQTTManager: Warning - Topic subscription failed");
        }
        
        resetConnectionState();
    } else {
        reconnect_attempts++;
        
        Serial.print("failed, rc=");
        Serial.print(mqtt_client->state());
        Serial.print(" (attempt ");
        Serial.print(reconnect_attempts);
        Serial.print("/");
        Serial.print(MAX_ATTEMPTS);
        Serial.print("), retry in ");
        Serial.print(backoff_delay / 1000);
        Serial.println(" seconds");
        
        // Exponential backoff with jitter
        backoff_delay = min(backoff_delay * 2, MAX_BACKOFF);
        backoff_delay += random(0, 1000); // Add jitter to prevent thundering herd
        
        if (reconnect_attempts >= MAX_ATTEMPTS) {
            Serial.println("MQTTManager: Multiple failures - check server configuration:");
            Serial.print("  Server: ");
            Serial.print(mqtt_server);
            Serial.print(":");
            Serial.println(mqtt_port);
            Serial.print("  Status codes: https://pubsubclient.knolleary.net/api.html#state");
        }
    }
}

bool MQTTManager::isConnected() const {
    return mqtt_client && mqtt_client->connected();
}

bool MQTTManager::isConfigured() const {
    return is_configured;
}

bool MQTTManager::publish(const char* topic, const char* message, bool retain) {
    if (!isConnected()) {
        Serial.println("MQTTManager: Cannot publish - not connected");
        return false;
    }
    
    if (topic == NULL || message == NULL) {
        Serial.println("MQTTManager: Invalid publish parameters");
        return false;
    }
    
    bool result = mqtt_client->publish(topic, message, retain);
    
    if (result) {
        Serial.print("MQTTManager: Published to ");
        Serial.print(topic);
        Serial.print(": ");
        Serial.println(message);
    } else {
        Serial.print("MQTTManager: Publish failed to ");
        Serial.println(topic);
    }
    
    return result;
}

void MQTTManager::publishTelemetry() {
    if (!isConnected()) {
        return;
    }
    
    // Publish RSSI (signal strength)
    String rssi_topic = "ledSign/" + device_id + "/rssi";
    String rssi_value = String(WiFi.RSSI());
    publish(rssi_topic.c_str(), rssi_value.c_str(), true);
    
    // Publish IP address
    String ip_topic = "ledSign/" + device_id + "/ip";
    String ip_value = WiFi.localIP().toString();
    publish(ip_topic.c_str(), ip_value.c_str(), true);
    
    // Publish uptime in seconds
    String uptime_topic = "ledSign/" + device_id + "/uptime";
    String uptime_value = String(millis() / 1000);
    publish(uptime_topic.c_str(), uptime_value.c_str(), true);
    
    // Publish memory statistics
    String memory_topic = "ledSign/" + device_id + "/memory";
    String memory_value = String(ESP.getFreeHeap());
    publish(memory_topic.c_str(), memory_value.c_str(), true);
    
    Serial.print("MQTTManager: Telemetry published - RSSI: ");
    Serial.print(WiFi.RSSI());
    Serial.print(", Free Memory: ");
    Serial.println(ESP.getFreeHeap());
}

bool MQTTManager::subscribeToTopics() {
    if (!isConnected()) {
        return false;
    }

    // Subscribe to zone-specific message topic (per ESP32_BETABRITE_IMPLEMENTATION.md)
    // Format: ledSign/{zone}/message
    String zone_topic = "ledSign/" + zone_name + "/message";
    bool zone_sub = mqtt_client->subscribe(zone_topic.c_str(), MQTT_QOS_LEVEL);

    if (zone_sub) {
        Serial.print("MQTTManager: Subscribed to zone topic: ");
        Serial.println(zone_topic);
        Serial.print("MQTTManager: QoS Level: ");
        Serial.println(MQTT_QOS_LEVEL);
        return true;
    } else {
        Serial.println("MQTTManager: Zone topic subscription failed");
        return false;
    }
}

String MQTTManager::getConnectionStatus() const {
    if (!is_configured) {
        return "Not Configured";
    }
    
    if (isConnected()) {
        return "Connected";
    }
    
    // Return descriptive status based on MQTT client state
    int state = mqtt_client->state();
    switch (state) {
        case MQTT_CONNECTION_TIMEOUT:
            return "Connection Timeout";
        case MQTT_CONNECTION_LOST:
            return "Connection Lost";
        case MQTT_CONNECT_FAILED:
            return "Connect Failed";
        case MQTT_DISCONNECTED:
            return "Disconnected";
        case MQTT_CONNECT_BAD_PROTOCOL:
            return "Bad Protocol";
        case MQTT_CONNECT_BAD_CLIENT_ID:
            return "Bad Client ID";
        case MQTT_CONNECT_UNAVAILABLE:
            return "Server Unavailable";
        case MQTT_CONNECT_BAD_CREDENTIALS:
            return "Bad Credentials";
        case MQTT_CONNECT_UNAUTHORIZED:
            return "Unauthorized";
        default:
            return "Unknown State (" + String(state) + ")";
    }
}

void MQTTManager::getConnectionStats(int* attempts, int* backoff, unsigned long* last_attempt) const {
    if (attempts) *attempts = reconnect_attempts;
    if (backoff) *backoff = backoff_delay;
    if (last_attempt) *last_attempt = last_attempt_time;
}

void MQTTManager::forceReconnect() {
    Serial.println("MQTTManager: Forcing reconnection attempt");
    if (mqtt_client->connected()) {
        mqtt_client->disconnect();
    }
    resetConnectionState();
}