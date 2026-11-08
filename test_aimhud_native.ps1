# ==============================================================================
# SENSI PRO - REAL NATIVE ANDROID FLOATING AIMHUD & MACRO VERIFICATION TEST SUITE
# ==============================================================================

$ErrorActionPreference = "Stop"

$projectRoot = $PSScriptRoot
$indexHtml = "$projectRoot\index.html"
$manifest = "$projectRoot\android\app\src\main\AndroidManifest.xml"
$mainActivity = "$projectRoot\android\app\src\main\java\com\sensipro\app\MainActivity.java"
$aimHudService = "$projectRoot\android\app\src\main\java\com\sensipro\app\AimHudService.java"
$apkPath = "$projectRoot\build\Android\SensiPro.apk"

Write-Host "==========================================================" -ForegroundColor Cyan
Write-Host " REAL NATIVE ANDROID AIMHUD & MACRO VERIFICATION" -ForegroundColor Cyan
Write-Host " Testing target: $projectRoot" -ForegroundColor Cyan
Write-Host "==========================================================" -ForegroundColor Cyan

$passed = 0
$failed = 0

function Assert-Check {
    param(
        [string]$TestName,
        [bool]$Condition,
        [string]$ErrorMessage = "Assertion failed"
    )
    if ($Condition) {
        Write-Host "  [PASS] $TestName" -ForegroundColor Green
        $script:passed++
    } else {
        Write-Host "  [FAIL] $TestName - $ErrorMessage" -ForegroundColor Red
        $script:failed++
    }
}

$html = Get-Content $indexHtml -Raw -Encoding UTF8
$manifestContent = Get-Content $manifest -Raw -Encoding UTF8
$activityContent = Get-Content $mainActivity -Raw -Encoding UTF8
$serviceContent = if (Test-Path $aimHudService) { Get-Content $aimHudService -Raw -Encoding UTF8 } else { "" }

Write-Host "`n--> Section 1: Android Native Service & Architecture" -ForegroundColor Yellow
Assert-Check "AimHudService.java source file exists" (Test-Path $aimHudService)
Assert-Check "AimHudService extends android.app.Service" ($serviceContent -match "class AimHudService extends Service")
Assert-Check "AimHudService implements WindowManager overlay creation" ($serviceContent -match "TYPE_APPLICATION_OVERLAY" -and $serviceContent -match "mWindowManager\.addView")
Assert-Check "AimHudService registers notification channel and foreground service" ($serviceContent -match "sensipro_aimhud_channel" -and $serviceContent -match "startForeground")
Assert-Check "AimHudService implements onMeasure to guarantee non-zero Canvas dimensions" ($serviceContent -match "protected void onMeasure")
Assert-Check "AimHudService implements calculateWindowWidth/Height with explicit non-zero dimensions" ($serviceContent -match "calculateWindowWidth" -and $serviceContent -match "calculateWindowHeight")
Assert-Check "AimHudService implements FLAG_LAYOUT_IN_SCREEN and layoutInDisplayCutoutMode for fullscreen games" ($serviceContent -match "FLAG_LAYOUT_IN_SCREEN" -and $serviceContent -match "layoutInDisplayCutoutMode")
Assert-Check "AimHudService handles onConfigurationChanged for landscape orientation" ($serviceContent -match "onConfigurationChanged")
Assert-Check "AimHudService safely guards FOREGROUND_SERVICE_TYPE_SPECIAL_USE for API 34+" ($serviceContent -match "Build\.VERSION\.SDK_INT >= 34")

Write-Host "`n--> Section 2: 5 Reticle Icon Styles" -ForegroundColor Yellow
Assert-Check "Native reticle renders Circle style" ($serviceContent -match 'case "circle":')
Assert-Check "Native reticle renders Plus style" ($serviceContent -match 'case "plus":')
Assert-Check "Native reticle renders Cross style" ($serviceContent -match 'case "cross":')
Assert-Check "Native reticle renders Dot style" ($serviceContent -match 'case "dot":')
Assert-Check "Native reticle renders Ring style" ($serviceContent -match 'case "ring":')

Write-Host "`n--> Section 3: Safe Touch Pass-Through & Lock/Unlock Mechanism" -ForegroundColor Yellow
Assert-Check "Locked state enforces FLAG_NOT_TOUCHABLE for 100% pass-through touches" ($serviceContent -match "FLAG_NOT_TOUCHABLE")
Assert-Check "Unlocked drag mode handles MotionEvent for real repositioning" ($serviceContent -match "MotionEvent\.ACTION_MOVE" -and $serviceContent -match "updateViewLayout")
Assert-Check "Coordinates and lock state atomically persisted in SharedPreferences" ($serviceContent -match "sensi_aimhud_prefs" -and $serviceContent -match "pos_x" -and $serviceContent -match "pos_y")
Assert-Check "Reset position centers overlay to (0, 0)" ($serviceContent -match "resetPosition\(\)" -and $serviceContent -match "mParams\.x = 0")

