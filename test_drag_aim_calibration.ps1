# ==============================================================================
# SENSI PRO — HEADSHOT-FIRST DRAG AIM OPTIMIZATION & TRAINING GROUND TEST SUITE
# ==============================================================================

$targetFile = "index.html"
if (-not (Test-Path $targetFile)) {
    Write-Host "[FAIL] index.html not found!" -ForegroundColor Red
    exit 1
}

$content = Get-Content $targetFile -Raw
$passCount = 0
$failCount = 0

function Assert-Test($condition, $description) {
    if ($condition) {
        Write-Host "  [PASS] $description" -ForegroundColor Green
        $global:passCount++
    } else {
        Write-Host "  [FAIL] $description" -ForegroundColor Red
        $global:failCount++
    }
}

Write-Host "==========================================================" -ForegroundColor Cyan
Write-Host " SENSI PRO: HEADSHOT-FIRST DRAG AIM OPTIMIZATION SUITE" -ForegroundColor Cyan
Write-Host " Target: $targetFile" -ForegroundColor Cyan
Write-Host "==========================================================" -ForegroundColor Cyan

# ------------------------------------------------------------------------------
# SECTION 1: Screen 5 Drag Aim Lab Architecture & Markup
# ------------------------------------------------------------------------------
Write-Host "`n--> Section 1: Screen 5 Drag Aim Lab Architecture & Markup" -ForegroundColor Yellow

Assert-Test ($content -match 'id="screenView5"') "Screen 5 container view exists"
Assert-Test ($content -match 'DRAG AIM LAB') "Screen 5 header displays DRAG AIM LAB"
Assert-Test ($content -match '1\. DEVICE & DRAG BASELINE') "Section 1 defines Device & Drag Baseline"
Assert-Test ($content -match '2\. TRAINING GROUND TEST SETUP') "Section 2 defines Training Ground Test Setup"
Assert-Test ($content -match '3\. EMPIRICAL DRAG OUTCOME') "Section 3 defines Empirical Drag Outcome Diagnostics"
Assert-Test ($content -match '4\. CONTROLLED FINE-TUNING') "Section 4 defines Controlled Fine-Tuning Steppers"
Assert-Test ($content -match '5\. PROFILE COMPARISON') "Section 5 defines Profile Comparison & A/B Evaluator"
Assert-Test ($content -match '6\. SAVED PRACTICE RESULTS LOG') "Section 6 defines Saved Practice Results Log"

# ------------------------------------------------------------------------------
# SECTION 2: Navigation & Tab Bar Integration
# ------------------------------------------------------------------------------
Write-Host "`n--> Section 2: Navigation & Tab Bar Integration" -ForegroundColor Yellow

Assert-Test ($content -match 'id="btnTab5"[^>]*>[\s\S]*?Drag Aim') "Top navigation tab 5 labeled Drag Aim"
Assert-Test ($content -match 'id="bottomTabAI"[^>]*>[\s\S]*?DRAG AIM') "Bottom tab bar button 5 labeled DRAG AIM"
Assert-Test ($content -match 'switchView\(5\)') "Top & bottom navigation triggers switchView(5)"
Assert-Test ($content -match 'menu_ai_pro">Drag Aim Training') "Drawer menu item 5 labeled Drag Aim Training"
Assert-Test ($content -match 'viewNum === 5 && typeof initDragAimCalibration === .function.') "switchView(5) triggers initDragAimCalibration()"

# ------------------------------------------------------------------------------
# SECTION 3: Starting Drag Profiles & Baselines
# ------------------------------------------------------------------------------
Write-Host "`n--> Section 3: Starting Drag Profiles & Baselines" -ForegroundColor Yellow

Assert-Test ($content -match 'selectDragBaseline\(''Fast Drag''') "Fast Drag baseline selectable"
Assert-Test ($content -match 'selectDragBaseline\(''Balanced Headshot''') "Balanced Headshot baseline selectable"
Assert-Test ($content -match 'selectDragBaseline\(''Precision Drag''') "Precision Drag baseline selectable"
Assert-Test ($content -match 'selectDragBaseline\(''Smooth Tracking''') "Smooth Tracking baseline selectable"
Assert-Test ($content -match 'resetDragCalibrationToBaseline\(\)') "Reset to starting baseline function present"

