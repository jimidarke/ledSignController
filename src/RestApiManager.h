#ifndef REST_API_MANAGER_H
#define REST_API_MANAGER_H

#include <ESPAsyncWebServer.h>
#include <AsyncJson.h>
#include <ArduinoJson.h>
#include "defines.h"

// Default API username and password
#define API_DEFAULT_USERNAME "admin"
#define API_DEFAULT_PASSWORD "ledsign"

// API Base URL
#define API_BASE_PATH "/api"

// Function type for message handler (to reuse MQTT callback)
typedef std::function<void(const char* msg)> ApiMessageHandler;

class RestApiManager {
private:
    AsyncWebServer* _server;
    String _username;
    String _password;
    ApiMessageHandler _messageHandler;
    String _deviceId;
    
    // Check if request is authenticated
    bool isAuthenticated(AsyncWebServerRequest *request);
    
    // API endpoints
    void handleMessage(AsyncWebServerRequest *request, JsonVariant &json);
    void handleInfo(AsyncWebServerRequest *request);
    void handleNotFound(AsyncWebServerRequest *request);
    
public:
    RestApiManager();
    ~RestApiManager();
    
    // Initialize the API server
    void begin(const char* username = API_DEFAULT_USERNAME, const char* password = API_DEFAULT_PASSWORD);
    
    // Set message handler callback
    void setMessageHandler(ApiMessageHandler handler);
    
    // Set device ID for info responses
    void setDeviceId(const char* deviceId);
    
    // Add additional API endpoints
    void addEndpoint(const char* uri, WebRequestMethodComposite method, 
                    ArRequestHandlerFunction handler);
    
    // Stop the server
    void stop();
};

#endif // REST_API_MANAGER_H
