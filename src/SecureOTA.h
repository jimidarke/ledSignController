/**
 * @file SecureOTA.h
 * @brief Secure Over-The-Air update system for LED Sign Controller
 * 
 * This module provides secure OTA functionality with:
 * - HTTPS-only downloads
 * - Cryptographic signature verification
 * - Checksum validation
 * - Rollback capability
 * - User consent management
 * - Progress reporting
 * - Multiple update sources (HTTP, Home Assistant, GitHub)
 * 
 * @author LED Sign Controller Project
 * @version 0.1.4
 * @date 2024
 */

#ifndef SECURE_OTA_H
#define SECURE_OTA_H

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <Update.h>
#include <ArduinoJson.h>
#include <WiFiClientSecure.h>
#include <functional>

/**
 * @brief OTA update sources enumeration
 */
enum class OTASource {
    HTTP_JSON,      ///< HTTP/HTTPS with JSON metadata
    GITHUB_RELEASES,///< GitHub releases API
    HOME_ASSISTANT, ///< Home Assistant OTA
    CUSTOM          ///< Custom implementation
};

/**
 * @brief OTA update information structure
 */
struct OTAUpdateInfo {
    String version;         ///< New version string
    String download_url;    ///< Download URL for firmware
    String checksum_sha256; ///< SHA256 checksum
    String signature;       ///< Cryptographic signature
    size_t size;           ///< Firmware size in bytes
    String release_notes;   ///< Human-readable release notes
    bool mandatory;         ///< Whether update is mandatory
    String compatibility;   ///< Compatibility requirements
    unsigned long release_date; ///< Release timestamp
};

/**
 * @brief OTA progress callback function type
 * @param current Current download bytes
 * @param total Total firmware size
 * @param percentage Completion percentage (0-100)
 */
typedef std::function<void(size_t current, size_t total, uint8_t percentage)> OTAProgressCallback;

/**
 * @brief OTA status callback function type
 * @param message Status message
 * @param is_error Whether message indicates an error
 */
typedef std::function<void(const String& message, bool is_error)> OTAStatusCallback;

/**
 * @brief Secure Over-The-Air update manager
 * 
 * Provides secure, robust OTA update functionality with multiple
 * verification layers and user consent management.
 */
class SecureOTA {
private:
    // Configuration
    String current_version;     ///< Current firmware version
    String device_id;          ///< Unique device identifier
    OTASource update_source;   ///< Update source type
    String update_url;         ///< Base URL for updates
    String api_key;            ///< API key for authenticated requests
    
    // Security settings
    bool require_signature;    ///< Whether to require signature verification
    String public_key;         ///< Public key for signature verification
    bool allow_downgrade;      ///< Whether to allow version downgrades
    
    // State management
    bool update_available;     ///< Whether update is available
    bool update_in_progress;   ///< Whether update is currently running
    OTAUpdateInfo pending_update; ///< Information about pending update
    
    // Callbacks
    OTAProgressCallback progress_callback;
    OTAStatusCallback status_callback;
    
    // Constants
    static const size_t MAX_FIRMWARE_SIZE = 2 * 1024 * 1024; ///< 2MB max firmware
    static const unsigned long CHECK_INTERVAL = 3600000;     ///< Check every hour
    static const int MAX_DOWNLOAD_RETRIES = 3;               ///< Max download attempts
    
    /**
     * @brief Check for updates from HTTP/HTTPS JSON endpoint
     * @param url URL to check for updates
     * @return true if update check successful, false otherwise
     */
    bool checkHTTPUpdates(const String& url);
    
    /**
     * @brief Check for updates from GitHub releases
     * @param repo_url GitHub repository URL
     * @return true if update check successful, false otherwise
     */
    bool checkGitHubUpdates(const String& repo_url);
    
    /**
     * @brief Verify firmware signature
     * @param firmware_data Firmware binary data
     * @param signature Signature to verify
     * @return true if signature is valid, false otherwise
     */
    bool verifySignature(const uint8_t* firmware_data, size_t size, const String& signature);
    