# ------------------------------------------------------------------------------
# SECTION 4: Training Ground Weapon & Distance Test Matrix
# ------------------------------------------------------------------------------
Write-Host "`n--> Section 4: Training Ground Test Matrix" -ForegroundColor Yellow

Assert-Test ($content -match 'selectTrainingWeapon\(''SMG'',\s*''MP40 / UMP''') "Training weapon SMG (MP40 / UMP) selectable"
Assert-Test ($content -match 'selectTrainingWeapon\(''Shotgun'',\s*''M1014 / M1887''') "Training weapon Shotgun (M1014 / M1887) selectable"
Assert-Test ($content -match 'selectTrainingWeapon\(''AR'',\s*''Woodpecker / AK''') "Training weapon AR (Woodpecker / AK) selectable"
Assert-Test ($content -match 'selectTrainingWeapon\(''Sniper'',\s*''AWM / M82B''') "Training weapon Sniper (AWM / M82B) selectable"

Assert-Test ($content -match 'selectTrainingDistance\(''Close \(5-10m\)''') "Target distance Close (5-10m) selectable"
Assert-Test ($content -match 'selectTrainingDistance\(''Mid \(15-25m\)''') "Target distance Mid (15-25m) selectable"
Assert-Test ($content -match 'selectTrainingDistance\(''Long \(30-50m\)''') "Target distance Long (30-50m) selectable"

# ------------------------------------------------------------------------------
# SECTION 5: Empirical Drag Diagnostic Outcomes
# ------------------------------------------------------------------------------
Write-Host "`n--> Section 5: Empirical Drag Diagnostic Outcomes" -ForegroundColor Yellow

Assert-Test ($content -match 'applyDragAimDiagnostic\(''overshoot''\)') "Diagnostic card for overshoot present"
Assert-Test ($content -match 'applyDragAimDiagnostic\(''undershoot''\)') "Diagnostic card for chest-stuck undershoot present"
Assert-Test ($content -match 'applyDragAimDiagnostic\(''jitter''\)') "Diagnostic card for jittery aim present"
Assert-Test ($content -match 'applyDragAimDiagnostic\(''sluggish''\)') "Diagnostic card for sluggish drag present"
Assert-Test ($content -match 'applyDragAimDiagnostic\(''perfect''\)') "Diagnostic card for optimal headshot present"

# ------------------------------------------------------------------------------
# SECTION 6: Controlled Micro-Stepper Controls (All 6 Scopes + Button Size)
# ------------------------------------------------------------------------------
Write-Host "`n--> Section 6: Controlled Micro-Stepper Controls" -ForegroundColor Yellow

Assert-Test ($content -match 'nudgeDragAimParam\(''General'',\s*-5\)') "General sensitivity -5 stepper present"
Assert-Test ($content -match 'nudgeDragAimParam\(''General'',\s*5\)') "General sensitivity +5 stepper present"
Assert-Test ($content -match 'nudgeDragAimParam\(''RedDot'',\s*-5\)') "Red Dot sensitivity -5 stepper present"
Assert-Test ($content -match 'nudgeDragAimParam\(''RedDot'',\s*5\)') "Red Dot sensitivity +5 stepper present"
Assert-Test ($content -match 'nudgeDragAimParam\(''Scope2x'',\s*-5\)') "2x Scope sensitivity -5 stepper present"
Assert-Test ($content -match 'nudgeDragAimParam\(''Scope4x'',\s*-5\)') "4x Scope sensitivity -5 stepper present"
Assert-Test ($content -match 'nudgeDragAimParam\(''Sniper'',\s*-5\)') "Sniper Scope sensitivity -5 stepper present"
Assert-Test ($content -match 'nudgeDragAimParam\(''FreeLook'',\s*-5\)') "Free Look sensitivity -5 stepper present"
Assert-Test ($content -match 'nudgeDragAimParam\(''ButtonSize'',\s*-2\)') "Fire Button Size -2% stepper present"
Assert-Test ($content -match 'nudgeDragAimParam\(''ButtonSize'',\s*2\)') "Fire Button Size +2% stepper present"

