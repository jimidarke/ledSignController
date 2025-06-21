/**
 * @file MQTTManager.cpp
 * @brief Implementation of MQTT connection and message handling
 * 
 * Contains the implementation of robust MQTT management with exponential
 * backoff, health monitoring, and telemetry publishing.
 */

#include "MQTTManager.h"

// Static instance pointer for callback routing
MQTTManager* MQTTManager::instance = nullptr;

MQTTManager::MQTTManager(WiFiClient* wifi_client, const String& device_id) 
    : wifi_client(wifi_client), device_id(device_id) {
    
    // Initialize connection parameters
    memset(mqtt_server, 0, sizeof(mqtt_server));
    mqtt_port = 1883;
    memset(mqtt_user, 0, sizeof(mqtt_user));
    memset(mqtt_pass, 0, sizeof(mqtt_pass));
    
    // Initialize state variables
    is_configured = false;
    last_attempt_time = 0;
    reconnect_attempts = 0;
    backoff_delay = INITIAL_BACKOFF;
    last_telemetry_time = 0;
    
    // Create MQTT client
    mqtt_client = new PubSubClient(*wifi_client);
    
    // Set static instance for callback routing
    instance = this;
    
    Serial.println("MQTTManager: Initialized");
}

MQTTManager::~MQTTManager() {
    if (mqtt_client) {
        mqtt_client->disconnect();
        delete mqtt_client;
    }
    instance = nullptr;
}

bool MQTTManager::configure(const char* server, uint16_t port, 
                           const char* username, const char* password) {
    
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
    
    // Store credentials (can be empty)
    if (username != NULL) {
        strncpy(mqtt_user, username, sizeof(mqtt_user) - 1);
        mqtt_user[sizeof(mqtt_user) - 1] = '\0';
    }
    
    if (password != NULL) {
        strncpy(mqtt_pass, password, sizeof(mqtt_pass) - 1);
        mqtt_pass[sizeof(mqtt_pass) - 1] = '\0';
    }
    
    is_configured = true;
    
    Serial.print("MQTTManager: Configured - Server: ");
    Serial.print(mqtt_server);
    Serial.print(":");
    Serial.print(mqtt_port);
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
    
    // Attempt connection
    Serial.print("MQTTManager: Attempting connection... ");
    
    String client_id = "LEDSign_" + device_id;
    bool connected = false;
    
    if (strlen(mqtt_user) > 0) {
        connected = mqtt_client->connect(client_id.c_str(), mqtt_user, mqtt_pass);
    } else {
        connected = mqtt_client->connect(client_id.c_str());
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
    
    // Subscribe to device-specific message topic
    String device_topic = "ledSign/" + device_id + "/message";
    bool device_sub = mqtt_client->subscribe(device_topic.c_str());
    
    // Subscribe to general broadcast topic
    bool general_sub = mqtt_client->subscribe("ledSign/message");
    
    if (device_sub && general_sub) {
        Serial.print("MQTTManager: Subscribed to topics: ");
        Serial.print(device_topic);
        Serial.println(" and ledSign/message");
        return true;
    } else {
        Serial.println("MQTTManager: Topic subscription failed");
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