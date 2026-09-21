<#
.SYNOPSIS
    Compiles and runs tests\ipc_smoke.cpp against the running Discord client.
    Requires Visual Studio 2022 Build Tools (C++ workload). Discord must be running.

.EXAMPLE
    .\tests\run_smoke.ps1 -ClientId 123456789012345678 -Seconds 15
#>
param(
    [string]$ClientId,
    [int]$Seconds = 9
)

$ErrorActionPreference = "Stop"
$root = Split-Path $PSScriptRoot -Parent
$out = Join-Path $root "build\tests"
New-Item -ItemType Directory -Force $out | Out-Null

if (-not $ClientId) {
    # Fall back to the ClientId in the repo's ini.
    $line = Get-Content (Join-Path $root "RDRBetterPresence.ini") | Where-Object { $_ -match '^ClientId=' } | Select-Object -First 1
    if ($line) { $ClientId = ($line -split '=', 2)[1].Trim() }
}
if (-not $ClientId) { throw "Pass -ClientId or set ClientId in RDRBetterPresence.ini" }

$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
$vsPath = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath | Select-Object -First 1
if (-not $vsPath) { throw "MSVC not found" }
$vcvars = Join-Path $vsPath "VC\Auxiliary\Build\vcvars64.bat"

$sources = @(
    "tests\ipc_smoke.cpp",
    "src\Log.cpp",
    "src\Config.cpp",
    "src\discord\DiscordIPC.cpp",
    "src\game\Regions.cpp",
    "src\game\Scripts.cpp",
    "src\presence\PresenceBuilder.cpp"
) | ForEach-Object { '"' + (Join-Path $root $_) + '"' }

$cmd = "call `"$vcvars`" >nul && cl /nologo /std:c++latest /EHsc /MT /O1 /utf-8 /W3 /DRDRBP_NO_REDHOOK /I`"$root\src`" /I`"$root\sdk`" /Fo`"$out\\`" /Fe`"$out\ipc_smoke.exe`" $($sources -join ' ')"
Write-Host "==> Compiling smoke test" -ForegroundColor Cyan
cmd /c $cmd
if ($LASTEXITCODE -ne 0) { throw "compile failed" }

Write-Host "==> Running (client $ClientId, $Seconds s)" -ForegroundColor Cyan
Push-Location $out
try {
    & "$out\ipc_smoke.exe" --client-id $ClientId --seconds $Seconds
    exit $LASTEXITCODE
} finally {
    Pop-Location
}
