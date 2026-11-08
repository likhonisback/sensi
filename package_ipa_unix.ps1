Add-Type -AssemblyName System.IO.Compression
Add-Type -AssemblyName System.IO.Compression.FileSystem

$payloadDir = "D:\iphone sensi\build\iOS\Payload"
$baseDir = "D:\iphone sensi\build\iOS"
$ipaPath = "D:\iphone sensi\build\iOS\SensiPanel.ipa"
$ipaPathMirror = "D:\iphone sensi\build\iOS\Sensi.ipa"

if (Test-Path $ipaPath) { Remove-Item $ipaPath -Force }
if (Test-Path $ipaPathMirror) { Remove-Item $ipaPathMirror -Force }

$zip = [System.IO.Compression.ZipFile]::Open($ipaPath, [System.IO.Compression.ZipArchiveMode]::Create)
$files = Get-ChildItem -Path $payloadDir -Recurse -File

foreach ($file in $files) {
    $relPath = $file.FullName.Substring($baseDir.Length + 1).Replace("\", "/")
    [System.IO.Compression.ZipFileExtensions]::CreateEntryFromFile($zip, $file.FullName, $relPath, [System.IO.Compression.CompressionLevel]::Optimal) | Out-Null
    Write-Host "Added: $relPath"
}
$zip.Dispose()

Copy-Item -Path $ipaPath -Destination $ipaPathMirror -Force
Write-Host "Successfully generated SensiPanel.ipa and Sensi.ipa with standard forward-slash entries!"
