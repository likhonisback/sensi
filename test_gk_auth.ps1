# ==============================================================================
# GK AUTH COMPREHENSIVE SECURITY & VERIFICATION TEST SUITE
# Validates Scenarios A through I as required by GK Auth Security Specification
# ==============================================================================

$ErrorActionPreference = "Stop"
$projectRoot = $PSScriptRoot

Write-Host "==========================================================" -ForegroundColor Cyan
Write-Host " GK AUTH VERIFICATION TEST SUITE (SCENARIOS A - I)" -ForegroundColor Cyan
Write-Host " Testing target: $projectRoot\index.html" -ForegroundColor Cyan
Write-Host "==========================================================" -ForegroundColor Cyan

$passed = 0
$failed = 0

function Assert-Auth([string]$caseId, [string]$name, [scriptblock]$testBlock) {
    try {
        & $testBlock
        Write-Host "  [PASS] [Case $caseId] $name" -ForegroundColor Green
        $script:passed++
    } catch {
        Write-Host "  [FAIL] [Case $caseId] $name - Error: $_" -ForegroundColor Red
        $script:failed++
    }
}

$html = Get-Content "$projectRoot\index.html" -Raw

# ------------------------------------------------------------------------------
# Case A: Fresh application launch with no authenticated session
# ------------------------------------------------------------------------------
Assert-Auth "A" "Fresh application launch enforces locked state and presents login gateway" {
    # 1. Main app container must default to display: none
    if (-not ($html -match 'id="mainSensiApp"[^>]*style="display:\s*none;"')) {
        throw "mainSensiApp is not hidden by default in DOM"
    }
    # 2. Login screen must be present in DOM
    if (-not ($html -match 'id="sensiLoginScreen"')) {
        throw "sensiLoginScreen not found in DOM"
    }
    # 3. DOMContentLoaded handler must hide main app and display login screen
    if (-not ($html -match "mainApp\.style\.display\s*=\s*'none'")) {
        throw "DOMContentLoaded does not explicitly set mainApp display to none"
    }
    if (-not ($html -match "loginScreen\.style\.display\s*=\s*'flex'")) {
        throw "DOMContentLoaded does not explicitly set loginScreen display to flex"
    }
    # 4. GKAuth initial state must have authenticated: false
    if (-not ($html -match "authenticated:\s*false")) {
        throw "GKAuth.authenticated does not default to false"
    }
}

# ------------------------------------------------------------------------------
# Case B: Empty license key submission
# ------------------------------------------------------------------------------
Assert-Auth "B" "Empty or whitespace-only license key is rejected without server call or access grant" {
    # 1. submitLogin checks if (!key)
    if (-not ($html -match 'if\s*\(!key\)\s*\{\s*this\.showNotification\([^;]*Please enter your license key')) {
        throw "submitLogin does not validate and reject empty keys with warning message"
    }
    # 2. licenseLogin checks if (!licenseKey)
    if (-not ($html -match 'if\s*\(!licenseKey\)\s*\{\s*return\s*\{\s*success:\s*false')) {
        throw "licenseLogin does not reject empty keys"
    }
}

# ------------------------------------------------------------------------------
# Case C: Invalid license key submission against live GK Auth server
# ------------------------------------------------------------------------------
Assert-Auth "C" "Invalid license key receives server rejection and application remains strictly locked" {
    # Test live endpoint directly
    $apiUrl = "https://script.google.com/macros/s/AKfycbxeP11Q-xe0uRFIvqpAHRHvjjGDF1Bfl3ZFAC9eWYNcRpnqKgY42jUwwmx6IVu1WoONeQ/exec"
    $payload = @{
        sheet_id = "1MjFDrLZOX7IwGu7flMG5LwNs_vh8P5tK_e2OrXA_Xxs"
        action = "licenseLogin"
        license_key = "INVALID-TEST-KEY-XYZ-999"
        hwid = "TEST-HWID-AUTOMATION"
        uuid = "TEST-UUID-AUTOMATION"
    } | ConvertTo-Json -Compress

    $resp = curl.exe -L -s -H "Content-Type: application/json" -d $payload $apiUrl
    if (-not $resp.Contains('"success":false')) {
        throw "Live server did not reject invalid key: $resp"
    }

    # Verify client handler rejects invalid responses
    if (-not ($html -match 'Invalid license key or not bound to this device')) {
        throw "Missing user notification for invalid license key"
    }
}

# ------------------------------------------------------------------------------
# Case D: Valid license key processing logic
# ------------------------------------------------------------------------------
Assert-Auth "D" "Valid license key confirms server success and executes grantAccess" {
    # Check that grantAccess is called ONLY when res.success === true
    if (-not ($html -match 'if\s*\(res\s*&&\s*res\.success\s*===\s*true\)\s*\{[^}]*grantAccess')) {
        throw "grantAccess is not gated strictly by res.success === true"
    }
    # Verify grantAccess sets authenticated = true and shows main app
    if (-not ($html -match 'this\.authenticated\s*=\s*true')) {
        throw "grantAccess does not set authenticated = true"
    }
    if (-not ($html -match "mainApp\.style\.display\s*=\s*'flex'")) {
        throw "grantAccess does not display mainSensiApp"
    }
}

