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
#include <ObjBase.h>
#include <AclAPI.h>
#include "FileTransactionEx.h"
#include "PwUtil.h"
#include "StrUtil.h"

#include <boost/scoped_array.hpp>

using boost::scoped_array;

LPCTSTR g_lpTempSuffix = _T(".tmp");
LPCTSTR g_lpTxfTempPrefix = _T("KeePass_TxF_");
LPCTSTR g_lpTxfTempSuffix = _T(".tmp");

CFileTransactionEx::CFileTransactionEx(LPCTSTR lpBaseFile, bool bTransacted) :
	m_bMadeUnhidden(false), m_pSec(NULL), m_hKernel32(NULL), m_hKtmW32(NULL),
	m_fnCreateTransaction(NULL), m_fnMoveFileTransacted(NULL),
	m_fnCommitTransaction(NULL)
{
	m_bTransacted = bTransacted;

	if(lpBaseFile == NULL) { ASSERT(FALSE); return; }

	scoped_array<TCHAR> aPath(new TCHAR[MAX_PATH + 1]);
	ZeroMemory(aPath.get(), (MAX_PATH + 1) * sizeof(TCHAR));
	DWORD r = GetFullPathName(lpBaseFile, MAX_PATH, aPath.get(), NULL);
	if((r == 0) || (r > MAX_PATH)) { ASSERT(FALSE); m_strBase = lpBaseFile; }
	else m_strBase = aPath.get();

	const DWORD dw = GetFileAttributes(m_strBase.c_str());
	if(dw != INVALID_FILE_ATTRIBUTES)
	{
		// Symbolic links are realized via reparse points;
		// https://msdn.microsoft.com/en-us/library/windows/desktop/aa365503.aspx
		// https://msdn.microsoft.com/en-us/library/windows/desktop/aa365680.aspx
		// https://msdn.microsoft.com/en-us/library/windows/desktop/aa365006.aspx
		// Performing a file transaction on a symbolic link
		// would delete/replace the symbolic link instead of
		// writing to its target
		if((dw & FILE_ATTRIBUTE_REPARSE_POINT) != 0) m_bTransacted = false;
	}
	else
	{
		// If the base and the temporary file are in different
		// folders and the base file doesn't exist (i.e. we can't
		// backup the ACL), a transaction would cause the new file
		// to have the default ACL of the temporary folder instead
		// of the one of the base folder; therefore, we don't use
		// a transaction when the base file doesn't exist (this
		// also results in other applications monitoring the folder
		// to see one file creation only)
		m_bTransacted = false;
	}

	if(m_bTransacted)
	{
		m_strTemp = m_strBase + g_lpTempSuffix;
		TxfPrepare(); // Adjusts m_strTemp
	}
	else m_strTemp = m_strBase;
}

CFileTransactionEx::~CFileTransactionEx()
{
	const DWORD dwError = GetLastError();

	ASSERT(m_pSec == NULL);

	if(m_hKtmW32 != NULL)
	{
		m_fnCreateTransaction = NULL;
		m_fnCommitTransaction = NULL;

		VERIFY(FreeLibrary(m_hKtmW32));
		m_hKtmW32 = NULL;
	}

	if(m_hKernel32 != NULL)
	{
		m_fnMoveFileTransacted = NULL;

		VERIFY(FreeLibrary(m_hKernel32));
		m_hKernel32 = NULL;
	}

	for(size_t i = 0; i < m_vToDelete.size(); ++i)
	{
		if(GetFileAttributes(m_vToDelete[i].c_str()) != INVALID_FILE_ATTRIBUTES)
		{
			VERIFY(DeleteFile(m_vToDelete[i].c_str()));
		}
	}
	m_vToDelete.clear();

	SetLastError(dwError);
}

// Must set last error before returning false
bool CFileTransactionEx::OpenWrite(std_string& strOutBufferFile)
{
	if(m_strBase.size() == 0)
	{
		ASSERT(FALSE);
		strOutBufferFile = _T("");
		SetLastError(ERROR_INVALID_HANDLE);
		return false;
	}

	if(!m_bTransacted) m_bMadeUnhidden |= CPwUtil::UnhideFile(m_strTemp.c_str());

	strOutBufferFile = m_strTemp;
	return true;
}

// Must set last error before returning false
bool CFileTransactionEx::CommitWrite()
{
	if(m_strBase.size() == 0)
	{
		ASSERT(FALSE);
		SetLastError(ERROR_INVALID_HANDLE);
		return false;
	}

	bool r = true;
	if(!m_bTransacted)
	{
		if(m_bMadeUnhidden) CPwUtil::HideFile(m_strTemp.c_str(), true);
	}
	else r = CommitWriteTransaction();

	m_strBase = _T(""); // Dispose
	if(m_pSec != NULL) { VERIFY(LocalFree(m_pSec) == NULL); m_pSec = NULL; }

	return r;
}

