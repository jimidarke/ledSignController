/**
 * @file HADiscovery.h
 * @brief Home Assistant MQTT Discovery for LED Sign Controller
 *
 * Implements Home Assistant's MQTT Discovery protocol to automatically
 * register the LED sign as a device with multiple entities:
 *
 * Controls:
 * - text: manual_message - Send custom messages to display
 * - select: display_effect - Choose animation (rotate, flash, scroll, etc.)
 * - select: color - Choose color (red, green, amber)
 * - button: clear_sign - Clear all messages
 * - button: reboot - Restart device
 *
 * Sensors:
 * - binary_sensor: status - Online/offline (via LWT)
 * - sensor: rssi - WiFi signal strength
 * - sensor: uptime - Seconds since boot
 * - sensor: ip - IP address
 * - sensor: memory - Free heap bytes
 *
 * @see https://www.home-assistant.io/docs/mqtt/discovery/
 * @author LED Sign Controller Project
 * @version 0.2.1
 * @date 2024
 */

#ifndef HA_DISCOVERY_H
#define HA_DISCOVERY_H

#include <Arduino.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

// Discovery topic prefix (Home Assistant default)
#define HA_DISCOVERY_PREFIX       "homeassistant"

// Device class constants
#define HA_DEVICE_CLASS_SIGNAL    "signal_strength"
#define HA_DEVICE_CLASS_DURATION  "duration"
#define HA_DEVICE_CLASS_DATA_SIZE "data_size"

// Entity categories
#define HA_CATEGORY_CONFIG        "config"
#define HA_CATEGORY_DIAGNOSTIC    "diagnostic"

/**
 * @brief Home Assistant MQTT Discovery manager
 *
 * Publishes discovery messages to register the LED sign with Home Assistant.
 * Subscribes to command topics for interactive control.
 */
class HADiscovery {
public:
    /**
     * @brief Callback type for message commands
     * @param message The message text to display
     */
    using MessageCallback = std::function<void(const String& message)>;

    /**
     * @brief Callback type for effect changes
     * @param effect The effect name (rotate, flash, scroll, etc.)
     */
    using EffectCallback = std::function<void(const String& effect)>;

    /**
     * @brief Callback type for color changes
     * @param color The color name (red, green, amber)
     */
    using ColorCallback = std::function<void(const String& color)>;

    /**
     * @brief Callback type for button presses
     */
    using ButtonCallback = std::function<void()>;

    /**
     * @brief Constructor
     * @param mqtt_client Pointer to PubSubClient for publishing
     * @param device_id Unique device identifier (MAC-based)
     * @param device_name Human-readable device name
     * @param zone_name Zone for topic routing
     */
    HADiscovery(PubSubClient* mqtt_client,
                const String& device_id,
                const String& device_name = "LED Sign",
                const String& zone_name = "default");

    /**
     * @brief Set callback for manual message commands
     */
    void setMessageCallback(MessageCallback callback);

    /**
     * @brief Set callback for effect changes
     */
    void setEffectCallback(EffectCallback callback);

    /**
     * @brief Set callback for color changes
     */
    void setColorCallback(ColorCallback callback);

    /**
     * @brief Set callback for clear button
     */
    void setClearCallback(ButtonCallback callback);

    /**
     * @brief Set callback for reboot button
     */
    void setRebootCallback(ButtonCallback callback);

    /**
     * @brief Publish all discovery messages to Home Assistant
     * @return true if all messages published successfully
     */
    bool publishDiscovery();

    /**
     * @brief Remove all discovery messages (unregister from HA)
     * @return true if all messages removed successfully
     */
    bool removeDiscovery();

    /**
     * @brief Subscribe to all command topics
     * @return true if all subscriptions successful
     */
    bool subscribeToCommands();

    /**
     * @brief Handle incoming MQTT message (call from main MQTT callback)
     * @param topic The topic the message was received on
     * @param payload The message payload
     * @param length Payload length
     * @return true if message was handled by HADiscovery
     */
    bool handleMessage(const char* topic, const uint8_t* payload, unsigned int length);

    /**
     * @brief Update sensor values (call periodically)
     * @param rssi WiFi signal strength
     * @param uptime Seconds since boot
     * @param ip IP address string
     * @param free_memory Free heap bytes
     */
    void updateSensors(int rssi, unsigned long uptime, const String& ip, uint32_t free_memory);

    /**
     * @brief Update availability status
     * @param online true if device is online
     */
    void updateAvailability(bool online);

    /**
     * @brief Get the LWT (Last Will and Testament) topic
     * @return LWT topic string for MQTTManager to use
     */
    String getLWTTopic() const;

    /**
     * @brief Get the LWT offline payload
     */
    String getLWTOfflinePayload() const { return "offline"; }

    /**
     * @brief Get the LWT online payload
     */
    String getLWTOnlinePayload() const { return "online"; }

private:
    PubSubClient* mqtt_client;
    String device_id;
    String device_name;
    String zone_name;
    String unique_id_prefix;

    // Callbacks
    MessageCallback message_callback;
    EffectCallback effect_callback;
    ColorCallback color_callback;
    ButtonCallback clear_callback;
    ButtonCallback reboot_callback;

    // Topic builders
    String getDiscoveryTopic(const char* component, const char* object_id) const;
    String getStateTopic(const char* entity) const;
    String getCommandTopic(const char* entity) const;
    String getAvailabilityTopic() const;

    // Discovery message publishers
    bool publishTextEntity();
    bool publishEffectSelect();
    bool publishColorSelect();
    bool publishClearButton();
    bool publishRebootButton();
    bool publishStatusSensor();
    bool publishRSSISensor();
    bool publishUptimeSensor();
    bool publishIPSensor();
    bool publishMemorySensor();

    // Helper to build device info JSON object
    void addDeviceInfo(JsonObject& doc);

    // Helper to publish JSON document
    bool publishJson(const char* topic, DynamicJsonDocument& doc, bool retain = true);
};

#endif // HA_DISCOVERY_H
