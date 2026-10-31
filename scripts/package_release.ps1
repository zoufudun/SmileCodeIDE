# ==============================================================================
# SmileCodeIDE All-in-One Packaging Script (Portable ZIP + EXE Setup + MSI Setup)
# Version: v2.3.1
# ==============================================================================

param(
    [string]$Version = "v2.3.1",
    [string]$QtBinDir = "D:\Soft\Qt\6.11.2\mingw_64\bin",
    [string]$MingwBinDir = "D:\Soft\Qt\Tools\mingw1310_64\bin",
    [string]$QtIfwBinDir = "D:\Soft\Qt\Tools\QtInstallerFramework\4.11\bin",
    [string]$WixBinDir = "tools\wix"
)

$ErrorActionPreference = "Stop"

Write-Host "==========================================================" -ForegroundColor Cyan
Write-Host " Building Full Packaging Suite for SmileCodeIDE $Version" -ForegroundColor Cyan
Write-Host " Target Platforms: Windows 10 / 11 64-bit" -ForegroundColor Cyan
Write-Host " Packages: 1. Portable ZIP  2. Setup EXE  3. Setup MSI" -ForegroundColor Cyan
Write-Host "==========================================================" -ForegroundColor Cyan

$distDir = "dist"
$targetDir = "$distDir/SmileCodeIDE-$Version-Windows-x64"
$zipPath = "$distDir/SmileCodeIDE-$Version-Windows-x64.zip"
$exePath = "$distDir/SmileCodeIDE-$Version-Setup.exe"
$msiPath = "$distDir/SmileCodeIDE-$Version-Setup.msi"

# ------------------------------------------------------------------------------
# STEP 1: Prepare Portable Distribution Directory
# ------------------------------------------------------------------------------
Write-Host "`n[STEP 1/5] Preparing clean portable distribution files..." -ForegroundColor Green
if (-not (Test-Path $targetDir)) {
    New-Item -ItemType Directory -Path $targetDir -Force | Out-Null
}

# Copy main binary and core dynamic libraries
Copy-Item "PhudonTools/release/PhudonTools.exe" "$targetDir/SmileCodeIDE.exe" -Force
Copy-Item "PhudonTools/release/qscintilla2_qt6.dll" "$targetDir/" -Force
Copy-Item "PhudonTools/release/ControlCAN.dll" "$targetDir/" -Force
Copy-Item "PhudonTools/release/zlgcan.dll" "$targetDir/" -Force

# Copy kerneldlls (CAN drivers, protocols, and hardware adapters)
if (Test-Path "PhudonTools/release/kerneldlls") {
    Write-Host "  -> Copying kerneldlls (hardware adapters and drivers)..." -ForegroundColor Gray
    Copy-Item "PhudonTools/release/kerneldlls" "$targetDir/kerneldlls" -Recurse -Force
}

# Copy plugins
if (Test-Path "PhudonTools/release/plugins") {
    Write-Host "  -> Copying plugins (STM32WorkbenchPlugin.dll)..." -ForegroundColor Gray
    Copy-Item "PhudonTools/release/plugins" "$targetDir/plugins" -Recurse -Force
}

# Deploy Qt dependencies via windeployqt
$windeployqt = Join-Path $QtBinDir "windeployqt.exe"
if (Test-Path $windeployqt) {
    Write-Host "  -> Running windeployqt to collect Qt runtime components..." -ForegroundColor Gray
    & $windeployqt --release --compiler-runtime --no-translations "$targetDir/SmileCodeIDE.exe" | Out-Null
}

# Copy MinGW compiler runtime DLLs
Write-Host "  -> Copying MinGW runtime libraries..." -ForegroundColor Gray
$mingwDlls = @("libgcc_s_seh-1.dll", "libstdc++-6.dll", "libwinpthread-1.dll")
foreach ($dll in $mingwDlls) {
    $src = Join-Path $MingwBinDir $dll
    if (Test-Path $src) {
        Copy-Item $src "$targetDir/" -Force
    }
}

# Copy release notes and documentation
Copy-Item "README.md" "$targetDir/" -Force -ErrorAction SilentlyContinue
Copy-Item "CHANGELOG.zh.md" "$targetDir/" -Force -ErrorAction SilentlyContinue
Copy-Item "RELEASE_NOTES_v2.3.1.md" "$targetDir/" -Force -ErrorAction SilentlyContinue

# ------------------------------------------------------------------------------
# STEP 2: Build Portable ZIP Package
# ------------------------------------------------------------------------------
Write-Host "`n[STEP 2/5] Creating Portable ZIP archive..." -ForegroundColor Green
if (Test-Path $zipPath) {
    Remove-Item $zipPath -Force
}
Compress-Archive -Path "$targetDir/*" -DestinationPath $zipPath -Force
$zipSize = [math]::Round((Get-Item $zipPath).Length / 1MB, 2)
Write-Host "  ✓ Portable ZIP successfully built: $zipPath ($zipSize MB)" -ForegroundColor Yellow

