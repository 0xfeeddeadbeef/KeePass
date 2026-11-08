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
using System.Globalization;
using System.IO;
using System.Reflection;
using System.Text;
using System.Text.RegularExpressions;
using System.Xml;
using System.Xml.Serialization;
using System.Xml.XPath;
using System.Xml.Xsl;

using KeePassLib.Delegates;
using KeePassLib.Interfaces;
using KeePassLib.Serialization;

namespace KeePassLib.Utility
{
	public static class XmlUtilEx
	{
		public static XmlDocument CreateXmlDocument()
		{
			XmlDocument xd = new XmlDocument();

			// .NET 4.5.2 and newer do not resolve external XML resources
			// by default; for older .NET versions, we explicitly
			// prevent resolving
			xd.XmlResolver = null; // Default in old .NET: XmlUrlResolver object

			return xd;
		}

		private static XmlDocument LoadXmlDocument(TextReader tr)
		{
			if(tr == null) throw new ArgumentNullException("tr");

			XmlDocument xd = CreateXmlDocument();
			XmlReaderSettings xrs = CreateXmlReaderSettings();

			using(XmlReader xr = XmlReader.Create(tr, xrs))
			{
				xd.Load(xr);
			}

			return xd;
		}

		internal static XmlDocument LoadXmlDocument(Stream s, Encoding enc)
		{
			if(s == null) throw new ArgumentNullException("s");
			if(enc == null) throw new ArgumentNullException("enc");

			using(StreamReader sr = new StreamReader(s, enc, true))
			{
				return LoadXmlDocument(sr);
			}
		}

		internal static XmlDocument LoadXmlDocument(string strFilePath, Encoding enc)
		{
			if(strFilePath == null) throw new ArgumentNullException("strFilePath");
			if(enc == null) throw new ArgumentNullException("enc");

			using(FileStream fs = new FileStream(strFilePath, FileMode.Open,
				FileAccess.Read, FileShare.Read))
			{
				return LoadXmlDocument(fs, enc);
			}
		}

		internal static XmlDocument LoadXmlDocumentFromString(string strXml)
		{
			if(strXml == null) throw new ArgumentNullException("strXml");

			using(StringReader sr = new StringReader(strXml))
			{
				return LoadXmlDocument(sr);
			}
		}

		public static XmlReaderSettings CreateXmlReaderSettings()
		{
			XmlReaderSettings xrs = new XmlReaderSettings();

			xrs.CloseInput = false;
			xrs.IgnoreComments = true;
			xrs.IgnoreProcessingInstructions = true;
			xrs.IgnoreWhitespace = true;

#if KeePassUAP
			xrs.DtdProcessing = DtdProcessing.Ignore;
#else
			Type tXrs = typeof(XmlReaderSettings);
			Type tDtd = tXrs.Assembly.GetType("System.Xml.DtdProcessing", false);
			object oDtdIgnore = ((tDtd != null) ? MemUtil.GetEnumValue(tDtd,
				"Ignore") : null);
			PropertyInfo piDtd = tXrs.GetProperty("DtdProcessing",
				BindingFlags.Public | BindingFlags.Instance);
			if((piDtd != null) && (oDtdIgnore != null))
				piDtd.SetValue(xrs, oDtdIgnore, null); // .NET >= 4 only
			else
			{
#pragma warning disable 618
				xrs.ProhibitDtd = true; // Obsolete in .NET 4, but still there
#pragma warning restore 618
			}
#endif

			xrs.ValidationType = ValidationType.None;
			xrs.XmlResolver = null;

			return xrs;
		}

		public static XmlReader CreateXmlReader(Stream s)
		{
			if(s == null) { Debug.Assert(false); throw new ArgumentNullException("s"); }

			return XmlReader.Create(s, CreateXmlReaderSettings());
		}

		public static XmlWriterSettings CreateXmlWriterSettings()
		{
			XmlWriterSettings xws = new XmlWriterSettings();

			xws.CloseOutput = false;
			xws.Encoding = StrUtil.Utf8;
			xws.Indent = true;
			xws.IndentChars = "\t";
			xws.NewLineOnAttributes = false;

			return xws;
		}

		public static XmlWriter CreateXmlWriter(Stream s)
		{
			if(s == null) { Debug.Assert(false); throw new ArgumentNullException("s"); }

			return XmlWriter.Create(s, CreateXmlWriterSettings());
		}

