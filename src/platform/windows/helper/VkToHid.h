#pragma once
#include <windows.h>
#include <stdint.h>

// Simple VK to HID mapping
inline uint8_t VkToHid(DWORD vkCode) {
    if (vkCode >= 'A' && vkCode <= 'Z') return 0x04 + (vkCode - 'A');
    if (vkCode >= '1' && vkCode <= '9') return 0x1E + (vkCode - '1');
    if (vkCode == '0') return 0x27;
    
    switch (vkCode) {
        case VK_RETURN: return 0x28;
        case VK_ESCAPE: return 0x29;
        case VK_BACK: return 0x2A;
        case VK_TAB: return 0x2B;
        case VK_SPACE: return 0x2C;
        case VK_OEM_MINUS: return 0x2D;
        case VK_OEM_PLUS: return 0x2E;
        case VK_OEM_4: return 0x2F; // [
        case VK_OEM_6: return 0x30; // ]
        case VK_OEM_5: return 0x31; // \
        case VK_OEM_1: return 0x33; // ;
        case VK_OEM_7: return 0x34; // '
        case VK_OEM_3: return 0x35; // `
        case VK_OEM_COMMA: return 0x36;
        case VK_OEM_PERIOD: return 0x37;
        case VK_OEM_2: return 0x38; // /
        case VK_CAPITAL: return 0x39;
        case VK_F1: return 0x3A;
        case VK_F2: return 0x3B;
        case VK_F3: return 0x3C;
        case VK_F4: return 0x3D;
        case VK_F5: return 0x3E;
        case VK_F6: return 0x3F;
        case VK_F7: return 0x40;
        case VK_F8: return 0x41;
        case VK_F9: return 0x42;
        case VK_F10: return 0x43;
        case VK_F11: return 0x44;
        case VK_F12: return 0x45;
        case VK_INSERT: return 0x49;
        case VK_HOME: return 0x4A;
        case VK_PRIOR: return 0x4B;
        case VK_DELETE: return 0x4C;
        case VK_END: return 0x4D;
        case VK_NEXT: return 0x4E;
        case VK_RIGHT: return 0x4F;
        case VK_LEFT: return 0x50;
        case VK_DOWN: return 0x51;
        case VK_UP: return 0x52;
        case VK_LCONTROL: return 0xE0;
        case VK_LSHIFT: return 0xE1;
        case VK_LMENU: return 0xE2;
        case VK_LWIN: return 0xE3;
        case VK_RCONTROL: return 0xE4;
        case VK_RSHIFT: return 0xE5;
        case VK_RMENU: return 0xE6;
        case VK_RWIN: return 0xE7;
    }
    return 0; // Unknown
}

// HID to VK mapping
inline DWORD HidToVk(uint8_t hidCode) {
    if (hidCode >= 0x04 && hidCode <= 0x1D) return 'A' + (hidCode - 0x04);
    if (hidCode >= 0x1E && hidCode <= 0x26) return '1' + (hidCode - 0x1E);
    if (hidCode == 0x27) return '0';
    
    switch (hidCode) {
        case 0x28: return VK_RETURN;
        case 0x29: return VK_ESCAPE;
        case 0x2A: return VK_BACK;
        case 0x2B: return VK_TAB;
        case 0x2C: return VK_SPACE;
        case 0x2D: return VK_OEM_MINUS;
        case 0x2E: return VK_OEM_PLUS;
        case 0x2F: return VK_OEM_4; // [
        case 0x30: return VK_OEM_6; // ]
        case 0x31: return VK_OEM_5; // \
        case 0x33: return VK_OEM_1; // ;
        case 0x34: return VK_OEM_7; // '
        case 0x35: return VK_OEM_3; // `
        case 0x36: return VK_OEM_COMMA;
        case 0x37: return VK_OEM_PERIOD;
        case 0x38: return VK_OEM_2; // /
        case 0x39: return VK_CAPITAL;
        case 0x3A: return VK_F1;
        case 0x3B: return VK_F2;
        case 0x3C: return VK_F3;
        case 0x3D: return VK_F4;
        case 0x3E: return VK_F5;
        case 0x3F: return VK_F6;
        case 0x40: return VK_F7;
        case 0x41: return VK_F8;
        case 0x42: return VK_F9;
        case 0x43: return VK_F10;
        case 0x44: return VK_F11;
        case 0x45: return VK_F12;
        case 0x49: return VK_INSERT;
        case 0x4A: return VK_HOME;
        case 0x4B: return VK_PRIOR;
        case 0x4C: return VK_DELETE;
        case 0x4D: return VK_END;
        case 0x4E: return VK_NEXT;
        case 0x4F: return VK_RIGHT;
        case 0x50: return VK_LEFT;
        case 0x51: return VK_DOWN;
        case 0x52: return VK_UP;
        case 0xE0: return VK_LCONTROL;
        case 0xE1: return VK_LSHIFT;
        case 0xE2: return VK_LMENU;
        case 0xE3: return VK_LWIN;
        case 0xE4: return VK_RCONTROL;
        case 0xE5: return VK_RSHIFT;
        case 0xE6: return VK_RMENU;
        case 0xE7: return VK_RWIN;
    }
    return 0; // Unknown
}
