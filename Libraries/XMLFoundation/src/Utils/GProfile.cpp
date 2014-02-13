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

#ifdef WINCE
	#include  "WinCERuntimeC.h"
#endif
#include "GProfile.h"
#include "GException.h"
#include "GDirectory.h"
#include "Base64.h"
#include <stdlib.h> //for: getenv()
#include <string.h> //for: memcpy()
#include <stdio.h>  //for: fopen(),fclose();


#ifndef _WIN32
	#include <errno.h>
#endif


#ifdef _WIN32
#include <Windows.h>
#endif

#include "TwoFish.h"

static GProfile *g_AlternateProfile = 0;
static GProfile *g_pDefaultProfile = 0;

class GProfileCleanup
{
public:
	GProfileCleanup(){};
	~GProfileCleanup()
	{
		if (g_pDefaultProfile)
		{
			delete g_pDefaultProfile;
			g_pDefaultProfile = 0;
		}
	}
};
GProfileCleanup g_DefaultProfileCleanup;


GProfile *SetProfile(GProfile *pApplicationProfile)
{
	GProfile *pReturn = g_AlternateProfile;
	g_AlternateProfile = pApplicationProfile;
	return pReturn;
}

GProfile &GetProfile()
{
	if (g_AlternateProfile)
		return *g_AlternateProfile;
	if (!g_pDefaultProfile)
		g_pDefaultProfile = new GProfile(); // no external profile config, no override
	return *g_pDefaultProfile;
}


GProfile::GProfile(const char *pzFileName, const char *pzEnvOverride )
	: m_bCached(0)
{
	m_pTreeNotify = new GBTree();
	m_strEnvOverride = pzEnvOverride;
	m_strBaseFileName = pzFileName; // no path

	GString strConfigFileDefault;

	if (pzFileName && pzFileName[0])
	{
		// see if we can open the file from the current or supplied path
		// if not look for it in the root of the file system
		FILE *fp = fopen(pzFileName,"rb");
		if (fp)
		{
			fclose(fp);
			strConfigFileDefault = pzFileName;
		}
		else
		{
		#ifdef _WIN32					// like "txml.txt"
			strConfigFileDefault.Format("c:\\%s",pzFileName);
		#else
			strConfigFileDefault.Format("/%s",pzFileName);
		#endif
		}
	}
	if (pzEnvOverride && pzEnvOverride[0])
	{
#ifndef __WINPHONE
		// env var like "XML_CONFIG_FILE"
		if (getenv(pzEnvOverride))
			// Use the system environment setting
			m_strFile = getenv(pzEnvOverride);
		else
			// Use the default
			m_strFile = strConfigFileDefault;
#endif
	}
}


//
// load the profile configuration file yourself, 
// and create this object "with no disk config file"
GProfile::GProfile(const char *szBuffer, __int64 dwSize)
{
	m_pTreeNotify = new GBTree();
	ProfileParse(szBuffer, dwSize);
}


// create an empty profile
GProfile::GProfile()
	: m_bCached(0)
{
	m_pTreeNotify = new GBTree();
	m_strEnvOverride = "NONE";
	m_strBaseFileName = "NONE";
	m_strFile = "NONE";
}

// create a profile from the supplied file name
GProfile::GProfile(const char *pzFilePathAndName )
	: m_bCached(0)
{
	m_pTreeNotify = new GBTree();
	m_strEnvOverride = "NONE";
	m_strBaseFileName = GDirectory::LastLeaf(pzFilePathAndName); //"txml.txt";

	if (pzFilePathAndName && pzFilePathAndName[0])
		// use the supplied pzFilePathAndName
		m_strFile = pzFilePathAndName;
}

GProfile::~GProfile()
{
	if (m_pTreeNotify)
		delete m_pTreeNotify;
	Destroy();
}


long GProfile::SetConfigDefault(const char *szSection, const char *szEntry, const char *pzValue)
{
	return SetConfig(szSection, szEntry, pzValue, 1);
}

