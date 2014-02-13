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

#include "FileDataSource.h"
#include "GException.h"
#include "GProfile.h"


CFileDataSource::~CFileDataSource()
{
}

CFileDataSource::CFileDataSource(const char **ppQueryAttributes)
	: CXMLDataSource(ppQueryAttributes,0,0)
{
	m_pDirectMemoryBuffer = 0;
	m_bProcessed = 0;
	m_bIsFile = 0;
}

void CFileDataSource::SetConnectionInfo(const char *szAddress, short nPort)
{
	// Port -2 means that the address is a path to a file
	if (nPort == -2 && szAddress && szAddress[0])
	{
		m_strFilePath = szAddress;
		m_bIsFile = 1;
	}
	// Port -1 means that the address is a path to a directory
	else if (nPort == -1 && szAddress && szAddress[0])
	{
		m_strFilePath = szAddress;
		if ( !( m_strFilePath.Right(1) == "/" || m_strFilePath.Right(1) == "\\") )
		{
	#ifdef _WIN32
			m_strFilePath += "\\";
	#else
			m_strFilePath += "/";
	#endif
		}
	}
	else
	{
		CXMLDataSource::SetConnectionInfo(szAddress, nPort);
	}
}

void CFileDataSource::SetConnectionInfo(const char *pzXML)
{
	m_pDirectMemoryBuffer = pzXML;
}



int CFileDataSource::release(const char *resultFromSend, void *PooledConnection, void *ppUserData)
{
	if (m_bProcessed)
	{
		delete (GString *)ppUserData;
		return 1;
	}
	return 0;
}

const char *CFileDataSource::send(const char *pzProcedureName, 
								  const char *xml, 
								  __int64 nXMLLen, 
								  void *PooledConnection,
								  void **ppUserData,
								  const char *pzNamespace)
{
	// set to 0 assuming we will not service this request
	m_bProcessed = 0;
	const char *pReturnXML = 0;


	const char *pDir = 0;

	// If txml.txt has redirected all client requests to a disk directory
	const char *pDebugResponseFile = GetProfile().GetPath("TXML", "DebugXMLServerEmulation",false);
	if (pDebugResponseFile && pDebugResponseFile[0])
	{
		pDir = pDebugResponseFile;
	}

	// If we're configured to look for responses in a disk directory
	if (pDir || m_strFilePath.Length())
	{
		// calling SetConnectionInfo() overrides txml.txt default
		if (m_strFilePath.Length())
		{
			pDir = m_strFilePath;
		}
		
		// create the file name to look for in strInputFile
		GString strInputFile = pDir;
		if (!m_bIsFile)
		{
			strInputFile += pzProcedureName;
			strInputFile += ".xml";
		}

		// Allocate storage and keep it in the procedure call, not the DataSource.
		GString *strFileBuffer = new GString();
		*ppUserData = (void *)strFileBuffer;

		// if we are looking for a file in the current directory (and on WIN32)
		if (m_strFilePath == ".\\")
		{
			// dont throw an exception just yet if we fail
			if ( !strFileBuffer->FromFile((const char *)strInputFile,0) )
			{
				// also check one directory back "..\" - so that we can run it from inside the 
				// cmd.exe shell in the "Release/Debug" folder and still find the 'source like' 
				// data file with the source, not in "Release" with the generated exe and obj files.
				strInputFile.Prepend(1,'.');
				
				// now throw the exception if we cant find it.
				strFileBuffer->FromFile((const char *)strInputFile); 
			}
		}
		else
		{
			strFileBuffer->FromFile((const char *)strInputFile);
		}
		pReturnXML = (const char *)*strFileBuffer;
		m_bProcessed = 1;// flag for delete
	}
	// or if we're configured to query from memory, return the location
	else if (m_pDirectMemoryBuffer)
	{
		pReturnXML = m_pDirectMemoryBuffer;
	}
	return pReturnXML;
}