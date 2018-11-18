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
#include "PwSafe.h"
#include "LanguagesDlg.h"

#include "../KeePassLibCpp/PwManager.h"
#include "../KeePassLibCpp/Util/TranslateEx.h"
#include "Util/PrivateConfigEx.h"
#include "NewGUI/NewGUICommon.h"
#include "NewGUI/TaskDialog/VistaTaskDialog.h"
#include "Util/CmdLine/Executable.h"
#include "Util/WinUtil.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////

CLanguagesDlg::CLanguagesDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CLanguagesDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CLanguagesDlg)
	//}}AFX_DATA_INIT
}

void CLanguagesDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CLanguagesDlg)
	DDX_Control(pDX, IDC_BTN_GETLANGUAGE, m_btGetLang);
	DDX_Control(pDX, IDC_BTN_OPENFOLDER, m_btOpenFolder);
	DDX_Control(pDX, IDC_LANGUAGES_LIST, m_listLang);
	DDX_Control(pDX, IDCANCEL, m_btClose);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CLanguagesDlg, CDialog)
	//{{AFX_MSG_MAP(CLanguagesDlg)
	ON_NOTIFY(NM_CLICK, IDC_LANGUAGES_LIST, OnClickLanguagesList)
	ON_BN_CLICKED(IDC_BTN_GETLANGUAGE, OnBtnGetLanguage)
	ON_BN_CLICKED(IDC_BTN_OPENFOLDER, OnBtnOpenFolder)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////

bool CLanguagesDlg::InitEx(HWND hParent) 
{
	bool bRet = true;

	std_string strDir = SU_DriveLetterToUpper(Executable::instance().getPathOnly());
	std_string strFilter = strDir + _T("*.lng");

	CFileFind ff;
	if(ff.FindFile(strFilter.c_str(), 0) != FALSE)
	{
		std_string str = TRL("One or more language files have been found in the KeePass application directory.");
		str += _T("\r\n\r\n");

		std_string strFiles;
		DWORD cFiles = 0;
		const DWORD cMaxFL = 6;
		BOOL bMore = TRUE;
		while(bMore != FALSE)
		{
			bMore = ff.FindNextFile();

			++cFiles;
			if(cFiles <= cMaxFL)
			{
				strFiles += ((cFiles == cMaxFL) ? _T("...") : (LPCTSTR)ff.GetFilePath());
				strFiles += _T("\r\n");
			}
		}
		str += strFiles;
		str += _T("\r\n");

		str += TRL("Loading language files directly from the application directory is not supported. Language files should instead be stored in the 'Languages' folder of the application directory.");
		str += _T("\r\n\r\n");
		str += TRL("Do you want to open the application directory (in order to move or delete language files)?");

		if(::MessageBox(hParent, str.c_str(), PWM_PRODUCT_NAME_SHORT, MB_ICONWARNING |
			MB_YESNO) == IDYES)
		{
			CString strUrl = strDir.c_str();
			if(strDir.size() > 3) strUrl = strUrl.TrimRight(_T('\\'));
			strUrl = CString(_T("cmd://\"")) + strUrl + _T("\"");
			OpenUrlEx(strUrl, hParent);

			bRet = false;
		}
	}

	ff.Close();
	return bRet;
}

