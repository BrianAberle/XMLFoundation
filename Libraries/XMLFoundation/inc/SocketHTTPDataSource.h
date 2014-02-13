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

#ifndef _WINSOCK_HTTP_DS_INC__
#define _WINSOCK_HTTP_DS_INC__

#include "xmlDataSource.h"
#include "FileDataSource.h"

class GString;

class CSocketHTTPDataSource : public CFileDataSource
{
	// a packet buffer
	char m_winsockBuffer[10000];
	const char *m_XML;
	void SendHelper(const char *pzRequest, int nRequestLen, GString &destinationStream, const char *pzNamespace);
public:
	CSocketHTTPDataSource(const char **ppQueryAttributes = 0);
	~CSocketHTTPDataSource();
	const char *send(	const char *szProcedureName, 
						const char *xml, 
						int nXMLLen, 
						void *PooledConnection,
						void **ppUserData, const char *pzNamespace);
	int release(const char *resultFromSend, void *PooledConnection, void *ppUserData);
};


class CSocketError
{
public:
	int m_nErrorCode;
	char m_pzOperation[64];
	CSocketError(int nSocketHandle, const char *pzOperation);
};


class CSocketWrap
{
public:
	int m_nSocketHandle;
	CSocketWrap(int nSocketHandle);
	operator int() {return m_nSocketHandle;}
	~CSocketWrap();
};

#endif //_WINSOCK_HTTP_DS_INC__