long GProfile::SetConfigDefault(const char *szSection, const char *szEntry, int pzValue)
{
	GString strConvert;
	strConvert << pzValue;
	return SetConfig(szSection, szEntry, strConvert, 1 );
}

long GProfile::SetConfigDefault(const char *szSection, const char *szEntry, long lValue)
{
	GString strConvert;
	strConvert << lValue;
	return SetConfig(szSection, szEntry, strConvert, 1 );
}

long GProfile::SetConfigDefault(const char *szSection, const char *szEntry, __int64 lValue)
{
	GString strConvert;
	strConvert << lValue;
	return SetConfig(szSection, szEntry, strConvert, 1 );
}

long GProfile::SetConfigBinaryDefault(const char *szSection, const char *szEntry, unsigned char *lValue, int nValueLength)
{
	// UUencode the binary
	BUFFER b;
	BufferInit(&b);
	uuencode(lValue, nValueLength, &b);
	GString strEncoded((char *)b.pBuf, b.cLen);
	BufferTerminate(&b);
	
	return SetConfig(szSection, szEntry, strEncoded, 1 );
}

long GProfile::SetConfigCipherDefault(const char *szSection, const char *szEntry, const char *pzPassword, const char *lValue, int nValueLength)
{
	GString strDest;
	GString strErrorOut;
	GString strPass(pzPassword);
	EncryptMemoryToMemory(strPass, lValue,nValueLength,strDest, strErrorOut);
	return SetConfigBinaryDefault(szSection, szEntry, (unsigned char *)strDest.Buf(), (int)strDest.Length());
}

long GProfile::SetConfig(const char *szSectionName, const char *szKey, __int64 lValue)
{
	GString strConvert;
	strConvert << lValue;
	return SetConfig(szSectionName, szKey, strConvert, 0 );
}

long GProfile::SetConfig(const char *szSectionName, const char *szKey, long lValue)
{
	GString strConvert;
	strConvert << lValue;
	return SetConfig(szSectionName, szKey, strConvert, 0 );
}
long GProfile::SetConfig(const char *szSectionName, const char *szKey, int nValue)
{
	GString strConvert;
	strConvert << nValue;
	return SetConfig(szSectionName, szKey, strConvert, 0 );
}

long GProfile::SetConfigCipher(const char *szSection, const char *szEntry, const char *pzPassword, const char *lValue, int nValueLength)
{
	GString strDest;
	GString strErrorOut;
	GString strPass(pzPassword);
	EncryptMemoryToMemory(strPass, lValue,nValueLength,strDest, strErrorOut);
	return SetConfigBinary(szSection, szEntry, (unsigned char *)strDest.Buf(), (int)strDest.Length());
}

long GProfile::SetConfigBinary(const char *szSection, const char *szEntry, unsigned char *lValue, int nValueLength)
{
	// UUencode the binary
	BUFFER b;
	BufferInit(&b);
	uuencode(lValue, nValueLength, &b);
	GString strEncoded((char *)b.pBuf, b.cLen);
	BufferTerminate(&b);
	
	return SetConfig(szSection, szEntry, strEncoded, 0 );
}

long GProfile::SetConfig(const char *szSectionName, const char *szKey, const char *pzValue)
{
	return SetConfig(szSectionName, szKey, pzValue, 0);
}

long GProfile::SetConfig(const char *szSectionName, const char *szKey, const char *pzValue, short bSetDefault)
{
	NameValuePair *pNVP;
	Section *pSection = FindSection(szSectionName);
	if (pSection)
	{
		pNVP = FindKey(szKey, pSection);
		if (pNVP)
		{
			if (!bSetDefault)
				pNVP->m_strValue = pzValue;
		}
		else
		{
			// new key
			pNVP = new NameValuePair;
			pSection->m_lstNVP.AddLast(pNVP);
			pNVP->m_strName = szKey;
			pNVP->m_strValue = pzValue;
		}
	}
	else
	{
		// new section
		pSection = new Section;
		pSection->m_strName = szSectionName;
		m_lstSections.AddLast(pSection);

		// new key
		pNVP = new NameValuePair;
		pSection->m_lstNVP.AddLast(pNVP);
		pNVP->m_strName = szKey;
		pNVP->m_strValue = pzValue;
	}

	ChangeNotify(szSectionName, szKey, pzValue);
	return 1;
}


