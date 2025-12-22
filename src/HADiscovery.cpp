/**
 * @file HADiscovery.cpp
 * @brief Implementation of Home Assistant MQTT Discovery
 */

#include "HADiscovery.h"

// Forward declare what we need from defines.h to avoid multiple definition issues
#ifndef FIRMWARE_VERSION
#define FIRMWARE_VERSION "0.2.1"
#endif

#ifndef MQTT_MAX_PACKET_SIZE
#define MQTT_MAX_PACKET_SIZE 2048
#endif

HADiscovery::HADiscovery(PubSubClient* mqtt_client,
                         const String& device_id,
                         const String& device_name,
                         const String& zone_name)
    : mqtt_client(mqtt_client),
      device_id(device_id),
      device_name(device_name),
      zone_name(zone_name) {

    // Create unique ID prefix for all entities
    unique_id_prefix = "ledsign_" + device_id;

    Serial.println("HADiscovery: Initialized");
    Serial.print("HADiscovery: Device ID: ");
    Serial.println(device_id);
    Serial.print("HADiscovery: Unique ID prefix: ");
    Serial.println(unique_id_prefix);
}

void HADiscovery::setMessageCallback(MessageCallback callback) {
    message_callback = callback;
}

void HADiscovery::setEffectCallback(EffectCallback callback) {
    effect_callback = callback;
}

void HADiscovery::setColorCallback(ColorCallback callback) {
    color_callback = callback;
}

void HADiscovery::setClearCallback(ButtonCallback callback) {
    clear_callback = callback;
}

void HADiscovery::setRebootCallback(ButtonCallback callback) {
    reboot_callback = callback;
}

// Topic builders
String HADiscovery::getDiscoveryTopic(const char* component, const char* object_id) const {
    // Format: homeassistant/{component}/{node_id}/{object_id}/config
    return String(HA_DISCOVERY_PREFIX) + "/" + component + "/" +
           unique_id_prefix + "/" + object_id + "/config";
}

String HADiscovery::getStateTopic(const char* entity) const {
    return "ledSign/" + device_id + "/" + entity;
}

String HADiscovery::getCommandTopic(const char* entity) const {
    return "ledSign/" + device_id + "/" + entity + "/set";
}

String HADiscovery::getAvailabilityTopic() const {
    return "ledSign/" + device_id + "/status";
}

String HADiscovery::getLWTTopic() const {
    return getAvailabilityTopic();
}

void HADiscovery::addDeviceInfo(JsonObject& doc) {
    JsonObject device = doc.createNestedObject("device");

    JsonArray identifiers = device.createNestedArray("identifiers");
    identifiers.add(unique_id_prefix);

    device["name"] = device_name + " (" + zone_name + ")";
    device["model"] = "BetaBrite LED Sign Controller";
    device["manufacturer"] = "Darke Tech Corp";
    device["sw_version"] = FIRMWARE_VERSION;
}

bool HADiscovery::publishJson(const char* topic, DynamicJsonDocument& doc, bool retain) {
    String payload;
    serializeJson(doc, payload);

    if (payload.length() > MQTT_MAX_PACKET_SIZE - 100) {
        Serial.print("HADiscovery: Warning - Payload too large for topic: ");
        Serial.println(topic);
        return false;
    }

    bool result = mqtt_client->publish(topic, payload.c_str(), retain);

    if (result) {
        Serial.print("HADiscovery: Published to ");
        Serial.println(topic);
    } else {
        Serial.print("HADiscovery: Failed to publish to ");
        Serial.println(topic);
    }

    return result;
}

bool HADiscovery::publishDiscovery() {
    Serial.println("HADiscovery: Publishing discovery messages...");

    bool success = true;

    // Controls
    success &= publishTextEntity();
    success &= publishEffectSelect();
    success &= publishColorSelect();
    success &= publishClearButton();
    success &= publishRebootButton();

    // Sensors
    success &= publishStatusSensor();
    success &= publishRSSISensor();
    success &= publishUptimeSensor();
    success &= publishIPSensor();
    success &= publishMemorySensor();

    if (success) {
        Serial.println("HADiscovery: All discovery messages published");
    } else {
        Serial.println("HADiscovery: Some discovery messages failed");
    }

    return success;
}

