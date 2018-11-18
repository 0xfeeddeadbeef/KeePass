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

#ifndef ___AUTO_COMPLETE_EX_H___
#define ___AUTO_COMPLETE_EX_H___

#pragma once

#include "../../KeePassLibCpp/SysDefEx.h"
#include "../../KeePassLibCpp/Util/StrUtil.h"
#include <boost/utility.hpp>

class CAutoCompleteEx : boost::noncopyable
{
private:
	CAutoCompleteEx() { };

public:
	static void Init(HWND hWnd, const std::vector<LPCTSTR>& vItems);

private:
	static void InitPriv(HWND hWnd, const std::vector<LPCTSTR>& vItems);
};

#endif // ___AUTO_COMPLETE_EX_H___
