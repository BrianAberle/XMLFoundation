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
	#include "WinCERuntimeC.h" // for errno
#endif

#include "SocketHTTPDataSource.h"
#include "GString.h"
#include "GException.h"
#include "GProfile.h"


#include <stdio.h> // for: sprintf()
#include <string.h> // for: strlen() memcpy(), memset() strcat()
#ifndef _WIN32
#include <errno.h> // for global errno an all OS's except MSFT
#endif

#ifndef WINCE
	#include <sys/types.h>
#endif


#ifdef _WIN32
	// NT only
	#include <winsock.h>
	#ifdef __WINPHONE
		#include <windows.h>
		#include <winsock2.h>
		#include <ws2tcpip.h>
		#include <errno.h>
		#pragma comment(lib, "ws2_32.lib")
	#endif
#else 
	// _LINUX && __sun &&  _HPUX
	#include <netinet/in.h>
	#include <sys/socket.h>
	#include <netdb.h> // for gethostbyname() and hostent structure definition
	#include <arpa/inet.h>	// for inet_addr struct
#endif

#ifdef __sun 
	#include <unistd.h>		// for close()
#endif
#ifdef _LINUX
	#include <unistd.h>		// for close()
#endif
#ifdef _IOS
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <fcntl.h>
	#include <unistd.h>
#endif


CSocketHTTPDataSource::CSocketHTTPDataSource(const char **ppQueryAttributes/* = 0*/) : 
	CFileDataSource(ppQueryAttributes)
{
}

CSocketHTTPDataSource::~CSocketHTTPDataSource()
{
}