bool HADiscovery::removeDiscovery() {
    Serial.println("HADiscovery: Removing discovery messages...");

    // Publish empty payload to remove each entity
    const char* topics[] = {
        "text/message",
        "select/effect",
        "select/color",
        "button/clear",
        "button/reboot",
        "binary_sensor/status",
        "sensor/rssi",
        "sensor/uptime",
        "sensor/ip",
        "sensor/memory"
    };

    bool success = true;
    for (const char* topic_suffix : topics) {
        String topic = String(HA_DISCOVERY_PREFIX) + "/" + topic_suffix;
        // Parse component and object_id from suffix
        String suffix_str = topic_suffix;
        int slash_pos = suffix_str.indexOf('/');
        String component = suffix_str.substring(0, slash_pos);
        String object_id = suffix_str.substring(slash_pos + 1);

        String full_topic = getDiscoveryTopic(component.c_str(), object_id.c_str());
        success &= mqtt_client->publish(full_topic.c_str(), "", true);
    }

    return success;
}

bool HADiscovery::subscribeToCommands() {
    Serial.println("HADiscovery: Subscribing to command topics...");

    bool success = true;

    // Subscribe to command topics
    String message_cmd = getCommandTopic("message");
    String effect_cmd = getCommandTopic("effect");
    String color_cmd = getCommandTopic("color");
    String clear_cmd = getCommandTopic("clear");
    String reboot_cmd = getCommandTopic("reboot");

    success &= mqtt_client->subscribe(message_cmd.c_str());
    success &= mqtt_client->subscribe(effect_cmd.c_str());
    success &= mqtt_client->subscribe(color_cmd.c_str());
    success &= mqtt_client->subscribe(clear_cmd.c_str());
    success &= mqtt_client->subscribe(reboot_cmd.c_str());

    if (success) {
        Serial.println("HADiscovery: Subscribed to all command topics");
    } else {
        Serial.println("HADiscovery: Some subscriptions failed");
    }

    return success;
}

bool HADiscovery::handleMessage(const char* topic, const uint8_t* payload, unsigned int length) {
    String topic_str = topic;
    String payload_str;
    payload_str.reserve(length + 1);
    for (unsigned int i = 0; i < length; i++) {
        payload_str += (char)payload[i];
    }

    Serial.print("HADiscovery: Received on ");
    Serial.print(topic);
    Serial.print(": ");
    Serial.println(payload_str);

    // Check each command topic
    if (topic_str == getCommandTopic("message")) {
        if (message_callback) {
            message_callback(payload_str);
        }
        return true;
    }

    if (topic_str == getCommandTopic("effect")) {
        if (effect_callback) {
            effect_callback(payload_str);
        }
        // Publish state back
        mqtt_client->publish(getStateTopic("effect").c_str(), payload_str.c_str(), true);
        return true;
    }

    if (topic_str == getCommandTopic("color")) {
        if (color_callback) {
            color_callback(payload_str);
        }
        // Publish state back
        mqtt_client->publish(getStateTopic("color").c_str(), payload_str.c_str(), true);
        return true;
    }

    if (topic_str == getCommandTopic("clear")) {
        if (clear_callback) {
            clear_callback();
        }
        return true;
    }

    if (topic_str == getCommandTopic("reboot")) {
        if (reboot_callback) {
            reboot_callback();
        }
        return true;
    }

    return false; // Not handled
}

void HADiscovery::updateSensors(int rssi, unsigned long uptime, const String& ip, uint32_t free_memory) {
    // Publish sensor values
    mqtt_client->publish(getStateTopic("rssi").c_str(), String(rssi).c_str(), true);
    mqtt_client->publish(getStateTopic("uptime").c_str(), String(uptime).c_str(), true);
    mqtt_client->publish(getStateTopic("ip").c_str(), ip.c_str(), true);
    mqtt_client->publish(getStateTopic("memory").c_str(), String(free_memory).c_str(), true);
}