# ------------------------------------------------------------------------------
# STEP 3: Build Windows EXE Setup Wizard (via Qt Installer Framework)
# ------------------------------------------------------------------------------
Write-Host "`n[STEP 3/5] Building Windows EXE Setup Wizard (Qt Installer Framework)..." -ForegroundColor Green
$binarycreator = Join-Path $QtIfwBinDir "binarycreator.exe"
if (Test-Path $binarycreator) {
    # Synchronize files to packages/com.smilecode.ide/data
    $packageDataDir = "packages/com.smilecode.ide/data"
    if (Test-Path $packageDataDir) {
        Remove-Item $packageDataDir -Recurse -Force
    }
    New-Item -ItemType Directory -Path $packageDataDir -Force | Out-Null
    Copy-Item "$targetDir/*" "$packageDataDir/" -Recurse -Force

    if (Test-Path $exePath) {
        Remove-Item $exePath -Force
    }

    Write-Host "  -> Running binarycreator to bundle installer..." -ForegroundColor Gray
    & $binarycreator --offline-only -c "installer-config/config.xml" -p "packages" "$exePath"

    if (Test-Path $exePath) {
        $exeSize = [math]::Round((Get-Item $exePath).Length / 1MB, 2)
        Write-Host "  ✓ Setup EXE successfully built: $exePath ($exeSize MB)" -ForegroundColor Yellow
    } else {
        Write-Warning "Setup EXE generation failed."
    }
} else {
    Write-Warning "binarycreator.exe not found at $binarycreator"
}

# ------------------------------------------------------------------------------
# STEP 4: Build Windows MSI Package (via WiX Toolset)
# ------------------------------------------------------------------------------
Write-Host "`n[STEP 4/5] Building Windows MSI Installer (WiX Toolset)..." -ForegroundColor Green
$heat = Join-Path $WixBinDir "heat.exe"
$candle = Join-Path $WixBinDir "candle.exe"
$light = Join-Path $WixBinDir "light.exe"

if ((Test-Path $heat) -and (Test-Path $candle) -and (Test-Path $light)) {
    $wixBuildDir = "build_msi"
    if (Test-Path $wixBuildDir) {
        Remove-Item $wixBuildDir -Recurse -Force
    }
    New-Item -ItemType Directory -Path $wixBuildDir -Force | Out-Null

    $wixFilesWxs = "$wixBuildDir/app_files.wxs"
    Write-Host "  -> Harvesting application directory with heat.exe..." -ForegroundColor Gray
    & $heat dir "$targetDir" -cg AppFiles -gg -scom -sreg -sfrag -srd -dr INSTALLFOLDER -var var.SourceDir -out "$wixFilesWxs" | Out-Null

    Write-Host "  -> Compiling WiX XML with candle.exe..." -ForegroundColor Gray
    $sourceAbs = (Resolve-Path $targetDir).Path
    & $candle -dSourceDir="$sourceAbs" "installer-config/product.wxs" "$wixFilesWxs" -out "$wixBuildDir/" | Out-Null

    Write-Host "  -> Linking MSI database with light.exe..." -ForegroundColor Gray
    if (Test-Path $msiPath) {
        Remove-Item $msiPath -Force
    }
    & $light -ext WixUIExtension -b "$sourceAbs" "$wixBuildDir/product.wixobj" "$wixBuildDir/app_files.wixobj" -out "$msiPath" -sval | Out-Null

    if (Test-Path $msiPath) {
        $msiSize = [math]::Round((Get-Item $msiPath).Length / 1MB, 2)
        Write-Host "  ✓ Setup MSI successfully built: $msiPath ($msiSize MB)" -ForegroundColor Yellow
    } else {
        Write-Warning "Setup MSI generation failed."
    }

    # Clean intermediate WiX build directory
    Remove-Item $wixBuildDir -Recurse -Force -ErrorAction SilentlyContinue
} else {
    Write-Warning "WiX tools not found in $WixBinDir"
}

# ------------------------------------------------------------------------------
# STEP 5: Generate Checksums & Package Summary
# ------------------------------------------------------------------------------
Write-Host "`n[STEP 5/5] Generating SHA-256 and MD5 checksums..." -ForegroundColor Green
$checksumsFile = "$distDir/checksums.txt"
$reportLines = @(
    "# SmileCodeIDE $Version Release Artifacts Checksum Report",
    "# Generated on: $(Get-Date -Format 'yyyy-MM-dd HH:mm:ss')",
    ""
)

$artifacts = @($zipPath, $exePath, $msiPath)
foreach ($file in $artifacts) {
    if (Test-Path $file) {
        $item = Get-Item $file
        $sha256 = (Get-FileHash -Path $file -Algorithm SHA256).Hash
        $md5 = (Get-FileHash -Path $file -Algorithm MD5).Hash
        $sizeMb = [math]::Round($item.Length / 1MB, 2)
        $reportLines += "File: $($item.Name)"
        $reportLines += "Size: $sizeMb MB ($($item.Length) bytes)"
        $reportLines += "SHA256: $sha256"
        $reportLines += "MD5:    $md5"
        $reportLines += ""
    }
}

$reportLines | Out-File -FilePath $checksumsFile -Encoding utf8
Get-Content $checksumsFile | Write-Host -ForegroundColor Cyan

Write-Host "==========================================================" -ForegroundColor Green
Write-Host " All 3 Release Packages Generated Successfully!" -ForegroundColor Green
Write-Host "==========================================================" -ForegroundColor Green
