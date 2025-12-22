/**
 * GitHubOTA.h
 *
 * Simple, secure OTA update system using GitHub Releases
 *
 * Features:
 * - GitHub Releases API integration (private repo support)
 * - HTTPS with certificate validation
 * - SHA256 checksum verification
 * - Semantic version comparison
 * - Periodic automatic checking
 * - Sign feedback during updates
 *
 * Usage:
 *   GitHubOTA ota("username", "repo-name", &sign);
 *   ota.begin("0.2.0");
 *   ota.setGitHubToken(token);
 *   ota.setCheckInterval(24 * 60 * 60 * 1000); // 24 hours
 *
 *   // In loop():
 *   ota.loop();
 */

#ifndef GITHUBOTA_H
#define GITHUBOTA_H

#include <Arduino.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <Update.h>
#include <ArduinoJson.h>
#include "BETABRITE.h"

// Default configuration
#define DEFAULT_CHECK_INTERVAL (24 * 60 * 60 * 1000UL)  // 24 hours in milliseconds
#define UPDATE_TIMEOUT_MS 60000                          // 60 seconds for download (HTTPClient setTimeout uses uint16_t)
#define MAX_FIRMWARE_SIZE (2 * 1024 * 1024)             // 2MB max firmware size
#define GITHUB_API_HOST "api.github.com"
#define GITHUB_API_PORT 443

class GitHubOTA {
public:
    /**
     * Constructor
     * @param repoOwner GitHub repository owner/organization
     * @param repoName GitHub repository name
     * @param sign Pointer to BETABRITE sign for status messages
     */
    GitHubOTA(const char* repoOwner, const char* repoName, BETABRITE* sign);

    /**
     * Initialize the OTA system
     * @param currentVersion Current firmware version (e.g., "0.2.0")
     * @return true if initialization successful
     */
    bool begin(const char* currentVersion);

    /**
     * Set the GitHub Personal Access Token for private repos
     * @param token GitHub token (will be stored in memory)
     */
    void setGitHubToken(const char* token);

    /**
     * Set the interval between automatic update checks
     * @param intervalMs Interval in milliseconds (default: 24 hours)
     */
    void setCheckInterval(unsigned long intervalMs);

    /**
     * Call this from main loop() - handles periodic checking
     * Non-blocking except during actual update process
     */
    void loop();

    /**
     * Manually trigger an update check (blocking)
     * @return true if check successful (doesn't mean update available)
     */
    bool checkForUpdate();

    /**
     * Check if an update is available
     * @return true if newer version available
     */
    bool isUpdateAvailable() const;

    /**
     * Get the latest version string from last check
     * @return Version string (e.g., "0.2.1") or empty if no check performed
     */
    String getLatestVersion() const;

    /**
     * Get current operation status
     * @return Status message string
     */
    String getStatus() const;

    /**
     * Perform the update if one is available (blocking)
     * Will display status on sign and reboot on success
     * @return true if update started (device will reboot), false on error
     */
    bool performUpdate();

    /**
     * Enable/disable automatic updates
     * When disabled, only manual checks via checkForUpdate() will work
     * @param enabled true to enable periodic automatic updates
     */
    void setAutoUpdate(bool enabled);

private:
    // Configuration
    const char* _repoOwner;
    const char* _repoName;
    String _currentVersion;
    String _githubToken;
    unsigned long _checkInterval;
    bool _autoUpdateEnabled;

    // State
    BETABRITE* _sign;
    unsigned long _lastCheckTime;
    bool _updateAvailable;
    String _latestVersion;
    String _firmwareUrl;
    String _firmwareChecksum;
    size_t _firmwareSize;
    String _statusMessage;

    // Helper functions

    /**
     * Fetch latest release info from GitHub API
     * @return true if successful and release found
     */
    bool fetchLatestRelease();

    /**
     * Download firmware binary from URL
     * @param url Direct download URL for firmware.bin
     * @return true if download and flash successful
     */
    bool downloadAndFlash(const String& url);

    /**
     * Download checksum file from GitHub release
     * @param checksumUrl URL to firmware.sha256 file
     * @return SHA256 checksum string, or empty on failure
     */
    String downloadChecksum(const String& checksumUrl);

    /**
     * Compare two semantic version strings
     * @param v1 First version (e.g., "0.2.0")
     * @param v2 Second version (e.g., "0.1.9")
     * @return -1 if v1 < v2, 0 if equal, 1 if v1 > v2
     */
    int compareVersions(const String& v1, const String& v2);

    /**
     * Parse version string into major.minor.patch
     * @param version Version string
     * @param major Output: major version
     * @param minor Output: minor version
     * @param patch Output: patch version
     * @return true if parsing successful
     */
    bool parseVersion(const String& version, int& major, int& minor, int& patch);

    /**
     * Display message on LED sign
     * @param message Message to display
     */
    void displayMessage(const String& message);

    /**
     * Convert bytes to human-readable size
     * @param bytes Size in bytes
     * @return Formatted string (e.g., "1.5 MB")
     */
    String formatBytes(size_t bytes);

    /**
     * Set status message (for getStatus())
     * @param status Status message
     */
    void setStatus(const String& status);
};

#endif // GITHUBOTA_H
