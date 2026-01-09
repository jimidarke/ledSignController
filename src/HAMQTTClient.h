/****************************************************************************************************************************
  HAMQTTClient.h
  Lightweight MQTT client for Home Assistant integration

  Simple MQTT client for local Home Assistant broker:
  - No TLS (local network)
  - No authentication (anonymous)
  - Basic reconnection logic
  - Designed for secondary broker isolation
 *****************************************************************************************************************************/

#ifndef HA_MQTT_CLIENT_H
#define HA_MQTT_CLIENT_H

#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <functional>
#include "defines.h"

/**
 * @brief Lightweight MQTT client for Home Assistant integration
 *
 * This class provides a simple MQTT client specifically designed for
 * local Home Assistant brokers. It differs from MQTTManager in that:
 * - No TLS support (local network assumption)
 * - No authentication (anonymous access)
 * - Simple reconnection (no exponential backoff)
 * - Minimal memory footprint
 */
class HAMQTTClient {
public:
    /**
     * @brief Construct a new HAMQTTClient
     * @param device_id Unique device identifier (MAC-based)
     */
    explicit HAMQTTClient(const String& device_id);

    /**
     * @brief Destructor - cleans up MQTT client
     */
    ~HAMQTTClient();

    // ============ Configuration ============

    /**
     * @brief Configure the MQTT connection parameters
     * @param server MQTT broker hostname or IP
     * @param port MQTT broker port (default 1883)
     * @return true if configuration is valid
     */
    bool configure(const char* server, uint16_t port = HA_MQTT_DEFAULT_PORT);

    /**
     * @brief Set callback for incoming messages
     * @param callback Function to handle messages (topic, payload, length)
     */
    void setMessageCallback(std::function<void(char*, uint8_t*, unsigned int)> callback);

    // ============ Lifecycle ============

    /**
     * @brief Initialize the MQTT client
     * @return true if initialization successful
     */
    bool begin();

    /**
     * @brief Process MQTT events and handle reconnection
     * Call this in the main loop
     */
    void loop();

    // ============ Connection Status ============

    /**
     * @brief Check if connected to broker
     * @return true if connected
     */
    bool isConnected() const;

    /**
     * @brief Check if client is configured
     * @return true if configured with valid server
     */
    bool isConfigured() const { return configured; }

    /**
     * @brief Force a reconnection attempt
     */
    void forceReconnect();

    // ============ MQTT Operations ============

    /**
     * @brief Publish a message
     * @param topic MQTT topic
     * @param message Message payload
     * @param retain Retain flag
     * @return true if published successfully
     */
    bool publish(const char* topic, const char* message, bool retain = false);

    /**
     * @brief Subscribe to a topic
     * @param topic MQTT topic pattern
     * @param qos Quality of service (default 0)
     * @return true if subscribed successfully
     */
    bool subscribe(const char* topic, uint8_t qos = 0);

    /**
     * @brief Get the underlying PubSubClient
     * @return Pointer to PubSubClient (for HADiscovery)
     */
    PubSubClient* getClient() const { return mqtt_client; }

    /**
     * @brief Get the device ID
     * @return Device ID string
     */
    const String& getDeviceId() const { return device_id; }

private:
    // WiFi and MQTT clients
    WiFiClient wifi_client;
    PubSubClient* mqtt_client;

    // Configuration
    String device_id;
    char server[41];
    uint16_t port;
    bool configured;

    // Reconnection state
    unsigned long last_reconnect_attempt;
    int reconnect_attempts;
    static const int MAX_RECONNECT_ATTEMPTS = 5;
    static const unsigned long LONG_RECONNECT_DELAY = 60000;  // 1 minute

    // Static callback routing
    static HAMQTTClient* instance;
    static void staticCallback(char* topic, byte* payload, unsigned int length);
    std::function<void(char*, uint8_t*, unsigned int)> message_callback;

    /**
     * @brief Attempt to connect to the broker
     * @return true if connected
     */
    bool connect();

    /**
     * @brief Get the LWT (availability) topic
     * @return Availability topic string
     */
    String getAvailabilityTopic() const;
};

#endif // HA_MQTT_CLIENT_H