BOOL CLanguagesDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();

	NewGUI_TranslateCWnd(this);
	EnumChildWindows(this->m_hWnd, NewGUI_TranslateWindowCb, 0);
	
	NewGUI_XPButton(m_btClose, IDB_CANCEL, IDB_CANCEL);
	NewGUI_XPButton(m_btGetLang, IDB_LANGUAGE, IDB_LANGUAGE);
	NewGUI_XPButton(m_btOpenFolder, IDB_TB_OPEN, IDB_TB_OPEN);

	NewGUI_ConfigSideBanner(&m_banner, this);
	m_banner.SetIcon(AfxGetApp()->LoadIcon(IDI_WORLD),
		KCSB_ICON_LEFT | KCSB_ICON_VCENTER);
	m_banner.SetTitle(TRL("Select Language"));
	m_banner.SetCaption(TRL("Here you can change the user interface language."));

	RECT rcList;
	m_listLang.GetClientRect(&rcList);
	const int wList = rcList.right - rcList.left - GetSystemMetrics(SM_CXVSCROLL);
	const int w2 = (wList * 2) / 20;
	const int w3 = (wList * 3) / 20;
	const int w5 = (wList * 5) / 20;
	m_listLang.InsertColumn(0, TRL("Installed Languages"), LVCFMT_LEFT, w5, 0);
	m_listLang.InsertColumn(1, TRL("Version"), LVCFMT_LEFT, w2, 1);
	m_listLang.InsertColumn(2, TRL("Author"), LVCFMT_LEFT, w5, 2);
	m_listLang.InsertColumn(3, TRL("Contact"), LVCFMT_LEFT, w5, 3);
	m_listLang.InsertColumn(4, TRL("File"), LVCFMT_LEFT, w3, 4);

	// m_ilIcons.Create(CPwSafeApp::GetClientIconsResourceID(), 16, 1, RGB(255,0,255));
	CPwSafeApp::CreateHiColorImageList(&m_ilIcons, IDB_CLIENTICONS_EX, 16);
	m_listLang.SetImageList(&m_ilIcons, LVSIL_SMALL);

	m_listLang.PostMessage(LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_SI_REPORT |
		LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_ONECLICKACTIVATE |
		LVS_EX_UNDERLINEHOT | LVS_EX_INFOTIP);

	m_listLang.DeleteAllItems();

	LV_ITEM lvi;
	ZeroMemory(&lvi, sizeof(LV_ITEM));
	lvi.iItem = m_listLang.InsertItem(LVIF_TEXT | LVIF_IMAGE, m_listLang.GetItemCount(),
		_T("English"), 0, 0, 1, NULL);

	CString strTemp;
	
	strTemp = PWM_VERSION_STR;
	lvi.iSubItem = 1; lvi.mask = LVIF_TEXT;
	lvi.pszText = (LPTSTR)(LPCTSTR)strTemp;
	m_listLang.SetItem(&lvi);

	strTemp = PWMX_ENGLISH_AUTHOR;
	lvi.iSubItem = 2; lvi.mask = LVIF_TEXT;
	lvi.pszText = (LPTSTR)(LPCTSTR)strTemp;
	m_listLang.SetItem(&lvi);

	strTemp = PWMX_ENGLISH_CONTACT;
	lvi.iSubItem = 3; lvi.mask = LVIF_TEXT;
	lvi.pszText = (LPTSTR)(LPCTSTR)strTemp;
	m_listLang.SetItem(&lvi);

	strTemp = TRL("Built-in");
	lvi.iSubItem = 4; lvi.mask = LVIF_TEXT;
	lvi.pszText = (LPTSTR)(LPCTSTR)strTemp;
	m_listLang.SetItem(&lvi);

	const std_string strActive = GetCurrentTranslationTable();

	std_string strFilter = SU_DriveLetterToUpper(Executable::instance().getPathOnly());
	strFilter += PWM_DIR_LANGUAGES;
	strFilter += _T("\\*.lng");

	CFileFind ff;
	BOOL bMore = ff.FindFile(strFilter.c_str(), 0);
	while(bMore != FALSE)
	{
		bMore = ff.FindNextFile();

		// Ignore KeePass 2.x LNGX files (these are found even though
		// "*.lng" is specified as file mask)
		CString strFileName = ff.GetFileName();
		strFileName = strFileName.MakeLower();
		if((strFileName.GetLength() >= 5) && (strFileName.Right(5) == _T(".lngx")))
			continue;

		CString strID = ff.GetFileTitle();
		strID = strID.MakeLower();
		if((strID != _T("standard")) && (strID != _T("english")))
		{
			VERIFY(LoadTranslationTable((LPCTSTR)ff.GetFileTitle()));

			strTemp = (LPCTSTR)ff.GetFileTitle();
			// strTemp += _T(" - "); // Name is used as identifier
			// strTemp += TRL("~LANGUAGENAME");

			lvi.iItem = m_listLang.InsertItem(LVIF_TEXT | LVIF_IMAGE,
				m_listLang.GetItemCount(), strTemp, 0, 0, 1, NULL);

			strTemp = TRL("~LANGUAGEVERSION");
			if(strTemp == _T("~LANGUAGEVERSION")) strTemp.Empty();
			lvi.iSubItem = 1; lvi.mask = LVIF_TEXT;
			lvi.pszText = (LPTSTR)(LPCTSTR)strTemp;
			m_listLang.SetItem(&lvi);

			strTemp = TRL("~LANGUAGEAUTHOR");
			if(strTemp == _T("~LANGUAGEAUTHOR")) strTemp.Empty();
			lvi.iSubItem = 2; lvi.mask = LVIF_TEXT;
			lvi.pszText = (LPTSTR)(LPCTSTR)strTemp;
			m_listLang.SetItem(&lvi);

			strTemp = TRL("~LANGUAGEAUTHOREMAIL");
			if(strTemp == _T("~LANGUAGEAUTHOREMAIL")) strTemp.Empty();
			lvi.iSubItem = 3; lvi.mask = LVIF_TEXT;
			lvi.pszText = (LPTSTR)(LPCTSTR)strTemp;
			m_listLang.SetItem(&lvi);

			strTemp = ff.GetFilePath();
			lvi.iSubItem = 4; lvi.mask = LVIF_TEXT;
			lvi.pszText = (LPTSTR)(LPCTSTR)strTemp;
			m_listLang.SetItem(&lvi);
		}
	}

	ff.Close();

	VERIFY(LoadTranslationTable(strActive.c_str()));
	return TRUE;
}

