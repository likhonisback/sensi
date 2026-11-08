# ==============================================================================
# SENSI PRO - End-to-End Profile Persistence & Pre-Game Workflow Test
# ==============================================================================
$ErrorActionPreference = "Stop"
$projectRoot = $PSScriptRoot

Write-Host "==========================================================" -ForegroundColor Cyan
Write-Host " SENSI PRO - PROFILE PERSISTENCE & PRE-GAME WORKFLOW TEST" -ForegroundColor Cyan
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

# 1. Auto-save triggers
Assert-Check "updateSensi triggers autoSaveSensitivityChanges" `
    ($html -match "function updateSensi\(scopeName, val\)[\s\S]*?autoSaveSensitivityChanges\(\);") `
    "updateSensi does not call autoSaveSensitivityChanges()"

Assert-Check "updateButtonSize triggers autoSaveSensitivityChanges" `
    ($html -match "function updateButtonSize\(val\)[\s\S]*?autoSaveSensitivityChanges\(\);") `
    "updateButtonSize does not call autoSaveSensitivityChanges()"

Assert-Check "selectDpi triggers autoSaveSensitivityChanges" `
    ($html -match "function selectDpi\(val, btnElem\)[\s\S]*?autoSaveSensitivityChanges\(\);") `
    "selectDpi does not call autoSaveSensitivityChanges()"

# 2. Persistence Layer
Assert-Check "PermanentStorage verifies write with read-back check" `
    ($html.Contains("const verified = localStorage.getItem(key);")) `
    "Verified write-readback missing from PermanentStorage.setItem"

Assert-Check "PermanentStorage mirrors asynchronously to IndexedDB (SensiIDB)" `
    ($html.Contains("window.SensiIDB.set(key, valStr)")) `
    "SensiIDB IndexedDB mirror call missing from PermanentStorage.setItem"

# 3. Startup Restoration
Assert-Check "loadSavedState restores active profile and applies to UI" `
    ($html -match "loadSavedState\(\)[\s\S]*?applySensiValuesToUI\(\);[\s\S]*?renderRedTxt\(\);") `
    "loadSavedState does not apply restored profile values to UI"

# 4. Pre-Game Workflow
Assert-Check "Ready to Play workflow button exists on Screen 2" `
    ($html.Contains('id="btnReadyToPlay"')) `
    "btnReadyToPlay element missing from DOM"

Assert-Check "Ready to Play modal and summary grid exist" `
    ($html.Contains('id="modalReadyToPlay"') -and $html.Contains('id="readySensiGrid"')) `
    "modalReadyToPlay or readySensiGrid missing from DOM"

Assert-Check "Ready to Play copies summary to clipboard" `
    ($html -match "function copyReadyConfiguration\(\)[\s\S]*?SENSI PRO • CONFIGURATION SUMMARY") `
    "copyReadyConfiguration does not format expected summary"

Assert-Check "Legitimate game launcher launches protocol schemes" `
    ($html -match "function launchGameShortcut\(gameKey\)[\s\S]*?freefire://[\s\S]*?freefiremax://") `
    "launchGameShortcut does not support standard Free Fire schemes"

# 5. Profile CRUD Actions
Assert-Check "Profile CRUD lifecycle functions exist in script" `
    ($html.Contains("function saveActiveProfile()") -and `
     $html.Contains("function openNewProfileModal()") -and `
     $html.Contains("function duplicateActiveProfile()") -and `
     $html.Contains("function openRenameProfileModal()") -and `
     $html.Contains("function confirmResetToDefault()") -and `
     $html.Contains("function deleteActiveProfile()")) `
    "One or more required profile functions missing from script"

# 6. Storage Error Handling
Assert-Check "saveActiveProfile handles storage failures gracefully" `
    ($html -match "Could not write settings to local storage") `
    "Truthful storage failure notification missing"

Write-Host "==========================================================" -ForegroundColor Cyan
Write-Host " Results: $testsPassed Passed, $testsFailed Failed" -ForegroundColor $(if ($testsFailed -eq 0) { "Green" } else { "Red" })
Write-Host "==========================================================" -ForegroundColor Cyan

if ($testsFailed -gt 0) { exit 1 } else { exit 0 }
