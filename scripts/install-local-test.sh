#!/bin/bash
# GitHub: https://github.com/tctvn/cay
# Len cau cai dat: wget -qO- https://raw.githubusercontent.com/tctvn/cay/main/scripts/install-local-test.sh | bash

# CayIME - Trinh cai dat Fcitx5 local cho kiem thu
# Script nay xay dung plugin Fcitx5 tu mau nguon va cai dat bang sudo.

set -e

# Mau sac cho output
GREEN='\033[0;32m'
CYAN='\033[0;36m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0;m' # Khong mau

echo -e "${CYAN}================================================${NC}"
echo -e "${CYAN}   CayIME Fcitx5 Plugin Local Test Installer    ${NC}"
echo -e "${CYAN}================================================${NC}"

# Di den thu muc goc du an
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" >/dev/null 2>&1 && pwd)"
cd "$SCRIPT_DIR/.."

# Kiem tra co thu muc goc du an
if [ ! -f "CMakeLists.txt" ] || [ ! -d "src/platform/fcitx5" ]; then
    echo -e "${RED}Loi: Khong tim thay thu muc goc du an (thieu CMakeLists.txt).${NC}"
    exit 1
fi

# Buoc 1: Dung Fcitx5 de giai file lock
echo -e "\n${YELLOW}[1/3] Dung Fcitx5...${NC}"
killall fcitx5 2>/dev/null || true
sleep 1
killall -9 fcitx5 2>/dev/null || true
sleep 1

# Buoc 2: Cai dat vao he thong bang sudo
echo -e "\n${YELLOW}[2/3] Cai dat plugin vao he thong (/usr)...${NC}"

# Tim thu muc build
BUILD_DIR=""
if [ -d "build_local" ] && [ -f "build_local/Makefile" ]; then BUILD_DIR="build_local"; fi
if [ -d "build_system" ] && [ -f "build_system/Makefile" ]; then BUILD_DIR="build_system"; fi
if [ -d "build" ] && [ -f "build/Makefile" ]; then BUILD_DIR="build"; fi

if [ -z "$BUILD_DIR" ]; then
    echo -e "${RED}Loi: Khong tim thay thu muc build (build_local, build_system, hoac build). Vui long build du an truoc!${NC}"
    exit 1
fi

echo "Can ban su mat khau (sudo) de cai dat file vao /usr/lib va /usr/share."
sudo cmake --install "$BUILD_DIR"

# Neu Fcitx5 tren Ubuntu/Debian yeu cau ki thuat lap trinh, sao chep den x86_64-linux-gnu neu co
if [ -d "/usr/lib/x86_64-linux-gnu/fcitx5" ]; then
    sudo cp -f "$BUILD_DIR/src/platform/fcitx5/cayime-fcitx5.so" /usr/lib/x86_64-linux-gnu/fcitx5/ 2>/dev/null || true
fi

# Buoc 3: Tu dong cau hinh profile Fcitx5 (Zero-Click Magic)
echo -e "\n${YELLOW}[3/3] Cau hinh tu dong Fcitx5...${NC}"

PROFILE="$HOME/.config/fcitx5/profile"
mkdir -p "$HOME/.config/fcitx5"

if [ ! -f "$PROFILE" ]; then
cat <<EOF > "$PROFILE"
[Groups/0]
Name=Default
Default Layout=us
DefaultIM=cayime

[Groups/0/Items/0]
Name=cayime
Layout=

[Groups/0/Items/1]
Name=keyboard-us
Layout=

[GroupOrder]
0=Default
EOF
else
    if ! grep -qi "Name=cayime" "$PROFILE"; then
        echo -e "Injecting CayIME vao profile Fcitx5 hien tai..."
        MAX_ITEM=$(grep -o '\[Groups/0/Items/[0-9]*\]' "$PROFILE" | cut -d '/' -f 3 | tr -d ']' | sort -n | tail -1)
        if [ -z "$MAX_ITEM" ]; then NEXT_ITEM=1; else NEXT_ITEM=$((MAX_ITEM + 1)); fi
        sed -i "s/\[Groups\/0\/Items\/0\]/\[Groups\/0\/Items\/${NEXT_ITEM}\]/g" "$PROFILE"
        echo "" >> "$PROFILE"
        echo "[Groups/0/Items/0]" >> "$PROFILE"
        echo "Name=cayime" >> "$PROFILE"
        echo "Layout=" >> "$PROFILE"
    fi
    sed -i 's/^DefaultIM=.*/DefaultIM=cayime/' "$PROFILE"
fi

# Khoi dong lai Fcitx5 de nap thong tin moi
nohup fcitx5 -d -r > /dev/null 2>&1 &
disown

echo -e "\n${GREEN}================================================${NC}"
echo -e "${GREEN}   Cai dat local thanh cong!                     ${NC}"
echo -e "${GREEN}================================================${NC}"
echo -e "${CYAN}CayIME da duoc build lai va tu dong cap nhat vao Fcitx5.${NC}"
echo -e "${CYAN}Nhan Ctrl+Space (hoac phim tat chuyen ngon ngu) de su dung!\n${NC}"