# ------------------------------------------------------------------------------
# SECTION 7: Before vs. After & A/B Profile Comparison Evaluator
# ------------------------------------------------------------------------------
Write-Host "`n--> Section 7: Before vs. After & A/B Profile Comparison Evaluator" -ForegroundColor Yellow

Assert-Test ($content -match 'id="dragAimComparisonTbody"') "Comparison table body exists in DOM"
Assert-Test ($content -match 'id="compareProfileASelect"') "Profile A comparison dropdown exists"
Assert-Test ($content -match 'id="compareProfileBSelect"') "Profile B comparison dropdown exists"
Assert-Test ($content -match 'function populateProfileComparisonDropdowns\(\)') "populateProfileComparisonDropdowns implemented"
Assert-Test ($content -match 'function onCompareProfilesChange\(\)') "onCompareProfilesChange implemented"
Assert-Test ($content -match 'function renderDragAimComparison\(\)') "renderDragAimComparison function implemented"
Assert-Test ($content -match 'diff-delta pos') "Positive delta styling handled"
Assert-Test ($content -match 'diff-delta neg') "Negative delta styling handled"
Assert-Test ($content -match 'diff-delta zero') "Zero delta baseline styling handled"

# ------------------------------------------------------------------------------
# SECTION 8: Saved Practice Results Log & History Persistence
# ------------------------------------------------------------------------------
Write-Host "`n--> Section 8: Saved Practice Results Log & History Persistence" -ForegroundColor Yellow

Assert-Test ($content -match 'id="dragAimPracticeLogContainer"') "Practice log container exists in DOM"
Assert-Test ($content -match 'function recordPracticeResult\(') "recordPracticeResult implemented"
Assert-Test ($content -match 'function renderDragAimPracticeLog\(\)') "renderDragAimPracticeLog implemented"
Assert-Test ($content -match 'function loadPracticeTrialSnapshot\(') "loadPracticeTrialSnapshot implemented"
Assert-Test ($content -match 'function clearDragAimPracticeLog\(\)') "clearDragAimPracticeLog implemented"
Assert-Test ($content -match 'sensi_practice_log') "sensi_practice_log storage key used for persistence"
Assert-Test ($content -match 'loadSavedState[\s\S]*?initDragAimCalibration') "loadSavedState restores Drag Aim state & practice log"

# ------------------------------------------------------------------------------
# SECTION 9: Master Save, Apply, & Anti-Cheat Compliance
# ------------------------------------------------------------------------------
Write-Host "`n--> Section 9: Master Save, Apply, & Anti-Cheat Compliance" -ForegroundColor Yellow

Assert-Test ($content -match 'applyAndSaveDragAimCalibration\(\)') "applyAndSaveDragAimCalibration implemented"
Assert-Test ($content -match 'revertDragAimCalibration\(\)') "revertDragAimCalibration implemented"
Assert-Test ($content -match 'does NOT inject touch inputs|strictly does not inject touch events') "Anti-cheat compliance disclaimer present"
Assert-Test ($content -match 'saveActiveProfile\(\)') "Calibration changes commit atomically via saveActiveProfile"

# ------------------------------------------------------------------------------
# SUMMARY
# ------------------------------------------------------------------------------
Write-Host "`n==========================================================" -ForegroundColor Cyan
Write-Host " DRAG AIM SUITE RESULTS: $passCount Passed, $failCount Failed" -ForegroundColor $(if ($failCount -eq 0) { "Green" } else { "Red" })
Write-Host "==========================================================" -ForegroundColor Cyan

if ($failCount -gt 0) { exit 1 } else { exit 0 }