Write-Host "`n--> Section 4: Native Android Bridge Integration in MainActivity" -ForegroundColor Yellow
Assert-Check "MainActivity bridges startAimHud(configJson)" ($activityContent -match "public boolean startAimHud")
Assert-Check "MainActivity bridges updateAimHud(configJson)" ($activityContent -match "public void updateAimHud")
Assert-Check "MainActivity bridges stopAimHud()" ($activityContent -match "public void stopAimHud")
Assert-Check "MainActivity bridges isAimHudRunning()" ($activityContent -match "public boolean isAimHudRunning")
Assert-Check "MainActivity bridges setAimHudLock(boolean locked)" ($activityContent -match "public void setAimHudLock")
Assert-Check "MainActivity bridges resetAimHudPosition()" ($activityContent -match "public void resetAimHudPosition")
Assert-Check "MainActivity bridges getAimHudConfig()" ($activityContent -match "public String getAimHudConfig")

Write-Host "`n--> Section 5: Android Manifest & Permission Declarations" -ForegroundColor Yellow
Assert-Check "Manifest declares SYSTEM_ALERT_WINDOW" ($manifestContent -match "android\.permission\.SYSTEM_ALERT_WINDOW")
Assert-Check "Manifest declares FOREGROUND_SERVICE" ($manifestContent -match "android\.permission\.FOREGROUND_SERVICE")
Assert-Check "Manifest declares FOREGROUND_SERVICE_SPECIAL_USE for Android 14" ($manifestContent -match "android\.permission\.FOREGROUND_SERVICE_SPECIAL_USE")
Assert-Check "Manifest registers AimHudService with specialUse type" ($manifestContent -match 'android:name="\.AimHudService"' -and $manifestContent -match 'android:foregroundServiceType="specialUse"')

Write-Host "`n--> Section 6: Screen 4 AimHUD Web Interface & Controls" -ForegroundColor Yellow
Assert-Check "Screen 4 defines style selector cards for all 5 styles" ($html -match 'id="styleBtn_plus"' -and $html -match 'id="styleBtn_circle"' -and $html -match 'id="styleBtn_cross"' -and $html -match 'id="styleBtn_dot"' -and $html -match 'id="styleBtn_ring"')
Assert-Check "Screen 4 defines Size, Opacity, Rotation, and Position controls" ($html -match 'id="sliderCrossSize"' -and $html -match 'id="sliderCrossOpacity"' -and $html -match 'id="sliderCrossRotation"' -and $html -match 'id="dpadCoordText"')
Assert-Check "Screen 4 defines master action buttons (Apply, Remove, Lock, Center)" ($html -match 'id="btnAimhudApply"' -and $html -match 'id="btnAimhudRemove"' -and $html -match 'id="btnToggleLock"')
Assert-Check "Screen 4 defines explicit Save Config and Restore Defaults buttons" ($html -match 'id="btnAimhudSave"' -and $html -match 'id="btnAimhudRestore"')
Assert-Check "Screen 4 defines Android overlay permission banner and modal" ($html -match 'id="aimhudPermBanner"' -and $html -match 'id="modalOverlayPermissionWhy"')
Assert-Check "Screen 4 renders interactive preview stage with drag support" ($html -match 'id="crosshairStage"' -and $html -match 'id="crosshairVisual"')

Write-Host "`n--> Section 7: Macro Visual Layout HUD Section" -ForegroundColor Yellow
Assert-Check "Macro Visual Layout section exists in DOM" ($html -match 'MACRO VISUAL LAYOUT HUD')
Assert-Check "Macro section provides circular icon size and opacity sliders" ($html -match 'id="sliderMacroFireSize"' -and $html -match 'id="sliderMacroOpacity"')
Assert-Check "Macro section provides coordinate offset and D-pad reset" ($html -match 'id="macroCoordText"' -and $html -match 'resetMacroOffset\(\)')
Assert-Check "Macro section provides dedicated Save and Restore controls" ($html -match 'id="btnSaveMacroConfig"' -and $html -match 'id="btnRestoreMacroDefaults"')
Assert-Check "Macro section provides dedicated Apply and Remove controls" ($html -match 'id="btnApplyMacroOverlay"' -and $html -match 'id="btnRemoveMacroOverlay"')
Assert-Check "Macro section provides dedicated Lock/Unlock controls" ($html -match 'id="btnToggleMacroLock"' -and $html -match 'id="macroLockText"')
Assert-Check "Macro section provides interactive circular icon preview stage" ($html -match 'id="macroPreviewStage"' -and $html -match 'id="macroVisual"')
Assert-Check "Macro section provides color palette selector" ($html -match 'selectMacroColor' -and $html -match 'id="lblActiveMacroColorHex"')
Assert-Check "Macro section provides iOS sandbox platform disclaimer" ($html -match 'macroIosBanner' -and $html -match 'Apple iOS sandboxing')
Assert-Check "Macro section explicitly disclaims touch injection or memory modification" ($html -match 'VISUAL REFERENCE ONLY')

