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

#ifndef _FILE_DS_INC__
#define _FILE_DS_INC__

#include "xmlDataSource.h"

class GString;

// a very simple data source for disk or memory input sources
class CFileDataSource : public CXMLDataSource
{
	GString m_strFilePath;
	const char *m_pDirectMemoryBuffer;
	int m_bProcessed;
	int m_bIsFile;
public:
	CFileDataSource(const char **ppQueryAttributes = 0);
	~CFileDataSource();
	// Port -2 means that the address is a path to a file
	// Port -1 means that the address is a path to a directory
	virtual void SetConnectionInfo(const char *szAddress, short nPort);
	void SetConnectionInfo(const char *pzXMLThatSendShouldReturn);
	const char *send(	const char *szProcedureName, 
						const char *xml, 
						__int64 nXMLLen, 
						void *PooledConnection,
						void **ppUserData, const char *pzNamespace);
	int release(const char *resultFromSend, void *PooledConnection, void *ppUserData);
};


#endif //_FILE_DS_INC__
