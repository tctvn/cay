#!/bin/bash
# GitHub: https://github.com/tctvn/cay
# Len cau cai dat: wget -qO- https://raw.githubusercontent.com/tctvn/cay/main/scripts/uninstall-fcitx5.sh | bash

# CayIME - Fcitx5 Giai nen Script
# This script fully removes CayIME from Fcitx5 (both system and local directories)

set -e

# Mau sac cho output
GREEN='\033[0;32m'
CYAN='\033[0;36m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m' # No Color

echo -e "${CYAN}================================================${NC}"
echo -e "${CYAN}        CayIME Fcitx5 Giai nen               ${NC}"
echo -e "${CYAN}================================================${NC}"

# Stop fcitx5 to avoid file lock
echo -e "\n${YELLOW}[1/4] Dang dung tien trinh Fcitx5...${NC}"
killall fcitx5 2>/dev/null || true
sleep 1
killall -9 fcitx5 2>/dev/null || true
sleep 1

# Xoa file tren he thong (/usr)
echo -e "\n${YELLOW}[2/4] Xoa cac file plugin he thong (can quyen sudo)...${NC}"
echo "Vui long nhap mat khau neu duoc yeu cau:"

sudo rm -f /usr/lib/fcitx5/cayime-fcitx5.so
sudo rm -f /usr/lib/x86_64-linux-gnu/fcitx5/cayime-fcitx5.so
sudo rm -f /usr/share/fcitx5/addon/cayime.conf
sudo rm -f /usr/share/fcitx5/inputmethod/cayime-im.conf
sudo rm -f /usr/share/fcitx5/inputmethod/cayime.conf
sudo rm -f /usr/share/icons/hicolor/scalable/apps/cayime.svg

# Xoa file local cua user (neu co)
echo -e "\n${YELLOW}[3/4] Don dep cache va file local...${NC}"
rm -f ~/.local/lib/fcitx5/cayime-fcitx5.so
rm -f ~/.local/lib/x86_64-linux-gnu/fcitx5/cayime-fcitx5.so
rm -f ~/.local/share/fcitx5/addon/cayime.conf
rm -f ~/.local/share/fcitx5/inputmethod/cayime-im.conf
rm -f ~/.local/share/fcitx5/inputmethod/cayime.conf
rm -rf ~/.config/fcitx5/cayime 2>/dev/null || true

# Khoi phuc DefaultIM trong Fcitx5 profile ve tieng Anh neu dang set cayime
PROFILE="${HOME}/.config/fcitx5/profile"
if [ -f "$PROFILE" ]; then
    sed -i 's/^DefaultIM=cayime/DefaultIM=keyboard-us/' "$PROFILE"
fi

# Xoa cache de Fcitx5 quet lai danh sach Addon
rm -rf ~/.cache/fcitx5
sudo gtk-update-icon-cache -f -t /usr/share/icons/hicolor 2>/dev/null || true

# Khoi dong lai Fcitx5
echo -e "\n${YELLOW}[4/4] Khoi dong lai Fcitx5...${NC}"
nohup fcitx5 -d -r > /dev/null 2>&1 &
disown

echo -e "\n${GREEN}================================================${NC}"
echo -e "${GREEN}   Giai cau dat thanh cong! CayIME da duoc xoa. ${NC}"
echo -e "${GREEN}================================================${NC}"
