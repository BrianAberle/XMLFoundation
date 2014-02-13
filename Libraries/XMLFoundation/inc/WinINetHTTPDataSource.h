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

#ifndef _WININET_HTTP_DS_INC__
#define _WININET_HTTP_DS_INC__

#ifdef _WIN32 
// this Type of DataSource is only available on 95/98/ME/NT/00.
// functionally the same as SocketHTTPDataSource, but this one supports
// SSL when the port = 443.


#include "XMLDataSource.h"

// microsoft makes me include wtypes.h, before Wininet.h can be included.
#include <wtypes.h>		
#include "Wininet.h"

class GString;
class CWinINetHTTPDataSource : public CXMLDataSource
{
	enum {
		_HTTP_VERB_MIN      = 0,
		HTTP_VERB_POST      = 0,
		HTTP_VERB_GET       = 1,
		HTTP_VERB_HEAD      = 2,
		HTTP_VERB_PUT       = 3,
		HTTP_VERB_LINK      = 4,
		HTTP_VERB_DELETE    = 5,
		HTTP_VERB_UNLINK    = 6,
		_HTTP_VERB_MAX      = 6,
	};

	HINTERNET m_hSession;
	HINTERNET m_hConnection;
	HINTERNET m_hFile;
	
	bool internetConnect(const char *pzIP = 0, int nPort = 80 );
	void internetDisconnect();
	
	const char *SendViaHTTP(const char *xml, __int64 nXMLLen, int nPort, const char *pzHTMLDoc, const char *pIP,const char *pszEntry, GString &XMLResults);
	void HandleProcessReturnCode(DWORD dwRet);
	void ThrowLastError();

public:
	virtual void IPAddressChange();
	
	CWinINetHTTPDataSource(const char **ppQueryAttributes = 0);
	~CWinINetHTTPDataSource();
	
	void AddAttributes(XMLObject *pObj);
	virtual bool Ping();

	const char *send(	const char *szProcedureName, 
						const char *xml, 
						__int64 nXMLLen, 
						void *PooledConnection,
						void **ppUserData,
						const char *pzNameSpace);


	int release(const char *resultFromSend, void *PooledConnection, void *pUserData);
	
};

#endif // _WIN32 
#endif //_WININET_HTTP_DS_INC__
