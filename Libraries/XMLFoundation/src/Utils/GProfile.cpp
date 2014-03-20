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
//#include <stdlib.h> //for: getenv()
#include <string.h> //for: memcpy()
#include <stdio.h>  //for: fopen(),fclose();
#ifndef _WIN32
	#include <errno.h>
#endif
#ifdef _WIN32
#include <Windows.h>
#endif
#include "TwoFish.h"




IMPLEMENT_FACTORY(GProfileSection, section);
IMPLEMENT_FACTORY(GProfileEntry, setting);
IMPLEMENT_FACTORY(GProfile, configuration)
void GProfile::MapXMLTagsToMembers()
{
	MapMember(&m_lstSections, GProfileSection::GetStaticTag());
}
void GProfileEntry::MapXMLTagsToMembers()
{
	MapAttribute(&m_strName,"name");
	MapAttribute(&m_strValue,"value");
}
void GProfileSection::MapXMLTagsToMembers()
{
	MapAttribute(&m_strName,"name");
	MapMember(&m_lstNVP, GProfileEntry::GetStaticTag());
}


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


// create a profile from the supplied file name
GProfile::GProfile(const char *pzFilePathAndName, bool bIsXML/* = 0 */)
	: m_bCached(0)
{
	m_bIsXML = bIsXML;
	m_pTreeNotify = new GBTree();
	if (pzFilePathAndName && pzFilePathAndName[0])
	{
		// use the supplied pzFilePathAndName - it will be loaded when data is called for
		m_strFile = pzFilePathAndName;
	}
}


//
// load the profile configuration file yourself, 
// and create this object "with no disk config file"
GProfile::GProfile(const char *szBuffer, __int64 dwSize, bool bIsXML/* = 0*/)
{
	m_bIsXML = bIsXML;
	m_pTreeNotify = new GBTree();
	if (m_bIsXML)
	{
		FromXMLX(szBuffer);
		m_bCached = true;
	}
	else
	{
		ProfileParse(szBuffer, dwSize);
	}
}