bool CFileTransactionEx::CommitWriteTransaction()
{
	const bool bMadeUnhidden = CPwUtil::UnhideFile(m_strBase.c_str());

	const DWORD dwAttrib = GetFileAttributes(m_strBase.c_str());
	const bool bAttrib = (dwAttrib != INVALID_FILE_ATTRIBUTES);
	CNullableEx<FILETIME> otCreation;
	PACL pAcl = NULL;

	if(bAttrib) // Base file exists
	{
		if((dwAttrib & FILE_ATTRIBUTE_ENCRYPTED) != 0)
			DecryptFile(m_strBase.c_str(), 0); // For TxF

		otCreation = CPwUtil::GetFileCreationTime(m_strBase.c_str());

		if(GetNamedSecurityInfo(const_cast<LPTSTR>(m_strBase.c_str()),
			SE_FILE_OBJECT, DACL_SECURITY_INFORMATION, NULL, NULL,
			&pAcl, NULL, &m_pSec) != ERROR_SUCCESS)
			pAcl = NULL;
	}

	bool bFatalError = false;
	if(!TxfMove(bFatalError))
	{
		if(bFatalError) return false;

		if(bAttrib) // Base file exists
		{
			if(DeleteFile(m_strBase.c_str()) == FALSE) return false;
		}

		if(MoveFile(m_strTemp.c_str(), m_strBase.c_str()) == FALSE)
			return false;
	}
	else { ASSERT(pAcl != NULL); } // TxF success => NTFS => has ACL

	if(otCreation.HasValue())
		CPwUtil::SetFileCreationTime(m_strBase.c_str(), otCreation.GetValuePtr());
	if(bAttrib && ((dwAttrib & FILE_ATTRIBUTE_ENCRYPTED) != 0))
		EncryptFile(m_strBase.c_str());
	if(pAcl != NULL)
	{
		VERIFY(SetNamedSecurityInfo(const_cast<LPTSTR>(m_strBase.c_str()),
			SE_FILE_OBJECT, DACL_SECURITY_INFORMATION, NULL, NULL, pAcl,
			NULL) == ERROR_SUCCESS);
	}
	if(bMadeUnhidden)
		CPwUtil::HideFile(m_strBase.c_str(), true);

	return true;
}

std_string CFileTransactionEx::TxfCreateID()
{
	GUID g;
	if(FAILED(CoCreateGuid(&g)))
	{
		ASSERT(FALSE);

		g.Data1 = GetTickCount();
		g.Data2 = static_cast<USHORT>(randXorShift());
		g.Data3 = static_cast<USHORT>(rand());

		LARGE_INTEGER li;
		QueryPerformanceCounter(&li);
		memcpy(&g.Data4[0], &li, 8);
	}

	CString strID;
	_UuidToString((BYTE*)&g, &strID);
	return std_string((LPCTSTR)strID);
}

bool CFileTransactionEx::TxfIsSupported(TCHAR chDriveLetter)
{
	if(chDriveLetter == _T('\0')) return false;

	TCHAR tszRoot[4];
	tszRoot[0] = chDriveLetter;
	tszRoot[1] = _T(':');
	tszRoot[2] = _T('\\');
	tszRoot[3] = _T('\0');

	const DWORD cch = MAX_PATH + 1;
	scoped_array<TCHAR> aName(new TCHAR[cch + 1]);
	DWORD dwSerial = 0, cchMaxComp = 0, uFlags = 0;
	scoped_array<TCHAR> aFileSystem(new TCHAR[cch + 1]);

	if(GetVolumeInformation(&tszRoot[0], aName.get(), cch, &dwSerial,
		&cchMaxComp, &uFlags, aFileSystem.get(), cch) == FALSE)
		return false;

	return ((uFlags & FILE_SUPPORTS_TRANSACTIONS) != 0);
}

