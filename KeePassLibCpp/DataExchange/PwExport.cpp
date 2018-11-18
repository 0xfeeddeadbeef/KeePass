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
#include "PwExport.h"
#include <vector>
#include <boost/lexical_cast.hpp>

#include "../Util/MemUtil.h"
#include "../Util/Base64.h"
#include "../Util/TranslateEx.h"

CPwExport::CPwExport()
{
	m_pMgr = NULL;
	m_nFormat = 0;
	m_fp = NULL;

	SetNewLineSeq(FALSE);

	ZeroMemory(&m_aDefaults[PWEXP_TXT], sizeof(PWEXPORT_OPTIONS));
	m_aDefaults[PWEXP_TXT].bGroup = TRUE;
	m_aDefaults[PWEXP_TXT].bTitle = TRUE;
	m_aDefaults[PWEXP_TXT].bUserName = TRUE;
	m_aDefaults[PWEXP_TXT].bURL = TRUE;
	m_aDefaults[PWEXP_TXT].bPassword = TRUE;
	m_aDefaults[PWEXP_TXT].bNotes = TRUE;

	ZeroMemory(&m_aDefaults[PWEXP_HTML], sizeof(PWEXPORT_OPTIONS));
	m_aDefaults[PWEXP_HTML].bGroup = TRUE;
	m_aDefaults[PWEXP_HTML].bTitle = TRUE;
	m_aDefaults[PWEXP_HTML].bUserName = TRUE;
	m_aDefaults[PWEXP_HTML].bURL = TRUE;
	m_aDefaults[PWEXP_HTML].bPassword = TRUE;
	m_aDefaults[PWEXP_HTML].bNotes = TRUE;

	ZeroMemory(&m_aDefaults[PWEXP_XML], sizeof(PWEXPORT_OPTIONS));
	m_aDefaults[PWEXP_XML].bGroup = TRUE;
	m_aDefaults[PWEXP_XML].bGroupTree = TRUE;
	m_aDefaults[PWEXP_XML].bTitle = TRUE;
	m_aDefaults[PWEXP_XML].bUserName = TRUE;
	m_aDefaults[PWEXP_XML].bURL = TRUE;
	m_aDefaults[PWEXP_XML].bPassword = TRUE;
	m_aDefaults[PWEXP_XML].bNotes = TRUE;
	m_aDefaults[PWEXP_XML].bUUID = TRUE;
	m_aDefaults[PWEXP_XML].bImage = TRUE;
	m_aDefaults[PWEXP_XML].bCreationTime = TRUE;
	m_aDefaults[PWEXP_XML].bLastAccTime = TRUE;
	m_aDefaults[PWEXP_XML].bLastModTime = TRUE;
	m_aDefaults[PWEXP_XML].bExpireTime = TRUE;
	m_aDefaults[PWEXP_XML].bAttachment = TRUE;

	ZeroMemory(&m_aDefaults[PWEXP_CSV], sizeof(PWEXPORT_OPTIONS));
	m_aDefaults[PWEXP_CSV].bTitle = TRUE;
	m_aDefaults[PWEXP_CSV].bUserName = TRUE;
	m_aDefaults[PWEXP_CSV].bURL = TRUE;
	m_aDefaults[PWEXP_CSV].bPassword = TRUE;
	m_aDefaults[PWEXP_CSV].bNotes = TRUE;

	ZeroMemory(&m_aDefaults[PWEXP_KEEPASS], sizeof(PWEXPORT_OPTIONS));
	m_aDefaults[PWEXP_KEEPASS].bGroup = TRUE;
	m_aDefaults[PWEXP_KEEPASS].bGroupTree = TRUE;
	m_aDefaults[PWEXP_KEEPASS].bTitle = TRUE;
	m_aDefaults[PWEXP_KEEPASS].bUserName = TRUE;
	m_aDefaults[PWEXP_KEEPASS].bURL = TRUE;
	m_aDefaults[PWEXP_KEEPASS].bPassword = TRUE;
	m_aDefaults[PWEXP_KEEPASS].bNotes = TRUE;
	m_aDefaults[PWEXP_KEEPASS].bUUID = TRUE;
	m_aDefaults[PWEXP_KEEPASS].bImage = TRUE;
	m_aDefaults[PWEXP_KEEPASS].bCreationTime = TRUE;
	m_aDefaults[PWEXP_KEEPASS].bLastAccTime = TRUE;
	m_aDefaults[PWEXP_KEEPASS].bLastModTime = TRUE;
	m_aDefaults[PWEXP_KEEPASS].bExpireTime = TRUE;
	m_aDefaults[PWEXP_KEEPASS].bAttachment = TRUE;
	m_aDefaults[PWEXP_KEEPASS].bExportBackups = TRUE;
}

CPwExport::~CPwExport()
{
	m_pMgr = NULL;
	m_nFormat = 0;
	m_fp = NULL;
}

void CPwExport::SetManager(CPwManager *pMgr)
{
	ASSERT(pMgr != NULL);
	m_pMgr = pMgr;
}

void CPwExport::SetFormat(int nFormat)
{
	ASSERT((nFormat > PWEXP_NULL) && (nFormat < PWEXP_LAST));
	if((nFormat > PWEXP_NULL) && (nFormat < PWEXP_LAST))
		m_nFormat = nFormat;
}

void CPwExport::SetNewLineSeq(BOOL bWindows)
{
	if(bWindows == TRUE) m_pszNewLine = _T("\r\n");
	else m_pszNewLine = _T("\n");
}

void CPwExport::_ExpStr(LPCTSTR lpString)
{
	if((lpString != NULL) && (_tcslen(lpString) > 0))
	{
		UTF8_BYTE *pUtf8String = _StringToUTF8(lpString);

		ASSERT(pUtf8String != NULL);
		if(pUtf8String != NULL)
		{
			fwrite(pUtf8String, 1, strlen((char *)pUtf8String), m_fp);
			SAFE_DELETE_ARRAY(pUtf8String);
		}
	}
}