void GProfile::Destroy()
{
	GListIterator itSections(&m_lstSections);
	while (itSections())
	{
		Section *pSection = (Section *)itSections++;

		GListIterator itNVP(&pSection->m_lstNVP);
		while (itNVP())
		{
			NameValuePair *pNVP = (NameValuePair *)itNVP++;
			delete pNVP;
		}

		delete pSection;
	}

	m_lstSections.RemoveAll();
}

#ifdef _WIN32
void GProfile::ThrowLastError(const char *pzFile)
{
	DWORD dwExp = GetLastError();
#ifdef __WINPHONE
	char pzMsgBuf[256];
	FormatMessage( 
					FORMAT_MESSAGE_FROM_SYSTEM | 
					FORMAT_MESSAGE_IGNORE_INSERTS,
					0,
					dwExp,
					MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
					pzMsgBuf,
					0,
					NULL 
				);

	GString strTemp = pzMsgBuf;
#else
	LPVOID lpMsgBuf;
	FormatMessage( 
					FORMAT_MESSAGE_ALLOCATE_BUFFER | 
					FORMAT_MESSAGE_FROM_SYSTEM | 
					FORMAT_MESSAGE_IGNORE_INSERTS,
					NULL,
					dwExp,
					MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
					(LPTSTR) &lpMsgBuf,
					0,
					NULL 
				);
	GString strTemp = (char *)lpMsgBuf;
	LocalFree( lpMsgBuf );
#endif
	strTemp.TrimRightWS();

	GString strFile;
	strFile.Format("%s - %s", (const char *)strTemp,pzFile);
	GException exp(dwExp,(const char *)strFile);
	throw exp;
}
#endif

void GProfile::GetLine(GString &strLine, 
					   const char *szBuffer, 
					   __int64 *nIdx, 
					   __int64 dwSize)
{
	strLine.Empty();
	while (*nIdx < dwSize)
	{
		if (szBuffer[*nIdx] == '\n')
			break;
		strLine += szBuffer[*nIdx];
		(*nIdx)++;
	}

	(*nIdx)++;
}

void GProfile::ProfileParse(const char *szBuffer, __int64 dwSize)
{
	Section *pSection = 0;

	// parse the file
	__int64 nIdx = 0;

	GString strLine;
	while (nIdx < dwSize)
	{
		GetLine(strLine, szBuffer, &nIdx, dwSize);

		strLine.TrimRightWS();
		strLine.TrimLeftWS();

		if ((strLine.IsEmpty()) || (strLine.GetAt(0) == ';'))
			continue;


		strLine.Replace("\\n", '\n');

		if (strLine.GetAt(0) == '[')
		{
			__int64 nIdx = strLine.Find(']');
			if (nIdx == -1)
				nIdx = strLine.Length();

			// new section
			pSection = new Section;
			pSection->m_strName = strLine.Mid(1, nIdx - 1);
			pSection->m_strName.TrimLeftWS();
			pSection->m_strName.TrimRightWS();

			m_lstSections.AddLast(pSection);
		}
		else if (pSection)
		{
			__int64 nIdx = strLine.Find('=');
			if (nIdx == -1)
				continue;
			NameValuePair *pNVP = new NameValuePair;
			pSection->m_lstNVP.AddLast(pNVP);
			pNVP->m_strName = strLine.Left(nIdx);
			pNVP->m_strName.TrimLeftWS();
			pNVP->m_strName.TrimRightWS();
			
			pNVP->m_strValue = strLine.Mid(nIdx + 1);
			pNVP->m_strValue.TrimLeftWS();
			pNVP->m_strValue.TrimRightWS();
		}
	}
	m_bCached = true;
}

