/**
 * GitHubOTA.cpp
 *
 * Implementation of GitHub-based OTA update system
 */

#include "GitHubOTA.h"
#include <mbedtls/md.h>

// Constructor
GitHubOTA::GitHubOTA(const char* repoOwner, const char* repoName, BETABRITE* sign)
    : _repoOwner(repoOwner),
      _repoName(repoName),
      _sign(sign),
      _checkInterval(DEFAULT_CHECK_INTERVAL),
      _lastCheckTime(0),
      _updateAvailable(false),
      _autoUpdateEnabled(true),
      _firmwareSize(0) {
}

// Initialize the OTA system
bool GitHubOTA::begin(const char* currentVersion) {
    if (!currentVersion || strlen(currentVersion) == 0) {
        setStatus("ERROR: Invalid current version");
        Serial.println("GitHubOTA: Invalid current version provided");
        return false;
    }

    _currentVersion = String(currentVersion);

    // Remove 'v' prefix if present (e.g., "v0.2.0" -> "0.2.0")
    if (_currentVersion.startsWith("v") || _currentVersion.startsWith("V")) {
        _currentVersion = _currentVersion.substring(1);
    }

    Serial.printf("GitHubOTA: Initialized for %s/%s, current version: %s\n",
                  _repoOwner, _repoName, _currentVersion.c_str());

    setStatus("OTA ready");
    return true;
}

// Set GitHub token
void GitHubOTA::setGitHubToken(const char* token) {
    if (token && strlen(token) > 0) {
        _githubToken = String(token);
        Serial.println("GitHubOTA: GitHub token configured");
    } else {
        _githubToken = "";
        Serial.println("GitHubOTA: No GitHub token (public repo mode)");
    }
}

// Set check interval
void GitHubOTA::setCheckInterval(unsigned long intervalMs) {
    _checkInterval = intervalMs;
    Serial.printf("GitHubOTA: Check interval set to %lu ms (%.1f hours)\n",
                  intervalMs, intervalMs / 3600000.0);
}

// Enable/disable auto-update
void GitHubOTA::setAutoUpdate(bool enabled) {
    _autoUpdateEnabled = enabled;
    Serial.printf("GitHubOTA: Auto-update %s\n", enabled ? "enabled" : "disabled");
}

// Main loop function - call from sketch loop()
void GitHubOTA::loop() {
    if (!_autoUpdateEnabled) {
        return;
    }

    unsigned long now = millis();

    // Handle rollover
    if (now < _lastCheckTime) {
        _lastCheckTime = 0;
    }

    // Check if it's time for a periodic update check
    if (now - _lastCheckTime >= _checkInterval) {
        Serial.println("GitHubOTA: Periodic update check triggered");

        if (checkForUpdate()) {
            if (_updateAvailable) {
                Serial.printf("GitHubOTA: Update available: %s -> %s\n",
                              _currentVersion.c_str(), _latestVersion.c_str());
                performUpdate();  // This will reboot if successful
            }
        }

        _lastCheckTime = now;
    }
}

// Check for updates (manual trigger)
bool GitHubOTA::checkForUpdate() {
    setStatus("Checking for updates...");
    displayMessage("CHECKING FOR UPDATES");

    if (!fetchLatestRelease()) {
        setStatus("Update check failed");
        displayMessage("UPDATE CHECK FAILED");
        delay(3000);
        return false;
    }

    return true;
}

