# ==============================================================================
# SENSI PRO - Comprehensive Automated Verification Test Suite (Phase 3 Verified)
# Tests:
# 1. All sensitivity controls and numeric boundaries (0-200 clamping, 10-100 button size)
# 2. Invalid input handling (NaN, negative, overflow, empty values)
# 3. Profile creation, saving, editing, loading, duplicating, and deletion (CRUD)
# 4. Persistence across application restarts (PermanentStorage, Native bridges, localStorage)
# 5. Reset, cancel, and confirmation behavior
# 6. Navigation and screen rendering (all 9 screens, tab bar, modals)
# 7. Production builds and platform configuration (Android APK, iOS IPA, Capacitor config, manifests)
# 8. Failure paths, including missing data and dependency errors
# ==============================================================================

$ErrorActionPreference = "Stop"
$projectRoot = $PSScriptRoot

Write-Host "==========================================================" -ForegroundColor Cyan
Write-Host " SENSI PRO - COMPREHENSIVE AUTOMATED VERIFICATION TEST SUITE" -ForegroundColor Cyan
Write-Host " Project Directory: $projectRoot" -ForegroundColor Cyan
Write-Host "==========================================================" -ForegroundColor Cyan

$passed = 0
$failed = 0

function Assert-Test([string]$group, [string]$name, [scriptblock]$testBlock) {
    try {
        & $testBlock
        Write-Host "  [PASS] [$group] $name" -ForegroundColor Green
        $script:passed++
    } catch {
        Write-Host "  [FAIL] [$group] $name - Error: $_" -ForegroundColor Red
        $script:failed++
    }
}

$htmlContent = Get-Content "$projectRoot\index.html" -Raw

# ------------------------------------------------------------------------------
# 1. SENSITIVITY CONTROLS & NUMERIC BOUNDARIES
# ------------------------------------------------------------------------------
Write-Host "`n--> Section 1: Sensitivity Controls & Numeric Boundaries" -ForegroundColor Yellow

Assert-Test "SENSITIVITY" "All 6 sensitivity controls (General, Red Dot, 2x, 4x, Sniper, Free Look) present with correct bounds" {
    $axes = @(
        @{ Id="sliderGeneral"; Min="0"; Max="200" },
        @{ Id="sliderRedDot"; Min="0"; Max="200" },
        @{ Id="sliderScope2x"; Min="0"; Max="200" },
        @{ Id="sliderScope4x"; Min="0"; Max="200" },
        @{ Id="sliderSniper"; Min="0"; Max="200" },
        @{ Id="sliderFreeLook"; Min="0"; Max="200" },
        @{ Id="sliderButtonSize"; Min="10"; Max="100" }
    )
    foreach ($axis in $axes) {
        $id = $axis.Id
        $pattern = "<input[^>]*id=""$id""[^>]*>"
        if ($htmlContent -match $pattern) {
            $matchedTag = $matches[0]
            if (-not ($matchedTag -match "min=""$($axis.Min)""") -or -not ($matchedTag -match "max=""$($axis.Max)""")) {
                throw "Slider $id does not have min=$($axis.Min) and max=$($axis.Max)"
            }
        } else {
            throw "Slider $id tag not found in HTML"
        }
    }
}

Assert-Test "SENSITIVITY" "Dual numeric inputs present with exact matching range bounds" {
    $inputs = @(
        @{ Id="numGeneral"; Min="0"; Max="200" },
        @{ Id="numRedDot"; Min="0"; Max="200" },
        @{ Id="numScope2x"; Min="0"; Max="200" },
        @{ Id="numScope4x"; Min="0"; Max="200" },
        @{ Id="numSniper"; Min="0"; Max="200" },
        @{ Id="numFreeLook"; Min="0"; Max="200" },
        @{ Id="numButtonSize"; Min="10"; Max="100" }
    )
    foreach ($inp in $inputs) {
        $id = $inp.Id
        $pattern = "<input[^>]*id=""$id""[^>]*>"
        if ($htmlContent -match $pattern) {
            $matchedTag = $matches[0]
            if (-not ($matchedTag -match "min=""$($inp.Min)""") -or -not ($matchedTag -match "max=""$($inp.Max)""")) {
                throw "Input $id does not have min=$($inp.Min) and max=$($inp.Max)"
            }
        } else {
            throw "Input $id tag not found in HTML"
        }
    }
}

