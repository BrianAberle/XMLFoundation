// --------------------------------------------------------------------------
//						United Business Technologies
//			  Copyright (c) 2000 - 2010  All Rights Reserved.
//
// Source in this file is released to the public under the following license:
// --------------------------------------------------------------------------
// This toolkit may be used free of charge for any purpose including corporate
// and academic use.  For profit, and Non-Profit uses are permitted.
//
// This source code and any work derived from this source code must retain 
// this copyright at the top of each source file.
// --------------------------------------------------------------------------
#include "GlobalInclude.h"
static char SOURCE_FILE[] = __FILE__;

#include "FrameworkAuditLog.h"
#include "GProfile.h"
#include "GException.h"

#include <stdio.h> // for sprintf(), vsprintf()

// (see FrameworkAuditLog.h for description of this variable)
char *g_pchLogFile = 0;

bool UBTAuditLogIsEnabled()
{
	if (g_pchLogFile == LOGGING_OFF)
		return false;
	if (g_pchLogFile == LOGGING_UNASSIGNED)
	{
		try
		{
			g_pchLogFile = (char *)GetProfile().GetString("Debug", "DebugTraceXML",false);
			if (g_pchLogFile && g_pchLogFile[0])
			{
				// g_pchLogFile is valid and logging is turned ON
			}
			else
			{
				// Turn off logging and don't check the environment variable anymore (it's slow)
				g_pchLogFile = (char *)LOGGING_OFF;
			}
		}
		catch (GException &)
		{
		}
	}
	if (g_pchLogFile == (char *)LOGGING_OFF)
		return false;
	return true;
}

#define FORCE_ANSI      0x10000
#define FORCE_UNICODE   0x20000
#define FORCE_INT64     0x40000

#ifndef ___max
#define ___max(a,b)            (((a) > (b)) ? (a) : (b))
#endif
#ifndef ___min
#define ___min(a,b)            (((a) < (b)) ? (a) : (b))
#endif





void SetLogFile(const char *pzText)
{
	g_pchLogFile = (char *)pzText;
}


void UBTAuditLog(const char *pzText, bool bNewLine)
{
	if (g_pchLogFile == LOGGING_UNASSIGNED)
	{
		try
		{
			g_pchLogFile = (char *)GetProfile().GetString("Debug", "DebugTraceXML",false);
			if (g_pchLogFile && g_pchLogFile[0])
			{
				// g_pchLogFile is valid and logging is turned ON
			}
			else
			{
				// Turn off logging and don't check the environment variable anymore (it's slow)
				g_pchLogFile = (char *)LOGGING_OFF;
			}
		}
		catch (GException &)
		{
			// If this fails once, don't try in the future
			g_pchLogFile = (char *)LOGGING_OFF;
		}
	}
	if (g_pchLogFile != LOGGING_OFF)
	{
		if (pzText && pzText[0])
		{
			GString str(pzText);
			if (bNewLine)
				str << "\n";
			str.ToFileAppend(g_pchLogFile,0);
		}
	}
}

static short nIndent = 0;
void UBTAuditLogEx(const char *szFile, const char *pzText, bool bNewLine, bool push)
{
	if (g_pchLogFile == LOGGING_UNASSIGNED)
	{
		try
		{
			g_pchLogFile = (char *)GetProfile().GetString("Debug", "DebugTraceXML",false);
			if (g_pchLogFile && g_pchLogFile[0])
			{
				// g_pchLogFile is valid and logging is turned ON
			}
			else
			{
				// Turn off logging and don't check the environment variable anymore (it's slow)
				g_pchLogFile = (char *)LOGGING_OFF;
			}
		}
		catch (GException &)
		{
		}
	}

	if (g_pchLogFile != LOGGING_OFF)
	{
		// trace out the xml being sent to the server
//		fstream fsOut(g_pchLogFile,  ios::out | ios::app);
		GString strOut;

		for (int i = 0; i < nIndent; i++)
			strOut << "   ";

		strOut << '(' << szFile << ')';

		if (push)
			strOut << " push: ";
		else
			strOut << " pop : ";

		if (pzText)
			strOut << pzText;
		else
			strOut << "(null)";

		if (bNewLine)
			strOut << "\n";
		strOut.ToFileAppend(g_pchLogFile);
	}
}