    /**
     * @brief Calculate SHA256 checksum of data
     * @param data Data to checksum
     * @param size Size of data
     * @return Hex-encoded SHA256 checksum
     */
    String calculateSHA256(const uint8_t* data, size_t size);
    
    /**
     * @brief Compare version strings semantically
     * @param version1 First version string
     * @param version2 Second version string
     * @return -1 if v1 < v2, 0 if equal, 1 if v1 > v2
     */
    int compareVersions(const String& version1, const String& version2);
    
    /**
     * @brief Download firmware with progress reporting
     * @param url Download URL
     * @param expected_size Expected firmware size
     * @param expected_checksum Expected SHA256 checksum
     * @return true if download successful, false otherwise
     */
    bool downloadFirmware(const String& url, size_t expected_size, const String& expected_checksum);
    
    /**
     * @brief Apply firmware update
     * @param firmware_data Verified firmware data
     * @param size Firmware size
     * @return true if update successful, false otherwise
     */
    bool applyUpdate(const uint8_t* firmware_data, size_t size);
    
    /**
     * @brief Report status to callback
     * @param message Status message
     * @param is_error Whether message indicates error
     */
    void reportStatus(const String& message, bool is_error = false);
    
    /**
     * @brief Report progress to callback
     * @param current Current bytes
     * @param total Total bytes
     */
    void reportProgress(size_t current, size_t total);
    
public:
    /**
     * @brief Constructor
     * @param current_version Current firmware version
     * @param device_id Unique device identifier
     */
    SecureOTA(const String& current_version, const String& device_id);
    
    /**
     * @brief Configure update source
     * @param source Update source type
     * @param url Base URL or endpoint
     * @param api_key Optional API key for authentication
     * @return true if configuration successful
     */
    bool configureSource(OTASource source, const String& url, const String& api_key = "");
    
    /**
     * @brief Configure security settings
     * @param require_sig Whether to require signature verification
     * @param pub_key Public key for signature verification (PEM format)
     * @param allow_downgrade Whether to allow version downgrades
     */
    void configureSecurity(bool require_sig = true, const String& pub_key = "", bool allow_downgrade = false);
    
    /**
     * @brief Set progress callback
     * @param callback Function to call with progress updates
     */
    void setProgressCallback(OTAProgressCallback callback);
    
    /**
     * @brief Set status callback
     * @param callback Function to call with status updates
     */
    void setStatusCallback(OTAStatusCallback callback);
    
    /**
     * @brief Check for available updates
     * @param force_check Force check even if recently checked
     * @return true if check completed successfully (regardless of availability)
     */
    bool checkForUpdates(bool force_check = false);
    
    /**
     * @brief Check if update is available
     * @return true if update is available and ready to install
     */
    bool isUpdateAvailable() const;
    
    /**
     * @brief Get information about pending update
     * @return OTAUpdateInfo structure with update details
     */
    const OTAUpdateInfo& getPendingUpdate() const;
    
    /**
     * @brief Begin update installation
     * @param user_consent Whether user has consented to update
     * @return true if update started successfully
     */
    bool beginUpdate(bool user_consent = true);
    
    /**
     * @brief Check if update is currently in progress
     * @return true if update is running
     */
    bool isUpdateInProgress() const;
    
    /**
     * @brief Cancel update in progress (if possible)
     * @return true if cancellation successful
     */
    bool cancelUpdate();
    
    /**
     * @brief Get current update status as string
     * @return Human-readable status string
     */
    String getStatus() const;
    
    /**
     * @brief Enable/disable automatic updates
     * @param enabled Whether to automatically install updates
     * @param mandatory_only Whether to only auto-install mandatory updates
     */
    void setAutoUpdate(bool enabled, bool mandatory_only = true);
    
    /**
     * @brief Main loop function - call regularly to handle updates
     */
    void loop();
    
    /**
     * @brief Trigger manual update check
     * Non-blocking, results available via callbacks
     */
    void triggerUpdateCheck();
    
    /**
     * @brief Get last update check time
     * @return Timestamp of last update check
     */
    unsigned long getLastCheckTime() const;
    
    /**
     * @brief Format update info as JSON string
     * @return JSON string with update information
     */
    String getUpdateInfoJSON() const;
};

#endif // SECURE_OTA_H