// Fetch latest release from GitHub API
bool GitHubOTA::fetchLatestRelease() {
    WiFiClientSecure client;
    HTTPClient https;

    // Use Arduino's built-in CA bundle
    client.setCACert(NULL);  // Use internal CA bundle
    client.setInsecure();    // For testing - should use proper cert validation in production

    String url = String("https://") + GITHUB_API_HOST + "/repos/" +
                 _repoOwner + "/" + _repoName + "/releases/latest";

    Serial.printf("GitHubOTA: Fetching %s\n", url.c_str());

    if (!https.begin(client, url)) {
        Serial.println("GitHubOTA: Failed to begin HTTPS connection");
        return false;
    }

    // Set headers
    https.addHeader("Accept", "application/vnd.github.v3+json");
    https.addHeader("User-Agent", "ESP32-GitHubOTA");

    if (_githubToken.length() > 0) {
        https.addHeader("Authorization", "token " + _githubToken);
        Serial.println("GitHubOTA: Using GitHub token for authentication");
    }

    int httpCode = https.GET();

    if (httpCode != HTTP_CODE_OK) {
        Serial.printf("GitHubOTA: HTTP error %d\n", httpCode);
        if (httpCode == 404) {
            Serial.println("GitHubOTA: Repository or release not found");
        } else if (httpCode == 401 || httpCode == 403) {
            Serial.println("GitHubOTA: Authentication failed - check token");
        }
        https.end();
        return false;
    }

    // Parse JSON response
    String payload = https.getString();
    https.end();

    DynamicJsonDocument doc(8192);  // Large buffer for release info
    DeserializationError error = deserializeJson(doc, payload);

    if (error) {
        Serial.printf("GitHubOTA: JSON parse error: %s\n", error.c_str());
        return false;
    }

    // Extract release information
    String tagName = doc["tag_name"].as<String>();
    if (tagName.length() == 0) {
        Serial.println("GitHubOTA: No tag_name in release");
        return false;
    }

    // Remove 'v' prefix from tag
    _latestVersion = tagName;
    if (_latestVersion.startsWith("v") || _latestVersion.startsWith("V")) {
        _latestVersion = _latestVersion.substring(1);
    }

    Serial.printf("GitHubOTA: Latest release: %s\n", _latestVersion.c_str());

    // Compare versions
    int comparison = compareVersions(_latestVersion, _currentVersion);

    if (comparison <= 0) {
        Serial.println("GitHubOTA: Already on latest version");
        _updateAvailable = false;
        setStatus("Up to date");
        return true;
    }

    // Find firmware.bin asset
    JsonArray assets = doc["assets"];
    bool foundFirmware = false;
    bool foundChecksum = false;

    for (JsonObject asset : assets) {
        String name = asset["name"].as<String>();

        if (name == "firmware.bin") {
            _firmwareUrl = asset["browser_download_url"].as<String>();
            _firmwareSize = asset["size"].as<size_t>();
            foundFirmware = true;
            Serial.printf("GitHubOTA: Found firmware.bin (%s)\n",
                          formatBytes(_firmwareSize).c_str());
        } else if (name == "firmware.sha256") {
            // Store checksum URL for later
            String checksumUrl = asset["browser_download_url"].as<String>();
            _firmwareChecksum = downloadChecksum(checksumUrl);
            if (_firmwareChecksum.length() > 0) {
                foundChecksum = true;
                Serial.printf("GitHubOTA: Found checksum: %s\n",
                              _firmwareChecksum.substring(0, 16).c_str());
            }
        }
    }

    if (!foundFirmware) {
        Serial.println("GitHubOTA: No firmware.bin in release assets");
        return false;
    }

    if (!foundChecksum) {
        Serial.println("GitHubOTA: Warning - no checksum file found");
        // Continue without checksum verification (not recommended for production)
    }

    _updateAvailable = true;
    setStatus("Update available: " + _latestVersion);
    return true;
}

// Download checksum file
String GitHubOTA::downloadChecksum(const String& checksumUrl) {
    WiFiClientSecure client;
    HTTPClient https;

    client.setInsecure();

    if (!https.begin(client, checksumUrl)) {
        Serial.println("GitHubOTA: Failed to fetch checksum");
        return "";
    }

    if (_githubToken.length() > 0) {
        https.addHeader("Authorization", "token " + _githubToken);
    }

    int httpCode = https.GET();
    if (httpCode != HTTP_CODE_OK) {
        Serial.printf("GitHubOTA: Checksum download failed: %d\n", httpCode);
        https.end();
        return "";
    }

    String checksum = https.getString();
    https.end();

    // Parse checksum (format: "hash  filename" or just "hash")
    checksum.trim();
    int spaceIndex = checksum.indexOf(' ');
    if (spaceIndex > 0) {
        checksum = checksum.substring(0, spaceIndex);
    }

    return checksum;
}

