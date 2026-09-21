<#
.SYNOPSIS
    Installs RDRBetterPresence (and RedHook if missing) into your Red Dead Redemption folder.

.DESCRIPTION
    Run this from the extracted release folder (the one containing RDRBetterPresence.red):

        powershell -ExecutionPolicy Bypass -File .\install.ps1

    The script:
      1. finds the game folder (Rockstar Games Launcher / Steam), or takes -GameDir;
      2. downloads RedHook v0.8 from its official GitHub release if RedHook.dll is not there yet;
      3. copies RDRBetterPresence.red and its .ini files (existing .ini files are kept);
      4. reminds you about the Visual C++ x64 redistributable RedHook needs.

.PARAMETER GameDir
    Folder containing RDR.exe, if it is not detected automatically.
#>
param(
    [string]$GameDir
)

$ErrorActionPreference = "Stop"
$here = $PSScriptRoot
$redHookZip = "https://github.com/K3rhos/RedHookSDK/releases/download/v0.8/RedHook.v0.8.zip"

function Find-GameDir {
    $candidates = @()
    foreach ($drive in (Get-PSDrive -PSProvider FileSystem | Select-Object -ExpandProperty Root)) {
        $candidates += Join-Path $drive "Program Files\Rockstar Games\Red Dead Redemption"
        $candidates += Join-Path $drive "Program Files (x86)\Steam\steamapps\common\Red Dead Redemption"
        $candidates += Join-Path $drive "SteamLibrary\steamapps\common\Red Dead Redemption"
        $candidates += Join-Path $drive "Steam\steamapps\common\Red Dead Redemption"
        $candidates += Join-Path $drive "Games\Red Dead Redemption"
    }
    # Steam library folders declared in libraryfolders.vdf
    $vdf = "${env:ProgramFiles(x86)}\Steam\steamapps\libraryfolders.vdf"
    if (Test-Path $vdf) {
        foreach ($m in [regex]::Matches((Get-Content $vdf -Raw), '"path"\s+"([^"]+)"')) {
            $candidates += Join-Path ($m.Groups[1].Value -replace '\\\\', '\') "steamapps\common\Red Dead Redemption"
        }
    }
    foreach ($c in $candidates) {
        if (Test-Path (Join-Path $c "RDR.exe")) { return $c }
    }
    return $null
}

# --- 1. Game folder ---------------------------------------------------------------------------
if (-not $GameDir) { $GameDir = Find-GameDir }
if (-not $GameDir -or -not (Test-Path (Join-Path $GameDir "RDR.exe"))) {
    Write-Host "Could not find RDR.exe. Run again with:  .\install.ps1 -GameDir `"D:\Path\To\Red Dead Redemption`"" -ForegroundColor Red
    exit 1
}
Write-Host "Game folder: $GameDir" -ForegroundColor Cyan

if (Get-Process -Name RDR -ErrorAction SilentlyContinue) {
    Write-Host "Red Dead Redemption is running - close it first." -ForegroundColor Red
    exit 1
}

# --- 2. RedHook ---------------------------------------------------------------------------------
if (Test-Path (Join-Path $GameDir "RedHook.dll")) {
    Write-Host "RedHook already installed." -ForegroundColor Green
} else {
    Write-Host "Downloading RedHook v0.8 (K3rhos, GitHub release)..." -ForegroundColor Cyan
    $tmp = Join-Path $env:TEMP "RedHook.v0.8.zip"
    Invoke-WebRequest -Uri $redHookZip -OutFile $tmp -UseBasicParsing
    $extract = Join-Path $env:TEMP "RedHook.v0.8"
    if (Test-Path $extract) { Remove-Item $extract -Recurse -Force }
    Expand-Archive -Path $tmp -DestinationPath $extract
    foreach ($f in @("winmm.dll", "RedHook.dll", "RedHook.ini")) {
        Copy-Item (Join-Path $extract $f) (Join-Path $GameDir $f) -Force
    }
    Write-Host "RedHook installed (winmm.dll, RedHook.dll, RedHook.ini)." -ForegroundColor Green
}

# --- 3. Plugin ----------------------------------------------------------------------------------
$plugin = Join-Path $here "RDRBetterPresence.red"
if (-not (Test-Path $plugin)) {
    Write-Host "RDRBetterPresence.red not found next to this script. Extract the whole release zip first." -ForegroundColor Red
    exit 1
}
Copy-Item $plugin (Join-Path $GameDir "RDRBetterPresence.red") -Force
Write-Host "Installed RDRBetterPresence.red" -ForegroundColor Green

foreach ($ini in @("RDRBetterPresence.ini", "RDRBetterPresence.regions.ini")) {
    $dest = Join-Path $GameDir $ini
    if (Test-Path $dest) {
        Write-Host "Kept existing $ini"
    } else {
        Copy-Item (Join-Path $here $ini) $dest
        Write-Host "Installed $ini" -ForegroundColor Green
    }
}

# --- 4. VC++ redistributable ------------------------------------------------------------------
$vc = Get-ItemProperty "HKLM:\SOFTWARE\Microsoft\VisualStudio\14.0\VC\Runtimes\x64" -ErrorAction SilentlyContinue
if ($vc -and $vc.Installed -eq 1) {
    Write-Host "Visual C++ x64 redistributable present ($($vc.Major).$($vc.Minor))." -ForegroundColor Green
} else {
    Write-Host "RedHook needs the Visual C++ x64 redistributable: https://aka.ms/vs/17/release/vc_redist.x64.exe" -ForegroundColor Yellow
}

Write-Host ""
Write-Host "Done. Start Discord, then the game. The presence appears on your Discord profile." -ForegroundColor Cyan
Write-Host "Log file: $(Join-Path $GameDir 'RDRBetterPresence.log')   (F8 in game opens the RedHook console)"
