#include "CayEngine.h"

// ============================================================================
// CayEngine.cpp  –  Free-style Telex state machine (RULE 3)
//
// Tổng quan kiến trúc
// ---------------------
// Engine duy trì hai array song song:
//   _buffer[_bufferCount]  – mọi phím thô user đã gõ
//   _text[_textLen]        – Unicode output đã được inject đến hiện tại
//
// Trên mỗi keydown engine:
//   1. Kiểm tra các phím đặc biệt/control (Backspace, Escape, Space, etc.)
//   2. Thử áp dụng phím như modifier dấu mũ đôi.
//   3. Thử áp dụng phím như modifier dấu mũ/dấu ngắn ('w').
//   4. Thử áp dụng phím như dấu thanh (s/f/r/x/j/z).
//   5. Fallback về thêm ký tự như chữ cái thuần.
//
// Sau mỗi mutation, _text đã được update được inject qua InputInjector::ReplaceText.
// ============================================================================

namespace Cay {

// ---------------------------------------------------------------------------
// Static helpers - Helper static
// ---------------------------------------------------------------------------
bool TelexEngine::IsAlpha(wchar_t ch) {
    return (ch >= L'a' && ch <= L'z') || (ch >= L'A' && ch <= L'Z');
}

wchar_t TelexEngine::ToLowerViet(wchar_t c) {
    if (c >= L'A' && c <= L'Z') return c + 32;
    if (c >= 0x00C0 && c <= 0x00DD && c != 0x00D7) return c + 0x20; // Latin-1
    if (c == 0x01AF) return 0x01B0; // Ư -> ư
    if (c >= 0x0102 && c <= 0x01A0 && (c % 2 == 0)) return c + 1;   // Ă, Đ, Ĩ, Ũ, Ơ
    if (c >= 0x1EA0 && c <= 0x1EF8 && (c % 2 == 0)) return c + 1;   // Ạ..Ỹ (Latin Extended Additional)
    return c;
}

wchar_t TelexEngine::ToUpperViet(wchar_t c) {
    if (c >= L'a' && c <= L'z') return c - 32;
    if (c >= 0x00E0 && c <= 0x00FD && c != 0x00F7) return c - 0x20;
    if (c == 0x01B0) return 0x01AF; // ư -> Ư
    if (c >= 0x0103 && c <= 0x01A1 && (c % 2 != 0)) return c - 1;
    if (c >= 0x1EA1 && c <= 0x1EF9 && (c % 2 != 0)) return c - 1;
    return c;
}

// ---------------------------------------------------------------------------
// Constructor - Hàm khởi tạo
// ---------------------------------------------------------------------------
TelexEngine::TelexEngine() {
    _canRestore = false;
    ResetState();
}

// ---------------------------------------------------------------------------
// ResetState – zero tất cả mà không chạm vào display.
// ---------------------------------------------------------------------------
void TelexEngine::ResetState() {
    _bufferCount = 0;
    _textLen     = 0;
    _toneIndex   = -1;
    _lastOutputLen = 0;
    _lastOutput[0] = L'\0';
    for (int i = 0; i < MAX_BUFFER; i++) {
        _buffer[i].raw    = 0;
        _buffer[i].output = 0;
        _text[i]          = 0;
    }
}

// ---------------------------------------------------------------------------
// ResetFull – discard buffer và invalidate recall state.
// ---------------------------------------------------------------------------
void TelexEngine::ResetFull() {
    ResetState();
    _canRestore = false;
}

// ---------------------------------------------------------------------------
// SaveState – cache state từ hiện tại để recall.
// ---------------------------------------------------------------------------
void TelexEngine::SaveState() {
    if (_bufferCount > 0) {
        _savedBufferCount = _bufferCount;
        for (int i = 0; i < _bufferCount; i++) _savedBuffer[i] = _buffer[i];
        
        _savedTextLen = _textLen;
        for (int i = 0; i < _textLen; i++) _savedText[i] = _text[i];
        
        _savedToneIndex = _toneIndex;
        _canRestore = true;
    }
}

// ---------------------------------------------------------------------------
// CommitWord – lưu state để recall, sau đó reset.
// ---------------------------------------------------------------------------
void TelexEngine::CommitWord() {
    SaveState();
    ResetState();
}

void TelexEngine::UpdateScreen(const wchar_t* newOutput, int newOutputLen) {
    // 1. Kiểm tra xem có giống nhau không
    bool isIdentical = (_lastOutputLen == newOutputLen);
    if (isIdentical) {
        for (int i = 0; i < newOutputLen; i++) {
            if (_lastOutput[i] != newOutput[i]) {
                isIdentical = false;
                break;
            }
        }
    }
    if (isIdentical) return;

    // 2. Tìm prefix chung
    int commonPrefixLen = 0;
    int minLen = _lastOutputLen < newOutputLen ? _lastOutputLen : newOutputLen;
    for (int i = 0; i < minLen; i++) {
        if (_lastOutput[i] == newOutput[i]) {
            commonPrefixLen++;
        } else {
            break;
        }
    }

    // 3. Tính toán diff chính xác mà không có padding giả
    int backspacesNeeded = _lastOutputLen - commonPrefixLen;
    const wchar_t* textToType = newOutput + commonPrefixLen;
    int textToTypeLen = newOutputLen - commonPrefixLen;

    // 4. Inject phím chính xác
    if (backspacesNeeded > 0 || textToTypeLen > 0) {
        if (OnInjectText) OnInjectText(backspacesNeeded, textToType, textToTypeLen);
    }

    // 5. Update state
    for (int i = 0; i < newOutputLen; i++) {
        _lastOutput[i] = newOutput[i];
    }
    _lastOutput[newOutputLen] = L'\0';
    _lastOutputLen = newOutputLen;
}

// ---------------------------------------------------------------------------
// Commit – thay thế từ đang hiển thị bằng _text[0.._textLen).
// ---------------------------------------------------------------------------
void TelexEngine::Commit(int extraBs) {
    UpdateScreen(_text, _textLen);
}

// ---------------------------------------------------------------------------
// FallbackToRaw – revert về các ký tự ASCII thô user đã gõ.
// Được gọi khi engine quyết định input là tiếng Anh.
// ---------------------------------------------------------------------------
void TelexEngine::FallbackToRaw() {
    // Xây chuỗi ASCII thô từ _buffer.
    wchar_t raw[MAX_BUFFER];
    int rawLen = 0;
    for (int i = 0; i < _bufferCount && rawLen < MAX_BUFFER - 1; i++) {
        raw[rawLen++] = _buffer[i].raw;
    }
    raw[rawLen] = L'\0';

    if (OnInjectText) OnInjectText(_textLen, raw, rawLen);

    // Update _text để phản ánh fallback.
    for (int i = 0; i < rawLen; i++) _text[i] = raw[i];
    _textLen = rawLen;
    _toneIndex = -1;
    _canRestore = false;
}

// ---------------------------------------------------------------------------
// LEVEL 2: Validator cấu trúc — Kiểm tra âm tiết bằng cách walk pointer
// ---------------------------------------------------------------------------
static bool IsCompleteSyllable(const wchar_t* s, int len) {
    if (len == 0 || len > 20) return false;

    // Bảng phụ âm đầu, sắp xếp: dài trước để tránh match ngắn hơn (đã có tiếng Việt)
    static const wchar_t* s_initials[] = {
        L"ngh", L"gh", L"gi", L"ng", L"nh", L"ph",
        L"qu", L"th", L"tr", L"ch", L"kh", L"đ",
        L"b", L"c", L"d", L"g", L"h", L"k",
        L"l", L"m", L"n", L"p", L"r", L"s",
        L"t", L"v", L"x",
        L"" // empty string = không có phụ âm đầu
    };

    // Bảng nhân nguyên âm hợp lệ, dài trước (đã có tiếng Việt)
    static const wchar_t* s_nuclei[] = {
        // 3 nguyên âm
        L"iêu", L"yêu", L"\u01b0\u01a1u", L"uôi", L"ươi", L"oai", L"oay",
        L"uya", L"uyê", L"ieu", L"yeu", L"uoi", L"uou", L"oao", L"oeo", L"uyu", L"uye",
        // 2 nguyên âm
        L"ai", L"ao", L"au", L"ay",
        L"\u00e2u", L"\u00e2y",
        L"eo", L"\u00eau",
        L"ia", L"i\u00ea", L"ie",
        L"iu",
        L"oa", L"oai", L"o\u0103", L"oe", L"oi", L"oo",
        L"\u00f4i",
        L"\u01a1i",
        L"ua", L"u\u00e2", L"u\u00ea", L"ui", L"u\u00f4", L"uy", L"uo", L"ue",
        L"\u01b0a", L"\u01b0i", L"\u01b0u", L"\u01b0\u01a1",
        L"ya", L"y\u00ea", L"ye",
        // 1 nguyên âm
        L"a", L"\u0103", L"\u00e2",
        L"e", L"\u00ea",
        L"i",
        L"o", L"\u00f4", L"\u01a1",
        L"u", L"\u01b0",
        L"y",
    };

    // Bảng phụ âm cuối hợp lệ, dài trước (đã có tiếng Việt)
    static const wchar_t* s_finals[] = {
        L"ng", L"nh", L"ch",
        L"c", L"m", L"n", L"p", L"t",
        L""  // không có phụ âm cuối
    };

    // Bảng vần phụ (tail) (đã có tiếng Việt)
    static const wchar_t* s_tails[] = {
        L"i", L"y", L"o", L"u",
        L""
    };

    const wchar_t* pos = s;
    const wchar_t* end = s + len;

    auto matchStr = [&](const wchar_t* pattern, int plen) -> bool {
        if (pos + plen > end) return false;
        for (int i = 0; i < plen; i++) {
            if (pos[i] != pattern[i]) return false;
        }
        return true;
    };

    // ── BLOCK 1: Khớp phụ âm đầu ────────────────────────────── (đã có tiếng Việt)
    const wchar_t* matchedInitial = nullptr;
    for (int i = 0; i < (int)(sizeof(s_initials)/sizeof(s_initials[0])); i++) {
        int ilen = CayStrLen(s_initials[i]);
        if (ilen == 0) { matchedInitial = L""; break; }
        if (matchStr(s_initials[i], ilen)) {
            pos += ilen;
            matchedInitial = s_initials[i];
            break;
        }
    }

    // Special case cho "gi": nếu phần còn lại KHÔNG bắt đầu bằng nguyên âm
    // (ví dụ "gì", "gìn", "gíp"), có nghĩa là 'i' thực sự là nhân nguyên âm.
    if (matchedInitial && matchedInitial[0] == L'g' && matchedInitial[1] == L'i' && matchedInitial[2] == L'\0') {
        if (pos == end || !CayData::IsVowel(CayData::StripTone(*pos))) {
            pos--; // Roll back 1 character để 'i' trở thành nhân nguyên âm
        }
    }
    if (!matchedInitial) return false;

    // ── BLOCK 2: Khớp nhân nguyên âm (bắt buộc) ─────────────── (đã có tiếng Việt)
    const wchar_t* matchedNucleus = nullptr;
    for (int i = 0; i < (int)(sizeof(s_nuclei)/sizeof(s_nuclei[0])); i++) {
        int nlen = CayStrLen(s_nuclei[i]);
        if (matchStr(s_nuclei[i], nlen)) {
            pos += nlen;
            matchedNucleus = s_nuclei[i];
            break;
        }
    }
    if (!matchedNucleus) return false;

    // ── BLOCK 3: Khớp phụ âm cuối (tùy chọn) ───────────────── (đã có tiếng Việt)
    for (int i = 0; i < (int)(sizeof(s_finals)/sizeof(s_finals[0])); i++) {
        int flen = CayStrLen(s_finals[i]);
        if (flen == 0) break;
        if (matchStr(s_finals[i], flen)) {
            pos += flen;
            break;
        }
    }

    // ── BLOCK 4: Khớp tail (tùy chọn) ──────────────────────── (đã có tiếng Việt)
    for (int i = 0; i < (int)(sizeof(s_tails)/sizeof(s_tails[0])); i++) {
        int tlen = CayStrLen(s_tails[i]);
        if (tlen == 0) break;
        if (matchStr(s_tails[i], tlen)) {
            pos += tlen;
            break;
        }
    }

    // ── KIỂM TRA KẾT THÚC ── (đã có tiếng Việt)
    return (pos == end);
}

// ---------------------------------------------------------------------------
// ShouldBypassWord
//
// Trả về true nếu sequence hiện tại trông giống tiếng Anh và nên bypass
// transform tiếng Việt.
// ---------------------------------------------------------------------------
bool TelexEngine::ShouldBypassWord() const {
    if (CayData::HasVietnameseMark(_text, _textLen)) return false;
    if (_bufferCount == 0) return false;

    // Trích xuất các phím gốc đã gõ (chuyển về chữ thường để dễ check)
    wchar_t raw[16] = {0};
    int len = _bufferCount < 15 ? _bufferCount : 15;
    for(int i = 0; i < len; i++) {
        raw[i] = ToLowerViet(_buffer[i].raw);
    }

    // LEVEL 1: Hard Filter — Cụm phụ âm đầu không hợp lệ
    if (raw[0] == L'w' || raw[0] == L'f' || raw[0] == L'j' || raw[0] == L'z') return true;

    if (len >= 2) {
        // Lu?t Q: B?t bu?c di v?i u
        if (raw[0] == L'q' && raw[1] != L'u') return true;
        
        // Lu?t P: B?t bu?c di v?i h (B? qua c�c t? mu?n nhu pin, pa-t� d? t?i uu g� public, padding)
        if (raw[0] == L'p' && raw[1] != L'h') return true;

        // Lu?t Ph? �m k�p: Ti?ng Vi?t ch? c� 8 c?p ph? �m k�p h?p l? ? d?u t?.
        // Helper: Ki?m tra xem k� t? c� ph?i l� ph? �m ASCII kh�ng
        auto isConsonant = [](wchar_t c) {
            return (c >= L'a' && c <= L'z') && 
                   (c != L'a' && c != L'e' && c != L'i' && c != L'o' && c != L'u' && c != L'y');
        };

        if (isConsonant(raw[0]) && isConsonant(raw[1])) {
            bool validVietCluster = 
                (raw[0] == L'c' && raw[1] == L'h') ||
                (raw[0] == L'g' && raw[1] == L'h') ||
                (raw[0] == L'k' && raw[1] == L'h') ||
                (raw[0] == L'n' && (raw[1] == L'g' || raw[1] == L'h')) ||
                (raw[0] == L'p' && raw[1] == L'h') ||
                (raw[0] == L't' && (raw[1] == L'h' || raw[1] == L'r')) ||
                (raw[0] == L'd' && raw[1] == L'd'); // <--- B? SUNG NGO?I L? CHO CH? "�" T?I ��Y
            
            // N?u l� 2 ph? �m d?ng d?u nhung kh�ng n?m trong danh s�ch tr�n -> 100% English (vd: class, style, block)
            if (!validVietCluster) return true;
        }
        if (raw[0] == L'c' && (raw[1] == L'i' || raw[1] == L'e' || raw[1] == L'\u00EA' || raw[1] == L'y')) return true;
        if (raw[0] == L'k' && !(raw[1] == L'h' || raw[1] == L'i' || raw[1] == L'e' || raw[1] == L'\u00EA' || raw[1] == L'y')) return true;
        if (raw[0] == L'g' && (raw[1] == L'e' || raw[1] == L'\u00EA' || raw[1] == L'y')) return true;
        if (len >= 3 && raw[0] == L'g' && raw[1] == L'h') {
            if (raw[2] != L'i' && raw[2] != L'e' && raw[2] != L'\u00EA' && raw[2] != L'y') return true;
        }
        if (len >= 3 && raw[0] == L'n' && raw[1] == L'g' && raw[2] != L'h') {
            if (raw[2] == L'i' || raw[2] == L'e' || raw[2] == L'\u00EA' || raw[2] == L'y') return true;
        }
        if (len >= 4 && raw[0] == L'n' && raw[1] == L'g' && raw[2] == L'h') {
            if (raw[3] != L'i' && raw[3] != L'e' && raw[3] != L'\u00EA' && raw[3] != L'y') return true;
        }
    }

    // LEVEL 2: Structural Validator
    bool hasVowel = false;
    for (int i = 0; i < _textLen; i++) {
        if (CayData::IsVowel(_text[i])) { hasVowel = true; break; }
    }
    
    if (hasVowel) {
        wchar_t textLo[MAX_BUFFER];
        for (int i = 0; i < _textLen; i++) {
            textLo[i] = ToLowerViet(CayData::StripTone(_text[i]));
        }
        textLo[_textLen] = L'\0';
        
        if (!IsCompleteSyllable(textLo, _textLen)) return true;
    } else {
        if (_textLen >= 5) return true;
    }

    return false;
}

// ---------------------------------------------------------------------------
// FindTonePosition
//
// Returns the index in _text[] where the tone mark should be placed.
// ---------------------------------------------------------------------------
int TelexEngine::FindTonePosition() const {
    if (_textLen == 0) return -1;

    // Collect vowel positions.
    int first = -1, last = -1, count = 0;
    for (int i = 0; i < _textLen; i++) {
        if (CayData::IsVowel(_text[i])) {
            if (first == -1) first = i;
            last = i;
            count++;
        }
    }
    if (count == 0) return -1;
    if (count == 1) return first;

    // Does a consonant follow the last vowel?
    bool hasConsonantFinal = false;
    for (int i = last + 1; i < _textLen; i++) {
        if (IsAlpha(_text[i]) && !CayData::IsVowel(_text[i])) {
            hasConsonantFinal = true;
            break;
        }
    }

    // Helper: get plain ASCII base of a potentially toned/hooked vowel.
    auto baseVowel = [](wchar_t c) -> wchar_t {
        return ToLowerViet(CayData::StripAccent(CayData::StripTone(c)));
    };

    if (count == 2) {
        if (hasConsonantFinal) return last; // e.g. "tuấn", "điện"

        wchar_t v1 = baseVowel(_text[first]);
        wchar_t v2 = baseVowel(_text[last]);

        // oa, oe
        if (v1 == L'o' && (v2 == L'a' || v2 == L'e')) return last;
        // uê, uy, uơ
        if (v1 == L'u' && (v2 == L'e' || v2 == L'y' || v2 == L'o')) return last;
        // iê
        if (v1 == L'i' && v2 == L'e') return last;

        // qu + vowel (e.g. quá)
        if (v1 == L'u' && first > 0 && ToLowerViet(_text[first - 1]) == L'q') return last;
        // gi + vowel (e.g. già)
        if (v1 == L'i' && first > 0 && ToLowerViet(_text[first - 1]) == L'g') return last;

        // Default for open 2-vowel syllable: first vowel (e.g. rồi, mèo, đôi, bơi, múa)
        return first;
    }

    if (count >= 3) {
        if (hasConsonantFinal) return last; // e.g. "tuyến", "giường"

        wchar_t v1 = baseVowel(_text[first]);
        wchar_t v2 = baseVowel(_text[first + 1]);
        wchar_t v3 = baseVowel(_text[last]);

        // uyê (incomplete "nguyễ", "chuyế") -> tone on ê
        if (v1 == L'u' && v2 == L'y' && v3 == L'e') return last;
        // giuô, giươ (incomplete "giuộ", "giượ") -> tone on ô/ơ
        if (v1 == L'i' && v2 == L'u' && v3 == L'o') return last;

        // Default 3-vowel: middle vowel (e.g. người, ngoài, khuya)
        return first + 1;
    }

    return last;
}


// ---------------------------------------------------------------------------
// StripAllTones – rewrite _text[] removing any tone mark from every vowel.
// ---------------------------------------------------------------------------
void TelexEngine::StripAllTones() {
    for (int i = 0; i < _textLen; i++) {
        wchar_t stripped = CayData::StripTone(_text[i]);
        _text[i] = stripped;
    }
}

bool TelexEngine::ApplyDoubleKeys(wchar_t key) {
    wchar_t loKey = ToLowerViet(key);
    if (loKey != L'a' && loKey != L'e' && loKey != L'o' && loKey != L'd') return false;

    for (int j = _textLen - 1; j >= 0; j--) {
        wchar_t target = _text[j];
        wchar_t baseTarget = CayData::StripTone(target);
        wchar_t loBase = ToLowerViet(baseTarget);
        bool isUpper = (ToLowerViet(target) != target) || (target >= L'A' && target <= L'Z');

        int tone = 0;
        for (int t = 1; t <= 5; t++) {
            if (CayData::GetToneMark(baseTarget, t) == target || CayData::GetToneMark(ToLowerViet(baseTarget), t) == ToLowerViet(target)) {
                tone = t; break;
            }
        }

        // 1. Undo logic
        if (loKey == L'a' && (loBase == L'\u00e2' || loBase == L'\u0103')) {
            wchar_t newBase = isUpper ? L'A' : L'a';
            _text[j] = tone ? CayData::GetToneMark(newBase, tone) : newBase;
            if (_textLen < MAX_BUFFER - 1) { _text[_textLen++] = key; _text[_textLen] = L'\0'; }
 return true;
        }
        if (loKey == L'e' && loBase == L'\u00ea') {
            wchar_t newBase = isUpper ? L'E' : L'e';
            _text[j] = tone ? CayData::GetToneMark(newBase, tone) : newBase;
            if (_textLen < MAX_BUFFER - 1) { _text[_textLen++] = key; _text[_textLen] = L'\0'; }
 return true;
        }
        if (loKey == L'o' && (loBase == L'\u00f4' || loBase == L'\u01a1')) {
            wchar_t newBase = isUpper ? L'O' : L'o';
            _text[j] = tone ? CayData::GetToneMark(newBase, tone) : newBase;
            if (_textLen < MAX_BUFFER - 1) { _text[_textLen++] = key; _text[_textLen] = L'\0'; }
 return true;
        }
        if (loKey == L'd' && loBase == L'\u0111') {
            wchar_t newBase = isUpper ? L'D' : L'd';
            _text[j] = tone ? CayData::GetToneMark(newBase, tone) : newBase;
            if (_textLen < MAX_BUFFER - 1) { _text[_textLen++] = key; _text[_textLen] = L'\0'; }
 return true;
        }

        // 2. Apply logic
        if (loKey == L'a' && loBase == L'a') {
            wchar_t newBase = isUpper ? L'\u00C2' : L'\u00E2';
            _text[j] = tone ? CayData::GetToneMark(newBase, tone) : newBase;
 return true;
        }
        if (loKey == L'e' && loBase == L'e') {
            wchar_t newBase = isUpper ? L'\u00CA' : L'\u00EA';
            _text[j] = tone ? CayData::GetToneMark(newBase, tone) : newBase;
 return true;
        }
        if (loKey == L'o' && loBase == L'o') {
            wchar_t newBase = isUpper ? L'\u00D4' : L'\u00F4';
            _text[j] = tone ? CayData::GetToneMark(newBase, tone) : newBase;
 return true;
        }
        if (loKey == L'd' && loBase == L'd') {
            wchar_t newBase = isUpper ? L'\u0110' : L'\u0111';
            _text[j] = tone ? CayData::GetToneMark(newBase, tone) : newBase;
 return true;
        }
        
        if (!CayData::IsVowel(loBase) && loBase != L'd' && loBase != L'\u0111') {
            break;
        }
    }
    return false;
}

bool TelexEngine::ApplyHookKeys(wchar_t key) {
    if (ToLowerViet(key) != L'w') return false;

    for (int j = _textLen - 1; j >= 0; j--) {
        wchar_t target = _text[j];
        wchar_t baseTarget = CayData::StripTone(target);
        wchar_t loBase = ToLowerViet(baseTarget);
        bool isUpper = (ToLowerViet(target) != target) || (target >= L'A' && target <= L'Z');

        int tone = 0;
        for (int t = 1; t <= 5; t++) {
            if (CayData::GetToneMark(baseTarget, t) == target || CayData::GetToneMark(ToLowerViet(baseTarget), t) == ToLowerViet(target)) {
                tone = t; break;
            }
        }

        // 1. Undo logic
        if (loBase == L'\u0103' || loBase == L'\u01a1' || loBase == L'\u01b0') {
            if (loBase == L'\u01a1' && j > 0 && ToLowerViet(CayData::StripTone(_text[j-1])) == L'\u01b0') {
                wchar_t prevBase = CayData::StripTone(_text[j-1]);
                bool prevUpper = (ToLowerViet(prevBase) != prevBase) || (prevBase >= L'A' && prevBase <= L'Z');
                int prevTone = 0;
                for (int t = 1; t <= 5; t++) {
                    if (CayData::GetToneMark(prevBase, t) == _text[j-1] || CayData::GetToneMark(ToLowerViet(prevBase), t) == ToLowerViet(_text[j-1])) {
                        prevTone = t; break;
                    }
                }
                wchar_t newPrevBase = prevUpper ? L'U' : L'u';
                wchar_t newCurrBase = isUpper ? L'O' : L'o';
                _text[j-1] = prevTone ? CayData::GetToneMark(newPrevBase, prevTone) : newPrevBase;
                _text[j]   = tone ? CayData::GetToneMark(newCurrBase, tone) : newCurrBase;
            } else {
                wchar_t newBase = (loBase == L'\u0103') ? (isUpper ? L'A' : L'a') :
                                  (loBase == L'\u01a1') ? (isUpper ? L'O' : L'o') : (isUpper ? L'U' : L'u');
                _text[j] = tone ? CayData::GetToneMark(newBase, tone) : newBase;
            }
            if (_textLen < MAX_BUFFER - 1) { _text[_textLen++] = key; _text[_textLen] = L'\0'; }
 return true;
        }

        // 2. Apply logic
        if (loBase == L'o' && j > 0 && ToLowerViet(CayData::StripTone(_text[j-1])) == L'u') {
            wchar_t prevBase = CayData::StripTone(_text[j-1]);
            bool prevUpper = (ToLowerViet(prevBase) != prevBase) || (prevBase >= L'A' && prevBase <= L'Z');
            int prevTone = 0;
            for (int t = 1; t <= 5; t++) {
                if (CayData::GetToneMark(prevBase, t) == _text[j-1] || CayData::GetToneMark(ToLowerViet(prevBase), t) == ToLowerViet(_text[j-1])) {
                    prevTone = t; break;
                }
            }
            wchar_t newPrevBase = prevUpper ? L'\u01AF' : L'\u01b0'; // Ư/ư
            wchar_t newCurrBase = isUpper ? L'\u01A0' : L'\u01a1'; // Ơ/ơ
            _text[j-1] = prevTone ? CayData::GetToneMark(newPrevBase, prevTone) : newPrevBase;
            _text[j]   = tone ? CayData::GetToneMark(newCurrBase, tone) : newCurrBase;
 return true;
        }
        if (loBase == L'a' && j > 0 && ToLowerViet(CayData::StripTone(_text[j-1])) == L'u') {
            wchar_t prevBase = CayData::StripTone(_text[j-1]);
            bool prevUpper = (ToLowerViet(prevBase) != prevBase) || (prevBase >= L'A' && prevBase <= L'Z');
            int prevTone = 0;
            for (int t = 1; t <= 5; t++) {
                if (CayData::GetToneMark(prevBase, t) == _text[j-1] || CayData::GetToneMark(ToLowerViet(prevBase), t) == ToLowerViet(_text[j-1])) {
                    prevTone = t; break;
                }
            }
            wchar_t newPrevBase = prevUpper ? L'\u01AF' : L'\u01b0'; // Ư/ư
            _text[j-1] = prevTone ? CayData::GetToneMark(newPrevBase, prevTone) : newPrevBase;
 return true;
        }
        
        wchar_t hookRule = CayData::GetHookRule(baseTarget);
        if (hookRule != L'\0') {
            _text[j] = tone ? CayData::GetToneMark(hookRule, tone) : hookRule;
 return true;
        }
        
        if (!CayData::IsVowel(loBase)) {
            continue;
        }
    }
    return false;
}

// ---------------------------------------------------------------------------
// ApplyToneMarks
//
// Applies or changes the tone mark on the appropriate vowel of _text[].
// If the same tone is applied twice, removes it (toggle).
// ---------------------------------------------------------------------------
bool TelexEngine::ApplyToneMarks(int toneIndex) {
    if (_textLen == 0) return false;

    int currentTone = 0;
    int tonePos = -1;

    // 1. Identify if the word currently has a tone and where it is
    for (int i = 0; i < _textLen; i++) {
        wchar_t c = _text[i];
        wchar_t base = CayData::StripTone(c);
        if (base != c) {
            tonePos = i;
            for (int t = 1; t <= 5; t++) {
                if (CayData::GetToneMark(base, t) == c || 
                    CayData::GetToneMark(ToLowerViet(base), t) == ToLowerViet(c)) {
                    currentTone = t;
                    break;
                }
            }
            break;
        }
    }

    // 2. Handle 'z' key (toneIndex == 0)
    if (toneIndex == 0) {
        if (currentTone > 0) {
            _text[tonePos] = CayData::StripTone(_text[tonePos]); // Remove tone
            _toneIndex = -1;

            return true; // Consumed 'z', do not append it
        }
        return false; // No tone to remove, return false so 'z' gets appended normally
    }

    // 3. Handle double-typing the SAME tone key (Undo tone and append raw key)
    if (currentTone == toneIndex) {
        _text[tonePos] = CayData::StripTone(_text[tonePos]); // Remove tone
        _toneIndex = -1;

        return false; // Return false so the raw tone key (e.g., 's') gets appended
    }

    // 4. Apply or replace tone
    int targetPos = (tonePos >= 0) ? tonePos : FindTonePosition();
    if (targetPos >= 0) {
        wchar_t originalChar = _text[targetPos];
        // Kiểm tra xem chữ gốc có phải là chữ IN HOA không
        bool isUpper = (originalChar == ToUpperViet(originalChar) && originalChar != ToLowerViet(originalChar));

        wchar_t base = CayData::StripTone(originalChar);
        wchar_t baseLo = ToLowerViet(base); // Đưa về chữ thường để lấy dấu trong thư viện

        wchar_t tonedLo = CayData::GetToneMark(baseLo, toneIndex);
        if (tonedLo != L'\0') {
            // Nâng lên lại IN HOA nếu cần
            _text[targetPos] = isUpper ? ToUpperViet(tonedLo) : tonedLo;
            _toneIndex = toneIndex;

            return true;
        }
    }

    return false;
}

// ---------------------------------------------------------------------------
// OnKeyDown – main entry point
// ---------------------------------------------------------------------------
void TelexEngine::OnKeyDown(Cay::KeyEvent& e) {
    // 0. Reset state on Navigation or Control keys to prevent buffer desync
    if (e.keyCode == Cay::KeyCode::Enter || e.keyCode == Cay::KeyCode::Tab || e.keyCode == Cay::KeyCode::Escape ||
       (e.keyCode >= Cay::KeyCode::PageUp && e.keyCode <= Cay::KeyCode::Down)) { 
        // Cay::KeyCode::PageUp (33) to Cay::KeyCode::Down (40) covers PageUp, PageDown, End, Home, Left, Up, Right, Down
        ResetState();
        _canRestore = false;
        return;
    }

    Cay::KeyCode vk = e.keyCode;

    // -----------------------------------------------------------------------
    // 1. Non-alpha keys that reset or terminate the current word.
    // -----------------------------------------------------------------------
    switch (vk) {
    case Cay::KeyCode::Backspace:
        if (_bufferCount == 0 && _canRestore) {
            // Restore state
            _bufferCount = _savedBufferCount;
            for (int i = 0; i < _bufferCount; i++) _buffer[i] = _savedBuffer[i];
            
            _textLen = _savedTextLen;
            for (int i = 0; i < _textLen; i++) _text[i] = _savedText[i];
            
            _toneIndex = _savedToneIndex;
            
            // Sync output state with restored text (so UpdateScreen works properly on next keystroke)
            _lastOutputLen = _savedTextLen;
            for (int i = 0; i < _savedTextLen; i++) _lastOutput[i] = _savedText[i];
            _lastOutput[_lastOutputLen] = L'\0';
            
            _canRestore = false;
            // DO NOT set e.handled = true here. We let the OS physically delete the Space character.
            return;
        }

        if (_bufferCount > 0) {
            e.handled = true; // suppress the raw backspace
            _canRestore = false;
            // Remove last character from our text.
            if (_textLen > 0) _textLen--;
            if (_bufferCount > 0) _bufferCount--;
            _text[_textLen] = L'\0';

            // Re-derive tone index.
            _toneIndex = -1;
            for (int i = 0; i < _bufferCount; i++) {
                int ti = CayData::GetToneIndex(_buffer[i].raw);
                if (ti >= 0) _toneIndex = ti;
            }

            if (_bufferCount == 0) {
                ResetState();
                // Send one backspace to clear the last remaining displayed char.
                if (OnInjectText) OnInjectText(1, nullptr, 0);
            } else {
                UpdateScreen(_text, _textLen); // Automatically calculates and sends 1 Backspace
            }
        }
        return;

    case Cay::KeyCode::Escape:
        ResetFull();
        return;

    case Cay::KeyCode::Enter:
    case Cay::KeyCode::Tab:
    case Cay::KeyCode::Space:
        if (_bufferCount > 0) CommitWord();
        else ResetFull();
        return;

    case Cay::KeyCode::Left: case Cay::KeyCode::Right: case Cay::KeyCode::Up: case Cay::KeyCode::Down:
    case Cay::KeyCode::Home: case Cay::KeyCode::End:  case Cay::KeyCode::PageUp: case Cay::KeyCode::PageDown:
    case Cay::KeyCode::Delete:
        ResetFull();
        return;
    }

    // -----------------------------------------------------------------------
    // 2. Only process printable ASCII alpha characters.
    // -----------------------------------------------------------------------
    if (vk < Cay::KeyCode::KeyA || vk > Cay::KeyCode::KeyZ) {
        // Non-alpha printable (digits, punctuation) – commit word.
        if (_bufferCount > 0) CommitWord();
        else ResetFull();
        return;
    }

    // Determine the actual character pressed (respecting Shift).
    wchar_t ch = e.character;
    wchar_t lo = ToLowerViet(ch);

    // -----------------------------------------------------------------------
    // 3. Guard: buffer overflow -> fall through as plain text.
    // -----------------------------------------------------------------------
    if (_bufferCount >= MAX_BUFFER - 1 || _textLen >= MAX_BUFFER - 1) {
        ResetFull();
        // Do NOT set e.handled = true here, let the OS handle the keystroke naturally
        return;
    }

    // -----------------------------------------------------------------------
    // 4. Record raw keystroke.
    // -----------------------------------------------------------------------
    if (_bufferCount == 0) _canRestore = false;

    _buffer[_bufferCount].raw    = ch;
    _buffer[_bufferCount].output = ch; // will be updated after transformation
    _bufferCount++;

    // -----------------------------------------------------------------------
    // 5. Try modifier keys (bypass = english mode check).
    // -----------------------------------------------------------------------
    bool bypass = ShouldBypassWord();
    bool appliedModifier = false;

    // 5a. Double-key circumflex / stroke (aa oo ee dd).
    if (!bypass && (lo == L'a' || lo == L'e' || lo == L'o' || lo == L'd')) {
        if (_textLen > 0 && ApplyDoubleKeys(ch)) {
            appliedModifier = true;
        }
    }

    // 5b. Hook key 'w'.
    if (!bypass && !appliedModifier && lo == L'w' && _textLen > 0) {
        if (ApplyHookKeys(ch)) {
            appliedModifier = true;
        }
    }

    // 5c. Tone mark keys (s f r x j z).
    if (!bypass && !appliedModifier) {
        int ti = CayData::GetToneIndex(lo);
        if (ti >= 0 && _textLen > 0) {
            if (ApplyToneMarks(ti)) {
                appliedModifier = true;
            }
        }
    }

    // -----------------------------------------------------------------------
    // 6. Plain character – append to text buffer.
    // -----------------------------------------------------------------------
    if (!appliedModifier) {
        if (_textLen < MAX_BUFFER - 1) {
            _text[_textLen++] = ch;
            _text[_textLen]   = L'\0';
        }
    }

    // -----------------------------------------------------------------------
    // 7. Sync screen (Centralized Rendering)
    // -----------------------------------------------------------------------
    e.handled = true;
    UpdateScreen(_text, _textLen);
}

// ---------------------------------------------------------------------------
// OnKeyUp – currently unused; reserved for future modifier tracking.
// ---------------------------------------------------------------------------
void TelexEngine::OnKeyUp(Cay::KeyEvent& e) {
    // No-op for now.
    (void)e;
}

} // namespace Cay