void HADiscovery::updateAvailability(bool online) {
    mqtt_client->publish(getAvailabilityTopic().c_str(),
                        online ? getLWTOnlinePayload().c_str() : getLWTOfflinePayload().c_str(),
                        true);
}

// Entity publishers

bool HADiscovery::publishTextEntity() {
    DynamicJsonDocument doc(1024);

    doc["name"] = "Message";
    doc["unique_id"] = unique_id_prefix + "_message";
    doc["command_topic"] = getCommandTopic("message");
    doc["icon"] = "mdi:message-text";
    doc["entity_category"] = HA_CATEGORY_CONFIG;
    doc["mode"] = "text";
    doc["min"] = 1;
    doc["max"] = 125;

    // Availability
    doc["availability_topic"] = getAvailabilityTopic();

    JsonObject obj = doc.as<JsonObject>();
    addDeviceInfo(obj);

    return publishJson(getDiscoveryTopic("text", "message").c_str(), doc);
}

bool HADiscovery::publishEffectSelect() {
    DynamicJsonDocument doc(1536);

    doc["name"] = "Display Effect";
    doc["unique_id"] = unique_id_prefix + "_effect";
    doc["command_topic"] = getCommandTopic("effect");
    doc["state_topic"] = getStateTopic("effect");
    doc["icon"] = "mdi:animation";
    doc["entity_category"] = HA_CATEGORY_CONFIG;

    // Effect options (BetaBrite display modes)
    JsonArray options = doc.createNestedArray("options");
    options.add("rotate");
    options.add("hold");
    options.add("flash");
    options.add("scroll");
    options.add("rollup");
    options.add("rolldown");
    options.add("rollleft");
    options.add("rollright");
    options.add("wipeup");
    options.add("wipedown");
    options.add("wipeleft");
    options.add("wiperight");
    options.add("wipein");
    options.add("wipeout");
    options.add("twinkle");
    options.add("sparkle");
    options.add("snow");
    options.add("interlock");
    options.add("switch");
    options.add("spray");
    options.add("starburst");

    doc["availability_topic"] = getAvailabilityTopic();

    JsonObject obj = doc.as<JsonObject>();
    addDeviceInfo(obj);

    return publishJson(getDiscoveryTopic("select", "effect").c_str(), doc);
}

bool HADiscovery::publishColorSelect() {
    DynamicJsonDocument doc(1024);

    doc["name"] = "Display Color";
    doc["unique_id"] = unique_id_prefix + "_color";
    doc["command_topic"] = getCommandTopic("color");
    doc["state_topic"] = getStateTopic("color");
    doc["icon"] = "mdi:palette";
    doc["entity_category"] = HA_CATEGORY_CONFIG;

    // Color options (BetaBrite colors)
    JsonArray options = doc.createNestedArray("options");
    options.add("red");
    options.add("green");
    options.add("amber");
    options.add("dimred");
    options.add("dimgreen");
    options.add("brown");
    options.add("orange");
    options.add("yellow");
    options.add("rainbow1");
    options.add("rainbow2");
    options.add("mix");
    options.add("auto");

    doc["availability_topic"] = getAvailabilityTopic();

    JsonObject obj = doc.as<JsonObject>();
    addDeviceInfo(obj);

    return publishJson(getDiscoveryTopic("select", "color").c_str(), doc);
}

bool HADiscovery::publishClearButton() {
    DynamicJsonDocument doc(512);

    doc["name"] = "Clear Display";
    doc["unique_id"] = unique_id_prefix + "_clear";
    doc["command_topic"] = getCommandTopic("clear");
    doc["payload_press"] = "PRESS";
    doc["icon"] = "mdi:notification-clear-all";
    doc["entity_category"] = HA_CATEGORY_CONFIG;

    doc["availability_topic"] = getAvailabilityTopic();

    JsonObject obj = doc.as<JsonObject>();
    addDeviceInfo(obj);

    return publishJson(getDiscoveryTopic("button", "clear").c_str(), doc);
}