void CPwExport::_ExpLine(LPCTSTR lpString)
{
	_ExpStr(lpString);
	_ExpStr(m_pszNewLine);
}

void CPwExport::_ExpXmlStr(LPCTSTR lpString, DWORD dwXmlEncFlags)
{
	std::basic_string<TCHAR> str = MakeSafeXmlString(lpString, dwXmlEncFlags);
	if(str.size() == 0) return;

	UTF8_BYTE *pUtf8String = _StringToUTF8(str.c_str());
	fwrite(pUtf8String, 1, strlen((char *)pUtf8String), m_fp);
	SAFE_DELETE_ARRAY(pUtf8String);
}

void CPwExport::_ExpHtmlStr(LPCTSTR lpString, DWORD dwXmlEncFlags)
{
	if(lpString[0] == 0) _ExpStr(_T("&nbsp;"));
	else _ExpXmlStr(lpString, dwXmlEncFlags);
}

void CPwExport::_ExpStrIf(BOOL bCondition, LPCTSTR lpString)
{
	if(bCondition == TRUE)
	{
		if(m_bOneSkipped == TRUE) _ExpStr(m_lpSep);
		else m_bOneSkipped = TRUE;

		if(m_pOptions->bEncodeNewlines == TRUE)
		{
			CString str = lpString;
			str.Replace(_T("\r"), _T("\\r"));
			str.Replace(_T("\n"), _T("\\n"));
			_ExpStr(str);
		}
		else _ExpStr(lpString);
	}
}

void CPwExport::_ExpXmlStrIf(BOOL bCondition, LPCTSTR lpString)
{
	if(bCondition == TRUE)
	{
		if(m_bOneSkipped == TRUE) _ExpStr(m_lpSep);
		else m_bOneSkipped = TRUE;

		if(m_pOptions->bEncodeNewlines == TRUE)
		{
			CString str = lpString;
			str.Replace(_T("\r"), _T("\\r"));
			str.Replace(_T("\n"), _T("\\n"));
			_ExpXmlStr(str);
		}
		else _ExpXmlStr(lpString);
	}
}

void CPwExport::_ExpHtmlStrIf(BOOL bCondition, LPCTSTR lpString)
{
	if(bCondition == TRUE)
	{
		if(m_bOneSkipped == TRUE) _ExpStr(m_lpSep);
		else m_bOneSkipped = TRUE;

		if(m_pOptions->bEncodeNewlines == TRUE)
		{
			CString str = lpString;
			str.Replace(_T("\r"), _T("\\r"));
			str.Replace(_T("\n"), _T("\\n"));
			_ExpHtmlStr(str);
		}
		else _ExpHtmlStr(lpString);
	}
}

void CPwExport::_ExpCsvStrIf(BOOL bCondition, LPCTSTR lpString)
{
	if(bCondition == TRUE)
	{
		if(m_bOneSkipped == TRUE) _ExpStr(m_lpSep);
		else m_bOneSkipped = TRUE;

		CString str = lpString;
		str.Replace(_T("\\"), _T("\\\\"));
		str.Replace(_T("\""), _T("\\\""));

		if(m_pOptions->bEncodeNewlines == TRUE)
		{
			str.Replace(_T("\r"), _T("\\r"));
			str.Replace(_T("\n"), _T("\\n"));
			_ExpStr(str);
		}
		else _ExpStr(str);
	}
}

void CPwExport::_ExpResetSkip()
{
	m_bOneSkipped = FALSE;
}

void CPwExport::_ExpSetSep(LPCTSTR lpSep)
{
	if(lpSep == NULL) m_lpSep = _T("");
	else m_lpSep = lpSep;
}

BOOL CPwExport::ExportAll(const TCHAR *pszFile, const PWEXPORT_OPTIONS *pOptions, CPwManager *pStoreMgr)
{
	ASSERT(pszFile != NULL);
	return ExportGroup(pszFile, DWORD_MAX, pOptions, pStoreMgr);
}

CString CPwExport::MakeGroupTreeString(DWORD dwGroupId, bool bXmlEncode) const
{
	CString str;

	PW_GROUP *pg = m_pMgr->GetGroupById(dwGroupId);
	ASSERT(pg != NULL); if(pg == NULL) return str;

	const USHORT usLevel = pg->usLevel;
	ASSERT(usLevel != 0xFFFF); if(usLevel == 0xFFFF) return str;
	if(usLevel == 0) return str;
	DWORD *pdwIndices = new DWORD[usLevel + 2];
	ASSERT(pdwIndices != NULL); if(pdwIndices == NULL) return str;

	if(m_pMgr->GetGroupTree(dwGroupId, pdwIndices) == TRUE)
	{
		for(USHORT i = 0; i < usLevel; ++i)
		{
			pg = m_pMgr->GetGroup(pdwIndices[i]);

			if(pg != NULL)
			{
				if(i != 0) str += _T("\\");

				CString strTemp;
				if(bXmlEncode)
				{
					std::basic_string<TCHAR> strGrp = MakeSafeXmlString(pg->pszGroupName);
					strTemp = strGrp.c_str();
				}
				else strTemp = pg->pszGroupName;

				strTemp.Replace(_T("\\"), _T("/")); // Avoid ambiguity with tree separator
				str += strTemp;
			}
		}
	}

	SAFE_DELETE_ARRAY(pdwIndices);
	return str;
}

