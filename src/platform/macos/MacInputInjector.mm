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

    // Backspace × N: hoạt động đúng trên cả GUI text-field lẫn TTY (Terminal).
    // Atomic và universal hơn Shift+Left (chỉ work trên GUI có concept selection).
    if (backspaceCount > 0) {
        for (int i = 0; i < backspaceCount; ++i) {
            PostKeyboardEvent(51, true);   // kVK_Delete (Backspace)
            PostKeyboardEvent(51, false);
        }
    }

    // Insert Unicode tại vị trí cursor (sau khi đã xoá N ký tự).
    if (newText != nullptr && newTextLen > 0) {
        UniChar chars[64];
        int uniLen = 0;
        for (int i = 0; i < newTextLen && uniLen < 64; ++i) {
            chars[uniLen++] = (UniChar)newText[i];
        }
        PostUnicodeString(chars, uniLen);
    }
}

} // namespace CayIME
