/*------------------------------------------------------------------------------
UniKey - Vietnamese Keyboard for Windows
Copyright (C) 1998-2002 Pham Kim Long
Contact: unikey@gmail.com

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
--------------------------------------------------------------------------------*/
//#include <windows.h>
#include "prehdr.h"
#include "resource.h"
#include "mainwnd.h"
#include "about.h"
#include "userpref.h"
#include "vnconv.h"

#define UNIKEY_HOME_PAGE _TEXT("https://github.com/tctvn/cay")
#define AUTHOR_EMAIL _TEXT("mailto:unikey@gmail.com")
#define SUPPORT_HOME_PAGE _TEXT("https://github.com/tctvn/cay/issues")

extern HINSTANCE AppInstance;
extern CUserPref UserPref; // user preference

DlgItemInfo VnAboutDlgItems[] = {
	{IDC_AUTHOR,				(wchar_t*)L"Cay Classic \u2013 B\u1ed9 g\u00f5 ti\u1ebfng Vi\u1ec7t v1.0.4"},
	{IDC_SUPPORT_PROVIDER,		(wchar_t*)L"D\u1ef1 \u00e1n m\u00e3 ngu\u1ed3n m\u1edf:"},
	{IDC_SUPPORT_LINK,		(wchar_t*)L"https://github.com/tctvn/cay"},
	{IDC_COPYRIGHT,		(wchar_t*)L"Ph\u00e1t tri\u1ec3n b\u1edfi tctvn\nGi\u1ea5y ph\u00e9p: GPL-3.0"},
	{IDC_INFO,			(wchar_t*)L" Cay Classic l\u00e0 ch\u01b0\u01a1ng tr\u00ecnh m\u00e3 ngu\u1ed3n m\u1edf d\u1ef1a tr\u00ean b\u1ed9 g\u00f5 UniKey.\n T\u00e1c gi\u1ea3 g\u1ed1c UniKey: Ph\u1ea1m Kim Long (unikey.org)\n B\u1ea1n c\u00f3 th\u1ec3 ph\u00e2n ph\u1ed1i, s\u1eeda \u0111\u1ed5i tu\u00e2n theo The GNU General Public License."}};

#define MAX_ITEM_LEN 256

#ifdef _UNICODE

wchar_t UniVnAboutDlgTexts[sizeof(VnAboutDlgItems)/sizeof(AnsiDlgItemInfo)][MAX_ITEM_LEN];
DlgItemInfo UniVnAboutDlgItems[sizeof(VnAboutDlgItems)/sizeof(AnsiDlgItemInfo)];

#endif

BOOL CAboutDlg::DialogProc(
	HWND hDlg,  // handle to dialog box
	UINT uMsg,     // message
	WPARAM wParam, // first message parameter
	LPARAM lParam  // second message parameter
)
{
	switch (uMsg) {
	case WM_DRAWITEM:
		// Send the reflection message back to the control
		return SendDlgItemMessage(hDlg, wParam, WM_DRAWITEM, wParam, lParam);
	case WM_CTLCOLORSTATIC:
		// Send the reflection message back to the control
		return SendMessage((HWND)lParam, WM_CTLCOLORSTATIC, wParam, 0);
	case WM_DESTROY:
		DestroyCursor(m_hLinkCursor);
		if (m_hStdFont != NULL) {
			DeleteObject(m_hStdFont);
			m_hStdFont = NULL;
		}
		break;
	}
	return CVirDialog::DialogProc(hDlg, uMsg, wParam, lParam);
}

//---------------------------------------------
BOOL CAboutDlg::onInitDialog()
{
	if (!CVirDialog::onInitDialog()) return FALSE;
	if (m_langID == 1) {
		setItemsText(1);
		setDlgFont(1);
	}

	//setup buttons
	m_okBtn.init(m_hInst, m_hWnd, GetDlgItem(m_hWnd, IDOK));
	//m_okBtn.setTextColor(RGB(255, 0, 0));
	m_okBtn.setIcon(IDI_OK, 16, 16);
	m_okBtn.setDefaultButton(TRUE);
	//setup hyper-link labels
	m_hLinkCursor = (HCURSOR)LoadImage(
								m_hInst, 
								MAKEINTRESOURCE(IDC_HAND_CURSOR), 
								IMAGE_CURSOR, 32, 32, 0);
	CLinkLabel::setLinkCursor(m_hLinkCursor);
	m_webLink.init(m_hInst, m_hWnd, GetDlgItem(m_hWnd, IDC_WEB_LINK), UNIKEY_HOME_PAGE);
	m_supportLink.init(m_hInst, m_hWnd, GetDlgItem(m_hWnd, IDC_SUPPORT_LINK), SUPPORT_HOME_PAGE);
/*
#ifndef _UNICODE
	m_supportLink.setFontName(VIET_FONT_NAME);
	m_supportLink.reconstructFont();
#endif
*/
	//m_backupWebLink.init(m_hInst, m_hWnd, GetDlgItem(m_hWnd, IDC_BACKUP_WEB_LINK), UNIKEY_BACKUP_HOME_PAGE);
	//m_mailLink.init(m_hInst, m_hWnd, GetDlgItem(m_hWnd, IDC_MAIL_LINK), AUTHOR_EMAIL);

	return TRUE;
}
//----------------------------------------------
// set text labels according to the chosen GUI language
// langID = 0 -> English (default)
//----------------------------------------------
void CAboutDlg::setItemsText(int langID)
{
	int items, i;
	if (langID == 0)
		return;
	items = sizeof(VnAboutDlgItems)/sizeof(DlgItemInfo);
	for (i=0; i<items; i++) {
		SendDlgItemMessage(m_hWnd, 
			VnAboutDlgItems[i].id, WM_SETTEXT, 0, 
			(LPARAM)VnAboutDlgItems[i].text);
	}
}

//----------------------------------------------
// set font for dialog items accoring to the selected GUI language
// langID = 0 -> English (default)
//----------------------------------------------
void CAboutDlg::setDlgFont(int langID)
{
	if (m_hStdFont != NULL)
		DeleteObject(m_hStdFont);
	HDC hDC = GetDC(m_hWnd);
	if (langID == 0)
		m_hStdFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
	else
		m_hStdFont = CustomCreateFont(hDC, VIET_FONT_NAME, VIET_DLG_FONT_SIZE);
	SendMessage(m_hWnd, WM_SETFONT, (WPARAM)m_hStdFont, TRUE);
	HWND wndChild = GetTopWindow(m_hWnd);
	for ( ; wndChild != NULL; wndChild = GetWindow(wndChild, GW_HWNDNEXT))
		SendMessage(wndChild, WM_SETFONT, (WPARAM)m_hStdFont, TRUE);
	ReleaseDC(m_hWnd, hDC);
}

//------------------------------------------------
void DoAbout(HINSTANCE hInst, HWND hParent, int langID)
{
	CAboutDlg dlg(langID);
	dlg.init(hInst, MAKEINTRESOURCE(IDD_ABOUT), hParent);
	dlg.ShowDialog();
}

