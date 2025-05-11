#include "MQTTManager.h"
#include "defines.h"
#include <Arduino.h>

MQTTManager::MQTTManager(WiFiClient* wifiClient) {
    _wifiClient = wifiClient;
    _mqttClient = new PubSubClient(*wifiClient);
}

MQTTManager::~MQTTManager() {
    if (_mqttClient) {
        delete _mqttClient;
    }
}

void MQTTManager::configure(const char* server, int port, const char* username, const char* password, const char* clientId) {
    _server = String(server);
    _port = port;
    _username = String(username);
    _password = String(password);
    _clientId = String(clientId);
    
    // Set MQTT parameters
    _mqttClient->setServer(server, port);
    _mqttClient->setBufferSize(MQTT_MAX_PACKET_SIZE); // Set buffer size from defines.h
    _mqttClient->setKeepAlive(MQTT_KEEPALIVE);        // Set keepalive timeout from defines.h
    _isConfigured = true;
    
    Serial.print("MQTT Configured: Server=");
    Serial.print(server);
    Serial.print(", Port=");
    Serial.print(port);
    Serial.print(", ClientId=");
    Serial.print(clientId);
    Serial.print(", BufferSize=");
    Serial.print(MQTT_MAX_PACKET_SIZE);
    Serial.print(", KeepAlive=");
    Serial.println(MQTT_KEEPALIVE);
}

void MQTTManager::setCallback(MQTTMessageCallback callback) {
    _messageCallback = callback;
    _mqttClient->setCallback([this](char* topic, byte* payload, unsigned int length) {
        if (_messageCallback) {
            _messageCallback(topic, payload, length);
        }
    });
}

bool MQTTManager::connect() {
    if (!_isConfigured) {
        Serial.println("MQTT not configured");
        return false;
    }
    
    if (_mqttClient->connected()) {
        return true;
    }
    
    Serial.print("Attempting MQTT connection...");
    
    // Set up Last Will and Testament if using Home Assistant
    bool result;
    if (_hassDiscoveryEnabled && _deviceId.length() > 0) {
        String availabilityTopic = "ledSign/" + _deviceId + "/availability";
        
        if (_username.length() > 0) {
            result = _mqttClient->connect(
                _clientId.c_str(), 
                _username.c_str(), 
                _password.c_str(),
                availabilityTopic.c_str(),
                0,  // QoS
                true,  // Retain
                "offline"  // Last will message
            );
        } else {
            result = _mqttClient->connect(
                _clientId.c_str(),
                availabilityTopic.c_str(),
                0,  // QoS
                true,  // Retain
                "offline"  // Last will message
            );
        }
    } else {
        // Standard connection without LWT
        if (_username.length() > 0) {
            result = _mqttClient->connect(_clientId.c_str(), _username.c_str(), _password.c_str());
        } else {
            result = _mqttClient->connect(_clientId.c_str());
        }
    }
      if (result) {
        Serial.println("connected");
        _connectionAttempts = 0;
        
        // Publish online availability for Home Assistant
        if (_hassDiscoveryEnabled && _deviceId.length() > 0) {
            publish(("ledSign/" + _deviceId + "/availability").c_str(), "online", true);
        }
    } else {
        _connectionAttempts++;
        Serial.print("failed, rc=");
        Serial.print(_mqttClient->state());
        Serial.print(", attempts=");
        Serial.println(_connectionAttempts);
        
        // If we have too many connection attempts, wait longer
        if (_connectionAttempts >= MAX_MQTT_CONN_ATTEMPTS) {
            Serial.println("Too many failed attempts, waiting longer before retry");
            _lastReconnectAttempt = millis();
        }
    }
    
    return result;
}

bool MQTTManager::checkConnection() {
    if (!_isConfigured) {
        return false;
    }
    
    if (!_mqttClient->connected()) {
        // Check if we need to wait before trying again
        if (_connectionAttempts >= MAX_MQTT_CONN_ATTEMPTS) {
            unsigned long currentMillis = millis();
            if (currentMillis - _lastReconnectAttempt < MQTT_RECONNECT_INTERVAL) {
                return false;
            }
            _lastReconnectAttempt = currentMillis;
        }
        
        return connect();
    }
    
    return true;
}

bool MQTTManager::subscribe(const char* topic) {
    if (!checkConnection()) {
        Serial.println("Cannot subscribe, not connected");
        return false;
    }
    
    bool result = _mqttClient->subscribe(topic);
    if (result) {
        Serial.print("Subscribed to ");
        Serial.println(topic);
    } else {
        Serial.print("Failed to subscribe to ");
        Serial.println(topic);
    }
    
    return result;
}

