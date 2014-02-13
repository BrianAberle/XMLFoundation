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
#ifndef _GSocketHelpers__
#define _GSocketHelpers__

class GString;
#ifndef _WIN32
	#ifndef __int64
		#define	__int64 long long	// this definition is also in GString.h
	#endif
#endif


extern GString g_strThisIPAddress;
extern GString g_strThisHostName;
extern int g_nMaxSocketWriteBlock;

int readableTimeout(int fd, int seconds, int microseconds) ;
int nonblockrecvAny(int fd,char *pBuf,int nMaxDataLen, int nTimeOutSeconds = 60, int nTimeOutMicroSeconds = 0);
int nonblockrecv(int fd,char *pBuf,int nExpectedDataLen);
int writableTimeout(int fd, int seconds, int microseconds);
int nonblocksend(int fd,unsigned char *pData,int nDataLen);
int nonblocksend(int fd,const char *pData,int nDataLen);
unsigned __int64 ntoh64(unsigned __int64 n);
unsigned __int64 hton64(unsigned __int64 n);
void IOBlocking(int fd, int isNonBlocking);  // 1 is async(non blocking), 0 is synchronous - blocking
int nonblockrecvHTTP(int fd,char *sockBuffer,int nMaxLen, char **pContentData = 0, int *pnContentLen = 0, int nBytesReadAhead = 0, int nTimeOutSeconds = 30, int bReadHeadersOnly = 0);
int HTTPSend(int fd, const char *pzData, int nDataLen, const char *pzContentType=0);
int PortableClose(int fd, const char *pzFromLocation);
int MakeHTTPHeader(GString &strHeader, unsigned long nDataLen, const char *pzCfgSection = 0, const char *pzAltCfgSection = 0, const char *pzContentType =0, int nKeepAliveTimeout=300,  int nMaxKeepAlives = 150);
void PortableSleep(int nSeconds, int nNanoSeconds); // There are 1,000,000,000 nanoseconds in a second
void EnableSocketHandleDebugTrace( int n );
void SetSocketHandleDebugTrace( void (*pfn) (int, GString &) );


void AddDebugSocket(int fd);
void RemoveDebugSocket(int fd);
int ListOpenSockets(GString &strDest);





// given either an ip address or a host name, this function  returns true if the address is 'this' machine
int IsMe(const char *pzIP);


class AutoInitThisHostAndIP
{
public:
	AutoInitThisHostAndIP();
};


#ifdef _WIN32			
	#define SOCK_ERR WSAGetLastError()
#else
	#define SOCK_ERR errno
#endif


#endif // _GSocketHelpers__