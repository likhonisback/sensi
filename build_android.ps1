# ==============================================================================
# Sensi Pro - Automated Android APK Build Script
# Compiles, dexes, aligns, signs, and generates build\Android\SensiPro.apk
# ==============================================================================

param(
    [string]$ReleaseKeystore = "",
    [string]$KeystorePass = "",
    [string]$KeyAlias = "",
    [string]$KeyPass = ""
)

$ErrorActionPreference = "Stop"

$projectRoot = $PSScriptRoot
$androidDir = "$projectRoot\android"
$buildDir = "$androidDir\build"
$outputDir = "$projectRoot\build\Android"
$outputApk = "$outputDir\SensiPro.apk"

# Toolchain paths
$androidSdk = "C:\Users\Administrator\AppData\Local\Android\Sdk"
$buildTools = "$androidSdk\build-tools\36.0.0"
$androidJar = "$androidSdk\platforms\android-37.0\android.jar"
$jbrBin = "C:\Program Files\Android\Android Studio\jbr\bin"

$env:JAVA_HOME = "C:\Program Files\Android\Android Studio\jbr"
$env:PATH = "$jbrBin;$env:PATH"

$aapt2 = "$buildTools\aapt2.exe"
$d8 = "$buildTools\d8.bat"
$zipalign = "$buildTools\zipalign.exe"
$apksigner = "$buildTools\apksigner.bat"
$javac = "$jbrBin\javac.exe"
$jar = "$jbrBin\jar.exe"
$keytool = "$jbrBin\keytool.exe"

Write-Host "==========================================================" -ForegroundColor Cyan
Write-Host " Building Genuine Sensi Pro Android APK" -ForegroundColor Cyan
Write-Host " Target Output: $outputApk" -ForegroundColor Cyan
Write-Host "==========================================================" -ForegroundColor Cyan

# Ensure directories
if (-not (Test-Path $outputDir)) { New-Item -ItemType Directory -Force -Path $outputDir | Out-Null }
if (Test-Path $buildDir) { Remove-Item -Recurse -Force $buildDir | Out-Null }
New-Item -ItemType Directory -Force -Path "$buildDir\gen", "$buildDir\obj", "$buildDir\dex" | Out-Null

# Update assets with latest index.html
Copy-Item "$projectRoot\index.html" "$androidDir\app\src\main\assets\index.html" -Force

# 1. Compile Resources with aapt2
Write-Host "[1/6] Compiling Android resources with AAPT2..." -ForegroundColor Green
& $aapt2 compile --dir "$androidDir\app\src\main\res" -o "$buildDir\res.zip"

# 2. Link Resources
Write-Host "[2/6] Linking resources and packaging assets..." -ForegroundColor Green
& $aapt2 link -I $androidJar `
    --manifest "$androidDir\app\src\main\AndroidManifest.xml" `
    -A "$androidDir\app\src\main\assets" `
    -o "$buildDir\unaligned.apk" `
    --java "$buildDir\gen" `
    "$buildDir\res.zip" `
    --auto-add-overlay

# 3. Compile Java Sources
Write-Host "[3/6] Compiling Java classes with javac (Java 17 target)..." -ForegroundColor Green
$javaFiles = @(
    "$buildDir\gen\com\sensipro\app\R.java",
    "$androidDir\app\src\main\java\com\sensipro\app\MainActivity.java",
    "$androidDir\app\src\main\java\com\sensipro\app\FloatingGuideService.java",
    "$androidDir\app\src\main\java\com\sensipro\app\AimHudService.java"
)
& $javac --release 17 -encoding UTF-8 -d "$buildDir\obj" -cp $androidJar $javaFiles

# 4. Dex bytecode with D8
Write-Host "[4/6] Converting bytecode to Dalvik/ART classes.dex..." -ForegroundColor Green
$classFiles = Get-ChildItem "$buildDir\obj\com\sensipro\app\*.class" | ForEach-Object { $_.FullName }
& $d8 --output "$buildDir\dex" $classFiles --lib $androidJar

# 5. Add classes.dex into unaligned.apk
Write-Host "[5/6] Embedding classes.dex into APK..." -ForegroundColor Green
Push-Location "$buildDir\dex"
& $jar -uf "$buildDir\unaligned.apk" "classes.dex"
Pop-Location

# 6. Zipalign & Sign APK
Write-Host "[6/6] Zipaligning and signing SensiPro.apk..." -ForegroundColor Green
& $zipalign -f -p 4 "$buildDir\unaligned.apk" "$buildDir\aligned.apk"

# apksigner requires Java in PATH
$env:PATH = "$jbrBin;$env:PATH"
$env:JAVA_HOME = "C:\Program Files\Android\Android Studio\jbr"

if ($ReleaseKeystore -and (Test-Path $ReleaseKeystore)) {
    Write-Host "--> Signing with RELEASE keystore: $ReleaseKeystore" -ForegroundColor Cyan
    $passArg = if ($KeystorePass) { "pass:$KeystorePass" } else { "pass:android" }
    $aliasArgs = if ($KeyAlias) { @("--ks-key-alias", $KeyAlias) } else { @() }
    & $apksigner sign --ks $ReleaseKeystore --ks-pass $passArg @aliasArgs --out $outputApk "$buildDir\aligned.apk"
} else {
    Write-Host "--> Signing with DEVELOPMENT keystore (debug.keystore)" -ForegroundColor Yellow
    $keystore = "$androidDir\debug.keystore"
    if (-not (Test-Path $keystore)) {
        & $keytool -genkey -v -keystore $keystore -storepass android -alias androiddebugkey -keypass android -keyalg RSA -keysize 2048 -validity 10000 -dname "CN=Android Debug,O=Android,C=US"
    }
    & $apksigner sign --ks $keystore --ks-pass "pass:android" --out $outputApk "$buildDir\aligned.apk"
}

if (Test-Path $outputApk) {
    $apkSize = (Get-Item $outputApk).Length
    Write-Host "==========================================================" -ForegroundColor Green
    Write-Host " [SUCCESS] Genuine SensiPro.apk generated successfully!" -ForegroundColor Green
    Write-Host " Path: $outputApk" -ForegroundColor Green
    Write-Host " Size: $apkSize bytes ($([math]::Round($apkSize / 1KB, 1)) KB)" -ForegroundColor Green
    Write-Host "==========================================================" -ForegroundColor Green
} else {
    Write-Host "[ERROR] Failed to generate $outputApk" -ForegroundColor Red
    exit 1
}
