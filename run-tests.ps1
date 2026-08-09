[CmdletBinding()]
param(
    [string]$BuildDir = "build",
    [int]$Port = 18080,
    [switch]$SkipBuild
)

$ErrorActionPreference = "Stop"
$root = $PSScriptRoot
$failures = 0

function Section($text) { Write-Host "`n=== $text ===" -ForegroundColor Cyan }

# Assert that $actual (trimmed) equals $expected, print PASS/FAIL, tally failures.
function Check($name, $expected, $actual) {
    $a = ($actual | Out-String).Trim()
    if ($a -eq $expected) {
        Write-Host "  PASS  $name" -ForegroundColor Green
    } else {
        Write-Host "  FAIL  $name" -ForegroundColor Red
        Write-Host "        expected: '$expected'"
        Write-Host "        actual:   '$a'"
        $script:failures++
    }
}

# Assert that $actual contains $substr.
function CheckContains($name, $substr, $actual) {
    $a = ($actual | Out-String)
    if ($a -match [regex]::Escape($substr)) {
        Write-Host "  PASS  $name" -ForegroundColor Green
    } else {
        Write-Host "  FAIL  $name (missing '$substr')" -ForegroundColor Red
        Write-Host "        actual: '$($a.Trim())'"
        $script:failures++
    }
}

# ---- Build --------------------------------------------------------------------

if (-not $SkipBuild) {
    Section "Configure + build"
    $env:CC = "gcc"; $env:CXX = "g++"
    & cmake -S $root -B (Join-Path $root $BuildDir) -G Ninja
    if ($LASTEXITCODE -ne 0) { Write-Host "configure failed" -ForegroundColor Red; exit 1 }
    & cmake --build (Join-Path $root $BuildDir)
    if ($LASTEXITCODE -ne 0) { Write-Host "build failed" -ForegroundColor Red; exit 1 }
}

$server = Join-Path $root "$BuildDir/server.exe"
$client = Join-Path $root "$BuildDir/client.exe"
if (-not (Test-Path $server) -or -not (Test-Path $client)) {
    Write-Host "server/client not built - run without -SkipBuild" -ForegroundColor Red
    exit 1
}

# ---- Unit tests ---------------------------------------------------------------

Section "Unit tests (ctest)"
& ctest --test-dir (Join-Path $root $BuildDir) --output-on-failure
if ($LASTEXITCODE -ne 0) { $failures++ }

# ---- End-to-end ---------------------------------------------------------------

Section "End-to-end (server + client)"
$log = Join-Path $root "$BuildDir/e2e-server.log"
$proc = Start-Process -FilePath $server `
    -ArgumentList "--listen_port", "$Port", "--log_level", "warn" `
    -PassThru -RedirectStandardOutput $log -WindowStyle Hidden

try {
    # Wait until the server is actually accepting connections (up to ~5s).
    $ready = $false
    foreach ($i in 1..50) {
        try {
            $c = New-Object System.Net.Sockets.TcpClient
            $c.Connect("127.0.0.1", $Port); $c.Close(); $ready = $true; break
        } catch { Start-Sleep -Milliseconds 100 }
    }
    if (-not $ready) { throw "server did not start listening on port $Port" }

    $cp = @("--connect_port", "$Port")
    Check        "set returns OK"        "OK"        (& $client @cp set user1 Alice)
    Check        "get returns value"     "Alice"     (& $client @cp get user1)
    Check        "get missing key"       "NOT_FOUND" (& $client @cp get ghost)
    Check        "set overwrites"        "OK"        (& $client @cp set user1 Bob)
    Check        "get sees new value"    "Bob"       (& $client @cp get user1)
    & $client @cp set user2 Carol | Out-Null
    $dump = & $client @cp dump
    CheckContains "dump has key count"   "Total keys: 2" $dump
    CheckContains "dump has user1"       "user1 = Bob"   $dump
    CheckContains "dump has user2"       "user2 = Carol" $dump
}
finally {
    if ($proc -and -not $proc.HasExited) { Stop-Process -Id $proc.Id -Force }
}

# ---- Summary ------------------------------------------------------------------

Section "Summary"
if ($failures -eq 0) {
    Write-Host "All checks passed." -ForegroundColor Green
    exit 0
} else {
    Write-Host "$failures check(s) failed." -ForegroundColor Red
    exit 1
}
