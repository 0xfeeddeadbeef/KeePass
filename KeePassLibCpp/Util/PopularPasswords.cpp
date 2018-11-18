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
#include "PopularPasswords.h"
#include <boost/static_assert.hpp>

std::vector<LPWSTR> CPopularPasswords::g_vMem;
std::vector<TppDictPtr> CPopularPasswords::g_vDicts;

CPopularPasswords::CPopularPasswords()
{
}

void CPopularPasswords::Clear()
{
	g_vDicts.clear();

	for(size_t i = 0; i < g_vMem.size(); ++i)
	{
		SAFE_DELETE_ARRAY(g_vMem[i]);
	}
	g_vMem.clear();
}

size_t CPopularPasswords::GetMaxLength()
{
	const size_t s = g_vDicts.size();
	if(s == 0) { ASSERT(FALSE); return 0; } // Should be initialized

	ASSERT(g_vDicts[s - 1].get() != NULL);
	ASSERT(g_vDicts[s - 1]->size() > 0);
	return (s - 1);
}

bool CPopularPasswords::ContainsLength(size_t uLen)
{
	if(uLen == 0) return false;
	if(uLen >= g_vDicts.size()) return false;

	const TppDictPtr& p = g_vDicts[uLen];
	ASSERT((p.get() == NULL) || (p->size() > 0));
	return (p.get() != NULL);
}

bool CPopularPasswords::IsPopular(LPCWSTR lpw, size_t* pdwDictSize)
{
	if(lpw == NULL) { ASSERT(FALSE); return false; }

	const size_t uLen = wcslen(lpw);
	if(uLen >= g_vDicts.size()) return false;

#ifdef _DEBUG
	for(size_t i = 0; i < uLen; ++i) { ASSERT(lpw[i] == towlower(lpw[i])); }
#endif

	const TppDictPtr& p = g_vDicts[uLen];
	if(p.get() == NULL) return false;

	if(pdwDictSize != NULL) *pdwDictSize = p->size();
	return (p->count(lpw) > 0);
}

void CPopularPasswords::Add(const UTF8_BYTE* pTextUTF8)
{
	if(pTextUTF8 == NULL) { ASSERT(FALSE); return; }

	BOOST_STATIC_ASSERT(sizeof(UTF8_BYTE) == sizeof(char));
	LPCSTR lpc = (LPCSTR)pTextUTF8;
	const int nUTF8Len = static_cast<int>(szlen(lpc));
	if(nUTF8Len <= 0) return;

	const int cchWBuf = nUTF8Len + 16;
	LPWSTR lpw = new WCHAR[cchWBuf];
	if(lpw == NULL) { ASSERT(FALSE); return; }
	ZeroMemory(lpw, cchWBuf * sizeof(WCHAR));

	const int r = MultiByteToWideChar(CP_UTF8, 0, lpc, nUTF8Len, lpw, cchWBuf - 8);
	ASSERT(r <= nUTF8Len);
	if((r == 0) || (lpw[0] == L'\0')) { ASSERT(FALSE); SAFE_DELETE_ARRAY(lpw); return; }

	size_t n = wcslen(lpw);
	ASSERT(n > 0);
	if(iswspace(lpw[n - 1]) == 0)
	{
		lpw[n] = L'\n';
		++n;
	}

	LPCWSTR lpWord = NULL;
	for(size_t i = 0; i < n; ++i)
	{
		lpw[i] = towlower(lpw[i]);

		if(iswspace(lpw[i]) != 0)
		{
			if(lpWord != NULL)
			{
				lpw[i] = L'\0'; // Terminate the word

				const size_t cc = wcslen(lpWord);
				if(cc >= g_vDicts.size()) g_vDicts.resize(cc + 1);

				TppDictPtr p = g_vDicts[cc];
				if(p.get() == NULL)
				{
					p = TppDictPtr(new TppDict());
					g_vDicts[cc] = p;
				}

				p->insert(lpWord);
				lpWord = NULL;
			}
		}
		else if(lpWord == NULL) lpWord = &lpw[i];
	}

	g_vMem.push_back(lpw);
}

void CPopularPasswords::AddResUTF8(LPCTSTR lpResName, LPCTSTR lpResType)
{
	if((lpResName == NULL) || (lpResType == NULL)) { ASSERT(FALSE); return; }

	HRSRC hRes = ::FindResource(NULL, lpResName, lpResType);
	if(hRes == NULL) { ASSERT(FALSE); return; }

	const DWORD dwSize = ::SizeofResource(NULL, hRes);
	if(dwSize == 0) { ASSERT(FALSE); return; }

	HGLOBAL hgData = ::LoadResource(NULL, hRes);
	if(hgData == NULL) { ASSERT(FALSE); return; }

	LPVOID lpData = ::LockResource(hgData);
	if(lpData == NULL) { ASSERT(FALSE); return; }

	std::vector<BYTE> v(dwSize + 4);
	ASSERT(v.size() == (dwSize + 4));
	memcpy(&v[0], lpData, dwSize);
	v[dwSize] = 0; v[dwSize + 1] = 0; v[dwSize + 2] = 0; v[dwSize + 3] = 0;

	BOOST_STATIC_ASSERT(sizeof(UTF8_BYTE) == sizeof(BYTE));
	CPopularPasswords::Add(&v[0]);
}
