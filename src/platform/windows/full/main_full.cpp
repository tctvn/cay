
#include <windows.h>
#include <shellapi.h>
#include <winreg.h>
#include "CayData.h"
#include "CayEngine.h"
#include "KeyboardHookManager.h"
#include "InputInjector.h"
#include "resource.h"
#include "ConfigManager.h"

#pragma comment(linker, "/NODEFAULTLIB")
#pragma comment(linker, "/OPT:REF")
#pragma comment(linker, "/OPT:ICF")
#pragma comment(linker, "/MERGE:.rdata=.text")
#pragma comment(linker, "/MERGE:.pdata=.text")
#pragma comment(lib, "kernel32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "advapi32.lib")

#define WM_TRAYICON (WM_USER + 1)

#define IDM_TOGGLE 1001
#define IDM_AUTOSTART 1002
#define IDM_ABOUT 1003
#define IDM_EXIT 1004
#define IDM_SETTINGS 1005

HWND g_hWnd = nullptr;
HWND g_hDlg = nullptr;
HINSTANCE g_hInstance = nullptr;
NOTIFYICONDATAW g_nid = { 0 };
Cay::TelexEngine g_engine;
CayIME::InputHookManager* g_hookManager = nullptr;

bool g_enabled = true;
HICON g_iconOn = nullptr;
HICON g_iconOff = nullptr;

ConfigData g_config;
bool g_capturingHotkey = false;

// Forward declarations
void Toggle();

// Macro lookup callback
const wchar_t* MacroLookupCallback(const wchar_t* input) {
    if (!input) return nullptr;
    auto it = g_config.macros.find(input);
    if (it != g_config.macros.end()) {
        return it->second.c_str();
    }
    return nullptr;
}

// Refresh Macro ListBox
void RefreshMacroList(HWND hDlg) {
    HWND hList = GetDlgItem(hDlg, IDC_LST_MACROS);
    SendMessage(hList, LB_RESETCONTENT, 0, 0);
    for (const auto& pair : g_config.macros) {
        std::wstring item = pair.first + L" \x2192 " + pair.second; // Mũi tên ->
        SendMessageW(hList, LB_ADDSTRING, 0, (LPARAM)item.c_str());
    }
}

// Format Hotkey String
std::wstring GetHotkeyString() {
    std::wstring s;
    if (g_config.ctrl) s += L"Ctrl + ";
    if (g_config.shift) s += L"Shift + ";
    if (g_config.alt) s += L"Alt + ";
    if (g_config.win) s += L"Win + ";
    if (g_config.vkCode != 0 && g_config.vkCode != VK_CONTROL && g_config.vkCode != VK_LCONTROL && g_config.vkCode != VK_RCONTROL &&
        g_config.vkCode != VK_SHIFT && g_config.vkCode != VK_LSHIFT && g_config.vkCode != VK_RSHIFT &&
        g_config.vkCode != VK_MENU && g_config.vkCode != VK_LMENU && g_config.vkCode != VK_RMENU &&
        g_config.vkCode != VK_LWIN && g_config.vkCode != VK_RWIN) {
        wchar_t name[64];
        if (GetKeyNameTextW(MapVirtualKeyW(g_config.vkCode, MAPVK_VK_TO_VSC) << 16, name, 64) > 0) {
            s += name;
        } else {
            s += L"Key(" + std::to_wstring(g_config.vkCode) + L")";
        }
    } else {
        if (!s.empty()) s.pop_back(), s.pop_back(), s.pop_back();
    }
    if (s.empty()) return L"None";
    return s;
}

