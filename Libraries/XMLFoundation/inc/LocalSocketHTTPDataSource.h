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
#ifndef _SOCKET_LOCAL_HTTP_DS_INC__
#define _SOCKET_LOCAL_HTTP_DS_INC__


#include "SocketHTTPDataSource.h"

#include "SQLExecution.h"
#include "DataAbstraction.h"
#include "xmlServer.h"


class CFileDataSource;
class GString;
class CLocalSocketHTTPDataSource : public CSocketHTTPDataSource
{
	GString *m_pzLocalXML;
	CFileDataSource *m_pFileDataSource;

public:
	CLocalSocketHTTPDataSource(const char **ppQueryAttributes = 0);

	const char *send(const char *pzProcedureName, 
				  const char *xml, 
				  int nXMLLen, 
				  void *PooledConnection,
				  void **ppUserData,const char *pzNameSpace = "TransactXML=");

	int release(const char *resultFromSend, void *PooledConnection,void *ppUserData);

	void SetFileDataSource(CFileDataSource *pFileDataSource)
	{
		m_pFileDataSource = pFileDataSource;
	}
};


#endif //_SOCKET_LOCAL_HTTP_DS_INC__