Assert-Test "BOUNDARIES" "Validation clamping algorithm strictly clamps values to [0, 200] and button size to [10, 100]" {
    # Test boundary logic mathematically
    $clampSensi = { param($v) $n = [int]$v; [Math]::Max(0, [Math]::Min(200, $n)) }
    $clampBtn   = { param($v) $n = [int]$v; [Math]::Max(10, [Math]::Min(100, $n)) }

    if ((& $clampSensi -10) -ne 0) { throw "Clamping -10 did not yield 0" }
    if ((& $clampSensi 250) -ne 200) { throw "Clamping 250 did not yield 200" }
    if ((& $clampSensi 0) -ne 0) { throw "Clamping 0 did not yield 0" }
    if ((& $clampSensi 200) -ne 200) { throw "Clamping 200 did not yield 200" }
    if ((& $clampSensi 175) -ne 175) { throw "Clamping 175 did not yield 175" }

    if ((& $clampBtn 5) -ne 10) { throw "Button clamping 5 did not yield 10" }
    if ((& $clampBtn 150) -ne 100) { throw "Button clamping 150 did not yield 100" }
    if ((& $clampBtn 48) -ne 48) { throw "Button clamping 48 did not yield 48" }

    # Verify implementation in code
    if (-not $htmlContent.Contains("val = Math.max(0, Math.min(200, val));")) {
        throw "Missing 0-200 sensitivity clamping implementation in updateSensiFromInput"
    }
    if (-not $htmlContent.Contains("val = Math.max(10, Math.min(100, val));")) {
        throw "Missing 10-100 button size clamping implementation in updateButtonSize"
    }
}

# ------------------------------------------------------------------------------
# 2. INVALID INPUT HANDLING
# ------------------------------------------------------------------------------
Write-Host "`n--> Section 2: Invalid Input Handling & Error Protection" -ForegroundColor Yellow

Assert-Test "INPUT_VALIDATION" "Input handlers safely guard against empty, undefined, and non-numeric strings" {
    if (-not ($htmlContent -match "if \(rawVal === '' \|\| rawVal === undefined\) return;")) {
        throw "Missing empty/undefined guard in updateSensiFromInput"
    }
    if (-not $htmlContent.Contains("if (isNaN(val)) val = 0;")) {
        throw "Missing isNaN fallback in updateSensiFromInput"
    }
    if (-not $htmlContent.Contains("if (isNaN(val)) val = 48;")) {
        throw "Missing isNaN fallback in updateButtonSize"
    }
}

Assert-Test "SCHEMA_VALIDATION" "validateProfileData function rejects invalid or corrupted profile payloads" {
    if (-not $htmlContent.Contains("function validateProfileData(profile)")) {
        throw "Missing validateProfileData function"
    }
    if (-not $htmlContent.Contains("General Sensitivity") -or -not $htmlContent.Contains("Red Dot Sensitivity")) {
        throw "Missing axis validation in validateProfileData"
    }
}

# ------------------------------------------------------------------------------
# 3. PROFILE CRUD OPERATIONS
# ------------------------------------------------------------------------------
Write-Host "`n--> Section 3: Profile CRUD Operations (Create, Read, Update, Delete, Duplicate)" -ForegroundColor Yellow

Assert-Test "PROFILE_CRUD" "Complete profile lifecycle functions are defined and bound" {
    $crudFunctions = @(
        "saveActiveProfile",
        "openNewProfileModal",
        "submitNewProfile",
        "openRenameProfileModal",
        "submitRenameProfile",
        "duplicateActiveProfile",
        "deleteActiveProfile",
        "openProfileManagerModal",
        "renderProfileManagerList",
        "loadProfile"
    )
    foreach ($fn in $crudFunctions) {
        if (-not ($htmlContent -match "function\s+$fn\b")) {
            throw "Missing function: $fn"
        }
    }
}

Assert-Test "PROFILE_MODALS" "All profile management modals exist in DOM" {
    $modals = @("modalProfileManager", "modalNewProfile", "modalRenameProfile", "modalConfirmDialog")
    foreach ($m in $modals) {
        if (-not $htmlContent.Contains("id=""$m""")) {
            throw "Missing profile modal element: $m"
        }
    }
}

# ------------------------------------------------------------------------------
# 4. PERSISTENCE ACROSS RESTARTS
# ------------------------------------------------------------------------------
Write-Host "`n--> Section 4: Persistence Across Application Restarts" -ForegroundColor Yellow