INT_PTR CALLBACK SettingsDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_INITDIALOG:
        SetDlgItemTextW(hDlg, IDC_TXT_HOTKEY, GetHotkeyString().c_str());
        RefreshMacroList(hDlg);
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (HIWORD(wParam) == EN_SETFOCUS) {
            if (LOWORD(wParam) == IDC_TXT_MACRO_KEY || LOWORD(wParam) == IDC_TXT_MACRO_VAL) {
                SendMessage(hDlg, DM_SETDEFID, IDC_BTN_MACRO_ADD, 0);
                PostMessage((HWND)lParam, EM_SETSEL, 0, -1);
            }
        }
        else if (HIWORD(wParam) == EN_KILLFOCUS) {
            if (LOWORD(wParam) == IDC_TXT_MACRO_KEY || LOWORD(wParam) == IDC_TXT_MACRO_VAL) {
                SendMessage(hDlg, DM_SETDEFID, IDOK, 0);
            }
        }
        
        if (LOWORD(wParam) == IDC_BTN_HOTKEY_SAVE) {
            g_capturingHotkey = true;
            SetDlgItemTextW(hDlg, IDC_TXT_HOTKEY, L"[Nhấn phím tắt mới...]");
            SetFocus(GetDlgItem(hDlg, IDC_STATIC)); // Unfocus to avoid editing
        }
        else if (LOWORD(wParam) == IDC_BTN_MACRO_ADD) {
            wchar_t key[64], val[128];
            GetDlgItemTextW(hDlg, IDC_TXT_MACRO_KEY, key, 64);
            GetDlgItemTextW(hDlg, IDC_TXT_MACRO_VAL, val, 128);
            if (wcslen(key) > 0 && wcslen(val) > 0) {
                g_config.macros[key] = val;
                ConfigManager::SaveConfig(g_config);
                RefreshMacroList(hDlg);
                SetDlgItemTextW(hDlg, IDC_TXT_MACRO_KEY, L"");
                SetDlgItemTextW(hDlg, IDC_TXT_MACRO_VAL, L"");
                SetFocus(GetDlgItem(hDlg, IDC_TXT_MACRO_KEY));
            }
        }
        else if (LOWORD(wParam) == IDC_LST_MACROS && HIWORD(wParam) == LBN_DBLCLK) {
            HWND hList = GetDlgItem(hDlg, IDC_LST_MACROS);
            int idx = (int)SendMessage(hList, LB_GETCURSEL, 0, 0);
            if (idx != LB_ERR) {
                int len = (int)SendMessage(hList, LB_GETTEXTLEN, idx, 0);
                std::wstring item(len, L'\0');
                SendMessageW(hList, LB_GETTEXT, idx, (LPARAM)&item[0]);
                size_t pos = item.find(L" \x2192 ");
                if (pos != std::wstring::npos) {
                    std::wstring key = item.substr(0, pos);
                    std::wstring val = item.substr(pos + 3);
                    
                    // Remove from dictionary
                    g_config.macros.erase(key);
                    ConfigManager::SaveConfig(g_config);
                    RefreshMacroList(hDlg);
                    
                    // Put into textboxes for editing
                    SetDlgItemTextW(hDlg, IDC_TXT_MACRO_KEY, key.c_str());
                    SetDlgItemTextW(hDlg, IDC_TXT_MACRO_VAL, val.c_str());
                    SetFocus(GetDlgItem(hDlg, IDC_TXT_MACRO_VAL));
                }
            }
        }
        else if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) {
            DestroyWindow(hDlg);
            g_hDlg = nullptr;
            g_capturingHotkey = false;
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

void ShowSettings() {
    if (!g_hDlg) {
        g_hDlg = CreateDialogParamW(g_hInstance, MAKEINTRESOURCEW(IDD_SETTINGS_DIALOG), nullptr, SettingsDlgProc, 0);
        ShowWindow(g_hDlg, SW_SHOW);
    } else {
        SetForegroundWindow(g_hDlg);
    }
}

HICON CreateTrayIcon(COLORREF color) {
    int width = GetSystemMetrics(SM_CXSMICON);
    int height = GetSystemMetrics(SM_CYSMICON);
    
    HDC hdc = GetDC(nullptr);
    HDC hMemDC = CreateCompatibleDC(hdc);
    HBITMAP hBitmap = CreateCompatibleBitmap(hdc, width, height);
    
    HBITMAP hOldBitmap = (HBITMAP)SelectObject(hMemDC, hBitmap);
    
    RECT rect = { 0, 0, width, height };
    HBRUSH hBrush = CreateSolidBrush(color);
    FillRect(hMemDC, &rect, hBrush);
    DeleteObject(hBrush);
    
    SetTextColor(hMemDC, RGB(255, 255, 255));
    SetBkMode(hMemDC, TRANSPARENT);
    HFONT hFont = CreateFontW(-MulDiv(10, GetDeviceCaps(hdc, LOGPIXELSY), 72), 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_OUTLINE_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, VARIABLE_PITCH, L"Arial");
    HFONT hOldFont = (HFONT)SelectObject(hMemDC, hFont);
    DrawTextW(hMemDC, L"V", -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    
    SelectObject(hMemDC, hOldFont);
    DeleteObject(hFont);
    SelectObject(hMemDC, hOldBitmap);
    DeleteDC(hMemDC);
    ReleaseDC(nullptr, hdc);
    
    // Tạo mask monochrome (tất cả 0s / đen nghĩa là bitmap màu opaque)
    BYTE maskBits[256] = {0};
    HBITMAP hMask = CreateBitmap(width, height, 1, 1, maskBits);
    
    ICONINFO ii = { 0 };
    ii.fIcon = TRUE;
    ii.hbmMask = hMask;
    ii.hbmColor = hBitmap;
    
    HICON hIcon = CreateIconIndirect(&ii);
    
    DeleteObject(hMask);
    DeleteObject(hBitmap);
    
    return hIcon;
}

bool IsAutoStart() {
    HKEY hKey;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        DWORD type;
        if (RegQueryValueExW(hKey, L"Cay", nullptr, &type, nullptr, nullptr) == ERROR_SUCCESS) {
            RegCloseKey(hKey);
            return true;
        }
        RegCloseKey(hKey);
    }
    return false;
}

void ToggleAutoStart() {
    bool on = !IsAutoStart();
    HKEY hKey;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_WRITE, &hKey) == ERROR_SUCCESS) {
        if (on) {
            wchar_t exePath[MAX_PATH];
            GetModuleFileNameW(nullptr, exePath, MAX_PATH);
            RegSetValueExW(hKey, L"Cay", 0, REG_SZ, (const BYTE*)exePath, (DWORD)(lstrlenW(exePath) + 1) * sizeof(wchar_t));
        } else {
            RegDeleteValueW(hKey, L"Cay");
        }
        RegCloseKey(hKey);
    }
}

