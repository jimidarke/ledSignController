/**
 * @file MQTTManager.h
 * @brief MQTT connection and message handling for LED Sign Controller
 *
 * This module manages all MQTT-related functionality including:
 * - Server-only TLS: CA cert validates broker, username/password authenticates device
 * - Connection management with exponential backoff
 * - Message subscription and publishing
 * - Telemetry data transmission
 * - Connection health monitoring
 * - Graceful error recovery
 *
 * Security Model:
 * - TLS 1.2 encryption (standard port 8883)
 * - Server certificate validation via CA cert (data/certs/ca.crt)
 * - Device authentication via MQTT username/password
 * - Optional mTLS if client.crt and client.key are present
 *
 * @author LED Sign Controller Project
 * @version 0.2.1
 * @date 2024
 */

#ifndef MQTT_MANAGER_H
#define MQTT_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <LittleFS.h>
#include <functional>

// Certificate paths for TLS authentication (from defines.h)
#ifndef CERT_PATH_CA
#define CERT_PATH_CA              "/certs/ca.crt"
#define CERT_PATH_CLIENT_CERT     "/certs/client.crt"
#define CERT_PATH_CLIENT_KEY      "/certs/client.key"
#endif

// MQTT Configuration constants (from defines.h)
#ifndef MQTT_MAX_PACKET_SIZE
#define MQTT_MAX_PACKET_SIZE      2048
#endif
#ifndef MQTT_QOS_LEVEL
#define MQTT_QOS_LEVEL            1
#endif
#ifndef MQTT_CLEAN_SESSION
#define MQTT_CLEAN_SESSION        false
#endif

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
    String device_id;               ///< Unique device identifier (MAC-based)
    String zone_name;               ///< Zone name for topic routing (e.g., "kitchen")

    // TLS/Security parameters
    bool use_tls;                   ///< Whether to use TLS/SSL
    bool certificates_loaded;       ///< Whether certificates loaded successfully
    char* ca_cert_data;             ///< CA certificate storage on heap (must persist for WiFiClientSecure)
    char* client_cert_data;         ///< Client certificate storage on heap (must persist for WiFiClientSecure)
    char* client_key_data;          ///< Private key storage on heap (must persist for WiFiClientSecure)

    // MQTT client instances
    WiFiClient* wifi_client;              ///< Basic WiFi client (fallback)
    WiFiClientSecure* wifi_client_secure; ///< Secure WiFi client for TLS
    PubSubClient* mqtt_client;            ///< MQTT client instance
    
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

    /**
     * @brief Load TLS certificates from SPIFFS
     * @return true if all certificates loaded successfully, false otherwise
     */
    bool loadCertificates();

    /**
     * @brief Load a certificate file from SPIFFS
     * @param path File path in SPIFFS
     * @return Certificate content as String (empty if failed)
     */
    String loadCertificateFile(const char* path);
    
public:
    /**
     * @brief Constructor - initializes MQTT manager
     * @param wifi_client Pointer to WiFiClient instance (for fallback)
     * @param device_id Unique device identifier string (MAC-based)
     * @param zone_name Zone name for topic routing (default: "default")
     */
    MQTTManager(WiFiClient* wifi_client, const String& device_id, const String& zone_name = "default");

    /**
     * @brief Destructor - cleans up resources
     */
    ~MQTTManager();

    /**
     * @brief Configure MQTT connection parameters
     * @param server MQTT server hostname/IP
     * @param port MQTT server port (default: 8883 for TLS)
     * @param username MQTT username (required for server-only TLS auth)
     * @param password MQTT password (required for server-only TLS auth)
     * @param use_tls Whether to use TLS (default: true)
     * @return true if configuration is valid, false otherwise
     */
    bool configure(const char* server, uint16_t port = 8883,
                   const char* username = "", const char* password = "",
                   bool use_tls = true);
    
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