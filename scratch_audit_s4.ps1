$html = Get-Content "D:\iphone sensi\index.html" -Raw -Encoding UTF8

# Find screenView4
$idx1 = $html.IndexOf('id="screenView4"')
$idx2 = $html.IndexOf('id="screenView5"')
if ($idx1 -ge 0 -and $idx2 -gt $idx1) {
    $s4 = $html.Substring($idx1, $idx2 - $idx1)
    Write-Host "Screen 4 block found, length: $($s4.Length)"
    
    $regex = [regex]'(onclick|oninput|onchange)="([^"]+)"'
    $matches = $regex.Matches($s4)
    Write-Host "Found $($matches.Count) event handler attributes:"
    
    foreach ($m in $matches) {
        $attr = $m.Groups[1].Value
        $call = $m.Groups[2].Value
        $fnRegex = [regex]'^([a-zA-Z0-9_]+)\('
        $fnMatch = $fnRegex.Match($call)
        if ($fnMatch.Success) {
            $fn = $fnMatch.Groups[1].Value
            $defined = $html -match "function\s+$fn\b"
            Write-Host "  [$attr] $call -> $fn (Defined in script: $defined)"
        } else {
            Write-Host "  [$attr] $call"
        }
    }
} else {
    Write-Host "Could not find bounds"
}
