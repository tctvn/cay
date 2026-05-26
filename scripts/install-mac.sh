#!/bin/bash
# GitHub: https://github.com/tctvn/cay
# Len cau cai dat: wget -qO- https://raw.githubusercontent.com/tctvn/cay/main/scripts/install-mac.sh | bash

# CayIME - Trinh cai dat tu dong cho macOS
# Script nay tai ve cay-mac.zip tu GitHub, giai nen va cai dat vao thu muc /Applications.

set -e

# Mau sac cho output (khong dau)
GREEN='\032[0;32m'
CYAN='\032[0;36m'
YELLOW='\032[1;33m'
RED='\032[0;31m'
NC='\032[0m' # Khong mau

echo -e "${CYAN}================================================${NC}"
echo -e "${CYAN}   CayIME macOS Tu Dong Cai Dat                     ${NC}"
echo -e "${CYAN}================================================${NC}"

REPO="tctvn/cay"
ASSET_NAME="cay-mac.zip"
DOWNLOAD_URL="https://github.com/${REPO}/releases/latest/download/${ASSET_NAME}"
INSTALL_DIR="/Applications"
APP_NAME="cay.app"
APP_PATH="${INSTALL_DIR}/${APP_NAME}"

# Buoc 1: Tai ve file binary
echo -e "\n${YELLOW}[1/3] Tai ve cay-mac.zip moi nhat tu GitHub...${NC}"
WORK_DIR=$(mktemp -d)
cd "$WORK_DIR"

if ! curl -sL "$DOWNLOAD_URL" -o "$ASSET_NAME"; then
    echo -e "${RED}Tai xuong ${ASSET_NAME} that bai. Kiem tra co tap phien ban tren GitHub khong.${NC}"
    exit 1
fi

# Buoc 2: Giai nen va cai dat
echo -e "\n${YELLOW}[2/3] Giai nen va cai dat vao /Applications...${NC}"
unzip -q "$ASSET_NAME"

# Dong app neu dang chay
echo -e "\n${YELLOW}[2.5/3] Dong app dang chay (neu co)...${NC}"
killall cay 2>/dev/null || true
sleep 1
killall -9 cay 2>/dev/null || true

# Copy vao thu muc Applications
if [ -d "$APP_PATH" ]; then
    rm -rf "$APP_PATH"
fi
cp -R "$APP_NAME" "$INSTALL_DIR/"

# Cau hinh tu dong StartUp (LaunchAgent)
echo -e "\n${YELLOW}[3/3] Cau hinh tu dong LaunchAgent...${NC}"
BUNDLE_ID="com.tctvn.cay"
PLIST_DIR="$HOME/Library/LaunchAgents"
PLIST_PATH="$PLIST_DIR/${BUNDLE_ID}.plist"
EXEC_PATH="/Applications/cay.app/Contents/MacOS/cay"

mkdir -p "$PLIST_DIR"
cat > "$PLIST_PATH" <<EOF
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>Label</key>
    <string>$BUNDLE_ID</string>
    <key>ProgramArguments</key>
    <array>
        <string>$EXEC_PATH</string>
    </array>
    <key>RunAtLoad</key>
    <true/>
</dict>
</plist>
EOF

# Xoa quarantine de chay duoc
echo -e "\n${YELLOW}[4/4] Xoa quarantine flag va chay app...${NC}"
xattr -cr "$APP_PATH"
cd ~
rm -rf "$WORK_DIR"

open "$APP_PATH"

echo -e "\n${GREEN}================================================${NC}"
echo -e "${GREEN}   Cai dat thanh cong!                               ${NC}"
echo -e "${GREEN}================================================${NC}"
echo -e "${CYAN}CayIME da duoc cai dat vao Applications va dang chay.${NC}"
echo -e "${CYAN}Ban co the can cap quyen Accessibility trong System Settings lan dau su dung.${NC}"
