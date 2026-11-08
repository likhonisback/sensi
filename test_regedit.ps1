# ==============================================================================
# SENSI PRO: REGEDIT ENGINE & CUSTOM HUD VERIFICATION TEST SUITE
# Phase 2 & Phase 3 Audit & Functional Verification
# ==============================================================================

$projectRoot = "d:\iphone sensi"
$htmlPath = "$projectRoot\index.html"

if (-not (Test-Path $htmlPath)) {
    Write-Error "CRITICAL: index.html not found at $htmlPath"
    exit 1
}

$html = Get-Content $htmlPath -Raw

$passed = 0
$failed = 0

function Assert-Check($name, $condition) {
    if ($condition) {
        Write-Host "  [PASS] $name" -ForegroundColor Green
        $global:passed++
    } else {
        Write-Host "  [FAIL] $name" -ForegroundColor Red
        $global:failed++
    }
}

Write-Host "`n==========================================================" -ForegroundColor Cyan
Write-Host " SENSI PRO: REGEDIT ENGINE & CUSTOM HUD TEST SUITE" -ForegroundColor Cyan
Write-Host " Target: $htmlPath" -ForegroundColor Cyan
Write-Host "==========================================================" -ForegroundColor Cyan

# ------------------------------------------------------------------------------
# SECTION 1: REGEDIT VISIBILITY & NAVIGATION INTEGRATION
# ------------------------------------------------------------------------------
Write-Host "`n--> Section 1: REGEDIT Navigation & Visibility" -ForegroundColor Yellow

Assert-Check "Top navigation bar includes REGEDIT button (btnTab9)" ($html -match 'id="btnTab9"[^>]*onclick="switchView\(9\)"' -and $html -match 'REGEDIT')
Assert-Check "Bottom navigation bar includes REGEDIT tab (bottomTabRegedit)" ($html -match 'id="bottomTabRegedit"[^>]*onclick="switchView\(9\)"')
Assert-Check "Drawer navigation includes REGEDIT item" ($html -match 'onclick="openDrawerSection\(9\)"' -and $html -match 'REGEDIT')
Assert-Check "switchView highlights bottomTabRegedit when viewNum is 9" ($html -match 'viewNum === 9\)\s*document\.getElementById\(''bottomTabRegedit''\)\?\.classList\.add\(''active''\)')
Assert-Check "switchView triggers initRegeditUI when viewNum is 9" ($html -match 'viewNum === 9' -and $html -match 'initRegeditUI\(\)')

# ------------------------------------------------------------------------------
# SECTION 2: REGEDIT ENGINE DOM & INTERACTION CONTROLS
# ------------------------------------------------------------------------------
Write-Host "`n--> Section 2: REGEDIT Engine DOM & Interactive Controls" -ForegroundColor Yellow

Assert-Check "Screen 9 container exists (id=screenView9)" ($html -match 'id="screenView9"')
Assert-Check "REGEDIT header and live status badge exist" ($html -match 'id="regeditStatusBadge"' -and $html -match 'REGEDIT ENGINE & CALIBRATION')
Assert-Check "Master REGEDIT active toggle exists (regeditMasterToggle)" ($html -match 'id="regeditMasterToggle"[^>]*onchange="toggleRegedit\(this\.checked\)"')
Assert-Check "Universal platform badge exists (regeditPlatformBadge)" ($html -match 'id="regeditPlatformBadge"')
Assert-Check "REGEDIT Response Curve presets exist (Aggressive, Balanced, Linear)" ($html -match 'id="regeditCurveAggressive"' -and $html -match 'id="regeditCurveBalanced"' -and $html -match 'id="regeditCurveLinear"')

# ------------------------------------------------------------------------------
# SECTION 3: REGEDIT PROFILE MANAGEMENT SUITE
# ------------------------------------------------------------------------------
Write-Host "`n--> Section 3: REGEDIT Profile Suite & CRUD Operations" -ForegroundColor Yellow