// create an empty profile
GProfile::GProfile()
	: m_bCached(0)
{
	m_pTreeNotify = new GBTree();
	m_strFile = "NONE";
	m_bIsXML = 0;
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
	GProfileEntry *pNVP;
	GProfileSection *pSection = FindSection(szSectionName);
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
			pNVP = new GProfileEntry;
			pSection->m_lstNVP.AddLast(pNVP);
			pNVP->m_strName = szKey;
			pNVP->m_strValue = pzValue;
		}
	}
	else
	{
		// new section
		pSection = new GProfileSection;
		pSection->m_strName = szSectionName;
		m_lstSections.AddLast(pSection);

		// new key
		pNVP = new GProfileEntry;
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
		GProfileSection *pSection = (GProfileSection *)itSections++;

		GListIterator itNVP(&pSection->m_lstNVP);
		while (itNVP())
		{
			GProfileEntry *pNVP = (GProfileEntry *)itNVP++;
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

void GProfile::GetLine(GString &strLine, const char *szBuffer,  __int64 *nIdx,  __int64 dwSize)
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
	GProfileSection *pSection = 0;

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
			pSection = new GProfileSection;
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
			GProfileEntry *pNVP = new GProfileEntry;
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

	if (m_bIsXML)
	{
		FromXMLFile(m_strFile);
		m_bCached = true;
		return;
	}


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
	catch(GException &)
	{
		GString strMessage("\nFailed to open a configuration file for this application.\n");
		throw GException(2, (const char *)strMessage);
	}

#else
	// open the file
	GString strTemp;
	int nResult = strTemp.FromFile((const char *)m_strFile, 0);
	if (nResult == 0)
	{
		GString strMessage("\nFailed to open a configuration file for this application.\n");

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


GProfileSection *GProfile::FindSection(const char *szSection)
{
	Load();

	GProfileSection *pRet = 0;

	GListIterator itSections(&m_lstSections);
	while ( itSections() && (!pRet))
	{
		GProfileSection *pSection = (GProfileSection *)itSections++;

		if (pSection->m_strName.CompareNoCase(szSection) == 0)
			pRet = pSection;
	}

	return pRet;
}


long GProfile::WriteCurrentConfigSection(GString *pzDestStr, const char *pzSection, bool bWriteXML/* = 0*/)
{
	long lret = GProfile::WriteCurrentConfigHelper(0,pzDestStr, pzSection, bWriteXML);
	return lret;
}


// Serialize memory config
// Returns the number of bytes written to the destination on success or 0 for failure.
long GProfile::WriteCurrentConfigHelper(const char *pzPathAndFileName, GString *pDest, const char *pzSection/*=0*/, bool bWriteXML/*=0*/)
{
	GString strLocalDest;
	GString *strConfigData = (pDest) ? pDest : &strLocalDest;

	try
	{
		if (bWriteXML)
		{
		    *strConfigData << "<configuration>\n";
		}

		GListIterator itSections(&m_lstSections);
		while ( itSections() )
		{
			GProfileSection *pSection = (GProfileSection *)itSections++;
			if (pzSection)
			{
				if( pSection->m_strName.CompareNoCase(pzSection) != 0)
				{
					continue;
				}
			}
			if (bWriteXML)
			{
				pSection->ToXML(strConfigData, 1);
			}
			else
			{
				(*strConfigData) << "[" << pSection->m_strName << "]\r\n";

				GListIterator itNVP(&pSection->m_lstNVP);
				while (itNVP())
				{
					GProfileEntry *pNVP = (GProfileEntry *)itNVP++;
					(*strConfigData) << pNVP->m_strName << "=" << pNVP->m_strValue << "\r\n";
				}
				if (itSections())
					(*strConfigData) << "\r\n\r\n";
			}
		}
		if (bWriteXML)
		{
		    *strConfigData << "\r\n</configuration>";
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



GProfileEntry *GProfile::FindKey(const char *szKey, GProfileSection *pSection)
{
	GProfileEntry *pRet = 0;
	
	if (pSection)
	{
		GListIterator itNVP(&pSection->m_lstNVP);
		while ((itNVP()) && (!pRet))
		{
			GProfileEntry *pNVP = (GProfileEntry *)itNVP++;
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
			GProfileSection *pSection = (GProfileSection *)itSections++;
			lpList->AddLast(pSection->m_strName);
		}
	}
}

int GProfile::RemoveEntry(const char *szSectionName, const char *szEntry)
{
	GProfileSection *pSection = FindSection(szSectionName);
	if (pSection)
	{
		GListIterator itNVP(&pSection->m_lstNVP);
		while (itNVP())
		{
			GProfileEntry *pNVP = (GProfileEntry *)itNVP++;
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

void GProfile::AddSection(GProfileSection *pS, int bIssueChangeNotification/*=1*/)
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
				GProfileEntry *pNVP = (GProfileEntry *)itNVP++;
				ChangeNotify(pS->m_strName, pNVP->m_strName, pNVP->m_strValue);
			}
		}
	}
}

GProfileSection *GProfile::RemoveSection(const char *szSection)
{
	GProfileSection *pS = FindSection(szSection);
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
	GProfileSection *pSection = FindSection(szSectionName);

	if (pSection)
		return pSection->m_lstNVP.Size();
	
	return 0;
}

const GList *GProfile::GetSection(const char *szSectionName)
{
	GProfileSection *pSection = FindSection(szSectionName);

	if (pSection)
		return &pSection->m_lstNVP;
	
	return 0;
}



// function retrieves a boolean from the specified section
short GProfile::GetBool(const char *szSectionName, const char *szKey, short bThrowNotFound /* = true */)
{
	GProfileSection *pSection = FindSection(szSectionName);
	if (pSection)
	{
		GProfileEntry *pNVP = FindKey(szKey, pSection);
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
	GProfileSection *pSection = FindSection(szSectionName);
	if (pSection)
	{
		GProfileEntry *pNVP = FindKey(szKey, pSection);
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
	GProfileSection *pSection = FindSection(szSectionName);
	if (pSection)
	{
		GProfileEntry *pNVP = FindKey(szKey, pSection);
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
	GProfileSection *pSection = FindSection(szSectionName);
	if (pSection)
	{
		GProfileEntry *pNVP = FindKey(szKey, pSection);
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
	GProfileSection *pSection = FindSection(szSectionName);
	if (pSection)
	{
		GProfileEntry *pNVP = FindKey(szKey, pSection);
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
	GProfileSection *pSection = FindSection(szSectionName);
	if (pSection)
	{
		return 1;
	}
	return 0;
}

int GProfile::DoesExist(const char *szSectionName, const char *szKey)
{
	GProfileSection *pSection = FindSection(szSectionName);
	if (pSection)
	{
		GProfileEntry *pNVP = FindKey(szKey, pSection);
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
	GProfileSection *pSection = FindSection(szSectionName);
	if (pSection)
	{
		GProfileEntry *pNVP = FindKey(szKey, pSection);
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


long GProfile::WriteCurrentConfig(const char *pzPathAndFileName, bool bWriteXML/* = 0*/)
{
	long lret = GProfile::WriteCurrentConfigHelper(pzPathAndFileName,0,0,bWriteXML);
	return lret;
}
long GProfile::WriteCurrentConfig(GString *pzDestStr, bool bWriteXML/* = 0*/)
{
	return GProfile::WriteCurrentConfigHelper(0,pzDestStr,0,bWriteXML);
}
short GProfile::GetBoolOrDefault(const char *szSectionName, const char *szKey, short bDefault)
{
	GProfileSection *pSection = FindSection(szSectionName);
	if (pSection)
	{
		GProfileEntry *pNVP = FindKey(szKey, pSection);
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
	GProfileSection *pSection = FindSection(szSectionName);
	if (pSection)
	{
		GProfileEntry *pNVP = FindKey(szKey, pSection);
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
		GProfileSection *pSection = (GProfileSection *)itSections++;

		GListIterator itNVP(&pSection->m_lstNVP);
		while (itNVP())
		{
			GProfileEntry *pNVP = (GProfileEntry *)itNVP++;

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