#ifdef __TRACE_STACK__

	void UBTStackTrace::FormatV(const char *lpszFormat, va_list argList)
	{
		va_list argListSave = argList;

		// make a guess at the maximum length of the resulting string
		int nMaxLen = 0;
		for (const char *lpsz = lpszFormat; *lpsz != '\0'; lpsz++)
		{
			// handle '%' character, but watch out for '%%'
			if (*lpsz != '%' || *(lpsz = lpsz + 1) == '%')
			{
				nMaxLen += strlen(lpsz);
				continue;
			}

			int nItemLen = 0;

			// handle '%' character with format
			int nWidth = 0;
			for (; *lpsz != '\0'; lpsz++)
			{
				// check for valid flags
				if (*lpsz == '#')
					nMaxLen += 2;   // for '0x'
				else if (*lpsz == '*')
					nWidth = va_arg(argList, int);
				else if (*lpsz == '-' || *lpsz == '+' || *lpsz == '0' ||
					*lpsz == ' ')
					;
				else // hit non-flag character
					break;
			}
			// get width and skip it
			if (nWidth == 0)
			{
				// width indicated by
				nWidth = atoi(lpsz);
				for (; *lpsz != '\0' && isdigit(*lpsz); lpsz++)
					;
			}
			(nWidth >= 0);

			int nPrecision = 0;
			if (*lpsz == '.')
			{
				// skip past '.' separator (width.precision)
				lpsz++;

				// get precision and skip it
				if (*lpsz == '*')
				{
					nPrecision = va_arg(argList, int);
					lpsz++;
				}
				else
				{
					nPrecision = atoi(lpsz);
					for (; *lpsz != '\0' && isdigit(*lpsz); lpsz++)
						;
				}
				(nPrecision >= 0);
			}

			// should be on type modifier or specifier
			int nModifier = 0;
			if (strncmp(lpsz, "I64", 3) == 0)
			{
				lpsz += 3;
				nModifier = FORCE_INT64;
			}
			else
			{
				switch (*lpsz)
				{
				// modifiers that affect size
				case 'h':
					nModifier = FORCE_ANSI;
					lpsz++;
					break;
				case 'l':
					nModifier = FORCE_UNICODE;
					lpsz++;
					break;

				// modifiers that do not affect size
				case 'F':
				case 'N':
				case 'L':
					lpsz++;
					break;
				}
			}

			// now should be on specifier
			switch (*lpsz | nModifier)
			{
			// single characters
			case 'c':
			case 'C':
				nItemLen = 2;
#ifdef _LINUX
				va_arg(argList, int); //char is promoted to int when passed through ...
#else
				va_arg(argList, char);
#endif
				break;

			// strings
			case 's':
				{
					const char *pstrNextArg = va_arg(argList, const char *);
					if (pstrNextArg == NULL)
					   nItemLen = 6;  // "(null)"
					else
					{
					   nItemLen = strlen(pstrNextArg);
					   nItemLen = ___max(1, nItemLen);
					}
				}
				break;

			case 's'|FORCE_ANSI:
			case 'S'|FORCE_ANSI:
				{
					const char * pstrNextArg = va_arg(argList, const char *);
					if (pstrNextArg == NULL)
					   nItemLen = 6; // "(null)"
					else
					{
					   nItemLen = strlen(pstrNextArg);
					   nItemLen = ___max(1, nItemLen);
					}
				}
				break;
			}
			// adjust nItemLen for strings
			if (nItemLen != 0)
			{
				if (nPrecision != 0)
					nItemLen = ___min(nItemLen, nPrecision);
				nItemLen = ___max(nItemLen, nWidth);
			}
			else
			{
				switch (*lpsz)
				{
				// integers
				case 'd':
				case 'i':
				case 'u':
				case 'x':
				case 'X':
				case 'o':
					if (nModifier & FORCE_INT64)
						va_arg(argList, __int64);
					else
						va_arg(argList, int);
					nItemLen = 32;
					nItemLen = ___max(nItemLen, nWidth+nPrecision);
					break;

				case 'e':
				case 'g':
				case 'G':
					va_arg(argList, double);
					nItemLen = 128;
					nItemLen = ___max(nItemLen, nWidth+nPrecision);
					break;

				case 'f':
					{
						double f;
						// 312 == strlen("-1+(309 zeroes).")
						// 309 zeroes == max precision of a double
						// 6 == adjustment in case precision is not specified,
						//   which means that the precision defaults to 6
						char *pszTemp = new char[___max(nWidth, 312+nPrecision+6)];

						f = va_arg(argList, double);
						sprintf( pszTemp,  "%*.*f", nWidth, nPrecision+6, f );
						nItemLen = strlen(pszTemp);

						delete [] pszTemp;
					}
					break;

				case 'p':
					va_arg(argList, void*);
					nItemLen = 32;
					nItemLen = ___max(nItemLen, nWidth+nPrecision);
					break;

				// no output
				case 'n':
					va_arg(argList, int*);
					break;
				}
			}

			// adjust nMaxLen for output nItemLen
			nMaxLen += nItemLen;
		}

		m_szTrace = new char[nMaxLen + 1];
		vsprintf(m_szTrace, lpszFormat, argListSave);

		va_end(argListSave);
	}

	// Traces out the file name and message
	UBTStackTrace::UBTStackTrace(const char *szFile, const char *lpszFormat, ...)
	{
#ifdef _WIN32
		char chTerm = '\\';
#else
		char chTerm = '/';
#endif
		char *szTemp = (char *)&szFile[strlen(szFile) - 1];
		while (*szTemp != chTerm)
			szTemp--;
		szTemp++;
		m_szFile = new char[strlen(szTemp) + 1];
		strcpy(m_szFile, szTemp);
		va_list argList;
		va_start(argList, lpszFormat);
		FormatV(lpszFormat, argList);
		va_end(argList);

		UBTAuditLogEx(m_szFile, m_szTrace, true, true);

		nIndent++;
	}

	// Traces out the file name and message
	// and cleans up all memory allocation.
	UBTStackTrace::~UBTStackTrace()
	{
		nIndent--;
		UBTAuditLogEx(m_szFile, m_szTrace, true, false);

		delete [] m_szFile;
		delete [] m_szTrace;
	}

#endif // __TRACE_STACK__
