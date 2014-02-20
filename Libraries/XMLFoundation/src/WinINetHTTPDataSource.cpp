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

	


// ***************************************************************************
// *******  all source code in this file is compiled only under _WIN32  ******
// ***************************************************************************

#ifdef _WIN32	

#ifndef WINCE	// this will be easy to port to Windows CE when necessary.  
#include "GlobalInclude.h"
static char SOURCE_FILE[] = __FILE__;

#include "WinINetHTTPDataSource.h"
#include "GString.h"
#include "GException.h"
#include "FrameworkAuditLog.h"



#ifdef _countof 
	#undef _countof
#endif
#define _countof(array) (sizeof(array)/sizeof(array[0]))

bool CWinINetHTTPDataSource::Ping()
{
	return 1;
}

void CWinINetHTTPDataSource::ThrowLastError()
{
	LPVOID lpMsgBuf;

	DWORD dwError = GetLastError();
	FormatMessage( 
					FORMAT_MESSAGE_ALLOCATE_BUFFER | 
					FORMAT_MESSAGE_FROM_SYSTEM | 
					FORMAT_MESSAGE_IGNORE_INSERTS |
					FORMAT_MESSAGE_FROM_HMODULE,
					GetModuleHandleA("wininet.dll"),
					dwError,
					MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
					(LPTSTR) &lpMsgBuf,
					0,
					NULL 
				);

	if (lpMsgBuf)
	{
		GException err(dwError, (const char *)lpMsgBuf);
		LocalFree( lpMsgBuf );
		throw err;
	}

	LocalFree( lpMsgBuf );
	GException err(dwError, (const char *)"Unknown wininet error.");
	throw err;
}

CWinINetHTTPDataSource::CWinINetHTTPDataSource(const char **ppQueryAttributes/* = 0*/) : 
	CXMLDataSource(ppQueryAttributes, 0, 0),
	m_hSession(0), 
	m_hConnection(0), 
	m_hFile(0)
{
}

CWinINetHTTPDataSource::~CWinINetHTTPDataSource()
{
	internetDisconnect();
}

void CWinINetHTTPDataSource::IPAddressChange()
{
	if (m_hSession != NULL)
	{
		InternetCloseHandle(m_hSession);
		m_hSession = NULL;
	}

	if (m_hConnection != NULL)
	{
		InternetCloseHandle(m_hConnection);
		m_hConnection = NULL;
	}
}

void CWinINetHTTPDataSource::internetDisconnect()
{
	if (m_hFile != NULL)
	{
		InternetCloseHandle(m_hFile);
		m_hFile = NULL;
	}
	
	IPAddressChange();

}

bool CWinINetHTTPDataSource::internetConnect(const char *pszServer/* = 0*/, int nPort /* = 80 */)
{
	const char *pServer = (const char *)m_strAddress;

	if (pszServer)
		pServer = pszServer;
	// don't connect to an HTTP server if we're going 
	// local we're already at the correct machine.
	if ( !pServer || pServer[0] == 0 || strcmp(pServer,"127.0.0.1") == 0  )
		return false;

	if (m_hSession)
		return true;

	m_hSession = InternetOpenA(
								"Client Session",				/* agent */
								INTERNET_OPEN_TYPE_PRECONFIG ,	/* access type */
								NULL,							/* proxy name */
								NULL,							/* proxy bypass */
								0								/* flags */
							 );

	if (m_hSession == NULL)
		ThrowLastError();

	m_hConnection = InternetConnectA(
						m_hSession, 
						(pServer)? pServer: "127.0.0.1",/*Server or local host*/
						nPort,						// 443 if SSL, usually 80 otherwise
						NULL,						/* user name */
						NULL,						/* user password */ 
						INTERNET_SERVICE_HTTP,		/* service */
						0,							/* flags */
						1							/* context */
								   );

	if (m_hConnection == NULL)
		ThrowLastError();

	return true;
}

char *ReSpace(char *p)
{
	int i =0;
	while( p[i] )
	{
		if ( p[i] == '+' )
			p[i] = ' ';
		i++;
	}
	return p;
}	

char *ExtractIISError(char *pzData)
{
	char *pStart = pzData;
	while(*pzData != '<') 
		pzData++;
	*pzData = 0;
	return pStart;
}

