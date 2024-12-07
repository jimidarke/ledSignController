#include "OTAUpdate.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <Update.h>

void checkForUpdates(const char *currentVersion, const char *versionURL, const char *serverURL)
{
    WiFiClient client;
    HTTPClient http;

    Serial.println("Checking for updates...");
    if (http.begin(client, versionURL))
    {
        int httpCode = http.GET();
        if (httpCode == 200)
        {
            String serverVersion = http.getString();
            serverVersion.trim();
            Serial.println("Current version: " + String(currentVersion));
            Serial.println("Server version: " + serverVersion);
            if (serverVersion != currentVersion)
            {
                Serial.println("New version available! Starting update...");
                if (http.begin(client, serverURL))
                {
                    int firmwareHttpCode = http.GET();
                    if (firmwareHttpCode == 200)
                    {
                        int contentLength = http.getSize();
                        if (Update.begin(contentLength))
                        {
                            size_t written = Update.writeStream(http.getStream());
                            if (written == contentLength && Update.end() && Update.isFinished())
                            {
                                Serial.println("Update successfully installed. Rebooting...");
                                ESP.restart();
                            }
                            else
                            {
                                Serial.println("Update failed. Error: " + String(Update.getError()));
                            }
                        }
                        else
                        {
                            Serial.println("Not enough space to start update.");
                        }
                    }
                    else
                    {
                        Serial.println("Failed to download firmware. HTTP code: " + String(firmwareHttpCode));
                    }
                }
            }
            else
            {
                Serial.println("Firmware is up to date.");
            }
        }
        else
        {
            Serial.println("Failed to fetch version file. HTTP code: " + String(httpCode));
        }
        http.end();
    }
    else
    {
        Serial.println("Unable to connect to server.");
    }
}
