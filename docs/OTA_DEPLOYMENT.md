# OTA Update Deployment Guide

This guide explains how to set up and use the GitHub-based OTA (Over-The-Air) update system for the LED Sign Controller.

## Table of Contents

- [Overview](#overview)
- [Prerequisites](#prerequisites)
- [Initial Setup](#initial-setup)
- [Creating a Release](#creating-a-release)
- [Testing Updates](#testing-updates)
- [Troubleshooting](#troubleshooting)
- [Security Considerations](#security-considerations)

---

## Overview

The LED Sign Controller uses a GitHub Releases-based OTA update system with the following features:

- **GitHub Releases API** for firmware distribution
- **HTTPS-only** downloads with certificate validation
- **SHA256 checksum verification** to prevent corrupted firmware
- **Semantic versioning** (e.g., 0.2.0 → 0.2.1)
- **Automatic periodic checks** (default: every 24 hours)
- **LED sign feedback** during update process
- **Private or public repository** support

### Update Flow

1. Device periodically checks GitHub Releases API for latest release
2. Compares latest version with current firmware version
3. If newer version available, downloads `firmware.bin` and `firmware.sha256`
4. Verifies checksum matches downloaded firmware
5. Displays "UPDATING FIRMWARE" on LED sign
6. Flashes new firmware to ESP32
7. Displays "UPDATE COMPLETE - REBOOTING" and restarts

---

## Prerequisites

### GitHub Repository

1. **Create or use existing repository** for your LED sign firmware
   - Can be public or private (private recommended for production)
   - Repository name: `ledSignController` (or your choice)

2. **Update `defines.h`** with your repository info:
   ```cpp
   #define GITHUB_REPO_OWNER         "yourusername"     // Your GitHub username
   #define GITHUB_REPO_NAME          "ledSignController"  // Your repo name
   ```

### GitHub Personal Access Token (for Private Repos)

If using a **private repository**, you need a Personal Access Token:

1. Go to https://github.com/settings/tokens
2. Click **"Generate new token (classic)"**
3. Give it a descriptive name: `ESP32-OTA-Access`
4. Set expiration: **No expiration** (or choose appropriate duration)
5. Select scopes:
   - ✅ `repo` (Full control of private repositories)
   - OR ✅ `public_repo` (if using public repository)
6. Click **"Generate token"**
7. **COPY THE TOKEN** immediately - you won't see it again!
8. Token format: `ghp_xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx`

### Tools Required

- **PlatformIO** (for building firmware)
- **sha256sum** (for generating checksums)
  - Linux/macOS: pre-installed
  - Windows: install Git Bash or use `CertUtil`

---

## Initial Setup

### 1. Configure Repository in Code

Edit `src/defines.h`:

```cpp
// GitHub Repository Configuration
#define GITHUB_REPO_OWNER         "myusername"         // Your GitHub username
#define GITHUB_REPO_NAME          "ledSignController"  // Your repository name
```

### 2. Upload GitHub Token to ESP32 (Private Repos Only)

**Option A: Manual Upload via SPIFFS**

1. Create `data/github_token.txt` in your project:
   ```bash
   mkdir -p data
   echo "ghp_your_token_here" > data/github_token.txt
   ```

2. Upload filesystem to ESP32:
   ```bash
   pio run -t uploadfs
   ```

**Option B: Programmatic Upload (Advanced)**

You can also create a web interface or serial command to upload the token after deployment.

### 3. Verify Configuration

1. Build and upload firmware:
   ```bash
   pio run -t upload
   ```

2. Monitor serial output:
   ```bash
   pio device monitor
   ```

3. Look for these messages:
   ```
   OTA: SPIFFS mounted successfully
   OTA: GitHub token loaded successfully
   OTA: Manager initialized successfully
   ```

---

## Creating a Release

### Step 1: Update Version Number

Before creating a release, update the version in `src/defines.h`:

```cpp
#define FIRMWARE_VERSION          "0.2.1"  // Increment from 0.2.0
```

### Step 2: Build Firmware Binary

```bash
# Clean previous builds
pio run -t clean

# Build firmware
pio run

# Firmware binary will be at:
# .pio/build/esp32dev/firmware.bin
```

### Step 3: Generate SHA256 Checksum

**Linux/macOS:**
```bash
cd .pio/build/esp32dev/
sha256sum firmware.bin > firmware.sha256
```

**Windows (Git Bash):**
```bash
cd .pio/build/esp32dev/
sha256sum firmware.bin > firmware.sha256
```

**Windows (PowerShell):**
```powershell
cd .pio\build\esp32dev\
CertUtil -hashfile firmware.bin SHA256 | findstr /v ":" > firmware.sha256
```

### Step 4: Create GitHub Release

**Via GitHub Web Interface:**

1. Go to your repository on GitHub
2. Click **"Releases"** → **"Create a new release"**
3. Click **"Choose a tag"** → type `v0.2.1` → click **"Create new tag"**
4. **Release title:** `Version 0.2.1` (or descriptive title)
5. **Description:** Add release notes (features, bug fixes, etc.)
6. **Attach files:**
   - Drag and drop `.pio/build/esp32dev/firmware.bin`
   - Drag and drop `.pio/build/esp32dev/firmware.sha256`
7. Click **"Publish release"**

**Via GitHub CLI (gh):**

```bash
# Create release and upload files
gh release create v0.2.1 \
  --title "Version 0.2.1" \
  --notes "Bug fixes and performance improvements" \
  .pio/build/esp32dev/firmware.bin \
  .pio/build/esp32dev/firmware.sha256
```

### Step 5: Verify Release

1. Go to your repository's Releases page
2. Verify the release shows:
   - ✅ Tag: `v0.2.1`
   - ✅ Assets: `firmware.bin` and `firmware.sha256`
3. Test download links manually to ensure they work

---

## Testing Updates

### Method 1: Wait for Automatic Check (Default: 24 hours)

The device will automatically check for updates every 24 hours (configurable in `defines.h`).

**Serial Monitor Output:**
```
GitHubOTA: Periodic update check triggered
GitHubOTA: Fetching https://api.github.com/repos/user/repo/releases/latest
GitHubOTA: Latest release: 0.2.1
GitHubOTA: Update available: 0.2.0 -> 0.2.1
GitHubOTA: Starting update to version 0.2.1
```

**LED Sign Display:**
```
CHECKING FOR UPDATES
DOWNLOADING
INSTALLING 50%
VERIFYING CHECKSUM
UPDATE COMPLETE - REBOOTING
```

### Method 2: Manual Trigger (Requires Code Modification)

Add an MQTT command handler or web button to trigger `ota_manager->checkForUpdate()`.

### Method 3: Enable Boot-Time Check

Edit `src/defines.h`:
```cpp
#define OTA_BOOT_CHECK_ENABLED    true   // Check on every boot
```

---

## Configuration Options

All OTA settings are in `src/defines.h`:

```cpp
// Update check frequency
#define OTA_CHECK_INTERVAL_HOURS  24      // Check every 24 hours
#define OTA_CHECK_INTERVAL_MS     (OTA_CHECK_INTERVAL_HOURS * 60 * 60 * 1000UL)

// Update behavior
#define OTA_AUTO_UPDATE_ENABLED   true    // Auto-download and install updates
#define OTA_BOOT_CHECK_ENABLED    false   // Also check on boot

// Security settings
#define OTA_VERIFY_CHECKSUM       true    // Require SHA256 verification
#define OTA_ALLOW_DOWNGRADE       false   // Prevent downgrading to older versions
```

---

## Troubleshooting

### Device Not Detecting Updates

**Check 1: Verify GitHub Configuration**
```cpp
// In defines.h - make sure these match your repo
#define GITHUB_REPO_OWNER         "yourusername"
#define GITHUB_REPO_NAME          "ledSignController"
```

**Check 2: Verify Token (Private Repos)**
```bash
# Serial monitor should show:
OTA: GitHub token loaded successfully
```

If you see `"No GitHub token found"`, re-upload `data/github_token.txt`.

**Check 3: Verify Release Format**

- Tag must start with `v` (e.g., `v0.2.1`)
- Assets must include **both** `firmware.bin` and `firmware.sha256`
- Checksum file must contain only the SHA256 hash (64 hex characters)

**Check 4: Test GitHub API Manually**

```bash
# Test API access (replace with your repo)
curl -H "Authorization: token YOUR_TOKEN" \
  https://api.github.com/repos/USERNAME/REPO/releases/latest
```

### Checksum Verification Fails

**Symptom:** Serial monitor shows:
```
GitHubOTA: CHECKSUM MISMATCH - Update aborted!
```

**Solution:**

1. Verify `firmware.sha256` was generated correctly:
   ```bash
   cat .pio/build/esp32dev/firmware.sha256
   # Should show: 64_hex_characters  firmware.bin
   ```

2. Ensure file wasn't corrupted during upload to GitHub

3. Regenerate checksum and create new release

### Update Fails to Flash

**Symptom:** Serial monitor shows:
```
GitHubOTA: Update.begin failed: Not enough space
```

**Solution:**

Check partition scheme in `platformio.ini`:
```ini
board_build.partitions = min_spiffs.csv
```

This provides 2x ~896KB OTA partitions. If your firmware is larger, create a custom partition scheme.

### GitHub API Rate Limiting

**Symptom:**
```
GitHubOTA: HTTP error 403
```

**Solution:**

- GitHub API rate limit without token: **60 requests/hour**
- With token: **5000 requests/hour**
- Ensure your device has a GitHub token configured

### WiFi Disconnects During Update

**Symptom:** Update stops mid-download

**Solution:**

- Improve WiFi signal strength
- Increase `UPDATE_TIMEOUT_MS` in `GitHubOTA.h` (default: 5 minutes)
- Update downloads are blocking - device will retry on next interval

---

## Security Considerations

### GitHub Token Security

⚠️ **IMPORTANT:** GitHub tokens grant access to your repositories!

**Best Practices:**

1. **Use tokens with minimal scope:**
   - Private repo: `repo` scope
   - Public repo: `public_repo` scope only

2. **Token storage:**
   - Tokens are stored in SPIFFS **unencrypted**
   - Consider enabling ESP32 flash encryption for production
   - Rotate tokens periodically

3. **Per-device tokens (Advanced):**
   - Generate separate tokens for each device
   - Allows revoking access for individual devices

### Firmware Authenticity

**Current Security Level: BASIC**

✅ **Implemented:**
- HTTPS download (prevents man-in-the-middle)
- SHA256 checksum (prevents corruption)
- Semantic versioning (prevents accidental downgrades)

❌ **NOT Implemented (Future Enhancement):**
- Cryptographic signature verification
- Would prevent malicious firmware even if GitHub account is compromised
- Requires private key management (complex)

**For Maximum Security:**

- Keep your GitHub account secure (2FA enabled)
- Use a private repository
- Regularly audit repository access
- Consider implementing signature verification (see `SecureOTA.h` for design)

### Network Security

- All downloads use HTTPS (TLS)
- Certificate validation enabled by default
- GitHub's infrastructure provides DDoS protection
- No custom server maintenance required

---

## Advanced Usage

### Custom Update Check Intervals

Modify in `defines.h`:

```cpp
#define OTA_CHECK_INTERVAL_HOURS  6   // Check every 6 hours
```

Or dynamically in code:

```cpp
ota_manager->setCheckInterval(6 * 60 * 60 * 1000); // 6 hours
```

### Disable Automatic Updates

```cpp
// In defines.h:
#define OTA_AUTO_UPDATE_ENABLED   false

// Or in code:
ota_manager->setAutoUpdate(false);

// Manual trigger required:
ota_manager->checkForUpdate();
if (ota_manager->isUpdateAvailable()) {
    ota_manager->performUpdate();  // User must confirm
}
```

### MQTT-Triggered Updates

Add to `handleMQTTMessage()`:

```cpp
if (strcmp(topic, "ledSign/YOUR_ZONE/command") == 0) {
    if (strcmp(message.c_str(), "check_update") == 0) {
        if (ota_manager) {
            ota_manager->checkForUpdate();
        }
    }
}
```

### Pre-Release / Beta Testing

Use pre-release tags:

```bash
gh release create v0.3.0-beta \
  --prerelease \
  --title "Beta Release 0.3.0" \
  firmware.bin firmware.sha256
```

**Note:** Current implementation fetches `releases/latest`, which excludes pre-releases. To include pre-releases, modify `GitHubOTA.cpp` API endpoint.

---

## Automated Release Workflow (GitHub Actions)

**Highly Recommended:** Automate builds and releases using GitHub Actions to eliminate manual steps!

### Benefits of GitHub Actions

- ✅ **Fully automated**: Push a tag, get a release
- ✅ **Consistent builds**: Same environment every time
- ✅ **Fast**: Builds in GitHub's cloud infrastructure
- ✅ **Free**: Unlimited for public repos, 2,000 min/month for private
- ✅ **Version validation**: Automatically checks `defines.h` matches tag
- ✅ **Size checks**: Fails if firmware exceeds OTA partition
- ✅ **Auto-generated release notes**: Professional release documentation

### Quick Start

1. **Create workflow file** `.github/workflows/release-firmware.yml`
2. **Push the workflow** to your repository
3. **Update version** in `src/defines.h`
4. **Create and push tag**: `git tag v0.2.1 && git push origin v0.2.1`
5. **Done!** GitHub Actions builds and releases automatically

### Workflow Overview

```yaml
name: Build and Release Firmware

on:
  push:
    tags:
      - 'v*'

jobs:
  build-and-release:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v3
      - name: Set up Python
        uses: actions/setup-python@v4
        with:
          python-version: '3.11'
      - name: Install PlatformIO
        run: pip install --upgrade platformio
      - name: Build firmware
        run: pio run -e esp32dev
      - name: Generate checksum
        run: |
          cd .pio/build/esp32dev
          sha256sum firmware.bin > firmware.sha256
      - name: Create Release
        uses: softprops/action-gh-release@v1
        with:
          files: |
            .pio/build/esp32dev/firmware.bin
            .pio/build/esp32dev/firmware.sha256
        env:
          GITHUB_TOKEN: ${{ secrets.GITHUB_TOKEN }}
```

### Complete Documentation

For the **full production-ready workflow** with version validation, size checks, auto-generated release notes, and advanced features, see:

📖 **[GITHUB_ACTIONS_OTA.md](GITHUB_ACTIONS_OTA.md)** - Complete GitHub Actions guide

This includes:
- Production-ready workflow file
- Multiple board support
- Pre-release/beta handling
- Automated testing
- Slack/Discord notifications
- Team workflow best practices
- Troubleshooting guide

---

## FAQ

**Q: Can I use a public repository?**

A: Yes! If your repository is public, you don't need a GitHub token. Just set `GITHUB_REPO_OWNER` and `GITHUB_REPO_NAME` in `defines.h`.

**Q: How do I rollback a bad update?**

A: Publish a new release with a higher version number containing the previous working firmware. The OTA system uses semantic versioning and won't downgrade.

**Q: Can I disable updates temporarily?**

A: Yes, set `OTA_AUTO_UPDATE_ENABLED false` in `defines.h` and rebuild firmware.

**Q: What happens if update fails mid-flash?**

A: The ESP32 will reboot and run the previous firmware. The OTA partition scheme maintains the old firmware until the new one is successfully validated.

**Q: Can I host firmware on my own server instead of GitHub?**

A: Yes, but you'll need to modify `GitHubOTA.cpp` to fetch from a different URL. GitHub Releases provides reliable hosting with global CDN, which is why it was chosen.

---

## Support

For issues or questions:

- Check the [Troubleshooting](#troubleshooting) section
- Review serial monitor output for detailed error messages
- Verify your GitHub Release follows the correct format
- Ensure ESP32 has internet connectivity and can reach GitHub API

Happy updating! 🚀
