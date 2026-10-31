# ==============================================================================
# SmileCodeIDE Release Packaging Script for Windows x64
# Version: v2.3.1
# ==============================================================================

param(
    [string]$Version = "v2.3.1",
    [string]$QtBinDir = "D:\Soft\Qt\6.11.2\mingw_64\bin",
    [string]$MingwBinDir = "D:\Soft\Qt\Tools\mingw1310_64\bin"
)

$ErrorActionPreference = "Stop"

Write-Host "==========================================================" -ForegroundColor Cyan
Write-Host " Packaging SmileCodeIDE $Version Portable for Windows x64" -ForegroundColor Cyan
Write-Host "==========================================================" -ForegroundColor Cyan

$distDir = "dist"
$targetDir = "$distDir/SmileCodeIDE-$Version-Windows-x64"
$zipPath = "$distDir/SmileCodeIDE-$Version-Windows-x64.zip"

# 1. Clean and prepare target directory
if (Test-Path $targetDir) {
    Write-Host "[1/6] Cleaning existing target directory..." -ForegroundColor Yellow
    Remove-Item $targetDir -Recurse -Force
}
New-Item -ItemType Directory -Path $targetDir -Force | Out-Null

# 2. Copy compiled binary and custom DLLs
Write-Host "[2/6] Copying main executable and custom dynamic libraries..." -ForegroundColor Green
Copy-Item "PhudonTools/release/PhudonTools.exe" "$targetDir/SmileCodeIDE.exe" -Force
Copy-Item "PhudonTools/release/qscintilla2_qt6.dll" "$targetDir/" -Force
Copy-Item "PhudonTools/release/ControlCAN.dll" "$targetDir/" -Force
Copy-Item "PhudonTools/release/zlgcan.dll" "$targetDir/" -Force

if (Test-Path "PhudonTools/release/plugins") {
    Write-Host "      Copying plugins..." -ForegroundColor Green
    Copy-Item "PhudonTools/release/plugins" "$targetDir/plugins" -Recurse -Force
}

# 3. Deploy Qt dependencies using windeployqt
Write-Host "[3/6] Running windeployqt to collect Qt runtime components..." -ForegroundColor Green
$windeployqt = Join-Path $QtBinDir "windeployqt.exe"
if (Test-Path $windeployqt) {
    & $windeployqt --release --compiler-runtime --no-translations "$targetDir/SmileCodeIDE.exe"
} else {
    Write-Warning "windeployqt not found at $windeployqt, skipping automatic Qt deployment."
}

# 4. Copy MinGW compiler runtime DLLs
Write-Host "[4/6] Copying MinGW runtime DLLs..." -ForegroundColor Green
$mingwDlls = @("libgcc_s_seh-1.dll", "libstdc++-6.dll", "libwinpthread-1.dll")
foreach ($dll in $mingwDlls) {
    $src = Join-Path $MingwBinDir $dll
    if (Test-Path $src) {
        Copy-Item $src "$targetDir/" -Force
    }
}

# 5. Copy documentation and metadata
Write-Host "[5/6] Copying release documentation and notes..." -ForegroundColor Green
Copy-Item "README.md" "$targetDir/" -Force -ErrorAction SilentlyContinue
Copy-Item "CHANGELOG.zh.md" "$targetDir/" -Force -ErrorAction SilentlyContinue
Copy-Item "RELEASE_NOTES_v2.3.1.md" "$targetDir/" -Force -ErrorAction SilentlyContinue

# 6. Compress package into portable zip
Write-Host "[6/6] Compressing to $zipPath..." -ForegroundColor Green
if (Test-Path $zipPath) {
    Remove-Item $zipPath -Force
}
Compress-Archive -Path "$targetDir/*" -DestinationPath $zipPath -Force

$zipItem = Get-Item $zipPath
$zipSizeMb = [math]::Round($zipItem.Length / 1MB, 2)

Write-Host "==========================================================" -ForegroundColor Cyan
Write-Host " Package generated successfully!" -ForegroundColor Green
Write-Host " Path: $zipPath" -ForegroundColor Green
Write-Host " Size: $zipSizeMb MB" -ForegroundColor Green
Write-Host "==========================================================" -ForegroundColor Cyan

