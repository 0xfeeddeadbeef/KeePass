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

#ifndef ___FILE_TRANSACTION_EX_H___
#define ___FILE_TRANSACTION_EX_H___

#pragma once

#include <vector>
#include <string>
#include <tchar.h>

typedef std::basic_string<TCHAR> std_string;

typedef HANDLE(WINAPI *LPCREATETRANSACTION)(LPSECURITY_ATTRIBUTES lpTransactionAttributes,
	LPGUID UOW, DWORD CreateOptions, DWORD IsolationLevel, DWORD IsolationFlags,
	DWORD Timeout, LPWSTR Description);
typedef BOOL(WINAPI *LPMOVEFILETRANSACTED)(LPCTSTR lpExistingFileName,
	LPCTSTR lpNewFileName, LPPROGRESS_ROUTINE lpProgressRoutine,
	LPVOID lpData, DWORD dwFlags, HANDLE hTransaction);
typedef BOOL(WINAPI *LPCOMMITTRANSACTION)(HANDLE TransactionHandle);

class CFileTransactionEx
{
public:
	CFileTransactionEx(LPCTSTR lpBaseFile, bool bTransacted);
	virtual ~CFileTransactionEx();

	bool OpenWrite(std_string& strOutBufferFile);
	bool CommitWrite();

private:
	bool CommitWriteTransaction();

	static std_string TxfCreateID();
	static bool TxfIsSupported(TCHAR chDriveLetter);
	static void TxfClosePrsv(HANDLE hTx);

	void TxfPrepare();
	bool TxfMove(bool& bFatalError);
	bool TxfMoveWithTx();

	bool m_bTransacted;
	std_string m_strBase;
	std_string m_strTemp;
	std_string m_strTxfMidFallback;

	bool m_bMadeUnhidden;
	PSECURITY_DESCRIPTOR m_pSec;

	HMODULE m_hKernel32;
	HMODULE m_hKtmW32;
	LPCREATETRANSACTION m_fnCreateTransaction;
	LPMOVEFILETRANSACTED m_fnMoveFileTransacted;
	LPCOMMITTRANSACTION m_fnCommitTransaction;

	std::vector<std_string> m_vToDelete;
};

#endif // ___FILE_TRANSACTION_EX_H___
