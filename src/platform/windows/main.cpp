
#include <windows.h>
#include <shellapi.h>
#include <winreg.h>
#include "CayData.h"
#include "CayEngine.h"
#include "KeyboardHookManager.h"
#include "InputInjector.h"

#pragma comment(linker, "/NODEFAULTLIB")
#pragma comment(linker, "/ENTRY:wWinMainCRTStartup")
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

HWND g_hWnd = nullptr;
NOTIFYICONDATAW g_nid = { 0 };
Cay::TelexEngine g_engine;
CayIME::InputHookManager* g_hookManager = nullptr;

bool g_enabled = true;
HICON g_iconOn = nullptr;
HICON g_iconOff = nullptr;
bool g_ctrl = false, g_win = false, g_alt = false, g_pendingToggle = false;

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
    lstrcpyW(g_nid.szTip, g_enabled ? L"Cay - B\u1eadt (Ctrl+Shift \u0111\u1ec3 t\u1eaft)" : L"Cay - T\u1eaft (Ctrl+Shift \u0111\u1ec3 b\u1eadt)");
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
    AppendMenuW(hMenu, MF_STRING | (IsAutoStart() ? MF_CHECKED : MF_UNCHECKED), IDM_AUTOSTART, L"T\x1EF1 kh\x1EDFi \x0111\x1ED9ng");
    AppendMenuW(hMenu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(hMenu, MF_STRING, IDM_ABOUT, L"Th\x00F4ng tin");
    AppendMenuW(hMenu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(hMenu, MF_STRING, IDM_EXIT, L"Tho\x00E1t");

    SetForegroundWindow(g_hWnd);
    TrackPopupMenu(hMenu, TPM_BOTTOMALIGN | TPM_LEFTALIGN, pt.x, pt.y, 0, g_hWnd, nullptr);
    DestroyMenu(hMenu);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_TRAYICON:
        if (LOWORD(lParam) == WM_LBUTTONUP) {
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
        case IDM_AUTOSTART: ToggleAutoStart(); break;
        case IDM_ABOUT:
            MessageBoxW(hWnd, 
                L"Cay \u2013 B\u1ed9 g\u00f5 ti\u1ebfng Vi\u1ec7t Telex v1.0\n\n"
                L"Ctrl+Shift = B\u1eadt / T\u1eaft\n\n"
                L"aa\u2192\u00e2  aw\u2192\u0103  dd\u2192\u0111  ee\u2192\u00ea  oo\u2192\u00f4  ow\u2192\u01a1  uw\u2192\u01b0\n"
                L"s=s\u1eafc  f=huy\u1ec1n  r=h\u1ecfi  x=ng\u00e3  j=n\u1eb7ng",
                L"Gi\u1edbi thi\u1ec7u", MB_OK | MB_ICONINFORMATION);
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

    if (e.keyCode == VK_LCONTROL || e.keyCode == VK_RCONTROL) {
        g_ctrl = true; g_engine.ResetFull(); return;
    }
    if (e.keyCode == VK_LWIN || e.keyCode == VK_RWIN) {
        g_win = true; g_engine.ResetFull(); return;
    }
    if (e.keyCode == VK_LMENU || e.keyCode == VK_RMENU) {
        g_alt = true; g_engine.ResetFull(); return;
    }

    if (g_ctrl && (e.keyCode == VK_LSHIFT || e.keyCode == VK_RSHIFT)) {
        g_pendingToggle = true; return;
    }

    if (g_ctrl || g_win || g_alt) return;
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
        g_ctrl = false; 
        if (g_pendingToggle) { g_pendingToggle = false; Toggle(); } 
        return;
    }
    if (e.keyCode == VK_LWIN || e.keyCode == VK_RWIN) { g_win = false; return; }
    if (e.keyCode == VK_LMENU || e.keyCode == VK_RMENU) { g_alt = false; return; }
    
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

