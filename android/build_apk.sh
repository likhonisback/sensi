#!/usr/bin/env bash
set -e

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
ANDROID_DIR="$PROJECT_ROOT/android"
BUILD_DIR="$ANDROID_DIR/build"
OUTPUT_DIR="$PROJECT_ROOT/build/Android"
OUTPUT_APK="$OUTPUT_DIR/SensiPro.apk"

echo "=========================================================="
echo " Building Sensi Pro Android APK (Linux / CI Toolchain)"
echo " Output: $OUTPUT_APK"
echo "=========================================================="

mkdir -p "$OUTPUT_DIR"
rm -rf "$BUILD_DIR"
mkdir -p "$BUILD_DIR/gen" "$BUILD_DIR/obj" "$BUILD_DIR/dex"

# Sync assets
mkdir -p "$ANDROID_DIR/app/src/main/assets"
cp "$PROJECT_ROOT/index.html" "$ANDROID_DIR/app/src/main/assets/index.html"

# Discover Android SDK tools
ANDROID_SDK_ROOT="${ANDROID_HOME:-$ANDROID_SDK_ROOT}"
if [ -z "$ANDROID_SDK_ROOT" ]; then
    echo "ERROR: ANDROID_HOME or ANDROID_SDK_ROOT not set"
    exit 1
fi

BUILD_TOOLS_DIR=$(find "$ANDROID_SDK_ROOT/build-tools" -maxdepth 1 -mindepth 1 | sort -V | tail -n 1)
PLATFORM_DIR=$(find "$ANDROID_SDK_ROOT/platforms" -maxdepth 1 -mindepth 1 | sort -V | tail -n 1)

AAPT2="$BUILD_TOOLS_DIR/aapt2"
D8="$BUILD_TOOLS_DIR/d8"
ZIPALIGN="$BUILD_TOOLS_DIR/zipalign"
APKSIGNER="$BUILD_TOOLS_DIR/apksigner"
ANDROID_JAR="$PLATFORM_DIR/android.jar"

echo "Using Build-Tools: $BUILD_TOOLS_DIR"
echo "Using Android Jar: $ANDROID_JAR"

# 1. Compile Resources
"$AAPT2" compile --dir "$ANDROID_DIR/app/src/main/res" -o "$BUILD_DIR/res.zip"

# 2. Link Resources
"$AAPT2" link -I "$ANDROID_JAR" \
    --manifest "$ANDROID_DIR/app/src/main/AndroidManifest.xml" \
    -A "$ANDROID_DIR/app/src/main/assets" \
    -o "$BUILD_DIR/unaligned.apk" \
    --java "$BUILD_DIR/gen" \
    "$BUILD_DIR/res.zip" \
    --auto-add-overlay

# 3. Compile Java Sources
JAVA_FILES=(
    "$BUILD_DIR/gen/com/sensipro/app/R.java"
    "$ANDROID_DIR/app/src/main/java/com/sensipro/app/MainActivity.java"
    "$ANDROID_DIR/app/src/main/java/com/sensipro/app/FloatingGuideService.java"
    "$ANDROID_DIR/app/src/main/java/com/sensipro/app/AimHudService.java"
)
javac --release 17 -encoding UTF-8 -d "$BUILD_DIR/obj" -cp "$ANDROID_JAR" "${JAVA_FILES[@]}"

# 4. Dex Bytecode
CLASS_FILES=$(find "$BUILD_DIR/obj" -name "*.class")
"$D8" --output "$BUILD_DIR/dex" $CLASS_FILES --lib "$ANDROID_JAR"

# 5. Add classes.dex into unaligned.apk
(cd "$BUILD_DIR/dex" && jar -uf "$BUILD_DIR/unaligned.apk" classes.dex)

# 6. Zipalign & Sign
"$ZIPALIGN" -f -p 4 "$BUILD_DIR/unaligned.apk" "$BUILD_DIR/aligned.apk"

KEYSTORE="$ANDROID_DIR/debug.keystore"
if [ ! -f "$KEYSTORE" ]; then
    keytool -genkey -v -keystore "$KEYSTORE" -storepass android -alias androiddebugkey -keypass android -keyalg RSA -keysize 2048 -validity 10000 -dname "CN=Android Debug,O=Android,C=US"
fi

"$APKSIGNER" sign --ks "$KEYSTORE" --ks-pass "pass:android" --out "$OUTPUT_APK" "$BUILD_DIR/aligned.apk"

if [ -f "$OUTPUT_APK" ]; then
    APK_SIZE=$(stat -c%s "$OUTPUT_APK" 2>/dev/null || stat -f%z "$OUTPUT_APK" 2>/dev/null || echo "0")
    echo "=========================================================="
    echo " [SUCCESS] SensiPro.apk generated successfully: $APK_SIZE bytes"
    echo " Path: $OUTPUT_APK"
    echo "=========================================================="
fi
