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

#include "GSocketHelpers.h"
#include "GString.h"
#include "GProfile.h"
#include "GThread.h"

#ifdef _WIN32			
	#include <winsock.h>	
#else // Linux & Solaris & HPUX & AIX
	#include <sys/socket.h>
	#include <errno.h>
	#include <netdb.h>		// for gethostbyname()
	#include <unistd.h>		// for gethostname(), execv(). close()
	#include <fcntl.h>

	#include <netinet/in.h> // for inet_ntoa()
	#include <arpa/inet.h>	// for inet_ntoa()
#endif
#ifdef __WINPHONE
	#include <windows.h> // for Sleep()
	#include <winsock2.h>
	#include <Ws2tcpip.h>
	#pragma comment(lib, "Ws2_32.lib")
#endif


#include <stdlib.h>			// for atol(), strtol(), srand(), rand(), atoi()
#include <string.h>			// for strcmp(), memcpy(), memset(), strlen(), strpbrk()

#include <time.h>	
#include "GThread.h"


GString g_strThisIPAddress;
GString g_strThisHostName;
GList g_lstSockethandles;				// for debug tracing of open socket handles


int g_DebugSocketLockInit = 0;
gthread_mutex_t g_DebugSocketMutex;



typedef void(*fnInfoLog)(int, GString &);
fnInfoLog g2_fnLog = 0;
int g2_TraceSocketHandles = 0;
void EnableSocketHandleDebugTrace( int n )
{
	g2_TraceSocketHandles = n;
}
void SetSocketHandleDebugTrace( void (*pfn) (int, GString &) )
{
	g2_fnLog = pfn;
}

int PortableClose(int fd, const char *pzFromLocation)
{
	if (fd != -1)
	{
		
		if (g2_TraceSocketHandles)
		{
			GString strTrace("Close Socket:");
			strTrace << fd << " at [" << pzFromLocation << "]";
			g2_fnLog(888,strTrace);
		}

		#ifdef _WIN32    
			shutdown(fd, 2);
			closesocket(fd);
		#elif _HPUX
			shutdown(fd, 2);
		#else
			close(fd);
		#endif

		RemoveDebugSocket(fd);
	}
	return 1;
}


int ListOpenSockets(GString &strDest)
{
	if (!g_DebugSocketLockInit)
	{
		g_DebugSocketLockInit = 1;
		gthread_mutex_init(&g_DebugSocketMutex, NULL);
	}
	int n = 0;
	gthread_mutex_lock(&g_DebugSocketMutex);
	if (g_lstSockethandles.Size())
	{
		GListIterator it(&g_lstSockethandles);
		while(it())
		{
#if defined(_LINUX64) || defined(_WIN64)  || defined(_IOS)
			strDest << (__int64)it++ << "   ";
#else
			strDest << (int)it++ << "   ";
#endif
			n++;
		}
	}
	gthread_mutex_unlock(&g_DebugSocketMutex);
	return n;
}

void AddDebugSocket(int fd)
{
	if (!g_DebugSocketLockInit)
	{
		g_DebugSocketLockInit = 1;
		gthread_mutex_init(&g_DebugSocketMutex, NULL);
	}
	gthread_mutex_lock(&g_DebugSocketMutex);
	g_lstSockethandles.AddLast((void *)fd);
	gthread_mutex_unlock(&g_DebugSocketMutex);
}
void RemoveDebugSocket(int fd)
{
	if (!g_DebugSocketLockInit)
	{
		g_DebugSocketLockInit = 1;
		gthread_mutex_init(&g_DebugSocketMutex, NULL);
	}
	gthread_mutex_lock(&g_DebugSocketMutex);
	g_lstSockethandles.Remove((void *)fd);
	gthread_mutex_unlock(&g_DebugSocketMutex);
}


