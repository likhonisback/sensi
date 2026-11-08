# ==============================================================================
# Sensi Pro - High-Quality App Icon Generator for Android & iOS
# Uses user-provided reference image (media_1792339966901_4754d302.png)
# ==============================================================================

Add-Type -AssemblyName System.Drawing

$projectRoot = $PSScriptRoot
$srcPath = "C:\Users\Administrator\.gemini\antigravity\brain\40cbb77a-8f4f-451e-95fd-f6a3abfa1bc0\.user_uploaded\media_1792339966901_4754d302.png"

if (-not (Test-Path $srcPath)) {
    Write-Error "Source reference image not found at $srcPath"
    exit 1
}

Write-Host "Loading reference image: $srcPath" -ForegroundColor Cyan
$srcBmp = [System.Drawing.Bitmap]::FromFile($srcPath)

# 1. Create transparent foreground cutout
$cutout = New-Object System.Drawing.Bitmap $srcBmp.Width, $srcBmp.Height, ([System.Drawing.Imaging.PixelFormat]::Format32bppArgb)

for ($y = 0; $y -lt $srcBmp.Height; $y++) {
    for ($x = 0; $x -lt $srcBmp.Width; $x++) {
        $p = $srcBmp.GetPixel($x, $y)
        $dist = [Math]::Sqrt([Math]::Pow($p.R - 16, 2) + [Math]::Pow($p.G - 33, 2) + [Math]::Pow($p.B - 66, 2))
        
        $alpha = 0.0
        if ($dist -gt 22) {
            $alpha = 1.0
        } elseif ($dist -gt 6) {
            $alpha = ($dist - 6) / (22 - 6)
        }
        
        # Smooth vertical feathering at bottom collar (y=57..62)
        if ($y -ge 57 -and $alpha -gt 0) {
            $yFade = [Math]::Max(0.0, [Math]::Min(1.0, (62.0 - $y) / (62.0 - 57.0)))
            $alpha = $alpha * $yFade
        }
        
        $finalA = [int]($alpha * 255)
        if ($finalA -gt 0) {
            $cutout.SetPixel($x, $y, [System.Drawing.Color]::FromArgb($finalA, $p.R, $p.G, $p.B))
        } else {
            $cutout.SetPixel($x, $y, [System.Drawing.Color]::FromArgb(0, 0, 0, 0))
        }
    }
}