void GProfile::Load()
{
	// if there is no external config file, there is nothing to load.
	if (m_strFile == "NONE")
		return;	

	if (m_bCached)
		return;

	// destroy the cached objects
	Destroy();

	char *szBuffer = 0;
	long dwSize = 0;

#if defined(_WIN32) && !defined(__WINPHONE)
	try {
		// open the file
		HANDLE hFile = CreateFile(m_strFile, 
								  GENERIC_READ, 0, 0, OPEN_EXISTING,
								  FILE_ATTRIBUTE_NORMAL, 0);
		if (INVALID_HANDLE_VALUE == hFile)
			ThrowLastError(m_strFile);

		dwSize = GetFileSize(hFile, NULL); 
		if (dwSize == -1)
			ThrowLastError(m_strFile);
 
		szBuffer = new char[dwSize + 1];
		if (!szBuffer)
			throw -1;

		// read the file
		long dwRead;
		if (!ReadFile(hFile, szBuffer, dwSize, (DWORD *)&dwRead, 0))
		{
			delete [] szBuffer;
			CloseHandle(hFile);
			ThrowLastError(m_strFile);
		}

		// close the file
		CloseHandle(hFile);
	}
	catch(GException &r)
	{
		GString strMessage;
		strMessage.Format(
		"\nFailed to open a configuration file for this application.\n"
		"You may choose one of two options for locating the config file.\n"
		"1. Place the file [%s] in the root of your file system (C:\\ )\n"
		"2. Set the environment variable [%s] to a fully \n"
		"   qualified file name like:\n"
		"       c:\\mypath\\%s             \n"
		"** You must be logged on with read permissions to this file **\n\n\n"
		"%s\n",(const char *)m_strBaseFileName,
		(const char *)m_strEnvOverride, 
		(const char *)m_strBaseFileName,
		(const char *)m_strBaseFileName,
		r.GetDescription());

		throw GException(2, (const char *)strMessage);
	}

#else
	// open the file
	GString strTemp;
	int nResult = strTemp.FromFile((const char *)m_strFile, 0);
	if (nResult == 0)
	{
		GString strMessage;
		strMessage.Format(
		"\nFailed to open a configuration file for this application.\n"
		"You may choose one of two options for locating the config file.\n"
		"1. Place the file [%s] in the root of your file system (/)\n"
		"2. Set the environment variable [%s] to a fully qualified file name like:\n"
		"       /usr/local/app/%s			\n"
		"** You must be logged on as a user with read permissions to this file **\n\n\n"
		"OS reported error code:%d\n",(const char *)m_strBaseFileName,
		(const char *)m_strEnvOverride, 
		(const char *)m_strBaseFileName,
		(const char *)m_strBaseFileName,
		errno);

		throw GException(2, (const char *)strMessage);
	}
	szBuffer = new char[strTemp.Length() + 1];
	memcpy(szBuffer,(const char *)strTemp, strTemp.Length());
	dwSize = strTemp.Length();

#endif


	// terminate the buffer
	szBuffer[dwSize] = 0;
	ProfileParse(szBuffer, dwSize);
	delete [] szBuffer;
}


GProfile::Section *GProfile::FindSection(const char *szSection)
{
	Load();

	Section *pRet = 0;

	GListIterator itSections(&m_lstSections);
	while ( itSections() && (!pRet))
	{
		Section *pSection = (Section *)itSections++;

		if (pSection->m_strName.CompareNoCase(szSection) == 0)
			pRet = pSection;
	}

	return pRet;
}


long GProfile::WriteCurrentConfigSection(GString *pzDestStr, const char *pzSection)
{
	long lret = GProfile::WriteCurrentConfigHelper(0,pzDestStr, pzSection);
	return lret;
}


