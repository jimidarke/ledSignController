/**
 * @file MQTTManager.h
 * @brief MQTT connection and message handling for LED Sign Controller
 * 
 * This module manages all MQTT-related functionality including:
 * - Connection management with exponential backoff
 * - Message subscription and publishing
 * - Telemetry data transmission
 * - Connection health monitoring
 * - Graceful error recovery
 * 
 * @author LED Sign Controller Project
 * @version 0.1.4
 * @date 2024
 */

#ifndef MQTT_MANAGER_H
#define MQTT_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <functional>

/**
 * @brief MQTT connection and message management class
 * 
 * Handles all aspects of MQTT communication including connection management,
 * message routing, and telemetry publishing with robust error handling.
 */
class MQTTManager {
private:
    // Connection parameters
    char mqtt_server[40];           ///< MQTT server hostname/IP
    uint16_t mqtt_port;             ///< MQTT server port
    char mqtt_user[32];             ///< MQTT username
    char mqtt_pass[32];             ///< MQTT password
    String device_id;               ///< Unique device identifier
    
    // MQTT client instances
    WiFiClient* wifi_client;        ///< WiFi client for MQTT
    PubSubClient* mqtt_client;      ///< MQTT client instance
    
    // Connection management
    bool is_configured;             ///< Whether MQTT is configured
    unsigned long last_attempt_time; ///< Last connection attempt timestamp
    int reconnect_attempts;         ///< Current reconnection attempt count
    int backoff_delay;              ///< Current backoff delay in ms
    
    // Connection parameters
    static const int MAX_BACKOFF = 60000;      ///< Maximum backoff delay (60s)
    static const int MAX_ATTEMPTS = 10;        ///< Max attempts before long delay
    static const int INITIAL_BACKOFF = 1000;   ///< Initial backoff delay (1s)
    static const unsigned long LONG_DELAY = 300000; ///< Long delay after max attempts (5min)
    
    // Telemetry management
    unsigned long last_telemetry_time; ///< Last telemetry publish timestamp
    static const unsigned long TELEMETRY_INTERVAL = 60000; ///< Telemetry interval (60s)
    
    // Message callback function
    std::function<void(char*, uint8_t*, unsigned int)> message_callback;
    
    /**
     * @brief Internal callback wrapper for PubSubClient
     * @param topic MQTT topic
     * @param payload Message payload
     * @param length Payload length
     */
    static void staticCallback(char* topic, byte* payload, unsigned int length);
    
    /**
     * @brief Reset connection state variables
     */
    void resetConnectionState();
    
public:
    /**
     * @brief Constructor - initializes MQTT manager
     * @param wifi_client Pointer to WiFiClient instance
     * @param device_id Unique device identifier string
     */
    MQTTManager(WiFiClient* wifi_client, const String& device_id);
    
    /**
     * @brief Destructor - cleans up resources
     */
    ~MQTTManager();
    
    /**
     * @brief Configure MQTT connection parameters
     * @param server MQTT server hostname/IP
     * @param port MQTT server port (default: 1883)
     * @param username MQTT username (can be empty)
     * @param password MQTT password (can be empty)
     * @return true if configuration is valid, false otherwise
     */
    bool configure(const char* server, uint16_t port = 1883, 
                   const char* username = "", const char* password = "");
    
    /**
     * @brief Set message callback function
     * @param callback Function to call when message received
     */
    void setMessageCallback(std::function<void(char*, uint8_t*, unsigned int)> callback);
    
    /**
     * @brief Initialize MQTT connection
     * @return true if initialization successful, false otherwise
     */
    bool begin();
    
    /**
     * @brief Main loop function - call regularly to maintain connection
     * Handles reconnection attempts, message processing, and telemetry
     */
    void loop();
    
    /**
     * @brief Check if MQTT client is connected
     * @return true if connected, false otherwise
     */
    bool isConnected() const;
    
    /**
     * @brief Check if MQTT is properly configured
     * @return true if configured, false otherwise
     */
    bool isConfigured() const;
    
    /**
     * @brief Publish a message to specified topic
     * @param topic MQTT topic to publish to
     * @param message Message content
     * @param retain Whether message should be retained
     * @return true if publish successful, false otherwise
     */
    bool publish(const char* topic, const char* message, bool retain = false);
    
    /**
     * @brief Publish telemetry data (RSSI, IP, uptime)
     * Called automatically by loop() at regular intervals
     */
    void publishTelemetry();
    
    /**
     * @brief Subscribe to device-specific and general message topics
     * @return true if subscription successful, false otherwise
     */
    bool subscribeToTopics();
    
    /**
     * @brief Get current connection status as string
     * @return Human-readable connection status
     */
    String getConnectionStatus() const;
    
    /**
     * @brief Get connection statistics
     * @param attempts Output: current reconnection attempts
     * @param backoff Output: current backoff delay
     * @param last_attempt Output: last attempt timestamp
     */
    void getConnectionStats(int* attempts, int* backoff, unsigned long* last_attempt) const;
    
    /**
     * @brief Force reconnection attempt (resets backoff)
     */
    void forceReconnect();
    
    // Static instance pointer for callback routing
    static MQTTManager* instance;
};

#endif // MQTT_MANAGER_H