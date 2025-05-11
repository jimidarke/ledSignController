#include "RestApiManager.h"

RestApiManager::RestApiManager() {
    _server = new AsyncWebServer(80);
}

RestApiManager::~RestApiManager() {
    if (_server) {
        _server->end();
        delete _server;
        _server = nullptr;
    }
}

void RestApiManager::begin(const char* username, const char* password) {
    _username = String(username);
    _password = String(password);
    _deviceId = "ledsign"; // Default value, will be updated later
    
    Serial.println("Setting up REST API on port 80...");
    
    // API endpoint for sending messages to the sign
    AsyncCallbackJsonWebHandler* messageHandler = new AsyncCallbackJsonWebHandler(
        String(API_BASE_PATH) + "/message", 
        [this](AsyncWebServerRequest *request, JsonVariant &json) {
            this->handleMessage(request, json);
        }
    );
    messageHandler->setMethod(HTTP_POST);
    _server->addHandler(messageHandler);
      // API endpoint for retrieving device info
    _server->on((String(API_BASE_PATH) + "/info").c_str(), HTTP_GET, [this](AsyncWebServerRequest *request) {
        this->handleInfo(request);
    });
    
    // Handle not found for API endpoints
    _server->onNotFound([this](AsyncWebServerRequest *request) {
        this->handleNotFound(request);
    });
    
    _server->begin();
    Serial.println("REST API server started");
}

void RestApiManager::stop() {
    if (_server) {
        _server->end();
        Serial.println("REST API server stopped");
    }
}

bool RestApiManager::isAuthenticated(AsyncWebServerRequest *request) {
    if (!request->authenticate(_username.c_str(), _password.c_str())) {
        request->requestAuthentication();
        return false;
    }
    return true;
}

void RestApiManager::setMessageHandler(ApiMessageHandler handler) {
    _messageHandler = handler;
}

void RestApiManager::setDeviceId(const char* deviceId) {
    _deviceId = String(deviceId);
}

void RestApiManager::handleMessage(AsyncWebServerRequest *request, JsonVariant &json) {
    // Check authentication
    if (!isAuthenticated(request)) {
        return;
    }
    
    // Process the message
    JsonObject jsonObj = json.as<JsonObject>();
    if (!jsonObj.isNull()) {
        // Convert to a string to pass to the message handler
        String jsonString;
        serializeJson(jsonObj, jsonString);
        
        Serial.print("REST API received message: ");
        Serial.println(jsonString);
        
        // Process using the same handler as MQTT messages
        if (_messageHandler) {
            _messageHandler(jsonString.c_str());
            
            // Respond with success
            AsyncResponseStream *response = request->beginResponseStream("application/json");
            response->print("{\"status\":\"success\",\"message\":\"Command accepted\"}");
            request->send(response);
        } else {
            request->send(500, "application/json", "{\"status\":\"error\",\"message\":\"Message handler not configured\"}");
        }
    } else {
        request->send(400, "application/json", "{\"status\":\"error\",\"message\":\"Invalid JSON format\"}");
    }
}

void RestApiManager::handleInfo(AsyncWebServerRequest *request) {
    // Check authentication
    if (!isAuthenticated(request)) {
        return;
    }
    
    // Create JSON response with basic info
    AsyncResponseStream *response = request->beginResponseStream("application/json");
    DynamicJsonDocument doc(512);
    
    doc["device_id"] = _deviceId;
    doc["ip"] = WiFi.localIP().toString();
    doc["mac"] = WiFi.macAddress();
    doc["rssi"] = WiFi.RSSI();
    doc["uptime"] = millis() / 1000;
   // doc["firmware"] = currentVersion;
    
    serializeJson(doc, *response);
    request->send(response);
}

void RestApiManager::handleNotFound(AsyncWebServerRequest *request) {
    // For API endpoints, return JSON error
    if (request->url().startsWith(API_BASE_PATH)) {
        request->send(404, "application/json", "{\"status\":\"error\",\"message\":\"Endpoint not found\"}");
    } else {
        // For non-API endpoints, redirect to basic info page
        String message = "<html><head><title>LED Sign Controller</title>";
        message += "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">";
        message += "<style>body{font-family:Arial,sans-serif;margin:20px;}</style></head>";
        message += "<body><h1>LED Sign Controller</h1>";
        message += "<p>Device ID: " + _deviceId + "</p>";
        message += "<p>API available at <code>/api/*</code> endpoints</p>";
        message += "<p><a href='/api/info'>View API Info</a> (requires authentication)</p>";
        message += "</body></html>";
        request->send(200, "text/html", message);
    }
}

void RestApiManager::addEndpoint(const char* uri, WebRequestMethodComposite method, ArRequestHandlerFunction handler) {
    if (_server) {
        _server->on(uri, method, handler);
    }
}