// Serialize memory config
// Returns the number of bytes written to the destination on success or 0 for failure.
long GProfile::WriteCurrentConfigHelper(const char *pzPathAndFileName, GString *pDest, const char *pzSection/*=0*/)
{
	GString strLocalDest;
	GString *strConfigData = (pDest) ? pDest : &strLocalDest;

	try
	{
		GListIterator itSections(&m_lstSections);
		while ( itSections() )
		{
			Section *pSection = (Section *)itSections++;
			
			if (pzSection)
			{
				if( pSection->m_strName.CompareNoCase(pzSection) != 0)
				{
					continue;
				}
			}
			
			(*strConfigData) << "[" << pSection->m_strName << "]\r\n";

			GListIterator itNVP(&pSection->m_lstNVP);
			while (itNVP())
			{
				NameValuePair *pNVP = (NameValuePair *)itNVP++;
				(*strConfigData) << pNVP->m_strName << "=" << pNVP->m_strValue << "\r\n";
			}
			if (itSections())
				(*strConfigData) << "\r\n\r\n";
		}
		if (pzPathAndFileName)
			strConfigData->ToFile(pzPathAndFileName);

		return (long)strConfigData->Length();
	}
	catch(GException &)
	{
		// most likely due to invalid path or file name.
		return 0;	
	}
	return 0; // can't get here
}



GProfile::NameValuePair *GProfile::FindKey(const char *szKey, GProfile::Section *pSection)
{
	NameValuePair *pRet = 0;
	
	if (pSection)
	{
		GListIterator itNVP(&pSection->m_lstNVP);
		while ((itNVP()) && (!pRet))
		{
			NameValuePair *pNVP = (NameValuePair *)itNVP++;
			if (pNVP->m_strName.CompareNoCase(szKey) == 0)
				pRet = pNVP;
		}
	}

	return pRet;
}

// function retrieves the names of all sections
void GProfile::GetSectionNames(GStringList *lpList)
{
	Load();

	if (lpList)
	{
		GListIterator itSections(&m_lstSections);
		while (itSections())
		{
			Section *pSection = (Section *)itSections++;
			lpList->AddLast(pSection->m_strName);
		}
	}
}

int GProfile::RemoveEntry(const char *szSectionName, const char *szEntry)
{
	Section *pSection = FindSection(szSectionName);
	if (pSection)
	{
		GListIterator itNVP(&pSection->m_lstNVP);
		while (itNVP())
		{
			NameValuePair *pNVP = (NameValuePair *)itNVP++;
			if (pNVP->m_strName.CompareNoCase(szEntry) == 0)
			{
				pSection->m_lstNVP.Remove(pNVP);
				ChangeNotify(szSectionName, szEntry, "");
				return 1;
			}
		}
	}
	return 0;
}

void GProfile::AddSection(GProfile::Section *pS, int bIssueChangeNotification/*=1*/)
{
	if (pS)
	{
		m_lstSections.AddLast(pS);

		// issue a ChangeNotification for each entry in this section
		if (bIssueChangeNotification)
		{
			GListIterator itNVP(&pS->m_lstNVP);
			while ( itNVP() )
			{
				NameValuePair *pNVP = (NameValuePair *)itNVP++;
				ChangeNotify(pS->m_strName, pNVP->m_strName, pNVP->m_strValue);
			}
		}
	}
}

GProfile::Section *GProfile::RemoveSection(const char *szSection)
{
	Section *pS = FindSection(szSection);
	if (pS)
	{
		m_lstSections.Remove(pS);
		return pS;
	}
	return 0;
}



// returns the number of entries for a given section
int GProfile::GetSectionEntryCount(const char *szSectionName)
{
	Section *pSection = FindSection(szSectionName);

	if (pSection)
		return pSection->m_lstNVP.Size();
	
	return 0;
}

const GList *GProfile::GetSection(const char *szSectionName)
{
	Section *pSection = FindSection(szSectionName);

	if (pSection)
		return &pSection->m_lstNVP;
	
	return 0;
}



