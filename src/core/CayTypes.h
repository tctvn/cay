#pragma once
#include <cstdint>

namespace Cay {

    // Mã phím - enum các phím được hỗ trợ
    enum class KeyCode : uint32_t {
        Unknown = 0,
        Backspace, Escape, Enter, Tab, Space,
        Left, Right, Up, Down, Home, End, PageUp, PageDown, Delete,
        KeyA = 'A', KeyB = 'B', KeyC = 'C', KeyD = 'D', KeyE = 'E',
        KeyF = 'F', KeyG = 'G', KeyH = 'H', KeyI = 'I', KeyJ = 'J',
        KeyK = 'K', KeyL = 'L', KeyM = 'M', KeyN = 'N', KeyO = 'O',
        KeyP = 'P', KeyQ = 'Q', KeyR = 'R', KeyS = 'S', KeyT = 'T',
        KeyU = 'U', KeyV = 'V', KeyW = 'W', KeyX = 'X', KeyY = 'Y', KeyZ = 'Z'
    };

    // Sự kiện phím - chứa thông tin về phím được nhấn
    struct KeyEvent {
        KeyCode keyCode = KeyCode::Unknown;          // Mã phím
        wchar_t character = 0;        // Ký tự Unicode đã chuyển đổi
        bool handled = false;             // Đã xử lý hay chưa
    };

    // Hàm callback để inject text - được gọi khi cần thay thế text
    typedef void (*InjectTextFunc)(int backspaceCount, const wchar_t* newText, int newTextLen);

    // Helper: tính độ dài chuỗi wide-char (không dùng CRT)
    inline int CayStrLen(const wchar_t* s) {
        int len = 0;
        while (s && *s) { len++; s++; }
        return len;
    }
    // Helper: so sánh chuỗi wide-char (không dùng CRT)
    inline int CayStrCmp(const wchar_t* s1, const wchar_t* s2) {
        while (*s1 && (*s1 == *s2)) { s1++; s2++; }
        return *s1 - *s2;
    }
} // namespace Cay