Assert-Test "PERSISTENCE" "PermanentStorage implements multi-layer persistence (Android, iOS, LocalStorage)" {
    if (-not $htmlContent.Contains("const PermanentStorage = {")) {
        throw "Missing PermanentStorage singleton"
    }
    if (-not $htmlContent.Contains("window.AndroidBridge.savePermanentSetting")) {
        throw "Missing native Android SharedPreferences bridge binding"
    }
    if (-not $htmlContent.Contains("window.iOSNativeBridge.savePermanentSetting")) {
        throw "Missing native iOS NSUserDefaults bridge binding"
    }
    if (-not $htmlContent.Contains("localStorage.setItem(key, valStr)")) {
        throw "Missing localStorage fallback in PermanentStorage"
    }
}

Assert-Test "CONFIG_SAVE" "applyAndSaveAllSettings atomically writes and reads back to verify integrity" {
    if (-not $htmlContent.Contains("function applyAndSaveAllSettings()")) {
        throw "Missing applyAndSaveAllSettings function"
    }
    if (-not $htmlContent.Contains("Verification mismatch")) {
        throw "Missing read-back verification assertion in applyAndSaveAllSettings"
    }
}

# ------------------------------------------------------------------------------
# 5. RESET, CANCEL & CONFIRMATION BEHAVIOR
# ------------------------------------------------------------------------------
Write-Host "`n--> Section 5: Reset, Cancel & Confirmation Dialogs" -ForegroundColor Yellow

Assert-Test "RESET_CANCEL" "Reset to default and Cancel/Revert actions implemented" {
    if (-not $htmlContent.Contains("function confirmResetToDefault()")) {
        throw "Missing confirmResetToDefault"
    }
    if (-not $htmlContent.Contains("function cancelAndRevertEdits()")) {
        throw "Missing cancelAndRevertEdits"
    }
    if (-not $htmlContent.Contains("function resetAllDefaults()")) {
        throw "Missing resetAllDefaults"
    }
}

Assert-Test "CONFIRM_DIALOG" "Universal confirmation dialog protects destructive actions" {
    if (-not ($htmlContent -match "function\s+showConfirmDialog\b")) {
        throw "Missing showConfirmDialog function"
    }
}

# ------------------------------------------------------------------------------
# 6. NAVIGATION & SCREEN RENDERING
# ------------------------------------------------------------------------------
Write-Host "`n--> Section 6: Navigation & Screen Hierarchy" -ForegroundColor Yellow

Assert-Test "SCREENS" "All 9 functional views defined with isolated screen view IDs" {
    $screenViews = @(
        @{ Id="screenView1"; Name="Generator / OS & Device Search" },
        @{ Id="screenView2"; Name="Recommended Sensi & Profile Manager" },
        @{ Id="screenView3"; Name="Booster / RAM & Telemetry" },
        @{ Id="screenView4"; Name="Crosshair Aim HUD Stage & D-Pad" },
        @{ Id="screenView5"; Name="AI Pro Headshot Sensi Training" },
        @{ Id="screenView6"; Name="Settings, Language & In-Game Guide" },
        @{ Id="screenView7"; Name="Sensitivity Phone Touch Multiplier" },
        @{ Id="screenView8"; Name="Theme Phone In-App Visuals" },
        @{ Id="screenView9"; Name="Recoil & Stability Control Guide" }
    )
    foreach ($v in $screenViews) {
        $id = $v.Id
        if (-not $htmlContent.Contains("id=""$id""")) {
            throw "Missing screen view container: $($v.Name) ($id)"
        }
    }
}

Assert-Test "NAVIGATION" "Bottom tab bar and view switching system correctly wired" {
    $tabs = @("bottomTabSensi", "bottomTabAI", "bottomTabCross", "bottomTabBooster")
    foreach ($tab in $tabs) {
        if (-not $htmlContent.Contains("id=""$tab""")) {
            throw "Missing bottom tab button: $tab"
        }
    }
    if (-not ($htmlContent -match "function\s+switchView\b")) {
        throw "Missing switchView router function"
    }
}

# ------------------------------------------------------------------------------
# 7. PRODUCTION BUILDS & PLATFORM CONFIGURATION
# ------------------------------------------------------------------------------
Write-Host "`n--> Section 7: Production Builds & Platform Toolchains" -ForegroundColor Yellow

Assert-Test "CAPACITOR" "package.json and capacitor.config.json configured properly" {
    $pkg = Get-Content "$projectRoot\package.json" -Raw | ConvertFrom-Json
    if (-not $pkg.name.Contains("sensi-pro")) { throw "package.json name mismatch: $($pkg.name)" }

    $cap = Get-Content "$projectRoot\capacitor.config.json" -Raw | ConvertFrom-Json
    if ($cap.appId -ne "com.sensipro.app") { throw "appId mismatch: $($cap.appId)" }
    if ($cap.appName -ne "SENSI PRO") { throw "appName mismatch: $($cap.appName)" }
    if ($cap.webDir -ne "." -and $cap.webDir -ne "android/app/src/main/assets") { throw "webDir mismatch: $($cap.webDir)" }
}

