# Sensi Pro - Mobile Distribution & Standalone Web Server
$port = 8080
$root = "D:\iphone sensi"
$listener = New-Object System.Net.HttpListener
$listener.Prefixes.Add("http://+:$port/")

try {
    $listener.Start()
    Write-Output "=========================================================="
    Write-Output " Sensi Pro Mobile Server Running on port $port"
    Write-Output " Access from iPhone or Android via: http://192.168.0.5:$port"
    Write-Output "=========================================================="
} catch {
    Write-Error "Failed to start server: $($_.Exception.Message)"
    exit 1
}

while ($listener.IsListening) {
    try {
        $context = $listener.GetContext()
        $request = $context.Request
        $response = $context.Response

        $path = $request.Url.LocalPath.TrimStart('/')
        if ([string]::IsNullOrWhiteSpace($path) -or $path -eq "index.html") {
            $filePath = "$root\index.html"
            $contentType = "text/html; charset=utf-8"
        } elseif ($path -eq "SensiPro.apk" -or $path -eq "android") {
            $filePath = "$root\build\Android\SensiPro.apk"
            $contentType = "application/vnd.android.package-archive"
            $response.AddHeader("Content-Disposition", "attachment; filename=`"SensiPro.apk`"")
        } elseif ($path -eq "SensiPanel.ipa" -or $path -eq "Sensi.ipa" -or $path -eq "ios") {
            $filePath = "$root\build\iOS\SensiPanel.ipa"
            if (-not (Test-Path $filePath)) {
                $filePath = "$root\build\iOS\Sensi.ipa"
            }
            $contentType = "application/octet-stream"
            $response.AddHeader("Content-Disposition", "attachment; filename=`"SensiPanel.ipa`"")
        } elseif ($path -eq "manifest.json") {
            $filePath = "$root\manifest.json"
            $contentType = "application/manifest+json"
        } elseif ($path -eq "sw.js") {
            $filePath = "$root\sw.js"
            $contentType = "application/javascript; charset=utf-8"
        } elseif ($path -eq "icon.png" -or $path -eq "apple-touch-icon.png") {
            $filePath = "$root\$path"
            if (-not (Test-Path $filePath)) {
                $filePath = "$root\ios\$path"
            }
            $contentType = "image/png"
        } else {
            $filePath = "$root\$path"
            $contentType = "application/octet-stream"
        }

        if (Test-Path $filePath) {
            $bytes = [System.IO.File]::ReadAllBytes($filePath)
            $response.ContentType = $contentType
            $response.ContentLength64 = $bytes.Length
            $response.StatusCode = 200
            $response.OutputStream.Write($bytes, 0, $bytes.Length)
        } else {
            $response.StatusCode = 404
            $errBytes = [System.Text.Encoding]::UTF8.GetBytes("File Not Found")
            $response.OutputStream.Write($errBytes, 0, $errBytes.Length)
        }
        $response.Close()
    } catch {
        # Catch individual request exceptions so server continues running
    }
}