bool HADiscovery::publishRebootButton() {
    DynamicJsonDocument doc(512);

    doc["name"] = "Reboot";
    doc["unique_id"] = unique_id_prefix + "_reboot";
    doc["command_topic"] = getCommandTopic("reboot");
    doc["payload_press"] = "PRESS";
    doc["icon"] = "mdi:restart";
    doc["entity_category"] = HA_CATEGORY_CONFIG;
    doc["device_class"] = "restart";

    doc["availability_topic"] = getAvailabilityTopic();

    JsonObject obj = doc.as<JsonObject>();
    addDeviceInfo(obj);

    return publishJson(getDiscoveryTopic("button", "reboot").c_str(), doc);
}

bool HADiscovery::publishStatusSensor() {
    DynamicJsonDocument doc(512);

    doc["name"] = "Status";
    doc["unique_id"] = unique_id_prefix + "_status";
    doc["state_topic"] = getAvailabilityTopic();
    doc["payload_on"] = "online";
    doc["payload_off"] = "offline";
    doc["device_class"] = "connectivity";
    doc["entity_category"] = HA_CATEGORY_DIAGNOSTIC;

    JsonObject obj = doc.as<JsonObject>();
    addDeviceInfo(obj);

    return publishJson(getDiscoveryTopic("binary_sensor", "status").c_str(), doc);
}

bool HADiscovery::publishRSSISensor() {
    DynamicJsonDocument doc(512);

    doc["name"] = "WiFi Signal";
    doc["unique_id"] = unique_id_prefix + "_rssi";
    doc["state_topic"] = getStateTopic("rssi");
    doc["unit_of_measurement"] = "dBm";
    doc["device_class"] = HA_DEVICE_CLASS_SIGNAL;
    doc["state_class"] = "measurement";
    doc["entity_category"] = HA_CATEGORY_DIAGNOSTIC;
    doc["icon"] = "mdi:wifi";

    doc["availability_topic"] = getAvailabilityTopic();

    JsonObject obj = doc.as<JsonObject>();
    addDeviceInfo(obj);

    return publishJson(getDiscoveryTopic("sensor", "rssi").c_str(), doc);
}

bool HADiscovery::publishUptimeSensor() {
    DynamicJsonDocument doc(512);

    doc["name"] = "Uptime";
    doc["unique_id"] = unique_id_prefix + "_uptime";
    doc["state_topic"] = getStateTopic("uptime");
    doc["unit_of_measurement"] = "s";
    doc["device_class"] = HA_DEVICE_CLASS_DURATION;
    doc["state_class"] = "total_increasing";
    doc["entity_category"] = HA_CATEGORY_DIAGNOSTIC;
    doc["icon"] = "mdi:clock-outline";

    doc["availability_topic"] = getAvailabilityTopic();

    JsonObject obj = doc.as<JsonObject>();
    addDeviceInfo(obj);

    return publishJson(getDiscoveryTopic("sensor", "uptime").c_str(), doc);
}

bool HADiscovery::publishIPSensor() {
    DynamicJsonDocument doc(512);

    doc["name"] = "IP Address";
    doc["unique_id"] = unique_id_prefix + "_ip";
    doc["state_topic"] = getStateTopic("ip");
    doc["entity_category"] = HA_CATEGORY_DIAGNOSTIC;
    doc["icon"] = "mdi:ip-network";

    doc["availability_topic"] = getAvailabilityTopic();

    JsonObject obj = doc.as<JsonObject>();
    addDeviceInfo(obj);

    return publishJson(getDiscoveryTopic("sensor", "ip").c_str(), doc);
}

bool HADiscovery::publishMemorySensor() {
    DynamicJsonDocument doc(512);

    doc["name"] = "Free Memory";
    doc["unique_id"] = unique_id_prefix + "_memory";
    doc["state_topic"] = getStateTopic("memory");
    doc["unit_of_measurement"] = "B";
    doc["device_class"] = HA_DEVICE_CLASS_DATA_SIZE;
    doc["state_class"] = "measurement";
    doc["entity_category"] = HA_CATEGORY_DIAGNOSTIC;
    doc["icon"] = "mdi:memory";

    doc["availability_topic"] = getAvailabilityTopic();

    JsonObject obj = doc.as<JsonObject>();
    addDeviceInfo(obj);

    return publishJson(getDiscoveryTopic("sensor", "memory").c_str(), doc);
}