// There are 1,000,000,000 nanoseconds in a second
void PortableSleep(int nSeconds, int nNanoSeconds)
{

#ifdef _WIN32
	DWORD dw = (nSeconds * 1000) + (nNanoSeconds / 1000);
	Sleep(dw);
	return;
#endif

#ifdef _LINUX

    struct gtimespec req={0};
    req.tv_sec=nSeconds;
    req.tv_nsec=nNanoSeconds;
    nanosleep(&req,&req);	// in <time.h>
	return;
#else 

// this is terribly non-optimized - look for usleep() on Solaris / AIX / HPUX
// this version of sleep required <phreads.h>

 #ifdef _AIX
		struct timespec ts;
		getclock(TIMEOFDAY, &abstime);
		ts.tv_sec += nSeconds;
 #else
		gtimespec ts;
		ts.tv_sec=time(NULL) + nSeconds;
 #endif

		ts.tv_nsec=nNanoSeconds;
		gthread_cond_t _TimeoutCond;
		gthread_mutex_t _TimeoutLock;
		gthread_mutex_init(&_TimeoutLock,0);
		gthread_cond_init(&_TimeoutCond,0);

		gthread_mutex_lock(&_TimeoutLock);
		gthread_cond_timedwait(&_TimeoutCond, &_TimeoutLock, &ts); 

		gthread_mutex_destroy(&_TimeoutLock);
		gthread_cond_destroy(&_TimeoutCond);

#endif

}




void GYield()
{
	#ifdef _WIN32
			Sleep(0);
	#else
			gsched_yield();
	#endif
}

int g_nMaxSocketWriteBlock = 65536;


int MakeHTTPHeader(GString &strHeader, unsigned long nDataLen, const char *pzCfgSection/* = 0*/, const char *pzAltCfgSection/* = 0*/, const char *pzContentType/*=0*/, int nKeepAliveTimeout/*=300*/,  int nMaxKeepAlives/*150*/)
{
	if (pzCfgSection && pzCfgSection[0])
	{
		GString strProxyFirewall = GetProfile().GetString(pzCfgSection,"ProxyTo",false);
		if ( !strProxyFirewall.Length() && pzAltCfgSection && pzAltCfgSection[0])
			strProxyFirewall = GetProfile().GetString(pzAltCfgSection,"ProxyTo",false);

		if ( strProxyFirewall.Length() )
		{
			strHeader = "POST http://";
			strHeader += strProxyFirewall;
			strHeader += "/ HTTP/1.0\r\n";
			strHeader += "Accept: image/gif, image/x-xbitmap, image/jpeg, image/pjpeg, application/vnd.ms-excel, application/msword, */*\r\n";
			strHeader += "Referer: http://"; strHeader += strProxyFirewall; strHeader += "/\r\n";
			strHeader += "Accept-Language: en-us\r\n";
			strHeader += "Proxy-Connection: Keep-Alive\r\n";
			strHeader += "User-Agent: Mozilla/4.0 (compatible; MSIE 6.0; Windows NT 5.0)\r\n";
			strHeader += "Host: "; strHeader += strProxyFirewall; strHeader +="\r\n";
			strHeader += "Content-Length: "; strHeader += nDataLen; strHeader +="\r\n";
			strHeader += "Pragma: no-cache\r\n\r\n";
			return (int)strHeader.Length();
		}
	}

	
	strHeader = "HTTP/1.1 200 OK\r\n";

	// HTTP header server name
	const char *pWebSrvType=GetProfile().GetString("HTTP","HTTPHeaderServerName",false);
	strHeader += "Server: ";
	if (pWebSrvType && pWebSrvType[0])
		strHeader += pWebSrvType;
	else
		strHeader += "5Loaves";
	strHeader += "\r\n";

	// add the current time
	char pzTime[128];
	struct tm *newtime;

#ifdef WINCE
	unsigned long ltime;
#else
	time_t ltime;
#endif

	time(&ltime);
	newtime = gmtime(&ltime);

	//  According to HTTP 1.1 specification:
	//	"To mark a response as "already expired," an origin server should use
	//   an Expires date that is equal to the Date header value" 
	strftime(pzTime, 128, "Date: %a, %d %b %Y %H:%M:%S GMT\r\n", newtime);
	strHeader += pzTime;
	strftime(pzTime, 128, "Expires: %a, %d %b %Y %H:%M:%S GMT\r\n", newtime);
	strHeader += pzTime;

	//  and Microsoft adds this Cache-Control:
	strHeader += "Cache-Control: no-cache, no-cache\r\n";

	//  and this can only help to guarantee expiration as well.
	strftime(pzTime, 128, "Last-modified: %a, %d %b %Y %H:%M:%S GMT\r\n", newtime);
	strHeader += pzTime;



	strHeader += "Content-Type: ";
	if (pzContentType)
		strHeader += pzContentType;
	else
		strHeader += "text/plain";
	strHeader += "\r\n";

	

	strHeader += "Connection: Keep-Alive\r\n";

	strHeader << "Keep-Alive: timeout=" << nKeepAliveTimeout << ", max=" << nMaxKeepAlives << "\r\n";

	strHeader += "Content-Length: ";
	strHeader += nDataLen;
	strHeader += "\r\n\r\n";

	return (int)strHeader.Length();
}