// function retrieves a boolean from the specified section
short GProfile::GetBool(const char *szSectionName, const char *szKey, short bThrowNotFound /* = true */)
{
	Section *pSection = FindSection(szSectionName);
	if (pSection)
	{
		NameValuePair *pNVP = FindKey(szKey, pSection);
		if (pNVP)
		{
			if (!pNVP->m_strValue.IsEmpty())
			{
				if (pNVP->m_strValue.GetAt(0) == '1')
					return 1;
				if (pNVP->m_strValue.CompareNoCase("Yes") == 0)
					return 1;
				if (pNVP->m_strValue.CompareNoCase("On") == 0)
					return 1;
			}
			return 0;
		}
		else if (bThrowNotFound)
		{
			// throw key not found
			throw GException("Profile", 0, szSectionName, szKey);
		}
	}
	else if (bThrowNotFound)
	{
		// throw key not found
		throw GException("Profile", 1, szSectionName);
	}

	return 0;
}

// function retrieves a boolean from the specified section
short GProfile::GetBoolean(const char *szSectionName, const char *szKey, short bThrowNotFound /* = true */)
{
	Section *pSection = FindSection(szSectionName);
	if (pSection)
	{
		NameValuePair *pNVP = FindKey(szKey, pSection);
		if (pNVP)
		{
			if (pNVP->m_strValue.CompareNoCase("true") == 0 ||
				pNVP->m_strValue.CompareNoCase("on") == 0  ||
				pNVP->m_strValue.CompareNoCase("yes") == 0 ||
				pNVP->m_strValue.CompareNoCase("1") == 0)
			{
				return 1;
			}
			else
			{
				return 0;
			}
		}
		else if (bThrowNotFound)
		{
			// throw key not found
			throw GException("Profile", 0, szSectionName, szKey);
		}
	}
	else if (bThrowNotFound)
	{
		// throw key not found
		throw GException("Profile", 1, szSectionName);
	}

	return 0;
}



// function retrieves a long from the specified section
__int64 GProfile::GetInt64(const char *szSectionName, const char *szKey, short bThrowNotFound)
{
	Section *pSection = FindSection(szSectionName);
	if (pSection)
	{
		NameValuePair *pNVP = FindKey(szKey, pSection);
		if (pNVP)
		{
			if (!pNVP->m_strValue.IsEmpty())
				return Xatoi64((const char *)pNVP->m_strValue);
		}
		else if (bThrowNotFound)
		{
			// throw key not found
			throw GException("Profile", 0, szSectionName, szKey);
		}
	}
	else if (bThrowNotFound)
	{
		// throw key not found
		throw GException("Profile", 1, szSectionName);
	}

	return 0;
}

int	 GProfile::GetInt(const char *szSectionName, const char *szKey, short bThrowNotFound/* = true*/)
{
	return (int)GetLong(szSectionName, szKey, bThrowNotFound);
}

long GProfile::GetLong(const char *szSectionName, const char *szKey, short bThrowNotFound /* = true */)
{
	Section *pSection = FindSection(szSectionName);
	if (pSection)
	{
		NameValuePair *pNVP = FindKey(szKey, pSection);
		if (pNVP)
		{
			if (!pNVP->m_strValue.IsEmpty())
				return atol((const char *)pNVP->m_strValue);
		}
		else if (bThrowNotFound)
		{
			// throw key not found
			throw GException("Profile", 0, szSectionName, szKey);
		}
	}
	else if (bThrowNotFound)
	{
		// throw key not found
		throw GException("Profile", 1, szSectionName);
	}

	return 0;
}

const char *GProfile::GetPath(const char *szSectionName, const char *szKey, short bThrowNotFound)
{
	Section *pSection = FindSection(szSectionName);
	if (pSection)
	{
		NameValuePair *pNVP = FindKey(szKey, pSection);
		if (pNVP)
		{
			if ( !( pNVP->m_strValue.Right(1) == "/" || pNVP->m_strValue.Right(1) == "\\") )
			{
		#ifdef _WIN32
				pNVP->m_strValue += "\\";
		#else
				pNVP->m_strValue += "/";
		#endif
			}
			return pNVP->m_strValue;
		}
		else if (bThrowNotFound)
		{
			// throw key not found
			throw GException("Profile", 0, szSectionName, szKey);
		}
	}
	else if (bThrowNotFound)
	{
		// throw key not found
		throw GException("Profile", 1, szSectionName);
	}

	return 0;
}