BOOL CPwExport::ExportGroup(const TCHAR *pszFile, DWORD dwGroupId, const PWEXPORT_OPTIONS *pOptions, CPwManager *pStoreMgr)
{
	FILE *fp = NULL;
	DWORD i, j, dwThisId, dwNumberOfGroups;
	PW_ENTRY *p;
	PW_GROUP *pg;
	BYTE aInitUTF8[3] = { 0xEF, 0xBB, 0xBF };
	CString str, strUUID, strImage, strCreationTime, strLastAccTime, strLastModTime;
	CString strExpireTime, strGroupTree;
	BYTE *pbEncodedAttachment = NULL;
	PW_TIME tNever;
	std::vector<DWORD> aGroupIds;
	USHORT usLevel = 0;
	BOOL bReturn = TRUE;

	ASSERT(m_pMgr != NULL); if(m_pMgr == NULL) return FALSE;
	m_pMgr->GetNeverExpireTime(&tNever);

	ASSERT(pszFile != NULL); if(pszFile == NULL) return FALSE;

	if(m_nFormat == PWEXP_KEEPASS) { ASSERT(pStoreMgr != NULL); if(pStoreMgr == NULL) return FALSE; }

	if(pOptions == NULL)
	{
		if(m_nFormat == PWEXP_TXT) pOptions = &m_aDefaults[PWEXP_TXT];
		else if(m_nFormat == PWEXP_HTML) pOptions = &m_aDefaults[PWEXP_HTML];
		else if(m_nFormat == PWEXP_XML) pOptions = &m_aDefaults[PWEXP_XML];
		else if(m_nFormat == PWEXP_CSV) pOptions = &m_aDefaults[PWEXP_CSV];
		else if(m_nFormat == PWEXP_KEEPASS) pOptions = &m_aDefaults[PWEXP_KEEPASS];
		else { ASSERT(FALSE); return FALSE; }
	}
	m_pOptions = pOptions;

	dwNumberOfGroups = m_pMgr->GetNumberOfGroups();
	const DWORD dwInvalidId1 = m_pMgr->GetGroupId(PWS_BACKUPGROUP_SRC);
	const DWORD dwInvalidId2 = m_pMgr->GetGroupId(PWS_BACKUPGROUP);

	if(dwGroupId != DWORD_MAX)
	{
		i = m_pMgr->GetGroupByIdN(dwGroupId);
		ASSERT(i != DWORD_MAX); if(i == DWORD_MAX) return FALSE;

		usLevel = m_pMgr->GetGroup(i)->usLevel;

		while(true)
		{
			pg = m_pMgr->GetGroup(i);
			ASSERT(pg != NULL); if(pg == NULL) break;

			if((pg->uGroupId != dwGroupId) && (pg->usLevel <= usLevel)) break;

			const DWORD dwToAdd = pg->uGroupId;
			aGroupIds.push_back(dwToAdd);

			++i;
			if(i == dwNumberOfGroups) break;
		}
	}
	else
	{
		for(i = 0; i < dwNumberOfGroups; ++i)
		{
			pg = m_pMgr->GetGroup(i);
			ASSERT(pg != NULL); if(pg == NULL) continue;

			const DWORD dwToAdd = pg->uGroupId;
			aGroupIds.push_back(dwToAdd);
		}
	}

	if(m_nFormat != PWEXP_KEEPASS)
	{
		fp = NULL;
		_tfopen_s(&fp, pszFile, _T("wb"));
		if(fp == NULL) { aGroupIds.clear(); return FALSE; }
		m_fp = fp;

		fwrite(aInitUTF8, 1, 3, fp);
	}

	if(m_nFormat == PWEXP_TXT)
	{
	}
	else if(m_nFormat == PWEXP_HTML)
	{
		_ExpLine(_T("<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Transitional//EN\""));
		_ExpLine(_T("\t\"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd\">"));

		LPCTSTR lpLang = TRL("~LANGISO6391CODE");
		if((lpLang == NULL) || (lpLang[0] == 0) || (_tcslen(lpLang) > 4))
			lpLang = _T("en");
		_ExpStr(_T("<html xmlns=\"http://www.w3.org/1999/xhtml\" lang=\""));
		_ExpXmlStr(lpLang);
		_ExpStr(_T("\" xml:lang=\""));
		_ExpXmlStr(lpLang);
		_ExpLine(_T("\">"));

		_ExpLine(_T("<head>"));
		_ExpLine(_T("<meta http-equiv=\"Content-Type\" content=\"text/html; charset=UTF-8\" />"));
		_ExpStr(_T("<title>"));
		_ExpXmlStr(TRL("Entry List"));
		_ExpStr(_T(" - "));
		_ExpXmlStr(PWM_PRODUCT_NAME_SHORT);
		_ExpLine(_T("</title>"));
		_ExpLine(_T("<meta http-equiv=\"expires\" content=\"0\" />"));
		_ExpLine(_T("<meta http-equiv=\"cache-control\" content=\"no-cache\" />"));
		_ExpLine(_T("<meta http-equiv=\"pragma\" content=\"no-cache\" />"));

		_ExpLine(_T("<style type=\"text/css\">"));
		_ExpLine(_T("/* <![CDATA[ */"));

		_ExpLine(_T("body, p, div, h1, h2, h3, h4, h5, h6, ol, ul, li, td, th, dd, dt, a {"));
		_ExpLine(_T("\tfont-family: \"Tahoma\", \"MS Sans Serif\", \"Sans Serif\", \"Verdana\", sans-serif;"));
		_ExpLine(_T("\tfont-size: 10pt;"));
		_ExpLine(_T("}"));

		_ExpLine(_T("span.fserif {"));
		_ExpLine(_T("\tfont-family: \"Times New Roman\", serif;"));
		_ExpLine(_T("}"));

		_ExpLine(_T("h1 { font-size: 2em; }"));
		_ExpLine(_T("h2 {"));
		_ExpLine(_T("\tfont-size: 1.5em;"));
		_ExpLine(_T("\tcolor: #000000;"));
		_ExpLine(_T("\tbackground-color: #D0D0D0;"));
		_ExpLine(_T("\tpadding-left: 2pt;"));
		_ExpLine(_T("\tpadding-right: 2pt;")); // RTL support
		_ExpLine(_T("}"));
		_ExpLine(_T("h3 {"));
		_ExpLine(_T("\tfont-size: 1.2em;"));
		_ExpLine(_T("\tcolor: #000000;"));
		_ExpLine(_T("\tbackground-color: #D0D0D0;"));
		_ExpLine(_T("\tpadding-left: 2pt;"));
		_ExpLine(_T("\tpadding-right: 2pt;")); // RTL support
		_ExpLine(_T("}"));
		_ExpLine(_T("h4 { font-size: 1em; }"));
		_ExpLine(_T("h5 { font-size: 0.89em; }"));
		_ExpLine(_T("h6 { font-size: 0.6em; }"));

		_ExpLine(_T("table {"));
		_ExpLine(_T("\twidth: 100%;"));
		_ExpLine(_T("\ttable-layout: fixed;"));
		_ExpLine(_T("}"));

		_ExpLine(_T("th {"));
		_ExpLine(_T("\ttext-align: left;"));
		_ExpLine(_T("\tvertical-align: top;"));
		_ExpLine(_T("\tfont-weight: bold;"));
		_ExpLine(_T("}"));

		_ExpLine(_T("td {"));
		_ExpLine(_T("\ttext-align: left;"));
		_ExpLine(_T("\tvertical-align: top;"));
		_ExpLine(_T("}"));

		_ExpLine(_T("a:visited {"));
		_ExpLine(_T("\ttext-decoration: none;"));
		_ExpLine(_T("\tcolor: #0000DD;"));
		_ExpLine(_T("}"));
		_ExpLine(_T("a:active {"));
		_ExpLine(_T("\ttext-decoration: none;"));
		_ExpLine(_T("\tcolor: #6699FF;"));
		_ExpLine(_T("}"));
		_ExpLine(_T("a:link {"));
		_ExpLine(_T("\ttext-decoration: none;"));
		_ExpLine(_T("\tcolor: #0000DD;"));
		_ExpLine(_T("}"));
		_ExpLine(_T("a:hover {"));
		_ExpLine(_T("\ttext-decoration: underline;"));
		_ExpLine(_T("\tcolor: #6699FF;"));
		_ExpLine(_T("}"));

		_ExpLine(_T(".field_name {"));
		_ExpLine(_T("\t-webkit-hyphens: auto;"));
		_ExpLine(_T("\t-moz-hyphens: auto;"));
		_ExpLine(_T("\t-ms-hyphens: auto;"));
		_ExpLine(_T("\thyphens: auto;"));
		_ExpLine(_T("}"));
		_ExpLine(_T(".field_data {"));
		// _ExpLine(_T("\tword-break: break-all;"));
		_ExpLine(_T("\toverflow-wrap: break-word;"));
		_ExpLine(_T("\tword-wrap: break-word;"));
		_ExpLine(_T("}"));

		_ExpLine(_T("/* ]]> */"));
		_ExpLine(_T("</style>"));

		_ExpLine(_T("</head>"));
		_ExpLine(_T("<body>"));
		_ExpStr(_T("<h2>"));
		_ExpXmlStr(TRL("Entry List"));
		_ExpStr(_T(" &#8211; "));
		_ExpXmlStr(PWM_PRODUCT_NAME_SHORT);
		_ExpLine(_T("</h2>"));
		_ExpLine(_T("<table><tr>"));

		DWORD dwEquiCols = 0;
		if(pOptions->bGroup != FALSE) ++dwEquiCols;
		if(pOptions->bGroupTree != FALSE) ++dwEquiCols;
		if(pOptions->bTitle != FALSE) ++dwEquiCols;
		if(pOptions->bUserName != FALSE) ++dwEquiCols;
		if(pOptions->bURL != FALSE) ++dwEquiCols;
		if(pOptions->bPassword != FALSE) ++dwEquiCols;
		if(pOptions->bNotes != FALSE) dwEquiCols += 2;
		if(pOptions->bUUID != FALSE) ++dwEquiCols;
		if(pOptions->bImage != FALSE) ++dwEquiCols;
		if(pOptions->bCreationTime != FALSE) ++dwEquiCols;
		if(pOptions->bLastAccTime != FALSE) ++dwEquiCols;
		if(pOptions->bLastModTime != FALSE) ++dwEquiCols;
		if(pOptions->bExpireTime != FALSE) ++dwEquiCols;
		if(pOptions->bAttachment != FALSE) ++dwEquiCols; // Description
		if(pOptions->bAttachment != FALSE) dwEquiCols += 2; // Data

		if(dwEquiCols == 0) dwEquiCols = 1;
		int nWidth = 10000 / static_cast<int>(dwEquiCols);

		std::basic_string<TCHAR> strWidth =
			boost::lexical_cast<std::basic_string<TCHAR> >(nWidth / 100);
		strWidth += _T(".");
		strWidth += boost::lexical_cast<std::basic_string<TCHAR> >(nWidth % 100);

		std::basic_string<TCHAR> strWidth2 =
			boost::lexical_cast<std::basic_string<TCHAR> >((nWidth * 2) / 100);
		strWidth2 += _T(".");
		strWidth2 += boost::lexical_cast<std::basic_string<TCHAR> >((nWidth * 2) % 100);

		std::basic_string<TCHAR> strTH = _T("<th class=\"field_name\" style=\"width: ");
		strTH += strWidth;
		strTH += _T("%;\">");

		std::basic_string<TCHAR> strTH2 = _T("<th class=\"field_name\" style=\"width: ");
		strTH2 += strWidth2;
		strTH2 += _T("%;\">");

		std::basic_string<TCHAR> strClTH = _T("</th>");
		strClTH += m_pszNewLine;
		strClTH += strTH;

		std::basic_string<TCHAR> strClTH2 = _T("</th>");
		strClTH2 += m_pszNewLine;
		strClTH2 += strTH2;

		if((pOptions->bGroup | pOptions->bGroupTree | pOptions->bTitle |
			pOptions->bUserName | pOptions->bURL | pOptions->bPassword) != FALSE)
			_ExpStr(strTH.c_str());
		else _ExpStr(strTH2.c_str());

		_ExpSetSep(strClTH.c_str());
		_ExpResetSkip();

		_ExpXmlStrIf(pOptions->bGroup, TRL("Group"));
		_ExpXmlStrIf(pOptions->bGroupTree, TRL("Group Tree"));
		_ExpXmlStrIf(pOptions->bTitle, TRL("Title"));
		_ExpXmlStrIf(pOptions->bUserName, TRL("User Name"));
		_ExpXmlStrIf(pOptions->bURL, TRL("URL"));
		_ExpXmlStrIf(pOptions->bPassword, TRL("Password"));

		_ExpSetSep(strClTH2.c_str());
		_ExpXmlStrIf(pOptions->bNotes, TRL("Notes"));
		_ExpSetSep(strClTH.c_str());

		_ExpXmlStrIf(pOptions->bUUID, TRL("UUID"));
		_ExpXmlStrIf(pOptions->bImage, TRL("Icon"));
		_ExpXmlStrIf(pOptions->bCreationTime, TRL("Creation Time"));
		_ExpXmlStrIf(pOptions->bLastAccTime, TRL("Last Access"));
		_ExpXmlStrIf(pOptions->bLastModTime, TRL("Last Modification"));
		_ExpXmlStrIf(pOptions->bExpireTime, TRL("Expires"));
		_ExpXmlStrIf(pOptions->bAttachment, TRL("Attachment Description"));

		_ExpSetSep(strClTH2.c_str());
		_ExpXmlStrIf(pOptions->bAttachment, TRL("Attachment"));
		_ExpSetSep(strClTH.c_str());

		_ExpLine(_T("</th></tr>"));
		_ExpSetSep(NULL); // Release strClTH
	}
	else if(m_nFormat == PWEXP_XML)
	{
		_ExpStr(_T("<?xml version=\"1.0\" encoding=\"utf-8\" standalone=\"yes\"?>"));
		_ExpStr(m_pszNewLine);
		_ExpStr(_T("<pwlist>"));
		_ExpStr(m_pszNewLine);
	}
	else if(m_nFormat == PWEXP_CSV)
	{
		_ExpStr(_T("\""));

		_ExpSetSep(_T("\",\""));
		_ExpResetSkip();
		_ExpStrIf(pOptions->bGroup, _T("Group"));
		_ExpStrIf(pOptions->bGroupTree, _T("Group Tree"));
		_ExpStrIf(pOptions->bTitle, _T("Account"));
		_ExpStrIf(pOptions->bUserName, _T("Login Name"));
		_ExpStrIf(pOptions->bPassword, _T("Password"));
		_ExpStrIf(pOptions->bURL, _T("Web Site"));
		_ExpStrIf(pOptions->bNotes, _T("Comments"));
		_ExpStrIf(pOptions->bUUID, _T("UUID"));
		_ExpStrIf(pOptions->bImage, _T("Icon"));
		_ExpStrIf(pOptions->bCreationTime, _T("Creation Time"));
		_ExpStrIf(pOptions->bLastAccTime, _T("Last Access"));
		_ExpStrIf(pOptions->bLastModTime, _T("Last Modification"));
		_ExpStrIf(pOptions->bExpireTime, _T("Expires"));
		_ExpStrIf(pOptions->bAttachment, _T("Attachment Description"));
		_ExpStrIf(pOptions->bAttachment, _T("Attachment"));

		_ExpStr(_T("\""));
		_ExpStr(m_pszNewLine);
	}
	else if(m_nFormat == PWEXP_KEEPASS)
	{
		DWORD dwGroupEnum;
		PW_GROUP pwgTemplate;
		for(dwGroupEnum = 0; dwGroupEnum < (DWORD)aGroupIds.size(); dwGroupEnum++)
		{
			PW_GROUP *pgCopy = m_pMgr->GetGroupById(aGroupIds[dwGroupEnum]);
			ASSERT(pgCopy != NULL); if(pgCopy == NULL) return FALSE;
			pwgTemplate = *pgCopy;
			pwgTemplate.usLevel = (USHORT)(pwgTemplate.usLevel - usLevel);
			pStoreMgr->AddGroup(&pwgTemplate);
		}
	}
	else { ASSERT(FALSE); }

	DWORD uNumEntries = m_pMgr->GetNumberOfEntries();

	for(j = 0; j < static_cast<DWORD>(aGroupIds.size()); ++j)
	{
		dwThisId = aGroupIds[j];

		if(m_pOptions->bExportBackups == FALSE)
		{
			if(dwThisId == dwInvalidId1) continue;
			if(dwThisId == dwInvalidId2) continue;
		}

		for(i = 0; i < uNumEntries; ++i)
		{
			p = m_pMgr->GetEntry(i);
			ASSERT_ENTRY(p);

			// Are we exporting this group?
			if(p->uGroupId != dwThisId) continue;

			pg = m_pMgr->GetGroupById(p->uGroupId);
			ASSERT(pg != NULL); if(pg == NULL) continue;

			m_pMgr->UnlockEntryPassword(p);

			_UuidToString(p->uuid, &strUUID);
			strImage.Format(_T("%u"), p->uImageId);

			if((m_nFormat == PWEXP_CSV) || (m_nFormat == PWEXP_XML))
			{
				_PwTimeToXmlTime(p->tCreation, &strCreationTime);
				_PwTimeToXmlTime(p->tLastAccess, &strLastAccTime);
				_PwTimeToXmlTime(p->tLastMod, &strLastModTime);
				_PwTimeToXmlTime(p->tExpire, &strExpireTime);
			}
			else
			{
				_PwTimeToString(p->tCreation, &strCreationTime);
				_PwTimeToString(p->tLastAccess, &strLastAccTime);
				_PwTimeToString(p->tLastMod, &strLastModTime);
				_PwTimeToString(p->tExpire, &strExpireTime);
			}

			strGroupTree = MakeGroupTreeString(pg->uGroupId, ((m_nFormat == PWEXP_XML) ||
				(m_nFormat == PWEXP_HTML)));

			ASSERT(p->pszBinaryDesc != NULL);

			if((pOptions->bAttachment == TRUE) && (p->uBinaryDataLen != 0) && (p->pBinaryData != NULL))
			{
				DWORD dwBufSize = (((p->uBinaryDataLen + 2) / 3) << 2) + 1;

				pbEncodedAttachment = new BYTE[dwBufSize + 1];
				ASSERT(pbEncodedAttachment != NULL);
				if(pbEncodedAttachment != NULL)
				{
					if(CBase64Codec::Encode(p->pBinaryData, p->uBinaryDataLen,
						pbEncodedAttachment, &dwBufSize) == false)
					{
						SAFE_DELETE_ARRAY(pbEncodedAttachment);
					}
				}
			}
			else pbEncodedAttachment = NULL;

			if(m_nFormat == PWEXP_TXT)
			{
				_ExpSetSep(NULL);

				_ExpStrIf(pOptions->bTitle, _T("["));
				_ExpStrIf(pOptions->bTitle, p->pszTitle);
				_ExpStrIf(pOptions->bTitle, _T("]"));
				if(pOptions->bTitle == TRUE) _ExpStr(m_pszNewLine);

				_ExpStrIf(pOptions->bGroup, TRL("Group:"));
				_ExpStrIf(pOptions->bGroup, _T(" "));
				_ExpStrIf(pOptions->bGroup, pg->pszGroupName);
				if(pOptions->bGroup == TRUE) _ExpStr(m_pszNewLine);

				_ExpStrIf(pOptions->bGroupTree, TRL("Group Tree"));
				_ExpStrIf(pOptions->bGroupTree, _T(": "));
				_ExpStrIf(pOptions->bGroupTree, strGroupTree);
				if(pOptions->bGroupTree == TRUE) _ExpStr(m_pszNewLine);

				_ExpStrIf(pOptions->bUserName, TRL("User Name"));
				_ExpStrIf(pOptions->bUserName, _T(": "));
				_ExpStrIf(pOptions->bUserName, p->pszUserName);
				if(pOptions->bUserName == TRUE) _ExpStr(m_pszNewLine);

				_ExpStrIf(pOptions->bURL, TRL("URL:"));
				_ExpStrIf(pOptions->bURL, _T(" "));
				_ExpStrIf(pOptions->bURL, p->pszURL);
				if(pOptions->bURL == TRUE) _ExpStr(m_pszNewLine);

				_ExpStrIf(pOptions->bPassword, TRL("Password:"));
				_ExpStrIf(pOptions->bPassword, _T(" "));
				_ExpStrIf(pOptions->bPassword, p->pszPassword);
				if(pOptions->bPassword == TRUE) _ExpStr(m_pszNewLine);

				_ExpStrIf(pOptions->bNotes, TRL("Notes:"));
				_ExpStrIf(pOptions->bNotes, _T(" "));
				CString strNotesConv = SU_ConvertNewLines(p->pszAdditional, m_pszNewLine);
				_ExpStrIf(pOptions->bNotes, strNotesConv);
				if(pOptions->bNotes == TRUE) _ExpStr(m_pszNewLine);

				_ExpStrIf(pOptions->bUUID, TRL("UUID"));
				_ExpStrIf(pOptions->bUUID, _T(": "));
				_ExpStrIf(pOptions->bUUID, strUUID);
				if(pOptions->bUUID == TRUE) _ExpStr(m_pszNewLine);

				_ExpStrIf(pOptions->bImage, TRL("Icon"));
				_ExpStrIf(pOptions->bImage, _T(": "));
				_ExpStrIf(pOptions->bImage, strImage);
				if(pOptions->bImage == TRUE) _ExpStr(m_pszNewLine);

				_ExpStrIf(pOptions->bCreationTime, TRL("Creation Time"));
				_ExpStrIf(pOptions->bCreationTime, _T(": "));
				_ExpStrIf(pOptions->bCreationTime, strCreationTime);
				if(pOptions->bCreationTime == TRUE) _ExpStr(m_pszNewLine);

				_ExpStrIf(pOptions->bLastAccTime, TRL("Last Access"));
				_ExpStrIf(pOptions->bLastAccTime, _T(": "));
				_ExpStrIf(pOptions->bLastAccTime, strLastAccTime);
				if(pOptions->bLastAccTime == TRUE) _ExpStr(m_pszNewLine);

				_ExpStrIf(pOptions->bLastModTime, TRL("Last Modification"));
				_ExpStrIf(pOptions->bLastModTime, _T(": "));
				_ExpStrIf(pOptions->bLastModTime, strLastModTime);
				if(pOptions->bLastModTime == TRUE) _ExpStr(m_pszNewLine);

				_ExpStrIf(pOptions->bExpireTime, TRL("Expires"));
				_ExpStrIf(pOptions->bExpireTime, _T(": "));
				_ExpStrIf(pOptions->bExpireTime, strExpireTime);
				if(pOptions->bExpireTime == TRUE) _ExpStr(m_pszNewLine);

				if(p->pszBinaryDesc[0] != 0)
				{
					_ExpStrIf(pOptions->bAttachment, TRL("Attachment Description"));
					_ExpStrIf(pOptions->bAttachment, _T(": "));
					_ExpStrIf(pOptions->bAttachment, p->pszBinaryDesc);
					if(pOptions->bAttachment == TRUE) _ExpStr(m_pszNewLine);
				}

				if((p->uBinaryDataLen != 0) && (pbEncodedAttachment != NULL))
				{
					_ExpStrIf(pOptions->bAttachment, TRL("Attachment"));
					_ExpStrIf(pOptions->bAttachment, _T(": "));
					_ExpStrIf(pOptions->bAttachment, (LPCTSTR)pbEncodedAttachment);
					if(pOptions->bAttachment == TRUE) _ExpStr(m_pszNewLine);
				}

				_ExpStr(m_pszNewLine);
			}
			else if(m_nFormat == PWEXP_HTML)
			{
				_ExpStr(_T("<tr><td class=\"field_data\">"));

				std::basic_string<TCHAR> strClTD = _T("</td>");
				strClTD += m_pszNewLine;
				strClTD += _T("<td class=\"field_data\">");

				_ExpSetSep(strClTD.c_str());
				_ExpResetSkip();

				_ExpHtmlStrIf(pOptions->bGroup, pg->pszGroupName);
				_ExpStrIf(pOptions->bGroupTree, strGroupTree); // Is encoded already
				_ExpHtmlStrIf(pOptions->bTitle, p->pszTitle);
				_ExpHtmlStrIf(pOptions->bUserName, p->pszUserName);

				if((pOptions->bURL != FALSE) && (_tcslen(p->pszURL) != 0))
				{
					_ExpStrIf(pOptions->bURL, _T("<a href=\""));
					_ExpXmlStr(p->pszURL); // Use XML encoding, no &nbsp; when empty
					_ExpStr(_T("\">"));
					_ExpHtmlStr(p->pszURL);
					_ExpStr(_T("</a>"));
				}
				else _ExpHtmlStrIf(pOptions->bURL, _T(""));

				if(pOptions->bPassword != FALSE)
				{
					_ExpStrIf(pOptions->bPassword, _T("<code>"));
					_ExpHtmlStr(p->pszPassword, XEF_NBSP);
					_ExpStr(_T("</code>"));
				}

				CString strNotesConv = SU_ConvertNewLines(p->pszAdditional, m_pszNewLine);
				_ExpHtmlStrIf(pOptions->bNotes, strNotesConv);
				_ExpHtmlStrIf(pOptions->bUUID, strUUID);
				_ExpHtmlStrIf(pOptions->bImage, strImage);
				_ExpHtmlStrIf(pOptions->bCreationTime, strCreationTime);
				_ExpHtmlStrIf(pOptions->bLastAccTime, strLastAccTime);
				_ExpHtmlStrIf(pOptions->bLastModTime, strLastModTime);
				_ExpHtmlStrIf(pOptions->bExpireTime, strExpireTime);

				if(p->pszBinaryDesc[0] != 0)
					_ExpHtmlStrIf(pOptions->bAttachment, p->pszBinaryDesc);
				else
					_ExpHtmlStrIf(pOptions->bAttachment, _T(""));

				if((p->uBinaryDataLen != 0) && (pbEncodedAttachment != NULL))
					_ExpHtmlStrIf(pOptions->bAttachment, (LPCTSTR)pbEncodedAttachment);
				else
					_ExpHtmlStrIf(pOptions->bAttachment, _T(""));

				_ExpLine(_T("</td></tr>"));
				_ExpSetSep(NULL); // Release strClTD
			}
			else if(m_nFormat == PWEXP_XML)
			{
				_ExpSetSep(NULL);

				_ExpStr(_T("<pwentry>")); _ExpStr(m_pszNewLine);

				_ExpStrIf(pOptions->bGroup, _T("\t<group"));
				if((pOptions->bGroup == TRUE) && (strGroupTree.GetLength() != 0))
				{
					_ExpStrIf(pOptions->bGroupTree, _T(" tree=\""));
					_ExpStrIf(pOptions->bGroupTree, (LPCTSTR)strGroupTree); // Is encoded
					_ExpStrIf(pOptions->bGroupTree, _T("\""));
				}
				_ExpStrIf(pOptions->bGroup, _T(">"));
				_ExpXmlStrIf(pOptions->bGroup, pg->pszGroupName);
				_ExpStrIf(pOptions->bGroup, _T("</group>"));
				if(pOptions->bGroup == TRUE) _ExpStr(m_pszNewLine);

				_ExpStrIf(pOptions->bTitle, _T("\t<title>"));
				_ExpXmlStrIf(pOptions->bTitle, p->pszTitle);
				_ExpStrIf(pOptions->bTitle, _T("</title>"));
				if(pOptions->bTitle == TRUE) _ExpStr(m_pszNewLine);

				_ExpStrIf(pOptions->bUserName, _T("\t<username>"));
				_ExpXmlStrIf(pOptions->bUserName, p->pszUserName);
				_ExpStrIf(pOptions->bUserName, _T("</username>"));
				if(pOptions->bUserName == TRUE) _ExpStr(m_pszNewLine);

				_ExpStrIf(pOptions->bURL, _T("\t<url>"));
				_ExpXmlStrIf(pOptions->bURL, p->pszURL);
				_ExpStrIf(pOptions->bURL, _T("</url>"));
				if(pOptions->bURL == TRUE) _ExpStr(m_pszNewLine);

				_ExpStrIf(pOptions->bPassword, _T("\t<password>"));
				_ExpXmlStrIf(pOptions->bPassword, p->pszPassword);
				_ExpStrIf(pOptions->bPassword, _T("</password>"));
				if(pOptions->bPassword == TRUE) _ExpStr(m_pszNewLine);

				_ExpStrIf(pOptions->bNotes, _T("\t<notes>"));
				CString strNotesConv = SU_ConvertNewLines(p->pszAdditional, m_pszNewLine);
				_ExpXmlStrIf(pOptions->bNotes, strNotesConv);
				_ExpStrIf(pOptions->bNotes, _T("</notes>"));
				if(pOptions->bNotes == TRUE) _ExpStr(m_pszNewLine);

				_ExpStrIf(pOptions->bUUID, _T("\t<uuid>"));
				_ExpXmlStrIf(pOptions->bUUID, strUUID);
				_ExpStrIf(pOptions->bUUID, _T("</uuid>"));
				if(pOptions->bUUID == TRUE) _ExpStr(m_pszNewLine);

				_ExpStrIf(pOptions->bImage, _T("\t<image>"));
				_ExpXmlStrIf(pOptions->bImage, (LPCTSTR)strImage);
				_ExpStrIf(pOptions->bImage, _T("</image>"));
				if(pOptions->bImage == TRUE) _ExpStr(m_pszNewLine);

				_ExpStrIf(pOptions->bCreationTime, _T("\t<creationtime>"));
				_ExpXmlStrIf(pOptions->bCreationTime, (LPCTSTR)strCreationTime);
				_ExpStrIf(pOptions->bCreationTime, _T("</creationtime>"));
				if(pOptions->bCreationTime == TRUE) _ExpStr(m_pszNewLine);

				_ExpStrIf(pOptions->bLastModTime, _T("\t<lastmodtime>"));
				_ExpXmlStrIf(pOptions->bLastModTime, (LPCTSTR)strLastModTime);
				_ExpStrIf(pOptions->bLastModTime, _T("</lastmodtime>"));
				if(pOptions->bLastModTime == TRUE) _ExpStr(m_pszNewLine);

				_ExpStrIf(pOptions->bLastAccTime, _T("\t<lastaccesstime>"));
				_ExpXmlStrIf(pOptions->bLastAccTime, (LPCTSTR)strLastAccTime);
				_ExpStrIf(pOptions->bLastAccTime, _T("</lastaccesstime>"));
				if(pOptions->bLastAccTime == TRUE) _ExpStr(m_pszNewLine);

				if(memcmp(&p->tExpire, &tNever, sizeof(PW_TIME)) == 0)
					_ExpStrIf(pOptions->bExpireTime, _T("\t<expiretime expires=\"false\">"));
				else
					_ExpStrIf(pOptions->bExpireTime, _T("\t<expiretime expires=\"true\">"));

				_ExpXmlStrIf(pOptions->bExpireTime, (LPCTSTR)strExpireTime);
				_ExpStrIf(pOptions->bExpireTime, _T("</expiretime>"));
				if(pOptions->bExpireTime == TRUE) _ExpStr(m_pszNewLine);

				if(p->pszBinaryDesc[0] != 0)
				{
					_ExpStrIf(pOptions->bAttachment, _T("\t<attachdesc>"));
					_ExpXmlStrIf(pOptions->bAttachment, p->pszBinaryDesc);
					_ExpStrIf(pOptions->bAttachment, _T("</attachdesc>"));
					if(pOptions->bAttachment == TRUE) _ExpStr(m_pszNewLine);
				}

				if((p->uBinaryDataLen != 0) && (pbEncodedAttachment != NULL))
				{
					_ExpStrIf(pOptions->bAttachment, _T("\t<attachment>"));
					_ExpStrIf(pOptions->bAttachment, (LPCTSTR)pbEncodedAttachment);
					_ExpStrIf(pOptions->bAttachment, _T("</attachment>"));
					if(pOptions->bAttachment == TRUE) _ExpStr(m_pszNewLine);
				}

				_ExpStr(_T("</pwentry>"));
				_ExpStr(m_pszNewLine);
			}
			else if(m_nFormat == PWEXP_CSV)
			{
				_ExpStr(_T("\""));

				_ExpSetSep(_T("\",\""));
				_ExpResetSkip();

				_ExpCsvStrIf(pOptions->bGroup, pg->pszGroupName);
				_ExpCsvStrIf(pOptions->bGroupTree, strGroupTree);
				_ExpCsvStrIf(pOptions->bTitle, p->pszTitle);
				_ExpCsvStrIf(pOptions->bUserName, p->pszUserName);
				_ExpCsvStrIf(pOptions->bPassword, p->pszPassword);
				_ExpCsvStrIf(pOptions->bURL, p->pszURL);
				CString strNotesConv = SU_ConvertNewLines(p->pszAdditional, m_pszNewLine);
				_ExpCsvStrIf(pOptions->bNotes, strNotesConv);
				_ExpCsvStrIf(pOptions->bUUID, strUUID);
				_ExpCsvStrIf(pOptions->bImage, strImage);
				_ExpCsvStrIf(pOptions->bCreationTime, strCreationTime);
				_ExpCsvStrIf(pOptions->bLastAccTime, strLastAccTime);
				_ExpCsvStrIf(pOptions->bLastModTime, strLastModTime);
				_ExpCsvStrIf(pOptions->bExpireTime, strExpireTime);

				if(p->pszBinaryDesc[0] != 0)
					_ExpCsvStrIf(pOptions->bAttachment, p->pszBinaryDesc);
				else
					_ExpCsvStrIf(pOptions->bAttachment, _T(""));

				if((p->uBinaryDataLen != 0) && (pbEncodedAttachment != NULL))
					_ExpCsvStrIf(pOptions->bAttachment, (LPCTSTR)pbEncodedAttachment);
				else
					_ExpCsvStrIf(pOptions->bAttachment, _T(""));

				_ExpStr(_T("\""));
				_ExpStr(m_pszNewLine);
			}
			else if(m_nFormat == PWEXP_KEEPASS)
			{
				VERIFY(pStoreMgr->AddEntry(p));
			}
			else { ASSERT(FALSE); }

			SAFE_DELETE_ARRAY(pbEncodedAttachment);

			m_pMgr->LockEntryPassword(p);
		}
	}

	if(m_nFormat == PWEXP_TXT)
	{ // Nothing to do to finalize TXT
	}
	else if(m_nFormat == PWEXP_HTML)
	{
		_ExpStr(_T("</table></body></html>"));
		_ExpStr(m_pszNewLine);
	}
	else if(m_nFormat == PWEXP_XML)
	{
		_ExpStr(_T("</pwlist>"));
		_ExpStr(m_pszNewLine);
	}
	else if(m_nFormat == PWEXP_CSV)
	{ // Nothing to do to finalize CSV
	}
	else if(m_nFormat == PWEXP_KEEPASS)
	{
		if(pStoreMgr->SaveDatabase(pszFile, NULL) != PWE_SUCCESS) bReturn = FALSE;
	}
	else { ASSERT(FALSE); } // Unknown format, should never happen

	if(m_nFormat != PWEXP_KEEPASS)
	{
		ASSERT(fp != NULL);
		if(fp != NULL) fclose(fp);
		fp = NULL; m_fp = NULL;
	}

	aGroupIds.clear();
	return bReturn;
}