int HTTPSend(int fd, const char *pzData, int nDataLen, const char *pzContentType/*=0*/)
{
	GString strHeader;
	MakeHTTPHeader(strHeader, nDataLen, 0/*pzCfgSection*/, 0/*pzAltCfgSection*/, pzContentType);
	int nSent = 0;

	
	// consolidate small packets for better performance
	if (nDataLen < 2048)
	{
		strHeader.write(pzData, nDataLen);
		int nRslt = nonblocksend(fd,strHeader, (int)strHeader.Length());
		if (nRslt != -1)
			nSent += nRslt;
		else
		{
#ifdef __SERVER_CORE
			GString str;
			str.Format("HTTPSend fails code:%d\n",(int)SOCK_ERR);
			InfoLog(11, str);
#endif
			return -1;
		}
	}
	else
	{
		int nRslt = nonblocksend(fd,strHeader, (int)strHeader.Length());
		if (nRslt != -1)
			nSent += nRslt;
		else
		{
#ifdef __SERVER_CORE
			GString str;
			str.Format("HTTPSend fails code:%d\n",(int)SOCK_ERR);
			InfoLog(12, str);
#endif
			return -1;
		}
		nRslt = nonblocksend(fd,pzData, nDataLen);
		if (nRslt != -1)
			nSent += nRslt;
		else
		{
#ifdef __SERVER_CORE
			GString str;
			str.Format("HTTPSend fails code:%d\n",(int)SOCK_ERR);
			InfoLog(13, str);
#endif
			return -1;
		}
	}
	return nSent;
}



// There are 1 million microseconds in a second.
int readableTimeout(int fd, int seconds, int microseconds) 
{ 
  struct timeval tv; 
  fd_set rset; 

  tv.tv_sec = seconds; 
  tv.tv_usec = microseconds; 
  
  FD_ZERO(&rset); 
  FD_SET(fd, &rset); 
  
  // returns 0 if the time limit expired.
  // returns -1 on error, otherwise there is data on the port ready to read.
  return select(fd+1, &rset, NULL, NULL, &tv); 
} 

int writableTimeout(int fd, int seconds, int microseconds) 
{ 
  struct timeval tv; 
  fd_set rset; 
   
  tv.tv_sec = seconds; 
  tv.tv_usec = microseconds; 
   
  FD_ZERO(&rset); 
  FD_SET(fd, &rset); 
   
  // returns 0 if the time limit expired.
  // returns -1 on error, otherwise data can be written.
  return select(fd+1, NULL, &rset, NULL, &tv); 
} 


int nonblockrecvAny(int fd,char *pBuf,int nMaxDataLen, int nTimeOutSeconds, int nTimeOutMicroSeconds)
{
	int nRslt;
	int nRecvRetry = 0;
RE_RECV:
	int rslt = readableTimeout(fd, nTimeOutSeconds, nTimeOutMicroSeconds);
	if ( rslt > 0)
	{
		nRslt = recv(fd, pBuf, nMaxDataLen, 0 );
		if (nRslt == 0)
			return -1;
		return nRslt; // -1 or the data size
	}
	else if (rslt == -1)
	{
		if (SOCK_ERR == 10035)
		{
			GYield();
			goto RE_RECV;
		}
		else if(nRecvRetry++ < 3 && SOCK_ERR != 10054 && SOCK_ERR != 10038) 
		{
			GYield();
			goto RE_RECV;
		}
		return -1;
	}

	// rslt = 0, the time limit expired.
	return 0;
}