int GProfile::DoesExist(const char *szSectionName)
{
	Section *pSection = FindSection(szSectionName);
	if (pSection)
	{
		return 1;
	}
	return 0;
}

int GProfile::DoesExist(const char *szSectionName, const char *szKey)
{
	Section *pSection = FindSection(szSectionName);
	if (pSection)
	{
		NameValuePair *pNVP = FindKey(szKey, pSection);
		if (pNVP)
		{
			return 1;
		}
	}
	return 0;
}


// function retrieves a string from the specified section
const char *GProfile::GetString(const char *szSectionName, const char *szKey, short bThrowNotFound /* = true */)
{
	Section *pSection = FindSection(szSectionName);
	if (pSection)
	{
		NameValuePair *pNVP = FindKey(szKey, pSection);
		if (pNVP)
		{
			return (const char *)pNVP->m_strValue;
		}
		else if (bThrowNotFound)
		{
			// throw key not found
			throw GException("Profile", 0, szSectionName, szKey);
		}
	}
	else if (bThrowNotFound)
	{
		// throw key not found
		throw GException("Profile", 1, szSectionName);
	}

	return 0;
}

unsigned char *GProfile::GetBinary(const char *szSectionName, const char *szKey, GString &strDest)
{
	GString strEncoded(GProfile::GetString(szSectionName, szKey, 0));
	BUFFER b2;
	BufferInit(&b2);
	unsigned int nDecoded;
	uudecode((char *)(const char *)strEncoded, &b2, &nDecoded, false);
	strDest.WriteOn(b2.pBuf,nDecoded);
	BufferTerminate(&b2);
	return (unsigned char *)strDest.Buf();
}

const char *GProfile::GetCiphered(const char *szSectionName, const char *szKey, const char *pzPassword, GString &strDest)
{
	GString strData;
	GetBinary(szSectionName, szKey, strData);
	GString strErrorOut;

	GString strPass(pzPassword);
	DecryptMemoryToMemory(strPass, strData, (int)strData.Length(), strDest, strErrorOut);
	return strDest;
}


long GProfile::WriteCurrentConfig(const char *pzPathAndFileName)
{
	long lret = GProfile::WriteCurrentConfigHelper(pzPathAndFileName,0);
	return lret;
}
long GProfile::WriteCurrentConfig(GString *pzDestStr)
{
	return GProfile::WriteCurrentConfigHelper(0,pzDestStr);
}
short GProfile::GetBoolOrDefault(const char *szSectionName, const char *szKey, short bDefault)
{
	Section *pSection = FindSection(szSectionName);
	if (pSection)
	{
		NameValuePair *pNVP = FindKey(szKey, pSection);
		if (pNVP)
		{
			if (!pNVP->m_strValue.IsEmpty())
			{
				if (pNVP->m_strValue.GetAt(0) == '1')
					return 1;
				if (pNVP->m_strValue.CompareNoCase("Yes") == 0)
					return 1;
				if (pNVP->m_strValue.CompareNoCase("On") == 0)
					return 1;
			}
			return 0;
		}
	}
	return bDefault;
//	short ret;
//	try
//	{
//		ret = GetBool(szSectionName, szKey);
//	}
//	catch(...)
//	{
//		ret = bDefault;
//	}
//	return ret;
}


__int64 GProfile::GetInt64OrDefault(const char *szSectionName, const char *szKey, __int64 nDefault)
{
	__int64 ret;
	try
	{
		ret = GetInt64(szSectionName, szKey);
	}
	catch(...)
	{
		ret = nDefault;
	}
	return ret;
}

long GProfile::GetLongOrDefault(const char *szSectionName, const char *szKey, long lDefault)
{
	long ret;
	try
	{
		ret = GetLong(szSectionName, szKey);
	}
	catch(...)
	{
		ret = lDefault;
	}
	return ret;
}
int	GProfile::GetIntOrDefault(const char *szSectionName, const char *szKey, int nDefault)
{
	int ret;
	try
	{
		ret = GetInt(szSectionName, szKey);
	}
	catch(...)
	{
		ret = nDefault;
	}
	return ret;
}