void CFileTransactionEx::TxfPrepare()
{
	const bool bUni = (sizeof(TCHAR) >= 2);

	if(m_hKtmW32 != NULL) { ASSERT(FALSE); return; }
	m_hKtmW32 = LoadLibrary(_T("KtmW32.dll"));
	if(m_hKtmW32 == NULL) return; // Windows <= XP

	m_fnCreateTransaction = (LPCREATETRANSACTION)GetProcAddress(m_hKtmW32,
		"CreateTransaction");
	if(m_fnCreateTransaction == NULL) { ASSERT(FALSE); return; }

	m_fnCommitTransaction = (LPCOMMITTRANSACTION)GetProcAddress(m_hKtmW32,
		"CommitTransaction");
	if(m_fnCommitTransaction == NULL) { ASSERT(FALSE); return; }

	if(m_hKernel32 != NULL) { ASSERT(FALSE); return; }
	m_hKernel32 = LoadLibrary(_T("Kernel32.dll"));
	if(m_hKernel32 == NULL) { ASSERT(FALSE); return; }

	m_fnMoveFileTransacted = (LPMOVEFILETRANSACTED)GetProcAddress(m_hKernel32,
		(bUni ? "MoveFileTransactedW" : "MoveFileTransactedA"));
	if(m_fnMoveFileTransacted == NULL) { ASSERT(FALSE); return; }

	std_string strID = TxfCreateID();

	TCHAR tszTempDir[MAX_PATH + 1];
	ZeroMemory(&tszTempDir[0], (MAX_PATH + 1) * sizeof(TCHAR));
	DWORD r = GetTempPath(MAX_PATH, &tszTempDir[0]);
	if((r == 0) || (r > MAX_PATH) || (tszTempDir[0] == _T('\0'))) { ASSERT(FALSE); return; }

	if(GetFileAttributes(&tszTempDir[0]) == INVALID_FILE_ATTRIBUTES)
	{
		ASSERT(FALSE);
		if(CreateDirectory(&tszTempDir[0], NULL) == FALSE) return;
	}

	std_string strTemp = &tszTempDir[0];
	if(strTemp[strTemp.size() - 1] != _T('\\')) strTemp += _T("\\");
	strTemp += g_lpTxfTempPrefix;
	strTemp += strID;
	strTemp += g_lpTxfTempSuffix;

	TCHAR chB = SU_GetDriveLetter(m_strBase.c_str());
	TCHAR chT = SU_GetDriveLetter(strTemp.c_str());
	if(!TxfIsSupported(chB)) return;
	if((chT != chB) && !TxfIsSupported(chT)) return;

	m_strTxfMidFallback = m_strTemp;
	m_strTemp = strTemp;

	m_vToDelete.push_back(strTemp);
}

bool CFileTransactionEx::TxfMove(bool& bFatalError)
{
	if(m_strTxfMidFallback.size() == 0) return false;

	if(TxfMoveWithTx()) return true;

	// Move the temporary file onto the base file's drive first,
	// such that it cannot happen that both the base file and
	// the temporary file are deleted/corrupted
	const DWORD f = (MOVEFILE_COPY_ALLOWED | MOVEFILE_REPLACE_EXISTING);
	bool b = (MoveFileEx(m_strTemp.c_str(), m_strTxfMidFallback.c_str(), f) != FALSE);
	if(b) b = (MoveFileEx(m_strTxfMidFallback.c_str(), m_strBase.c_str(), f) != FALSE);
	if(!b) { ASSERT(FALSE); bFatalError = true; return false; }

	ASSERT(GetFileAttributes(m_strTemp.c_str()) == INVALID_FILE_ATTRIBUTES);
	ASSERT(GetFileAttributes(m_strTxfMidFallback.c_str()) == INVALID_FILE_ATTRIBUTES);
	return true;
}

bool CFileTransactionEx::TxfMoveWithTx()
{
	std_string strTx = PWM_PRODUCT_NAME_SHORT;
	strTx += _T(" TxF - ");
	strTx += TxfCreateID();
	if(strTx.size() >= MAX_TRANSACTION_DESCRIPTION_LENGTH)
		strTx = strTx.substr(0, MAX_TRANSACTION_DESCRIPTION_LENGTH - 1);
	std::basic_string<WCHAR> strTxW = _StringToUnicodeStl(strTx.c_str());

	HANDLE hTx = m_fnCreateTransaction(NULL, NULL, 0, 0, 0, 0,
		const_cast<LPWSTR>(strTxW.c_str()));
	if(hTx == INVALID_HANDLE_VALUE) { ASSERT(FALSE); return false; }

	if(m_fnMoveFileTransacted(m_strTemp.c_str(), m_strBase.c_str(), NULL,
		NULL, (MOVEFILE_COPY_ALLOWED | MOVEFILE_REPLACE_EXISTING), hTx) == FALSE)
	{
		ASSERT(FALSE);
		TxfClosePrsv(hTx);
		return false;
	}

	if(m_fnCommitTransaction(hTx) == FALSE)
	{
		ASSERT(FALSE);
		TxfClosePrsv(hTx);
		return false;
	}

	ASSERT(GetFileAttributes(m_strTemp.c_str()) == INVALID_FILE_ATTRIBUTES);
	TxfClosePrsv(hTx);
	return true;
}

void CFileTransactionEx::TxfClosePrsv(HANDLE hTx)
{
	const DWORD dw = GetLastError();
	VERIFY(CloseHandle(hTx));
	SetLastError(dw);
}
