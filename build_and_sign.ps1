# ETDTimer PowerShell Build & Code Signing Script
$ErrorActionPreference = "Stop"

# Ensure MinGW bin is at head of PATH
$mingwBin = "C:\Users\vboxuser\Desktop\mingw64\bin"
if (Test-Path $mingwBin) {
    $env:Path = "$mingwBin;$env:Path"
}

$Compiler = "g++.exe"
$Windres  = "windres.exe"

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "   ETDTimer Native C++ Build System    " -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan

Set-Location "c:\Users\vboxuser\Desktop\ETDTimer"

# 1. Compile App Resource File (Icon + Manifest)
Write-Host "[1/5] Compiling Application Resources..." -ForegroundColor Yellow
"101 ICON ""res/app.ico""" | Out-File -FilePath "res/app_res.rc" -Encoding ascii
"1 24 ""res/app.manifest""" | Out-File -FilePath "res/app_res.rc" -Append -Encoding ascii
& $Windres "res/app_res.rc" -O coff -o "res/app_res.o"

# 2. Compile ETDTimer Application
Write-Host "[2/5] Compiling ETDTimer.exe..." -ForegroundColor Yellow
$appSources = @(
    "src/main.cpp",
    "src/app_window.cpp",
    "src/tools.cpp",
    "src/ui_renderer.cpp",
    "src/settings.cpp",
    "src/audio.cpp",
    "res/app_res.o"
)
& $Compiler -O3 -mwindows -municode -std=c++20 $appSources -lgdiplus -lgdi32 -luser32 -lwinmm -lole32 -lshlwapi -lcomctl32 -static -s -o "ETDTimer.exe"

if (Test-Path "ETDTimer.exe") {
    $appSize = (Get-Item "ETDTimer.exe").Length / 1KB
    Write-Host "   -> Success! ETDTimer.exe created ($([math]::Round($appSize, 2)) KB)" -ForegroundColor Green
} else {
    Write-Error "Failed to build ETDTimer.exe"
}

# 3. Compile Setup Resource File (Icon + Manifest + Embedded ETDTimer.exe Payload)
Write-Host "[3/5] Packing ETDTimer.exe into Setup Resources..." -ForegroundColor Yellow
"101 ICON ""res/app.ico""" | Out-File -FilePath "res/setup_res.rc" -Encoding ascii
"1 24 ""res/app.manifest""" | Out-File -FilePath "res/setup_res.rc" -Append -Encoding ascii
"102 256 ""ETDTimer.exe""" | Out-File -FilePath "res/setup_res.rc" -Append -Encoding ascii
& $Windres "res/setup_res.rc" -O coff -o "res/setup_res.o"

# 4. Compile Installer Wizard
Write-Host "[4/5] Compiling ETDTimerSetup.exe..." -ForegroundColor Yellow
$setupSources = @(
    "installer/installer_main.cpp",
    "res/setup_res.o"
)
& $Compiler -O3 -mwindows -municode -std=c++20 $setupSources -luser32 -lgdi32 -lole32 -lshell32 -lshlwapi -luuid -static -s -o "ETDTimerSetup.exe"

if (Test-Path "ETDTimerSetup.exe") {
    $setupSize = (Get-Item "ETDTimerSetup.exe").Length / 1KB
    Write-Host "   -> Success! ETDTimerSetup.exe created ($([math]::Round($setupSize, 2)) KB)" -ForegroundColor Green
} else {
    Write-Error "Failed to build ETDTimerSetup.exe"
}

# 5. Code Signing Executables
Write-Host "[5/5] Signing Executables with Self-Signed Certificate..." -ForegroundColor Yellow
$certSubject = "CN=Emir Tugra Dag Code Signing, O=ETDTimer"
$cert = Get-ChildItem -Path Cert:\CurrentUser\My -CodeSigningCert | Where-Object { $_.Subject -match "Emir Tugra Dag" } | Select-Object -First 1

if (-not $cert) {
    Write-Host "   Generating new self-signed Code Signing Certificate..." -ForegroundColor Gray
    $cert = New-SelfSignedCertificate -Type CodeSigningCert -Subject $certSubject -CertStoreLocation Cert:\CurrentUser\My -NotAfter (Get-Date).AddYears(5)
}

if ($cert) {
    Set-AuthenticodeSignature -FilePath "ETDTimer.exe" -Certificate $cert | Out-Null
    Set-AuthenticodeSignature -FilePath "ETDTimerSetup.exe" -Certificate $cert | Out-Null
    Write-Host "   -> Successfully digitally signed ETDTimer.exe and ETDTimerSetup.exe!" -ForegroundColor Green
} else {
    Write-Host "   Warning: Could not sign executables. Certificate creation skipped." -ForegroundColor Red
}

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "        Build Completed Successfully!   " -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
