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
#include "AutoCompleteEx.h"
#include <boost/shared_array.hpp>
#include <boost/static_assert.hpp>
#include <boost/unordered_set.hpp>

struct CACD_Hash
{
	size_t operator()(LPCTSTR p) const
	{
		if(p == NULL) { ASSERT(FALSE); return 0; }

		LPCTSTR lp = p;
		size_t h = 0xC17962B7U;
		while(true)
		{
			const TCHAR ch = *lp;
			if(ch == _T('\0')) break;

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

struct CACD_Equal
{
	bool operator()(LPCTSTR a, LPCTSTR b) const
	{
		if(a == NULL) { ASSERT(FALSE); return (b == NULL); }
		if(b == NULL) { ASSERT(FALSE); return false; }

		return (_tcscmp(a, b) == 0);
	}
};

typedef boost::unordered_set<LPCTSTR, CACD_Hash, CACD_Equal> CACD_Set;

bool CACD_Compare(const boost::shared_array<WCHAR>& a,
	const boost::shared_array<WCHAR>& b)
{
	return (_wcsicmp(a.get(), b.get()) < 0);
}

class CAutoCompleteData : public IEnumString
{
public:
	CAutoCompleteData(const std::vector<LPCTSTR>& vItems) :
		m_cRef(1), m_pos(0)
	{
		CACD_Set s;
		for(size_t i = 0; i < vItems.size(); ++i)
		{
			LPCTSTR lp = vItems[i];
			if((lp != NULL) && (*lp != _T('\0'))) s.insert(lp);
		}

		for(CACD_Set::const_iterator it = s.begin(); it != s.end(); ++it)
		{
			LPCTSTR lp = *it;
			if(lp == NULL) { ASSERT(FALSE); continue; }

#ifdef _UNICODE
			const size_t cc = wcslen(lp);
			boost::shared_array<WCHAR> sa(new WCHAR[cc + 1]);
			memcpy(sa.get(), lp, (cc + 1) * sizeof(WCHAR));
#else
			boost::shared_array<WCHAR> sa(_StringToUnicode(lp));
#endif

			m_v.push_back(sa);
		}

		std::sort(m_v.begin(), m_v.end(), CACD_Compare);
	}

	STDMETHODIMP_(ULONG) AddRef()
	{
		ASSERT(m_cRef > 0);

		const LONG r = InterlockedIncrement(&m_cRef);
		return static_cast<ULONG>(r);
	};

	STDMETHODIMP_(ULONG) Release()
	{
		ASSERT(m_cRef > 0);

		const LONG r = InterlockedDecrement(&m_cRef);
		if(r == 0) delete this;
		return static_cast<ULONG>(r);
	};

	STDMETHODIMP QueryInterface(REFIID riid, void** ppv)
	{
		if(ppv == NULL) return E_INVALIDARG;

		if(riid == IID_IUnknown)
		{
			IUnknown* p = this;
			*ppv = p;
			AddRef();
			return S_OK;
		}
		if(riid == IID_IEnumString)
		{
			IEnumString* p = this;
			*ppv = p;
			AddRef();
			return S_OK;
		}

		// The auto-completion object may ask for the optional IACList
		*ppv = NULL;
		return E_NOINTERFACE;
	};

	STDMETHODIMP Next(ULONG celt, LPOLESTR* rgelt, ULONG* pceltFetched)
	{
		if(pceltFetched != NULL) *pceltFetched = 0;

		if(celt == 0) return S_OK;
		if(rgelt == NULL) return E_INVALIDARG;
		if((celt >= 2) && (pceltFetched == NULL)) return E_INVALIDARG;

		const ULONG uAvail = static_cast<ULONG>(m_v.size() - m_pos);
		const ULONG uRet = min(celt, uAvail);

		for(ULONG i = 0; i < uRet; ++i)
		{
			LPCWSTR lp = m_v[m_pos].get();
			const size_t cc = wcslen(lp);

			BOOST_STATIC_ASSERT(sizeof(OLECHAR) == sizeof(WCHAR));
			LPVOID lpNew = CoTaskMemAlloc((cc + 1) * sizeof(WCHAR));
			if(lpNew == NULL) { ASSERT(FALSE); return E_OUTOFMEMORY; }
			memcpy(lpNew, lp, (cc + 1) * sizeof(WCHAR));

			// Caller is responsible for freeing it
			rgelt[i] = (LPOLESTR)lpNew;

			++m_pos;
		}

		if(pceltFetched != NULL) *pceltFetched = uRet;
		return ((uRet == celt) ? S_OK : S_FALSE);
	};

	STDMETHODIMP Skip(ULONG celt)
	{
		const size_t uNewPos = m_pos + celt;
		if((uNewPos <= m_v.size()) && (uNewPos >= celt))
		{
			m_pos = uNewPos;
			return S_OK;
		}

		m_pos = m_v.size();
		return S_FALSE;
	};

	STDMETHODIMP Reset()
	{
		m_pos = 0;
		return S_OK;
	};

	STDMETHODIMP Clone(IEnumString** ppenum)
	{
		if(ppenum == NULL) return E_INVALIDARG;

		CAutoCompleteData* p = new CAutoCompleteData();

		p->m_v = m_v;
		p->m_pos = m_pos;

		*ppenum = p;
		return S_OK;
	};

private:
	CAutoCompleteData() : m_cRef(1), m_pos(0)
	{
	};

private:
	LONG m_cRef;

	std::vector<boost::shared_array<WCHAR> > m_v;
	size_t m_pos;
};

void CAutoCompleteEx::Init(HWND hWnd, const std::vector<LPCTSTR>& vItems)
{
	if(hWnd == NULL) { ASSERT(FALSE); return; }
	if(vItems.size() == 0) return;

	if(SUCCEEDED(CoInitializeEx(NULL, COINIT_APARTMENTTHREADED)))
	{
		InitPriv(hWnd, vItems);

		CoUninitialize();
	}
	else { ASSERT(FALSE); }
}

void CAutoCompleteEx::InitPriv(HWND hWnd, const std::vector<LPCTSTR>& vItems)
{
	IAutoComplete2* pAC = NULL;
	if(FAILED(CoCreateInstance(CLSID_AutoComplete, NULL,
		CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pAC)))) { ASSERT(FALSE); return; }
	if(pAC == NULL) { ASSERT(FALSE); return; }

	IUnknown* pEnum = new CAutoCompleteData(vItems);

	if(SUCCEEDED(pAC->Init(hWnd, pEnum, NULL, NULL)))
	{
		VERIFY(SUCCEEDED(pAC->SetOptions(ACO_AUTOSUGGEST | ACO_AUTOAPPEND)));
	}
	else { ASSERT(FALSE); }

	pAC->Release();
	VERIFY(pEnum->Release() != 0); // pAC still references it
}
