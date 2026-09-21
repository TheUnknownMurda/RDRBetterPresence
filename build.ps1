<#
.SYNOPSIS
    Builds RDRBetterPresence.red with MSBuild (Visual Studio 2022 Build Tools) and,
    unless -NoDeploy is given, copies it next to RDR.exe.

.PARAMETER Configuration
    Release (default) or Debug.

.PARAMETER GameDir
    Red Dead Redemption install folder. Defaults to the Rockstar Launcher location on E:.

.PARAMETER NoDeploy
    Only build; do not copy anything into the game folder.

.EXAMPLE
    .\build.ps1
    .\build.ps1 -Configuration Debug
    .\build.ps1 -GameDir "D:\Games\Red Dead Redemption"
#>
param(
    [ValidateSet("Release", "Debug")]
    [string]$Configuration = "Release",
    [string]$GameDir = "E:\Program Files\Rockstar Games\Red Dead Redemption",
    [switch]$NoDeploy
)

$ErrorActionPreference = "Stop"
$root = $PSScriptRoot

# --- Locate MSBuild ----------------------------------------------------------------------------
$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
if (-not (Test-Path $vswhere)) {
    throw "vswhere.exe not found. Install Visual Studio 2022 Build Tools with the 'Desktop development with C++' workload."
}
$msbuild = & $vswhere -latest -products * -requires Microsoft.Component.MSBuild -find "MSBuild\**\Bin\MSBuild.exe" | Select-Object -First 1
if (-not $msbuild) {
    throw "MSBuild.exe not found. Install the 'Desktop development with C++' workload."
}

# --- Build ---------------------------------------------------------------------------------------
Write-Host "==> Building RDRBetterPresence ($Configuration)" -ForegroundColor Cyan
& $msbuild "$root\RDRBetterPresence.vcxproj" "/p:Configuration=$Configuration" "/p:Platform=x64" "/m" "/nologo" "/v:minimal"
if ($LASTEXITCODE -ne 0) {
    throw "Build failed (exit code $LASTEXITCODE)."
}

$plugin = Join-Path $root "build\$Configuration\RDRBetterPresence.red"
if (-not (Test-Path $plugin)) {
    throw "Build succeeded but $plugin is missing."
}
Write-Host "==> Built $plugin ($([math]::Round((Get-Item $plugin).Length / 1KB)) KB)" -ForegroundColor Green

if ($NoDeploy) {
    return
}

# --- Deploy --------------------------------------------------------------------------------------
if (-not (Test-Path (Join-Path $GameDir "RDR.exe"))) {
    throw "RDR.exe not found in '$GameDir'. Pass -GameDir."
}
if (-not (Test-Path (Join-Path $GameDir "RedHook.dll"))) {
    Write-Warning "RedHook.dll is not installed in '$GameDir' - the plugin will not load without it."
}

Copy-Item $plugin (Join-Path $GameDir "RDRBetterPresence.red") -Force
Write-Host "==> Deployed RDRBetterPresence.red to $GameDir" -ForegroundColor Green

# The .ini files hold user settings: copy them only when they do not exist yet.
foreach ($ini in @("RDRBetterPresence.ini", "RDRBetterPresence.regions.ini")) {
    $dest = Join-Path $GameDir $ini
    if (-not (Test-Path $dest)) {
        Copy-Item (Join-Path $root $ini) $dest
        Write-Host "==> Installed default $ini" -ForegroundColor Green
    } else {
        Write-Host "    Kept existing $ini"
    }
}
