#pragma once
#include "CayData.h"
#include "CayTypes.h"

namespace Cay {

// ---------------------------------------------------------------------------
// MyKey – bản ghi compact của một phím thô + ký tự nó tạo ra
// trong output buffer (có thể khác với phím thô sau khi transform).
// ---------------------------------------------------------------------------
struct MyKey {
    wchar_t raw;       // Ký tự thô người dùng gõ (ví dụ 'a', 'w', 's')
    wchar_t output;    // Ký tự hiện tại trong output text
};

// ---------------------------------------------------------------------------
// TelexEngine
//
// State machine xử lý Telex chính.
// Sở hữu:
//   _buffer[MAX_BUFFER]  – bản ghi phím thô
//   _bufferCount         – số entry hợp lệ trong _buffer
//   _text[MAX_BUFFER]    – ký tự output hiện tại (đã được inject)
//   _textLen             – độ dài của _text
//
// Tất cả xử lý được thực hiện qua backward-scan (QUY TẮC 3).
// Không cấp phát động, không STL, không CRT.
// ---------------------------------------------------------------------------
class TelexEngine {
public:
    TelexEngine();

    // Được gọi mỗi khi keydown (main.cpp delegate đến đây).
    void OnKeyDown(Cay::KeyEvent& e);

    // Được gọi mỗi khi keyup (hiện tại không làm gì, dành cho tương lai).
    void OnKeyUp(Cay::KeyEvent& e);

    // Hard reset: flush buffer và discard tất cả state.
    void ResetFull();

    // Commit từ hiện tại: lưu state để recall, sau đó reset.
    void CommitWord();

    typedef const wchar_t* (*MacroLookupFunc)(const wchar_t* input);
    MacroLookupFunc OnMacroLookup = nullptr;

    InjectTextFunc OnInjectText = nullptr;

    bool autoRestoreEnabled = true;
    InputMethod inputMethod = InputMethod::Telex;

private:
    MyKey _buffer[MAX_BUFFER];
    int   _bufferCount;

    wchar_t _text[MAX_BUFFER];
    int     _textLen;

    // Chỉ số dấu hiện tại (0–5) được áp dụng cho từ này, hoặc -1 nếu không có.
    int  _toneIndex;

    // Tracking output để update màn hình chính xác (tính toán diff chính xác)
    wchar_t _lastOutput[MAX_BUFFER];
    int     _lastOutputLen;

    // State recall từ
    MyKey   _savedBuffer[MAX_BUFFER];
    int     _savedBufferCount;
    wchar_t _savedText[MAX_BUFFER];
    int     _savedTextLen;
    int     _savedToneIndex;
    bool    _canRestore;

    void SaveState();

    // -----------------------------------------------------------------------
    // Các bước transform chính – tất cả mutate _text[] in place.
    // -----------------------------------------------------------------------

    // Xử lý phím dấu mũ đôi (aa->â, ee->ê, oo->ô, dd->đ).
    // Trả về true nếu phím được tiêu thụ như modifier phím đôi.
    bool ApplyDoubleKeys(wchar_t key);

    // Xử lý phím dấu mũ/dấu ngắn ('w').
    // Trả về true nếu phím được tiêu thụ như modifier dấu mũ.
    bool ApplyHookKeys(wchar_t key);

    // Áp dụng (hoặc thay đổi) dấu thanh vào output buffer.
    // Trả về true nếu dấu được áp dụng.
    bool ApplyToneMarks(int toneIndex);

    // Bỏ tất cả dấu thanh từ _text[], rewrite in place.
    void StripAllTones();

    // Tìm index nguyên âm ngoài cùng bên phải trong _text[0.._textLen)
    // là ứng viên tốt để nhận dấu thanh (quy tắc đặt dấu tiếng Việt).
    int FindTonePosition() const;

    // Update màn hình hiệu quả bằng cách tính toán backspaces chính xác
    void UpdateScreen(const wchar_t* newOutput, int newOutputLen);

    // Commit từ hiện tại: inject _text[] để thay thế những gì user thấy.
    void Commit(int extraBs = 0);

    // Revert về input ASCII thô (fallback tiếng Anh).
    void FallbackToRaw();

    // Kiểm tra xem buffer hiện tại có trông giống từ tiếng Anh không
    // và nên bypass xử lý tiếng Việt.
    bool ShouldBypassWord() const;

    // Reset internal state mà không gửi input nào.
    void ResetState();

    // Replay 1 phím thô vào engine (record + try modifiers + fallback append).
    // Dùng chung cho cả OnKeyDown lẫn Backspace replay, tránh duplicate logic.
    void ReplayKey(wchar_t ch);

    // Helpers
    static bool IsAlpha(wchar_t ch);
    static wchar_t ToLowerViet(wchar_t c);
    static wchar_t ToUpperViet(wchar_t c);
};

} // namespace Cay
