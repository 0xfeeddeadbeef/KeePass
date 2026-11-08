/*
  KeePass Password Safe - The Open-Source Password Manager
  Copyright (C) 2003-2026 Dominik Reichl <dominik.reichl@t-online.de>

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

using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Text;
using System.Windows.Forms;

namespace KeePass.Util
{
	public sealed class ClipboardContents
	{
		private string m_strText = null;
		private List<KeyValuePair<string, object>> m_lContents = null;

		public ClipboardContents(bool bGetCurrent, bool bSimpleOnly)
		{
			if(bGetCurrent) GetData(bSimpleOnly);
		}

		/// <summary>
		/// Get the current clipboard contents.
		/// </summary>
		/// <param name="bSimpleOnly">Get simple data (text) only. The
		/// advanced mode might throw in conjunction with applications
		/// that use clipboard tricks like delay rendering.</param>
		public void GetData(bool bSimpleOnly)
		{
			try { GetDataPriv(bSimpleOnly); }
			catch(Exception) { Debug.Assert(false); }
		}

		private void GetDataPriv(bool bSimpleOnly)
		{
			m_strText = null;
			m_lContents = null;

			if(bSimpleOnly)
			{
				if(ClipboardUtil.ContainsText())
					m_strText = ClipboardUtil.GetText();
			}
			else
			{
				m_lContents = new List<KeyValuePair<string, object>>();

				IDataObject d = Clipboard.GetDataObject();
				foreach(string strFormat in d.GetFormats())
					m_lContents.Add(new KeyValuePair<string, object>(strFormat,
						d.GetData(strFormat)));
			}
		}

		public void SetData()
		{
			try { SetDataPriv(); }
			catch(Exception) { Debug.Assert(false); }
		}

		private void SetDataPriv()
		{
			if(m_strText != null)
				ClipboardUtil.Copy(m_strText, false, false, null, null, IntPtr.Zero);
			else if(m_lContents != null)
			{
				DataObject d = new DataObject();
				foreach(KeyValuePair<string, object> kvp in m_lContents)
					d.SetData(kvp.Key, kvp.Value);

				ClipboardUtil.Clear();
				Clipboard.SetDataObject(d);
			}
		}
	}
}