void CSocketHTTPDataSource::SendHelper(const char *pzRequest, int nRequestLen, GString &destinationStream, const char *pzNamespace)
{
	int fd, numbytes;  
#ifdef _WIN32
	WSADATA wsaData;
	WSAStartup(MAKEWORD( 1, 0 ), &wsaData);
#endif

	int nNamespaceLen = (int)strlen(pzNamespace);


	struct sockaddr_in their_addr; 
	their_addr.sin_family = AF_INET;      
	their_addr.sin_port = htons(m_nPort); 
	their_addr.sin_addr.s_addr = inet_addr (GetServerAddress());
	if (their_addr.sin_addr.s_addr == -1)
	{
		// resolve a DNS server name if inet_addr() came up empty.
		struct hostent *pHE = (struct hostent *)gethostbyname(GetServerAddress());
		if (pHE == 0)
		{ 
			GString strError;
			strError.Format("gethostbyname() failed to resolve[%s]",GetServerAddress());
			throw CSocketError(errno,(const char *)strError);
		}
		memcpy((char *)&(their_addr.sin_addr), pHE->h_addr,pHE->h_length); 
	}
	memset(&(their_addr.sin_zero),0, 8);//zero the rest of the struct


	if ((fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) 
	{	
		throw CSocketError(0,"socket");
	}
	CSocketWrap sockfd(fd); 


	// local bind() is not required on WIN32 platform
	struct sockaddr_in localAddr; 
	localAddr.sin_family = AF_INET;
	localAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	localAddr.sin_port = htons(0);
	int rc = bind(fd, (struct sockaddr *) &localAddr, sizeof(localAddr));
	if(rc<0) 
	{
		GString strError;
		strError.Format("failed to bind([%s]:%d)",GetServerAddress(),(int)m_nPort);
		throw CSocketError(errno,(const char *)strError);
	}


	if (connect(fd, (struct sockaddr *)&their_addr, sizeof(struct sockaddr)) == -1) 
	{
		GString strError;
		strError.Format("failed to connect([%s]:%d)",GetServerAddress(),(int)m_nPort);
		throw CSocketError(errno,(const char *)strError);
	}

	// 200 bytes in excess
	char *pSendBuf = new char[200 + nRequestLen + nNamespaceLen];
	sprintf(pSendBuf,"POST /XMLServer/UBT.dll?%s HTTP/1.1\r\nUser-Agent: XML_Object_Framework_DataSource\r\nHost: 100.100.100.100\r\nContent-Length:  ",pzNamespace);
	
	GString strTemp;
	strTemp.Format("%d\r\n\r\n%s",(int)nNamespaceLen + nRequestLen,pzNamespace);
	strcat(&pSendBuf[120],(const char *)strTemp);
	int nHeaderLen = (int)strlen(&pSendBuf[120]) + 120;
	strcat(&pSendBuf[120],pzRequest);


	int nTotalMessageLength = nHeaderLen + nRequestLen;


	if ((numbytes=sendto(sockfd, pSendBuf, nTotalMessageLength, 0, 
		 (struct sockaddr *)&their_addr, sizeof(struct sockaddr))) == -1) 
	{
		delete[] pSendBuf;
		throw CSocketError(sockfd,"sendto");
	}
	delete[] pSendBuf;

	int nCumulativeBytes = 0;
	numbytes = 0;
	do
	{
		if ((numbytes=recv(sockfd, m_winsockBuffer, sizeof(m_winsockBuffer), 0)) == -1) 
		{
			continue;
		}
		destinationStream.write(m_winsockBuffer,numbytes);
		nCumulativeBytes += numbytes;
	}while( numbytes != 0 );
}


const char *CSocketHTTPDataSource::send(const char *pzProcedureName, 
								  const char *xml, 
								  int nXMLLen, 
								  void *PooledConnection,
								  void **ppUserData,
								  const char *pzNamespace)
{
	// service the request locally if conf'd by txml.txt or SetConnectionInfo()
	const char *pXML = CFileDataSource::send(pzProcedureName,xml,nXMLLen,PooledConnection,ppUserData,pzNamespace);

	if (pXML)
		return pXML;

	char *pHTTP = 0;
	
	GString *XMLResults = new GString(100000);
	GString **pp = (GString **)ppUserData;
	GString *p = *pp;
	p = XMLResults;

	// go to the server for the response
	int nAttempts = 0;
	for (int i = 0; i < 1; i++)
	{
		try
		{
			SendHelper( xml, nXMLLen, *XMLResults, pzNamespace );
			nAttempts = 0;
			pHTTP = (char *)(const char *)*XMLResults;
			// XML starts after HTTP 1.1 headers
			pXML = pHTTP;
			while(memcmp(pXML,"HTTP",4)==0)
			{
				// advance past the HTTP header(s)
				pXML = strstr(pXML,"\r\n\r\n") + 4;
			}
		}catch( CSocketError &rErr )
		{
			nAttempts++;
			i--;
			if (nAttempts > 3)
			{
				throw GException("Socket data soruce", 2001, rErr.m_nErrorCode, rErr.m_pzOperation);
			}
		}
	}
	return pXML;
}


int CSocketHTTPDataSource::release(const char *resultFromSend, void *PooledConnection, void *ppUserData)
{
	// if the base class can't delete it
	if (!CFileDataSource::release(resultFromSend,PooledConnection,ppUserData))
	{
		// then do it here
		delete (GString *)ppUserData;
	}
	return 1;
}



CSocketError::CSocketError(int nSocketHandle, const char *pzOperation)
{
	strcpy(m_pzOperation,pzOperation);
	m_nErrorCode = 0;
#ifdef _WIN32		
	m_nErrorCode = WSAGetLastError();
#endif
	if (!m_nErrorCode)
	{
		int size = sizeof(m_nErrorCode);
#ifdef _WIN32
		getsockopt(nSocketHandle, SOL_SOCKET, SO_ERROR, (char *) &m_nErrorCode, &size);
#else
		getsockopt(nSocketHandle, SOL_SOCKET, SO_ERROR, (char *) &m_nErrorCode, (socklen_t *)&size);
#endif

		if (!m_nErrorCode)
			m_nErrorCode = errno;
		if (!m_nErrorCode)
			m_nErrorCode = -1;
	}

}

CSocketWrap::CSocketWrap(int nSocketHandle)
{
	m_nSocketHandle = nSocketHandle;
}

CSocketWrap::~CSocketWrap()
{
#ifdef _WIN32    
	closesocket(m_nSocketHandle);
	WSACleanup();
#elif _HPUX
	shutdown(m_nSocketHandle,2);
#else
	close(m_nSocketHandle);
#endif
}