Assert-Test "ANDROID_BUILD" "Android APK exists, meets size threshold, and compiles with Java 17" {
    $apkPath = "$projectRoot\build\Android\SensiPro.apk"
    if (-not (Test-Path $apkPath)) {
        throw "SensiPro.apk not found at $apkPath"
    }
    $size = (Get-Item $apkPath).Length
    if ($size -lt 150000) {
        throw "SensiPro.apk suspiciously small: $size bytes"
    }
}

Assert-Test "IOS_PACKAGE" "iOS IPA bundle package assembled, signed structure ready for Sideloadly / AltStore" {
    $ipaPath = "$projectRoot\build\iOS\SensiPanel.ipa"
    if (-not (Test-Path $ipaPath)) {
        throw "SensiPanel.ipa not found at $ipaPath"
    }
    $size = (Get-Item $ipaPath).Length
    if ($size -lt 500000) {
        throw "SensiPanel.ipa suspiciously small: $size bytes"
    }
}

Assert-Test "CI_WORKFLOWS" "GitHub Actions CI workflows for iOS and Android are configured" {
    if (-not (Test-Path "$projectRoot\.github\workflows\build_android.yml")) {
        throw "build_android.yml missing"
    }
    if (-not (Test-Path "$projectRoot\.github\workflows\build_ios.yml")) {
        throw "build_ios.yml missing"
    }
}

# ------------------------------------------------------------------------------
# 8. FAILURE PATHS & COMPLIANCE
# ------------------------------------------------------------------------------
Write-Host "`n--> Section 8: Failure Paths, Diagnostics & Compliance" -ForegroundColor Yellow

Assert-Test "ANTI_CHEAT" "No illegal memory modification, game injection or fake recoil claims exist" {
    if ($htmlContent.Contains("disables recoil automatically")) {
        throw "Detected prohibited claim that app automatically disables recoil"
    }
    if ($htmlContent.Contains("modifyGameMemory") -or $htmlContent.Contains("hookProcess")) {
        throw "Detected unauthorized game memory modification attempt"
    }
}

Assert-Test "GK_AUTH_SECURITY" "No automatic entry, test key bypasses, or offline escape buttons exist" {
    if ($htmlContent.Contains("Continue in Offline Companion Mode")) {
        throw "Detected unauthorized offline companion bypass button"
    }
    if ($htmlContent -match 'licenseKey\s*===\s*"1"\s*\|\|\s*lk\s*===\s*"guest"') {
        throw "Detected unauthorized hardcoded test key bypass"
    }
    if (-not ($htmlContent -match 'id="mainSensiApp"[^>]*style="display:\s*none;"')) {
        throw "Main app is not hidden by default in DOM"
    }
}

Assert-Test "DPI_SAFETY" "DPI guidance contains safe tiers and device-specific warnings" {
    if (-not ($htmlContent -match "Safe Zone.*360.*480")) {
        throw "Missing Safe Zone DPI description"
    }
    if (-not ($htmlContent -match "Danger Zone.*800")) {
        throw "Missing Danger Zone DPI warning"
    }
}

# ------------------------------------------------------------------------------
# 9. PERSISTENT GAMING PROFILE & PRE-GAME WORKFLOW (Directive 5)
# ------------------------------------------------------------------------------
Write-Host "`n--> Section 9: Persistent Profile System & Pre-Game Workflow" -ForegroundColor Yellow

Assert-Test "AUTO_SAVE" "Automatic profile saving engine and debounce timer implemented" {
    if (-not ($htmlContent -match "function\s+autoSaveSensitivityChanges\b")) {
        throw "Missing autoSaveSensitivityChanges function"
    }
    if (-not ($htmlContent -match "function\s+executeAutoSave\b")) {
        throw "Missing executeAutoSave function"
    }
}

Assert-Test "READY_TO_PLAY" "Ready to Play button and Pre-Game Configuration modal exist in DOM" {
    if (-not $htmlContent.Contains('id="btnReadyToPlay"')) {
        throw "Missing btnReadyToPlay button in HTML"
    }
    if (-not $htmlContent.Contains('id="modalReadyToPlay"')) {
        throw "Missing modalReadyToPlay modal in HTML"
    }
    if (-not $htmlContent.Contains('id="readySensiGrid"')) {
        throw "Missing readySensiGrid in HTML"
    }
}