int nonblockrecv(int fd,char *pBuf,int nExpectedDataLen)
{
	int nBytesIn = 0;
	int nRecvRetry = 0;
RE_RECV:
	while (nBytesIn < nExpectedDataLen)
	{
		int rslt = readableTimeout(fd, 60, 0);
		if ( rslt > 0)
		{
			int nRslt = recv(fd, &pBuf[nBytesIn], nExpectedDataLen - nBytesIn, 0 );
			if (nRslt > 0)
			{
				nBytesIn += nRslt;
			}
		}
		else if (rslt == -1)
		{
			if (SOCK_ERR == 10035)
			{
				GYield();
				goto RE_RECV;
			}
			else if(nRecvRetry++ < 3 && SOCK_ERR != 10054 && SOCK_ERR != 10038) 
			{
				GYield();
				goto RE_RECV;
			}
			return -1;
		}
		else
		{
			break;  // timeout
		}
	}
	if (nBytesIn == nExpectedDataLen)
	{
		return nBytesIn;
	}

	// rslt = 0, the time limit expired.
	return 0;
}


int nonblocksend(int fd,const char *pData,int nDataLen)
{
	int nSendRetry = 0;
	int nSent = 0;
	while(nSent < nDataLen)
	{
		int nRet = writableTimeout(fd, 120, 0);
		if (nRet < 1) // 0  or -1
		{
			return 0; // failed to send code = SOCK_ERR;
		}

		int nToSend = (nDataLen-nSent < g_nMaxSocketWriteBlock) ? nDataLen-nSent : g_nMaxSocketWriteBlock; 

		nRet = send(fd,(const char *)&pData[nSent],nToSend,0);
		if (nRet == -1)
		{ 
			int nErrorCode = SOCK_ERR;
			if (nErrorCode == 10035)
			{
				GYield();
				continue; 
			}
			else if(nSendRetry++ < 3 && nErrorCode != 10054 && nErrorCode != 10038) 
			{
				GYield();
				continue; 
			}
			return 0;
		}
		else
		{
			nSent += nRet;
		}
	}
	return nSent; // bytes sent
}

int nonblocksend(int fd,unsigned char *pData,int nDataLen)
{
	return nonblocksend(fd,(const char *)pData,nDataLen);
}


unsigned __int64 ntoh64(unsigned __int64 n) 
{
	static short int word = 0x0001; // being static, it's only initialized the first time this is called
	static char *byte = (char *) &word;

	if (!byte[0]) // BigEndian		// being dynamic, I didn't have to go change all the make files
		return n;
	else
		return (((unsigned __int64)ntohl((unsigned long)n)) << 32) + ntohl( (unsigned long)(n >> 32) );
}
unsigned __int64 hton64(unsigned __int64 n) 
{
	static short int word = 0x0001;
	static char *byte = (char *) &word;

	if (!byte[0]) // BigEndian
		return n;
	else
		return (((unsigned __int64)htonl((unsigned long)n)) << 32) + htonl((unsigned long)(n >> 32));
}


// 1 is async(non blocking), 0 is synchronus - blocking
void IOBlocking(int fd, int isNonBlocking)
{
	if (isNonBlocking)
	{
// some notes for future ports here:
//  int i = 1;
//	BEOS			setsockopt(fd, SOL_SOCKET, SO_NONBLOCK, &i, sizeof(i));  
//	OTHER-OS's		ioctl(fd, FIONBIO, &i);

		// Set non-blocking IO
		#ifdef _WIN32
			unsigned long icmd = 1;   
			ioctlsocket(fd,FIONBIO,&icmd);
		#else
			int fdflags = fcntl(fd, F_GETFL);
			fcntl(fd, F_SETFL, fdflags | O_NONBLOCK); 
		#endif
	}
	else
	{
		// Set blocking IO
		#ifdef _WIN32
			unsigned long icmd = 0;   
			ioctlsocket(fd,FIONBIO,&icmd);
		#else
			int fdflags = fcntl(fd, F_GETFL);
			fdflags ^= O_NONBLOCK;
			fcntl(fd, F_SETFL, fdflags); 
		#endif
	}
}