bool MQTTManager::publish(const char* topic, const char* payload, bool retain) {
    if (!checkConnection()) {
        Serial.println("Cannot publish, not connected");
        return false;
    }
    
    bool result = _mqttClient->publish(topic, payload, retain);
    if (!result) {
        Serial.print("Failed to publish message to ");
        Serial.println(topic);
    }
    
    return result;
}

void MQTTManager::loop() {    
    if (_isConfigured) {
        checkConnection();
        if (_mqttClient->connected()) {
            _mqttClient->loop();
        }
    }
}

void MQTTManager::setDeviceDetails(const char* deviceId, const char* firmwareVersion) {
    _deviceId = String(deviceId);
    _firmwareVersion = String(firmwareVersion);
}

bool MQTTManager::publishHassDiscovery() {
    if (!_hassDiscoveryEnabled) {
        return false;
    }
    
    if (!checkConnection()) {
        Serial.println("Cannot publish discovery, not connected");
        return false;
    }
    
    // Skip if device ID isn't set
    if (_deviceId.length() == 0) {
        Serial.println("Cannot publish discovery, no device ID set");
        return false;
    }
    
    Serial.println("Publishing Home Assistant discovery information...");
    
    // Create device information JSON object (reused for all entities)
    String deviceJson = "\"device\":{";
    deviceJson += "\"identifiers\":[\"" + _deviceId + "\"],";
    deviceJson += "\"name\":\"LED Sign (" + _deviceId + ")\",";
    deviceJson += "\"model\":\"LED Sign Controller\",";
    deviceJson += "\"manufacturer\":\"Darke Tech Corp\",";
    if (_firmwareVersion.length() > 0) {
        deviceJson += "\"sw_version\":\"" + _firmwareVersion + "\",";
    }
    deviceJson += "\"via_device\":\"MQTT\"}";
    
    // Unique node id for this device
    String nodeId = String(HASS_NODE_ID) + "_" + _deviceId;
    
    // ----- MQTT Message Sensor -----
    String sensorConfig = "{";
    sensorConfig += "\"name\":\"LED Sign Message\",";
    sensorConfig += "\"unique_id\":\"" + nodeId + "_message\",";
    sensorConfig += "\"state_topic\":\"ledSign/" + _deviceId + "/state\",";
    sensorConfig += "\"json_attributes_topic\":\"ledSign/" + _deviceId + "/status\",";
    sensorConfig += "\"value_template\":\"{{ value }}\",";
    sensorConfig += deviceJson;
    sensorConfig += "}";
    
    publishHassConfig("sensor", (nodeId + "_message").c_str(), "LED Sign Message", sensorConfig.c_str());
    
    // ----- LED Sign Text Command -----
    String textCommandConfig = "{";
    textCommandConfig += "\"name\":\"LED Sign Display Text\",";
    textCommandConfig += "\"unique_id\":\"" + nodeId + "_display_text\",";
    textCommandConfig += "\"command_topic\":\"ledSign/" + _deviceId + "/message\",";
    textCommandConfig += "\"retain\":false,";
    textCommandConfig += deviceJson;
    textCommandConfig += "}";
    
    publishHassConfig("text", (nodeId + "_display_text").c_str(), "LED Sign Display Text", textCommandConfig.c_str());
    
    // ----- LED Sign JSON Command -----
    String jsonCommandConfig = "{";
    jsonCommandConfig += "\"name\":\"LED Sign JSON Command\",";
    jsonCommandConfig += "\"unique_id\":\"" + nodeId + "_json_command\",";
    jsonCommandConfig += "\"command_topic\":\"ledSign/" + _deviceId + "/json\",";
    jsonCommandConfig += "\"retain\":false,";
    jsonCommandConfig += deviceJson;
    jsonCommandConfig += "}";
    
    publishHassConfig("text", (nodeId + "_json_command").c_str(), "LED Sign JSON Command", jsonCommandConfig.c_str());
    
    // ----- LED Sign as Light -----
    String lightConfig = "{";
    lightConfig += "\"name\":\"LED Sign\",";
    lightConfig += "\"unique_id\":\"" + nodeId + "_light\",";
    lightConfig += "\"schema\":\"json\",";
    lightConfig += "\"brightness\":false,";
    lightConfig += "\"command_topic\":\"ledSign/" + _deviceId + "/json\",";
    lightConfig += "\"state_topic\":\"ledSign/" + _deviceId + "/light_state\",";
    lightConfig += "\"availability_topic\":\"ledSign/" + _deviceId + "/availability\",";
    lightConfig += "\"payload_on\":\"{\\\"type\\\":\\\"normal\\\",\\\"text\\\":\\\"ON\\\",\\\"color\\\":\\\"green\\\"}\",";
    lightConfig += "\"payload_off\":\"{\\\"type\\\":\\\"clear\\\"}\",";
    lightConfig += "\"state_value_template\":\"{{ value_json.state }}\",";
    lightConfig += "\"optimistic\":false,";
    lightConfig += "\"qos\":0,";
    lightConfig += "\"retain\":false,";
    lightConfig += deviceJson;
    lightConfig += "}";
    
    publishHassConfig("light", (nodeId + "_light").c_str(), "LED Sign Light", lightConfig.c_str());
    
    // ----- Network Signal Sensor -----
    String signalConfig = "{";
    signalConfig += "\"name\":\"LED Sign Signal\",";
    signalConfig += "\"unique_id\":\"" + nodeId + "_signal\",";
    signalConfig += "\"state_topic\":\"ledSign/" + _deviceId + "/rssi\",";
    signalConfig += "\"unit_of_measurement\":\"dBm\",";
    signalConfig += "\"device_class\":\"signal_strength\",";
    signalConfig += "\"entity_category\":\"diagnostic\",";
    signalConfig += deviceJson;
    signalConfig += "}";
    
    publishHassConfig("sensor", (nodeId + "_signal").c_str(), "LED Sign Signal", signalConfig.c_str());
    
    // ----- Uptime Sensor -----
    String uptimeConfig = "{";
    uptimeConfig += "\"name\":\"LED Sign Uptime\",";
    uptimeConfig += "\"unique_id\":\"" + nodeId + "_uptime\",";
    uptimeConfig += "\"state_topic\":\"ledSign/" + _deviceId + "/uptime\",";
    uptimeConfig += "\"unit_of_measurement\":\"s\",";
    uptimeConfig += "\"device_class\":\"duration\",";
    uptimeConfig += "\"entity_category\":\"diagnostic\",";
    uptimeConfig += deviceJson;
    uptimeConfig += "}";
    
    publishHassConfig("sensor", (nodeId + "_uptime").c_str(), "LED Sign Uptime", uptimeConfig.c_str());
    
    // ----- Button for clearing the sign -----
    String clearButtonConfig = "{";
    clearButtonConfig += "\"name\":\"LED Sign Clear\",";
    clearButtonConfig += "\"unique_id\":\"" + nodeId + "_clear\",";
    clearButtonConfig += "\"command_topic\":\"ledSign/" + _deviceId + "/json\",";
    clearButtonConfig += "\"payload_press\":\"{\\\"type\\\":\\\"clear\\\"}\",";
    clearButtonConfig += "\"device_class\":\"restart\",";
    clearButtonConfig += "\"entity_category\":\"config\",";
    clearButtonConfig += "\"qos\":0,";
    clearButtonConfig += deviceJson;
    clearButtonConfig += "}";
    
    publishHassConfig("button", (nodeId + "_clear").c_str(), "LED Sign Clear", clearButtonConfig.c_str());
    
    // Publish availability
    publish(("ledSign/" + _deviceId + "/availability").c_str(), "online", true);
    
    // Publish initial light state
    publish(("ledSign/" + _deviceId + "/light_state").c_str(), "{\"state\":\"OFF\"}", true);
    
    Serial.println("Home Assistant discovery completed");
    return true;
}

