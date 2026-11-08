# ==============================================================================
# Sensi Pro - Pure Logo Launcher Icon Generator (Transparent, No Background)
# Uses user's exact uploaded transparent logo: media_1792342097568_5a08f04e.png
# Extracts ONLY the head logo (hair, forehead, eyes, demon teeth, jaw/chin)
# Eliminates collar bar, residual outer dark background boxes, and blue halos
# Preserves internal facial shadows and glowing features
# ==============================================================================

Add-Type -AssemblyName System.Drawing

$projectRoot = $PSScriptRoot
$srcPath = "C:\Users\Administrator\.gemini\antigravity\brain\40cbb77a-8f4f-451e-95fd-f6a3abfa1bc0\.user_uploaded\media_1792342097568_5a08f04e.png"

if (-not (Test-Path $srcPath)) {
    Write-Error "Source image not found at $srcPath"
    exit 1
}

Write-Host "Loading user-provided head logo: $srcPath" -ForegroundColor Cyan
$srcBmp = [System.Drawing.Bitmap]::FromFile($srcPath)

# 1. Clean the source image: remove collar bar, outer lower boxes, and navy halo
$cleanBmp = New-Object System.Drawing.Bitmap $srcBmp.Width, $srcBmp.Height, ([System.Drawing.Imaging.PixelFormat]::Format32bppArgb)

# Chin contour boundaries:
$chinBounds = @{
    55 = @{ min = 26; max = 47 }
    56 = @{ min = 26; max = 46 }
    57 = @{ min = 26; max = 46 }
    58 = @{ min = 27; max = 45 }
    59 = @{ min = 27; max = 44 }
    60 = @{ min = 28; max = 44 }
    61 = @{ min = 29; max = 43 }
    62 = @{ min = 30; max = 42 }
    63 = @{ min = 32; max = 41 }
    64 = @{ min = 35; max = 39 }
}

for ($y = 0; $y -lt $srcBmp.Height; $y++) {
    for ($x = 0; $x -lt $srcBmp.Width; $x++) {
        $c = $srcBmp.GetPixel($x, $y)
        
        # 1. Low alpha removal
        if ($c.A -lt 25) {
            $cleanBmp.SetPixel($x, $y, [System.Drawing.Color]::Transparent)
            continue
        }

        # 2. Outer blue/navy halo removal
        if ($c.B -gt ($c.R + 12) -and $c.B -gt ($c.G + 8) -and $c.R -lt 60) {
            $cleanBmp.SetPixel($x, $y, [System.Drawing.Color]::Transparent)
            continue
        }

        # 3. Y >= 65 is collar bar
        if ($y -ge 65) {
            $cleanBmp.SetPixel($x, $y, [System.Drawing.Color]::Transparent)
            continue
        }

        # 4. Chin contour taper for Y >= 55
        if ($chinBounds.ContainsKey($y)) {
            $cmin = $chinBounds[$y].min
            $cmax = $chinBounds[$y].max
            
            if ($x -lt $cmin) {
                # Check left hair strand (ends at Y=56, X>=21, silver color)
                if ($y -le 56 -and $x -ge 21 -and ($c.R -gt 85 -or $c.G -gt 50)) {
                    # keep hair strand
                } else {
                    $cleanBmp.SetPixel($x, $y, [System.Drawing.Color]::Transparent)
                    continue
                }
            }
            if ($x -gt $cmax) {
                $cleanBmp.SetPixel($x, $y, [System.Drawing.Color]::Transparent)
                continue
            }
        }

        # For Y=64 (chin tip), soften alpha slightly for natural taper
        if ($y -eq 64) {
            $softAlpha = [int]($c.A * 0.75)
            $cleanBmp.SetPixel($x, $y, [System.Drawing.Color]::FromArgb($softAlpha, $c.R, $c.G, $c.B))
            continue
        }

        $cleanBmp.SetPixel($x, $y, $c)
    }
}

# 2. Find exact bounding box of cleaned head
$minX = 999; $maxX = -1; $minY = 999; $maxY = -1
for ($y = 0; $y -lt $cleanBmp.Height; $y++) {
    for ($x = 0; $x -lt $cleanBmp.Width; $x++) {
        if ($cleanBmp.GetPixel($x, $y).A -gt 0) {
            if ($x -lt $minX) { $minX = $x }
            if ($x -gt $maxX) { $maxX = $x }
            if ($y -lt $minY) { $minY = $y }
            if ($y -gt $maxY) { $maxY = $y }
        }
    }
}

$headW = $maxX - $minX + 1
$headH = $maxY - $minY + 1
Write-Host "Exact Head Bounds: X=$minX..$maxX (W=$headW), Y=$minY..$maxY (H=$headH)" -ForegroundColor Yellow

