# ==============================================================================
# SENSI PRO - Animation System, Status Feedback & Performance Verification Suite
# ==============================================================================
$ErrorActionPreference = "Stop"
$projectRoot = $PSScriptRoot

Write-Host "==========================================================" -ForegroundColor Cyan
Write-Host " SENSI PRO - ANIMATION SYSTEM & PERFORMANCE TEST SUITE" -ForegroundColor Cyan
Write-Host "==========================================================" -ForegroundColor Cyan

$html = Get-Content "$projectRoot\index.html" -Raw

$testsPassed = 0
$testsFailed = 0

function Assert-Check([string]$name, [bool]$condition, [string]$err) {
    if ($condition) {
        Write-Host "  [PASS] $name" -ForegroundColor Green
        $script:testsPassed++
    } else {
        Write-Host "  [FAIL] $name - $err" -ForegroundColor Red
        $script:testsFailed++
    }
}

# ---------------------------------------------------------
# 1. Hardware-Accelerated Button Micro-Interactions
# ---------------------------------------------------------
Assert-Check "Buttons have hardware-accelerated transforms & transitions" `
    ($html.Contains("button:active, .btn-primary:active, .btn-action:active, .profile-btn:active, .btn-tab:active") -and `
     $html.Contains("transform: scale(0.96) translateY(1px);")) `
    "Button :active tactile compression styling missing"

Assert-Check "Button processing lockout class (.btn-processing) defined" `
    ($html.Contains(".btn-processing {") -and `
     $html.Contains("pointer-events: none !important;") -and `
     $html.Contains("opacity: 0.6 !important;")) `
    "Button processing lockout style missing"

Assert-Check "Double-click protection implemented in saveActiveProfile" `
    ($html.Contains("if (window._saveProcessing || _saveProcessing) return;") -and `
     $html.Contains("window._saveProcessing = true;")) `
    "saveActiveProfile lacks re-entrancy / double-click guard"

# ---------------------------------------------------------
# 2. Toggle Switch Spring Physics & Micro-Interactions
# ---------------------------------------------------------
Assert-Check "Toggle switch contains elastic spring physics transition" `
    ($html.Contains("transition: transform 0.28s cubic-bezier(0.34, 1.56, 0.64, 1)") -or `
     $html.Contains("transition: transform 0.28s cubic-bezier(0.34, 1.56, 0.64, 1), width 0.2s ease")) `
    "Toggle switch thumb missing spring cubic-bezier curve"

Assert-Check "Toggle thumb responds to tactile press with width expansion" `
    ($html.Contains(".slider-toggle:active:before") -and `
     $html.Contains("width: 22px;")) `
    "Toggle switch active press stretch feedback missing"

Assert-Check "Toggles trigger contextual notifications on activation/deactivation" `
    ($html.Contains("showToast(state ? 'Phone Sensi Enabled' : 'Phone Sensi Disabled'") -and `
     $html.Contains("showToast(state ? 'iPhone Sensi Mode Activated' : 'iPhone Sensi Mode Deactivated'") -and `
     $html.Contains("showToast(state ? 'Recoil Control Stabilized' : 'Recoil Control Turned Off'") -and `
     $html.Contains("showToast(state ? 'Crosshair Aim Assistance Active' : 'Crosshair Aim Disabled'")) `
    "Toggle switches do not fire dynamic status notifications"

# ---------------------------------------------------------
# 3. Slider Performance Optimization (Zero Frame Drops)
# ---------------------------------------------------------
Assert-Check "updateSensi decoupled from synchronous storage persistence" `
    ($html.Contains("function updateSensi(scopeName, val) {") -and `
     $html.Contains("autoSaveSensitivityChanges();") -and `
     (-not ($html.Contains("function updateSensi(scopeName, val) {`n    persistActiveDeviceConfig();")))) `
    "updateSensi still contains blocking synchronous storage calls on raw input frames"

Assert-Check "autoSaveSensitivityChanges uses debounce timer" `
    ($html.Contains("function autoSaveSensitivityChanges() {") -and `
     $html.Contains("clearTimeout(window._autoSaveDebounceTimer);") -and `
     $html.Contains("window._autoSaveDebounceTimer = setTimeout(")) `
    "autoSaveSensitivityChanges missing debouncing mechanism"

Assert-Check "flushPendingSaves flushes debounced state immediately on release" `
    ($html.Contains("function flushPendingSaves() {") -and `
     $html.Contains("persistActiveDeviceConfig();")) `
    "flushPendingSaves missing or doesn't persist active config"

Assert-Check "Slider input elements wire flushPendingSaves on onchange and ontouchend" `
    ($html.Contains('onchange="flushPendingSaves()"') -and `
     $html.Contains('ontouchend="flushPendingSaves()"')) `
    "Range sliders do not flush pending changes on touch release"

# ---------------------------------------------------------
# 4. Centralized Status Feedback & Toast Engine
# ---------------------------------------------------------
Assert-Check "Centralized Notification System (SensiNotify) implemented" `
    ($html.Contains("const SensiNotify = {") -and `
     $html.Contains("show: function(message") -and `
     $html.Contains("success: function(message") -and `
     $html.Contains("error: function(message") -and `
     $html.Contains("info: function(message") -and `
     $html.Contains("warning: function(message")) `
    "SensiNotify modular notification engine missing"

Assert-Check "Toast notifications support semantic status styling" `
    ($html.Contains(".toast-banner.toast-success") -and `
     $html.Contains(".toast-banner.toast-error") -and `
     $html.Contains(".toast-banner.toast-info") -and `
     $html.Contains(".toast-banner.toast-warning")) `
    "Semantic toast color classes missing in CSS"

Assert-Check "Toast notifications have animated icon pop keyframes" `
    ($html.Contains("@keyframes toastIconPop {") -and `
     $html.Contains("transform: scale(0.6)") -and `
     $html.Contains("transform: scale(1.18)") -and `
     $html.Contains("transform: scale(1)")) `
    "toastIconPop animation keyframes missing"

Assert-Check "Toast banner container handles safe-area insets" `
    ($html.Contains("top: max(16px, env(safe-area-inset-top, 16px));")) `
    "Notification container does not respect safe-area-inset-top"

# ---------------------------------------------------------
# 5. Screen View & Bottom Navigation Optimization
# ---------------------------------------------------------
Assert-Check "Screen transition animation uses hardware-accelerated transforms" `
    ($html.Contains("@keyframes screenViewEntrance {") -and `
     $html.Contains("transform: translateY(8px);") -and `
     $html.Contains("transform: translateY(0);")) `
    "screenViewEntrance animation missing or not GPU-optimized"

Assert-Check "Bottom navigation bar promoted to dedicated composite layer" `
    ($html.Contains(".bottom-tab-bar {") -and `
     $html.Contains("will-change: transform;") -and `
     $html.Contains("transform: translateZ(0);")) `
    "Bottom tab bar missing GPU compositing hints"

# ---------------------------------------------------------
# 6. Accessibility & prefers-reduced-motion
# ---------------------------------------------------------
Assert-Check "prefers-reduced-motion query implemented" `
    ($html.Contains("@media (prefers-reduced-motion: reduce) {") -and `
     $html.Contains("animation-duration: 0.01ms !important;") -and `
     $html.Contains("transition-duration: 0.01ms !important;")) `
    "prefers-reduced-motion accessible media query missing"

# ---------------------------------------------------------
# Summary
# ---------------------------------------------------------
Write-Host "==========================================================" -ForegroundColor Cyan
Write-Host " TEST RESULTS: $testsPassed Passed, $testsFailed Failed" -ForegroundColor $(if ($testsFailed -eq 0) { "Green" } else { "Red" })
Write-Host "==========================================================" -ForegroundColor Cyan

if ($testsFailed -gt 0) {
    exit 1
} else {
    exit 0
}