void CLanguagesDlg::OnOK() 
{
	CDialog::OnOK();
}

void CLanguagesDlg::OnCancel() 
{
	CDialog::OnCancel();
}

void CLanguagesDlg::OnClickLanguagesList(NMHDR* pNMHDR, LRESULT* pResult) 
{
	UNREFERENCED_PARAMETER(pNMHDR);
	*pResult = 0;

	CPoint mousePoint;
	GetCursorPos(&mousePoint);
	m_listLang.ScreenToClient(&mousePoint);

	UINT nFlags = 0;
	const int iHitItem = m_listLang.HitTest(mousePoint, &nFlags);
	if((iHitItem < 0) || ((nFlags & LVHT_ONITEM) == 0)) return;

	CString strLang = m_listLang.GetItemText(iHitItem, 0);
	_LoadLanguage(strLang);
}

void CLanguagesDlg::_LoadLanguage(LPCTSTR szLang)
{
	CPrivateConfigEx cConfig(TRUE);

	if(_tcscmp(szLang, _T("English")) != 0)
	{
		std_string strFile =  SU_DriveLetterToUpper(Executable::instance().getPathOnly());
		strFile += PWM_DIR_LANGUAGES;
		strFile += _T("\\");
		strFile += szLang;
		strFile += _T(".lng");

		FILE* fp = NULL;
		_tfopen_s(&fp, strFile.c_str(), _T("rb"));
		ASSERT(fp != NULL);
		if(fp == NULL)
		{
			MessageBox(TRL("Language file cannot be opened!"), TRL("Loading error"), MB_OK | MB_ICONWARNING);
			return;
		}
		fclose(fp);

		if(cConfig.Set(PWMKEY_LANG, szLang) == FALSE)
		{
			MessageBox(TRL("Language file cannot be activated!"), TRL("Loading error"), MB_OK | MB_ICONWARNING);
			return;
		}
	}
	else cConfig.Set(PWMKEY_LANG, _T("Standard"));

	CString str = TRL("The selected language has been activated. KeePass must be restarted in order to load the language.");
	str += _T("\r\n\r\n");
	str += TRL("Do you wish to restart KeePass now?");

	int iResult = CVistaTaskDialog::ShowMessageBox(this->m_hWnd, TRL("Restart KeePass?"),
		str, MTDI_QUESTION, TRL("&Yes"), IDOK, TRL("&No"), IDCANCEL);
	if(iResult < 0)
		iResult = MessageBox(str, TRL("Restart KeePass?"), MB_YESNO | MB_ICONQUESTION);
	if((iResult == IDOK) || (iResult == IDYES)) CDialog::OnOK();
}

void CLanguagesDlg::OnBtnGetLanguage() 
{
	OpenUrlEx(PWM_URL_TRL, this->m_hWnd);
	OnCancel();
}

void CLanguagesDlg::OnBtnOpenFolder() 
{
	std_string str =  SU_DriveLetterToUpper(Executable::instance().getPathOnly());
	str += PWM_DIR_LANGUAGES;

	if(GetFileAttributes(str.c_str()) == INVALID_FILE_ATTRIBUTES)
		CreateDirectory(str.c_str(), NULL);

	str = std_string(_T("cmd://\"")) + str + _T("\"");
	OpenUrlEx(str.c_str(), this->m_hWnd);
	OnCancel();
}
