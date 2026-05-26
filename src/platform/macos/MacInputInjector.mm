#import "MacInputInjector.h"
#import <CoreGraphics/CoreGraphics.h>

namespace CayIME {

// Cờ đánh dấu các event do bộ gõ tự tạo, tránh loop
const int CAY_EVENT_MAGIC_FLAG = 9999;

// Cờ báo hiệu vừa có sự kiện thay thế văn bản xảy ra
bool g_justInjected = false;

static void PostKeyboardEvent(CGKeyCode keyCode, bool isKeyDown) {
    CGEventRef event = CGEventCreateKeyboardEvent(NULL, keyCode, isKeyDown);
    if (event) {
        CGEventSetIntegerValueField(event, kCGEventSourceUserData, CAY_EVENT_MAGIC_FLAG);
        CGEventPost(kCGHIDEventTap, event);
        CFRelease(event);
    }
}

static void PostUnicodeString(const UniChar* chars, size_t length) {
    CGEventRef event = CGEventCreateKeyboardEvent(NULL, 0, true);
    if (event) {
        CGEventKeyboardSetUnicodeString(event, length, chars);
        CGEventSetIntegerValueField(event, kCGEventSourceUserData, CAY_EVENT_MAGIC_FLAG);
        CGEventPost(kCGHIDEventTap, event);
        CFRelease(event);
    }
}

void MacInputInjector::ReplaceText(int backspaceCount, const wchar_t* newText, int newTextLen) {
    g_justInjected = true;

    int totalBackspaces = backspaceCount;
    // 1. Nếu có xoá, chúng ta gửi một dummy keydown/up (chữ 'a' = 0x00) 
    // để triệt tiêu popup gợi ý dấu của macOS (như trên Safari)
    if (backspaceCount > 0) {
        PostKeyboardEvent(0, true);
        PostKeyboardEvent(0, false);
        totalBackspaces += 1; // Xoá luôn cả phím dummy 'a' vừa thêm
    }

    // 2. Gửi backspace
    for (int i = 0; i < totalBackspaces; ++i) {
        PostKeyboardEvent(51, true); // 51 là mã kVK_Delete (Backspace)
        PostKeyboardEvent(51, false);
    }

    // 3. Gửi Unicode string mới
    if (newText != nullptr && newTextLen > 0) {
        // wchar_t trên Mac là 32-bit (UTF-32), CGEventKeyboardSetUnicodeString cần UniChar (16-bit UTF-16)
        UniChar chars[64];
        int uniLen = 0;
        for (int i = 0; i < newTextLen && uniLen < 64; ++i) {
            // Đơn giản hoá: giả sử ký tự nằm trong BMP (0x0000 - 0xFFFF)
            // Tiếng Việt hoàn toàn nằm trong BMP nên an toàn
            chars[uniLen++] = (UniChar)newText[i];
        }
        PostUnicodeString(chars, uniLen);
    }
}

} // namespace CayIME
