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

#ifndef ___POPULAR_PASSWORDS_H___
#define ___POPULAR_PASSWORDS_H___

#pragma once

#include "../SysDefEx.h"
#include <boost/shared_ptr.hpp>
#include <boost/unordered_set.hpp>
#include <boost/utility.hpp>
#include <tchar.h>
#include <vector>
#include <stdlib.h>
#include "StrUtil.h"

struct TppDictHash
{
	size_t operator()(LPCWSTR p) const
	{
		if(p == NULL) { ASSERT(FALSE); return 0; }

		LPCWSTR lp = p;
		size_t h = 0xC17962B7U;
		while(true)
		{
			const WCHAR ch = *lp;
			if(ch == L'\0') break;

			h += static_cast<size_t>(ch);
#if (SIZE_MAX == 0xFFFFFFFFU)
			h = _rotl(h * 0x5FC34C67U, 13);
#elif (SIZE_MAX == 0xFFFFFFFFFFFFFFFFUL)
			h = _rotl64(h * 0x54724D3EA2860CBBULL, 29);
#else
#error Unknown SIZE_MAX!
#endif
			++lp;
		}

		return h;
	}
};

struct TppDictPred
{
	bool operator()(LPCWSTR a, LPCWSTR b) const
	{
		if(a == NULL) { ASSERT(FALSE); return (b == NULL); }
		if(b == NULL) { ASSERT(FALSE); return false; }

		return (wcscmp(a, b) == 0);
	}
};

typedef boost::unordered_set<LPCWSTR, TppDictHash, TppDictPred> TppDict;
typedef boost::shared_ptr<TppDict> TppDictPtr;

class CPopularPasswords : boost::noncopyable
{
private:
	CPopularPasswords();

public:
	static void Clear();

	static size_t GetMaxLength();
	static bool ContainsLength(size_t uLen);

	static bool IsPopular(LPCWSTR lpw, size_t* pdwDictSize);

	static void Add(const UTF8_BYTE* pTextUTF8);
	static void AddResUTF8(LPCTSTR lpResName, LPCTSTR lpResType);

private:
	static std::vector<LPWSTR> g_vMem;
	static std::vector<TppDictPtr> g_vDicts;
};

#endif // ___POPULAR_PASSWORDS_H___
