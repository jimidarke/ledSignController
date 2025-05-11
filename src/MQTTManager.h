#ifndef MQTT_MANAGER_H
#define MQTT_MANAGER_H

#include "defines.h"
#include <PubSubClient.h>
#include <WiFi.h>
#include <functional>

// Maximum number of connection attempts before waiting
#define MAX_MQTT_CONN_ATTEMPTS 5

// Time to wait between connection attempts in ms
#define MQTT_CONN_RETRY_DELAY 5000

// Time in ms to wait between consecutive reconnect attempts
#define MQTT_RECONNECT_INTERVAL 10000

// Function type for message callback
typedef std::function<void(char*, byte*, unsigned int)> MQTTMessageCallback;

class MQTTManager {
private:
    WiFiClient* _wifiClient;
    PubSubClient* _mqttClient;
    String _clientId;
    String _username;
    String _password;
    String _server;
    int _port;
    String _deviceId;
    String _firmwareVersion;
    unsigned long _lastReconnectAttempt = 0;
    int _connectionAttempts = 0;
    bool _isConfigured = false;
    bool _hassDiscoveryEnabled = HASS_DISCOVERY_ENABLED;
    MQTTMessageCallback _messageCallback;
    
public:
    MQTTManager(WiFiClient* wifiClient);
    ~MQTTManager();
    
    // Configure MQTT connection
    void configure(const char* server, int port, const char* username, const char* password, const char* clientId);
    
    // Set callback handler for incoming messages
    void setCallback(MQTTMessageCallback callback);
    
    // Connect to MQTT broker
    bool connect();
    
    // Check connection and reconnect if needed
    bool checkConnection();
    
    // Subscribe to a topic
    bool subscribe(const char* topic);
    
    // Publish a message to a topic
    bool publish(const char* topic, const char* payload, bool retain = false);
    
    // Process incoming messages
    void loop();
    
    // Is the MQTT client configured?
    bool isConfigured() const { return _isConfigured; }
    
    // Get the client for direct access if needed
    PubSubClient* getClient() { return _mqttClient; }
      // Reset connection attempts counter
    void resetConnectionAttempts() { _connectionAttempts = 0; }
    
    // Home Assistant MQTT discovery methods
    void setDeviceDetails(const char* deviceId, const char* firmwareVersion);
    bool publishHassDiscovery();
    bool publishHassConfig(const char* component, const char* objectId, const char* name, const char* configJson, bool retain = true);
    void removeHassDiscovery();
    
    // Enable/disable Home Assistant discovery
    void enableHassDiscovery(bool enable) { _hassDiscoveryEnabled = enable; }
    bool isHassDiscoveryEnabled() const { return _hassDiscoveryEnabled; }
};

#endif // MQTT_MANAGER_H