// return will equal -10035 when [nTimeOutSeconds] times out
int nonblockrecvHTTP(int fd,char *sockBuffer,int nMaxLen, char **pContentData/* = 0*/, int *pnContentLen/* = 0*/, int nBytesReadAhead/* = 0*/, int nTimeOutSeconds/* = 30*/, int bReadHeadersOnly/* = 0*/)
{
	int nHeaderLength = 0;
	int nContentLength = 0;
	int nCumulativeBytes = 0;
	int nBytes = 0;
	int nPartialHeaderRead = 0;
	
	if (nBytesReadAhead == 0)
		sockBuffer[0] = 0;


	__int64 lStartTime = time(0);   
	int bSkipInitialRead = nBytesReadAhead;
	do
	{
		if (bSkipInitialRead == 0)
		{
KEEP_LOOKING_FOR_DATA:
			int rslt = readableTimeout(fd, nTimeOutSeconds, 0);
			// failure during wait || time limit expired
			if (rslt == -1)
			{
				return SOCK_ERR * -1;
			}
			
			int nMaxRead = nMaxLen - nBytesReadAhead;
			if (nHeaderLength)
				nMaxRead = nHeaderLength + nContentLength - nCumulativeBytes - nBytesReadAhead;

			int sockBufferIndex = nCumulativeBytes + nBytesReadAhead;

			if(!nHeaderLength) // if we don't know the content length yet read byte by byte
			{
				int nHeadIndex = sockBufferIndex;
				int nHeadBytes = 0;
				int nReadHeadBytes = 0;
				int nZeroReads = 0;
				while(1)
				{
KEEP_READING_HEADER:
					if (time(0) - lStartTime > (unsigned long)nTimeOutSeconds) 
					{
						return SOCK_ERR * -1; // total time limit expired while reading HTTP header
					}

					nReadHeadBytes = recv(fd, &sockBuffer[nHeadIndex], 1, 0 );

					if (nReadHeadBytes == -1 && SOCK_ERR == 10035)
					{
						GYield();
						goto KEEP_READING_HEADER;
					}

					if (nReadHeadBytes == 0)
					{
						int rslt = readableTimeout(fd, nTimeOutSeconds, 0);
						if (rslt == -1)
						{
							return SOCK_ERR * -1; // time limit expired in a single read
						}
						GYield();
						if (nZeroReads++ < 3)
							goto KEEP_READING_HEADER;
						return -1;
					}
					if (nReadHeadBytes < 1)
					{
						// -1 the client aborted 
						return SOCK_ERR * -1;
					}
					nHeadBytes += nReadHeadBytes;


//					if (nHeadBytes == 0 && time(0) - lStartTime < (unsigned long)nTimeOutSeconds)
//					{
//						gsched_yield();
//						PortableSleep(1,0);
//						goto KEEP_LOOKING_FOR_DATA;
//					}


					if (sockBuffer[nHeadIndex] == '\n' && nHeadIndex > 4)
					{
						if ( sockBuffer[nHeadIndex-1] == '\r' &&
							 sockBuffer[nHeadIndex-2] == '\n' &&
							 sockBuffer[nHeadIndex-3] == '\r' )
						{
							nBytes = nHeadBytes;
							break;
						}

					}
					nHeadIndex++;
				}
			}
			else // read as much as we can
			{
				nBytes = recv(fd, &sockBuffer[sockBufferIndex], nMaxRead, 0 );
			}

			if (SOCK_ERR == 10035)
			{
				GYield();
				goto KEEP_LOOKING_FOR_DATA;
			}
			if (nBytes < 0)
			{
				// -1 the client aborted 
				return SOCK_ERR * -1;
			}
			if (nBytes == 0 && time(0) - lStartTime < (unsigned long)nTimeOutSeconds)
			{
				GYield();
				
#ifdef _WIN32
				Sleep(1000);
#else
				PortableSleep(1,0);
#endif
				
				goto KEEP_LOOKING_FOR_DATA;
			}
		}
		bSkipInitialRead = 0;


		// parse the HTTP headers in the first read from the stream
		if (!nHeaderLength)
		{
			char *pContent= stristr(sockBuffer,"Content-Length:");
			if (pContent)
			{
				pContent += 15; // first byte of content length in the HTTP header
				nContentLength = atoi(pContent);
				if (pnContentLen)
				{
					*pnContentLen = nContentLength;
				}
				
				
				int nContentTermLen = 4;
				pContent= strstr(pContent,"\r\n\r\n");


				if (pContent)
				{
					pContent += nContentTermLen; // first byte of content data
					if (pContentData)
					{
						*pContentData = pContent;
					}
					nHeaderLength = (int)((pContent) - sockBuffer);
				}
				
				if (bReadHeadersOnly)
				{
					sockBuffer[nBytes] = 0;
					return nBytes;
				}

			}
			else 
			{
				// Expected HTTP reply headers, but found something else
				// maybe "HTTP/1.1 401 Unauthorized"
				if ( strstr(sockBuffer,"\r\n\r\n") )
				{
					return nBytes;
				}
				if (nPartialHeaderRead++ < 2)
				{
					nCumulativeBytes += nBytes;
					goto KEEP_LOOKING_FOR_DATA;
				}
				
				return 0;
			}
			if ( nHeaderLength < 1 || nContentLength < 1)
			{
				// no data
				return 0;
			}
		}
		nCumulativeBytes += nBytes;
	}while( (nCumulativeBytes + nBytesReadAhead < nHeaderLength + nContentLength)  );


	if (nCumulativeBytes + nBytesReadAhead > nHeaderLength + nContentLength)
	{
#ifdef __SERVER_CORE
		GString strLog;
		strLog.Format("packet overrun  %d    %d    %d     %d     %d\n",(int)nCumulativeBytes ,(int)nBytesReadAhead, (int)nHeaderLength , (int)nContentLength, (int)nMaxLen);
		InfoLog(26,strLog);
#endif
	}
	
	// terminate the content with a null if there is room in the buffer
	if (nCumulativeBytes + nBytesReadAhead < nMaxLen)
		sockBuffer[nCumulativeBytes + nBytesReadAhead] = 0;

	return nCumulativeBytes + nBytesReadAhead;
}



