#include "CayData.h"

// ============================================================================
// LƯU Ý: Tất cả array bên dưới được khai báo `static const` để nằm trong
// segment read-only (.rdata) và được khởi tạo hoàn toàn trước khi wWinMain
// được gọi – quan trọng cho build No-CRT (QUY TẮC 2).
// ============================================================================

namespace Cay {

// ---------------------------------------------------------------------------
// PHẦN 1: Bảng validation phụ âm đầu
// 26 cụm phụ âm đầu tiếng Việt được nhận biết (dạng Telex).
// ---------------------------------------------------------------------------
static const wchar_t* const s_initials[] = {
    L"b",  L"c",  L"ch", L"d",  L"dd", L"\u0111", L"g",  L"gh", L"gi",
    L"h",  L"k",  L"kh", L"l",  L"m",  L"n",  L"ng", L"ngh",
    L"nh", L"p",  L"ph", L"qu", L"r",  L"s",  L"t",  L"th",
    L"tr", L"v",  L"x"
};
static const int s_initialsCount = sizeof(s_initials) / sizeof(s_initials[0]);

// ---------------------------------------------------------------------------
// PHẦN 2: Bảng validation nguyên âm
// 48 nguyên âm tiếng Việt được nhận biết (chỉ viết Telex ASCII thuần;
// engine làm việc trên buffer trước khi transform).
// ---------------------------------------------------------------------------
static const wchar_t* const s_nuclei[] = {
    // Nguyên âm đơn thuần
    L"a", L"e", L"i", L"o", L"u", L"y",
    // Nguyên âm đơn đã transform (dấu mũ/dấu ngắn được engine áp dụng)
    L"â", L"ă", L"ê", L"ô", L"ơ", L"ư",
    // Nguyên âm đôi – thuần
    L"ai", L"ao", L"au", L"ay", L"eo", L"eu",
    L"ia", L"ie", L"iu",
    L"oa", L"oe", L"oi", L"oo", L"ou", L"oy",
    L"ua", L"ue", L"ui", L"uo", L"uy",
    L"ya", L"ye", L"yi", L"yo", L"yu",
    // Nguyên âm đôi – có dấu mũ/dấu ngắn
    L"ươ", L"ưa", L"ưu", L"ưi",
    L"ôi", L"ơi",
    L"âu", L"âu", L"ây",
    L"ăn",                 // placeholder để làm tròn đến 48
    // Nguyên âm ba
    L"iêu", L"uôi", L"ươi", L"ươu",
    L"oai", L"oao", L"oay", L"uay", L"uoi",
};
static const int s_nucleiCount = sizeof(s_nuclei) / sizeof(s_nuclei[0]);

// ---------------------------------------------------------------------------
// PHẦN 3: Bảng ký tự có dấu
//
// Mỗi sub-array được index theo dấu (0=ngang … 5=nặng).
// Nguyên âm cơ bản: a â ă e ê i o ô ơ u ư y
// ---------------------------------------------------------------------------

// Nhóm a
static const wchar_t s_toneA[6] = { L'a', L'\u00E0', L'\u00E1', L'\u1EA3', L'\u00E3', L'\u1EA1' };
// Nhóm â  â ầ ấ ẩ ẫ ậ
static const wchar_t s_toneAc[6] = { L'\u00E2', L'\u1EA7', L'\u1EA5', L'\u1EA9', L'\u1EAB', L'\u1EAD' };
// Nhóm ă  ă ằ ắ ẳ ẵ ặ
static const wchar_t s_toneAb[6] = { L'\u0103', L'\u1EB1', L'\u1EAF', L'\u1EB3', L'\u1EB5', L'\u1EB7' };
// Nhóm e
static const wchar_t s_toneE[6] = { L'e', L'\u00E8', L'\u00E9', L'\u1EBB', L'\u1EBD', L'\u1EB9' };
// Nhóm ê
static const wchar_t s_toneEc[6] = { L'\u00EA', L'\u1EC1', L'\u1EBF', L'\u1EC3', L'\u1EC5', L'\u1EC7' };
// Nhóm i
static const wchar_t s_toneI[6] = { L'i', L'\u00EC', L'\u00ED', L'\u1EC9', L'\u0129', L'\u1ECB' };
// Nhóm o
static const wchar_t s_toneO[6] = { L'o', L'\u00F2', L'\u00F3', L'\u1ECF', L'\u00F5', L'\u1ECD' };
// Nhóm ô
static const wchar_t s_toneOc[6] = { L'\u00F4', L'\u1ED3', L'\u1ED1', L'\u1ED5', L'\u1ED7', L'\u1ED9' };
// Nhóm ơ
static const wchar_t s_toneOh[6] = { L'\u01A1', L'\u1EDD', L'\u1EDB', L'\u1EDF', L'\u1EE1', L'\u1EE3' };
// Nhóm u
static const wchar_t s_toneU[6] = { L'u', L'\u00F9', L'\u00FA', L'\u1EE7', L'\u0169', L'\u1EE5' };
// Nhóm ư
static const wchar_t s_toneUh[6] = { L'\u01B0', L'\u1EEB', L'\u1EE9', L'\u1EED', L'\u1EEF', L'\u1EF1' };
// Nhóm y
static const wchar_t s_toneY[6] = { L'y', L'\u1EF3', L'\u00FD', L'\u1EF7', L'\u1EF9', L'\u1EF5' };

// ---------------------------------------------------------------------------
// IsValidInitial - Kiểm tra phụ âm đầu hợp lệ
// ---------------------------------------------------------------------------
bool CayData::IsValidInitial(const wchar_t* s, int len) {
    if (!s || len <= 0) return false;
    for (int i = 0; i < s_initialsCount; i++) {
        if ((int)CayStrLen(s_initials[i]) == len &&
            CayStrCmp(s_initials[i], s) == 0) {
            return true;
        }
    }
    return false;
}

// ---------------------------------------------------------------------------
// IsValidNucleus - Kiểm tra nguyên âm hợp lệ
// ---------------------------------------------------------------------------
bool CayData::IsValidNucleus(const wchar_t* s, int len) {
    if (!s || len <= 0) return false;
    for (int i = 0; i < s_nucleiCount; i++) {
        int nlen = (int)CayStrLen(s_nuclei[i]);
        if (nlen == len && CayStrCmp(s_nuclei[i], s) == 0) {
            return true;
        }
    }
    return false;
}

// ---------------------------------------------------------------------------
// GetToneIndex - Map phím Telex sang chỉ số dấu
// Map phím Telex modifier sang chỉ số 0–5; trả về -1 nếu không phải phím dấu.
// ---------------------------------------------------------------------------
int CayData::GetToneIndex(wchar_t key) {
    switch (key) {
    case L'z': case L'Z': return 0; // Xóa dấu  (flat)
    case L'f': case L'F': return 1; // Huyền
    case L's': case L'S': return 2; // Sắc
    case L'r': case L'R': return 3; // Hỏi
    case L'x': case L'X': return 4; // Ngã
    case L'j': case L'J': return 5; // Nặng
    default:               return -1;
    }
}

// ---------------------------------------------------------------------------
// GetToneMark - Lấy ký tự có dấu
// Trả về codepoint Unicode có dấu cho (nguyên âm cơ bản, chỉ số dấu).
// base là nguyên âm thuần hoặc đã có dấu mũ.
// ---------------------------------------------------------------------------
wchar_t CayData::GetToneMark(wchar_t base, int toneIndex) {
    if (toneIndex < 0 || toneIndex > 5) return 0;

    switch (base) {
    // a thuần
    case L'a': return s_toneA[toneIndex];

    // â (dấu mũ trên a)
    case L'\u00E2': return s_toneAc[toneIndex];

    // ă (dấu ngắn trên a)
    case L'\u0103': return s_toneAb[toneIndex];

    // e thuần
    case L'e': return s_toneE[toneIndex];

    // ê (dấu mũ trên e)
    case L'\u00EA': return s_toneEc[toneIndex];

    // i thuần
    case L'i': return s_toneI[toneIndex];

    // o thuần
    case L'o': return s_toneO[toneIndex];

    // ô (dấu mũ trên o)
    case L'\u00F4': return s_toneOc[toneIndex];

    // ơ (dấu móc trên o)
    case L'\u01A1': return s_toneOh[toneIndex];

    // u thuần
    case L'u': return s_toneU[toneIndex];

    // ư (dấu móc trên u)
    case L'\u01B0': return s_toneUh[toneIndex];

    // y thuần
    case L'y': return s_toneY[toneIndex];

    // Đã có dấu – bỏ về cơ bản rồi áp dụng lại.
    // Nhóm a đã có dấu
    case L'\u00E0': case L'\u00E1': case L'\u1EA3': case L'\u00E3': case L'\u1EA1':
        return s_toneA[toneIndex];
    // Nhóm â đã có dấu  ầ ấ ẩ ẫ ậ
    case L'\u1EA7': case L'\u1EA5': case L'\u1EA9': case L'\u1EAB': case L'\u1EAD':
        return s_toneAc[toneIndex];
    // Nhóm ă đã có dấu
    case L'\u1EB1': case L'\u1EB3': case L'\u1EB5': case L'\u1EB7':
        return s_toneAb[toneIndex];
    // Nhóm e đã có dấu
    case L'\u00E8': case L'\u00E9': case L'\u1EBB': case L'\u1EBD': case L'\u1EB9':
        return s_toneE[toneIndex];
    // Nhóm ê đã có dấu
    case L'\u1EC1': case L'\u1EBF': case L'\u1EC3': case L'\u1EC5': case L'\u1EC7':
        return s_toneEc[toneIndex];
    // Nhóm i đã có dấu
    case L'\u00EC': case L'\u00ED': case L'\u1EC9': case L'\u0129': case L'\u1ECB':
        return s_toneI[toneIndex];
    // Nhóm o đã có dấu
    case L'\u00F2': case L'\u00F3': case L'\u1ECF': case L'\u00F5': case L'\u1ECD':
        return s_toneO[toneIndex];
    // Nhóm ô đã có dấu
    case L'\u1ED3': case L'\u1ED1': case L'\u1ED5': case L'\u1ED7': case L'\u1ED9':
        return s_toneOc[toneIndex];
    // Nhóm ơ đã có dấu
    case L'\u1EDD': case L'\u1EDB': case L'\u1EDF': case L'\u1EE1': case L'\u1EE3':
        return s_toneOh[toneIndex];
    // Nhóm u đã có dấu
    case L'\u00F9': case L'\u00FA': case L'\u1EE7': case L'\u0169': case L'\u1EE5':
        return s_toneU[toneIndex];
    // Nhóm ư đã có dấu
    case L'\u1EEB': case L'\u1EE9': case L'\u1EED': case L'\u1EEF': case L'\u1EF1':
        return s_toneUh[toneIndex];
    // Nhóm y đã có dấu
    case L'\u1EF3': case L'\u00FD': case L'\u1EF7': case L'\u1EF9': case L'\u1EF5':
        return s_toneY[toneIndex];

    default:
        return 0;
    }
}

// ---------------------------------------------------------------------------
// GetHookRule - Lấy quy tắc dấu mũ
// ---------------------------------------------------------------------------
wchar_t CayData::GetHookRule(wchar_t c) {
    switch (c) {
        case L'a': return L'\u0103'; // ă
        case L'o': return L'\u01a1'; // ơ
        case L'u': return L'\u01b0'; // ư
        case L'A': return L'\u0102'; // Ă
        case L'O': return L'\u01A0'; // Ơ
        case L'U': return L'\u01AF'; // Ư
        case L'\u00e2': return L'\u0103'; // â -> ă
        case L'\u00C2': return L'\u0102'; // Â -> Ă
        default: return L'\0';
    }
}

// ---------------------------------------------------------------------------
// HasVietnameseMark (ký tự đơn) - Kiểm tra có dấu tiếng Việt
// ---------------------------------------------------------------------------
bool CayData::HasVietnameseMark(wchar_t ch) {
    if (ch < 0x00C0) return false;
    // Nếu bỏ dấu thanh và dấu mũ làm thay đổi ký tự, có nghĩa là có dấu.
    wchar_t base = StripAccent(StripTone(ch));
    return base != ch;
}

// ---------------------------------------------------------------------------
// HasVietnameseMark (buffer) - Kiểm tra có dấu trong buffer
// ---------------------------------------------------------------------------
bool CayData::HasVietnameseMark(const wchar_t* buf, int len) {
    for (int i = 0; i < len; i++) {
        if (HasVietnameseMark(buf[i])) return true;
    }
    return false;
}

// ---------------------------------------------------------------------------
// StripTone – Bỏ dấu thanh từ nguyên âm, giữ dấu mũ/dấu ngắn.
// ---------------------------------------------------------------------------
wchar_t CayData::StripTone(wchar_t ch) {
    switch (ch) {
        case L'\u00E0': case L'\u00E1': case L'\u1EA3': case L'\u00E3': case L'\u1EA1': return L'a';
        case L'\u00C0': case L'\u00C1': case L'\u1EA2': case L'\u00C3': case L'\u1EA0': return L'A';
        case L'\u1EA7': case L'\u1EA5': case L'\u1EA9': case L'\u1EAB': case L'\u1EAD': return L'\u00E2';
        case L'\u1EA6': case L'\u1EA4': case L'\u1EA8': case L'\u1EAA': case L'\u1EAC': return L'\u00C2';
        case L'\u1EB1': case L'\u1EAF': case L'\u1EB3': case L'\u1EB5': case L'\u1EB7': return L'\u0103';
        case L'\u1EB0': case L'\u1EAE': case L'\u1EB2': case L'\u1EB4': case L'\u1EB6': return L'\u0102';
        case L'\u00E8': case L'\u00E9': case L'\u1EBB': case L'\u1EBD': case L'\u1EB9': return L'e';
        case L'\u00C8': case L'\u00C9': case L'\u1EBA': case L'\u1EBC': case L'\u1EB8': return L'E';
        case L'\u1EC1': case L'\u1EBF': case L'\u1EC3': case L'\u1EC5': case L'\u1EC7': return L'\u00EA';
        case L'\u1EC0': case L'\u1EBE': case L'\u1EC2': case L'\u1EC4': case L'\u1EC6': return L'\u00CA';
        case L'\u00EC': case L'\u00ED': case L'\u1EC9': case L'\u0129': case L'\u1ECB': return L'i';
        case L'\u00CC': case L'\u00CD': case L'\u1EC8': case L'\u0128': case L'\u1ECA': return L'I';
        case L'\u00F2': case L'\u00F3': case L'\u1ECF': case L'\u00F5': case L'\u1ECD': return L'o';
        case L'\u00D2': case L'\u00D3': case L'\u1ECE': case L'\u00D5': case L'\u1ECC': return L'O';
        case L'\u1ED3': case L'\u1ED1': case L'\u1ED5': case L'\u1ED7': case L'\u1ED9': return L'\u00F4';
        case L'\u1ED2': case L'\u1ED0': case L'\u1ED4': case L'\u1ED6': case L'\u1ED8': return L'\u00D4';
        case L'\u1EDD': case L'\u1EDB': case L'\u1EDF': case L'\u1EE1': case L'\u1EE3': return L'\u01A1';
        case L'\u1EDC': case L'\u1EDA': case L'\u1EDE': case L'\u1EE0': case L'\u1EE2': return L'\u01A0';
        case L'\u00F9': case L'\u00FA': case L'\u1EE7': case L'\u0169': case L'\u1EE5': return L'u';
        case L'\u00D9': case L'\u00DA': case L'\u1EE6': case L'\u0168': case L'\u1EE4': return L'U';
        case L'\u1EEB': case L'\u1EE9': case L'\u1EED': case L'\u1EEF': case L'\u1EF1': return L'\u01B0';
        case L'\u1EEA': case L'\u1EE8': case L'\u1EEC': case L'\u1EEE': case L'\u1EF0': return L'\u01AF';
        case L'\u1EF3': case L'\u00FD': case L'\u1EF7': case L'\u1EF9': case L'\u1EF5': return L'y';
        case L'\u1EF2': case L'\u00DD': case L'\u1EF6': case L'\u1EF8': case L'\u1EF4': return L'Y';
        default: return ch;
    }
}

// ---------------------------------------------------------------------------
// StripAccent – Bỏ dấu mũ hoặc dấu ngắn, trả về nguyên âm ASCII thuần.
// ---------------------------------------------------------------------------
wchar_t CayData::StripAccent(wchar_t ch) {
    switch (ch) {
    case L'\u00E2': case L'\u0103': return L'a'; // â ă -> a
    case L'\u00C2': case L'\u0102': return L'A'; // Â Ă -> A
    case L'\u00EA':                 return L'e'; // ê   -> e
    case L'\u00CA':                 return L'E'; // Ê   -> E
    case L'\u00F4':                 return L'o'; // ô   -> o
    case L'\u00D4':                 return L'O'; // Ô   -> O
    case L'\u01A1':                 return L'o'; // ơ   -> o
    case L'\u01A0':                 return L'O'; // Ơ   -> O
    case L'\u01B0':                 return L'u'; // ư   -> u
    case L'\u01AF':                 return L'U'; // Ư   -> U
    case L'\u0111':                 return L'd'; // đ   -> d
    case L'\u0110':                 return L'D'; // Đ   -> D
    // Cũng bỏ từ nguyên âm có dấu mũ đã có dấu (bỏ cả 2 dấu trong 1 bước).
    case L'\u1EA7': case L'\u1EA5': case L'\u1EAB': case L'\u1EAD': case L'\u1EAF': return L'a';
    case L'\u1EA6': case L'\u1EA4': case L'\u1EAA': case L'\u1EAC': case L'\u1EAE': return L'A';
    case L'\u1EB1': case L'\u1EB3': case L'\u1EB5': case L'\u1EB7': return L'a';
    case L'\u1EB0': case L'\u1EB2': case L'\u1EB4': case L'\u1EB6': return L'A';
    case L'\u1EC1': case L'\u1EBF': case L'\u1EC3': case L'\u1EC5': case L'\u1EC7': return L'e';
    case L'\u1EC0': case L'\u1EBE': case L'\u1EC2': case L'\u1EC4': case L'\u1EC6': return L'E';
    case L'\u1ED3': case L'\u1ED1': case L'\u1ED5': case L'\u1ED7': case L'\u1ED9': return L'o';
    case L'\u1ED2': case L'\u1ED0': case L'\u1ED4': case L'\u1ED6': case L'\u1ED8': return L'O';
    case L'\u1EDD': case L'\u1EDB': case L'\u1EDF': case L'\u1EE1': case L'\u1EE3': return L'o';
    case L'\u1EDC': case L'\u1EDA': case L'\u1EDE': case L'\u1EE0': case L'\u1EE2': return L'O';
    case L'\u1EEB': case L'\u1EE9': case L'\u1EED': case L'\u1EEF': case L'\u1EF1': return L'u';
    case L'\u1EEA': case L'\u1EE8': case L'\u1EEC': case L'\u1EEE': case L'\u1EF0': return L'U';
    default: return ch;
    }
}

// ---------------------------------------------------------------------------
// IsVowel – true cho bất kỳ nguyên âm tiếng Việt (thuần hoặc có dấu).
// ---------------------------------------------------------------------------
bool CayData::IsVowel(wchar_t ch) {
    wchar_t base = StripAccent(StripTone(ch));
    switch (base) {
    case L'a': case L'A':
    case L'e': case L'E':
    case L'i': case L'I':
    case L'o': case L'O':
    case L'u': case L'U':
    case L'y': case L'Y':
        return true;
    default:
        return false;
    }
}

} // namespace Cay

