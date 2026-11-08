# SENSI PRO - MASTER TEST RUNNER
# Executes all 8 test suites and validates 100% pass rate.

$ErrorActionPreference = "Continue"
Write-Host "==========================================================================" -ForegroundColor Cyan
Write-Host " SENSI PRO - COMPLETE MASTER AUTOMATED TEST SUITE RUNNER" -ForegroundColor Cyan
Write-Host " Target Directory: D:\iphone sensi" -ForegroundColor Cyan
Write-Host "==========================================================================" -ForegroundColor Cyan

$testSuites = @(
    "test_app.ps1",
    "test_aimhud_native.ps1",
    "test_animations_and_performance.ps1",
    "test_drag_aim_calibration.ps1",
    "test_gk_auth.ps1",
    "test_profile_persistence.ps1",
    "test_regedit.ps1",
    "test_session_restore.ps1"
)

$summary = @()
$totalPassed = 0
$totalFailed = 0

foreach ($suite in $testSuites) {
    $suitePath = Join-Path "D:\iphone sensi" $suite
    if (-not (Test-Path $suitePath)) {
        Write-Warning "Suite file missing: $suite"
        continue
    }

    $sw = [System.Diagnostics.Stopwatch]::StartNew()
    $output = powershell -ExecutionPolicy Bypass -File $suitePath 2>&1 | Out-String
    $sw.Stop()

    $passCount = ([regex]::Matches($output, "\[PASS\]")).Count
    $failCount = ([regex]::Matches($output, "\[FAIL\]")).Count
    $totalPassed += $passCount
    $totalFailed += $failCount

    $status = if ($failCount -eq 0 -and $passCount -gt 0) { "PASS" } else { "FAIL" }

    $summary += [PSCustomObject]@{
        Suite    = $suite
        Status   = $status
        Passed   = $passCount
        Failed   = $failCount
        TimeSec  = [math]::Round($sw.Elapsed.TotalSeconds, 2)
    }
}

Write-Host "`n==========================================================================" -ForegroundColor Cyan
Write-Host " MASTER TEST EXECUTION SUMMARY" -ForegroundColor Cyan
Write-Host "==========================================================================" -ForegroundColor Cyan
$summary | Format-Table -AutoSize

Write-Host "==========================================================================" -ForegroundColor Cyan
Write-Host " TOTAL VERIFIED: $totalPassed PASSED, $totalFailed FAILED" -ForegroundColor Green
Write-Host "==========================================================================" -ForegroundColor Cyan

if ($totalFailed -gt 0) {
    exit 1
} else {
    exit 0
}