// Perform the update
bool GitHubOTA::performUpdate() {
    if (!_updateAvailable) {
        Serial.println("GitHubOTA: No update available");
        return false;
    }

    Serial.printf("GitHubOTA: Starting update to version %s\n", _latestVersion.c_str());
    displayMessage("UPDATING FIRMWARE " + _latestVersion);
    delay(2000);

    bool success = downloadAndFlash(_firmwareUrl);

    if (success) {
        displayMessage("UPDATE COMPLETE - REBOOTING");
        delay(3000);
        Serial.println("GitHubOTA: Update successful, rebooting...");
        ESP.restart();
        return true;  // Won't reach here
    } else {
        setStatus("Update failed");
        displayMessage("UPDATE FAILED");
        delay(5000);
        return false;
    }
}

// Download and flash firmware
bool GitHubOTA::downloadAndFlash(const String& url) {
    WiFiClientSecure client;
    HTTPClient https;

    client.setInsecure();

    Serial.printf("GitHubOTA: Downloading firmware from %s\n", url.c_str());
    displayMessage("DOWNLOADING");

    if (!https.begin(client, url)) {
        Serial.println("GitHubOTA: Failed to begin download");
        return false;
    }

    if (_githubToken.length() > 0) {
        https.addHeader("Authorization", "token " + _githubToken);
    }

    https.setTimeout(UPDATE_TIMEOUT_MS);

    int httpCode = https.GET();
    if (httpCode != HTTP_CODE_OK) {
        Serial.printf("GitHubOTA: Download failed: %d\n", httpCode);
        https.end();
        return false;
    }

    int contentLength = https.getSize();
    if (contentLength <= 0) {
        Serial.println("GitHubOTA: Invalid content length");
        https.end();
        return false;
    }

    Serial.printf("GitHubOTA: Firmware size: %s\n", formatBytes(contentLength).c_str());

    // Initialize update
    if (!Update.begin(contentLength, U_FLASH)) {
        Serial.printf("GitHubOTA: Update.begin failed: %s\n", Update.errorString());
        https.end();
        return false;
    }

    // Setup SHA256 if we have expected checksum
    mbedtls_md_context_t ctx;
    mbedtls_md_type_t md_type = MBEDTLS_MD_SHA256;
    bool verifyChecksum = (_firmwareChecksum.length() == 64);  // SHA256 is 64 hex chars

    if (verifyChecksum) {
        mbedtls_md_init(&ctx);
        mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(md_type), 0);
        mbedtls_md_starts(&ctx);
        Serial.println("GitHubOTA: Checksum verification enabled");
    }

    // Download and flash with progress
    WiFiClient* stream = https.getStreamPtr();
    uint8_t buffer[1024];
    size_t written = 0;
    int lastPercent = -1;

    displayMessage("INSTALLING");

    while (https.connected() && written < contentLength) {
        size_t available = stream->available();

        if (available) {
            size_t bytesToRead = ((available > sizeof(buffer)) ? sizeof(buffer) : available);
            size_t bytesRead = stream->readBytes(buffer, bytesToRead);

            if (bytesRead > 0) {
                // Update checksum
                if (verifyChecksum) {
                    mbedtls_md_update(&ctx, buffer, bytesRead);
                }

                // Write to flash
                size_t bytesWritten = Update.write(buffer, bytesRead);
                if (bytesWritten != bytesRead) {
                    Serial.printf("GitHubOTA: Write error at byte %d\n", written);
                    Update.abort();
                    https.end();
                    return false;
                }

                written += bytesRead;

                // Update progress
                int percent = (written * 100) / contentLength;
                if (percent != lastPercent && percent % 10 == 0) {
                    Serial.printf("GitHubOTA: Progress: %d%%\n", percent);
                    displayMessage("INSTALLING " + String(percent) + "%");
                    lastPercent = percent;
                }
            }
        }
        delay(1);
    }

    https.end();

    if (written != contentLength) {
        Serial.printf("GitHubOTA: Download incomplete: %d/%d bytes\n", written, contentLength);
        Update.abort();
        return false;
    }

    Serial.println("GitHubOTA: Download complete");

    // Verify checksum
    if (verifyChecksum) {
        displayMessage("VERIFYING CHECKSUM");
        uint8_t hash[32];
        mbedtls_md_finish(&ctx, hash);
        mbedtls_md_free(&ctx);

        // Convert to hex string
        char hashStr[65];
        for (int i = 0; i < 32; i++) {
            sprintf(&hashStr[i * 2], "%02x", hash[i]);
        }
        hashStr[64] = 0;

        String calculatedChecksum = String(hashStr);
        Serial.printf("GitHubOTA: Calculated checksum: %s\n", calculatedChecksum.c_str());
        Serial.printf("GitHubOTA: Expected checksum:   %s\n", _firmwareChecksum.c_str());

        if (!calculatedChecksum.equalsIgnoreCase(_firmwareChecksum)) {
            Serial.println("GitHubOTA: CHECKSUM MISMATCH - Update aborted!");
            Update.abort();
            return false;
        }

        Serial.println("GitHubOTA: Checksum verified OK");
    }

    // Finalize update
    if (!Update.end(true)) {
        Serial.printf("GitHubOTA: Update.end failed: %s\n", Update.errorString());
        return false;
    }

    Serial.println("GitHubOTA: Firmware flashed successfully");
    return true;
}