		public static T Deserialize<T>(Stream s)
		{
			if(s == null) { Debug.Assert(false); throw new ArgumentNullException("s"); }

			XmlSerializer xs = new XmlSerializer(typeof(T));

			T t = default(T);
			using(XmlReader xr = CreateXmlReader(s))
			{
				t = (T)xs.Deserialize(xr);
			}

			return t;
		}

		public static void Serialize<T>(Stream s, T t)
		{
			if(s == null) { Debug.Assert(false); throw new ArgumentNullException("s"); }

			XmlSerializer xs = new XmlSerializer(typeof(T));
			using(XmlWriter xw = CreateXmlWriter(s))
			{
				xs.Serialize(xw, t);
			}
		}

		internal static void Serialize<T>(Stream s, T t, bool bRemoveXsdXsi)
		{
			// One way to remove the "xsd" and "xsi" namespace declarations
			// is to use an XmlSerializerNamespaces object containing only
			// a ""/"" pair; this seems to work, but Microsoft's
			// documentation explicitly states that it isn't supported:
			// https://docs.microsoft.com/en-us/dotnet/api/system.xml.serialization.xmlserializernamespaces
			// There are other, more complex ways, but these either rely on
			// undocumented details or require the type T to be modified.

			string str;
			using(MemoryStream ms = new MemoryStream())
			{
				Serialize<T>(ms, t);

				str = StrUtil.Utf8.GetString(ms.ToArray());
			}

			GFunc<string, string, bool> fFindPfx = delegate(string strText, string strSub)
			{
				int i = strText.IndexOf(strSub, StringComparison.Ordinal);
				if(i < 0) return false;
				if(i == 0) return true;
				return char.IsWhiteSpace(strText[i - 1]);
			};

			if(bRemoveXsdXsi)
			{
				if(!fFindPfx(str, "xsd:") && !fFindPfx(str, "xsi:"))
				{
					Debug.Assert(str.IndexOf("xmlns:xsd") > 0);
					str = str.Replace(" xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\"", string.Empty);
					Debug.Assert(str.IndexOf("xmlns:xsd") < 0);

					Debug.Assert(str.IndexOf("xmlns:xsi") > 0);
					str = str.Replace(" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"", string.Empty);
					Debug.Assert(str.IndexOf("xmlns:xsi") < 0);
				}
				else { Debug.Assert(false); } // "xsd"/"xsi" decl. may be required
			}

			MemUtil.Write(s, StrUtil.Utf8.GetBytes(str));
		}

#if DEBUG
		internal static void ValidateXml(string strXml, bool bReplaceStdEntities)
		{
			if(strXml == null) throw new ArgumentNullException("strXml");
			if(strXml.Length == 0) { Debug.Assert(false); return; }

			strXml = strXml.Replace("<!DOCTYPE html>", string.Empty);

			if(bReplaceStdEntities)
				strXml = strXml.Replace("&nbsp;", "&#160;");

			LoadXmlDocumentFromString(strXml);
		}
#endif

		internal static XPathNodeIterator FindNodes(PwDatabase pd, string strXPath,
			IStatusLogger sl, out XmlDocument xd)
		{
			if(pd == null) throw new ArgumentNullException("pd");
			if(strXPath == null) { Debug.Assert(false); strXPath = string.Empty; }

			using(MemoryStream msW = new MemoryStream())
			{
				KdbxFile kdbx = new KdbxFile(pd);
				kdbx.Save(msW, null, KdbxFormat.PlainXml, sl);

				using(MemoryStream msR = new MemoryStream(msW.ToArray(), false))
				{
					xd = LoadXmlDocument(msR, StrUtil.Utf8);
				}
			}

			XPathNavigator xpn = xd.CreateNavigator();
			XPathExpression xpe = xpn.Compile(strXPath);
			xpe.SetContext(new XueXsltContext());
			return xpn.Select(xpe);
		}

		private sealed class XueXsltContext : XsltContext
		{
			private const string g_strNamespace = "urn:custom-kp";

			private readonly XueXcfMatches m_xcfMatches = new XueXcfMatches();
			private readonly XueXcfStringCompare m_xcfStringCompare = new XueXcfStringCompare();

			public override bool Whitespace { get { return false; } }

			public XueXsltContext()
			{
				AddNamespace("kp", g_strNamespace);
			}

			public override int CompareDocument(string baseUri, string nextbaseUri)
			{
				return Math.Sign(string.CompareOrdinal(baseUri, nextbaseUri));
			}

			public override bool PreserveWhitespace(XPathNavigator node)
			{
				return false;
			}

			public override IXsltContextFunction ResolveFunction(string prefix,
				string name, XPathResultType[] ArgTypes)
			{
				if(string.IsNullOrEmpty(prefix) || (LookupNamespace(prefix) !=
					g_strNamespace))
					return null;

				if(name == "matches") return m_xcfMatches;
				if(name == "string-compare") return m_xcfStringCompare;

				return null;
			}

			public override IXsltContextVariable ResolveVariable(string prefix,
				string name)
			{
				return null;
			}
		}

		private static string XcfGetString(object[] v, int i)
		{
			if((v == null) || (i < 0)) { Debug.Assert(false); return string.Empty; }
			if(i >= v.Length) return string.Empty;

			object o = v[i];

			XPathNodeIterator it = (o as XPathNodeIterator);
			if(it != null) o = (it.MoveNext() ? it.Current.Value : null);

			return (((o != null) ? o.ToString() : null) ?? string.Empty);
		}

		// Cf. https://www.w3.org/TR/xpath-functions/#func-matches
		private sealed class XueXcfMatches : IXsltContextFunction
		{
			private readonly XPathResultType[] m_vArgTypes = new XPathResultType[] {
				XPathResultType.String, XPathResultType.String, XPathResultType.String };
			public XPathResultType[] ArgTypes { get { return m_vArgTypes; } }

			public int Minargs { get { return 2; } }
			public int Maxargs { get { return 3; } }

			public XPathResultType ReturnType { get { return XPathResultType.Boolean; } }

			public object Invoke(XsltContext xsltContext, object[] args,
				XPathNavigator docContext)
			{
				string strInput = XcfGetString(args, 0);
				string strPattern = XcfGetString(args, 1);
				string strFlags = XcfGetString(args, 2);

				RegexOptions ro = RegexOptions.None;
				foreach(char ch in strFlags)
				{
					switch(ch)
					{
						case 's': ro |= RegexOptions.Singleline; break;
						case 'm': ro |= RegexOptions.Multiline; break;
						case 'i': ro |= RegexOptions.IgnoreCase; break;
						case 'x': ro |= RegexOptions.IgnorePatternWhitespace; break;
						case 'q': strPattern = Regex.Escape(strPattern); break;
						default: throw new ArgumentOutOfRangeException("strFlags");
					}
				}

				return Regex.IsMatch(strInput, strPattern, ro);
			}
		}

		// Cf. https://learn.microsoft.com/en-us/previous-versions/dotnet/netframework-4.0/ms256114(v=vs.100)
		private sealed class XueXcfStringCompare : IXsltContextFunction
		{
			private readonly XPathResultType[] m_vArgTypes = new XPathResultType[] {
				XPathResultType.String, XPathResultType.String, XPathResultType.String,
				XPathResultType.String };
			public XPathResultType[] ArgTypes { get { return m_vArgTypes; } }

			public int Minargs { get { return 2; } }
			public int Maxargs { get { return 4; } }

			public XPathResultType ReturnType { get { return XPathResultType.Number; } }

			public object Invoke(XsltContext xsltContext, object[] args,
				XPathNavigator docContext)
			{
				string strX = XcfGetString(args, 0);
				string strY = XcfGetString(args, 1);
				string strLang = XcfGetString(args, 2);
				string strOptions = XcfGetString(args, 3);

				CultureInfo ci = ((strLang.Length == 0) ? CultureInfo.CurrentCulture :
					new CultureInfo(strLang));

				CompareOptions co = CompareOptions.None;
				bool bU = false;
				foreach(char ch in strOptions)
				{
					switch(ch)
					{
						case 'u': bU = true; break;
						case 'i':
							co |= CompareOptions.IgnoreCase | CompareOptions.IgnoreKanaType |
								CompareOptions.IgnoreWidth;
							break;
						default: throw new ArgumentOutOfRangeException("strOptions");
					}
				}

				if(bU)
				{
					if(co != CompareOptions.None)
						throw new ArgumentOutOfRangeException("strOptions");
					co = CompareOptions.IgnoreCase;
				}

				int r = Math.Sign(ci.CompareInfo.Compare(strX, strY, co));

				if(bU && (r == 0))
					r = -Math.Sign(ci.CompareInfo.Compare(strX, strY));

				return r;
			}
		}
	}
}
