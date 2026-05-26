#!/bin/bash
# GitHub: https://github.com/tctvn/cay
# Len cau cai dat: wget -qO- https://raw.githubusercontent.com/tctvn/cay/main/scripts/uninstall-mac.sh | bash

# CayIME - Giai nen tu dong cho macOS
# Script nay giai toan bo CayIME khoi Fcitx5 tren macOS (he thong va local)

set -e

# Mau sac cho output (khong dau)
GREEN='\032[0;32m'
CYAN='\032[0;36m'
YELLOW='\032[1;33m'
RED='\032[0;31m'
NC='\032[0m' # Khong mau

echo -e "${CYAN}================================================${NC}"
echo -e "${CYAN}   CayIME macOS Giai nen Tu Dong                     ${NC}"
echo -e "${CYAN}================================================${NC}"

# Buoc 1: Dong app neu dang chay
echo -e "\n${YELLOW}[1/4] Dong app CayIME (neu co)...${NC}"
killall cay 2>/dev/null || true
sleep 1
killall -9 cay 2>/dev/null || true

# Buoc 2: Xoa LaunchAgent neu co
echo -e "\n${YELLOW}[2/4] Xoa LaunchAgent...${NC}"
BUNDLE_ID="com.tctvn.cay"
PLIST_PATH="$HOME/Library/LaunchAgents/${BUNDLE_ID}.plist"
if [ -f "$PLIST_PATH" ]; then
    launchctl unload "$PLIST_PATH" 2>/dev/null || true
    rm -f "$PLIST_PATH"
fi

# Buoc 3: Xoa thu muc Applications
echo -e "\n${YELLOW}[3/4] Xoa thu muc Applications...${NC}"
INSTALL_DIR="/Applications"
APP_NAME="cay.app"
APP_PATH="${INSTALL_DIR}/${APP_NAME}"
if [ -d "$APP_PATH" ]; then
    rm -rf "$APP_PATH"
fi

# Buoc 4: Xoa preferences (cac file cau hinh)
echo -e "\n${YELLOW}[4/4] Xoa preferences...${NC}"
PREF_PATH="$HOME/Library/Preferences/com.tctvn.cay.plist"
if [ -f "$PREF_PATH" ]; then
    rm -f "$PREF_PATH"
fi

echo -e "\n${GREEN}================================================${NC}"
echo -e "${GREEN}   Giai nen thanh cong!                                 ${NC}"
echo -e "${GREEN}================================================${NC}"
echo -e "${CYAN}CayIME da duoc giai toan bo tu Mac cua ban.${NC}\n"