Assert-Check "REGEDIT Profile selector dropdown exists" ($html -match 'id="regeditProfileSelect"[^>]*onchange="onRegeditProfileChange\(this\.value\)"')
Assert-Check "Active profile tag display element exists" ($html -match 'id="regeditActiveProfileTag"')
Assert-Check "Create Profile button wired to openNewProfileModal" ($html -match 'onclick="openNewProfileModal\(\)"')
Assert-Check "Save Profile button wired to saveActiveProfile" ($html -match 'onclick="saveActiveProfile\(\)"')
Assert-Check "Rename Profile button wired to openRenameProfileModal" ($html -match 'onclick="openRenameProfileModal\(\)"')
Assert-Check "Duplicate Profile button wired to duplicateActiveProfile" ($html -match 'onclick="duplicateActiveProfile\(\)"')
Assert-Check "Delete Profile button wired to deleteActiveProfile" ($html -match 'onclick="deleteActiveProfile\(\)"')
Assert-Check "Restore Defaults button wired to restoreBenchmarkDefaults" ($html -match 'onclick="restoreBenchmarkDefaults\(\)"')

# ------------------------------------------------------------------------------
# SECTION 4: DUAL SLIDER & DIRECT NUMERIC REGEDIT CONTROLS
# ------------------------------------------------------------------------------
Write-Host "`n--> Section 4: Dual Slider & Direct Numeric Inputs" -ForegroundColor Yellow

$axes = @("General", "RedDot", "Scope2x", "Scope4x", "Sniper", "FreeLook")
foreach ($axis in $axes) {
    Assert-Check "REGEDIT General dual controls for $axis exist" (
        $html -match "id=""regeditSlider$axis""" -and
        $html -match "id=""regeditNum$axis""" -and
        $html -match "updateRegeditSensi\('$axis'"
    )
}

# ------------------------------------------------------------------------------
# SECTION 5: WEAPON RECOIL & TELEMETRY PRESERVATION
# ------------------------------------------------------------------------------
Write-Host "`n--> Section 5: Weapon Recoil & Telemetry Preservation" -ForegroundColor Yellow

Assert-Check "Recoil Master Toggle preserved (recoilControlActiveToggle)" ($html -match 'id="recoilControlActiveToggle"')
Assert-Check "Weapon category profiles preserved (Balanced, SMG, AR, Sniper)" ($html -match 'id="recoilProfBalanced"' -and $html -match 'id="recoilProfSmg"' -and $html -match 'id="recoilProfAr"' -and $html -match 'id="recoilProfSniper"')
Assert-Check "Counter-drag damping slider exists (sliderRecoilDamping)" ($html -match 'id="sliderRecoilDamping"')
Assert-Check "Horizontal jitter filter slider exists (sliderRecoilJitter)" ($html -match 'id="sliderRecoilJitter"')
Assert-Check "Head-level overshoot limit slider exists (sliderRecoilOvershoot)" ($html -match 'id="sliderRecoilOvershoot"')
Assert-Check "Digitizer refresh rate telemetry display preserved" ($html -match 'id="recoilDisplayRateText"')
Assert-Check "Estimated counter-drag metric display preserved" ($html -match 'id="recoilEstimatedDragDistance"')

# ------------------------------------------------------------------------------
# SECTION 6: CUSTOM HUD MULTI-PROFILE SLOTS (SCREEN 4)
# ------------------------------------------------------------------------------
Write-Host "`n--> Section 6: Custom HUD Profile Slots (Screen 4)" -ForegroundColor Yellow

Assert-Check "HUD Profile Preset Slots card exists in Screen 4" ($html -match 'HUD PROFILE PRESET SLOTS')
Assert-Check "5 HUD Profile Slot buttons exist (Slot 1 - Custom)" ($html -match 'id="btnHudSlot1"' -and $html -match 'id="btnHudSlot2"' -and $html -match 'id="btnHudSlot3"' -and $html -match 'id="btnHudSlot4"' -and $html -match 'id="btnHudSlot5"')
Assert-Check "Save to Current HUD Slot button exists" ($html -match 'onclick="saveToCurrentHudSlot\(\)"')
Assert-Check "Reload HUD Slot button exists" ($html -match 'onclick="loadCurrentHudSlot\(\)"')
Assert-Check "Active HUD Slot indicator exists" ($html -match 'id="lblActiveHudSlot"')

