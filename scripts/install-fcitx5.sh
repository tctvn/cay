#!/bin/bash
# GitHub: https://github.com/tctvn/cay
# Len cau cai dat: wget -qO- https://raw.githubusercontent.com/tctvn/cay/main/scripts/install-fcitx5.sh | bash

# CayIME - Fcitx5 Tu Dong Cai Dat Script
# This script downloads the pre-built Fcitx5 plugin from GitHub and installs it.

set -e

# Mau sac cho output
GREEN='\033[0;32m'
CYAN='\033[0;36m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m' # No Color

echo -e "${CYAN}================================================${NC}"
echo -e "${CYAN}   CayIME Fcitx5 Plugin Tu Dong Cai Dat     ${NC}"
echo -e "${CYAN}================================================${NC}"

REPO="tctvn/cay"
ASSET_NAME="cayime-fcitx5-linux.tar.gz"
DOWNLOAD_URL="https://github.com/${REPO}/releases/download/nightly/${ASSET_NAME}"

# Buoc 1: Tai xuong binary duoc xay dung truoc
echo -e "\n${YELLOW}[1/3] Downloading latest pre-built plugin from GitHub...${NC}"
WORK_DIR=$(mktemp -d)
cd "$WORK_DIR"

if ! wget -q --show-progress "$DOWNLOAD_URL"; then
    echo -e "${RED}Failed to download ${ASSET_NAME}. Please make sure the 'nightly' release exists on GitHub.${NC}"
    exit 1
fi

# Buoc 2: Giai nen va cai dat plugin vao he thong (/usr)
echo -e "\n${YELLOW}[2/3] Extracting and installing plugin to system (/usr)...${NC}"
tar -xzf "$ASSET_NAME"

# Dung fcitx5 truoc khi cai dat de giai phat file lock
echo -e "Stopping Fcitx5 to release file locks..."
killall fcitx5 2>/dev/null || true
sleep 1
killall -9 fcitx5 2>/dev/null || true
sleep 1

echo "We need your password (sudo) to copy files to /usr/lib and /usr/share."
# Su dung -f de ghi de
sudo cp -rf usr/* /usr/

# Clean up
cd ~
rm -rf "$WORK_DIR"

# Buoc 3: Tu dong cau hinh profile Fcitx5 (Zero-Click Magic)
echo -e "\n${YELLOW}[3/3] Automatically configuring Fcitx5...${NC}"

PROFILE="${HOME}/.config/fcitx5/profile"
mkdir -p "${HOME}/.config/fcitx5"

# Neu user chua co profile (cai fcitx5 moi tinh), tao profile mac dinh
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
    # Neu profile da ton tai, kiem tra cayime da duoc add chua
    if ! grep -qi "Name=cayime" "$PROFILE"; then
        echo -e "Injecting CayIME into existing Fcitx5 profile..."
        # Tim index lon nhat hien tai trong danh sach Items cua Group 0
        MAX_ITEM=$(grep -o '\[Groups/0/Items/[0-9]*\]' "$PROFILE" | cut -d '/' -f 3 | tr -d ']' | sort -n | tail -1)
        if [ -z "$MAX_ITEM" ]; then
            NEXT_ITEM=1
        else
            NEXT_ITEM=$((MAX_ITEM + 1))
        fi
        # Day bo go duoc chen o vi tri 0 xuong cuoi (NEXT_ITEM) de nho cho CayIME
        sed -i "s/\[Groups\/0\/Items\/0\]/\[Groups\/0\/Items\/${NEXT_ITEM}\]/g" "$PROFILE"
        # Bom cau hinh CayIME vao vi tri 0 (uu tien cao nhat)
        echo "" >> "$PROFILE"
        echo "[Groups/0/Items/0]" >> "$PROFILE"
        echo "Name=cayime" >> "$PROFILE"
        echo "Layout=" >> "$PROFILE"
    fi
    # Luon luon ep cap nhat DefaultIM thanh cayime de go tieng Viet ngay lap tuc
    sed -i 's/^DefaultIM=.*/DefaultIM=cayime/' "$PROFILE"
fi

# Dat Fcitx5 lam bo go nhap mac dinh cua he thong (giup tu khoi dong cung Ubuntu/Debian)
echo -e "\n${YELLOW}[4/4] Setting Fcitx5 as default system input method (Autostart)...${NC}"
if command -v im-config > /dev/null 2>&1; then
    im-config -n fcitx5
else
    # Fallback neu khong co im-config
    echo "run_im fcitx5" > ~/.xinputrc
fi

# Ep tu khoi dong Fcitx5 bang desktop entry (sua loi GNOME Wayland)
mkdir -p ~/.config/autostart
cp /usr/share/applications/org.fcitx.Fcitx5.desktop ~/.config/autostart/ 2>/dev/null || true

# Khoi dong lai Fcitx5 trong nen de nap cau hinh moi (tat tieng/output)
nohup fcitx5 -r -d > /dev/null 2>&1 &
disown

echo -e "\n${GREEN}================================================${NC}"
echo -e "${GREEN}   Installation Successful!                     ${NC}"
echo -e "${GREEN}================================================${NC}"
echo -e "${CYAN}CayIME da duoc tu dong them vao Fcitx5.${NC}"
echo -e "${CYAN}Hay nhan Ctrl+Space (hoac phim tat chuyen ngon ngu cua ban) va tan huong trai nghiem go bay bong!\n${NC}"
