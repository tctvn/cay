#!/usr/bin/env pwsh
# GitHub: https://github.com/tctvn/cay
# Len cau cai dat: irm https://raw.githubusercontent.com/tctvn/cay/main/scripts/uninstall-windows.ps1 | iex

# CayIME - Giai nen tu dong cho Windows
# Script nay giai toan bo CayIME khoi LocalAppData va Registry.

$ErrorActionPreference = "Stop"

Write-Host "================================================" -ForegroundColor Cyan
Write-Host "   CayIME Windows Tu Dong Giai Nen           " -ForegroundColor Cyan
Write-Host "================================================" -ForegroundColor Cyan

$installDir = "$env:LOCALAPPDATA\CayIME"

# Buoc 1: Dong cau trinh neu dang chay
Write-Host "`n[1/4] Dong CayIME (neu co)..." -ForegroundColor Yellow
Get-Process cay -ErrorAction SilentlyContinue | Stop-Process -Force
Get-Process cayy -ErrorAction SilentlyContinue | Stop-Process -Force
Start-Sleep -Seconds 1

# Buoc 2: Xoa khoi dang ky start up (Registry)
Write-Host "`n[2/4] Xoa khoi dang ky start up trong Registry..." -ForegroundColor Yellow
$runKey = "HKCU:\SOFTWARE\Microsoft\Windows\CurrentVersion\Run"
Remove-ItemProperty -Path $runKey -Name "Cay" -ErrorAction SilentlyContinue

# Buoc 3: Xoa thu muc cai dat
Write-Host "`n[3/4] Xoa thu muc cai dat..." -ForegroundColor Yellow
if (Test-Path -Path $installDir) {
    Remove-Item -Path $installDir -Recurse -Force -ErrorAction SilentlyContinue
}

# Buoc 4: Xoa cau hinh (Registry)
Write-Host "`n[4/4] Xoa cau hinh trong Registry..." -ForegroundColor Yellow
$cayConfigKey = "HKCU:\SOFTWARE\CayIME"
if (Test-Path -Path $cayConfigKey) {
    Remove-Item -Path $cayConfigKey -Recurse -Force -ErrorAction SilentlyContinue
}

Write-Host "`n================================================" -ForegroundColor Green
Write-Host "   Giai nen Thanh Cong!                     " -ForegroundColor Green
Write-Host "================================================" -ForegroundColor Green
Write-Host "`nCayIME da duoc giai toan bo tu Windows PC cua ban." 
