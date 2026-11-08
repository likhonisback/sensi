# 📱 Sensi Pro: Unified Cross-Platform iOS & Android Application

## 🌟 1. Overview & Architecture

**Sensi Pro** is a unified, production-ready cross-platform gaming optimization and sensitivity application engineered for **both iOS and Android** from a single, maintainable codebase.

The user interface and application engine faithfully reproduce all five reference screenshots with exact colors, typography, dimensions, icons, and 100% working interactive functionality.

---

## 🎨 2. Comprehensive Screenshot Alignment & Feature Breakdown

| Screen # | Reference Screenshot | Implemented Features & Functionality |
| :--- | :--- | :--- |
| **Screen 1** | **`GERADOR DE SENSI`** (Screenshot 1) | • Brand header with flame logo `SETE SENSI`<br>• Interactive **Operating System Switcher**: `🤖 ANDROID` vs. `🍏 IOS` (cleansed, single iOS option)<br>• **Model Field & Autocomplete**: Initially empty at launch; live searchable database for Apple, Samsung, Realme, Xiaomi/Poco, Vivo, Infinix; real device telemetry<br>• Glowing red action button: `🔥 GENERATE CONFIG` which calculates sensitivity via screen refresh rate math and navigates to Screen 2. |
| **Screen 2** | **`RECOMMENDED SENSI`** (Screenshot 2) | • Target device header: `📱 IPHONE 15 PRO` (or selected device)<br>• Header subtitle: `RECOMMENDED SENSI 🎯`<br>• **Tactical Preset Selectors**: `Balanced`, `Drag Headshot`, `Precision Aim`, `Fast Flick`, `Long Range`<br>• **Precise Sliders (0–200 scale)**: General (186), Red Dot (172), 2x Scope (161), 4x Scope (175)<br>• Live value feedback and real-time adjustment<br>• High DPI options: `No DPI`, `400 DPI`, `600 DPI`, `700 DPI`, `800 DPI`, `960 DPI`<br>• **RED.txt Export Engine**: Live preview, one-click copy, and file download (`RED.txt`). |
| **Screen 3** | **`SENSI PRO & BOOSTER`** (Screenshot 3) | • Header: `🛡️ SENSI PRO` logo with functional hamburger menu `☰`<br>• **Real Hardware RAM Usage Card**: Displays detected brand, overall RAM usage, dynamic progress bar, Total & Used memory metrics<br>• **Rocket Cleaner (`🚀`)**: Interactive RAM cleaning animation that clears garbage memory and updates metrics<br>• **Interactive Feature Toggles (Default OFF)**: `⚡ SENSITIVITY PHONE`, `🎯 CROSSHAIR AIM`, `📶 PING PHONE` (measures network latency)<br>• Primary Action: `🎮 OPEN GAME` (triggers Free Fire launch intent). |
| **Screen 4** | **`CROSSHAIR AIM OVERLAY`** (Screenshot 4) | • Top status bar and `SENSI /` header<br>• Master Toggle: `❖ CROSSHAIR` (`ON` / `OFF` blue pill button)<br>• **Interactive Sliders**: `⛶ SIZE` (12–100px), `◑ OPACITY` (20–100%), `⟳ ROTATION` (0–360°)<br>• **Live Interactive Crosshair Canvas**: Real-time SVG rendering updating smoothly on every adjustment<br>• **Color Picker**: White, Cyan, Red, Green, Yellow<br>• **Style Switcher**: Classic 4-Bar, Precision Dot, X-Cross, Circle-Dot, Cyber Diamond<br>• **Positioning D-Pad**: Directional arrows (▲, ▼, ◀, ▶) and center reset for pixel-accurate crosshair placement (`X: 0, Y: 0`). |
| **Screen 5** | **`HEADSHOT SENSI PRO AI`** (Screenshot 5) | • Skull logo `Headshot Sensi PRO TRAINING`<br>• Device badge with live battery percentage<br>• **Detected Device Card**: Real hardware telemetry and battery status<br>• Action Button: `🔄 REFRESH DEVICE INFO`<br>• **PRO SENSITIVITY - AI**: Live date badge, searchable device input, 6-button memory allocation grid (`4 GB`, `6 GB`, `8 GB`, `12 GB`, `16 GB`, `24 GB`), and `🎯 GENERATE SENSITIVITY ▶` button. |
| **Drawer & Settings** | **Navigation Drawer & Settings** | • **Top-Right Hamburger (`☰`)**: Opens smooth sliding drawer navigating to Settings, Recommended Sensi, Sensi Phone, Theme Phone, RAM & Device Info, Crosshair HUD, AI Pro, and M Hard.<br>• **Default Language**: English across entire application with persistent `localStorage`.<br>• **10-Language Selector**: English, Bengali, Hindi, French, Nepali, Arabic (with automatic `dir="rtl"` direction), Spanish, Portuguese, Indonesian, Chinese.<br>• **Theme Engine**: Default Dark, Crimson Flame, Cyberpunk Neon, AMOLED Black.<br>• **Dedicated Configuration Views**: Sensitivity Phone (X/Y touch multiplier), M Hard (aggression coefficient & stabilization). |