//200-299 Success 
//300-399 Information 
//400-499 Request error 
//500-599 Server error 
void CWinINetHTTPDataSource::HandleProcessReturnCode(DWORD dwRet)
{
	if (dwRet < 400)
		return;
	
	GString err;
	err.Format("www[%d]:",(int)dwRet);
	switch (dwRet)
	{
	case HTTP_STATUS_BAD_REQUEST         :err+="Invalid syntax"; break;
	case HTTP_STATUS_DENIED              :err+="Access denied"; break;
	case HTTP_STATUS_PAYMENT_REQ         :err+="Payment required"; break;
	case HTTP_STATUS_FORBIDDEN           :err+="Request forbidden"; break;
	case HTTP_STATUS_NOT_FOUND           :err+="Object not found"; break;
	case HTTP_STATUS_BAD_METHOD          :err+="Method is not allowed"; break;
	case HTTP_STATUS_NONE_ACCEPTABLE     :err+="No response acceptable to client found"; break;
	case HTTP_STATUS_PROXY_AUTH_REQ      :err+="Proxy authentication required"; break;
	case HTTP_STATUS_REQUEST_TIMEOUT     :err+="Server timed out waiting for request"; break;
	case HTTP_STATUS_CONFLICT            :err+="User should resubmit with more info"; break;
	case HTTP_STATUS_GONE                :err+="The resource is no longer available"; break;
	case HTTP_STATUS_LENGTH_REQUIRED     :err+="The server refused to accept request w/o a length"; break;
	case HTTP_STATUS_PRECOND_FAILED      :err+="Precondition given in request failed"; break;
	case HTTP_STATUS_REQUEST_TOO_LARGE   :err+="Request entity was too large"; break;
	case HTTP_STATUS_URI_TOO_LONG        :err+="Request URI too long"; break;
	case HTTP_STATUS_UNSUPPORTED_MEDIA   :err+="Unsupported media type"; break;
	case HTTP_STATUS_SERVER_ERROR        :err+="Internal server error"; break;
	case HTTP_STATUS_NOT_SUPPORTED       :err+="/XMLServer/UBT.dll not available"; break;
	case HTTP_STATUS_BAD_GATEWAY         :err+="Error response received from gateway"; break;
	case HTTP_STATUS_SERVICE_UNAVAIL     :err+="Temporarily overloaded"; break;
	case HTTP_STATUS_GATEWAY_TIMEOUT     :err+="Timed out waiting for gateway"; break;
	case HTTP_STATUS_VERSION_NOT_SUP     :err+="Version not supported"; break;
	default:
		//400-499 Request error 
		//500-599 Server error 
		err.Format("Unknown Error %d",(int)dwRet);
	}
	if (err.Length())
	{
		GException exp(dwRet, (const char *)err);
		throw exp;
	}
}


