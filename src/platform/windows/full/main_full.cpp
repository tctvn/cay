
#include <windows.h>
#include <shellapi.h>
#include <winreg.h>
#include "CayData.h"
#include "CayEngine.h"
#include "KeyboardHookManager.h"
#include "InputInjector.h"
#include "resource.h"

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
bool g_pendingToggle = false;

// Forward declarations
void Toggle();

// Dialog Procedure cho Settings
INT_PTR CALLBACK SettingsDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_INITDIALOG:
        CheckDlgButton(hDlg, IDC_CHK_ENABLE, g_enabled ? BST_CHECKED : BST_UNCHECKED);
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDC_CHK_ENABLE) {
            Toggle();
            SetDlgItemTextW(hDlg, IDC_LBL_STATUS, g_enabled ? L"Trạng thái: Đang hoạt động" : L"Trạng thái: Đã tắt");
        }
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) {
            DestroyWindow(hDlg);
            g_hDlg = nullptr;
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
    lstrcpyW(g_nid.szTip, g_enabled ? L"Cay - Bật (Ctrl+Shift để tắt)" : L"Cay - Tắt (Ctrl+Shift để bật)");
    Shell_NotifyIconW(isAdd ? NIM_ADD : NIM_MODIFY, &g_nid);
}

void Toggle() {
    g_enabled = !g_enabled;
    UpdateTrayIcon();
    g_engine.ResetFull();
    MessageBeep(MB_OK);
    
    // Update dialog checkbox if it is open
    if (g_hDlg) {
        CheckDlgButton(g_hDlg, IDC_CHK_ENABLE, g_enabled ? BST_CHECKED : BST_UNCHECKED);
        SetDlgItemTextW(g_hDlg, IDC_LBL_STATUS, g_enabled ? L"Trạng thái: Đang hoạt động" : L"Trạng thái: Đã tắt");
    }
}

void ShowContextMenu(POINT pt) {
    HMENU hMenu = CreatePopupMenu();
    AppendMenuW(hMenu, MF_STRING, IDM_SETTINGS, L"Cấu hình (Bản Full)");
    AppendMenuW(hMenu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(hMenu, MF_STRING | (IsAutoStart() ? MF_CHECKED : MF_UNCHECKED), IDM_AUTOSTART, L"Tự khởi động");
    AppendMenuW(hMenu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(hMenu, MF_STRING, IDM_ABOUT, L"Thông tin");
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
        case IDM_ABOUT:
            MessageBoxW(hWnd, 
                L"Cay (Bản Full) – Bộ gõ tiếng Việt siêu nhẹ v1.0.1\n\n"
                L"Ctrl+Shift = Bật / Tắt\n\n"
                L"aa→â  aw→ă  dd→đ  ee→ê  oo→ô  ow→ơ  uw→ư\n"
                L"s=sắc  f=huyền  r=hỏi  x=ngã  j=nặng\n\n"
                L"Source: github.com/tctvn/cay\n"
                L"License: GPL-3.0",
                L"Giới thiệu", MB_OK | MB_ICONINFORMATION);
            break;
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

    if (isCtrlPressed && (e.keyCode == VK_LSHIFT || e.keyCode == VK_RSHIFT)) {
        g_pendingToggle = true; 
        g_engine.ResetFull();
        return;
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

    if (e.keyCode == VK_LCONTROL || e.keyCode == VK_RCONTROL) {
        if (g_pendingToggle) { g_pendingToggle = false; Toggle(); } 
        return;
    }
    
    if (g_pendingToggle && (e.keyCode == VK_LSHIFT || e.keyCode == VK_RSHIFT)) {
        g_pendingToggle = false; Toggle(); return;
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
        if (MessageBoxW(nullptr, L"B\u1ea1n c\u00f3 mu\u1ed1n Cay t\u1ef1 \u0111\u1ed9ng ch\u1ea1y khi kh\u1edfi \u0111\u1ed9ng m\u00e1y kh\u00f4ng?", L"Cay - L\u1ea7n ch\u1ea1y \u0111\u1ea7u ti\u00ean", MB_YESNO | MB_ICONQUESTION) == IDYES) {
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

    // Bắt đầu Hooks
    g_hookManager = new CayIME::InputHookManager();
    g_hookManager->KeyDown = OnKeyDownHook;
    g_hookManager->KeyUp = OnKeyUpHook;
    g_engine.OnInjectText = CayIME::InputInjector::ReplaceText;
    g_hookManager->MouseClick = OnMouseClickHook;

    // Message Loop
    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
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