# 2. Helper to render square icon (solid #102142 background + centered avatar)
function New-SquareIcon([int]$size, [double]$avatarScaleRatio) {
    $bmp = New-Object System.Drawing.Bitmap $size, $size, ([System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
    $g = [System.Drawing.Graphics]::FromImage($bmp)
    $g.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
    $g.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::HighQuality
    $g.PixelOffsetMode = [System.Drawing.Drawing2D.PixelOffsetMode]::HighQuality
    $g.CompositingQuality = [System.Drawing.Drawing2D.CompositingQuality]::HighQuality

    # Background color #102142
    $bgColor = [System.Drawing.Color]::FromArgb(255, 16, 33, 66)
    $g.Clear($bgColor)

    # Avatar scale
    $s = ($size / 1024.0) * $avatarScaleRatio
    $dw = [int](83 * $s)
    $dh = [int](67 * $s)
    $dx = [int](($size - $dw) / 2)
    $dy = [int](($size / 2.0) - (37.5 * $s))

    $g.DrawImage($cutout, $dx, $dy, $dw, $dh)
    $g.Dispose()
    return $bmp
}

# 3. Helper to render circular icon (for Android roundIcon)
function New-RoundIcon([int]$size, [double]$avatarScaleRatio) {
    $bmp = New-Object System.Drawing.Bitmap $size, $size, ([System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
    $g = [System.Drawing.Graphics]::FromImage($bmp)
    $g.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
    $g.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::HighQuality
    $g.PixelOffsetMode = [System.Drawing.Drawing2D.PixelOffsetMode]::HighQuality
    $g.CompositingQuality = [System.Drawing.Drawing2D.CompositingQuality]::HighQuality

    $g.Clear([System.Drawing.Color]::Transparent)

    # Create circular clipping path
    $path = New-Object System.Drawing.Drawing2D.GraphicsPath
    $path.AddEllipse(0, 0, $size, $size)
    $g.SetClip($path)

    # Fill circle with #102142
    $bgColor = [System.Drawing.Color]::FromArgb(255, 16, 33, 66)
    $brush = New-Object System.Drawing.SolidBrush $bgColor
    $g.FillEllipse($brush, 0, 0, $size, $size)
    $brush.Dispose()

    # Draw centered avatar
    $s = ($size / 1024.0) * $avatarScaleRatio
    $dw = [int](83 * $s)
    $dh = [int](67 * $s)
    $dx = [int](($size - $dw) / 2)
    $dy = [int](($size / 2.0) - (37.5 * $s))

    $g.DrawImage($cutout, $dx, $dy, $dw, $dh)
    $path.Dispose()
    $g.Dispose()
    return $bmp
}

# 4. Helper to render adaptive icon foreground (transparent background)
function New-ForegroundIcon([int]$size, [double]$avatarScaleRatio) {
    $bmp = New-Object System.Drawing.Bitmap $size, $size, ([System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
    $g = [System.Drawing.Graphics]::FromImage($bmp)
    $g.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
    $g.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::HighQuality
    $g.PixelOffsetMode = [System.Drawing.Drawing2D.PixelOffsetMode]::HighQuality
    $g.CompositingQuality = [System.Drawing.Drawing2D.CompositingQuality]::HighQuality

    $g.Clear([System.Drawing.Color]::Transparent)

    # Adaptive icons use 108dp canvas with safe zone of central 72dp (66.6%)
    # In $size, scale ratio is relative to 432px
    $s = ($size / 432.0) * $avatarScaleRatio
    $dw = [int](83 * $s)
    $dh = [int](67 * $s)
    $dx = [int](($size - $dw) / 2)
    $dy = [int](($size / 2.0) - (37.5 * $s))

    $g.DrawImage($cutout, $dx, $dy, $dw, $dh)
    $g.Dispose()
    return $bmp
}

Write-Host "Generating Android launcher icon densities..." -ForegroundColor Green

# Android Mipmap Densities:
# mdpi: 48x48 (fg 108x108)
# hdpi: 72x72 (fg 162x162)
# xhdpi: 96x96 (fg 216x216)
# xxhdpi: 144x144 (fg 324x324)
# xxxhdpi: 192x192 (fg 432x432)
$androidRes = "$projectRoot\android\app\src\main\res"

$densities = @(
    @{ Name = "mipmap-mdpi"; Size = 48; FgSize = 108 },
    @{ Name = "mipmap-hdpi"; Size = 72; FgSize = 162 },
    @{ Name = "mipmap-xhdpi"; Size = 96; FgSize = 216 },
    @{ Name = "mipmap-xxhdpi"; Size = 144; FgSize = 324 },
    @{ Name = "mipmap-xxxhdpi"; Size = 192; FgSize = 432 }
)

foreach ($d in $densities) {
    $dir = "$androidRes\$($d.Name)"
    if (-not (Test-Path $dir)) { New-Item -ItemType Directory -Force -Path $dir | Out-Null }
    
    # 1. Standard square icon
    $sq = New-SquareIcon $d.Size 13.5
    $sq.Save("$dir\ic_launcher.png", [System.Drawing.Imaging.ImageFormat]::Png)
    $sq.Dispose()
    
    # 2. Round icon
    $rd = New-RoundIcon $d.Size 13.5
    $rd.Save("$dir\ic_launcher_round.png", [System.Drawing.Imaging.ImageFormat]::Png)
    $rd.Dispose()
    
    # 3. Adaptive foreground layer (avatar scale in safe zone)
    $fg = New-ForegroundIcon $d.FgSize 5.8
    $fg.Save("$dir\ic_launcher_foreground.png", [System.Drawing.Imaging.ImageFormat]::Png)
    $fg.Dispose()
}

# Adaptive XML in mipmap-anydpi-v26
$anydpiDir = "$androidRes\mipmap-anydpi-v26"
if (-not (Test-Path $anydpiDir)) { New-Item -ItemType Directory -Force -Path $anydpiDir | Out-Null }

$xmlAdaptive = @'
<?xml version="1.0" encoding="utf-8"?>
<adaptive-icon xmlns:android="http://schemas.android.com/apk/res/android">
    <background android:drawable="@color/ic_launcher_background" />
    <foreground android:drawable="@mipmap/ic_launcher_foreground" />
</adaptive-icon>
'@
Set-Content -Path "$anydpiDir\ic_launcher.xml" -Value $xmlAdaptive -Encoding UTF8
Set-Content -Path "$anydpiDir\ic_launcher_round.xml" -Value $xmlAdaptive -Encoding UTF8

# Ensure values\colors.xml defines ic_launcher_background
$colorsXml = "$androidRes\values\colors.xml"
$colorsContent = @'
<?xml version="1.0" encoding="utf-8"?>
<resources>
    <color name="ic_launcher_background">#102142</color>
    <color name="primary">#0b0c10</color>
    <color name="neon_red">#ff2b45</color>
</resources>
'@
Set-Content -Path $colorsXml -Value $colorsContent -Encoding UTF8

Write-Host "Generating iOS AppIcon assets..." -ForegroundColor Green

$iosAppDir = "$projectRoot\build\iOS\Payload\SensiPanel.app"
if (-not (Test-Path $iosAppDir)) { New-Item -ItemType Directory -Force -Path $iosAppDir | Out-Null }

# iOS required sizes
$iosSizes = @(
    @{ Name = "AppIcon60x60@2x.png"; Size = 120 },
    @{ Name = "AppIcon60x60@3x.png"; Size = 180 },
    @{ Name = "AppIcon76x76@2x~ipad.png"; Size = 152 },
    @{ Name = "AppIcon83.5x83.5@2x~ipad.png"; Size = 167 },
    @{ Name = "AppIcon-512.png"; Size = 512 },
    @{ Name = "AppIcon-1024.png"; Size = 1024 },
    @{ Name = "AppIcon.png"; Size = 120 },
    @{ Name = "iTunesArtwork"; Size = 512 },
    @{ Name = "iTunesArtwork@2x"; Size = 1024 }
)

foreach ($item in $iosSizes) {
    $outPath = "$iosAppDir\$($item.Name)"
    $icon = New-SquareIcon $item.Size 13.5
    $icon.Save($outPath, [System.Drawing.Imaging.ImageFormat]::Png)
    $icon.Dispose()
}

# Also save master icons in project root and web assets
$master1024 = New-SquareIcon 1024 13.5
$master1024.Save("$projectRoot\icon_master_1024.png", [System.Drawing.Imaging.ImageFormat]::Png)
$master1024.Save("$projectRoot\icon.png", [System.Drawing.Imaging.ImageFormat]::Png)

$appleTouchIcon = New-SquareIcon 180 13.5
$appleTouchIcon.Save("$projectRoot\apple-touch-icon.png", [System.Drawing.Imaging.ImageFormat]::Png)
$appleTouchIcon.Save("$projectRoot\android\app\src\main\assets\apple-touch-icon.png", [System.Drawing.Imaging.ImageFormat]::Png)
$appleTouchIcon.Save("$iosAppDir\apple-touch-icon.png", [System.Drawing.Imaging.ImageFormat]::Png)

$fav192 = New-SquareIcon 192 13.5
$fav192.Save("$projectRoot\android\app\src\main\assets\icon.png", [System.Drawing.Imaging.ImageFormat]::Png)
$fav192.Save("$iosAppDir\icon.png", [System.Drawing.Imaging.ImageFormat]::Png)

$fav192.Dispose()
$appleTouchIcon.Dispose()
$master1024.Dispose()
$cutout.Dispose()
$srcBmp.Dispose()

Write-Host "All Android & iOS App Icon assets successfully generated!" -ForegroundColor Green