# ------------------------------------------------------------------------------
# Case E: Expired or revoked license key handling
# ------------------------------------------------------------------------------
Assert-Auth "E" "Expired or revoked license key is explicitly identified and rejected" {
    if (-not ($html -match 'License key has expired\. Please renew your subscription')) {
        throw "Missing expired license error handler"
    }
}

# ------------------------------------------------------------------------------
# Case F: Authentication server timeout or connection failure
# ------------------------------------------------------------------------------
Assert-Auth "F" "Server timeout or network failure keeps protected functionality locked" {
    # 1. 12-second abort controller timeout present
    if (-not ($html -match 'setTimeout\(\(\)\s*=>\s*controller\.abort\(\),\s*12000\)')) {
        throw "Missing AbortController 12-second timeout"
    }
    # 2. Timeout returns success: false
    if (-not ($html -match 'Connection timeout: Licensing server did not respond in time')) {
        throw "Missing timeout error mapping"
    }
    # 3. Network error returns success: false
    if (-not ($html -match 'Authentication server unavailable: Unable to connect to licensing server')) {
        throw "Missing connection failure error mapping"
    }
    # 4. autoValidateSaved does NOT grant access on network error
    if ($html -match 'Offline Mode: Retaining cached license session') {
        throw "Detected unsafe offline fallback that grants access on network error"
    }
}

# ------------------------------------------------------------------------------
# Case G: Application restart without an authenticated session
# ------------------------------------------------------------------------------
Assert-Auth "G" "Restarting application without a valid session prompts for license key" {
    # Verify init checks for saved key, purges invalid/stale keys, and defaults to login screen
    if (-not ($html -match 'PermanentStorage\.removeItem\(''gk_auth_license''\)')) {
        throw "init does not purge stale keys"
    }
    # Verify hardcoded test bypass keys are eliminated
    if ($html.Contains('COMPANION-DIRECT-ACCESS') -and $html.Contains('onclick="GKAuth.grantAccess(\''COMPANION-DIRECT-ACCESS\'', true)"')) {
        throw "Detected active offline companion mode bypass button in login screen"
    }
    if ($html -match 'licenseKey\s*===\s*"1"\s*\|\|\s*lk\s*===\s*"guest"') {
        throw "Detected hardcoded test key bypasses in licenseLogin"
    }
}

# ------------------------------------------------------------------------------
# Case H: Repeated login clicks and repeated initialization (Race Condition Protection)
# ------------------------------------------------------------------------------
Assert-Auth "H" "Duplicate initialization and rapid multiple submit clicks are strictly guarded" {
    # 1. init _initialized guard
    if (-not ($html -match 'if\s*\(this\._initialized\)\s*return;')) {
        throw "Missing this._initialized guard in GKAuth.init"
    }
    # 2. submitLogin isProcessing guard
    if (-not ($html -match 'if\s*\(this\.isProcessing\)\s*return;')) {
        throw "Missing this.isProcessing guard in GKAuth.submitLogin"
    }
    # 3. Login button and input disabled while processing
    if (-not ($html -match 'if\s*\(btn\)\s*btn\.disabled\s*=\s*loading;')) {
        throw "Login button is not disabled during active processing"
    }
}

# ------------------------------------------------------------------------------
# Case I: Logout & Session Invalidation
# ------------------------------------------------------------------------------
Assert-Auth "I" "Signing out fully invalidates session, locks protected views, and returns to login gateway" {
    # 1. signOut removes stored credentials
    if (-not ($html -match 'PermanentStorage\.removeItem\(''gk_auth_license''\);[\s\r\n]*localStorage\.removeItem\(''gk_auth_license''\);')) {
        throw "signOut does not remove gk_auth_license from permanent storage and localStorage"
    }
    # 2. signOut sets authenticated = false
    if (-not ($html -match 'this\.authenticated\s*=\s*false;[\s\r\n]*this\.activeKey\s*=\s*"";')) {
        throw "signOut does not reset authenticated to false"
    }
    # 3. signOut hides main app and shows login screen
    if (-not ($html -match 'mainApp\.style\.display\s*=\s*''none'';[\s\r\n]*\}[\s\r\n]*const\s*loginScreen')) {
        throw "signOut does not hide mainSensiApp"
    }
    # 4. View switcher and menu block access when not authenticated
    if (-not ($html -match 'if\s*\(typeof GKAuth !== ''undefined'' && !GKAuth\.authenticated\)\s*\{\s*return;')) {
        throw "switchView is not guarded by GKAuth.authenticated"
    }
    if (-not ($html -match 'if\s*\(typeof GKAuth !== ''undefined'' && !GKAuth\.authenticated\)\s*\{\s*if \(typeof showToast')) {
        throw "openMenu is not guarded by GKAuth.authenticated"
    }
}

Write-Host "`n==========================================================" -ForegroundColor Cyan
$color = if ($failed -eq 0) { "Green" } else { "Red" }
Write-Host " GK Auth Test Suite Results: $passed Passed, $failed Failed" -ForegroundColor $color
Write-Host "==========================================================" -ForegroundColor Cyan

if ($failed -gt 0) { exit 1 } else { exit 0 }
