#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "pio_usb.h"
#include "tusb.h"
#include "bsp/board.h"
#include "CayEngine.h"

// Biến toàn cục
Cay::TelexEngine cayEngine;
bool isImeEnabled = true;
bool use_helper = false;


// Hàng đợi gửi HID report để tránh re-entrancy
#define INJECT_QUEUE_SIZE 512
hid_keyboard_report_t inject_queue[INJECT_QUEUE_SIZE];
volatile int inject_head = 0;
volatile int inject_tail = 0;

void enqueue_report(hid_keyboard_report_t const *report) {
    int next = (inject_tail + 1) % INJECT_QUEUE_SIZE;
    if (next != inject_head) {
        inject_queue[inject_tail] = *report;
        inject_tail = next;
    }
}

void enqueue_keys(uint8_t modifier, uint8_t key) {
    hid_keyboard_report_t rep = {0};
    rep.modifier = modifier;
    rep.keycode[0] = key;
    enqueue_report(&rep);
}

void enqueue_keys_array(uint8_t modifier, const uint8_t* keys) {
    hid_keyboard_report_t rep = {0};
    rep.modifier = modifier;
    memcpy(rep.keycode, keys, 6);
    enqueue_report(&rep);
}

// Đèn LED
#ifndef PICO_DEFAULT_LED_PIN
#define PICO_DEFAULT_LED_PIN 25 // Thay đổi tùy board
#endif

// Hàm callback inject text cho CayEngine
void onInjectText(int backspaceCount, const wchar_t* newText, int newTextLen) {
    bool needs_helper = false;
    for (int i = 0; i < newTextLen; i++) {
        if (newText[i] >= 128) {
            needs_helper = true;
            break;
        }
    }

    if (use_helper && needs_helper) {
        uint8_t raw_report[64] = {0};
        raw_report[0] = 0xAA; // Magic byte
        raw_report[1] = (uint8_t)backspaceCount;
        raw_report[2] = (uint8_t)newTextLen;
        
        // Copy wchar_t array explicitly as 16-bit to match Windows (Pico uses 32-bit wchar_t)
        int copy_len = newTextLen;
        if (copy_len > 30) copy_len = 30;
        for (int i = 0; i < copy_len; i++) {
            uint16_t ch = (uint16_t)newText[i];
            raw_report[3 + i*2] = (uint8_t)(ch & 0xFF);
            raw_report[3 + i*2 + 1] = (uint8_t)((ch >> 8) & 0xFF);
        }
        
        while (!tud_hid_n_ready(1)) { tud_task(); tuh_task(); }
        tud_hid_n_report(1, 0, raw_report, 64);
        return;
    }

    // 1. Gửi backspace
    for (int i = 0; i < backspaceCount; ++i) {
        enqueue_keys(0, HID_KEY_BACKSPACE);
        enqueue_keys(0, 0);
    }
    
    // 2. Gửi newText
    for (int i = 0; i < newTextLen; ++i) {
        wchar_t c = newText[i];
        
        if (c < 128) {
            // Chuyển ký tự ASCII sang HID keycode
            uint8_t keycode = 0;
            uint8_t modifier = 0;
            
            if (c >= 'a' && c <= 'z') keycode = HID_KEY_A + (c - 'a');
            else if (c >= 'A' && c <= 'Z') { keycode = HID_KEY_A + (c - 'A'); modifier = KEYBOARD_MODIFIER_LEFTSHIFT; }
            else if (c >= '1' && c <= '9') keycode = HID_KEY_1 + (c - '1');
            else if (c == '0') keycode = HID_KEY_0;
            else if (c == ' ') keycode = HID_KEY_SPACE;
            else if (c == '\n') keycode = HID_KEY_ENTER;
            // Dấu cơ bản
            else if (c == ',') keycode = HID_KEY_COMMA;
            else if (c == '.') keycode = HID_KEY_PERIOD;
            else if (c == '/') keycode = HID_KEY_SLASH;
            else if (c == ';') keycode = HID_KEY_SEMICOLON;
            else if (c == '\'') keycode = HID_KEY_APOSTROPHE;
            else if (c == '[') keycode = HID_KEY_BRACKET_LEFT;
            else if (c == ']') keycode = HID_KEY_BRACKET_RIGHT;
            else if (c == '\\') keycode = HID_KEY_BACKSLASH;
            else if (c == '-') keycode = HID_KEY_MINUS;
            else if (c == '=') keycode = HID_KEY_EQUAL;
            
            if (keycode != 0) {
                enqueue_keys(modifier, keycode);
                enqueue_keys(0, 0);
                continue;
            }
        }
        
        // Gửi Unicode bằng Alt + Numpad '+' + Hex
        // Yêu cầu Windows bật EnableHexNumpad trong Registry
        enqueue_keys(KEYBOARD_MODIFIER_LEFTALT, 0);
        enqueue_keys(KEYBOARD_MODIFIER_LEFTALT, HID_KEY_KEYPAD_ADD);
        enqueue_keys(KEYBOARD_MODIFIER_LEFTALT, 0);
        
        char hexStr[16];
        snprintf(hexStr, sizeof(hexStr), "%X", (unsigned int)c);
        for(int j=0; hexStr[j]; j++) {
            uint8_t key = 0;
            char h = hexStr[j];
            if (h >= '0' && h <= '9') {
                if (h == '0') key = HID_KEY_KEYPAD_0;
                else key = HID_KEY_KEYPAD_1 + (h - '1');
            } else if (h >= 'A' && h <= 'F') {
                key = HID_KEY_A + (h - 'A'); // Hex A-F dùng phím chữ thường
            }
            
            enqueue_keys(KEYBOARD_MODIFIER_LEFTALT, key);
            enqueue_keys(KEYBOARD_MODIFIER_LEFTALT, 0);
        }
        
        // Nhả Alt
        enqueue_keys(0, 0);
    }
}

