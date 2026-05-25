#pragma once
#include <windows.h>
#include "InputInjector.h"

namespace CayIME {

// ---------------------------------------------------------------------------
// HookKeyEventArgs – được truyền đến callbacks key-event.
// ---------------------------------------------------------------------------
struct HookKeyEventArgs {
    DWORD      keyCode;     // Virtual-key code (ví dụ VK_A, VK_BACK …)
    wchar_t    character;   // Ký tự Unicode đã dịch (0 nếu không có)
    ULONG_PTR  extraInfo;   // dwExtraInfo từ low-level hook
    bool       handled;     // Set true bởi handler để suppress keystroke
};

// ---------------------------------------------------------------------------
// InputHookManager
//
// Cài đặt hooks WH_KEYBOARD_LL và WH_MOUSE_LL.
// Callbacks là raw function pointers – không std::function, không heap allocation.
// Key debouncing dùng bitmask 256-bit (4 × DWORD64) được key bởi VK code.
// ---------------------------------------------------------------------------
class InputHookManager {
public:
    // Callback signatures.
    using KeyCallback   = void (*)(InputHookManager* sender, HookKeyEventArgs& e);
    using MouseCallback = void (*)(InputHookManager* sender);

    KeyCallback   KeyDown   = nullptr;
    KeyCallback   KeyUp     = nullptr;
    MouseCallback MouseClick = nullptr;

    InputHookManager();
    ~InputHookManager();

private:
    HHOOK _kbHook;
    HHOOK _mouseHook;

    // Bitmask pressed-state 256-bit.  Bit N = VK code N hiện đang down.
    DWORD64 _keyState[4];   // 4 × 64 = 256 bits

    void SetKeyBit(DWORD vk);
    void ClearKeyBit(DWORD vk);
    bool TestKeyBit(DWORD vk) const;

    static LRESULT CALLBACK KbProc  (int nCode, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK MouseProc(int nCode, WPARAM wParam, LPARAM lParam);

    // Singleton pointer để static callbacks có thể reach instance.
    static InputHookManager* s_instance;
};

} // namespace CayIME