// Compare semantic versions
int GitHubOTA::compareVersions(const String& v1, const String& v2) {
    int major1, minor1, patch1;
    int major2, minor2, patch2;

    if (!parseVersion(v1, major1, minor1, patch1)) {
        Serial.printf("GitHubOTA: Failed to parse version: %s\n", v1.c_str());
        return 0;
    }

    if (!parseVersion(v2, major2, minor2, patch2)) {
        Serial.printf("GitHubOTA: Failed to parse version: %s\n", v2.c_str());
        return 0;
    }

    // Compare major.minor.patch
    if (major1 != major2) return (major1 > major2) ? 1 : -1;
    if (minor1 != minor2) return (minor1 > minor2) ? 1 : -1;
    if (patch1 != patch2) return (patch1 > patch2) ? 1 : -1;

    return 0;  // Equal
}

// Parse version string
bool GitHubOTA::parseVersion(const String& version, int& major, int& minor, int& patch) {
    major = minor = patch = 0;

    String v = version;
    // Remove 'v' prefix if present
    if (v.startsWith("v") || v.startsWith("V")) {
        v = v.substring(1);
    }

    int firstDot = v.indexOf('.');
    if (firstDot < 0) return false;

    int secondDot = v.indexOf('.', firstDot + 1);
    if (secondDot < 0) return false;

    major = v.substring(0, firstDot).toInt();
    minor = v.substring(firstDot + 1, secondDot).toInt();
    patch = v.substring(secondDot + 1).toInt();

    return true;
}

// Display message on sign
void GitHubOTA::displayMessage(const String& message) {
    if (_sign) {
        // Display message with flash mode, red color, top line position, twinkle effect
        _sign->WriteTextFile('A', message.c_str(), BB_COL_RED, BB_DP_TOPLINE, BB_DM_FLASH, BB_SDM_TWINKLE);
        Serial.printf("GitHubOTA: Sign message: %s\n", message.c_str());
    }
}

// Format bytes for human readability
String GitHubOTA::formatBytes(size_t bytes) {
    if (bytes < 1024) {
        return String(bytes) + " B";
    } else if (bytes < (1024 * 1024)) {
        return String(bytes / 1024.0, 1) + " KB";
    } else {
        return String(bytes / 1048576.0, 2) + " MB";
    }
}

// Getters
bool GitHubOTA::isUpdateAvailable() const {
    return _updateAvailable;
}

String GitHubOTA::getLatestVersion() const {
    return _latestVersion;
}

String GitHubOTA::getStatus() const {
    return _statusMessage;
}

// Set status message
void GitHubOTA::setStatus(const String& status) {
    _statusMessage = status;
    Serial.printf("GitHubOTA: Status: %s\n", status.c_str());
}
