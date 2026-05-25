#include "KeyboardHookManager.h"

namespace CayIME {

// Static singleton pointer.
InputHookManager* InputHookManager::s_instance = nullptr;

// ---------------------------------------------------------------------------
// Construction / Destruction - Khởi tạo / Hủy
// ---------------------------------------------------------------------------
InputHookManager::InputHookManager() {
    s_instance = this;

    // Zero bitmask 256-bit.
    _keyState[0] = 0;
    _keyState[1] = 0;
    _keyState[2] = 0;
    _keyState[3] = 0;

    _kbHook    = SetWindowsHookExW(WH_KEYBOARD_LL, KbProc,    nullptr, 0);
    _mouseHook = SetWindowsHookExW(WH_MOUSE_LL,    MouseProc, nullptr, 0);
}

InputHookManager::~InputHookManager() {
    if (_kbHook)    { UnhookWindowsHookEx(_kbHook);    _kbHook    = nullptr; }
    if (_mouseHook) { UnhookWindowsHookEx(_mouseHook); _mouseHook = nullptr; }
    if (s_instance == this) s_instance = nullptr;
}

// ---------------------------------------------------------------------------
// 256-bit bitmask helpers - Helper bitmask 256-bit
// ---------------------------------------------------------------------------
void InputHookManager::SetKeyBit(DWORD vk) {
    if (vk >= 256) return;
    _keyState[vk >> 6] |=  (DWORD64(1) << (vk & 63));
}

void InputHookManager::ClearKeyBit(DWORD vk) {
    if (vk >= 256) return;
    _keyState[vk >> 6] &= ~(DWORD64(1) << (vk & 63));
}

bool InputHookManager::TestKeyBit(DWORD vk) const {
    if (vk >= 256) return false;
    return (_keyState[vk >> 6] & (DWORD64(1) << (vk & 63))) != 0;
}

// ---------------------------------------------------------------------------
// Low-level keyboard hook procedure - Procedure hook keyboard low-level
// ---------------------------------------------------------------------------
LRESULT CALLBACK InputHookManager::KbProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode < 0 || !s_instance) {
        return CallNextHookEx(nullptr, nCode, wParam, lParam);
    }

    KBDLLHOOKSTRUCT* kb = reinterpret_cast<KBDLLHOOKSTRUCT*>(lParam);
    if (kb->dwExtraInfo == InputInjector::MAGIC_EXTRA_INFO) {
        return CallNextHookEx(nullptr, nCode, wParam, lParam);
    }

    DWORD vk = kb->vkCode;

    // Dịch VK sang ký tự Unicode (best-effort, single char).
    wchar_t ch = 0;
    if (vk >= 'A' && vk <= 'Z') {
        // Mapping ASCII đơn giản: shift sang lowercase trừ khi Shift được giữ.
        bool shifted = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
        bool capsLk = (GetKeyState(VK_CAPITAL) & 0x0001) != 0;
        bool upper = shifted ^ capsLk;
        ch = upper ? (wchar_t)vk : (wchar_t)(vk + 32);
    } else if (vk >= '0' && vk <= '9') {
        ch = (wchar_t)vk;
    }

    bool isDown = (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN);
    bool isUp   = (wParam == WM_KEYUP   || wParam == WM_SYSKEYUP);

    // Debounce: skip key-down nếu đã được ghi nhận là down.
    if (isDown && s_instance->TestKeyBit(vk)) {
        return CallNextHookEx(nullptr, nCode, wParam, lParam);
    }

    HookKeyEventArgs args;
    args.keyCode   = vk;
    args.character = ch;
    args.extraInfo = kb->dwExtraInfo;
    args.handled   = false;

    if (isDown) {
        s_instance->SetKeyBit(vk);
        if (s_instance->KeyDown) {
            s_instance->KeyDown(s_instance, args);
        }
    } else if (isUp) {
        s_instance->ClearKeyBit(vk);
        if (s_instance->KeyUp) {
            s_instance->KeyUp(s_instance, args);
        }
    }

    if (args.handled) return 1; // suppress keystroke
    return CallNextHookEx(nullptr, nCode, wParam, lParam);
}

// ---------------------------------------------------------------------------
// Low-level mouse hook procedure - Procedure hook mouse low-level
// ---------------------------------------------------------------------------
LRESULT CALLBACK InputHookManager::MouseProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode >= 0 && s_instance) {
        // Bất kỳ nhấn nút mouse nào cũng reset engine.
        if (wParam == WM_LBUTTONDOWN || wParam == WM_RBUTTONDOWN ||
            wParam == WM_MBUTTONDOWN || wParam == WM_XBUTTONDOWN) {
            if (s_instance->MouseClick) {
                s_instance->MouseClick(s_instance);
            }
        }
    }
    return CallNextHookEx(nullptr, nCode, wParam, lParam);
}

} // namespace CayIME

