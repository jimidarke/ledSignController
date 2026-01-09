/****************************************************************************************************************************
  HAMQTTClient.cpp
  Lightweight MQTT client for Home Assistant integration

  Implementation of simple MQTT client for local HA broker.
 *****************************************************************************************************************************/

#include "HAMQTTClient.h"

// Static instance for callback routing
HAMQTTClient* HAMQTTClient::instance = nullptr;

HAMQTTClient::HAMQTTClient(const String& device_id)
    : device_id(device_id)
    , mqtt_client(nullptr)
    , port(HA_MQTT_DEFAULT_PORT)
    , configured(false)
    , last_reconnect_attempt(0)
    , reconnect_attempts(0)
    , message_callback(nullptr) {

    memset(server, 0, sizeof(server));
    instance = this;

    Serial.println("HAMQTTClient: Initialized for device " + device_id);
}

HAMQTTClient::~HAMQTTClient() {
    if (mqtt_client) {
        if (mqtt_client->connected()) {
            mqtt_client->disconnect();
        }
        delete mqtt_client;
        mqtt_client = nullptr;
    }
    if (instance == this) {
        instance = nullptr;
    }
}

bool HAMQTTClient::configure(const char* server_addr, uint16_t port_num) {
    if (server_addr == nullptr || strlen(server_addr) == 0) {
        Serial.println("HAMQTTClient: No server configured - HA integration disabled");
        configured = false;
        return false;
    }

    if (port_num == 0 || port_num > 65535) {
        Serial.println("HAMQTTClient: Invalid port");
        configured = false;
        return false;
    }

    strncpy(server, server_addr, sizeof(server) - 1);
    server[sizeof(server) - 1] = '\0';
    port = port_num;

    Serial.print("HAMQTTClient: Configured for ");
    Serial.print(server);
    Serial.print(":");
    Serial.println(port);

    configured = true;
    return true;
}

void HAMQTTClient::setMessageCallback(std::function<void(char*, uint8_t*, unsigned int)> callback) {
    message_callback = callback;
}

void HAMQTTClient::staticCallback(char* topic, byte* payload, unsigned int length) {
    if (instance && instance->message_callback) {
        instance->message_callback(topic, payload, length);
    }
}

bool HAMQTTClient::begin() {
    if (!configured) {
        Serial.println("HAMQTTClient: Cannot begin - not configured");
        return false;
    }

    // Create PubSubClient with plain WiFiClient (no TLS)
    mqtt_client = new PubSubClient(wifi_client);

    // Configure server and callback
    mqtt_client->setServer(server, port);
    mqtt_client->setCallback(staticCallback);
    mqtt_client->setKeepAlive(HA_MQTT_KEEPALIVE);

    // Set buffer size for JSON payloads
    mqtt_client->setBufferSize(1024);

    Serial.println("HAMQTTClient: Client initialized (plain MQTT, no auth)");
    return true;
}

void HAMQTTClient::loop() {
    if (!configured || !mqtt_client) {
        return;
    }

    if (mqtt_client->connected()) {
        mqtt_client->loop();
        reconnect_attempts = 0;
        return;
    }

    // Handle reconnection
    unsigned long now = millis();
    unsigned long reconnect_delay = HA_MQTT_RECONNECT_MS;

    // After max attempts, wait longer before retrying
    if (reconnect_attempts >= MAX_RECONNECT_ATTEMPTS) {
        reconnect_delay = LONG_RECONNECT_DELAY;
    }

    if (now - last_reconnect_attempt >= reconnect_delay) {
        last_reconnect_attempt = now;

        if (reconnect_attempts < MAX_RECONNECT_ATTEMPTS) {
            reconnect_attempts++;
            Serial.print("HAMQTTClient: Reconnect attempt ");
            Serial.print(reconnect_attempts);
            Serial.print("/");
            Serial.println(MAX_RECONNECT_ATTEMPTS);
        } else if (reconnect_attempts == MAX_RECONNECT_ATTEMPTS) {
            reconnect_attempts++;
            Serial.println("HAMQTTClient: Max attempts reached, backing off to 1 minute");
        }

        connect();
    }
}

bool HAMQTTClient::connect() {
    if (!mqtt_client) {
        return false;
    }

    // Build client ID: ha-ledsign-{device_id}
    String client_id = "ha-ledsign-" + device_id;

    // Get LWT topic for availability
    String availability_topic = getAvailabilityTopic();

    Serial.print("HAMQTTClient: Connecting to ");
    Serial.print(server);
    Serial.print(":");
    Serial.print(port);
    Serial.print(" as ");
    Serial.println(client_id);

    // Connect with LWT (Last Will and Testament)
    // No username/password (anonymous)
    bool connected = mqtt_client->connect(
        client_id.c_str(),
        nullptr,                           // No username
        nullptr,                           // No password
        availability_topic.c_str(),        // LWT topic
        0,                                 // LWT QoS
        true,                              // LWT retain
        "offline"                          // LWT message
    );

    if (connected) {
        Serial.println("HAMQTTClient: Connected to HA broker");

        // Publish online status
        mqtt_client->publish(availability_topic.c_str(), "online", true);

        reconnect_attempts = 0;
        return true;
    } else {
        Serial.print("HAMQTTClient: Connection failed, rc=");
        Serial.println(mqtt_client->state());
        return false;
    }
}

bool HAMQTTClient::isConnected() const {
    return mqtt_client && mqtt_client->connected();
}

void HAMQTTClient::forceReconnect() {
    if (mqtt_client && mqtt_client->connected()) {
        mqtt_client->disconnect();
    }
    reconnect_attempts = 0;
    last_reconnect_attempt = 0;
}

bool HAMQTTClient::publish(const char* topic, const char* message, bool retain) {
    if (!isConnected()) {
        return false;
    }
    return mqtt_client->publish(topic, message, retain);
}

bool HAMQTTClient::subscribe(const char* topic, uint8_t qos) {
    if (!isConnected()) {
        return false;
    }
    return mqtt_client->subscribe(topic, qos);
}

String HAMQTTClient::getAvailabilityTopic() const {
    return String(HA_TOPIC_PREFIX) + "ledSign/" + device_id + "/status";
}
