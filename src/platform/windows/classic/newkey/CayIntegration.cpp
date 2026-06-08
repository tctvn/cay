#include "prehdr.h"
#include "mainwnd.h"
#include "CayEngine.h"
#include <string>
#include "KeyboardHookManager.h"
#include "InputInjector.h"
#include <tchar.h>
#include "userpref.h"
#include "encode.h"
#include "resource.h"
#include "../vnconv/vnconv.h"

extern HINSTANCE AppInstance;
extern CUserPref UserPref;
extern CharsetInfo CharsetTable[];

SharedMem g_sharedMemInst = {0};
SharedMem * pSharedMem = &g_sharedMemInst;

SharedMem *GetSharedMem() { return &g_sharedMemInst; }

Cay::TelexEngine g_engine;
CayIME::InputHookManager* g_hookManager = nullptr;

#include "mainwnd.h"

extern CMainWnd MainWnd;
HINSTANCE GetVietHookDll() { return NULL; }

void CayUpdateInputMethod(int method) {
    g_engine.inputMethod = (Cay::InputMethod)method;
}

const wchar_t* CayMacroLookup(const wchar_t* input) {
    if (!UserPref.m_macroEnabled) return nullptr;

    char keyBuf[MAX_MACRO_KEY_LEN];
    int i = 0;
    for (; input[i] && i < MAX_MACRO_KEY_LEN - 1; i++) {
        keyBuf[i] = (char)input[i];
    }
    keyBuf[i] = '\0';

    for (int j = 0; j < MainWnd.m_macTab.m_count; j++) {
        if (lstrcmpiA(MainWnd.m_macTab.m_table[j].key, keyBuf) == 0) {
            static wchar_t s_macroResult[MAX_MACRO_TEXT_LEN];
            int inLen = -1;
            int maxOutLen = MAX_MACRO_TEXT_LEN * sizeof(wchar_t);
            if (VnConvert(CONV_CHARSET_VIQR, CONV_CHARSET_UNICODE, (BYTE*)MainWnd.m_macTab.m_table[j].text, (BYTE*)s_macroResult, inLen, maxOutLen) == VNCONV_NO_ERROR) {
                return s_macroResult;
            }
            return nullptr;
            return s_macroResult;
        }
    }
    return nullptr;
}

static std::wstring s_injectedHistory;

void ReplaceTextWrapper(int backspaceCount, const wchar_t* newText, int newTextLen) {
    if (newTextLen <= 0 && backspaceCount <= 0) return;

    int vnconvId = CharsetTable[UserPref.m_codeTable].vnconvId;

    if (vnconvId == CONV_CHARSET_UNICODE) {
        CayIME::InputInjector::ReplaceText(backspaceCount, newText, newTextLen);
        return;
    }

    if (backspaceCount > (int)s_injectedHistory.length()) {
        backspaceCount = (int)s_injectedHistory.length();
    }

    int physicalBs = backspaceCount;
    if (backspaceCount > 0) {
        std::wstring deletedStr = s_injectedHistory.substr(s_injectedHistory.length() - backspaceCount);
        int inLen = deletedStr.length() * sizeof(wchar_t);
        int maxOutLen = deletedStr.length() * 4 + 10;
        BYTE* outBuf = new BYTE[maxOutLen]();
        if (VnConvert(CONV_CHARSET_UNICODE, vnconvId, (BYTE*)deletedStr.c_str(), outBuf, inLen, maxOutLen) == VNCONV_NO_ERROR) {
            physicalBs = lstrlenA((char*)outBuf);
        }
        delete[] outBuf;
    }

    int finalNewTextLen = newTextLen;
    wchar_t* wOutBuf = nullptr;
    if (newTextLen > 0) {
        int inLen = newTextLen * sizeof(wchar_t);
        int maxOutLen = newTextLen * 4 + 10;
        BYTE* outBuf = new BYTE[maxOutLen]();
        if (VnConvert(CONV_CHARSET_UNICODE, vnconvId, (BYTE*)newText, outBuf, inLen, maxOutLen) == VNCONV_NO_ERROR) {
            finalNewTextLen = lstrlenA((char*)outBuf);
            wOutBuf = new wchar_t[finalNewTextLen + 1];
            for (int i = 0; i < finalNewTextLen; i++) {
                wOutBuf[i] = outBuf[i];
            }
            wOutBuf[finalNewTextLen] = L'\0';
        }
        delete[] outBuf;
    }

    s_injectedHistory.erase(s_injectedHistory.length() - backspaceCount);
    s_injectedHistory.append(newText, newTextLen);
    if (s_injectedHistory.length() > 500) {
        s_injectedHistory = s_injectedHistory.substr(s_injectedHistory.length() - 250);
    }

    if (wOutBuf) {
        CayIME::InputInjector::ReplaceText(physicalBs, wOutBuf, finalNewTextLen);
        delete[] wOutBuf;
    } else {
        CayIME::InputInjector::ReplaceText(physicalBs, newText, newTextLen);
    }
}