// Chuyển đổi HID Keycode sang Cay::KeyCode (Cơ bản)
Cay::KeyCode hid2cay(uint8_t hid_code) {
    if (hid_code >= HID_KEY_A && hid_code <= HID_KEY_Z) {
        return (Cay::KeyCode)('A' + (hid_code - HID_KEY_A));
    }
    switch (hid_code) {
        case HID_KEY_BACKSPACE: return Cay::KeyCode::Backspace;
        case HID_KEY_ENTER: return Cay::KeyCode::Enter;
        case HID_KEY_SPACE: return Cay::KeyCode::Space;
    }
    return Cay::KeyCode::Unknown;
}

// Hàm chuyển HID sang ký tự wchar_t (Dành cho CayEngine)
wchar_t hid2char(uint8_t hid_code, bool shift) {
    if (hid_code >= HID_KEY_A && hid_code <= HID_KEY_Z) {
        char base = shift ? 'A' : 'a';
        return base + (hid_code - HID_KEY_A);
    }
    return 0; // Các phím không phải chữ không cần character mapping cho Telex Engine
}

// Gửi một report tới máy tính
void forward_report(hid_keyboard_report_t const *report) {
    enqueue_report(report);
}

// Danh sách các phím đang bị suppress bởi CayEngine
static uint8_t suppressed_keys[6] = {0};

void add_suppressed_key(uint8_t key) {
    for(int i=0; i<6; i++) {
        if(suppressed_keys[i] == 0) {
            suppressed_keys[i] = key;
            return;
        }
    }
}

void remove_suppressed_key(uint8_t key) {
    for(int i=0; i<6; i++) {
        if(suppressed_keys[i] == key) {
            suppressed_keys[i] = 0;
        }
    }
}

bool is_suppressed(uint8_t key) {
    for(int i=0; i<6; i++) {
        if(suppressed_keys[i] == key && key != 0) return true;
    }
    return false;
}

// Xử lý báo cáo từ bàn phím Host
void process_kbd_report(hid_keyboard_report_t const *report) {
    static hid_keyboard_report_t prev_report = { 0, 0, {0} };
    
    bool current_ctrl = report->modifier & (KEYBOARD_MODIFIER_LEFTCTRL | KEYBOARD_MODIFIER_RIGHTCTRL);
    bool current_shift = report->modifier & (KEYBOARD_MODIFIER_LEFTSHIFT | KEYBOARD_MODIFIER_RIGHTSHIFT);
    bool prev_ctrl = prev_report.modifier & (KEYBOARD_MODIFIER_LEFTCTRL | KEYBOARD_MODIFIER_RIGHTCTRL);
    bool prev_shift = prev_report.modifier & (KEYBOARD_MODIFIER_LEFTSHIFT | KEYBOARD_MODIFIER_RIGHTSHIFT);
    
    bool current_alt = report->modifier & (KEYBOARD_MODIFIER_LEFTALT | KEYBOARD_MODIFIER_RIGHTALT);
    bool current_gui = report->modifier & (KEYBOARD_MODIFIER_LEFTGUI | KEYBOARD_MODIFIER_RIGHTGUI);
    
    // Bắt Ctrl + Shift để toggle IME
    if (current_ctrl && current_shift && !(prev_ctrl && prev_shift)) {
        isImeEnabled = !isImeEnabled;
        gpio_put(PICO_DEFAULT_LED_PIN, isImeEnabled);
        cayEngine.ResetFull();
        memset(suppressed_keys, 0, sizeof(suppressed_keys));
    }
    
    // Nếu có phím modifier (Ctrl, Alt, Win) được giữ, reset engine để không làm hỏng các phím tắt (Ctrl+A, Ctrl+C...)
    bool has_modifier = current_ctrl || current_alt || current_gui;
    if (has_modifier) {
        cayEngine.ResetFull();
    }
    
    // Xóa các phím đã thả khỏi danh sách suppress
    for (uint8_t i = 0; i < 6; i++) {
        uint8_t prev_key = prev_report.keycode[i];
        if (prev_key) {
            bool still_pressed = false;
            for (uint8_t j = 0; j < 6; j++) {
                if (report->keycode[j] == prev_key) still_pressed = true;
            }
            if (!still_pressed) remove_suppressed_key(prev_key);
        }
    }
    
    // Tạo report mới để gửi cho PC
    hid_keyboard_report_t new_report = *report;
    memset(new_report.keycode, 0, 6);
    uint8_t new_idx = 0;

    // Xử lý các phím đang nhấn
    for(uint8_t i=0; i<6; i++) {
        uint8_t key = report->keycode[i];
        if (key) {
            bool is_new = true;
            for(uint8_t j=0; j<6; j++) {
                if (key == prev_report.keycode[j]) {
                    is_new = false;
                    break;
                }
            }
            
            // Chỉ đưa vào CayEngine nếu IME đang bật và không có phím modifier nào được giữ
            if (is_new && isImeEnabled && !has_modifier) {
                Cay::KeyEvent e;
                e.keyCode = hid2cay(key);
                e.character = hid2char(key, current_shift);
                e.handled = false;
                
                if (e.keyCode == Cay::KeyCode::Backspace) {
                    // Bypass backspace để OS tự xóa và lặp phím khi giữ
                    cayEngine.ResetFull();
                } else if (e.keyCode != Cay::KeyCode::Unknown) {
                    cayEngine.OnKeyDown(e);
                }
                
                if (e.handled) {
                    add_suppressed_key(key);
                }
            }
            
            // Nếu phím không bị suppress, thêm vào report gửi đi
            if (!is_suppressed(key) && new_idx < 6) {
                new_report.keycode[new_idx++] = key;
            }
        }
    }
    
    forward_report(&new_report);
    prev_report = *report;
}

