#pragma once
#include <windows.h>

namespace CayIME {

class InputInjector {
public:
    // Giá trị sentinel được stamp trên mỗi sự kiện synthetic để hook của chúng ta ignore chúng.
    static const ULONG_PTR MAGIC_EXTRA_INFO = 0x1234;

    // Thay thế text tại vị trí caret hiện tại.
    // Gửi: [ZWJ dummy] + [backspaceCount x VK_BACK] + [newText characters]
    // tất cả trong một lần gọi SendInput để tránh Chrome autocomplete races.
    static void ReplaceText(int backspaceCount, const wchar_t* newText, int newTextLen);
};

} // namespace CayIME