void UpdateTrayIcon(bool isAdd = false) {
    g_nid.hIcon = g_enabled ? g_iconOn : g_iconOff;
    std::wstring hotkeyStr = GetHotkeyString();
    std::wstring tip = g_enabled ? L"Cay - Bật (" + hotkeyStr + L" để tắt)" : L"Cay - Tắt (" + hotkeyStr + L" để bật)";
    
    // limit length to 127 chars
    wcsncpy_s(g_nid.szTip, tip.c_str(), 127);
    
    Shell_NotifyIconW(isAdd ? NIM_ADD : NIM_MODIFY, &g_nid);
}

void Toggle() {
    g_enabled = !g_enabled;
    UpdateTrayIcon();
    g_engine.ResetFull();
    MessageBeep(MB_OK);
}

void ShowContextMenu(POINT pt) {
    HMENU hMenu = CreatePopupMenu();
    AppendMenuW(hMenu, MF_STRING, IDM_SETTINGS, L"Cấu hình Cayy");
    AppendMenuW(hMenu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(hMenu, MF_STRING | (IsAutoStart() ? MF_CHECKED : MF_UNCHECKED), IDM_AUTOSTART, L"Tự khởi động");
    AppendMenuW(hMenu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(hMenu, MF_STRING, IDM_EXIT, L"Thoát");

    SetForegroundWindow(g_hWnd);
    TrackPopupMenu(hMenu, TPM_BOTTOMALIGN | TPM_LEFTALIGN, pt.x, pt.y, 0, g_hWnd, nullptr);
    DestroyMenu(hMenu);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_TRAYICON:
        if (LOWORD(lParam) == WM_LBUTTONDBLCLK) {
            ShowSettings();
        } else if (LOWORD(lParam) == WM_LBUTTONUP) {
            // LBUTTONUP can toggle or show settings? Double click is better for settings.
            // Let's keep toggle on single click like minimal
            Toggle();
        } else if (LOWORD(lParam) == WM_RBUTTONUP) {
            POINT pt;
            GetCursorPos(&pt);
            ShowContextMenu(pt);
        }
        break;
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDM_TOGGLE: Toggle(); break;
        case IDM_SETTINGS: ShowSettings(); break;
        case IDM_AUTOSTART: ToggleAutoStart(); break;
        case IDM_EXIT:
            PostQuitMessage(0);
            break;
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProcW(hWnd, message, wParam, lParam);
    }
    return 0;
}

Cay::KeyCode MapVKToKeyCode(DWORD vk) {
    if (vk >= 'A' && vk <= 'Z') return (Cay::KeyCode)vk;
    switch (vk) {
        case VK_BACK: return Cay::KeyCode::Backspace;
        case VK_ESCAPE: return Cay::KeyCode::Escape;
        case VK_RETURN: return Cay::KeyCode::Enter;
        case VK_TAB: return Cay::KeyCode::Tab;
        case VK_SPACE: return Cay::KeyCode::Space;
        case VK_LEFT: return Cay::KeyCode::Left;
        case VK_RIGHT: return Cay::KeyCode::Right;
        case VK_UP: return Cay::KeyCode::Up;
        case VK_DOWN: return Cay::KeyCode::Down;
        case VK_HOME: return Cay::KeyCode::Home;
        case VK_END: return Cay::KeyCode::End;
        case VK_PRIOR: return Cay::KeyCode::PageUp;
        case VK_NEXT: return Cay::KeyCode::PageDown;
        case VK_DELETE: return Cay::KeyCode::Delete;
        default: return Cay::KeyCode::Unknown;
    }
}

void OnKeyDownHook(CayIME::InputHookManager* sender, CayIME::HookKeyEventArgs& e) {
    if (e.extraInfo == CayIME::InputInjector::MAGIC_EXTRA_INFO) return;

    bool isCtrlPressed = (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
    bool isAltPressed  = (GetAsyncKeyState(VK_MENU) & 0x8000) != 0;
    bool isWinPressed  = (GetAsyncKeyState(VK_LWIN) & 0x8000) != 0 || (GetAsyncKeyState(VK_RWIN) & 0x8000) != 0;
    bool isShiftPressed = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;

    // Bắt phím tắt nếu đang ở chế độ capture
    if (g_capturingHotkey) {
        if (e.keyCode != VK_CONTROL && e.keyCode != VK_LCONTROL && e.keyCode != VK_RCONTROL &&
            e.keyCode != VK_SHIFT && e.keyCode != VK_LSHIFT && e.keyCode != VK_RSHIFT &&
            e.keyCode != VK_MENU && e.keyCode != VK_LMENU && e.keyCode != VK_RMENU &&
            e.keyCode != VK_LWIN && e.keyCode != VK_RWIN) {
            
            g_config.ctrl = isCtrlPressed;
            g_config.shift = isShiftPressed;
            g_config.alt = isAltPressed;
            g_config.win = isWinPressed;
            g_config.vkCode = e.keyCode;
            
            ConfigManager::SaveConfig(g_config);
            g_capturingHotkey = false;
            
            if (g_hDlg) {
                SetDlgItemTextW(g_hDlg, IDC_TXT_HOTKEY, GetHotkeyString().c_str());
            }
            UpdateTrayIcon();
            e.handled = true;
            return;
        }
    }

    // Kiểm tra xem có đúng phím tắt bật/tắt không
    bool matchCtrl = (g_config.ctrl == isCtrlPressed);
    bool matchShift = (g_config.shift == isShiftPressed);
    bool matchAlt = (g_config.alt == isAltPressed);
    bool matchWin = (g_config.win == isWinPressed);
    
    // Nếu hotkey chỉ là modifier (ví dụ Ctrl+Shift) thì vkCode = 0
    if (g_config.vkCode == 0) {
        // Chờ nhả phím (KeyUp) để thực thi toggle với modifier only
    } else {
        // Phím có vkCode cụ thể
        if (matchCtrl && matchShift && matchAlt && matchWin && e.keyCode == g_config.vkCode) {
            Toggle();
            e.handled = true;
            return;
        }
    }

    if (isCtrlPressed || isAltPressed || isWinPressed) {
        g_engine.ResetFull();
        return;
    }

    if (!g_enabled) return;

    Cay::KeyEvent ce;
    ce.keyCode = MapVKToKeyCode(e.keyCode);
    ce.character = e.character;
    ce.handled = false;
    g_engine.OnKeyDown(ce);
    if (ce.handled) e.handled = true;
}

void OnKeyUpHook(CayIME::InputHookManager* sender, CayIME::HookKeyEventArgs& e) {
    if (e.extraInfo == CayIME::InputInjector::MAGIC_EXTRA_INFO) return;

    bool isCtrlPressed = (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
    bool isAltPressed  = (GetAsyncKeyState(VK_MENU) & 0x8000) != 0;
    bool isWinPressed  = (GetAsyncKeyState(VK_LWIN) & 0x8000) != 0 || (GetAsyncKeyState(VK_RWIN) & 0x8000) != 0;
    bool isShiftPressed = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;

    if (g_capturingHotkey) {
        // Nếu nhả một phím modifier, ta coi như người dùng muốn gán phím tắt chỉ gồm các modifier
        bool wasCtrl = (e.keyCode == VK_CONTROL || e.keyCode == VK_LCONTROL || e.keyCode == VK_RCONTROL);
        bool wasShift = (e.keyCode == VK_SHIFT || e.keyCode == VK_LSHIFT || e.keyCode == VK_RSHIFT);
        bool wasAlt = (e.keyCode == VK_MENU || e.keyCode == VK_LMENU || e.keyCode == VK_RMENU);
        bool wasWin = (e.keyCode == VK_LWIN || e.keyCode == VK_RWIN);
        
        if (wasCtrl || wasShift || wasAlt || wasWin) {
            g_config.ctrl = isCtrlPressed || wasCtrl;
            g_config.shift = isShiftPressed || wasShift;
            g_config.alt = isAltPressed || wasAlt;
            g_config.win = isWinPressed || wasWin;
            g_config.vkCode = 0; // Không có phím thường
            
            // Chỉ lưu nếu có ít nhất 1 modifier
            if (g_config.ctrl || g_config.shift || g_config.alt || g_config.win) {
                ConfigManager::SaveConfig(g_config);
                g_capturingHotkey = false;
                
                if (g_hDlg) {
                    SetDlgItemTextW(g_hDlg, IDC_TXT_HOTKEY, GetHotkeyString().c_str());
                }
                UpdateTrayIcon();
            }
            e.handled = true;
            return;
        }
    }

    // Toggle on modifier release if vkCode == 0
    if (g_config.vkCode == 0) {
        bool wasCtrl = (e.keyCode == VK_CONTROL || e.keyCode == VK_LCONTROL || e.keyCode == VK_RCONTROL);
        bool wasShift = (e.keyCode == VK_SHIFT || e.keyCode == VK_LSHIFT || e.keyCode == VK_RSHIFT);
        bool wasAlt = (e.keyCode == VK_MENU || e.keyCode == VK_LMENU || e.keyCode == VK_RMENU);
        bool wasWin = (e.keyCode == VK_LWIN || e.keyCode == VK_RWIN);
        
        if ((wasCtrl && g_config.ctrl && isShiftPressed == g_config.shift && isAltPressed == g_config.alt && isWinPressed == g_config.win) ||
            (wasShift && g_config.shift && isCtrlPressed == g_config.ctrl && isAltPressed == g_config.alt && isWinPressed == g_config.win) ||
            (wasAlt && g_config.alt && isCtrlPressed == g_config.ctrl && isShiftPressed == g_config.shift && isWinPressed == g_config.win) ||
            (wasWin && g_config.win && isCtrlPressed == g_config.ctrl && isShiftPressed == g_config.shift && isAltPressed == g_config.alt)) 
        {
            Toggle();
            return;
        }
    }

    if (!g_enabled) return;
    Cay::KeyEvent ce;
    ce.keyCode = MapVKToKeyCode(e.keyCode);
    ce.character = e.character;
    ce.handled = false;
    g_engine.OnKeyUp(ce);
    if (ce.handled) e.handled = true;
}

void OnMouseClickHook(CayIME::InputHookManager* sender) {
    g_engine.ResetFull();
}

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR lpCmdLine, int nCmdShow) {
    // Kiểm tra single instance
    wchar_t mutexName[256];
    wchar_t userName[256];
    DWORD userNameLen = 256;
    GetUserNameW(userName, &userNameLen);
    wsprintfW(mutexName, L"Global\\CayVN_%s", userName);
    
    HANDLE hMutex = CreateMutexW(nullptr, TRUE, mutexName);
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        MessageBoxW(nullptr, L"Cay \u0111\u00e1ng ch\u1ea1y!", L"Th\u00f4ng b\x00e1o", MB_OK | MB_ICONINFORMATION);
        CloseHandle(hMutex);
        return 0;
    }

    // Nâng priority
    SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);

    // Khởi tạo data (Đã xóa vì data là static)
    g_iconOn = CreateTrayIcon(RGB(255, 0, 0));
    g_iconOff = CreateTrayIcon(RGB(128, 128, 128));

    // Register Message-Only Window Class
    WNDCLASSEXW wcex = { 0 };
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.lpfnWndProc = WndProc;
    wcex.hInstance = hInstance;
    wcex.lpszClassName = L"CayMessageWindowClass";
    RegisterClassExW(&wcex);

    g_hWnd = CreateWindowExW(0, L"CayMessageWindowClass", L"CayMessageWindow", 0, 0, 0, 0, 0, HWND_MESSAGE, nullptr, hInstance, nullptr);

    // Setup Tray
    g_nid.cbSize = sizeof(NOTIFYICONDATAW);
    g_nid.hWnd = g_hWnd;
    g_nid.uID = 1;
    g_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    g_nid.uCallbackMessage = WM_TRAYICON;
    UpdateTrayIcon(true);

    // Check first launch
    HKEY hKeyFirst;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"SOFTWARE\\CayIME", 0, KEY_READ, &hKeyFirst) != ERROR_SUCCESS) {
        if (MessageBoxW(nullptr, L"Bạn có muốn Cay tự động chạy khi khởi động máy không?", L"Cay - Lần chạy đầu tiên", MB_YESNO | MB_ICONQUESTION) == IDYES) {
            if (!IsAutoStart()) ToggleAutoStart();
        }
        HKEY hKeyWrite;
        if (RegCreateKeyExW(HKEY_CURRENT_USER, L"SOFTWARE\\CayIME", 0, nullptr, 0, KEY_WRITE, nullptr, &hKeyWrite, nullptr) == ERROR_SUCCESS) {
            DWORD val = 1;
            RegSetValueExW(hKeyWrite, L"FirstLaunch", 0, REG_DWORD, (const BYTE*)&val, sizeof(val));
            RegCloseKey(hKeyWrite);
        }
    } else {
        RegCloseKey(hKeyFirst);
    }

    // Load Config and Macro
    g_config = ConfigManager::LoadConfig();

    // Bắt đầu Hooks
    g_hookManager = new CayIME::InputHookManager();
    g_hookManager->KeyDown = OnKeyDownHook;
    g_hookManager->KeyUp = OnKeyUpHook;
    g_engine.OnInjectText = CayIME::InputInjector::ReplaceText;
    g_engine.OnMacroLookup = MacroLookupCallback;
    g_hookManager->MouseClick = OnMouseClickHook;

    // Message Loop
    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        if (!g_hDlg || !IsDialogMessage(g_hDlg, &msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    // Cleanup
    delete g_hookManager;
    Shell_NotifyIconW(NIM_DELETE, &g_nid);
    DestroyWindow(g_hWnd);
    DestroyIcon(g_iconOn);
    DestroyIcon(g_iconOff);
    ReleaseMutex(hMutex);
    CloseHandle(hMutex);

    return (int)msg.wParam;
}