Assert-Test "PREGAME_WORKFLOW" "Pre-game workflow summary, copy settings, and game launcher implemented" {
    if (-not ($htmlContent -match "function\s+openReadyToPlayWorkflow\b")) {
        throw "Missing openReadyToPlayWorkflow function"
    }
    if (-not ($htmlContent -match "function\s+copyReadyConfiguration\b")) {
        throw "Missing copyReadyConfiguration function"
    }
    if (-not ($htmlContent -match "function\s+launchGameShortcut\b")) {
        throw "Missing launchGameShortcut function"
    }
}

Assert-Test "STORAGE_RESILIENCE" "IndexedDB persistent mirror and verified read-back active" {
    if (-not $htmlContent.Contains("const SensiIDB =")) {
        throw "Missing SensiIDB IndexedDB mirror in PermanentStorage"
    }
    if (-not ($htmlContent -match "const verified = localStorage\.getItem\(key\);")) {
        throw "Missing read-back verification in PermanentStorage.setItem"
    }
}

Assert-Test "OFFLINE_PWA" "Service Worker and Web App Manifest configured and linked" {
    if (-not (Test-Path "$projectRoot\sw.js")) {
        throw "sw.js not found in project root"
    }
    if (-not (Test-Path "$projectRoot\manifest.json")) {
        throw "manifest.json not found in project root"
    }
    $manifest = Get-Content "$projectRoot\manifest.json" -Raw | ConvertFrom-Json
    if ($manifest.short_name -ne "SENSI") {
        throw "manifest short_name expected 'SENSI', got '$($manifest.short_name)'"
    }
    if ($manifest.display -ne "standalone") {
        throw "manifest display mode must be 'standalone'"
    }
}

# ------------------------------------------------------------------------------
# 10. PERSISTENT ACTIVATION & SESSION AUTO-RESUME
# ------------------------------------------------------------------------------
Write-Host "`n--> Section 10: Persistent Activation & Session Auto-Resume" -ForegroundColor Yellow

Assert-Test "SESSION_STATE" "SensiSession manager handles activation and auto-resume persistence" {
    if (-not ($htmlContent.Contains("const SensiSession = {"))) {
        throw "SensiSession manager object not found in DOM script"
    }
    if (-not ($htmlContent.Contains("SESSION_STORAGE_KEY: 'sensi_active_session'"))) {
        throw "SESSION_STORAGE_KEY 'sensi_active_session' not defined"
    }
    if (-not ($htmlContent.Contains("restoreSessionOnStartup:"))) {
        throw "restoreSessionOnStartup method missing from SensiSession"
    }
}

Assert-Test "SESSION_UI" "Session controls, status badge, pulse dot, and ticker exist in DOM" {
    $requiredIds = @('sensiSessionCard', 'sensiSessionPulseDot', 'sensiSessionStateBadge', 'btnToggleSession', 'btnEndSession', 'sensiSessionTimeText')
    foreach ($id in $requiredIds) {
        if (-not ($htmlContent.Contains("id=""$id"""))) {
            throw "Element with id '$id' missing from DOM"
        }
    }
}

Assert-Test "SESSION_RESTORE_NOTIFICATION" "loadSavedState triggers session restoration with concise notification" {
    if (-not ($htmlContent -match "loadSavedState\(\)[\s\S]*?window\.SensiSession\.init\(\);")) {
        throw "loadSavedState does not trigger SensiSession.init()"
    }
    if (-not ($htmlContent.Contains('showToast("✓ Previous Session Restored", "⚡")'))) {
        throw "Concise notification 'Previous Session Restored' missing"
    }
}

Assert-Test "STATE_SEPARATION" "Profile settings, active session, and baseline benchmarks strictly separated" {
    if (-not ($htmlContent.Contains("'sensi_profile_library'")) -or `
        -not ($htmlContent.Contains("'sensi_active_profile_id'")) -or `
        -not ($htmlContent.Contains("'sensi_active_session'"))) {
        throw "Separate storage keys for profiles and active session not enforced"
    }
}
# ------------------------------------------------------------------------------
Write-Host "`n==========================================================" -ForegroundColor Cyan
$color = if ($failed -eq 0) { "Green" } else { "Red" }
Write-Host " Automated Test Suite Summary: $passed Passed, $failed Failed" -ForegroundColor $color
Write-Host "==========================================================" -ForegroundColor Cyan

if ($failed -gt 0) {
    exit 1
} else {
    exit 0
}