---

## 🤖 3. Android Build & Deployment

* **Platform Target**: Android (API 24+ / Android 7.0 through Android 15+)
* **Architecture**: Universal (ARM64, ARMv7, x86_64)
* **Output Artifact**:
  ```
  build/Android/SensiPro.apk
  ```
* **Native Android Integration**:
  * Native [`android/app/src/main/java/com/sensipro/app/MainActivity.java`](file:///C:/Users/Administrator/Desktop/iphone%20sensi/android/app/src/main/java/com/sensipro/app/MainActivity.java) hosting hardware-accelerated WebView.
  * Direct JavaScript bridge (`AndroidBridge`):
    * `getDeviceModel()`: returns real manufacturer & model via `Build.MODEL`.
    * `getBatteryLevel()`: returns live battery percentage via `BatteryManager`.
    * `getMemoryInfo()`: returns real RAM usage via `ActivityManager.MemoryInfo`.
    * `triggerHaptic()`: triggers physical device vibration.
    * `launchGame()`: fires launch intent for `com.dts.freefireth` / `com.dts.freefiremax`.
* **Building Android APK**:
  Run from project root:
  ```powershell
  powershell.exe -ExecutionPolicy Bypass -File .\build_android.ps1
  ```
  or double-click:
  ```cmd
  build_android_apk.bat
  ```
* **Signature Verification**:
  Verified via Google `apksigner`:
  * Verified using v2 scheme: **true**
  * Verified using v3 scheme: **true**

---

## 🍏 4. iOS Build & Deployment

* **Platform Target**: Apple iOS (iOS 15.0+ / iPhone & iPad)
* **Architecture**: arm64 (Native Mach-O 64-bit executable)
* **Target Output Artifact**:
  ```
  build/iOS/SensiPanel.ipa
  ```
* **Native iOS Implementation**:
  * [`SensiPanel.xcodeproj`](file:///C:/Users/Administrator/Desktop/iphone%20sensi/SensiPanel.xcodeproj/project.pbxproj) configured for Xcode 14, 15, and 16.
  * [`ios/ViewController.mm`](file:///C:/Users/Administrator/Desktop/iphone%20sensi/ios/ViewController.mm) integrating WebKit, script message handlers, native hardware model identification via `uname`, and haptic feedback (`UIImpactFeedbackGenerator`).
  * Resource phase automatically embeds `index.html` and app icons into the application bundle.
* **macOS & Xcode Toolchain Requirement**:
  * Genuine iOS applications require a compiled 64-bit ARM Mach-O executable, an embedded provisioning profile (`embedded.mobileprovision`), and an Apple Developer Code Signature (`_CodeSignature/CodeResources`).
  * Because Windows does not possess the Apple Darwin/Clang toolchain, an installable IPA cannot be generated natively on Windows.
* **Building Genuine SensiPanel.ipa on macOS**:
  1. Transfer project to a macOS machine with Xcode installed.
  2. Run the automated build script:
     ```bash
     chmod +x ios/build_ipa.sh
     ./ios/build_ipa.sh
     ```
  3. Or open `SensiPanel.xcodeproj` in Xcode, select your Apple Developer Team in **Signing & Capabilities**, and select **Product ➔ Archive**.
* **Remote Mac Automated Build from Windows**:
  ```powershell
  powershell -ExecutionPolicy Bypass -File .\ios\build_remote.ps1 -MacHost <mac_ip> -MacUser <username>
  ```
* **Signing, Provisioning & Installation**:
  * Requires an Apple Developer Account (free or paid) to provision a profile for bundle ID `com.sensi.panel.ios`.
  * Once compiled on macOS, install via **Xcode**, **Apple Configurator**, **AltStore**, or **Sideloadly**.

---

## 🌐 5. Cross-Platform Web & Desktop Compatibility

* **Direct Web / PWA**:
  Open [`index.html`](file:///C:/Users/Administrator/Desktop/iphone%20sensi/index.html) in any modern browser (Chrome, Safari, Edge, Firefox) on desktop or mobile.
* **Responsive Layout**:
  Automatically adapts to full-screen mobile dimensions (390px to 430px mobile canvas or edge-to-edge on physical mobile devices).
* **Navigation Bar**:
  Top pill tabs and bottom navigation bar allow fluid, 1-click switching across all 5 reference screens.