# ------------------------------------------------------------------------------
# SECTION 7: JAVASCRIPT ENGINE & BIDIRECTIONAL SYNCHRONIZATION
# ------------------------------------------------------------------------------
Write-Host "`n--> Section 7: JavaScript Engine & State Synchronization" -ForegroundColor Yellow

Assert-Check "AppState.regedit initialized" ($html -match 'AppState\.regedit\s*=')
Assert-Check "initRegeditUI function implemented" ($html -match 'function initRegeditUI\(\)')
Assert-Check "toggleRegedit function implemented" ($html -match 'function toggleRegedit\(enabled\)')
Assert-Check "selectRegeditCurve function implemented" ($html -match 'function selectRegeditCurve\(curveName')
Assert-Check "updateRegeditSensi function implemented" ($html -match 'function updateRegeditSensi\(axis, rawVal, source\)')
Assert-Check "onRegeditProfileChange function implemented" ($html -match 'function onRegeditProfileChange\(profileId\)')
Assert-Check "saveRegeditConfig function implemented" ($html -match 'function saveRegeditConfig\(\)')
Assert-Check "applySensiValuesToUI synchronizes regeditSlider and regeditNum elements" ($html -match 'regeditSliderGeneral' -and $html -match 'regeditNumGeneral')
Assert-Check "selectHudSlot function implemented" ($html -match 'function selectHudSlot\(slotNum\)')
Assert-Check "saveToCurrentHudSlot function implemented" ($html -match 'function saveToCurrentHudSlot\(\)')
Assert-Check "loadCurrentHudSlot function implemented" ($html -match 'function loadCurrentHudSlot\(\)')

# ------------------------------------------------------------------------------
# SECTION 8: PERSISTENCE & RESTORATION ACROSS APP RESTARTS
# ------------------------------------------------------------------------------
Write-Host "`n--> Section 8: Persistence & Restoration Across App Restarts" -ForegroundColor Yellow

Assert-Check "sensi_regedit_config storage key used in saveRegeditConfig" ($html -match 'PermanentStorage\.setItem\(''sensi_regedit_config''')
Assert-Check "loadSavedState restores sensi_regedit_config" ($html -match 'PermanentStorage\.getItem\(''sensi_regedit_config''')
Assert-Check "sensi_aimhud_slots storage key used in saveToCurrentHudSlot" ($html -match 'PermanentStorage\.setItem\(''sensi_aimhud_slots''')
Assert-Check "loadSavedState restores sensi_aimhud_slots" ($html -match 'PermanentStorage\.getItem\(''sensi_aimhud_slots''')

# ------------------------------------------------------------------------------
# SECTION 9: FAIR-PLAY & ANTI-CHEAT COMPLIANCE
# ------------------------------------------------------------------------------
Write-Host "`n--> Section 9: Fair-Play & Anti-Cheat Compliance Disclosures" -ForegroundColor Yellow

Assert-Check "Disclaims memory injection and illegal hacks" ($html -match 'without memory injection' -or $html -match 'zero game memory injection')
Assert-Check "Explains legitimate digitizer calibration and touch-smoothing" ($html -match 'legitimate digitizer touch-curve calibration' -or $html -match 'touch-coordinate smoothing')
Assert-Check "Transparent Android & iOS platform support affirmed" ($html -match 'ANDROID \+ iOS' -or $html -match 'Android and iOS')

# ------------------------------------------------------------------------------
# SUMMARY
# ------------------------------------------------------------------------------
$summaryColor = if ($failed -eq 0) { "Green" } else { "Red" }
Write-Host "`n==========================================================" -ForegroundColor Cyan
Write-Host " REGEDIT & CUSTOM HUD RESULTS: $passed Passed, $failed Failed" -ForegroundColor $summaryColor
Write-Host "==========================================================" -ForegroundColor Cyan

if ($failed -gt 0) {
    exit 1
}
