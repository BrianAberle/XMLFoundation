#include "GlobalInclude.h"
#include "PluginBuilderLowLevelStatic.h"
#include "GSocketHelpers.h"

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
	#include <winsock2.h>
	#include <Ws2tcpip.h>
	#pragma comment(lib, "Ws2_32.lib")
#endif

#include "GThread.h"

LowLevelStaticController::LowLevelStaticController(int nBytesIn, int nBytesExpected, int fd, char *Buf1)
{
	m_nBytesIn = nBytesIn, 
	m_BytesExpected = nBytesExpected;
	m_fd = fd;
	m_Buf1 = Buf1;

};

void LowLevelStaticController::Disconnect()
{
	PortableClose(m_fd,"Plugin1"); 
}

	// returns 1 on success, -1 on error, 0 when no more data, pnMoreBytes will be size of the the data found
int LowLevelStaticController::NextDataChunk(int *pnMoreBytes, char *pDest, int nMax, int nSecondsTimeout, int nMicrosecondsTimeout)
{
	int rslt = readableTimeout(m_fd, nSecondsTimeout, nMicrosecondsTimeout);
	// failure during wait || time limit expired
	if (rslt == -1 || rslt == 0)
	{
//		if(m_TraceConnection && rslt == 0)
//		{
//			GString strErr;
//			strErr.Format("NextDataChunk timeout(%d seconds) code %d",nSecondsTimeout,SOCK_ERR);
//			InfoLog(7000,strErr);
//		}
		return rslt; 
	}

	int nRcvRetryCount = 0;
SUB_RCV_RETRY:		
	int nBytes = recv(m_fd, pDest, nMax, 0 ); // take what ever is available and return to caller
	if (nBytes == -1)
	{
		// 10035 = Resource temporarily unavailable
		if (SOCK_ERR == 10035)
		{
			goto SUB_RCV_RETRY; 
		}
		if(nRcvRetryCount++ < 3 && SOCK_ERR != 10054 && SOCK_ERR != 10038) 
		{
			gsched_yield();
			goto SUB_RCV_RETRY; 
		}
		return -1; 
	}
	if (pnMoreBytes)
	{
		*pnMoreBytes = nBytes;
	}
	return 1; // process the data
}

int LowLevelStaticController::NextDataChunk(int *pnMoreBytes)
{
	int nTimeOutSec = 120;
	int rslt = readableTimeout(m_fd, nTimeOutSec, 0);
	// failure during wait || time limit expired
	if (rslt == -1 || rslt == 0)
	{
//		if(m_TraceConnection && rslt == 0)
//		{
//			GString strErr;
//			strErr.Format("NextDataChunk timeout(%d seconds) code %d",nTimeOutSec,SOCK_ERR);
//			InfoLog(7000,strErr);
//		}
		return rslt; 
	}

	int nRcvRetryCount = 0;
SUB_RCV_RETRY:		
	int nBytes = recv(m_fd, m_Buf1, 64000, 0 ); // take what ever is available and return to caller
	if (nBytes == -1)
	{
		// 10035 = Resource temporarily unavailable
		if (SOCK_ERR == 10035)
		{
			goto SUB_RCV_RETRY; 
		}
		if(nRcvRetryCount++ < 3 && SOCK_ERR != 10054 && SOCK_ERR != 10038) 
		{
			gsched_yield();
			goto SUB_RCV_RETRY; 
		}
		return -1; 
	}
	if (pnMoreBytes)
	{
		*pnMoreBytes = nBytes;
	}
	return 1; // process the data
}

