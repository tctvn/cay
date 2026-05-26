#import "MacHookManager.h"
#import <CoreGraphics/CoreGraphics.h>
#import <Cocoa/Cocoa.h>
#import "CayEngine.h"
#import "MacInputInjector.h"

namespace CayIME {

extern Cay::TelexEngine g_engine;
const int CAY_EVENT_MAGIC_FLAG = 9999;
extern bool g_justInjected;
bool g_enabled = true;

static CFMachPortRef eventTap = NULL;
static CFRunLoopSourceRef runLoopSource = NULL;
static bool s_pendingToggle = false;

static Cay::KeyCode MapCGKeyCodeToCayKeyCode(CGKeyCode vk) {
    switch (vk) {
        case 51: return Cay::KeyCode::Backspace;
        case 53: return Cay::KeyCode::Escape;
        case 36: return Cay::KeyCode::Enter;
        case 48: return Cay::KeyCode::Tab;
        case 49: return Cay::KeyCode::Space;
        case 123: return Cay::KeyCode::Left;
        case 124: return Cay::KeyCode::Right;
        case 126: return Cay::KeyCode::Up;
        case 125: return Cay::KeyCode::Down;
        case 115: return Cay::KeyCode::Home;
        case 119: return Cay::KeyCode::End;
        case 116: return Cay::KeyCode::PageUp;
        case 121: return Cay::KeyCode::PageDown;
        case 117: return Cay::KeyCode::Delete;
    }
    return Cay::KeyCode::Unknown;
}

CGEventRef EventTapCallback(CGEventTapProxy proxy, CGEventType type, CGEventRef event, void *refcon) {
    if (type == kCGEventTapDisabledByTimeout || type == kCGEventTapDisabledByUserInput) {
        CGEventTapEnable(eventTap, true);
        return event;
    }

    if (CGEventGetIntegerValueField(event, kCGEventSourceUserData) == CAY_EVENT_MAGIC_FLAG) {
        return event; // Bỏ qua event do app sinh ra
    }

    if (type == kCGEventLeftMouseDown || type == kCGEventRightMouseDown) {
        g_engine.ResetFull();
        s_pendingToggle = false;
        return event;
    }

    if (type == kCGEventFlagsChanged) {
        CGEventFlags flags = CGEventGetFlags(event);
        bool isCmd = (flags & kCGEventFlagMaskCommand) != 0;
        bool isShift = (flags & kCGEventFlagMaskShift) != 0;
        bool isCtrl = (flags & kCGEventFlagMaskControl) != 0;
        bool isAlt = (flags & kCGEventFlagMaskAlternate) != 0;
        
        // Nếu nhấn đủ Cmd + Shift và không nhấn thêm Ctrl/Alt
        if (isCmd && isShift && !isCtrl && !isAlt) {
            s_pendingToggle = true;
        } else {
            // Nếu đang pending mà nhả phím (flags mất Cmd hoặc Shift)
            if (s_pendingToggle) {
                s_pendingToggle = false;
                dispatch_async(dispatch_get_main_queue(), ^{
                    [[NSNotificationCenter defaultCenter] postNotificationName:@"ToggleIMENotification" object:nil];
                });
            }
        }
        return event;
    }

    if (type == kCGEventKeyDown) {
        s_pendingToggle = false; // Có phím xen ngang, hủy toggle

        CGKeyCode keyCode = (CGKeyCode)CGEventGetIntegerValueField(event, kCGKeyboardEventKeycode);
        CGEventFlags flags = CGEventGetFlags(event);
        
        bool isCmd = (flags & kCGEventFlagMaskCommand) != 0;
        bool isCtrl = (flags & kCGEventFlagMaskControl) != 0;
        bool isAlt = (flags & kCGEventFlagMaskAlternate) != 0;
        
        // Nếu bộ gõ đang tắt, bỏ qua
        if (!g_enabled) {
            return event;
        }

        // Nếu có modifier hệ thống (Command, Ctrl, Option), không xử lý Telex
        if (isCmd || isCtrl || isAlt) {
            g_engine.ResetFull();
            return event;
        }

        // Lấy ký tự thực tế do macOS render (đã bao gồm Shift/Capslock)
        UniChar chars[4];
        UniCharCount actualStringLength = 0;
        CGEventKeyboardGetUnicodeString(event, 4, &actualStringLength, chars);
        
        Cay::KeyEvent ce;
        ce.character = (actualStringLength > 0) ? chars[0] : 0;
        
        if ((ce.character >= L'a' && ce.character <= L'z') || (ce.character >= L'A' && ce.character <= L'Z')) {
            char upperChar = (ce.character >= L'a' && ce.character <= L'z') ? (char)(ce.character - 32) : (char)ce.character;
            ce.keyCode = (Cay::KeyCode)upperChar;
        } else {
            ce.keyCode = MapCGKeyCodeToCayKeyCode(keyCode);
        }
        
        ce.handled = false;
        
        g_justInjected = false;
        g_engine.OnKeyDown(ce);
        
        if (ce.handled) {
            return NULL; // Nuốt phím
        } else if (g_justInjected) {
            // CẢI TIẾN: Nếu engine KHÔNG nuốt event (như phím Space, Enter),
            // nhưng engine ĐÃ gọi OnInjectText, các event thay thế sẽ bị OS đẩy vào trước.
            // Điều này gây lỗi sýtem + Space -> ssystem trên Mac.
            // Giải pháp: Nuốt event hiện tại, và tự POST lại nó để nó nằm SAU các event thay thế!
            CGEventSetIntegerValueField(event, kCGEventSourceUserData, CAY_EVENT_MAGIC_FLAG);
            CGEventPost(kCGHIDEventTap, event);
            return NULL;
        }
    }
    
    return event;
}

bool MacHookManager::Initialize() {
    CGEventMask eventMask = (CGEventMaskBit(kCGEventKeyDown) | 
                             CGEventMaskBit(kCGEventKeyUp) | 
                             CGEventMaskBit(kCGEventFlagsChanged) | 
                             CGEventMaskBit(kCGEventLeftMouseDown) | 
                             CGEventMaskBit(kCGEventRightMouseDown));

    if (!AXIsProcessTrusted()) {
        NSDictionary *options = @{(__bridge id)kAXTrustedCheckOptionPrompt: @YES};
        AXIsProcessTrustedWithOptions((__bridge CFDictionaryRef)options);
    }

    eventTap = CGEventTapCreate(kCGSessionEventTap,
                                kCGHeadInsertEventTap,
                                kCGEventTapOptionDefault,
                                eventMask,
                                EventTapCallback,
                                NULL);

    if (!eventTap) {
        return false;
    }

    runLoopSource = CFMachPortCreateRunLoopSource(kCFAllocatorDefault, eventTap, 0);
    CFRunLoopAddSource(CFRunLoopGetCurrent(), runLoopSource, kCFRunLoopCommonModes);
    CGEventTapEnable(eventTap, true);

    return true;
}

void MacHookManager::Shutdown() {
    if (runLoopSource) {
        CFRunLoopRemoveSource(CFRunLoopGetCurrent(), runLoopSource, kCFRunLoopCommonModes);
        CFRelease(runLoopSource);
        runLoopSource = NULL;
    }
    if (eventTap) {
        CFRelease(eventTap);
        eventTap = NULL;
    }
}

void MacHookManager::ResetEngine() {
    g_engine.ResetFull();
}

} // namespace CayIME
