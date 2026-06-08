#!/usr/bin/env pwsh
# GitHub: https://github.com/tctvn/cay
# Len cau cai dat: irm https://raw.githubusercontent.com/tctvn/cay/main/scripts/uninstall-windows.ps1 | iex

# CayIME - Giai nen tu dong cho Windows
# Script nay giai toan bo CayIME khoi LocalAppData va Registry.
#
# ==============================================================================
# LUU Y QUAN TRONG KHI UPDATE SCRIPT NAY:
# 1. Script nay duoc goi tu dong boi tat ca cac script install-windows*.ps1 truoc khi cai.
# 2. Neu ban tao them file/thu muc/shortcut/registry moi o cac script install, 
#    BAT BUOC phai them lenh xoa tuong ung vao day de don dep sach se.
# 3. Script dang xu ly go bo cho ca 4 phien ban: Minimal, ARM, Full, Classic.
#    Cac ten tien trinh, ten startup (Cay, CayClassic), hay duong dan registry phai dong bo.
# ==============================================================================


$ErrorActionPreference = "Stop"

Write-Host "================================================" -ForegroundColor Cyan
Write-Host "   CayIME Windows Tu Dong Giai Nen           " -ForegroundColor Cyan
Write-Host "================================================" -ForegroundColor Cyan

$installDir = "$env:LOCALAPPDATA\CayIME"

# Buoc 1: Dong cau trinh neu dang chay
Write-Host "`n[1/5] Dong CayIME (neu co)..." -ForegroundColor Yellow
Get-Process cay -ErrorAction SilentlyContinue | Stop-Process -Force
Get-Process cayy -ErrorAction SilentlyContinue | Stop-Process -Force
Get-Process cay-arm -ErrorAction SilentlyContinue | Stop-Process -Force
Get-Process cay-classic -ErrorAction SilentlyContinue | Stop-Process -Force
Start-Sleep -Seconds 1

# Buoc 2: Xoa khoi dang ky start up (Registry)
Write-Host "`n[2/5] Xoa khoi dang ky start up trong Registry..." -ForegroundColor Yellow
$runKey = "HKCU:\SOFTWARE\Microsoft\Windows\CurrentVersion\Run"
Remove-ItemProperty -Path $runKey -Name "Cay" -ErrorAction SilentlyContinue
Remove-ItemProperty -Path $runKey -Name "CayClassic" -ErrorAction SilentlyContinue

# Buoc 3: Xoa thu muc cai dat
Write-Host "`n[3/5] Xoa thu muc cai dat..." -ForegroundColor Yellow
if (Test-Path -Path $installDir) {
    Remove-Item -Path $installDir -Recurse -Force -ErrorAction SilentlyContinue
}

# Buoc 4: Xoa cau hinh (Registry)
Write-Host "`n[4/5] Xoa cau hinh trong Registry..." -ForegroundColor Yellow
$cayConfigKey = "HKCU:\SOFTWARE\CayIME"
$cayClassicConfigKey = "HKCU:\Software\tctvn\CayClassic"
if ($env:CAY_KEEP_SETTINGS -ne "1") {
    if (Test-Path -Path $cayConfigKey) {
        Remove-Item -Path $cayConfigKey -Recurse -Force -ErrorAction SilentlyContinue
    }
    if (Test-Path -Path $cayClassicConfigKey) {
        Remove-Item -Path $cayClassicConfigKey -Recurse -Force -ErrorAction SilentlyContinue
    }
} else {
    Write-Host "  -> Bo qua (Giu lai cau hinh nguoi dung do CAY_KEEP_SETTINGS duoc thiet lap)." -ForegroundColor Gray
}

# Buoc 5: Xoa Desktop shortcuts
Write-Host "`n[5/5] Xoa Desktop shortcuts..." -ForegroundColor Yellow
$desktopPath = [Environment]::GetFolderPath("Desktop")
$shortcuts = @("CayIME.lnk", "CayIME (Full).lnk", "Cay Classic.lnk")
foreach ($sc in $shortcuts) {
    if (Test-Path "$desktopPath\$sc") {
        Remove-Item "$desktopPath\$sc" -Force -ErrorAction SilentlyContinue
    }
}

Write-Host "`n================================================" -ForegroundColor Green
Write-Host "   Giai nen Thanh Cong!                     " -ForegroundColor Green
Write-Host "================================================" -ForegroundColor Green
Write-Host "`nCayIME da duoc giai toan bo tu Windows PC cua ban." 
