#!/usr/bin/env pwsh
# GitHub: https://github.com/tctvn/cay
# Len cau cai dat: irm https://raw.githubusercontent.com/tctvn/cay/cayy/scripts/install-windows-full.ps1 | iex

# CayIME (Ban Full) - Trinh cai dat tu dong cho Windows
# Script nay tai ve cayy.exe tu GitHub, cai dat vao $env:LOCALAPPDATA\CayIME va cau hinh start up.
#
# ==============================================================================
# LUU Y QUAN TRONG KHI UPDATE SCRIPT NAY:
# 1. Script nay se goi uninstall-windows.ps1 tren GitHub de don dep ban cu.
# 2. Neu ban them lenh tao file/shortcut/registry moi o day, BAT BUOC phai 
#    cap nhat script uninstall-windows.ps1 de xoa nhung file/registry do.
# 3. Luu y co che sao luu/khoi phuc data macro (cayy.dat) phai duoc giu nguyen 
#    xuyen suot qua trinh goi uninstall-windows.ps1.
# ==============================================================================

$ErrorActionPreference = "Stop"

trap {
    Write-Host "`nXay ra loi khong mong muon: $($_.Exception.Message)" -ForegroundColor Red
    Write-Host "Vui long thu lai hoac bao loi tren GitHub." -ForegroundColor Yellow
    Write-Host "Nhan Enter de thoat..." -ForegroundColor Yellow
    Read-Host
    exit 1
}


Write-Host "================================================" -ForegroundColor Cyan
Write-Host "   CayIME (Full) Windows Tu Dong Cai Dat           " -ForegroundColor Cyan
Write-Host "================================================" -ForegroundColor Cyan

$repo = "tctvn/cay"
$assetName = "cayy.exe"
$downloadUrl = "https://github.com/$repo/releases/latest/download/$assetName"
$installDir = "$env:LOCALAPPDATA\CayIME"
$exePath = "$installDir\cayy.exe"

Write-Host "`n[0/3] Don dep phien ban cu (neu co)..." -ForegroundColor Yellow

# Sao luu macro truoc khi uninstall
$macroBackupPath = "$env:TEMP\cayy_macro_backup.dat"
if (Test-Path "$installDir\cayy.dat") {
    Copy-Item "$installDir\cayy.dat" -Destination $macroBackupPath -Force
}

$env:CAY_KEEP_SETTINGS = "1"
irm https://raw.githubusercontent.com/tctvn/cay/main/scripts/uninstall-windows.ps1 | iex
Remove-Item Env:\CAY_KEEP_SETTINGS -ErrorAction SilentlyContinue

Write-Host "`n[1/3] Tai ve CayIME (Full) moi nhat tu GitHub..." -ForegroundColor Yellow

if (!(Test-Path -Path $installDir)) {
    New-Item -ItemType Directory -Force -Path $installDir | Out-Null
}

try {
    Invoke-WebRequest -Uri $downloadUrl -OutFile $exePath
} catch {
    Write-Host "Tai xuong $assetName that bai tu $downloadUrl." -ForegroundColor Red
    Write-Host "Kiem tra xem phien ban moi nhat co san tren GitHub khong." -ForegroundColor Red
    Write-Host ""
    Write-Host "Nhan Enter de thoat..." -ForegroundColor Yellow
    Read-Host
    throw "Loi trong qua trinh cai dat!"
}

Write-Host "`n[2/3] Cau hinh start up tu dong (Registry)..." -ForegroundColor Yellow
# Dang ky Cay start up khi dang nhap Windows
$runKey = "HKCU:\SOFTWARE\Microsoft\Windows\CurrentVersion\Run"
Set-ItemProperty -Path $runKey -Name "Cay" -Value $exePath

# Dat FirstLaunch = 1 de tra trinh khong hien popup
$cayConfigKey = "HKCU:\SOFTWARE\CayIME"
if (!(Test-Path -Path $cayConfigKey)) { New-Item -Path $cayConfigKey -Force | Out-Null }
Set-ItemProperty -Path $cayConfigKey -Name "FirstLaunch" -Value 1 -Type DWord

# Khoi phuc macro
if (Test-Path $macroBackupPath) {
    Copy-Item $macroBackupPath -Destination "$installDir\cayy.dat" -Force
    Remove-Item $macroBackupPath -Force
}

# Tao Desktop shortcut
$desktopPath = [Environment]::GetFolderPath("Desktop")
$shortcutPath = "$desktopPath\CayIME (Full).lnk"
$WshShell = New-Object -ComObject WScript.Shell
$Shortcut = $WshShell.CreateShortcut($shortcutPath)
$Shortcut.TargetPath = $exePath
$Shortcut.Save()

Write-Host "`n[3/3] Khoi dong CayIME (Full)..." -ForegroundColor Yellow
Start-Process -FilePath $exePath

Write-Host "`n================================================" -ForegroundColor Green
Write-Host "   Cai dat Thanh Cong!                     " -ForegroundColor Green
Write-Host "================================================" -ForegroundColor Green
Write-Host "`nCayIME (Full) da duoc cai dat vao $installDir" 
Write-Host "Noi dung se duoc chay tu dong khi dang nhap Windows." 
Write-Host "Ban co the bat dau go tieng Viet ngay lap tuc!"
