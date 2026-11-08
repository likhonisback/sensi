# ==============================================================================
# SENSI PRO - Persistent Activation, Auto-Resume & Session Restoration Test Suite
# ==============================================================================
$ErrorActionPreference = "Stop"
$projectRoot = $PSScriptRoot

Write-Host "==========================================================" -ForegroundColor Cyan
Write-Host " SENSI PRO - SESSION RESTORATION & AUTO-RESUME TEST SUITE" -ForegroundColor Cyan
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

# 1. SensiSession Architecture
Assert-Check "SensiSession object defined with session storage key" `
    ($html.Contains("const SensiSession = {") -and $html.Contains("SESSION_STORAGE_KEY: 'sensi_active_session'")) `
    "SensiSession object or storage key missing"

Assert-Check "SensiSession implements complete lifecycle methods" `
    ($html.Contains("startSession:") -and `
     $html.Contains("restoreSessionOnStartup:") -and `
     $html.Contains("endSession:") -and `
     $html.Contains("isSessionActive:") -and `
     $html.Contains("onAppForegrounded:") -and `
     $html.Contains("onAppBackgrounded:") -and `
     $html.Contains("updateUI:") -and `
     $html.Contains("startTicker:")) `
    "One or more essential SensiSession lifecycle methods are missing"

# 2. UI Session Controls & Indicators
Assert-Check "Session status card and pulse dot exist on Screen 2" `
    ($html.Contains('id="sensiSessionCard"') -and `
     $html.Contains('id="sensiSessionPulseDot"') -and `
     $html.Contains('id="sensiSessionStateBadge"')) `
    "sensiSessionCard, pulse dot, or state badge missing from DOM"

Assert-Check "Session action buttons (Activate, End Session) exist" `
    ($html.Contains('id="btnToggleSession"') -and `
     $html.Contains('id="btnEndSession"') -and `
     $html.Contains('onclick="toggleSensiSession()"') -and `
     $html.Contains('onclick="endSensiSession()"')) `
    "btnToggleSession or btnEndSession missing from DOM"

Assert-Check "Session info and live elapsed ticker elements exist" `
    ($html.Contains('id="sensiSessionInfoText"') -and $html.Contains('id="sensiSessionTimeText"')) `
    "sensiSessionInfoText or sensiSessionTimeText missing from DOM"

Assert-Check "Header active session indicator badge exists" `
    ($html.Contains('id="headerSessionIndicator"')) `
    "headerSessionIndicator missing from top-bar-header"

Assert-Check "Modal Ready to Play displays session activation badge" `
    ($html.Contains('id="readySessionStatusBadge"')) `
    "readySessionStatusBadge missing from modalReadyToPlay"

# 3. Mobile Lifecycle Listeners (Android Chrome & iPhone Safari)
Assert-Check "Page visibility and pageshow listeners wired for backgrounding/resume" `
    ($html.Contains("document.addEventListener('visibilitychange'") -and `
     $html.Contains("window.addEventListener('pageshow'")) `
    "Lifecycle listeners missing for visibilitychange or pageshow"

# 4. Auto-Resume on Startup
Assert-Check "loadSavedState invokes SensiSession.init for automatic restoration" `
    ($html -match "loadSavedState\(\)[\s\S]*?window\.SensiSession\.init\(\);") `
    "loadSavedState does not trigger SensiSession.init()"

Assert-Check "Restoration displays concise 'Previous Session Restored' notification" `
    ($html.Contains('showToast("✓ Previous Session Restored", "⚡")')) `
    "Concise notification 'Previous Session Restored' missing from restore logic"

Assert-Check "Restoration sets UI badge to ACTIVE • SESSION RESTORED" `
    ($html.Contains('ACTIVE</span> • <span style="color:#38bdf8;">SESSION RESTORED</span>')) `
    "Restoration status badge missing from updateUI"

# 5. Pre-Game Workflow Launch Integration
Assert-Check "Game shortcut launch automatically activates SensiSession" `
    ($html -match "function launchGameShortcut\(gameKey\)[\s\S]*?window\.SensiSession\.startSession\(") `
    "launchGameShortcut does not automatically start SensiSession"

# 6. Honest OS Limitations Disclosure
Assert-Check "OS sandboxing limitations and persistent storage transparently disclosed" `
    ($html.Contains("Mobile OS sandboxing prevents any external application from modifying third-party game settings directly")) `
    "Legitimate integration notice missing from UI"

# 7. Accurate State Separation
Assert-Check "Distinct storage keys used for profile library, active profile, and session state" `
    ($html.Contains("'sensi_profile_library'") -and `
     $html.Contains("'sensi_active_profile_id'") -and `
     $html.Contains("'sensi_active_session'")) `
    "State separation between profiles and session state is incomplete"

Write-Host "==========================================================" -ForegroundColor Cyan
Write-Host " Results: $testsPassed Passed, $testsFailed Failed" -ForegroundColor $(if ($testsFailed -eq 0) { "Green" } else { "Red" })
Write-Host "==========================================================" -ForegroundColor Cyan

if ($testsFailed -gt 0) { exit 1 } else { exit 0 }
