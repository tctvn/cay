#!/usr/bin/env pwsh
# GitHub: https://github.com/tctvn/cay
# Len cau cai dat: irm https://raw.githubusercontent.com/tctvn/cay/main/scripts/install-windows.ps1 | iex

# CayIME - Trinh cai dat tu dong cho Windows
# Script nay tai ve cay.exe tu GitHub, cai dat vao $env:LOCALAPPDATA\CayIME va cau hinh start up.

$ErrorActionPreference = "Stop"

Write-Host "================================================" -ForegroundColor Cyan
Write-Host "   CayIME Windows Tu Dong Cai Dat           " -ForegroundColor Cyan
Write-Host "================================================" -ForegroundColor Cyan

$repo = "tctvn/cay"
$assetName = "cay.exe"
$downloadUrl = "https://github.com/$repo/releases/latest/download/$assetName"
$installDir = "$env:LOCALAPPDATA\CayIME"
$exePath = "$installDir\cay.exe"

Write-Host "`n[1/3] Tai ve CayIME moi nhat tu GitHub..." -ForegroundColor Yellow
# Dong tien trinh dang chay neu con lock
Get-Process cay -ErrorAction SilentlyContinue | Stop-Process -Force
Get-Process cayy -ErrorAction SilentlyContinue | Stop-Process -Force
Start-Sleep -Seconds 1

if (!(Test-Path -Path $installDir)) {
    New-Item -ItemType Directory -Force -Path $installDir | Out-Null
}

try {
    Invoke-WebRequest -Uri $downloadUrl -OutFile $exePath
} catch {
    Write-Host "Tai xuong $assetName that bai tu $downloadUrl." -ForegroundColor Red
    Write-Host "Kiem tra xem phien ban moi nhat co san tren GitHub khong." -ForegroundColor Red
    exit 1
}

Write-Host "`n[2/3] Cau hinh start up tu dong (Registry)..." -ForegroundColor Yellow
# Dang ky Cay start up khi dang nhap Windows
$runKey = "HKCU:\SOFTWARE\Microsoft\Windows\CurrentVersion\Run"
Set-ItemProperty -Path $runKey -Name "Cay" -Value $exePath

# Dat FirstLaunch = 1 de tra trinh khong hien popup
$cayConfigKey = "HKCU:\SOFTWARE\CayIME"
if (!(Test-Path -Path $cayConfigKey)) { New-Item -Path $cayConfigKey -Force | Out-Null }
Set-ItemProperty -Path $cayConfigKey -Name "FirstLaunch" -Value 1 -Type DWord

Write-Host "`n[3/3] Khoi dong CayIME..." -ForegroundColor Yellow
Start-Process -FilePath $exePath

Write-Host "`n================================================" -ForegroundColor Green
Write-Host "   Cai dat Thanh Cong!                     " -ForegroundColor Green
Write-Host "================================================" -ForegroundColor Green
Write-Host "`nCayIME da duoc cai dat vao $installDir" 
Write-Host "Noi dung se duoc chay tu dong khi dang nhap Windows." 
Write-Host "Ban co the bat dau go tieng Viet ngay lap tuc!"