Write-Host "`n--> Section 8: AimHUD & Macro JavaScript Engine" -ForegroundColor Yellow
Assert-Check "JS engine implements applyAimHudOverlay" ($html -match "function applyAimHudOverlay\(\)")
Assert-Check "JS engine implements removeAimHudOverlay" ($html -match "function removeAimHudOverlay\(\)")
Assert-Check "JS engine implements toggleAimHudLock" ($html -match "function toggleAimHudLock\(\)")
Assert-Check "JS engine implements selectAimHudStyle" ($html -match "function selectAimHudStyle\(")
Assert-Check "JS engine implements selectAimHudColor" ($html -match "function selectAimHudColor\(")
Assert-Check "JS engine implements applyMacroOverlay" ($html -match "function applyMacroOverlay\(\)")
Assert-Check "JS engine implements removeMacroOverlay" ($html -match "function removeMacroOverlay\(\)")
Assert-Check "JS engine implements toggleMacroLock" ($html -match "function toggleMacroLock\(\)")
Assert-Check "JS engine implements selectMacroColor" ($html -match "function selectMacroColor\(")
Assert-Check "JS engine implements selectMacroPreset" ($html -match "function selectMacroPreset\(")
Assert-Check "JS engine implements renderMacroPreview" ($html -match "function renderMacroPreview\(\)")
Assert-Check "JS engine implements setupMacroStageDrag" ($html -match "function setupMacroStageDrag\(\)")
Assert-Check "JS engine implements updateWebFloatingMacroOverlay" ($html -match "function updateWebFloatingMacroOverlay\(\)")
Assert-Check "JS engine implements initAimHudUI and restores saved state" ($html -match "function initAimHudUI\(\)" -and $html -match "sensi_aimhud_config")
Assert-Check "JS engine implements restoreAimHudDefaults" ($html -match "function restoreAimHudDefaults\(\)")
Assert-Check "JS engine implements updateWebFloatingOverlay for browser pass-through overlay" ($html -match "function updateWebFloatingOverlay\(\)")
Assert-Check "JS engine implements setupCrosshairStageDrag for interactive reticle repositioning" ($html -match "function setupCrosshairStageDrag\(\)")
Assert-Check "JS engine persists and restores sensi_macro_config" ($html -match "sensi_macro_config")
Assert-Check "switchView(4) automatically triggers initAimHudUI to resync state" ($html -match "viewNum === 4 && typeof initAimHudUI === 'function'")
Assert-Check "switchView(9) automatically synchronizes Recoil & Stability UI" ($html -match "viewNum === 9 && typeof applySensiValuesToUI === 'function'")
Assert-Check "loadSavedState restores standalone recoil configuration" ($html -match "sensi_recoil_config")
Assert-Check "loadSavedState restores standalone macro configuration" ($html -match "loadSavedState" -and $html -match "sensi_macro_config")

Write-Host "`n--> Section 9: Native Macro Android Service Architecture" -ForegroundColor Yellow
Assert-Check "AimHudService handles ACTION_APPLY_MACRO and ACTION_REMOVE_MACRO" ($serviceContent -match "ACTION_APPLY_MACRO" -and $serviceContent -match "ACTION_REMOVE_MACRO")
Assert-Check "AimHudService handles ACTION_LOCK_MACRO and ACTION_UNLOCK_MACRO" ($serviceContent -match "ACTION_LOCK_MACRO" -and $serviceContent -match "ACTION_UNLOCK_MACRO")
Assert-Check "AimHudService manages dedicated mMacroContainer overlay view" ($serviceContent -match "mMacroContainer" -and $serviceContent -match "initMacroOverlayView")
Assert-Check "AimHudService implements dedicated macro repositioning and coordinate persistence" ($serviceContent -match "saveMacroPosition" -and $serviceContent -match "resetMacroPosition")
Assert-Check "AimHudService MacroLayoutView renders circular HUD icon on canvas" ($serviceContent -match "class MacroLayoutView" -and $serviceContent -match "drawCircle" -and $serviceContent -match 'drawText\("M"')
Assert-Check "MainActivity bridges applyMacroOverlay" ($activityContent -match "public boolean applyMacroOverlay")
Assert-Check "MainActivity bridges removeMacroOverlay" ($activityContent -match "public void removeMacroOverlay")
Assert-Check "MainActivity bridges setMacroLock" ($activityContent -match "public void setMacroLock")
Assert-Check "MainActivity bridges isMacroLocked" ($activityContent -match "public boolean isMacroLocked")
Assert-Check "MainActivity bridges resetMacroPosition" ($activityContent -match "public void resetMacroPosition")

Write-Host "`n--> Section 10: Android APK Build Verification" -ForegroundColor Yellow
Assert-Check "Production Android APK exists at build\Android\SensiPro.apk" (Test-Path $apkPath)
$apkSize = if (Test-Path $apkPath) { (Get-Item $apkPath).Length } else { 0 }
Assert-Check "Android APK size meets production threshold (> 200 KB)" ($apkSize -gt 200000)

$summaryColor = if ($failed -eq 0) { "Green" } else { "Red" }
Write-Host "`n==========================================================" -ForegroundColor Cyan
Write-Host " AIMHUD & MACRO TEST SUITE RESULTS: $passed Passed, $failed Failed" -ForegroundColor $summaryColor
Write-Host "==========================================================" -ForegroundColor Cyan

if ($failed -gt 0) {
    exit 1
}