void ModifyStatusIcon() {
	NOTIFYICONDATA tnid;
	tnid.cbSize = sizeof(NOTIFYICONDATA);
	tnid.hWnd = GetSharedMem()->hMainDlg;
	tnid.uID = 1;
	tnid.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
	tnid.uCallbackMessage = GetSharedMem()->iconMsgId;
	tnid.hIcon = GetSharedMem()->vietKey ? GetSharedMem()->hVietIcon : GetSharedMem()->hEnIcon;
	lstrcpy(tnid.szTip, _T("Cay Classic"));
	if (GetSharedMem()->iconShown)
		Shell_NotifyIcon(NIM_MODIFY, &tnid);
	else {
		Shell_NotifyIcon(NIM_ADD, &tnid);
		GetSharedMem()->iconShown = 1;
	}
}

void DeleteStatusIcon() {
	if (GetSharedMem()->iconShown) {
		NOTIFYICONDATA tnid;
		tnid.cbSize = sizeof(NOTIFYICONDATA);
		tnid.hWnd = GetSharedMem()->hMainDlg;
		tnid.uID = 1;
		Shell_NotifyIcon(NIM_DELETE, &tnid);
		GetSharedMem()->iconShown = 0;
	}
}

void SwitchMode() {
	GetSharedMem()->vietKey = !GetSharedMem()->vietKey;
	ModifyStatusIcon();
}

int IsVietnamese() { return GetSharedMem()->vietKey; }

void SetInputMethod(int method, DWORD *DT) {
    GetSharedMem()->inMethod = method;
    g_engine.inputMethod = (Cay::InputMethod)method;
}
void SetKeyMode(WORD mode, int inMethod, CodeInfo *pTable) {}
void SetSwitchKey(int swKey) { GetSharedMem()->switchKey = swKey; }

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

	// Switch Key Logic
    int switchKey = GetSharedMem()->switchKey;
    if (switchKey == 0) { // CTRL+SHIFT
        bool isShiftVk = (e.keyCode == VK_SHIFT || e.keyCode == VK_LSHIFT || e.keyCode == VK_RSHIFT);
        bool isCtrlVk = (e.keyCode == VK_CONTROL || e.keyCode == VK_LCONTROL || e.keyCode == VK_RCONTROL);

        if ((isShiftVk && isCtrlPressed && !isAltPressed && !isWinPressed) ||
            (isCtrlVk && isShiftPressed && !isAltPressed && !isWinPressed)) {
            SwitchMode();
            g_engine.ResetFull();
        }
    } else if (switchKey == 1) { // ALT+Z
        if (e.keyCode == 'Z' && isAltPressed && !isCtrlPressed && !isShiftPressed && !isWinPressed) {
            SwitchMode();
            g_engine.ResetFull();
            e.handled = true;
            return;
        }
    }

    if (isCtrlPressed || isAltPressed || isWinPressed) {
        g_engine.ResetFull();
        return;
    }

    if (!GetSharedMem()->vietKey) return;

    Cay::KeyEvent ce;
    ce.keyCode = MapVKToKeyCode(e.keyCode);
    ce.character = e.character;
    ce.handled = false;
    g_engine.OnKeyDown(ce);
    if (ce.handled) e.handled = true;
}

void OnKeyUpHook(CayIME::InputHookManager* sender, CayIME::HookKeyEventArgs& e) {
    if (e.extraInfo == CayIME::InputInjector::MAGIC_EXTRA_INFO) return;

    if (!GetSharedMem()->vietKey) return;
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

BOOL initDLL(HWND hWnd)
{
	pSharedMem = GetSharedMem();

	// Init shared memory
	pSharedMem->hMainDlg = hWnd;
	pSharedMem->hEnIcon = LoadIcon(AppInstance,MAKEINTRESOURCE(IDI_EN));
	pSharedMem->hVietIcon = LoadIcon(AppInstance,MAKEINTRESOURCE(IDI_VIET));
	pSharedMem->iconShown = 0;
	pSharedMem->keyMode = CharsetTable[UserPref.m_codeTable].id;
	pSharedMem->iconMsgId = WM_USER+1; // WM_MYICON_NOTIFY
	pSharedMem->switchKey = UserPref.m_switchKey;
	pSharedMem->vietKey = UserPref.m_vietnamese;
	pSharedMem->inMethod = UserPref.m_inMethod;
	g_engine.inputMethod = (Cay::InputMethod)UserPref.m_inMethod;
	g_engine.OnMacroLookup = CayMacroLookup;

	// Start Cay engine hooks
	g_hookManager = new CayIME::InputHookManager();
	g_hookManager->KeyDown = OnKeyDownHook;
	g_hookManager->KeyUp = OnKeyUpHook;
	g_engine.OnInjectText = ReplaceTextWrapper;
	g_hookManager->MouseClick = OnMouseClickHook;

	pSharedMem->Initialized = 1;
	return TRUE;
}