// Callbacks của Host
void tuh_hid_keyboard_mounted_cb(uint8_t dev_addr) {
    (void)dev_addr;
}

void tuh_hid_keyboard_unmounted_cb(uint8_t dev_addr) {
    (void)dev_addr;
}

void tuh_hid_keyboard_isr(uint8_t dev_addr, xfer_result_t event) {
    (void)dev_addr;
    (void)event;
}

// Callback từ tinyusb host
void tuh_hid_report_received_cb(uint8_t dev_addr, uint8_t instance, uint8_t const* report, uint16_t len) {
    uint8_t const itf_protocol = tuh_hid_interface_protocol(dev_addr, instance);
    if (itf_protocol == HID_ITF_PROTOCOL_KEYBOARD) {
        process_kbd_report((hid_keyboard_report_t const*) report);
    }
    tuh_hid_receive_report(dev_addr, instance);
}

void tuh_hid_mount_cb(uint8_t dev_addr, uint8_t instance, uint8_t const* desc_report, uint16_t desc_len) {
    tuh_hid_receive_report(dev_addr, instance);
}

void tuh_hid_umount_cb(uint8_t dev_addr, uint8_t instance) {}

// Callbacks của Device
uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t reqlen) { return 0; }

// Bắt trạng thái LED từ PC và chuyển xuống Host keyboard, đồng thời nhận Handshake từ App Helper
static uint8_t host_led_mask = 0;
void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize) {
    if (instance == 0 && report_type == HID_REPORT_TYPE_OUTPUT && bufsize > 0) {
        host_led_mask = buffer[0];
        
        for(uint8_t dev_addr=1; dev_addr<=CFG_TUH_DEVICE_MAX; dev_addr++) {
            for(uint8_t inst=0; inst<CFG_TUH_HID; inst++) {
                if (tuh_hid_interface_protocol(dev_addr, inst) == HID_ITF_PROTOCOL_KEYBOARD) {
                    tuh_hid_set_report(dev_addr, inst, 0, HID_REPORT_TYPE_OUTPUT, (void*)&host_led_mask, 1);
                }
            }
        }
    } else if (instance == 1 && report_type == HID_REPORT_TYPE_OUTPUT && bufsize > 0) {
        // Nhận Handshake từ App Helper
        if (buffer[0] == 0xAA) {
            use_helper = true;
        } else if (buffer[0] == 0x00) {
            use_helper = false;
        }
    }
}

int main(void) {
    board_init();
    
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
    gpio_put(PICO_DEFAULT_LED_PIN, isImeEnabled);
    
    // Setup PIO USB host (sử dụng chân mặc định hoặc tùy config của TinyUSB/pico_pio_usb)
    pio_usb_configuration_t pio_cfg = PIO_USB_DEFAULT_CONFIG;
    pio_cfg.pin_dp = 12; // Dựa theo thiết kế Waveshare RP2040/RP2350 USB-A
    tuh_configure(1, TUH_CFGID_RPI_PIO_USB_CONFIGURATION, &pio_cfg);

    tusb_init();

    // Init CayEngine
    cayEngine.OnInjectText = onInjectText;

    while (1) {
        tud_task();
        tuh_task();
        
        if (inject_head != inject_tail && tud_hid_ready()) {
            tud_hid_keyboard_report(
                0, 
                inject_queue[inject_head].modifier, 
                inject_queue[inject_head].keycode
            );
            inject_head = (inject_head + 1) % INJECT_QUEUE_SIZE;
        }
    }

    return 0;
}