const char *CWinINetHTTPDataSource::SendViaHTTP(const char *xml, 
												__int64 nXMLLen, 
												int nPort, 
												const char *pzHTMLDoc, 
												const char *pServer,
												const char *pszEntry/* = "TransactXML="*/,
												GString &XMLResults)
{
	if (m_hFile != NULL)
	{
		InternetCloseHandle(m_hFile);
		m_hFile = NULL;
	}

	const char *pszHTML  = "XMLServer/UBT.dll";

	DWORD dwFlags = INTERNET_FLAG_DONT_CACHE;
	if (nPort == 443)
		dwFlags |= INTERNET_FLAG_SECURE;

RE_POST:

	DWORD dwBytes;
	
	if (m_hFile == NULL)
		m_hFile = HttpOpenRequestA(
								  m_hConnection,	
								  "POST", 
								  pszHTML,
								  NULL,
								  NULL, 
								  NULL, 
								  dwFlags, 
								  1
								 );

	if (m_hFile == NULL) // invalid handle starts here
		ThrowLastError();

	try
	{
		INTERNET_BUFFERS buffer;
		memset(&buffer, 0, sizeof(buffer));
		buffer.dwStructSize = sizeof(buffer);
		char szBuffer[80];
		DWORD dwLen = _countof(szBuffer);
		DWORD dwRet;
		if (!pzHTMLDoc)
		{
			static DWORD nCount = (DWORD)strlen(pszEntry);
			buffer.dwBufferTotal = nCount + nXMLLen;
RE_SEND1:
			if (!HttpSendRequestEx(m_hFile, &buffer, NULL, HSR_INITIATE, 1))
			{
					DWORD dwError = GetLastError();

					// For most implementations this is not an error
					if (dwError == ERROR_INTERNET_HTTPS_TO_HTTP_ON_REDIR ||
						dwError == ERROR_INTERNET_HTTP_TO_HTTPS_ON_REDIR ||
						dwError == ERROR_INTERNET_INVALID_CA ||
						dwError == ERROR_INTERNET_SEC_CERT_DATE_INVALID ||
						dwError == ERROR_INTERNET_SEC_CERT_CN_INVALID)
					{
						DWORD dwFlags;
						DWORD dwBuffLen = sizeof(dwFlags);

						InternetQueryOption (m_hFile, INTERNET_OPTION_SECURITY_FLAGS,
							(LPVOID)&dwFlags, &dwBuffLen);
						dwFlags |= SECURITY_FLAG_IGNORE_UNKNOWN_CA;
						dwFlags |= SECURITY_FLAG_IGNORE_REDIRECT_TO_HTTPS;
						dwFlags |= SECURITY_FLAG_IGNORE_REDIRECT_TO_HTTP;
						dwFlags |= SECURITY_FLAG_IGNORE_REVOCATION;
						dwFlags |= SECURITY_FLAG_IGNORE_WRONG_USAGE;
						dwFlags |= SECURITY_FLAG_IGNORE_CERT_DATE_INVALID;
						dwFlags |= SECURITY_FLAG_IGNORE_CERT_CN_INVALID;
						InternetSetOption (m_hFile, INTERNET_OPTION_SECURITY_FLAGS,
											&dwFlags, sizeof (dwFlags) );	
						goto RE_SEND1;
					}
					else
					{
						ThrowLastError();
					}
			}
			if (!InternetWriteFile(m_hFile, pszEntry, nCount, &dwBytes))
				ThrowLastError();
			if (dwBytes != nCount)
				ThrowLastError();
		}
		else
		{
			if (nXMLLen && xml)
			{
				buffer.dwBufferTotal = nXMLLen;
				if (!HttpSendRequestEx(m_hFile, &buffer, NULL, HSR_INITIATE, 1))
					ThrowLastError();
			}
			else
			{
RE_SEND:
				BOOL bRet = HttpSendRequest(m_hFile, 0, 0, 0, 0);
				if(!bRet)
				{
					DWORD dwError = GetLastError();

					// Ignore IE Security Errors
					if (dwError == ERROR_INTERNET_HTTPS_TO_HTTP_ON_REDIR ||
						dwError == ERROR_INTERNET_HTTP_TO_HTTPS_ON_REDIR ||
						dwError == ERROR_INTERNET_INVALID_CA ||
						dwError == ERROR_INTERNET_SEC_CERT_DATE_INVALID ||
						dwError == ERROR_INTERNET_SEC_CERT_CN_INVALID)
					{
						DWORD dwFlags;
						DWORD dwBuffLen = sizeof(dwFlags);

						InternetQueryOption (m_hFile, INTERNET_OPTION_SECURITY_FLAGS,
							(LPVOID)&dwFlags, &dwBuffLen);
						dwFlags |= SECURITY_FLAG_IGNORE_UNKNOWN_CA;
						dwFlags |= SECURITY_FLAG_IGNORE_REDIRECT_TO_HTTPS;
						dwFlags |= SECURITY_FLAG_IGNORE_REDIRECT_TO_HTTP;
						dwFlags |= SECURITY_FLAG_IGNORE_REVOCATION;
						dwFlags |= SECURITY_FLAG_IGNORE_WRONG_USAGE;
						dwFlags |= SECURITY_FLAG_IGNORE_CERT_DATE_INVALID;
						dwFlags |= SECURITY_FLAG_IGNORE_CERT_CN_INVALID;
						InternetSetOption (m_hFile, INTERNET_OPTION_SECURITY_FLAGS,
											&dwFlags, sizeof (dwFlags) );	
						goto RE_SEND;
					}
					else
					{
						ThrowLastError();
					}
				}
			}
		}
		// write the XML Data
		if (nXMLLen && xml)
		{
			if (!InternetWriteFile(m_hFile, xml, nXMLLen, &dwBytes))
				ThrowLastError();
			if (dwBytes != (DWORD)nXMLLen)
				ThrowLastError();	
			if (!HttpEndRequest(m_hFile, NULL, 0, 1))
			{
				// This has failed with 12152
				// 12152 = ERROR_HTTP_INVALID_SERVER_RESPONSE
				// DWORD dw = GetLastError();
				// ThrowLastError();
			}
		}

		if (HttpQueryInfo(m_hFile, HTTP_QUERY_STATUS_CODE, szBuffer, &dwLen, NULL))
			dwRet = (DWORD)atol(szBuffer);
		if (dwRet == HTTP_STATUS_PROXY_AUTH_REQ) 
		{
			// Attempt Proxy Auth and resend (This code may not work) 

			// This is some bizarre interworking between IE
			// and the Microsoft Proxy Server.  We even hired
			// Microsoft Techs and all they could come up with 
			// was a work around by disabling some type of
			// proxy authentication from the MS Proxy Server Manager.
			// This occurred in 1 of every 100 or so installs
			// that used microsoft networking and MS Proxy Server.
			// This issue may have been resolved with newer versions of
			// MS Proxy Server or IE
			if ( InternetErrorDlg (GetDesktopWindow(),
				  m_hFile, ERROR_INTERNET_INCORRECT_PASSWORD,
				  FLAGS_ERROR_UI_FILTER_FOR_ERRORS |
				  FLAGS_ERROR_UI_FLAGS_GENERATE_DATA |
				  FLAGS_ERROR_UI_FLAGS_CHANGE_OPTIONS,
				  NULL) == ERROR_INTERNET_FORCE_RETRY)
						goto RE_SEND; 
		}

		HandleProcessReturnCode(dwRet);
	}
	catch(GException &e)
	{
		if (e.GetError() == 12031)
		{
			internetDisconnect();
			internetConnect(pServer, nPort);
			goto RE_POST;
		}

		throw e;
	}
	catch(...)
	{
		throw GException("Socket data soruce", 2002);
	}

 
	try
	{
		char szBuffer[12288];
 
		do
		{
			// Start writing at the beginning of the current block
			if (!InternetReadFile(m_hFile, (LPVOID)szBuffer, 12288, &dwBytes))
				ThrowLastError();
 
			if (dwBytes > 4 && memcmp(szBuffer, "Error", 5) == 0 )
			{
				throw GException("Socket data soruce", 2003, &szBuffer[5]);
			}
			else if ( dwBytes > 29 && memcmp(szBuffer,"<BODY><H1>BAD REQUEST</H1><P>",29) == 0 )
			{
				throw GException("Socket data soruce", 2003, ExtractIISError(&szBuffer[29]));
			}
			if (dwBytes)
				XMLResults.write(szBuffer, dwBytes);
		}
		while (dwBytes);
		return XMLResults;
	}
	catch(GException &e)
	{
		if (e.GetError() == 12019)
			goto RE_POST;
		throw e;
	}
	catch(...)
	{
		throw GException("Socket data soruce", 2004);
	}
	return 0;
}


const char *CWinINetHTTPDataSource::send(const char *pzProcedureName, 
								  const char *xml, 
								  __int64 nXMLLen, 
								  void *PooledConnection,
								  void **ppUserData,
								  const char *szCommandPrefix/*Default: 'TransactXML=' */)
{
	internetConnect(m_strAddress, m_nPort);

	GString *XMLResults = new GString(100000);
	GString **pp = (GString **)ppUserData;
	GString *p = *pp;
	p = XMLResults;

	try
	{
		return SendViaHTTP(xml, nXMLLen, m_nPort, 0, m_strAddress, szCommandPrefix, *XMLResults);
	}
	catch(...)
	{
		delete XMLResults;
		p = 0;
		throw;
	}
}

int CWinINetHTTPDataSource::release(const char *resultFromSend, void *PooledConnection, void *pUserData)
{
	delete (GString *)pUserData;
	return 1;
}

#endif // #ifdef WINCE


#endif // #ifdef _WIN32
