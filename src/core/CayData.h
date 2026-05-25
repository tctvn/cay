#pragma once

#include "CayTypes.h"
#define MAX_BUFFER 64

namespace Cay {

// ---------------------------------------------------------------------------
// Chỉ số dấu (dùng làm index array trong toàn bộ engine).
//   0 = không dấu (ngang)
//   1 = huyền  (f)
//   2 = sắc    (s)
//   3 = hỏi    (r)
//   4 = ngã    (x)
//   5 = nặng   (j)
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// CayData
//
// Class helper static thuần túy: bảng validation và map ký tự có dấu.
// Không có instance, không cấp phát động, không dùng CRT.
// ---------------------------------------------------------------------------
class CayData {
public:
    // Trả về true nếu chuỗi `s` (đã null-terminated) với độ dài `len`
    // là một cụm phụ âm đầu tiếng Việt hợp lệ.
    static bool IsValidInitial(const wchar_t* s, int len);

    // Trả về true nếu chuỗi `s` (đã null-terminated) với độ dài `len`
    // là một nguyên âm tiếng Việt hợp lệ.
    static bool IsValidNucleus(const wchar_t* s, int len);

    // Map phím Telex modifier sang chỉ số dấu (0–5).
    // Trả về -1 nếu phím không phải là phím dấu.
    static int  GetToneIndex(wchar_t key);

    // Trả về codepoint có dấu cho nguyên âm cơ bản + chỉ số dấu.
    // Trả về 0 nếu không có mapping.
    static wchar_t GetToneMark(wchar_t base, int toneIndex);

    // Trả về true nếu `ch` là ký tự tiếng Việt có dấu
    // (đã có dấu mũ, dấu hỏi, dấu ngắn hoặc dấu thanh).
    static bool HasVietnameseMark(wchar_t ch);

    // Trả về true nếu bất kỳ ký tự nào trong `buf[0..len)` có dấu tiếng Việt.
    static bool HasVietnameseMark(const wchar_t* buf, int len);

    // Bỏ dấu thanh từ nguyên âm, trả về nguyên âm ASCII thuần (a/e/i/o/u/y).
    // Trả về `ch` không đổi nếu không phải nguyên âm có dấu.
    static wchar_t StripTone(wchar_t ch);

    // Bỏ dấu mũ/dấu hỏi/dấu ngắn từ nguyên âm, trả về nguyên âm ASCII thuần.
    // Trả về `ch` không đổi nếu không có dấu nào.
    static wchar_t StripAccent(wchar_t ch);

    // Trả về true nếu `ch` là bất kỳ dạng nào của nguyên âm tiếng Việt (thuần hoặc có dấu).
    static bool IsVowel(wchar_t ch);

    // Lấy quy tắc dấu mũ cho nguyên âm
    static wchar_t GetHookRule(wchar_t c);
};

} // namespace Cay