int IsMe(const char *pzIP)
{
	if (g_strThisIPAddress.Compare(pzIP) == 0)
		return 1;
	if (g_strThisHostName.CompareNoCase(pzIP) == 0)
		return 1;
	if(strcmp(pzIP,"127.0.0.1") == 0)
		return 1;

	struct hostent *pHELocal = (struct hostent *)gethostbyname(pzIP);
	if (pHELocal)
	{
		struct sockaddr_in my_addr; 
		memcpy((char *)&(my_addr.sin_addr), pHELocal->h_addr,pHELocal->h_length); 
		memset(&(my_addr.sin_zero),0, 8);// zero the rest of the (unused) struct
		
		const char *pzThatIPResolved = inet_ntoa(my_addr.sin_addr);
		if (pzThatIPResolved && pzThatIPResolved[0])
		{
			if (g_strThisIPAddress.Compare(pzThatIPResolved) == 0)
				return 1;
		}
	}

	return 0;
}




AutoInitThisHostAndIP::AutoInitThisHostAndIP()
{
#ifdef _WIN32
	WSADATA wsaData;
	WSAStartup(MAKEWORD( 2, 2 ),&wsaData );
#endif

	char pzHostName[64];
	gethostname(pzHostName,64); //uname() on some systems
	struct hostent *pHELocal = 0;
	pHELocal = (struct hostent *)gethostbyname((const char *)pzHostName);
	if (pHELocal)
	{
		struct sockaddr_in my_addr; 
		memcpy((char *)&(my_addr.sin_addr), pHELocal->h_addr,pHELocal->h_length); 
		memset(&(my_addr.sin_zero),0, 8);// zero the rest of the (unused) struct
		g_strThisIPAddress = inet_ntoa(my_addr.sin_addr);
	}
	else
	{
		// pHELocal is NULL on Fedora Core 6 when host == "localhost.localdomain" - why?
		g_strThisIPAddress = "127.0.0.1"; // todo: this is a problem, we need the real IP.
		// todo: search for the source code ifconfig.c, that will have the resolution
	}
	g_strThisHostName=pzHostName;
}

AutoInitThisHostAndIP g_AutoInitIt;
