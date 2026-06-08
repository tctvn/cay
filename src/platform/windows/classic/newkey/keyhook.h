#ifndef _KEYHOOK_H
#define _KEYHOOK_H 1
#include <windows.h>
#include "keycons.h"

struct CodeInfo {
	DWORD DT[256];
	unsigned char BD[12][6];
	unsigned char BK[8];
	unsigned char BW[6];
	unsigned char BT[6];
	WORD ToUniH[256];
	WORD ToUniL[256];
	int multiBytes; // 2 bytes charset (Unicode, VNI, ....)
	int encoding; //UCS-2, UTF-8, Reference
	int singleBackspace;
//	unsigned char remapLayout[256]; //used to remap Vietnamese layout to US layout
};

#define CTRL_SHIFT_SW 0
#define ALT_Z_SW 1

struct KeyboardOptions
{
	int freeMarking;
	int toneNextToVowel;
	int modernStyle;
	int macroEnabled;
	int useUnicodeClipboard;
	int alwaysMacro;
	int useIME;
};

struct HookMacroDef
{
	int keyOffset;
	int textOffset;
};

struct SharedMem {
	//states
	int Initialized;
	int vietKey;
	int iconShown;
	int switchKey;

	KeyboardOptions options;

	WORD keyMode;
	int inMethod;
	CodeInfo codeTable;

	// system
	HHOOK keyHook,mouseHook;
	HWND hMainDlg;
	UINT iconMsgId;
	HICON hVietIcon,hEnIcon;
	int unicodePlatform;
	DWORD winMajorVersion, winMinorVersion;

	// macro table
//	MacroDef macroTable[MAX_MACRO_ITEMS];
	HookMacroDef macroTable[MAX_MACRO_ITEMS];
	char macroMem[MACRO_MEM_SIZE];
	int macroCount;
};

//----------------------------------------------------------------
// Interface functions 
LRESULT CALLBACK MyKeyHook(int code, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK MyMouseHook(int code, WPARAM wParam, LPARAM lParam);
HINSTANCE GetVietHookDll();

void SwitchMode();
void ModifyStatusIcon();
void DeleteStatusIcon();

int IsVietnamese();
//void InitSharedMem(SharedMem *pPara);
void SetInputMethod(int method, DWORD *DT);
void SetKeyMode(WORD mode, int inMethod, CodeInfo *pTable);
void SetSwitchKey(int swKey);
//void SetKeyOptions(int freeMarking, int toneNextToVowel, int modernStyle);

SharedMem *GetSharedMem();

#define WM_HOOK_TOOLKIT_SHORTCUT (WM_USER+102)
#define WM_HOOK_PANEL_SHORTCUT (WM_USER+103)
#define WM_HOOK_FLY_CONVERT (WM_USER+104)
#define WM_HOOK_CHANGE_CHARSET (WM_USER+105)

#endif
