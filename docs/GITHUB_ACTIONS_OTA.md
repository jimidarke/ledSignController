# GitHub Actions for Automated OTA Releases

This guide explains how to use GitHub Actions to automate the firmware build and release process for OTA (Over-The-Air) updates to your LED sign controllers.

## Table of Contents

- [Overview](#overview)
- [How It Works](#how-it-works)
- [Setup](#setup)
- [Usage](#usage)
- [Advanced Features](#advanced-features)
- [Security](#security)
- [Team Workflow](#team-workflow)
- [Troubleshooting](#troubleshooting)
- [Cost](#cost)

---

## Overview

**GitHub Actions** is GitHub's built-in CI/CD (Continuous Integration/Continuous Deployment) platform that can automatically build, test, and release your firmware whenever you push changes.

### Benefits

✅ **Automation**: Push a tag, get a release - no manual steps required

✅ **Consistency**: Every build uses the same environment (no "works on my machine" issues)

✅ **Speed**: Builds happen in GitHub's cloud infrastructure

✅ **Traceability**: Every release is linked to the exact commit with full build logs

✅ **Security**: Firmware built in a clean, controlled environment

✅ **Collaboration**: Team members can trigger releases without PlatformIO installed locally

✅ **Easy Rollback**: Can re-release previous versions with a single click

✅ **Free**: Unlimited builds for public repositories, 2,000 minutes/month free for private repos

---

## How It Works

### The Automated Release Pipeline

```
Developer pushes tag     GitHub Actions automatically:
    v0.2.1            →  1. Checks out code from repository
         ↓               2. Sets up Python environment
         ↓               3. Installs PlatformIO
         ↓               4. Builds firmware.bin for ESP32
         ↓               5. Generates firmware.sha256 checksum
         ↓               6. Creates GitHub Release with tag
         ↓               7. Uploads firmware.bin as release asset
         ↓               8. Uploads firmware.sha256 as release asset
         ↓               9. Generates release notes
         ↓
    ESP32 devices    → 10. Devices detect new release (24h interval)
    auto-update         11. Download firmware over HTTPS
                       12. Verify SHA256 checksum
                       13. Flash and reboot automatically
```

### Traditional vs Automated Workflow

**Before (Manual Process)**:
```bash
1. vim src/defines.h                              # Update version
2. pio run -t clean                               # Clean build
3. pio run                                        # Build firmware (~3 min)
4. cd .pio/build/esp32dev                        # Navigate to build dir
5. sha256sum firmware.bin > firmware.sha256      # Generate checksum
6. Open GitHub website                            # Manual navigation
7. Click "Create new release"                     # Manual process
8. Type tag name: v0.2.1                         # Manual entry
9. Write release notes                            # Manual entry
10. Drag and drop firmware.bin                    # Manual upload
11. Drag and drop firmware.sha256                 # Manual upload
12. Click "Publish release"                       # Manual confirmation

Total time: ~10-15 minutes of manual work
```

**After (GitHub Actions)**:
```bash
1. vim src/defines.h                              # Update version
2. git commit -am "Bump version to 0.2.1"        # Commit changes
3. git tag v0.2.1                                 # Create tag
4. git push origin v0.2.1                         # Push tag

# Everything else happens automatically in ~3-5 minutes!

Total time: ~30 seconds of developer time
```

---

## Setup

### Prerequisites

- GitHub repository (public or private)
- Basic familiarity with Git and GitHub
- Repository must contain PlatformIO project structure

### Step 1: Create Workflow Directory

In your project root, create the GitHub Actions workflow directory:

```bash
mkdir -p .github/workflows
```

### Step 2: Create Workflow File

Create `.github/workflows/release-firmware.yml`:

```yaml
name: Build and Release Firmware

# Trigger on version tags (v0.2.0, v0.2.1, etc.)
on:
  push:
    tags:
      - 'v*'

  # Also allow manual triggering from GitHub UI
  workflow_dispatch:
    inputs:
      version:
        description: 'Version tag (e.g., v0.2.1)'
        required: true
        type: string

jobs:
  build-and-release:
    runs-on: ubuntu-latest

    steps:
      # 1. Checkout the code at the tagged commit
      - name: Checkout code
        uses: actions/checkout@v3
        with:
          submodules: recursive  # If you use git submodules

      # 2. Set up Python for PlatformIO
      - name: Set up Python
        uses: actions/setup-python@v4
        with:
          python-version: '3.11'
          cache: 'pip'

      # 3. Install PlatformIO
      - name: Install PlatformIO
        run: |
          pip install --upgrade platformio
          pio --version
          pio platform install espressif32

      # 4. Build firmware
      - name: Build firmware
        run: |
          echo "Building firmware for ESP32..."
          pio run -e esp32dev
          echo "Build complete!"
          ls -lh .pio/build/esp32dev/

      # 5. Generate SHA256 checksum
      - name: Generate checksum
        run: |
          cd .pio/build/esp32dev
          sha256sum firmware.bin > firmware.sha256
          echo "Generated checksum:"
          cat firmware.sha256

      # 6. Get firmware size and validate
      - name: Check firmware size
        id: firmware_info
        run: |
          SIZE=$(stat -c%s .pio/build/esp32dev/firmware.bin)
          SIZE_MB=$(echo "scale=2; $SIZE / 1024 / 1024" | bc)
          echo "FIRMWARE_SIZE=$SIZE" >> $GITHUB_OUTPUT
          echo "FIRMWARE_SIZE_MB=$SIZE_MB" >> $GITHUB_OUTPUT
          echo "Firmware size: $SIZE_MB MB"

          # Fail if firmware is too large (>1.8MB for OTA partition)
          if [ $SIZE -gt 1887436 ]; then
            echo "ERROR: Firmware too large for OTA partition!"
            echo "Maximum: 1.8 MB, Actual: $SIZE_MB MB"
            exit 1
          fi

      # 7. Extract version from tag
      - name: Get version
        id: version
        run: |
          if [ "${{ github.event_name }}" = "workflow_dispatch" ]; then
            VERSION="${{ github.event.inputs.version }}"
          else
            VERSION="${GITHUB_REF#refs/tags/}"
          fi
          echo "VERSION=$VERSION" >> $GITHUB_OUTPUT
          echo "Building version: $VERSION"

      # 8. Validate version matches defines.h
      - name: Validate version
        run: |
          VERSION_TAG="${{ steps.version.outputs.VERSION }}"
          VERSION_TAG_CLEAN="${VERSION_TAG#v}"  # Remove 'v' prefix

          # Extract version from defines.h
          VERSION_IN_CODE=$(grep "FIRMWARE_VERSION" src/defines.h | grep -o '"[^"]*"' | tr -d '"')

          echo "Version in tag: $VERSION_TAG_CLEAN"
          echo "Version in code: $VERSION_IN_CODE"

          if [ "$VERSION_IN_CODE" != "$VERSION_TAG_CLEAN" ]; then
            echo "❌ ERROR: Version mismatch!"
            echo "Git tag: $VERSION_TAG_CLEAN"
            echo "defines.h: $VERSION_IN_CODE"
            echo ""
            echo "Please update FIRMWARE_VERSION in src/defines.h to match the tag."
            exit 1
          fi

          echo "✅ Version validation passed"

      # 9. Generate release notes
      - name: Generate release notes
        id: release_notes
        run: |
          cat > release_notes.md << EOF
          ## 🚀 Firmware Release ${{ steps.version.outputs.VERSION }}

          **Auto-generated release** from GitHub Actions

          ### 📦 Installation

          ESP32 devices with OTA enabled will **automatically detect and install** this update within 24 hours (default check interval).

          To force an immediate update, reboot the device or enable boot-time checks in \`defines.h\`.

          ### 📄 Files

          - \`firmware.bin\` - ESP32 firmware binary (${{ steps.firmware_info.outputs.FIRMWARE_SIZE_MB }} MB)
          - \`firmware.sha256\` - SHA256 checksum for verification

          ### 🔧 Build Information

          | Property | Value |
          |----------|-------|
          | **Version** | ${{ steps.version.outputs.VERSION }} |
          | **Commit** | [\`${GITHUB_SHA:0:7}\`](https://github.com/${{ github.repository }}/commit/${{ github.sha }}) |
          | **Build Date** | $(date -u +"%Y-%m-%d %H:%M:%S UTC") |
          | **Built by** | GitHub Actions |
          | **Firmware Size** | ${{ steps.firmware_info.outputs.FIRMWARE_SIZE_MB }} MB |
          | **Platform** | ESP32 (espressif32) |
          | **Framework** | Arduino |

          ### 📋 Changes

          See commit history: [Compare changes](https://github.com/${{ github.repository }}/compare/${{ steps.version.outputs.VERSION }}...main)

          ### 🔐 Security

          - ✅ HTTPS download with certificate validation
          - ✅ SHA256 checksum verification
          - ✅ Semantic versioning (no downgrades)
          - ✅ Automatic rollback on flash failure

          ---

          🤖 _This release was automatically built and published by [GitHub Actions](https://github.com/${{ github.repository }}/actions/runs/${{ github.run_id }})_
          EOF

          cat release_notes.md

      # 10. Create GitHub Release
      - name: Create Release
        uses: softprops/action-gh-release@v1
        with:
          tag_name: ${{ steps.version.outputs.VERSION }}
          name: Release ${{ steps.version.outputs.VERSION }}
          body_path: release_notes.md
          files: |
            .pio/build/esp32dev/firmware.bin
            .pio/build/esp32dev/firmware.sha256
          draft: false
          prerelease: false
          generate_release_notes: false
        env:
          GITHUB_TOKEN: ${{ secrets.GITHUB_TOKEN }}

      # 11. Success notification
      - name: Build Summary
        if: success()
        run: |
          echo "✅ Release ${{ steps.version.outputs.VERSION }} created successfully!"
          echo "🔗 Release URL: https://github.com/${{ github.repository }}/releases/tag/${{ steps.version.outputs.VERSION }}"
          echo "📦 Firmware size: ${{ steps.firmware_info.outputs.FIRMWARE_SIZE_MB }} MB"
          echo "🎯 Devices will auto-update within 24 hours"
```

### Step 3: Commit and Push Workflow

```bash
git add .github/workflows/release-firmware.yml
git commit -m "Add GitHub Actions workflow for automated OTA releases"
git push origin main
```

### Step 4: Verify Workflow

1. Go to your GitHub repository
2. Click the "Actions" tab
3. You should see "Build and Release Firmware" in the workflows list
4. The workflow won't run yet (no tags pushed)

---

## Usage

### Method 1: Tag-Based Release (Recommended)

This is the standard workflow for releasing new firmware versions.

**Step-by-Step**:

1. **Update version** in `src/defines.h`:
   ```cpp
   #define FIRMWARE_VERSION          "0.2.1"  // Increment version
   ```

2. **Commit changes**:
   ```bash
   git add src/defines.h
   git commit -m "Bump version to 0.2.1"
   git push origin main
   ```

3. **Create and push tag**:
   ```bash
   git tag v0.2.1
   git push origin v0.2.1
   ```

4. **Watch the magic happen**:
   - Go to GitHub → Actions tab
   - See the workflow running in real-time
   - Build completes in ~3-5 minutes
   - Release appears automatically

5. **Verify release**:
   - Go to GitHub → Releases
   - See `v0.2.1` with firmware.bin and firmware.sha256 attached

**GitHub Actions will automatically**:
- Build the firmware
- Generate checksums
- Create the release
- Upload files
- Generate release notes

### Method 2: Manual Trigger

Useful for testing, rebuilding, or creating releases without code changes.

**Step-by-Step**:

1. Go to GitHub repository
2. Click **"Actions"** tab
3. Select **"Build and Release Firmware"** workflow
4. Click **"Run workflow"** button (top right)
5. Enter version tag (e.g., `v0.2.1`)
6. Click **"Run workflow"**

The workflow will:
- Use current code state
- Build firmware
- Create release with the specified tag

### Method 3: Re-Release Previous Version

If you need to re-release a previous version (e.g., rollback scenario):

```bash
# Delete the existing tag and release
gh release delete v0.2.0 --yes
git tag -d v0.2.0
git push origin :refs/tags/v0.2.0

# Check out the previous version
git checkout <commit-hash-of-v0.2.0>

# Re-create and push tag
git tag v0.2.0
git push origin v0.2.0

# GitHub Actions rebuilds and releases automatically
```

---

## Advanced Features

### 1. Build Multiple Board Variants

If you support multiple ESP32 board types:

```yaml
jobs:
  build-and-release:
    runs-on: ubuntu-latest
    strategy:
      matrix:
        board: [esp32dev, esp32-c3, esp32-s3]

    steps:
      - name: Build firmware for ${{ matrix.board }}
        run: pio run -e ${{ matrix.board }}

      - name: Rename firmware
        run: |
          mv .pio/build/${{ matrix.board }}/firmware.bin \
             .pio/build/${{ matrix.board }}/firmware-${{ matrix.board }}.bin

      - name: Create Release
        uses: softprops/action-gh-release@v1
        with:
          files: |
            .pio/build/${{ matrix.board }}/firmware-${{ matrix.board }}.bin
            .pio/build/${{ matrix.board }}/firmware-${{ matrix.board }}.sha256
```

### 2. Pre-Release / Beta Releases

Automatically mark beta releases:

```yaml
- name: Determine if pre-release
  id: prerelease
  run: |
    VERSION="${{ steps.version.outputs.VERSION }}"
    if [[ "$VERSION" =~ (alpha|beta|rc) ]]; then
      echo "IS_PRERELEASE=true" >> $GITHUB_OUTPUT
      echo "This is a pre-release version"
    else
      echo "IS_PRERELEASE=false" >> $GITHUB_OUTPUT
      echo "This is a stable release"
    fi

- name: Create Release
  uses: softprops/action-gh-release@v1
  with:
    prerelease: ${{ steps.prerelease.outputs.IS_PRERELEASE == 'true' }}
```

**Usage**:
```bash
git tag v0.3.0-beta.1
git push origin v0.3.0-beta.1
# Creates a pre-release (marked differently in GitHub)
```

### 3. Automatic Changelog Generation

Generate changelog from commit messages:

```yaml
- name: Generate Changelog
  id: changelog
  uses: mikepenz/release-changelog-builder-action@v3
  with:
    configuration: |
      {
        "categories": [
          {
            "title": "## 🚀 Features",
            "labels": ["feature", "enhancement"]
          },
          {
            "title": "## 🐛 Bug Fixes",
            "labels": ["bug", "fix"]
          },
          {
            "title": "## 📚 Documentation",
            "labels": ["documentation", "docs"]
          }
        ]
      }
  env:
    GITHUB_TOKEN: ${{ secrets.GITHUB_TOKEN }}

- name: Create Release
  uses: softprops/action-gh-release@v1
  with:
    body: ${{ steps.changelog.outputs.changelog }}
```

**Usage**: Use labels in commit messages or PRs:
```bash
git commit -m "feat: Add weather alert category support"
git commit -m "fix: Correct MQTT reconnection logic"
git commit -m "docs: Update OTA deployment guide"
```

### 4. Run Tests Before Release

Add automated testing:

```yaml
- name: Run unit tests
  run: |
    pio test -e native
    echo "All tests passed ✅"

- name: Lint code
  run: |
    pip install cpplint
    find src -name "*.cpp" -o -name "*.h" | xargs cpplint

- name: Check code formatting
  run: |
    pip install clang-format
    find src -name "*.cpp" -o -name "*.h" | xargs clang-format --dry-run --Werror
```

### 5. Slack/Discord Notifications

Get notified when releases are created:

**Slack**:
```yaml
- name: Notify Slack on success
  if: success()
  uses: 8398a7/action-slack@v3
  with:
    status: custom
    custom_payload: |
      {
        "text": "✅ Firmware ${{ steps.version.outputs.VERSION }} released!",
        "attachments": [{
          "color": "good",
          "fields": [
            {"title": "Version", "value": "${{ steps.version.outputs.VERSION }}", "short": true},
            {"title": "Size", "value": "${{ steps.firmware_info.outputs.FIRMWARE_SIZE_MB }} MB", "short": true}
          ]
        }]
      }
  env:
    SLACK_WEBHOOK_URL: ${{ secrets.SLACK_WEBHOOK }}
```

**Discord**:
```yaml
- name: Notify Discord
  uses: sarisia/actions-status-discord@v1
  with:
    webhook: ${{ secrets.DISCORD_WEBHOOK }}
    title: "Firmware Release ${{ steps.version.outputs.VERSION }}"
    description: "New firmware version released and available for OTA update"
    color: 0x00ff00
```

### 6. Deployment Statistics

Track deployment metrics:

```yaml
- name: Log deployment
  run: |
    curl -X POST https://your-analytics.com/api/deployments \
      -H "Content-Type: application/json" \
      -H "Authorization: Bearer ${{ secrets.ANALYTICS_TOKEN }}" \
      -d '{
        "version": "${{ steps.version.outputs.VERSION }}",
        "timestamp": "$(date -u +%s)",
        "commit": "${{ github.sha }}",
        "firmware_size": ${{ steps.firmware_info.outputs.FIRMWARE_SIZE }}
      }'
```

### 7. Build Artifacts for Debugging

Save build artifacts for later inspection:

```yaml
- name: Upload build artifacts
  uses: actions/upload-artifact@v3
  with:
    name: firmware-${{ steps.version.outputs.VERSION }}
    path: |
      .pio/build/esp32dev/firmware.bin
      .pio/build/esp32dev/firmware.elf
      .pio/build/esp32dev/firmware.map
    retention-days: 30
```

---

## Security

### GitHub Token Permissions

The workflow uses `GITHUB_TOKEN` which is automatically provided by GitHub:

- ✅ **Scoped to repository**: Can't access other repos
- ✅ **Temporary**: Expires after workflow completes
- ✅ **Read/Write**: Can create releases and upload files
- ✅ **No configuration needed**: Automatically available

### Secrets Management

For additional security (Slack webhooks, API keys, etc.):

1. Go to GitHub repository → **Settings** → **Secrets and variables** → **Actions**
2. Click **"New repository secret"**
3. Name: `SLACK_WEBHOOK` (or any name)
4. Value: Your secret value
5. Click **"Add secret"**

**Usage in workflow**:
```yaml
env:
  SLACK_WEBHOOK_URL: ${{ secrets.SLACK_WEBHOOK }}
```

### Branch Protection

Protect your release process:

1. Go to **Settings** → **Branches**
2. Click **"Add rule"**
3. Branch name pattern: `main`
4. Enable:
   - ✅ Require pull request reviews
   - ✅ Require status checks to pass
   - ✅ Require signed commits (optional)
5. Save changes

Now tags can only be created from protected, reviewed code.

### Signed Releases (Advanced)

Sign releases with GPG for maximum security:

```yaml
- name: Import GPG key
  uses: crazy-max/ghaction-import-gpg@v5
  with:
    gpg_private_key: ${{ secrets.GPG_PRIVATE_KEY }}
    passphrase: ${{ secrets.GPG_PASSPHRASE }}

- name: Sign firmware
  run: |
    gpg --armor --detach-sign .pio/build/esp32dev/firmware.bin
    # Creates firmware.bin.asc signature file

- name: Create Release
  uses: softprops/action-gh-release@v1
  with:
    files: |
      .pio/build/esp32dev/firmware.bin
      .pio/build/esp32dev/firmware.bin.asc
      .pio/build/esp32dev/firmware.sha256
```

---

## Team Workflow

### Developer Workflow

**Day-to-day development**:

```bash
# 1. Create feature branch
git checkout -b feature/add-weather-alerts

# 2. Make changes
vim src/main.cpp

# 3. Test locally
pio run -t upload
pio device monitor

# 4. Commit and push
git commit -am "Add weather alert category"
git push origin feature/add-weather-alerts

# 5. Create Pull Request
gh pr create --title "Add weather alert category" --body "Adds support for weather alert category with snow effect"

# 6. After PR review and approval, merge to main
# (This does NOT trigger a release yet)
```

**Creating a release**:

```bash
# 1. Pull latest main
git checkout main
git pull

# 2. Update version
vim src/defines.h  # Change to "0.2.1"

# 3. Commit version bump
git commit -am "Bump version to 0.2.1"
git push

# 4. Create and push tag
git tag v0.2.1
git push origin v0.2.1

# GitHub Actions automatically builds and releases!
```

### Release Manager Role

Designate someone to manage releases:

**Responsibilities**:
- Review pending changes for release
- Decide on version numbers (major/minor/patch)
- Create tags to trigger releases
- Monitor GitHub Actions for build success
- Verify releases are created correctly
- Track which devices have updated

**Tools**:
```bash
# View pending changes since last release
git log v0.2.0..HEAD --oneline

# See what will be in the next release
git diff v0.2.0..HEAD

# Check which releases exist
gh release list

# View release details
gh release view v0.2.1
```

### Hotfix Workflow

For urgent bug fixes:

```bash
# 1. Create hotfix branch from latest release
git checkout v0.2.1
git checkout -b hotfix/critical-mqtt-fix

# 2. Fix the bug
vim src/MQTTManager.cpp

# 3. Test thoroughly
pio run -t upload
pio device monitor

# 4. Commit fix
git commit -am "Fix MQTT reconnection crash"

# 5. Bump patch version
vim src/defines.h  # Change to "0.2.2"
git commit -am "Bump version to 0.2.2"

# 6. Merge to main
git checkout main
git merge hotfix/critical-mqtt-fix
git push

# 7. Create hotfix release
git tag v0.2.2
git push origin v0.2.2

# 8. Devices will auto-update within 24 hours
```

---

## Troubleshooting

### Build Fails: "platformio: command not found"

**Problem**: PlatformIO installation failed

**Solution**: Ensure Python setup step runs first:
```yaml
- name: Set up Python
  uses: actions/setup-python@v4
  with:
    python-version: '3.11'

- name: Install PlatformIO
  run: |
    pip install --upgrade pip
    pip install --upgrade platformio
```

### Build Fails: "Firmware too large"

**Problem**: Firmware exceeds OTA partition size (1.8 MB)

**Solutions**:

1. **Review partition scheme** in `platformio.ini`:
   ```ini
   board_build.partitions = min_spiffs.csv
   ```

2. **Enable compiler optimizations**:
   ```ini
   build_flags =
       -O2
       -DCORE_DEBUG_LEVEL=0
   ```

3. **Remove unused features**:
   ```cpp
   // Comment out unused includes
   // #include "UnusedFeature.h"
   ```

### Version Mismatch Error

**Problem**: `defines.h` version doesn't match git tag

**Error**:
```
❌ ERROR: Version mismatch!
Git tag: 0.2.1
defines.h: 0.2.0
```

**Solution**: Update `FIRMWARE_VERSION` in `src/defines.h` to match tag:
```cpp
#define FIRMWARE_VERSION          "0.2.1"
```

### Release Created But Files Missing

**Problem**: Release exists but firmware.bin not attached

**Solution**: Check file paths in workflow:
```yaml
files: |
  .pio/build/esp32dev/firmware.bin    # ← Verify this path
  .pio/build/esp32dev/firmware.sha256
```

Verify build output location:
```yaml
- name: Debug build output
  run: |
    find .pio -name "firmware.bin"
    ls -R .pio/build/
```

### Workflow Doesn't Trigger

**Problem**: Pushed tag but workflow doesn't run

**Possible causes**:

1. **Tag format wrong**: Must match `v*` pattern
   ```bash
   # ❌ Wrong
   git tag 0.2.1

   # ✅ Correct
   git tag v0.2.1
   ```

2. **Workflow file syntax error**: Check YAML syntax
   ```bash
   # Validate locally
   pip install yamllint
   yamllint .github/workflows/release-firmware.yml
   ```

3. **Actions disabled**: Check Settings → Actions → enabled

### Can't Create Release: Permission Denied

**Problem**: `403 Forbidden` when creating release

**Solution**: Ensure `GITHUB_TOKEN` has correct permissions

Add to workflow file:
```yaml
permissions:
  contents: write    # Required to create releases
  pull-requests: read
```

### Checksum Generation Fails on Windows Runners

**Problem**: `sha256sum` not available on Windows

**Solution**: Use cross-platform checksum generation:
```yaml
- name: Generate checksum (cross-platform)
  run: |
    cd .pio/build/esp32dev
    python -c "import hashlib; print(hashlib.sha256(open('firmware.bin','rb').read()).hexdigest() + '  firmware.bin')" > firmware.sha256
```

---

## Cost

### GitHub Actions Pricing

**Public Repositories**:
- ✅ **Unlimited free minutes**
- ✅ No credit card required
- ✅ All features available

**Private Repositories**:
- ✅ **2,000 minutes/month free** (all plans)
- Linux runners: $0.008/minute after free tier
- macOS runners: $0.08/minute
- Windows runners: $0.016/minute

### Typical Build Time

- **Firmware build**: 3-5 minutes per release
- **Per month estimate** (1 release/week): 12-20 minutes

**Cost for private repo**:
- Well within free tier (2,000 minutes/month)
- Even with 50 releases/month: 250 minutes (~$0 with free tier)

### Storage

- **Artifacts**: $0.25/GB/month
- **Typical firmware**: <1 MB (negligible cost)
- **Retention**: 30 days default (adjustable)

---

## Monitoring Releases

### View Release Status

**Via GitHub Web**:
1. Go to **Actions** tab
2. See all workflow runs
3. Click run for details and logs

**Via GitHub CLI**:
```bash
# List recent workflow runs
gh run list --workflow=release-firmware.yml

# View specific run details
gh run view <run-id>

# Watch run in real-time
gh run watch <run-id>
```

### Track Device Updates

Monitor which devices have updated:

```bash
# Subscribe to device telemetry
mosquitto_sub -h your-broker -t "ledSign/+/version" -v

# Expected output after update:
ledSign/aabbccddeeff/version 0.2.1
ledSign/112233445566/version 0.2.1
ledSign/ffeeddccbbaa/version 0.2.0  ← Not updated yet
```

### Release Analytics

Track release adoption over time:

```bash
# Check GitHub release download counts
gh release view v0.2.1 --json assets

# Sample output shows download count:
{
  "assets": [
    {
      "name": "firmware.bin",
      "downloadCount": 47  ← Number of downloads
    }
  ]
}
```

**Note**: Downloads from ESP32 devices count toward this number!

---

## Best Practices

### 1. Semantic Versioning

Follow semantic versioning (semver):

- **Major** (v1.0.0 → v2.0.0): Breaking changes, incompatible API
- **Minor** (v1.0.0 → v1.1.0): New features, backward compatible
- **Patch** (v1.0.0 → v1.0.1): Bug fixes, backward compatible

### 2. Version Tagging

Always use `v` prefix for tags:
```bash
git tag v0.2.1  # ✅ Correct
git tag 0.2.1   # ❌ Won't trigger workflow
```

### 3. Pre-Release Testing

Test before releasing to production:

```bash
# 1. Create beta release
git tag v0.3.0-beta.1
git push origin v0.3.0-beta.1

# 2. Test on select devices
# 3. If good, create stable release
git tag v0.3.0
git push origin v0.3.0
```

### 4. Keep Changelog

Maintain `CHANGELOG.md` for user-facing changes:

```markdown
# Changelog

## [0.2.1] - 2024-01-15
### Added
- Weather alert category with snow effect
- Improved error messages on sign display

### Fixed
- MQTT reconnection crash on WiFi dropout

## [0.2.0] - 2024-01-10
### Added
- GitHub-based OTA updates
- SHA256 checksum verification
```

### 5. Document Breaking Changes

In release notes, clearly mark breaking changes:

```markdown
## ⚠️ BREAKING CHANGES

This release requires certificate re-upload due to updated security protocols.

**Migration steps**:
1. Download new certificates from admin
2. Place in `data/certs/`
3. Run `pio run -t uploadfs`
4. Reboot device
```

---

## Summary

GitHub Actions transforms your OTA deployment process from a manual, error-prone task into a fully automated pipeline. With a single `git push` of a version tag, your firmware is:

1. ✅ Built in a clean environment
2. ✅ Validated for size and version
3. ✅ Checksummed for integrity
4. ✅ Released to GitHub
5. ✅ Automatically deployed to all devices within 24 hours

This allows you to focus on development while the CI/CD pipeline handles the operational complexity of firmware distribution.

**Next Steps**:
1. Create `.github/workflows/release-firmware.yml` in your repository
2. Commit and push the workflow file
3. Update your firmware version
4. Push a version tag: `git tag v0.2.1 && git push origin v0.2.1`
5. Watch GitHub Actions build and release automatically!

🚀 Happy automating!
