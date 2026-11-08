<#
.SYNOPSIS
    Remote Mac Build Workflow for SensiPanel.ipa
#>

param (
    [string]$MacHost = "",
    [string]$MacUser = "",
    [string]$RemoteDir = "~/SensiPanelBuild"
)

$targetDir = "$PSScriptRoot\..\build\iOS"
$targetIpa = "$targetDir\SensiPanel.ipa"

if (-not (Test-Path $targetDir)) {
    New-Item -ItemType Directory -Force -Path $targetDir | Out-Null
}

Write-Host "==========================================================" -ForegroundColor Cyan
Write-Host " Sensi Pro - Remote Mac Build Workflow" -ForegroundColor Cyan
Write-Host " Target Output: $targetIpa" -ForegroundColor Cyan
Write-Host "==========================================================" -ForegroundColor Cyan

if ([string]::IsNullOrWhiteSpace($MacHost)) {
    Write-Host "[INFO] No remote Mac host specified." -ForegroundColor Yellow
    Write-Host "Usage: .\ios\build_remote.ps1 -MacHost <ip_or_hostname> -MacUser <username>"
    exit 0
}

Write-Host "[1/4] Syncing source code to remote Mac ($MacUser@${MacHost}:${RemoteDir})..." -ForegroundColor Green
$projectRoot = Split-Path -Parent $PSScriptRoot
scp -r "$projectRoot" "${MacUser}@${MacHost}:${RemoteDir}"

Write-Host "[2/4] Triggering native Xcode build on macOS..." -ForegroundColor Green
ssh "${MacUser}@${MacHost}" "chmod +x ${RemoteDir}/ios/build_ipa.sh && ${RemoteDir}/ios/build_ipa.sh"

Write-Host "[3/4] Fetching verified SensiPanel.ipa back to Windows host..." -ForegroundColor Green
scp "${MacUser}@${MacHost}:${RemoteDir}/build/iOS/SensiPanel.ipa" "$targetIpa"

if (Test-Path $targetIpa) {
    $size = (Get-Item $targetIpa).Length
    Write-Host "[4/4] SUCCESS: Verified SensiPanel.ipa received ($size bytes)" -ForegroundColor Green
}
