/*
  KeePass Password Safe - The Open-Source Password Manager
  Copyright (C) 2003-2018 Dominik Reichl <dominik.reichl@t-online.de>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "StdAfx.h"
#include "AutoRichEditCtrlFx.h"
#include "../../KeePassLibCpp/Util/AppUtil.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CAutoRichEditCtrl

CAutoRichEditCtrlFx::CAutoRichEditCtrlFx()
{
	m_bPlainTextOnly = false;
}

CAutoRichEditCtrlFx::~CAutoRichEditCtrlFx()
{
}

BEGIN_MESSAGE_MAP(CAutoRichEditCtrlFx, CRichEditCtrl)
	//{{AFX_MSG_MAP(CAutoRichEditCtrlFx)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////

void CAutoRichEditCtrlFx::InitEx(bool bPlainTextOnly)
{
	m_bPlainTextOnly = bPlainTextOnly;
	if(bPlainTextOnly)
	{
		// SendMessage(EM_SETEDITSTYLE, SES_EMULATESYSEDIT, SES_EMULATESYSEDIT);

		// For EM_SETTEXTMODE the rich edit control must be empty
		// if(GetTextLength() > 0) { ASSERT(FALSE); SetRTF(_T(""), SF_TEXT); }
		// SendMessage(WM_SETTEXT, 0, (LPARAM)_T(""));

		// SetTextMode(TM_PLAINTEXT);
		// VERIFY(SendMessage(EM_SETTEXTMODE, TM_PLAINTEXT, 0) == 0);
		// ASSERT((GetTextMode() & TM_PLAINTEXT) != 0);
	}

	LimitText(0x7FFFFFF0);
}

CString CAutoRichEditCtrlFx::_StreamOutEx(int nFormat)
{
	EDITSTREAM es;
	ZeroMemory(&es, sizeof(EDITSTREAM));

	es.pfnCallback = CBStreamOut;

	CString sBuffer;
	es.dwCookie = (DWORD_PTR)&sBuffer;

	this->StreamOut(nFormat, es);
	return sBuffer;
}

CString CAutoRichEditCtrlFx::GetRTF()
{
	return this->_StreamOutEx(SF_RTF);
}

CString CAutoRichEditCtrlFx::GetTXT()
{
	return this->_StreamOutEx(SF_TEXT);
}

void CAutoRichEditCtrlFx::SetRTF(LPCTSTR lpRTF, int nStreamType)
{
	EDITSTREAM es;
	ZeroMemory(&es, sizeof(EDITSTREAM));

#ifdef _UNICODE
	if((nStreamType & SF_RTF) != 0) es.pfnCallback = CBStreamInRTF;
	else
#endif
	es.pfnCallback = CBStreamIn;

	LPCTSTR lpData = ((lpRTF != NULL) ? lpRTF : _T(""));
	m_strStreamInCache = lpData;
	es.dwCookie = (DWORD_PTR)&m_strStreamInCache;

	this->StreamIn(nStreamType, es);
}

DWORD CALLBACK CAutoRichEditCtrlFx::CBStreamIn(DWORD_PTR dwCookie, LPBYTE pbBuff, LONG cb, LONG *pcb)
{
	CString *pstr = (CString *)dwCookie;
	ASSERT(pstr != NULL); if(pstr == NULL) return 0;

	if((pstr->GetLength() * (int)(sizeof(TCHAR))) < cb)
	{
		*pcb = pstr->GetLength() * sizeof(TCHAR);
		memcpy(pbBuff, (LPCTSTR)*pstr, *pcb);
		pstr->Empty();
	}
	else
	{
		*pcb = cb;
		memcpy(pbBuff, (LPCTSTR)*pstr, *pcb);
		*pstr = pstr->Right(pstr->GetLength() - (cb / sizeof(TCHAR)));
	}

	return 0;
}

DWORD CALLBACK CAutoRichEditCtrlFx::CBStreamOut(DWORD_PTR dwCookie, LPBYTE pbBuff, LONG cb, LONG *pcb)
{
	UNREFERENCED_PARAMETER(pcb);

	CString *psEntry = (CString *)dwCookie;
	ASSERT(psEntry != NULL); if(psEntry == NULL) return 0;

	CString tmpEntry = (LPCTSTR)pbBuff;

	if(cb != 0) *psEntry += tmpEntry.Left(cb / sizeof(TCHAR));
	return 0;
}

#ifdef _UNICODE
DWORD CALLBACK CAutoRichEditCtrlFx::CBStreamInRTF(DWORD_PTR dwCookie, LPBYTE pbBuff, LONG cb, LONG *pcb)
{
	CString *pstr = (CString *)dwCookie;
	LPBYTE pstrb;
	int i;
	ASSERT(pstr != NULL); if(pstr == NULL) return 0;

	pstrb = (LPBYTE)(LPCTSTR)*pstr;
	*pcb = pstr->GetLength();
	if(*pcb < cb)
	{
		i = *pcb;
		while(i--)
		{
			*pbBuff++ = *pstrb;
			pstrb += 2;
		}
		pstr->Empty();
	}
	else
	{
		*pcb = cb;
		i = *pcb;
		while(i--)
		{
			*pbBuff++ = *pstrb;
			pstrb += 2;
		}
		*pstr = pstr->Right(pstr->GetLength() - cb);
	}

	return 0;
}

_AFX_RICHEDITEX_STATE::_AFX_RICHEDITEX_STATE()
{
	m_hInstRichEdit20 = NULL;
}

_AFX_RICHEDITEX_STATE::~_AFX_RICHEDITEX_STATE()
{
	if(m_hInstRichEdit20 != NULL) ::FreeLibrary(m_hInstRichEdit20);
	m_hInstRichEdit20 = NULL;
}

_AFX_RICHEDITEX_STATE _afxRichEditStateEx;

BOOL PASCAL AfxInitRichEditEx()
{
	if(! ::AfxInitRichEdit()) return FALSE;

	_AFX_RICHEDITEX_STATE *l_pState = &_afxRichEditStateEx;

	if(l_pState->m_hInstRichEdit20 == NULL)
		l_pState->m_hInstRichEdit20 = LoadLibrary(_T("RICHED20.DLL"));

	return (l_pState->m_hInstRichEdit20 != NULL);
}
#endif

BOOL CAutoRichEditCtrlFx::PreTranslateMessage(MSG* pMsg)
{
	if(pMsg->message == WM_KEYDOWN)
	{
		if(_HandleKey(static_cast<int>(pMsg->wParam), true)) return TRUE;
	}
	else if(pMsg->message == WM_KEYUP)
	{
		if(_HandleKey(static_cast<int>(pMsg->wParam), false)) return TRUE;
	}

	return CRichEditCtrl::PreTranslateMessage(pMsg);
}

bool CAutoRichEditCtrlFx::_HandleKey(int vk, bool bDown)
{
	if(::GetFocus() != m_hWnd) { ASSERT(FALSE); return false; }

	bool bCtrl = ((GetKeyState(VK_CONTROL) & 0x8000) != 0);
	bool bShift = ((GetKeyState(VK_SHIFT) & 0x8000) != 0);
	bool bAlt = ((GetKeyState(VK_MENU) & 0x8000) != 0);
	// bool bNumLock = ((GetKeyState(VK_NUMLOCK) & 1) != 0);

	if(m_bPlainTextOnly && ((bCtrl && !bAlt && (vk == 'V')) ||
		(bShift && !bAlt && (vk == VK_INSERT)))) // Ctrl may be pressed
		// (!bCtrl && bShift && !bAlt && !bNumLock && (vk == VK_NUMPAD0))))
	{
		if(bDown) PasteTextOnly();
		return true;
	}

	return false;
}

void CAutoRichEditCtrlFx::PasteTextOnly()
{
	// Wine doesn't support EM_PASTESPECIAL:
	// https://bugs.winehq.org/show_bug.cgi?id=14530
	if(AU_IsWine() != FALSE)
	{
		CHARRANGE crOld, crNew;
		ZeroMemory(&crOld, sizeof(CHARRANGE));
		ZeroMemory(&crNew, sizeof(CHARRANGE));

		GetSel(crOld);
		CString strOld = GetTXT();

		PasteSpecial(CF_TEXT); // Future Wine version may support it

		GetSel(crNew);
		CString strNew = GetTXT();

		if((strOld == strNew) && (memcmp(&crOld, &crNew, sizeof(CHARRANGE)) == 0))
			Paste();
	}
	else PasteSpecial(CF_TEXT);
}