const char *GProfile::GetStringOrDefault(const char *szSectionName, const char *szKey, const char *pzDefault)
{
	const char *ret;
	try
	{
		ret = GetString(szSectionName, szKey);
	}
	catch(...)
	{
		ret = pzDefault;
	}
	return ret;
}
const char *GProfile::GetPathOrDefault(const char *szSectionName, const char *szKey, const char *pzDefault)
{
	const char *ret;
	try
	{
		ret = GetPath(szSectionName, szKey);
	}
	catch(...)
	{
		ret = pzDefault;
	}
	return ret;
}

int GProfile::ChangeNotify(const char *pzSection, const char *pzEntry, const char *pzValue)
{
	if (!m_pTreeNotify)	// this can only happen if you DetatchNotification()
		return 0;

	GString strKey(pzSection);
	strKey << pzEntry;
	strKey.MakeUpper();
	fnChangeNotify fn  = (fnChangeNotify)m_pTreeNotify->search(strKey);
	if (fn)
	{
		return fn(pzSection, pzEntry, pzValue);
	}
	else 
	{
		// check for [section] monitors
		GString strKey(pzSection);
		strKey.MakeUpper();
		fnChangeNotify fn  = (fnChangeNotify)m_pTreeNotify->search(strKey);
		if (fn)
		{
			return fn(pzSection, pzEntry, pzValue);
		}
	}

	return 0;
}

void GProfile::UnRegisterChangeNotification(const char *pzSection, const char *pzEntry)
{
	if (!m_pTreeNotify)	// this can only happen if you DetatchNotification()
		return;

	if (pzSection)
	{
		GString strKey(pzSection);
		if ( pzEntry && pzEntry[0] )
			strKey << pzEntry;

		strKey.MakeUpper();
		m_pTreeNotify->remove(strKey);

		
		GString str(pzSection);
		str << "\t" << pzEntry;
		lstChangeNotifications.Remove(str, 0);
	}
}

void GProfile::RegisterChangeNotification(const char *pzSection, const char *pzEntry, fnChangeNotify fn)
{
	if (!m_pTreeNotify)	// this can only happen if you DetatchNotification()
		return;

	if (pzSection)
	{
		GString strKey(pzSection);
		if ( pzEntry && pzEntry[0] )
			strKey << pzEntry;

		strKey.MakeUpper();
		m_pTreeNotify->insert(strKey,(void *)fn);

		GString str(pzSection);
		str << "\t" << pzEntry;
		lstChangeNotifications.AddLast(str);
	}
}

int GProfile::ValueLength(const char *szSectionName, const char *szKey)
{
	Section *pSection = FindSection(szSectionName);
	if (pSection)
	{
		NameValuePair *pNVP = FindKey(szKey, pSection);
		if (pNVP)
		{
			return (int)pNVP->m_strValue.Length();
		}
	}
	return 0;
}

GBTree *GProfile::DetatchNotifications()
{
	GBTree *pRet = m_pTreeNotify;
	m_pTreeNotify = 0;
	return pRet;
}
GBTree *GProfile::AttachNotifications(GBTree *pTree)
{
	GBTree *pOld = m_pTreeNotify;
	m_pTreeNotify = pTree;
	return pOld;
}

void GProfile::ExecuteAllNotifications()
{
	if (!m_pTreeNotify)	// this can only happen if you DetatchNotification()
		return;

	GListIterator itSections(&m_lstSections);
	while ( itSections() )
	{
		Section *pSection = (Section *)itSections++;

		GListIterator itNVP(&pSection->m_lstNVP);
		while (itNVP())
		{
			NameValuePair *pNVP = (NameValuePair *)itNVP++;

			GString strKey(pSection->m_strName);
			strKey << pNVP->m_strName;
			strKey.MakeUpper();
			fnChangeNotify fn  = (fnChangeNotify)m_pTreeNotify->search(strKey);
			if (fn)
			{
				fn(pSection->m_strName, pNVP->m_strName, pNVP->m_strValue);
			}
		}
	}
}