bool MQTTManager::publishHassConfig(const char* component, const char* objectId, const char* name, const char* configJson, bool retain) {
    if (!_hassDiscoveryEnabled) {
        return false;
    }
    
    String topic = String(HASS_DISCOVERY_PREFIX) + "/" + component + "/" + objectId + "/config";
    
    Serial.print("Publishing HASS config for ");
    Serial.print(name);
    Serial.print(" to ");
    Serial.println(topic);
    
    return publish(topic.c_str(), configJson, retain);
}

void MQTTManager::removeHassDiscovery() {
    if (!_hassDiscoveryEnabled || _deviceId.length() == 0) {
        return;
    }
    
    String nodeId = String(HASS_NODE_ID) + "_" + _deviceId;
    
    // List of components to remove
    struct HassEntity {
        const char* component;
        const char* objectIdSuffix;
    };
    
    const HassEntity entities[] = {
        {"sensor", "_message"},
        {"sensor", "_signal"},
        {"sensor", "_uptime"},
        {"text", "_display_text"},
        {"text", "_json_command"},
        {"light", "_light"},
        {"button", "_clear"}
    };
    
    const int entityCount = sizeof(entities) / sizeof(entities[0]);
    
    // Remove all discovery configurations
    for (int i = 0; i < entityCount; i++) {
        String objectId = nodeId + entities[i].objectIdSuffix;
        String topic = String(HASS_DISCOVERY_PREFIX) + "/" + entities[i].component + "/" + objectId + "/config";
        publish(topic.c_str(), "", true); // Empty payload removes the configuration
    }
    
    // Set availability to offline
    publish(("ledSign/" + _deviceId + "/availability").c_str(), "offline", true);
}