# 3. Create 1024x1024 Master Transparent Bitmap
$masterBmp = New-Object System.Drawing.Bitmap 1024, 1024, ([System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
$gm = [System.Drawing.Graphics]::FromImage($masterBmp)
$gm.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
$gm.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::HighQuality
$gm.PixelOffsetMode = [System.Drawing.Drawing2D.PixelOffsetMode]::HighQuality
$gm.CompositingQuality = [System.Drawing.Drawing2D.CompositingQuality]::HighQuality
$gm.Clear([System.Drawing.Color]::Transparent)

# Centered with safe margin (~68% height)
$srcRect = New-Object System.Drawing.Rectangle $minX, $minY, $headW, $headH
$targetH = 680
$targetW = [int]($targetH * ($headW / [double]$headH))
$targetX = [int]((1024 - $targetW) / 2)
$targetY = [int]((1024 - $targetH) / 2)

$destRect = New-Object System.Drawing.Rectangle $targetX, $targetY, $targetW, $targetH
$gm.DrawImage($cleanBmp, $destRect, $srcRect, [System.Drawing.GraphicsUnit]::Pixel)
$gm.Dispose()
$cleanBmp.Dispose()
$srcBmp.Dispose()

# Save master PNG files
$masterBmp.Save("$projectRoot\master_logo_transparent_1024.png", [System.Drawing.Imaging.ImageFormat]::Png)
$masterBmp.Save("$projectRoot\icon_master_1024.png", [System.Drawing.Imaging.ImageFormat]::Png)
$masterBmp.Save("$projectRoot\icon.png", [System.Drawing.Imaging.ImageFormat]::Png)

# 4. Create 3 verification previews (512x512)
# A: Over Pure White
$prevWhite = New-Object System.Drawing.Bitmap 512, 512, ([System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
$gw = [System.Drawing.Graphics]::FromImage($prevWhite)
$gw.Clear([System.Drawing.Color]::White)
$gw.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
$gw.DrawImage($masterBmp, 0, 0, 512, 512)
$gw.Dispose()
$prevWhite.Save("$projectRoot\preview_on_white.png", [System.Drawing.Imaging.ImageFormat]::Png)
$prevWhite.Dispose()

# B: Over Pure Black
$prevBlack = New-Object System.Drawing.Bitmap 512, 512, ([System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
$gb = [System.Drawing.Graphics]::FromImage($prevBlack)
$gb.Clear([System.Drawing.Color]::Black)
$gb.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
$gb.DrawImage($masterBmp, 0, 0, 512, 512)
$gb.Dispose()
$prevBlack.Save("$projectRoot\preview_on_black.png", [System.Drawing.Imaging.ImageFormat]::Png)
$prevBlack.Dispose()

# C: Over Colorful Background (Gradient cyan to purple)
$prevColor = New-Object System.Drawing.Bitmap 512, 512, ([System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
$gc = [System.Drawing.Graphics]::FromImage($prevColor)
$colorBrush = New-Object System.Drawing.Drawing2D.LinearGradientBrush (New-Object System.Drawing.Point 0, 0), (New-Object System.Drawing.Point 512, 512), `
    ([System.Drawing.Color]::FromArgb(255, 0, 180, 219)), ([System.Drawing.Color]::FromArgb(255, 0, 131, 176))
$gc.FillRectangle($colorBrush, 0, 0, 512, 512)
$gc.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
$gc.DrawImage($masterBmp, 0, 0, 512, 512)
$gc.Dispose()
$colorBrush.Dispose()
$prevColor.Save("$projectRoot\preview_on_color.png", [System.Drawing.Imaging.ImageFormat]::Png)
$prevColor.Dispose()

# Helper function to resize transparent bitmap with HighQualityBicubic
function Resize-TransparentBmp([System.Drawing.Bitmap]$source, [int]$nw, [int]$nh) {
    $dest = New-Object System.Drawing.Bitmap $nw, $nh, ([System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
    $g = [System.Drawing.Graphics]::FromImage($dest)
    $g.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
    $g.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::HighQuality
    $g.PixelOffsetMode = [System.Drawing.Drawing2D.PixelOffsetMode]::HighQuality
    $g.CompositingQuality = [System.Drawing.Drawing2D.CompositingQuality]::HighQuality
    $g.Clear([System.Drawing.Color]::Transparent)

    $g.DrawImage($source, 0, 0, $nw, $nh)
    $g.Dispose()
    return $dest
}

# Helper function for Android Adaptive Icon Foreground:
function New-AdaptiveForeground([System.Drawing.Bitmap]$source, [int]$canvasSize) {
    $dest = New-Object System.Drawing.Bitmap $canvasSize, $canvasSize, ([System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
    $g = [System.Drawing.Graphics]::FromImage($dest)
    $g.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
    $g.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::HighQuality
    $g.PixelOffsetMode = [System.Drawing.Drawing2D.PixelOffsetMode]::HighQuality
    $g.CompositingQuality = [System.Drawing.Drawing2D.CompositingQuality]::HighQuality
    $g.Clear([System.Drawing.Color]::Transparent)

    # In adaptive canvas, scale to 85% of canvas size so that entire head stays inside 66dp safe circle
    $logoSize = [int]($canvasSize * 0.85)
    $offset = [int](($canvasSize - $logoSize) / 2)

    $g.DrawImage($source, $offset, $offset, $logoSize, $logoSize)
    $g.Dispose()
    return $dest
}

Write-Host "Generating Android pure freeform transparent launcher icons (ic_launcher.png only)..." -ForegroundColor Green

$androidRes = "$projectRoot\android\app\src\main\res"

# Clean up any adaptive icon XMLs or round icon overrides to prevent OS from applying a square/round shape
$anydpiDir = "$androidRes\mipmap-anydpi-v26"
if (Test-Path $anydpiDir) { Remove-Item -Recurse -Force $anydpiDir | Out-Null }

$densities = @(
    @{ Name = "mipmap-mdpi"; Size = 48 },
    @{ Name = "mipmap-hdpi"; Size = 72 },
    @{ Name = "mipmap-xhdpi"; Size = 96 },
    @{ Name = "mipmap-xxhdpi"; Size = 144 },
    @{ Name = "mipmap-xxxhdpi"; Size = 192 }
)

foreach ($d in $densities) {
    $dir = "$androidRes\$($d.Name)"
    if (-not (Test-Path $dir)) { New-Item -ItemType Directory -Force -Path $dir | Out-Null }
    
    # Standard launcher icon (100% transparent background, pure head logo only)
    $sq = Resize-TransparentBmp $masterBmp $d.Size $d.Size
    $sq.Save("$dir\ic_launcher.png", [System.Drawing.Imaging.ImageFormat]::Png)
    $sq.Dispose()

    # Clean up any leftover round or foreground files in density folders
    if (Test-Path "$dir\ic_launcher_round.png") { Remove-Item -Force "$dir\ic_launcher_round.png" }
    if (Test-Path "$dir\ic_launcher_foreground.png") { Remove-Item -Force "$dir\ic_launcher_foreground.png" }
}

Write-Host "Generating iOS AppIcon assets..." -ForegroundColor Green

$iosAppDir = "$projectRoot\build\iOS\Payload\SensiPanel.app"
if (-not (Test-Path $iosAppDir)) { New-Item -ItemType Directory -Force -Path $iosAppDir | Out-Null }

# iOS required sizes: All exported directly from transparent master logo
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
    $icon = Resize-TransparentBmp $masterBmp $item.Size $item.Size
    $icon.Save($outPath, [System.Drawing.Imaging.ImageFormat]::Png)
    $icon.Dispose()
}

# Web & asset sync
$appleTouchIcon = Resize-TransparentBmp $masterBmp 180 180
$appleTouchIcon.Save("$projectRoot\apple-touch-icon.png", [System.Drawing.Imaging.ImageFormat]::Png)
$appleTouchIcon.Save("$projectRoot\android\app\src\main\assets\apple-touch-icon.png", [System.Drawing.Imaging.ImageFormat]::Png)
$appleTouchIcon.Save("$iosAppDir\apple-touch-icon.png", [System.Drawing.Imaging.ImageFormat]::Png)

$fav192 = Resize-TransparentBmp $masterBmp 192 192
$fav192.Save("$projectRoot\android\app\src\main\assets\icon.png", [System.Drawing.Imaging.ImageFormat]::Png)
$fav192.Save("$iosAppDir\icon.png", [System.Drawing.Imaging.ImageFormat]::Png)

$fav192.Dispose()
$appleTouchIcon.Dispose()
$masterBmp.Dispose()

# Copy previews and master to artifact directory
$artDir = "C:\Users\Administrator\.gemini\antigravity\brain\40cbb77a-8f4f-451e-95fd-f6a3abfa1bc0"
Copy-Item "$projectRoot\master_logo_transparent_1024.png" "$artDir\master_logo_transparent_1024.png" -Force
Copy-Item "$projectRoot\preview_on_white.png" "$artDir\preview_on_white.png" -Force
Copy-Item "$projectRoot\preview_on_black.png" "$artDir\preview_on_black.png" -Force
Copy-Item "$projectRoot\preview_on_color.png" "$artDir\preview_on_color.png" -Force

Write-Host "All transparent launcher icon assets generated successfully from uploaded image!" -ForegroundColor Green
