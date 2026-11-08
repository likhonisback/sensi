#!/usr/bin/env bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PROJECT_ROOT/build/iOS"

echo "=========================================================="
echo " Starting Genuine iOS Sensi Pro Build Workflow"
echo " Target: $BUILD_DIR/SensiPanel.ipa"
echo "=========================================================="

if [[ "$(uname)" != "Darwin" ]]; then
    echo "ERROR: Xcode build toolchain requires macOS environment."
    echo "Current Operating System: $(uname -s)"
    exit 1
fi

if ! command -v xcodebuild &> /dev/null; then
    echo "ERROR: xcodebuild command not found. Please install Xcode."
    exit 1
fi

mkdir -p "$BUILD_DIR"
cd "$PROJECT_ROOT"

ARCHIVE_PATH="$BUILD_DIR/SensiPanel.xcarchive"
rm -rf "$ARCHIVE_PATH"

echo "--> Archiving iOS Project..."
if [ -n "$DEVELOPMENT_TEAM" ]; then
    xcodebuild archive \
        -project SensiPanel.xcodeproj \
        -scheme SensiPanel \
        -configuration Release \
        -destination "generic/platform=iOS" \
        -archivePath "$ARCHIVE_PATH" \
        DEVELOPMENT_TEAM="$DEVELOPMENT_TEAM"
    
    echo "--> Exporting Signed IPA Package..."
    xcodebuild -exportArchive \
        -archivePath "$ARCHIVE_PATH" \
        -exportOptionsPlist "$SCRIPT_DIR/ExportOptions.plist" \
        -exportPath "$BUILD_DIR"
else
    echo "--> Building Release Binary for iOS Device (arm64)..."
    xcodebuild build \
        -project SensiPanel.xcodeproj \
        -scheme SensiPanel \
        -configuration Release \
        -sdk iphoneos \
        -destination "generic/platform=iOS" \
        CODE_SIGNING_ALLOWED=NO \
        CONFIGURATION_BUILD_DIR="$BUILD_DIR/Payload"

    echo "--> Packaging IPA Structure for Sideloading (AltStore / Sideloadly / TrollStore)..."
    cd "$BUILD_DIR"
    rm -f SensiPanel.ipa
    zip -r SensiPanel.ipa Payload/
fi

if [ -f "$BUILD_DIR/SensiPanel.ipa" ]; then
    IPA_SIZE=$(stat -f%z "$BUILD_DIR/SensiPanel.ipa" 2>/dev/null || stat -c%s "$BUILD_DIR/SensiPanel.ipa" 2>/dev/null || echo "Unknown")
    echo "SUCCESS: SensiPanel.ipa generated at $BUILD_DIR/SensiPanel.ipa ($IPA_SIZE bytes)"
fi

echo "=========================================================="
echo " iOS Build Process Completed"
echo "=========================================================="
