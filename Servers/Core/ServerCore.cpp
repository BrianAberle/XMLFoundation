// --------------------------------------------------------------------------
//						United Business Technologies
//			  Copyright (c) 2000 - 2014  All Rights Reserved.
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
#define __SERVER_CORE

#include "XMLFoundation.h"	// lists, sorts, xmlparser, object factory, cipher, compression, and other app framework tools

// comment out the following line to allow JIT Debugging(just In Time) to be hooked by your debugger
//#define _XMLF_JIT_


#ifdef _WIN32 // Windows 32 bit, 64 bit, Windows Mobile, WinCE, and Windows Phone all define _WIN32
	#ifdef WINCE
		#include "WinCERuntimeC.h" // Adds common C Runtime functions to Windows CE.
		#include <types.h>
		typedef int socklen_t;
		#define EBUSY  16
		#define ETIMEDOUT WSAETIMEDOUT
	#elif __WINPHONE
		#include <time.h>			// for tm struct and time()	
		#include <Ws2tcpip.h>		
		#include <errno.h>
		#include <direct.h>			// for mkdir()
	#else // 32 and 64 bit and also older MSC compilers
		#define socklen_t int
		#include <time.h>			// for tm struct and time()	
		#include <io.h>			// for _chmod() in _WIN32
		#include <process.h>		// for _spawnv()
		#include <direct.h>		// for mkdir()
		#include <errno.h>
		#if defined(_MSC_VER) && _MSC_VER <= 1200 
			#ifndef ETIMEDOUT
			  #define ETIMEDOUT               WSAETIMEDOUT
			#endif
		#endif
	#endif
#else // Linux & Solaris & HPUX & AIX & Android & iOS
	#include <arpa/inet.h>	// for inet_addr()
	#include <netdb.h>		// for gethostbyname()
	#include <unistd.h>		// for gethostname(), execv()
#endif

#ifdef _ANDROID
	#include <errno.h>		// for definition of EBUSY (16) Note: Needed ONLY for Android port
	// Android NDK pthreads does not have a pthread_cancel() so ServerCore's calls to gthread_cancel 
	// are #defined to pthread_cancel and resolved by this function here for the Andriod build
	int pthread_cancel(pthread_t pthread_id)
	{
		pthread_kill(pthread_id, SIGUSR1);
		return 0;
	}
#endif

// common to all platforms
#include <ctype.h>		// for isprint()
#include <stdlib.h>		// for atol(), strtol(), srand(), rand(), atoi()
#include <string.h>		// for strcmp(), memcpy(), memset(), strlen(), strpbrk()

#ifndef WINCE
	#include <sys/stat.h>	// for stat struct, stat(), and file stats flags
#endif
 
#ifdef _AIX
	#include <sys/select.h>		// for nonblocked IO event select()
#endif

#include "IntegrationLanguages.h" // integration to Perl, Python, Java, C++, VB, COM, CORBA, and TransactXML extensions
GList g_lstActivePlugins;


#ifdef ___XFER
	#include "../../Libraries/Xfer/inc/XferCommand.h"		// Xfer Protocol
	#include "../../Libraries/Xfer/inc/XferProtocol.h"		// Xfer Protocol
	#include "../../Libraries/Xfer/inc/XferDispatch.h"		// Xfer Protocol
	#include "../../Libraries/Xfer/inc/XferSwitchBoard.h"	// Xfer Protocol
	// Note: you must also link to "../../Libraries/Release/Xfer.lib" or "../../Libraries/Debug/Xfer.lib" or you can include the source like this:
	#ifdef ___XFER_SRC
		#include "../../Libraries/Xfer/src/XferCommand.cpp"		// Xfer Protocol
		#include "../../Libraries/Xfer/src/XferProtocol.cpp"	// Xfer Protocol
		#include "../../Libraries/Xfer/src/XferDispatch.cpp"	// Xfer Protocol
		#include "../../Libraries/Xfer/src/XferSwitchBoard.cpp"	// Xfer Protocol
	#else
		#pragma comment(lib, "../../Libraries/Xfer.lib")	
	#endif
#else
#endif //___XFER

#include "SwitchBoard.h"		
#include "GSocketHelpers.h" // nonblocksend(), nonblockrecvAny(), readableTimeout() writableTimeout() ntoh64(), SOCK_ERR

#include "GPerformanceTimer.h" // only used for getTimeMS() definition
#include "PluginBuilderLowLevelStatic.h"

#ifdef ___TXML
 #include "ServerGlobal.cpp"	// UBT Server extension for TransactXML
#else
 #define OBJECT_PRECACHE
 #define SERVER_MEMORY_EXTENSION
 #define SERVER_MEMORY_CLEANUP
 #define SERVER_MEMORY_RESET
 #define PROPIGATE_SERVER_EXCEPTION
#endif

#ifndef _NO_GZIP_
 #include "GZip.h"
#endif


// MAX_RAW_CHUNK can be any size you choose, but realistically it should not be less than 8192 in most cases, and it will reject big packets from the standard build, that may be desirable in some cases.
// This thread buffer doubles as the kernel TCP buffer. Each thread will allocate 2 buffers of this size.  The value should align on a page boundary, since the waste will be multiplied relative to the number of threads if it does not.
#define MAX_RAW_CHUNK		65536					// 64k max size of raw packet data, this is 8X larger than the OS kernel default in NT, cut it in half if you don't have memory to burn and the performance loss will be minimal.
// note: Xfer does assume that MAX_RAW_CHUNK > 2048, it can be smaller - but you must search for "1024 is not magic" and adjust it accordingly if you run very small packets.  Other than that no assumptions are made in the code.
#define MAX_DATA_CHUNK		MAX_RAW_CHUNK + 2048	// max size of compressed/ciphered data packet, it is possible for compressed data to expand a little under worst case scenerios
// The actual formula for buffer expansion is = (n * .01) + 600, so this size is safe and reserves working space that is used to prefix data such as HTTP headers, 256 bytes is the most Xfer ever currently uses.
// **** Note: Make thise values match XFER_RAW_CHUNK, and XFER_DATA_CHUNK respectively


// Data conversion attributes. 
#ifndef _ATTRIBS_DEFINED
#define _ATTRIBS_DEFINED
#define ATTRIB_SYNCRONIZED				0x0001
#define ATTRIB_COMPRESSED_GZIP			0x0002
#define ATTRIB_COMPRESSED_BZIP			0x0004
#define ATTRIB_CRYPTED_2FISH			0x0008
#define ATTRIB_UNUSED_BIT5_RESERVED		0x0010	// search [bValidDataFlags] in XferDispatch.cpp
#define ATTRIB_UNUSED_BIT6_RESERVED		0x0020	// for an explanation of what the reserved bits are for.
#define ATTRIB_UNUSED_BIT7_RESERVED		0x0040
#define ATTRIB_UNUSED_BIT8_RESERVED		0x0080
#define ATTRIB_UNUSED_BIT9				0x0100
#define ATTRIB_UNUSED_BIT10				0x0200
#define ATTRIB_UNUSED_BIT11				0x0400
#define ATTRIB_UNUSED_BIT12				0x0800
#define ATTRIB_UNUSED_BIT13				0x1000
#define ATTRIB_UNUSED_BIT14				0x2000
#define ATTRIB_UNUSED_BIT15				0x4000
#define ATTRIB_UNUSED_BIT16				0x8000
#endif




///////////////////////////////////////////////////////////////////////////////////////////////////
// Turn off Nagle Algorithm.  This is a major item to note with respect to performance within Internet protocols.  
// The Nagle Algorithm is all about improving performance of TCP applications via a generalized buffering algorithm.
// ServerCore uses it's own buffering system.  There are two "Large" system buffers.  Go ask Nagle.  Mine are big.
// Large is relative to year and device.  Nagle didnt have 2 64k system buffers.  I buffer better than Nagle, 
// especially considering that all of the protocols I have implemented via ServerCore 'hooks' (designed after ServerCore)
// use the reserved system buffers within the protocol design.  Nagle generally improves TCP application performance
// however Nagle can't help this server - it only slows it down and wastes memory.
#ifndef TCP_NODELAY 
	#define TCP_NODELAY 1   
#endif
///////////////////////////////////////////////////////////////////////////////////////////////////

// These are 3 Major Logic threads that a connection uses.  Needless to say - they use thread synchronization
void *clientThread(void *);
void *ProxyHelperThread(void *);
void *listenThread(void *);

// The FilePoster example uses this, this functioning "switchboard" implementation has changed little since since 2002.
SwitchBoard			g_SwitchBoard;

// global variables
int g_bEnableXferSwitchBoard = 0;
GString g_strMessagingSwitchPath;
int g_bEnableMessagingSwitchboard = 0; 

gthread_mutex_t PoolExpansionLock;
GArray *g_PoolClientThread;
GArray *g_PoolProxyHelperThread;

int g_ServerIsShuttingDown = 0;
int g_ThreadPoolSize = 0;
int g_ProxyPoolSize = 0;
int g_ThreadIDCount = 0;
int g_TraceTransferSizes = 0;
int g_TraceIn = 0;
int g_TraceOut = 0;
int g_TraceSocketHandles = 0;
int g_TraceHTTPBalance = 0;
int g_nRrandSeed = 777;		
int g_DataTrace = 0;
int g_ThreadPing = 0;
int g_PingListenThread = 0;
int g_TraceConnection = 0;
int g_HTTPHeaderTrace = 0;
int g_TraceHTTPFiles = 0;
int g_TraceThread = 0;
int g_LogStdOut = 0;
int g_listenThreadsRunning = 0;
int g_bPartialHeaderWait = 1;
int g_nPartialHeaderWaitSec = 5;	// 5:		normally HTTP headers arrive in a single read from the network, if a header is partially received allow N seconds for the rest of the header.  This guards against a certain type denial of service attack.
int g_nProxyReadWait = 75000;		// 75000:	in nanoseconds, time to block on IO reads in the proxy thread
int g_nProxyIdleReadWait = 1000000; // 1000000:	idle connection should block longer on IO reads
int g_nKernelSocketRcvBuf = 32768;	// 32768:	4 * default(8192 in 2K/XP/Vista)  If we see limits reached on page locks (ERROR_INSUFFICIENT_RESOURCES or WSAENOBUFS)  reduce the size
int g_nListenQueue = 64;			// 64:		Connections in this queue should not exist for more than a few microseconds.
int g_nProxyIdleAt = 20;			// 20:		idle proxy enters a lower processor time slice after [g_nProxyIdleAt] network reads at[g_nProxyReadWait] of no network activity.  Data on the network automatically re-elevates the time slice.
int g_nMaxHTTPHeaderSize = 4096;	// 4096:	4096 is plenty unless you are doing something custom
int g_StopDoubleBuffer = 1;			// 1:		when on(1) this sets SO_SND_BUF to 0, preventing double buffering since this server goes to great lengths to provide optimal buffering, when off(0) the TCP Nagle algorithm will queue up small data writes - but normally there are'nt any.  Some protocols will need Nagle, and they should turn it on themselves.
int g_bDisableLog = 0;
int g_NoLog = 0;
int g_nBlockedPortPenalty = 777;
int g_bUseHTTPKeepAlives = 1;
unsigned int g_nMaxConnections = 0xFFFFFFFF;
int g_nTimeLimit = 0;
int g_LogCache = 1; 
int g_nKeepAliveTimeout = 300;  // the timeout= in the Keep-Alive of the HTTP header  ("HTTP","KeepAliveTimeOut")
int g_nMaxKeepAlives = 150;		// the max= in the Keep-Alive of the HTTP header  ("HTTP","KeepAliveMax")

GString g_strLogFile;
GString g_strListeningOnPorts;


// These two variables are only used for thread tuning statistical information
unsigned int g_ThreadUsageMax = 0;
//unsigned int g_ThreadUsageCurrent = 0;

// stats for clientthread(), there are [g_ThreadPoolSize] of them
long g_nClientthreadsInUse = 0;;
long g_nClientthreadsPeakUse = 0;

// stats for ProxyThread(), there are [g_ProxyPoolSize] of them
long g_nProxyThreadsInUse = 0;;
long g_nProxyThreadsPeakUse = 0;

int g_nLockThreadBuffers = 1;


#ifndef _WIN32
char g_chSlash = '/';
#else
char g_chSlash = '\\';
#endif




// defined in in GString.cpp, K & R forgot this one
char * stristr(const char * str1, const char * str2); 


static const char *g_pzCoreErrorSection = "ServerCore";

unsigned int g_RandomNumbers[8];

time_t g_ServerStartTime;

// statistic counters
__int64 g_TotalHits = 0;
long g_TotalConnections = 0;
long g_ActiveConnections = 0;

__int64 g_Total404s = 0;
__int64 g_TotalTooBusy = 0;
__int64 g_PreZipBytes = 0;
__int64 g_PostZipBytes = 0;






typedef void(*CoreInfoLog )(int, GString &);
CoreInfoLog g_CoreLog = 0;
void SetServerCoreInfoLog( void (*pfn) (int, GString &) )
{
	g_CoreLog = pfn;
}

GString g_strCachedLog;
GString g_strPreLog;

gthread_mutex_t g_csMtx3;
int g_csMtxInit3 = 0;
void InfoLog(int nMsgID, GString &strSrc)
{
	if(g_CoreLog)
	{
		if (!g_csMtxInit3)
		{
			g_csMtxInit3 = 1;
			gthread_mutex_init(&g_csMtx3, NULL);
		}
		gthread_mutex_lock(&g_csMtx3);
		try
		{
			// This allows an application to trap special messages for dynamic debug breaks of special message notification handlers.
			strSrc.Prepend(g_strPreLog);
			g_CoreLog(nMsgID,strSrc);

		}catch(...)
		{
			// because if g_CoreLog throws for any reason the whole server would deadlock to death over logging
		}
		gthread_mutex_unlock(&g_csMtx3);
	}

	if (g_NoLog)	// if logging is off because (disk full/ disk error/ bad log path)
		return;

	if (g_bDisableLog) // if logging is off because the user has disabled it
		return;

	// Note that we dont use any logging related CPU until we know logging is enabled
	strSrc.Prepend(g_strPreLog);


	//  note: g_strLogFile may not have a value at startup( == ""), but it may get a value shortly thereafter so we should cache the log if so configured
	time_t ltime;
	time(&ltime);
	char pzTime[128];
	struct tm *newtime;
	newtime = localtime(&ltime);
	strftime(pzTime, 128, "%H:%M:%S", newtime);
	
	GString strTime;
	strTime.Format("%s[%03d] %s",pzTime,nMsgID,(const char *)strSrc);

	strTime.TrimRightWS();
	strTime += "\t\r\n\r\n";  // the '\t' is to make a record delimiter that will not occur in the data.  HTTP headers contain "\r\n\r\n" and may be logged.

	if (!g_csMtxInit3)
	{
		g_csMtxInit3 = 1;
		gthread_mutex_init(&g_csMtx3, NULL);
	}
	
	gthread_mutex_lock(&g_csMtx3);
	try
	{
		if (g_LogCache)
		{
			g_strCachedLog << strTime;
			if (g_strCachedLog.Length() > 32000)
			{
				g_strCachedLog.ToFileAppend(g_strLogFile);
				g_strCachedLog.Empty();
			}
		}
		else
		{
			strTime.ToFileAppend(g_strLogFile);
		}

	}catch(...)
	{
		if (!g_LogStdOut)
			g_NoLog = 1; // quit trying
	}
	gthread_mutex_unlock(&g_csMtx3);


		
	if (g_LogStdOut)
	{
		printf(strSrc.Buf());
		printf("\n");
	}
}
void InfoLog(int nMsgID, const char *pDataToLog)
{
	if (g_NoLog)
		return;
	GString str(pDataToLog);
	InfoLog(nMsgID, str);
}
void InfoLogFlush()
{
	if (g_LogCache && g_strCachedLog.Length())
	{
		gthread_mutex_lock(&g_csMtx3);
		try
		{
			g_strCachedLog.ToFileAppend(g_strLogFile);
			g_strCachedLog.Empty();
		}
		catch(...)
		{
		}
		gthread_mutex_unlock(&g_csMtx3);
	}
}



void DataTrace(const char *pzTag, const char *pData, int nBytes, const char *pzFileOverride = 0)
{
	GString strLabel;
	if (memcmp(pzTag,"raw",3) == 0)
	{
		strLabel.Format("\n\n\n\n\n%d Bytes %s:\n",(int)nBytes,pzTag);
	}
	else
	{
		strLabel.Format("\n\n%d Bytes %s:\n",(int)nBytes,pzTag);
	}


	// Format it
	GString strBinary(strLabel);
	strBinary.FormatBinary((unsigned char *)pData,nBytes);


	// write it to disk
	if (pzFileOverride && pzFileOverride[0])
	{
		strBinary.ToFileAppend(pzFileOverride);
	}
	else
	{
		strBinary.ToFileAppend(GetProfile().GetString("Trace","DataTraceFile",false));
	}
}







////////////////////////////////////////////////////////////////////////////////////
//
// External Connection Cache - currently used for connections to network proxy
//
////////////////////////////////////////////////////////////////////////////////////
GHash g_hshConnections;
gthread_mutex_t g_csMtxConnCache;
struct CacheConnectEntry
{
	long time;
	int fd;
	GString strServer;
};
int HasCachedConnection(int *fd, const char *strServer)
{
	gthread_mutex_lock(&g_csMtxConnCache);
	try
	{

FIND_ENTRY:
	CacheConnectEntry *pCCE = (CacheConnectEntry *)g_hshConnections.RemoveKey(strServer);
	if (pCCE)
	{
		// hold longer depending on the protocol
		// connections to the network HTTP proxy will not be kept for long.
		if ( time(0) - pCCE->time > 15 ) 
		{
			// ********* EXPIRED *********
			PortableClose(pCCE->fd,"Core1");

			goto FIND_ENTRY;
		}
		else
		{
			// ********* CACHE HIT *********
			*fd = pCCE->fd;
			delete pCCE;
		}
	}
	else
	{
		*fd = -1;
	}
	}
	catch( GException &rErr )
	{
		InfoLog(1, rErr.GetDescription());
	}
	catch(...)
	{
	}
	gthread_mutex_unlock(&g_csMtxConnCache);

	return (*fd != -1);
}
void CacheConnection(int fd, const char *strServer)
{
//  to disable the cache:
//	closesocket(fd);
//	return;


	static long lastExpCheck = 0; // needs mutexed on RISC machines

	gthread_mutex_lock(&g_csMtxConnCache);
	try
	{

	CacheConnectEntry *pCCE = new CacheConnectEntry;
	long lNow = time(0);
	pCCE->fd = fd;
	pCCE->time = lNow;
	pCCE->strServer = strServer;
	g_hshConnections.Insert(strServer, (void *)pCCE);
	if (lNow - lastExpCheck > 15)
	{
		// ********* CLEANUP ROUTINE *********
		lastExpCheck = lNow;
		// check every entry in the hash for expired connections
		GStringList lstRemoveStack;
		GHashIterator it(&g_hshConnections);
		while (it())
		{
			CacheConnectEntry *p = (CacheConnectEntry *)it++;
			if (lNow - p->time > 15)
			{
				lstRemoveStack.AddLast(p->strServer);
			}
		}
		//	********* lstRemoveStack.Size() items EXPIRED *********
		if (!lstRemoveStack.isEmpty())
		{
			GStringIterator itL(&lstRemoveStack);
			while(itL())
			{
				CacheConnectEntry *pCCE = (CacheConnectEntry *)g_hshConnections.RemoveKey(itL++);
				PortableClose(pCCE->fd,"Core2");
				delete pCCE;
			}
		}
	}
	}
	catch( GException &rErr )
	{
		InfoLog(2, rErr.GetDescription());
	}
	catch(...)
	{
	}
	gthread_mutex_unlock(&g_csMtxConnCache);
}


// Init application instance random numbers
void QuickInitRandomNumbers()
{
	// capture the server start time, used for admin statistic info.
	time(&g_ServerStartTime);

	// seed the system random number generator
	srand( g_ServerStartTime );

	// increment each by time() seeded rand() to give them a starting value
	for(int nIndex=0; nIndex < 8; nIndex++)
		g_RandomNumbers[nIndex] += rand();

#ifdef ___XFER
	memcpy(&g_nXferTxnID,g_RandomNumbers,sizeof(g_nXferTxnID));
#endif //___XFER
}
// since the psudo random seed could be a security issue, this defeats it, this is random
void InitRandomNumbers()
{
	InfoLog(3, "Random Key Init:");

	// capture the server start time, used for admin statistic info.
	time(&g_ServerStartTime);

	// seed the system random number generator
	srand( g_ServerStartTime );



	// increment each by time() seeded rand() to give them a starting value
	for(int nIndex=0; nIndex < 8; nIndex++)
		g_RandomNumbers[nIndex] += rand();


	// each number is incremented for a 'random()' amount of time. 
	// some machines are faster than others, and current system load also affects the values.
	for(int n_RandomNumberWorkerIndex=0; n_RandomNumberWorkerIndex < 8; n_RandomNumberWorkerIndex++)
	{
		unsigned long nYieldTime = 1 + (rand() % 7);
		unsigned long lStartTime = time(0);

		// round and round she goes, where she stops nobody knows.
		while (time(0) - lStartTime < nYieldTime)
		{
			g_RandomNumbers[n_RandomNumberWorkerIndex]++;
			
			// context switching randomizes the numbers better and magnifies the 
			// effects of other applications currently using system resource.
			gsched_yield(); 
		}
	}

	// now re-seed the random generator(with 8 different CPU-Cycle based seed numbers), 
	// then increment each value by rand() for each seed.
	for(int nSeedCount=0; nSeedCount<8; nSeedCount++)
	{
		srand( g_RandomNumbers[nSeedCount] );
		for(int nIndex2=0; nIndex2 < 8; nIndex2++)
			g_RandomNumbers[nIndex2] += rand();	
	}
	InfoLog(6,"Done");

	// finally make sure none of the numbers are == 0, it breaks other algorithms
	// that depend on them as valid seed numbers
	for(int nIndex3=0; nIndex3<8; nIndex3++)
	{
		if (g_RandomNumbers[nIndex3] == 0)
		{
			srand( time(0) );
			g_RandomNumbers[nIndex3] = rand();
			if (g_RandomNumbers[nIndex3] == 0)		
				g_RandomNumbers[nIndex3] = 777; // any non-zero number.
		}
	}

#ifdef ___XFER
	memcpy(&g_nXferTxnID,g_RandomNumbers,sizeof(g_nXferTxnID));
#endif // ___XFER
}



// Allows paths like this "/usr/path\subdir" or "c:\my/folder/file.ext"
const char *ReSlash(const char *pzInParam)
{
	char *pzIn = (char *)pzInParam; 
	int nLen = (int)strlen(pzInParam);
	for(int i=0; i<nLen; i++ )
	{
		if (pzIn[i] == '\\' || pzIn[i] == '/')
		{
			pzIn[i] = g_chSlash;
		}
	}
	return pzIn;
}



const char *GenerateCertificationData(GString &strDest)
{
	// variable length is allowed, here we always use 64 bytes
	int i;
	for(i=0; i < 64; i++)
	{
		strDest << (char)((rand() % 90) + 32);  
	}

	strDest.Replace( '/', '.' ); // no slashes allowed in random data
	strDest.Replace( '\0', '.' ); // no nulls allowed in random data

	// upper-case the key parts
	char *key = strDest.Buf();
	int len = (int)strlen(key);
	for (i = 0; i < len; i++)
		key[i] = toupper(key[i]);


	return strDest;
}



// This is a governor for the number of connections from a single 'reported' IP
// over the course of a specified time.  This feature prevents a type of
// 'denial of service' attack where the vandal steals your CPU time
// with a flood of instant 'hang-up calls', until a legitimate customer
// can no longer get through.  Each entry is in a hand-made unbalanced 
// tree to optimize insertion speed and reduce memory overhead.  
// The tree grows forever, so it may be trimmed by the FrequencyTimeRelease
// entry under the [System] section of the config file specifying the interval
// in seconds that the tree should be released.
class IPLog
{
public:
	int  nTime[64];					// an 'first in - last out' stack of connection times
	IPLog *Left;					// in a tree.
	IPLog *Right;
	char pzIP[32];					// for this IP address
	IPLog(const char *pIP)
	{
		memset(nTime,0,sizeof(nTime));
		strcpy((char *)pzIP,pIP);
		nTime[0] = time(0);
		Left = 0;
		Right = 0;
	}
};
IPLog *g_LogRoot = 0;
int g_FrequencyTimeRelease = 0;

void TimedRelease( IPLog* Root )
{
	if(Root)
	{
		if( Root->Left )
			TimedRelease( Root->Left );

		if( Root->Right )
			TimedRelease( Root->Right );

		delete Root;
	}
}

// This had to be extremely fast because the "Global refuse list" is searched for every new connection
// this custom built linked list has no unnecessary support (not doubly linked) (uses native types)
class IPQueryEntry
{
public:
	IPQueryEntry *pNext;
	unsigned long nRefuseUntil; 
	int nRefusedCount;
	GString strReason;

	unsigned long nIPBegin;
	unsigned long nIPEnd;
	IPQueryEntry(unsigned long IP)
	{
		nRefusedCount = 0;
		nRefuseUntil = 0;
		nIPBegin = IP;
		nIPEnd = IP;
		pNext=0;
	}
	IPQueryEntry(unsigned long IP1, unsigned long IP2)
	{
		nRefusedCount = 0;
		nRefuseUntil = 0;
		nIPBegin = IP1;
		nIPEnd = IP2;
		pNext=0;
	}
};
IPQueryEntry *g_FirstRefusedIP = 0;
IPQueryEntry *g_LastRefusedIP = 0;

IPQueryEntry *g_FirstAllowedIP = 0;
IPQueryEntry *g_LastAllowedIP = 0;


// return a numerical value of a "N.N.N.N" char representation ip address.
unsigned long IpToValue(const char *pzIp) 
{
#if !defined(_WIN32) && !defined(_INTEL) 
	return inet_addr(pzIp);
#else
	return ntohl(inet_addr("127.0.0.1"));
#endif
}


// dynamically flag an ip address as 'refuse service to' for some duration of seconds
void AddRefuseService(const char *pzIP, int nRefuse, const char *pzReason)
{
	IPQueryEntry *pRip = 0;
	unsigned long nNow = time(0);
	int bMadeNew = 0;

	// walk the list of refused IP's looking for an empty(expired) slot
	IPQueryEntry *pCurrRefusedIP = g_FirstRefusedIP;
	while (pCurrRefusedIP)
	{
		if (pCurrRefusedIP->nRefuseUntil < nNow && pCurrRefusedIP->nRefuseUntil != 0)
		{
			// flag it as taken so we do not collide with another thread
			pCurrRefusedIP->nRefuseUntil = nNow + 10;
			pCurrRefusedIP->nIPBegin = 0;
			pCurrRefusedIP->nIPEnd = 0;
			pCurrRefusedIP->nRefusedCount = 0;

			pRip = pCurrRefusedIP;
			break;
		}
		pCurrRefusedIP = pCurrRefusedIP->pNext;
	}

	// if there was not an expired one laying around make a new one
	if (!pRip)
	{
		pRip = new IPQueryEntry(IpToValue(pzIP));
		bMadeNew = 1;
	}
	else
	{
		pRip->nIPBegin = IpToValue(pzIP);
		pRip->nIPEnd = pRip->nIPBegin;
	}

	// set the duration of time that we reject this guy
	pRip->nRefuseUntil = nRefuse + nNow;
	pRip->strReason = pzReason;
	pRip->nRefusedCount = 0;

	// and to the list if a new structure was created
	if (bMadeNew)
	{
		if (!g_FirstRefusedIP)
		{
			g_FirstRefusedIP = pRip; // first one in the list
		}
		else
		{
			g_LastRefusedIP->pNext = pRip;
		}
		g_LastRefusedIP = pRip;
	}
}


int AddTime( IPLog** Root, const char *pzIP )
{
	int nNow = time(0);
	int nConnectLimit = GetProfile().GetInt("System","FrequencyConnectLimit",false);
	if (nConnectLimit < 1)
		return 0;
	nConnectLimit = (nConnectLimit > 32) ? 32 : nConnectLimit;
	
	// cleanup and reset the counters if it's time.
	if (g_FrequencyTimeRelease && nNow - g_FrequencyTimeRelease > GetProfile().GetInt("System","FrequencyTimeRelease",false) )
	{
		TimedRelease( g_LogRoot );
		g_LogRoot = 0;
		g_FrequencyTimeRelease = nNow;
	}


	if( *Root == 0 )    
	{

		*Root = new IPLog(pzIP);
		
		// if the global root (g_LogRoot) was just assigned - start the cleanup clock
		if (g_FrequencyTimeRelease == 0)
			g_FrequencyTimeRelease = nNow;

		return 0;
	}

	int nTreePlacementTest = strcmp((*Root)->pzIP,pzIP);
	
	
	if( nTreePlacementTest == 0 ) // if this IP has been here recently
	{
		
		// if less than N seconds have passed since this guy filled his time array.
		int N = GetProfile().GetInt("System","FrequencyTimeLimit",false);

		if (nNow - (*Root)->nTime[0] < N &&  (*Root)->nTime[nConnectLimit - 1] > 0)
		{

			//	caught a resource hog
			GString strErr;
			strErr << pzIP << ": violated connect limit of " << nConnectLimit << " over " << N << " seconds.";
			InfoLog(25,strErr);


			// if we are configured to 'shut this guy off' for a little while
			// set the time that we no longer accept connections from this 
			// resource hog - otherwise just disconnect this attempt and allow him 
			// to retry immediately.
			int nRefuseSeconds = GetProfile().GetInt("System","FrequencyViolatorRefuse",false);
			if (nRefuseSeconds)
			{
				AddRefuseService(pzIP,nRefuseSeconds,strErr);
			}

			return 1; // connections are coming too fast.
		}
		
		if ((*Root)->nTime[nConnectLimit - 1] > 0) // if the time array was full
		{
			for(int ii=0; ii<nConnectLimit; ii++ )
				(*Root)->nTime[ii] = (*Root)->nTime[ii+1]; // slide the time, bump off the oldest.
			(*Root)->nTime[nConnectLimit - 1] = nNow;
		}
		else // add this time to the end of the array.
		{
			for (int i = 1; i < nConnectLimit; i++)
			{
				if ((*Root)->nTime[i] == 0)
				{
					(*Root)->nTime[i] = nNow;
					break;
				}
			}
		}
		return 0;
	}

	if( nTreePlacementTest > 0 )
		return AddTime( &((*Root)->Left), pzIP );
	else 
		return AddTime( &((*Root)->Right), pzIP );
	return 0;
}





// For a fast implementation of connection restrictions:
// Incoming connections are hashed(based on IP address) to index 
// an integer in this array, that is incremented when a connection
// is being processed, and decremented when that thread completes.
//
// Collisions will case 2 ip's to 'double the cumulative active connection total'
// A large hash causes the collision factor to be almost non existent
// considering proxies/routers will cause the same (cumulative active connection total) 
// to all ip's on the subnet behind the proxy/router.  So connections are restricted
// to a limited number per subnet, for security that's even better than per IP.
//
//
// the hash is divided into two equal regions (increment & decrement). So :
//			hash[i] - hash[ACTIVE_CONN_HASH_SIZE / 2) + i]
//	equals the real-time current connection count for the ip(s) that hash to i.
//                              AND
//  hash[i] is the total number of connections from the IP that have been
//  counted since 'this' server has started, isn't that some interesting
//  stuff to have in memory?
//
#define ACTIVE_CONN_HASH_SIZE 200000
#define HASHLOCK_ARR_SIZE 50
static long HashOfActiveConnections[ACTIVE_CONN_HASH_SIZE];
static gthread_mutex_t ArrayOfLocks[HASHLOCK_ARR_SIZE];

struct ConnHashInit
{
	ConnHashInit()
	{ 
		memset(HashOfActiveConnections,0,ACTIVE_CONN_HASH_SIZE );
		for(int i=0;i<HASHLOCK_ARR_SIZE;i++)
		{
			gthread_mutex_init(&ArrayOfLocks[i],0);
		}
	}
	~ConnHashInit()
	{ 
		for(int i=0;i<HASHLOCK_ARR_SIZE;i++)
		{
			gthread_mutex_destroy(&ArrayOfLocks[i]);
		}
	}
} g_ConnHashInit ;





// nChange of 1 increments, -1 decrements
static unsigned int ConnectionCounter ( const char *pzAddress, int nChange )
{
	const unsigned char *ip = (const unsigned char *)pzAddress;
	unsigned long   h = 0, g;
    while ( *ip )
    {
        h = ( h << 4 ) + *ip++;
        if ( (g = h & 0xF0000000) )
            h ^= g >> 24;
        h &= ~g;
    }
    int idx = h % ( ACTIVE_CONN_HASH_SIZE / 2);

	if (nChange == 1)											
	{
#ifdef _WIN32						
		InterlockedIncrement(&HashOfActiveConnections[idx]);	// this is lockless
		InterlockedIncrement(&g_ActiveConnections);
		InterlockedIncrement(&g_TotalConnections);
#else
		int nLockIdx = idx % HASHLOCK_ARR_SIZE;
		gthread_mutex_lock(&ArrayOfLocks[nLockIdx]);			// and this blocks 50 times less than a single global lock
		HashOfActiveConnections[idx]++;
		gthread_mutex_unlock(&ArrayOfLocks[nLockIdx]);
		g_ActiveConnections++;
		g_TotalConnections++;
#endif


	}
	else
	{
#ifdef _WIN32
		InterlockedIncrement(&HashOfActiveConnections[(ACTIVE_CONN_HASH_SIZE / 2) + idx]);
		InterlockedDecrement(&g_ActiveConnections);
#else
		int nLockIdx = idx % HASHLOCK_ARR_SIZE;
		gthread_mutex_lock(&ArrayOfLocks[nLockIdx]);			
		HashOfActiveConnections[(ACTIVE_CONN_HASH_SIZE / 2) + idx]++;
		gthread_mutex_unlock(&ArrayOfLocks[nLockIdx]);
		g_ActiveConnections--;
#endif
	}
	
	return HashOfActiveConnections[idx] - 
			HashOfActiveConnections[(ACTIVE_CONN_HASH_SIZE / 2) + idx];
}


void BuildServicePermissionList(int nType)
{
	IPQueryEntry **pFirst = 0;
	IPQueryEntry **pLast = 0;
	GStringList lst;

	GString strReason;
	if (nType == 0)
	{
		pFirst = &g_FirstRefusedIP;
		pLast = &g_LastRefusedIP;
		lst.DeSerialize(",", GetProfile().GetString("System","RefuseService",false));
		strReason = "[System] RefuseService";
	}
	else if (nType == 1)
	{
		pFirst = &g_FirstAllowedIP;
		pLast = &g_LastAllowedIP;
		lst.DeSerialize(",", GetProfile().GetString("System","GrantService",false));
		strReason = "[System] GrantService";
	}


	if (*pFirst) // the first time the list is built this will be null
	{
		// todo: delete the previous settings if we are rebuilding the list
		*pFirst = 0;
	}


	GStringIterator itRefuse(&lst);
	while (itRefuse())
	{
		IPQueryEntry *pCurrent = 0;
		const char *pzIp = itRefuse++;
		char *pDash = (char *)strpbrk(pzIp, "-");
		if (pDash)
		{
			// a range between two IP's
			*pDash = 0; 
			pCurrent = new IPQueryEntry(IpToValue(pzIp),IpToValue(pDash+1));
			*pDash = '-';
		}
		else
		{
			// a single IP
			pCurrent = new IPQueryEntry(IpToValue(pzIp));
		}
		pCurrent->strReason = strReason;
		if (!(*pFirst) )
		{
			*pFirst = pCurrent; // first one in the list
			*pLast = pCurrent;
		}
		else
		{
			(*pLast)->pNext = pCurrent;
		}
		*pLast = pCurrent;
	}
}



// QueryServicePermission() returns 1 if the ip is in the specified list
// nType  0 == "global refuse service"
// nType  1 == "global allow service"
// todo: add  more specific IP based service permissions (per protocol, and per switchboard operation)
int QueryServicePermission(unsigned long ip, int nType)
{
	// load the denied IP's from the config file on the first lookup.
	IPQueryEntry *pFirst = 0;
	if (nType == 0)
	{
		pFirst = g_FirstRefusedIP;
	}
	else if (nType == 1)
	{
		pFirst = g_FirstAllowedIP;
	}

	IPQueryEntry *pCurrRefusedIP = pFirst;
	if (pCurrRefusedIP)
	{
	#ifdef _WIN32
		ip = htonl(ip);
	#endif
		unsigned long now = time(0);
		while (pCurrRefusedIP)
		{
			// integer ip comparison
			if (ip >= pCurrRefusedIP->nIPBegin && ip <= pCurrRefusedIP->nIPEnd)
			{
				if (pCurrRefusedIP->nRefuseUntil == 0)
				{
					// this ip is indefinitely in the list.
					pCurrRefusedIP->nRefusedCount++;
					return 1; 
				}

				if ( pCurrRefusedIP->nRefuseUntil > now )
				{
					// this ip is in the list for [pCurrRefusedIP->nRefuseUntil - now] more seconds.
					pCurrRefusedIP->nRefusedCount++;
					return 1; 
				}
			}
			pCurrRefusedIP = pCurrRefusedIP->pNext;
		}
	}
	return 0; // this ip is not in the list
}


void GetBalanceTo(const char *pzBalance, GString &strDest)
{
	if (pzBalance && pzBalance[0])
	{
		int nDistributionKey = rand() % 100; // creates a random #: 0-99
		GStringList lst(",",pzBalance);

		GStringIterator itParams(&lst);
		int nDistributionChannel = 0;
		while (itParams())
		{
			nDistributionChannel += atoi(itParams++); // should add up to 100
			if (!itParams())
				break; // bad config file format
			const char *pzServer = itParams++;
			if (nDistributionChannel > nDistributionKey)
			{
				strDest = pzServer;
				break;
			}
		}
	}
}


// feel free to put anything you need in this table
const char *mimeTypeTable[] = {
   "mpeg",  "video/x-mpeg", //(*.mpeg,*.mpg,*.m1s,*.m1v,*.m1a,*.m75,*.m15,*.mp2,*.mpm,*.mpv,*.mpa)
   "mpg",  "video/x-mpeg",
   "wav",  "audio/x-wav",
   "mp3",  "audio/mpeg",
   "mid",  "audio/midi",
   "doc",  "application/msword",
   "rtf",  "application/rtf",
   "swf",  "application/x-shockwave-flash",
   "tar",  "application/x-tar",
   "gtar",  "application/x-gtar",
   "xls",  "application/excel",
   "ico",  "image/x-icon",
   "dwf",  "drawing/x-dwf",
   "avi",  "video/x-msvideo",
   "pkm",  "application/octet-stream",
   "xml",  "text/xml",
   "css",  "text/css",
   "exe",  "application/octet-stream",
   "gif",  "image/gif",
   "jpeg", "image/jpeg",
   "jpg",  "image/jpeg",
   "htm",  "text/html",
   "html", "text/html",
   "txt",  "text/plain",
   "png",  "image/png",
   "pdf",  "application/pdf",
   "js",   "application/x-javascript",
   "zip",  "application/zip",
   "gz",   "application/x-gzip",
   "jar",  "application/java-archive",
   "apk",  "application/vnd.android.package-archive",
   0
};


const char *GetProtocolName(int nProtocol)
{
	static GString strStorage;
	switch(nProtocol)
	{
		case 1: strStorage = "HTTP"; break;
		case 2: strStorage = "Proxy-Tunnel"; break;
		case 3: strStorage = "Xfer"; break;
		case 4: strStorage = "TXML"; break;
		case 5: strStorage = "TXML+HTTP"; break;
		case 6: strStorage = "Remote-WS"; break;
		case 7: strStorage = "Reserved"; break;
		case 8: strStorage = "Messaging"; break;
		case 9: strStorage = "PrivateSurf"; break;
		case 10: strStorage = "PortBlock"; break;
		case 11: strStorage = "HTTPProxy"; break;
		case 12: strStorage = "OpenSurf"; break;
		case 13: strStorage = "RemoteConsole"; break;
		default : strStorage = "Unknown"; break;
	}
	return strStorage;
}



// This (structure/class/object) contains information "about a listener"
// That is everything needed to listen on a port and apply a protocol.
// This information is passed into the thread that "does the work" but
// no thread state should ever be written into this structure because
// multiple threads servicing this port will all point to the same 
// ThreadStartupData (structure/class/object) it should be treated
// as "read-only" to the servicing thread, they are public only for
// unwanted Get/Set accessors.
class ThreadStartupData
{
	ThreadStartupData *pNext;
public:
	int nState; // -2 bind error, -1 permission error, 0 stopped, 1 ready, 2 waiting, 3 servicing
	int nProtocol; // 0 Root, 1 HTTP, 2 Proxy/Tunnel, 3 Xfer, 4 TXML, 5 TXML&HTTP, 6 Remote-WS, 7 Reserved, 8 Messaging, 9 PrivateSurf, 10 PortBlock, 11 HTTPProxy, 12 OpenSurf
	int nInstanceLimit; // 0 is no limit, 1 services a single transaction at a time.
	int nPort;
	int sockfd;	// the handle to the port listener
	int bThreadShuttingDown;

	// stats for this port reported by listening thread
	int nTooFrequent;
	int nTooMany;
	int nRefused;
	int nCount;

	char szConfigSectionName[64];
	
	// if nProtocol == 2 this will also be set
	int nProxyToPort;
	char szProxyToServer[1024];
	char szHTTPProxyFirewall[1024];
	int nProxyTimeout;
	int bIsTunnel;

	// ThreadStartupData describes the static aspects of a type of connection,
	// If the connection was initiated through the (~route) mechanism, the nPort
	// will be set to -777 and pzPostConnectPathAndName will contain a value like:
	// /MySwitchPath/MyVNCSwitchName.html
	char pzPostConnectPathAndName[2048];

	CipherData *pcd;

	ThreadStartupData * Next(int bCreateIfNone = 0)
	{
		if (!pNext && bCreateIfNone)
			pNext = new ThreadStartupData;
		return pNext;
	}
	ThreadStartupData()
	{
		nTooFrequent = 0;
		nTooMany = 0;
		nRefused = 0;
		nCount = 0;
		nPort = -1;

		nInstanceLimit = 0;
		bThreadShuttingDown = 0;
		bIsTunnel = 0;
		nProtocol = 0;
		pNext = 0;
		nProxyToPort = 0;
		szProxyToServer[0] = 0;
		szHTTPProxyFirewall[0] = 0;
		szConfigSectionName[0] = 0;
		pcd = 0;
		nState = 0;
	}
};
ThreadStartupData g_TSD;



class ThreadConnectionData
{
public:
	int bHasStartedThread;


	int *pnProxyClosed;
	ThreadStartupData *pTSD;
	int sockfd;	// the handle for 'this' accept()'ed connection on pTSD->nPort
	int *pnProxyfd;	
	unsigned long *plLastActivityTime;
	int *pbHelperThreadIsRunning;
	int nParentThreadID;
	gthread_cond_t	*pcondHelper;


	char pzClientIP[64]; // machine IP or name that started this transaction

	// when this structure is used to poll for a connection
	// this variable contains the section name like "PollExternalConnect1" ... "PollExternalConnectN"
	// or "MsgFrom-User" when used for messaging
	char pzPollSectionName[64];

	// this is set when for any Xfer protocol proxy
	char pzIP[1024];	// could be a host name, make room for a big one this is a system wide limitation for a host name length

	char pzConnectRoute[1024];
	char pzRemoteKey[1024];
	int nConnectRouteSize;
	int nAction; // 1=Proxy, 2=Tunnel, 3=Destination, 4=ConvertHTTP

	// if using HTTP authentication to make a request outside the local network
	GString strLocalNetworkAuthenticate; // "user:password"


	int nPollThreadID;
	int nPollInterval;
	int nPollQueueMax;
	int nPollSrvWait;
	int bInUse;
	int nLastPollConnectStatus;
	time_t lPollResume;
	time_t lOnErrorBeginTime;
	ThreadConnectionData *pNext;
	ThreadConnectionData *pBaseTCD;// if a poll queue is 3, all 3 point to the same base for the same switchboard
	time_t lLastActivity;

	void Copy(ThreadConnectionData *pFrom)
	{
		bInUse = 0;
		bHasStartedThread = 0;
		pNext = 0;
		nPollThreadID = 0;

		pnProxyClosed = pFrom->pnProxyClosed;
		pTSD = pFrom->pTSD;
		sockfd = pFrom->sockfd;
		pnProxyfd = pFrom->pnProxyfd;
		plLastActivityTime = pFrom->plLastActivityTime;
		pbHelperThreadIsRunning = pFrom->pbHelperThreadIsRunning;
		nParentThreadID = pFrom->nParentThreadID;
		pcondHelper = pFrom->pcondHelper;

		strcpy(pzClientIP,pFrom->pzClientIP);
		strcpy(pzPollSectionName,pFrom->pzPollSectionName);
		strcpy(pzIP,pFrom->pzIP);
		strcpy(pzConnectRoute,pFrom->pzConnectRoute);
		strcpy(pzRemoteKey,pFrom->pzRemoteKey);

		nConnectRouteSize = pFrom->nConnectRouteSize;
		nAction = pFrom->nAction;
		strLocalNetworkAuthenticate = pFrom->strLocalNetworkAuthenticate;
		nPollInterval = pFrom->nPollInterval;
		nPollQueueMax = pFrom->nPollQueueMax;
		nPollSrvWait = pFrom->nPollSrvWait;
		nLastPollConnectStatus = pFrom->nLastPollConnectStatus;
		lPollResume = pFrom->lPollResume;
		lOnErrorBeginTime = pFrom->lOnErrorBeginTime;
	}

	void Init()
	{
		nAction = 0; // none
		pzIP[0] = 0;
		pzPollSectionName[0] = 0;
		pzConnectRoute[0] = 0;
		nConnectRouteSize = 0;
		pzRemoteKey[0] = 0;
		pzClientIP[0] = 0;
		
		nPollInterval = 15;
		nPollQueueMax = 1;
		nPollSrvWait = 300;
		
		lPollResume = 0;

		nParentThreadID = 0;
		pcondHelper = 0;
		plLastActivityTime = 0;
		pbHelperThreadIsRunning = 0;
		pnProxyClosed = 0;
		sockfd = 0;
		bInUse = 0;
		nLastPollConnectStatus = 0;
		pNext = 0;

		bHasStartedThread = 0;
		nPollThreadID = 0;
		pBaseTCD = 0;
		lLastActivity = 0;
	}
	
	ThreadConnectionData()
	{
		Init();
	}
};


// ThreadData's do not come and go, they are created 1 per thread and pooled with the thread.
class ThreadData
{
public:
	char *Buf1;
	char *Buf2;


	ThreadStartupData *pTSD;
	int *pnProxyClosed;

	gthread_t chld_thr;

	gthread_mutex_t lock;
	gthread_cond_t	cond;

	gthread_mutex_t lockFinished;
	gthread_cond_t	condFinished;

	int	sockfd;
	int nThreadIsBusy;
	int nThreadID;
	int m_bUseThreads;
	int nSequence; // nCmdSeq
	long starttime;

	int *m_pnKeepAlive;

	int nParentThreadID;

	unsigned long *plLastActivityTime;
	int *pbHelperThreadIsRunning;

	gthread_mutex_t lockLog;
	gthread_cond_t	condLog;

	gthread_mutex_t lockHelper;
	gthread_cond_t	condHelper;
	// only set if this is a ProxyHelperThread 'attached' to the ThreadData of it's 
	// clientthread.  The following variables will actually point to the variables 
	// preceding this comment in the ThreadData of the clientthread that is proxying.
	gthread_cond_t	*pcondHelper;
	int	*pnProxyfd;
	char pzClientIP[32];
	
	// for debugging, a text description of the action of this thread.
	GString strAction;



	char pzPollSectionName[64];



	char pzIP[1024];
	char pzConnectRoute[1024];
	int nAction; // 1=Proxy, 2=Tunnel, 3=Destination, 4=ConvertHTTP



	ThreadData( bool bUseThreads = 1 )
	{

		Buf1 = 0;
		Buf2 = 0;
		
		nSequence = 0;
		m_pnKeepAlive = 0;
		nAction = 0; // none
		pzIP[0] = 0;
		pzPollSectionName[0] = 0;
		pzConnectRoute[0] = 0;

		nParentThreadID = 0;
		pcondHelper = 0;
		pnProxyClosed = 0;
		plLastActivityTime = 0;
		pbHelperThreadIsRunning = 0;
		pTSD = 0;

		m_bUseThreads = bUseThreads;
		if (bUseThreads)
			nThreadID = ++g_ThreadIDCount;
		else
			nThreadID = 1;
		sockfd = 0;
		nThreadIsBusy = 0;
		if (m_bUseThreads)
		{
			gthread_mutex_init(&lock,0);
			gthread_cond_init(&cond,0);
			gthread_mutex_init(&lockHelper,0);
			gthread_cond_init(&condHelper,0);
			gthread_mutex_init(&lockLog,0);
			gthread_cond_init(&condLog,0);

			gthread_mutex_init(&lockFinished,0);
			gthread_cond_init(&condFinished,0);

			// lock the mutex so the thread awaits until woken.
			gthread_mutex_lock(&lockHelper);
			gthread_mutex_lock(&lockFinished);
			gthread_mutex_lock(&lock);
		}
	}
	~ThreadData()
	{
		if (m_bUseThreads)
		{
			gthread_mutex_destroy(&lock);
			gthread_cond_destroy(&cond);
			gthread_mutex_destroy(&lockHelper);
			gthread_cond_destroy(&condHelper);
			gthread_mutex_destroy(&lockLog);
			gthread_cond_destroy(&condLog);

			gthread_mutex_destroy(&lockFinished);
			gthread_cond_destroy(&condFinished);
		}
	}
};
int g_nProxyPoolRoundRobin = 0;
int g_nThreadPoolRoundRobin = 0;


void PingPool(GArray *pPool, int nPoolSize)
{
	g_ThreadPing = 1;
	if( nPoolSize > 1 )
	{
		for(int i=0; i < nPoolSize; i++)
		{
			ThreadData *td = (ThreadData *)(pPool->m_arrObjects[i].m_pValue);
			if (!td->nThreadIsBusy)
			{
				if (gthread_mutex_trylock( &td->lock ) == EBUSY)
				{
					continue;
				}
				gthread_mutex_unlock( &td->lock );

				// wake it up it'll call InfoLog() then go back to sleep.
				gthread_cond_signal(  &(td->cond) ); 
			}
		}
	}
	
	// give 'em plenty of time to finish before we turn off the g_ThreadPing flag.
	PortableSleep(1,0);

	g_ThreadPing = 0;
}



void DestroyThreadPool(GArray *pPool, int nPoolSize)
{
	g_ServerIsShuttingDown = 1;
	if( nPoolSize > 1 )
	{
		GList lstHungOrBusy;
		int i;
		for(i=0; i < nPoolSize; i++)
		{
			ThreadData *td = (ThreadData *)(pPool->m_arrObjects[i].m_pValue);
			if (!td->nThreadIsBusy)
			{
				if (gthread_mutex_trylock( &td->lock ) == EBUSY)
				{
					lstHungOrBusy.AddLast(td);
					continue;
				}
				gthread_mutex_unlock( &td->lock );
				// wake it up to die a natural death
				gthread_cond_signal(  &td->cond ); 
			}
			else
			{
				lstHungOrBusy.AddLast(td);
			}
		}
		// give the threads plenty of time to end normally.
		for(i=0 ;i < nPoolSize * 2; i++)
			gsched_yield();

		if (lstHungOrBusy.Size())
		{
			// we can kill them here, if needed
			InfoLog(1, "Threads still working.");
		}
	}
}


int KillTid (int ntidToKill, int bOnlyEndIt = 0)
{
try
{
	if( g_ThreadPoolSize > 1 )
	{
		// primary pool
		for(int i=0; i < g_ThreadPoolSize; i++)
		{
			ThreadData *td = (ThreadData *)(g_PoolClientThread->m_arrObjects[i].m_pValue);

			if(td->nThreadID == ntidToKill)
			{
				if (bOnlyEndIt)
				{
					PortableClose(td->sockfd,"Core3");
					*td->pnProxyClosed = 1; // 1 == Shut down in progress.
				}
				else
				{
					int nRet = gthread_cancel (td->chld_thr);
					if (nRet == 0) // if it worked
					{
						if (td->Buf1)
							delete td->Buf1; // search this file for KillTidMemoryNote
						gthread_create(&td->chld_thr,NULL, clientThread, (void *)td );
						td->nThreadIsBusy = 0;
					}
				}
				return 0;
			}
		}
	}
	if( g_ProxyPoolSize > 1 )
	{
		// proxy pool
		for(int i=0; i < g_ProxyPoolSize; i++)
		{
			ThreadData *td = (ThreadData *)(g_PoolProxyHelperThread->m_arrObjects[i].m_pValue);

			if(td->nThreadID == ntidToKill)
			{
				int nRet = gthread_cancel (td->chld_thr);
				if (nRet == 0) // if it worked
				{
					if (td->Buf1)
						delete td->Buf1; // search this file for KillTidMemoryNote
					gthread_create(&td->chld_thr,NULL, ProxyHelperThread, (void *)td );
					td->nThreadIsBusy = 0;
				}
				return 0;
			}
		}

		// invalid [ntidToKill]
		return 1;
	}
}
catch(...)
{
	return 3; // OS error
}

	// system not using threads
	return 2;
}


void CreateThreadPool(GArray **pGPool, int nPoolSize, void *(*pfnThreadProc) (void *))
{
	if( nPoolSize > 1 )
	{
		*pGPool = new GArray(nPoolSize * 2); // the pool starts at the specified size and currently cannot ever exceed twice that size

		for(int i=0; i < nPoolSize; i++)
		{
			ThreadData *td = new ThreadData;

			(*pGPool)->AddElement(td);
			// the mutex is locked so pfnThreadProc hangs shortly after creation
			gthread_create(&td->chld_thr,NULL, pfnThreadProc, (void *)td );
			gsched_yield();
		}
	}
}



void AttachToClientThreadHelper(ThreadData *pTD, ThreadConnectionData *pTCD)
{
	ThreadStartupData *pTSD = pTCD->pTSD;

	pTD->sockfd = pTCD->sockfd;
	pTD->pTSD = pTSD;
	pTD->pcondHelper = pTCD->pcondHelper;
	pTD->pnProxyClosed = pTCD->pnProxyClosed;
	pTD->nParentThreadID = pTCD->nParentThreadID;
	pTD->pnProxyfd = pTCD->pnProxyfd;
	pTD->plLastActivityTime = pTCD->plLastActivityTime;
	pTD->pbHelperThreadIsRunning = pTCD->pbHelperThreadIsRunning;
	strcpy(pTD->pzClientIP,pTCD->pzClientIP);


	strcpy(pTD->pzIP,pTCD->pzIP); // for protocol 6 && 3 && 8

	strcpy(pTD->pzPollSectionName,pTCD->pzPollSectionName );

	if (pTCD->nConnectRouteSize) // protocols 6,11, and 2 use this
	{
		memcpy(pTD->pzConnectRoute,pTCD->pzConnectRoute,pTCD->nConnectRouteSize);
	}
	gthread_cond_signal(  &(pTD->cond) ); 
}

ThreadData *AttachToClientThread(ThreadConnectionData *pTCD, GArray *pGPool,int nPoolSize, int *pnPoolIssuedCount)
{
	// try round robin approach first, see if the thread that was woken up the longest time ago has completed it's 
	// task, and is sleeping again.  Depending on load and pool size 99+% of connections attach on Round Robin.
	// If we're handing out something like a telnet connection that may last for days - it's not so likely to 
	// hit an idle thread in the pool on the the first try - but then speed is a little less an issue anyhow.
	//
	// It's faster than using a stack(list) of available threads since no synchronizations objects need to be 
	// used to set the index or check the thread at that index doing it this way.  Even on RISC or multi CPU
	// machines this is safe because we always index at the modulus of the result.
	int n = ++(*pnPoolIssuedCount) % nPoolSize;
	ThreadData *pTD = (ThreadData *)(pGPool->m_arrObjects[n].m_pValue);


	// This Flag + Lock check guarantees proper pool checkin/checkout - either one alone is insufficient.
	// only 1 in a million (or less) will ever call gthread_mutex_trylock() and not obtain the lock since that would
	// mean that a thread was preempted exactly between the assignment of nThreadIsBusy and the gthread_cond_wait()
	// gthread_cond_wait() is the very next line of code after the assignment of nThreadIsBusy in the workerthread.
	// It does happen - very rarely.
	if (!pTD->nThreadIsBusy  &&  gthread_mutex_trylock( &pTD->lock ) != EBUSY)
	{									
		pTD->nThreadIsBusy = 1;		
		gthread_mutex_unlock( &pTD->lock );
		AttachToClientThreadHelper(pTD, pTCD);
		return pTD;
	}
	
	else // that thread is still busy, find another one.
	{
		// walk through the thread pool until we find an available thread.
		// it's not a list, it's an array so walking it is very fast.
		for(int i = 0;;i++)	
		{
			pTD = (ThreadData *)(pGPool->m_arrObjects[i].m_pValue);

			if (!pTD->nThreadIsBusy  &&  gthread_mutex_trylock( &pTD->lock ) != EBUSY)
			{
				pTD->nThreadIsBusy = 1;
				gthread_mutex_unlock( &pTD->lock );
				AttachToClientThreadHelper(pTD, pTCD);
				return pTD;
			}
			if (i == (nPoolSize - 1) ) // if we made it all the way through the pool and they are all busy
			{
				// todo: check system memory to see if auto grow is feasible

				// lock only the grow procedure, 
				// the pool is NOT locked, - other threads may still be using it during it's expansion.
				// however, no other threads may be expanding it while this thread is expanding it.
				gthread_mutex_lock(&PoolExpansionLock); 

				int bCreatedThread = 0;
				ThreadData *pNewTD = 0;
				// is is very important that the array is not reallocated here, this check ensures it never happens.
				// it also ensures that the pool does not grow forever, since it cannot exceed the pre-allocated grow space.
				if (pGPool->GetItemCount() < pGPool->GetSize())
				{
					pNewTD = new ThreadData;
					pNewTD->nThreadIsBusy = 1;

					if(pGPool == g_PoolClientThread) // there are two thread pools, decide which one we are dealing with
					{
						if ( gthread_create(&pNewTD->chld_thr,NULL, clientThread, (void *)pNewTD ) == 0)
						{
							bCreatedThread = 1;
						}
					}
					else
					{
						if (gthread_create(&pNewTD->chld_thr,NULL, ProxyHelperThread, (void *)pNewTD ) == 0)
						{
							bCreatedThread = 1;
						}
					}
					if(bCreatedThread)
					{
						while(pNewTD->nThreadIsBusy) // when it's not busy, it is ready, initialized and waiting
						{
							gsched_yield(); // it won't take more than a loop or two
						}
						// now, before we increment the pool size (making it available for other connections to use)
						// re-flag it as in use, so that nobody else can pick it up before the guy we created it for.
						pNewTD->nThreadIsBusy = 1; 

						pGPool->AddElement(pNewTD);	// now it's in the pool, but no other thread could know it's there because it's past the bounds of the known pool size
						
						if(pGPool == g_PoolClientThread) // there are two thread pools, decide which one we are dealing with
							g_ThreadPoolSize++; // now it's in the pool for everyone to see.
						else
							g_ProxyPoolSize++;  // now it's in the pool for everyone to see.
					}
					else
					{
						// we failed to create the thread - but we should have never tried if resources were low
						// so this could only happen if the thread pool was grossly mis-configured for this hardware.
						delete pNewTD; 
					}
				}

				gthread_mutex_unlock(&PoolExpansionLock);

				if (bCreatedThread)
				{
					AttachToClientThreadHelper(pNewTD, pTCD);
					return pNewTD;
				}
				else
				{

					//10=-- Server Too Busy
					GString strLog;
					strLog.LoadResource(g_pzCoreErrorSection, 10);
					InfoLog(28,strLog);

					// bad news.  The requests are coming faster than we can deal with them, and we are not configured to, or cannot, create another thread to handle this request.
					// If this happens, you need a bigger thread pool, more memory,  or a faster server. - depending on your configuration settings
					// It's also possible that we have too many idle connections, try reducing timeout values
					g_TotalTooBusy++;

					// reject the pending connection.
					PortableClose(pTCD->sockfd,"Core4");
					return 0;
				}
			}
		}
	}
	return 0;
}



gthread_mutex_t g_csMtx2;
int g_csMtxInit2 = 0;
GString g_strHTTPHitLog;

void MutexLog(const char *strFileNoPath,const char *pzRequestingIP,const char *strReferer, const char *pzExt, const char *pzCommentNoCommas)
{
	if (g_strHTTPHitLog.IsEmpty())
	{
		return;
	}

	// If we have been explicitly told not to log requests for this file type - leave.
	// note: "static" means that the data is initialized only once, not every time this execution come here.
	static const char *pzNonLogTypes = GetProfile().GetString("HTTP","NonLoggingTypes",false);
	if (pzNonLogTypes)
	{
		static GStringList row(",",pzNonLogTypes);
		GStringIterator itParams(&row);
		while (itParams())
		{
			GString strNonLogType(itParams++);
			if (strNonLogType.CompareNoCase(pzExt) == 0 )
				return; // don't log this one
		}
	}


	// If we have been explicitly told not to log requests for this file - leave.
	static const char *pzNonLogFiles = GetProfile().GetString("HTTP","NonLoggingFiles",false);
	if (pzNonLogTypes)
	{
		static GStringList row2(",",pzNonLogFiles);
		GStringIterator itParams(&row2);
		while (itParams())
		{
			GString strNonLogType(itParams++);
			if (strNonLogType.CompareNoCase(strFileNoPath) == 0 )
				return; // don't log this one
		}
	}


	time_t lTime;
	time(&lTime);
	
	GString strComment(pzCommentNoCommas);
	GString strLog;
	strLog << lTime << "," << strComment << "," << strFileNoPath << "," << pzRequestingIP << ","  << strReferer << "\n";

	if (!g_csMtxInit2)
	{
		g_csMtxInit2 = 1;
		gthread_mutex_init(&g_csMtx2, NULL);
	}

	gthread_mutex_lock(&g_csMtx2);
	strLog.ToFileAppend(g_strHTTPHitLog,0);
	gthread_mutex_unlock(&g_csMtx2);
}


// When posting data through a GET, from a browser, the 
// URL Data has certain characters hex encoded (a space becomes %20 for example)
// RFC 2396  = "-" | "_" | "." | "!" | "~" | "%" |  "*" | "'" | "(" | ")"
// You may supply the same memory address from [pSource] and [pDest] to this function
int unEscapeData(const char *pSource,const char *pDest)
{
	// cast off the const and store the values
	char *pszQuery = (char *)pSource;
	char *t = (char *)pDest;
	
	// walk the pSource until we find a NULL or a space
	while (*pszQuery && *pszQuery != 32)
	{
		switch (*pszQuery)
		{
		case '+' :
			*t = ' ';
			break;
		case '%' :
			{
				pszQuery++;
				char chTemp = pszQuery[2];
				pszQuery[2] = 0;
				char *pStart = pszQuery;
				char *pEnd;
				*t = (char)strtol(pStart, &pEnd, 16);
				pszQuery[2] = chTemp;
				pszQuery = pEnd;
				t++;
				continue;
			}
			break;
		default :
			*t = *pszQuery;
			break;
		}
		pszQuery++;
		t++;
	}
	*t = 0;
	return t - pDest;
}

int DoesCacheHTTPContent(GString &strExt, long lBytes)
{
	if ( GetProfile().GetBoolOrDefault("HTTP","ContentCache",0) )
	{
		int nMax = GetProfile().GetInt("HTTP","MaxCacheFileSize",false);
		if (nMax > 1)
		{
			if (lBytes > nMax)
			{
				return 0; // too big to cache
			}
		}
		const char *pzNonCacheTypes = GetProfile().GetString("HTTP","NonCacheTypes",false);
		if (pzNonCacheTypes)
		{
			GStringList types(",",pzNonCacheTypes);
			GStringIterator itTypes(&types);
			while (itTypes())
			{
				// if this is a file type that does not get cached
				if ( strExt.CompareNoCase( itTypes++ ) == 0 )
				{
					return 0; // don't cache it
				}
			}
		}

		const char *pzOnlyCacheTypes = GetProfile().GetString("HTTP","OnlyCacheTypes",false);
		if (pzOnlyCacheTypes && pzOnlyCacheTypes[0] )
		{
			GStringList types(",",pzOnlyCacheTypes);
			GStringIterator itTypes(&types);
			while (itTypes())
			{
				// if this is a file type that does not get cached
				if ( strExt.CompareNoCase( itTypes++ ) == 0 )
				{
					return 1; //cache it
				}
			}
			return 0; //don't cache it, it's not one of the types we cache
		}
	}
	else
	{
		return 0; // caching turned off
	}
	return 1;
}


// for sending back 404's or 301's , any non-file response.
int SendHTTPNoFileResponse(int fd, GString &strError, GString &strHTML)
{

	// HTTP header server name
	const char *pWebSrvType=GetProfile().GetString("HTTP","HTTPHeaderServerName",false);
	strError += "Server: ";
	if (pWebSrvType && pWebSrvType[0])
		strError += pWebSrvType;
	else
		strError += "5Loaves";
	strError += "\r\n";

	// add the current time
	char pzTime[128];
	struct tm *newtime;
	time_t ltime;
	time(&ltime);
	newtime = gmtime(&ltime);
	strftime(pzTime, 128, "Date: %a, %d %b %Y %H:%M:%S GMT\r\n", newtime);
	strError += pzTime;

	strError += "Content-Type: text/html\r\n";

	strError += "Content-Length: ";
	strError += strHTML.Length();
	strError += "\r\n\r\n";
	strError += strHTML;
	
	return nonblocksend(fd,strError, (int)strError.Length());
}

void BuildHTTPHeader(char *header, const char *pzFileName, const char *pzConnectionType, int nIsKeepAlive, int nKeepAliveReturnValue, const char *pzETag, const char *pzExt, long lBytes, int nBeginRange, int nEndRange)
{
	memset(header,0,512);
	if (nBeginRange > -1)
		strcat(header,"HTTP/1.1 206 Partial Content\r\n");
	else
		strcat(header,"HTTP/1.1 200 OK\r\n");

	// HTTP header: "Date: Sunday Sun, 21 Oct 1995 19:38:46 GMT\n"
	char pzTime[128];
	struct tm *newtime;
	time_t ltime;
	time(&ltime);
	newtime = gmtime(&ltime);
	strftime(pzTime, 128, "Date: %a, %d %b %Y %H:%M:%S GMT\r\n", newtime);
	strcat(header,pzTime );

	// HTTP header server name
	const char *pWebSrvType=GetProfile().GetString("HTTP","HTTPHeaderServerName",false);
	strcat(header,"Server: ");
	if (pWebSrvType && pWebSrvType[0])
		strcat(header,pWebSrvType);
	else
		strcat(header,"5Loaves");
	strcat(header,"\r\n");

	
	// HTTP header connection type
	GString strBrowserCachedHeader;
	strBrowserCachedHeader += "Connection: "; // "close" or "Keep-Alive"
	strBrowserCachedHeader += pzConnectionType;
	strBrowserCachedHeader += "\r\n";
	if (nIsKeepAlive)
	{
		strBrowserCachedHeader += "Keep-Alive: timeout=";
		strBrowserCachedHeader += g_nKeepAliveTimeout;
		strBrowserCachedHeader += ", max=";
		strBrowserCachedHeader += nKeepAliveReturnValue;
		strBrowserCachedHeader +=  "\r\n";
	}
	strcat(header,(const char *)strBrowserCachedHeader);

	
	// HTTP header: "Last-modified: Sunday Sun, 21 Oct 1995 19:38:46 GMT\n"
	if (pzETag)
	{
		time_t lFileTime = atol(pzETag);
		struct tm *tmFileTime = gmtime(&lFileTime);
		strftime(pzTime, 128, "Last-modified: %a, %d %b %Y %H:%M:%S GMT\r\n", tmFileTime);
		strcat(header,pzTime );
	}
	else
	{
		strftime(pzTime, 128, "Last-modified: %a, %d %b %Y %H:%M:%S GMT\r\n", newtime);
		strcat(header,pzTime );
	}



	// HTTP header: ETag
	if (pzFileName)
	{
		struct stat ss;
		stat(pzFileName,&ss);
		GString strETag("ETag: ");
		strETag += (unsigned long)ss.st_mtime; 
		strETag += "\r\n";
		strcat(header,(const char *)strETag);
	}
	
	// HTTP header: Content-type
	int bAddedContentType = 0;
	strcat(header,"Content-type: "); 
	if (pzExt)
	{
		GString strExt(pzExt);
		for (int iMime = 0; ;iMime++)
		{
			if (mimeTypeTable[iMime])
			{
				if ( strExt.CompareNoCase( mimeTypeTable[iMime] ) == 0)
				{
					bAddedContentType = 1;
					strcat(header,mimeTypeTable[iMime + 1]); 
					strcat(header,"\r\n");
					break;
				}
				else
				{
					iMime++;
				}
			}
			else
				break;
		}
		
		// check the user defined mime types
		if (!bAddedContentType)
		{
			const GList *pMimePairs = GetProfile().GetSection("MimeTypes");
			if (pMimePairs)
			{
				GListIterator itNVP(pMimePairs);
				while (itNVP())
				{
					GProfileEntry *pNVP = (GProfileEntry *)itNVP++;

					if ( strExt.CompareNoCase( pNVP->m_strName ) == 0)
					{
						bAddedContentType = 1;
						strcat(header,pNVP->m_strValue);
						strcat(header,"\r\n");
						break;
					}
				}
			}
		}

	}
	if (!bAddedContentType)
	{
		strcat(header,"text/html\r\n");
	}


	// If the return value is only a partial range of the entire file
	if (nBeginRange > -1)
	{
		// HTTP header: Content-length
		char cl[128];
		sprintf(cl,"Content-length: %d\r\n",(int) (nEndRange + 1) - nBeginRange);
		strcat (header,cl);

		// HTTP header: Content-Range: bytes 0-9699463/9699464
		sprintf(cl,"Content-Range: bytes %d-%d/%ld\r\n\r\n",(int)nBeginRange,(int)nEndRange,(int)lBytes);
		strcat (header,cl);
	}
	else // 98% go here - return the whole file
	{
		// HTTP header: Content-length
		char cl[128];
		sprintf(cl,"Content-length: %ld\r\n\r\n",(int)lBytes);
		strcat (header,cl);
	}
}


// Return file content from an HTTP request.
int ReturnFile(ThreadData *td, const char *pzHomeDir, const char *pzHTTPGET, int nSize, const char *pzRequestingIP, int nKeepAlive, int bisHead, char *buf1, char*buf2, int *nTotalProxiedBytes)
{
	g_TotalHits++;
	
	char pzFilePartialPath[1024];		// relative path (ex. /images/pic.gif)
	GString strFileNoPath;				// no path - file name only (ex. pic.gif)
	GString strExt(" ");				// ext only(ex.  gif)
	GString strFileName(pzHomeDir);		// complete path (ex. d:\home/images/pic.gif)
	GString strError(512);		// only used when sending back a 404, or 301				
	GString strHTML(512);		// only used when sending back a 404, or 301


	if (!pzHomeDir || pzHomeDir[0] == 0) // the HTTP server is enabled, perhaps to run the switchboard, but there are 
										 // no files to serve via HTTP requests because there is no home directory
	{
		// build a 404 Error
		strError = "HTTP/1.1 403 Forbidden\r\n";
		strHTML = "<HTML><HEAD><TITLE>403 Forbidden</TITLE></HEAD><BODY><H1>403 Forbidden</H1>This server is not configured to publish web content because no home folder is specified</BODY></HTML>";
		int nRet = SendHTTPNoFileResponse(td->sockfd, strError, strHTML);
		if (nRet > 0)
			*nTotalProxiedBytes += nRet;
		return 0;
		
	}


	//
	// Get the file name
	//
	char *pSpace = strpbrk((char *)pzHTTPGET," \r\n"); // pzHTTPGET = "/images/pic.gif HTTP/1.1 ............."
	char chSave;
	if (pSpace)	
	{
		// get the file name previous to the space before HTTP/1.1
		chSave = *pSpace;				// temp null 1st byte after the file name
		*pSpace = 0;
		strncpy(pzFilePartialPath,pzHTTPGET,1023);
		pzFilePartialPath[1023] = 0;


		// When posting data through a GET, from a browser, the 
		// URL Data has certain characters hex encoded (a space becomes %20 for example)
		unEscapeData(pzFilePartialPath,pzFilePartialPath);

		
		// append the file name to the path
		strFileName += pzFilePartialPath;	

		*pSpace = chSave;				// un-null
	}
	else
	{
		// This is a GET with no HTTP headers
		GString strF(pzHTTPGET, nSize);
		strFileName += strF;
		strcpy(pzFilePartialPath,pzHTTPGET);
	}


	// Test1:
	// prevent any file access behind the root or home directory.
	// the file path may be "/images/pic.gif", may not be "../images/pic.gif"
	// Test2:
	// Disable the HTTP "Range" feature, that is primarily used so that
	// one client can use multiple threads in the server to speed up downloads
	// by fetching different ranges of the same file at the same time.
	int bRange = GetProfile().GetBoolOrDefault("HTTP","EnableRange",1);
	if (strstr(pzFilePartialPath,"..") || ( strstr(pzHTTPGET,"Range:") && !bRange ) )
	{
		// build a 404 Error
		strError = "HTTP/1.1 404 Not Found\r\n";
		strHTML.Format("<HTML><HEAD><TITLE>404 Not Found</TITLE></HEAD><BODY><H1>404 Not Found</H1>The requested URL <code>%s</code> was not found on this server.</BODY></HTML>",(const char *)pzFilePartialPath);
		int nRet = SendHTTPNoFileResponse(td->sockfd, strError, strHTML);
		if (nRet > 0)
			*nTotalProxiedBytes += nRet;
		return 0;
	}

	char *pRange = strstr((char *)pzHTTPGET,"Range:");
	int nBeginRange = -1;
	int nEndRange = -1;
	if (pRange)
	{
		char *pNum = strpbrk(pRange + 6, "=");
		if (pNum)
		{
			nBeginRange = atoi(pNum+1);
			int bZeroOnly = GetProfile().GetBoolOrDefault("HTTP","RangeZeroBeginOnly",false);
			if (bZeroOnly && ( nBeginRange != 0) )
			{
				// drop the connection (like the server is too busy to handle it)
				// for working compatibility with most file download managers that
				// will finish the download on the primary connection.
				// This prevents a download manager to workaround connection bandwidth limits.
				return 0;
			}
			pNum = strpbrk(pNum+1, "-");
			if (pNum)
			{
				if (*pNum+1 > 47 && *pNum+1 < 58)
					nEndRange = atoi(pNum+1);
			}
		}
	}


	// This HTTP server can be configured to load balance by redirecting
	// the client to another server.  Check to see if Balancing is enabled.
	// This can be changed on the fly to balance per peak times or to
	// dodge forms of denial of service attacks.
	static const char *pzBalance = GetProfile().GetString("HTTP","Balance",false);
	// issue an HTTP redirect to another server.
	GString strBalanceToServer;
	GetBalanceTo(pzBalance,strBalanceToServer);

	// as long as the config file is valid - strBalanceToServer will always have a value
	if (strBalanceToServer.Length())
	{
		if (strBalanceToServer.CompareNoCase("this") != 0)
		{
			// build the redirect location
			GString strHREF(strBalanceToServer);	// different server
			strHREF += pzFilePartialPath;			// same place.

			strError =  "HTTP/1.1 301 Moved Permanently\r\n";
			strError += "Location: ";
			strError += strHREF;
			strError += "\r\n";

			strHTML.Format("<head><title>Document Moved</title></head><body><h1>Object Moved</h1>This document may be found <a HREF=\"%s\">here</a></body>",(const char *)strHREF);
			int nRet = SendHTTPNoFileResponse(td->sockfd, strError, strHTML);
			if (nRet > 0)
				*nTotalProxiedBytes += nRet;
			return 0; // this server has finished dealing with this request
		}
		// or service this request from this machine
	}


	// a GET for the index looks like this from IE or Netscape: pzHTTPGET= "/ HTTP/1.1 ..." 
	// also if the browser gives us a path (with no file) the FileName may end with
	//   a slash - append the default index page name as specified in the config file.
	if (strFileName.GetAt(strFileName.Length()-1) == '/' )
	{
		const char *pzIndex = GetProfile().GetString("HTTP","Index",false);
		if (pzIndex)
		{
			strcat(pzFilePartialPath,pzIndex);
			strFileName += pzIndex;
		}
		else
		{
			strcat(pzFilePartialPath,"index.html");
			strFileName += "index.html";
		}
	}
	
	
	// if strFileName contains only the home-dir path... give them the homepage
	if (pzHomeDir && strFileName.Compare(pzHomeDir) == 0)
	{
		const char *pzIndex = GetProfile().GetString("HTTP","Index",false);
		if (pzIndex)
		{
			strcat(pzFilePartialPath,pzIndex);
			strFileName += g_chSlash;
			strFileName += pzIndex;
		}
		else
		{
			strcat(pzFilePartialPath,"index.html");
			strFileName += g_chSlash;
			strFileName += "index.html";
		}
	}


	// walk the file name backwards until the first '.' to get the extension
	int nIdx = strlen(pzFilePartialPath) - 1;
	int bHasExt = 0;
	while (nIdx > -1)
	{
		if ( pzFilePartialPath[nIdx] == '.' && bHasExt == 0)
		{
			strExt = &pzFilePartialPath[nIdx] + 1;
			bHasExt = 1;
		}
		if ( pzFilePartialPath[nIdx] == '\\' ||  pzFilePartialPath[nIdx] == '/')
		{
			strFileNoPath = &pzFilePartialPath[nIdx] + 1;
			break;
		}
		nIdx--;
	}
	

	static const char *pzIPPage = GetProfile().GetString("HTTP","ShowIPAddressPageName",false);
	if ( pzIPPage && (strFileNoPath.CompareNoCase(pzIPPage) == 0) )
	{
		GString strIP;
		strIP.Format("<head><title>Your IP Address</title></head><body>Your Address is:%s</body>",pzRequestingIP);
		int nRet = HTTPSend(td->sockfd, strIP, (int)strIP.Length());
		if (nRet > 0)
			*nTotalProxiedBytes += nRet;
		return 0;
	}


	// if the page being requested has been configured as a page that is not browser-navigatable
	// but has been requested through an automated link-walking application.
	//
	// todo: the "static" below is there so that pzNoCrawl is only initialized one time and we do not incur the
	// overhead of this call for every HTTP page request, that's good, but it's bad because we cannot add this
	// to the "Defensive configuration" settings while the server is active and under attack.
	// use an integer comparison to a value mapped to a RegisterChangeNotification(), for the best of both worlds.
	static const char *pzNoCrawl = GetProfile().GetString("HTTP","DisableCrawlersPage",false);
	if (pzNoCrawl)
	{
		if ( !(pzNoCrawl[0] == 'n' && pzNoCrawl[1] == 'o') )
		{
			GStringList lst(",",pzNoCrawl);
			GStringIterator itPages(&lst);
			while (itPages())
			{
				if (strFileNoPath.CompareNoCase(itPages++) == 0)
				{
					// disable this IP(pzRequestingIP) for a while
					int nRefuseSeconds = GetProfile().GetInt("HTTP","CrawlerPenalty",false);
					
					GString strErr("[HTTP] CrawlerPenalty:");
					strErr << strFileNoPath;
					AddRefuseService(pzRequestingIP,nRefuseSeconds,strErr);
					return 0;
				}
			}
		}
	}




	// determine (next transaction) connection attributes
	GString strConnectionType("close");
	int nIsKeepAlive = 0;
	int nKeepAliveReturnValue = 0;
	if ( g_bUseHTTPKeepAlives )
	{
		char *pCon = strstr((char *)pzHTTPGET,"Connection:");
		if (pCon)
		{
			char *pEnd = strpbrk(pCon + 12, " \r\n");
			if (pEnd)
			{
				char chSave = *pEnd;
				*pEnd = 0;
				strConnectionType = pCon + 12;
				*pEnd = chSave;
				if (strConnectionType.CompareNoCase("Keep-Alive") == 0)
				{
					nIsKeepAlive = 1;
					nKeepAliveReturnValue = --nKeepAlive;
				}
			}
		}
	}



	// extract the Referer from the HTTP headers if supplied.

	// Interesting fact: The misspelling referer originated in the original proposal by computer scientist Phillip Hallam-Baker to incorporate the field into 
	// the HTTP specification.  The misspelling was set in stone by the time of its incorporation into the standards document Request for Comments (RFC) 1945; 
	// document co-author Roy Fielding has remarked that neither "referrer" nor the misspelling "referer" were recognized by the standard Unix spell checker 
	// of the period.  "RFC" means "Request For Comments" and nobody commented on it until it was already a standard so dont blame Phillip Hallam-Baker.
	// Blame the multitude who failed to comment.
	GString strReferer(" ");
	char *pReferer = (char *)strstr(pzHTTPGET,"Referer:");
	if (pReferer)
	{
		char *pRefererEnd = strpbrk(pReferer,"\r\n");
		if (pRefererEnd)
		{
			char chSave = *pRefererEnd;
			*pRefererEnd = 0;
			strReferer = pReferer + 9; // skip 9 bytes "Referer: "
			*pRefererEnd = chSave;
		}
	}




	// replace the file name they asked for with what we want them to have
	GString strLogComment;
	if (GetProfile().GetBoolean("HTTP","EnableLinkSwap",false))
	{
		const GList *pSpecialFile = GetProfile().GetSection(strFileNoPath);
		if (pSpecialFile)
		{
			GListIterator itNVP(pSpecialFile);
			while (itNVP())
			{
				GProfileEntry *pNVP = (GProfileEntry *)itNVP++;
				GString strCopy = pNVP->m_strValue;
				char *pSep = (char *)strpbrk(strCopy, "*");
				if (pSep)
				{
					*pSep = 0;
					if (strReferer.FindCaseInsensitive(pSep+1) != -1)
					{
						if (GetProfile().GetBoolean(strFileNoPath,"Disconnect",false))
						{
							// do not give them another image, give them nothing
							return 0;
						}

						// they asked for one thing, but give them another.
						strLogComment << "[Swapped]:" << (const char *)strFileNoPath;
						strFileNoPath = (const char *)strCopy;

						int nIdx = strlen(pzFilePartialPath) - 1;
						while (nIdx > -1)
						{
							if ( pzFilePartialPath[nIdx] == '\\' ||  pzFilePartialPath[nIdx] == '/')
							{
								strcpy(&pzFilePartialPath[nIdx+1],(const char *)pNVP->m_strName);
								break;
							}
							nIdx--;
						}
					}
				}
			}
		}
	}




	// this is a real time 'info' for this thread viewable by a thread process monitor
	td->strAction = "GET ";
	td->strAction << strFileNoPath;
	if (strReferer.Length() > 2)
		td->strAction << "," << strReferer;

	 

	// If the config file specifies that this file should be served from this, or other machines
	// as opposed to exclusively 'this' machine, then obtain the comma separated list of 
	// servers that this request might balance to.
	short bEnableHTTPBalancing = GetProfile().GetBoolean("HTTP-Balance","Enable",false);
	if (bEnableHTTPBalancing)
	{
		const char *pzBalanceFile = GetProfile().GetString("HTTP-Balance",strFileNoPath,false);
		const char *pzBalanceType = GetProfile().GetString("HTTP-Balance",strExt,false);

		
		
		const char *pzBalanceRange = 0;
		if ( pzBalanceFile || pzBalanceType || pzBalanceRange )
		{
			// Decide which balancing instructions to use:
			// Exact file name is highest precedence, next to extension match,
			// followed by a balance range match.
			const char *pzUse = (pzBalanceFile) ? pzBalanceFile : pzBalanceType;
			if (!pzUse)
				pzUse = pzBalanceRange;


			GString strBalanceToServer;
			GetBalanceTo(pzUse, strBalanceToServer);
			

			// if the 'balance to' is * then this server will handle the request,
			// any value is either an IP address or server name.
			if (strBalanceToServer.Length() && (strBalanceToServer.Compare("*") != 0) )
			{
				// Log the redirection
				strLogComment << "[Moved]:" << strBalanceToServer;
				MutexLog(strFileNoPath,pzRequestingIP,strReferer,strExt,strLogComment);

				strBalanceToServer += pzFilePartialPath;

				strError =  "HTTP/1.1 301 Moved Permanently\r\n";
				strError += "Location: ";
				strError += strBalanceToServer;
				strError += "\r\n";
				
				if(g_TraceHTTPBalance)
				{
					GString strMsg("Redirected To:");
					strMsg += strBalanceToServer;
					InfoLog(30,strMsg);
				}

				strHTML = "";
				int nRet = SendHTTPNoFileResponse(td->sockfd, strError, strHTML);
				if (nRet > 0)
					*nTotalProxiedBytes += nRet;

				// return and close the session - connection not kept alive
				return 0;

			}
			if(g_TraceHTTPBalance && (strBalanceToServer.Compare("*") == 0))
			{
				GString strMsg("NOT Redirected:");
				strMsg += strFileNoPath;
				InfoLog(31,strMsg);
			}
		}
	}



	// We will serve the file from this machine, log it to the file that exclusively logs HTTP file hits
	if (!bisHead)
		MutexLog(strFileNoPath,pzRequestingIP,strReferer,strExt,strLogComment);

	// write it to debug tracing too, if so configured
	if(g_TraceHTTPFiles)
	{
		GString strLog;
		strLog << "[" << strFileNoPath << "] to [" << pzRequestingIP << "]  HTTP referrer [" << strReferer << "]";
		InfoLog(111,strLog);
	}



	// Extract the ETag from the HTTP GET command, 
	GString strETag;
	char *p = strstr((char *)pzHTTPGET,"If-None-Match: ");
	if(p)
	{
		p+=15;// move past "If-None-Match: "
		char *pEnd = strpbrk(p, " \r\n");
		if (pEnd)
		{
			char chSave = *pEnd;
			*pEnd = 0;
			strETag = p;
			*pEnd = chSave;
		}
	}
	
	// if there was an ETag in the HTTP headers
	if (strETag.Length()) 
	{
		// load the last modified time from the file on disk
		struct stat s;
		stat((const char *)strFileName,&s);
		GString strLastModTime;
		strLastModTime.Format("%ld",s.st_mtime);

		// if it matches the ETag - shoot back a 304 to have the browser use it's local cache.
		if ( strLastModTime.Compare(strETag) == 0 )
		{
			// attach GString to pre-allocated memory 
			GString strBrowserCachedHeader(buf1, 0, MAX_RAW_CHUNK, 0);

			strBrowserCachedHeader += "HTTP/1.1 304 Not Modified\r\n";
			const char *pWebSrvType=GetProfile().GetString("HTTP","HTTPHeaderServerName",false);
			strBrowserCachedHeader += "Server: ";
			if (pWebSrvType && pWebSrvType[0])
				strBrowserCachedHeader += pWebSrvType;
			else
				strBrowserCachedHeader += "5Loaves";
			strBrowserCachedHeader += "\r\n";
				

			char pzTime[128];
			struct tm *newtime;
			time_t ltime;
			time(&ltime);
			newtime = gmtime(&ltime);
			strftime(pzTime, 128, "Date: %a, %d %b %Y %H:%M:%S GMT\r\n", newtime);

			strBrowserCachedHeader += pzTime;
			
			strBrowserCachedHeader += "Connection: "; // "close" or "Keep-Alive"
			strBrowserCachedHeader += (const char *)strConnectionType;
			strBrowserCachedHeader += "\r\n";

	
			// Defensive configuration supported - not reported.
			if (nIsKeepAlive)
			{
				strBrowserCachedHeader << "Keep-Alive: timeout=" << g_nKeepAliveTimeout << ", max=" << nKeepAliveReturnValue << "\r\n";
			}

			strBrowserCachedHeader += "ETag: "; 
			strBrowserCachedHeader +=  strETag;
			strBrowserCachedHeader +=  "\r\n";
			strBrowserCachedHeader +=  "Content-Length: 0\r\n\r\n";

			if ( nonblocksend(td->sockfd,strBrowserCachedHeader,(int)strBrowserCachedHeader.Length()) == strBrowserCachedHeader.Length() )
				*nTotalProxiedBytes += (int)strBrowserCachedHeader.Length();

			return nKeepAliveReturnValue;
		}
		else
		{
			// otherwise uncache any file that has been changed since it was cached.
			char *pOldHTTPData = (char *)cacheManager.UncacheData(strFileName);
			if (pOldHTTPData)
				delete pOldHTTPData;
		}
	}


	// look in the data cache for the pre-assembled HTTP headers and unciphered data.
	long length = 0;
	unsigned char *pData = (unsigned char *)cacheManager.GetCacheData((const char *)strFileName, &length);
	if (pData)
	{
		// if they only requested part of the file with a valid Begin Range
		if (nBeginRange > -1 && nBeginRange < length) 
		{
			// size of cached data + pre-built HTTP headers
			char *pContent= strstr((char *)pData,"\r\n\r\n");
			int nHeaderLen = (pContent - (const char *)pData) + 4; 
			int nContentLen = length - nHeaderLen; // entire length of raw file data
			
			if (nBeginRange < nContentLen)
			{
				if (nEndRange > 0 && nEndRange < nContentLen)
				{
					// user set a valid end range
				}
				else // we set it for the user
				{
					nEndRange = nContentLen - 1; // a zero based index in the HTTP header
				}
				nContentLen = (nEndRange + 1) - nBeginRange; // # of bytes in file to send


				// send them back a piece of the file in the cache.  This is request for a range
				// of bytes that will be sent back out of our complete HTTP response (headers+data)
				// that was cached.
				if (nContentLen > 0)
				{
					// send the headers
					BuildHTTPHeader(buf1, strFileName, "close" , 0, 0, strETag, strExt, length - nHeaderLen, nBeginRange, nEndRange);
					int nHdLen = strlen(buf1);

					if ( nonblocksend(td->sockfd,buf1,nHdLen) == nHdLen)
					{
						*nTotalProxiedBytes += nHdLen;

						// send the data
						int nCtLen = (nEndRange + 1) - nBeginRange;
						if ( nonblocksend(td->sockfd,pContent+4+nBeginRange, nCtLen ) == nCtLen)
						{
							*nTotalProxiedBytes += nCtLen;
						}
					}
				}
			}
		}
		else // send the entire header/content chunk directly from cache-memory (98%)
		{
			if (bisHead) // HTTP HEAD command requests the headers only, no data
			{
				char *pContent= strstr((char *)pData,"\r\n\r\n");
				length = (pContent - (const char *)pData) + 4;
			}

			// send him the file he asked for
			if ( nonblocksend(td->sockfd,pData,length) == length)
			{
				*nTotalProxiedBytes += length;
			}
		}
	}
	
	// else the file was not found in the cache, go get it from disk
	else
	{
		char *buf = 0;
		long lBytes = 0;

		const char *pDiskCipherPass=GetProfile().GetString("HTTP","EncryptedHomeDirPassword",false);
		if (pDiskCipherPass && !pDiskCipherPass[0])
			pDiskCipherPass = 0;

		if (pDiskCipherPass)
		{
			// returns 1 on success, 0 on Fail with description in  strErrorOut
			// pDest will be allocated upon success.
			// pDest will always start with these 7 bytes: 5Loaves, the nDestLen will 
			// be set to the length of the data following the first 7 bytes.
			int nDestLen;
			GString strErrorOut;
			GString strPass(pDiskCipherPass);
			if (!FileDecryptToMemory(strPass, strFileName, &buf, &nDestLen, strErrorOut))
			{
				GString strErr;
				strErr.Format("\nEncryptedHomeDir file[%s]\nReports:[%s]",(const char *)strFileName,(const char *)strErrorOut);
				InfoLog(32,strErr);

				// build a 404 Error
				strError = "HTTP/1.1 404 Not Found\r\n";
				strHTML.Format("<HTML><HEAD><TITLE>404 Not Found</TITLE></HEAD><BODY><H1>404 Not Found</H1>The requested URL <code>%s</code> was not found on this server.</BODY></HTML>",(const char *)pzFilePartialPath);
				int nRet = SendHTTPNoFileResponse(td->sockfd, strError, strHTML);
				if (nRet > 0)
					*nTotalProxiedBytes += nRet;
				return 0;
			}
			lBytes = nDestLen;

		}
		else // get the file from an unencrypted disk file
		{

			
			FILE *fp = fopen((const char *)strFileName,"rb");
			if (fp)
			{
				// get the size of the file
				fseek(fp,0,SEEK_END);
				lBytes = ftell(fp);
				fseek(fp,0,SEEK_SET);

				// if the file is < 1 Meg (or we're going to cache the content) read in the whole thing
				if (lBytes < 1024000 || DoesCacheHTTPContent(strExt,lBytes) ) 
				{
					// load the file content file
					buf = new char[lBytes +1];
					if (fread(buf,sizeof(char),lBytes,fp) != (unsigned int)lBytes)
					{
						InfoLog(33,"Failed to read entire file\n");
						lBytes = 0;
					}
				}
				fclose(fp);
			}
			else // if the requested file was not found try appending
				 // a default extension if configured to do so.  For example, if 
				 // "document" was requested and we are configured to run with
				 // a default extension of ".html" - look for "document.html" on disk.
			{
				const char *pzDefaultExtensions = GetProfile().GetString("HTTP","DefaultExtensions",false);
				if (pzDefaultExtensions)
				{
					GStringList types(",",pzDefaultExtensions);
					GStringIterator itTypes(&types);
					while (itTypes())
					{
						GString strNewFileName(strFileName);
						const char *pzAppendedExtension = itTypes++;
						strNewFileName += ".";
						strNewFileName += pzAppendedExtension;

						// now try to open the file with the default extension appended to the file name
						FILE *fp = fopen((const char *)strNewFileName,"rb");
						if (fp)
						{
							
							strFileName = strNewFileName;
							strExt = pzAppendedExtension;

							// get the size of the file
							fseek(fp,0,SEEK_END);
							lBytes = ftell(fp);
							fseek(fp,0,SEEK_SET);

							if (lBytes < 1024000 || DoesCacheHTTPContent(strExt,lBytes) ) 
							{
								// load the file content
								buf = new char[lBytes +1];
								if (fread(buf,sizeof(char),lBytes,fp) != (unsigned int)lBytes)
								{
									InfoLog(34,"Failed to read entire file\n");
									lBytes = 0;
								}
							}
							fclose(fp);
							break; // out of the "while (itTypes())"
						}
					}
				}
			}
		}
		if (lBytes)
		{
			//
			// Build the HTTP Headers
			//
			
			// if they only requested part of the file (2%)
			if (nBeginRange > -1 && nBeginRange < lBytes) 
			{
				
				int nContentBytes = -1;
				if ( nEndRange > 0 && nEndRange < lBytes )
				{
					// user set a valid end range
				}
				else
				{
					// we set it for the user at the last indexable byte
					nEndRange = lBytes - 1;
				}
				nContentBytes = (nEndRange + 1) - nBeginRange;


				BuildHTTPHeader(buf1, strFileName, "close" , 0, 0, strETag, strExt, lBytes, nBeginRange, nEndRange);
				int nHeaderLen = strlen(buf1);
				
		
				// open the file(already verified as valid/accessible) and obtain the piece requested
				if (nContentBytes > 0)
				{
					// send the header
					if ( nonblocksend(td->sockfd,buf1,nHeaderLen) == nHeaderLen)
						*nTotalProxiedBytes += nHeaderLen;


					// send the data
					char *pzBlockData = buf1;
					FILE *fp = fopen((const char *)strFileName,"rb");
					fseek(fp,nBeginRange,SEEK_SET);

					if (fp)
					{
						int nBytesToSend = nContentBytes;
						while (nBytesToSend > 0)
						{
							int nChunkSize = (nBytesToSend > MAX_RAW_CHUNK) ? MAX_RAW_CHUNK : nBytesToSend;
							if (fread(pzBlockData,sizeof(char),nChunkSize,fp) != (unsigned int)nChunkSize)
							{
								InfoLog(35,"Failed to read file chunk\n");
								lBytes = 0;
								break;
							}
							if (nonblocksend(td->sockfd,pzBlockData,nChunkSize) != nChunkSize)
							{
								break;
							}
							nBytesToSend -= nChunkSize;
							*nTotalProxiedBytes += nChunkSize;
						}
						fclose(fp);
					}
				}
			}
			else // they want the whole file (98%)
			{
				BuildHTTPHeader(buf1, strFileName, strConnectionType, nIsKeepAlive, nKeepAliveReturnValue, strETag, strExt, lBytes, -1, -1);
				int nHeaderLen = strlen(buf1);

				char *pzCacheData = 0;
				// Add Headers + data then send the whole thing in one write
				if (buf) 
				{
					pzCacheData = new char[lBytes + nHeaderLen + 1];
					strcpy(pzCacheData,buf1);
					if (pDiskCipherPass) // start past the "5Loaves" tag in the deciphered data stream
						memcpy(&pzCacheData[nHeaderLen],&buf[7],lBytes);
					else
						memcpy(&pzCacheData[nHeaderLen],buf,lBytes);

					// send it back (to the browser usually)
					if (nonblocksend(td->sockfd,pzCacheData,nHeaderLen+lBytes) == nHeaderLen+lBytes)
						*nTotalProxiedBytes += nHeaderLen+lBytes;

				}
				else // chunk up the data and send it back
				{
					
					// send the (headers and) data in the first chunk
					char *pzCacheData = buf2;
					FILE *fp = fopen((const char *)strFileName,"rb");
					if (fp)
					{
						int nBytesToSend = lBytes;
						int bIsFirstChunk = 1;
						int nHeaderOffset = 0;
						while (nBytesToSend > 0)
						{
							int nChunkSize = (nBytesToSend > MAX_RAW_CHUNK) ? MAX_RAW_CHUNK : nBytesToSend;
							if(bIsFirstChunk)
							{
								bIsFirstChunk = 0;
								nChunkSize -= nHeaderLen;
								memcpy(pzCacheData,buf1,nHeaderLen);
								nHeaderOffset = nHeaderLen;
							}
							else
							{
								nHeaderOffset = 0;
							}

							if (fread(pzCacheData + nHeaderOffset,sizeof(char),nChunkSize,fp) != (unsigned int)nChunkSize)
							{
								InfoLog(36,"Failed to read file chunk\n");
								lBytes = 0;
								break;
							}
							if (nonblocksend(td->sockfd,pzCacheData,nChunkSize+nHeaderOffset) != nChunkSize+nHeaderOffset)
							{
								break;
							}
							nBytesToSend -= nChunkSize;
							*nTotalProxiedBytes += nChunkSize;
						}
						fclose(fp);
					}
				}

				// if so configured, leave this complete HTTP response (headers + data) in the cache
				if (DoesCacheHTTPContent(strExt,lBytes))
				{
					cacheManager.EnterCacheData(nHeaderLen+lBytes,pzCacheData,(const char *)strFileName);
				}
				else
				{
					// otherwise free the memory and rebuild this message next time someone asks for it.
					delete pzCacheData;

					// todo: if we know in advance that we will NOT cache this AND know in advance that 
					// our system buffer is large enough to contain the data, then use the system buffer 
					// and do not make the allocation, and obviously do not free it here. == FASTER.
				}
			}
			if (buf)
			{
				delete buf; // the 'whole file' data buffer

				// todo:avoid the allocation of buf if the system buffer can be used and make sure not to free what we did not allocate.
				// This will require a new decryption API.  If decryptions are always cached, this unnecessary allocation will only happen 
				// on the first hit from disk, if so - this is very minor.
			}
		}
		else // we failed to find the file requested.
		{
			// Rather than sputtering out a 404, we auto-redirect them to an alternate page/site.
			const char *pz404Redirect = GetProfile().GetString("HTTP","PageRedirect",false);

			// this only applies to HTML pages (not images, javascript etc)
			const char *p = strExt;
			if (  
				  pz404Redirect 
				  && strlen(p) > 2 
				  && (p[0] == 'H' || p[0] == 'h') 
				  && (p[1] == 'T' || p[1] == 't') 
				  && (p[2] == 'M' || p[2] == 'm') 
			   )
			{

				g_Total404s++;	// HTTP 404 error count
				// log the "broken link" if so configured
				const char *pzLog = GetProfile().GetString("HTTP","404Log",false);
				if (pzLog)
				{
					strError.Format("File [%s] not found\n",(const char *)strFileName);
					strError.ToFileAppend(pzLog,0);
				}

				strError =  "HTTP/1.1 301 Moved Permanently\r\n";
				strError += "Location: ";
				strError += pz404Redirect;
				strError += "\r\n";

				strHTML.Format("<head><title>Document Moved</title></head><body><h1>Object Moved</h1>This document may be found <a HREF=\"%s\">here</a></body>",pz404Redirect);
			}
			else
			{
				
				// see if strFileName is a directory.  If the user supplied a trailing slash
				// we already knew that this was a directory and never got here - but since you
				// cannot differentiate between a file and a folder(unless a trailing slash is 
				// provided) we assumed it was a file - but didn't find this file.  Now, if this
				// turns out to be a folder, append the trailing slash and redirect the browser
				// back to the same location with the the trailing slash.
				if ( GDirectory::IsDirectory(ReSlash(strFileName)) )
				{
					// in the config file is a setting like this:
					// ThisSiteURL=http://www.UnitedBusinessTechnologies.com
					const char *pz404Redirect = GetProfile().GetString("HTTP","ThisSiteURL",false);
					if (pz404Redirect)
					{
						GString strRedirectTo(pz404Redirect);
						strRedirectTo += pzFilePartialPath; // add something like "/AtHome"
						strRedirectTo += "/";

						strError =  "HTTP/1.1 301 Moved Permanently\r\n";
						strError += "Location: ";
						strError += strRedirectTo;
						strError += "\r\n";

						strHTML.Format("<head><title>Document Moved</title></head><body><h1>Object Moved</h1>This document may be found <a HREF=\"%s\">here</a></body>",(const char *)strRedirectTo);
					}
				}
				else
				{
					// build a 404 Error
					strError = "HTTP/1.1 404 Not Found\r\n";
					strHTML.Format("<HTML><HEAD><TITLE>404 Not Found</TITLE></HEAD><BODY><H1>404 Not Found</H1>The requested URL <code>%s</code> was not found on this server.</BODY></HTML>",(const char *)pzFilePartialPath);

					g_Total404s++;	// HTTP 404 error count
					// log the "broken link" if so configured
					const char *pzLog = GetProfile().GetString("HTTP","404Log",false);
					if (pzLog)
					{
						//12=HTTP Content File [%s] requested but not found
						GString strLog;
						strLog.Format("File[%s] or Method[%s] Not found",(const char *)strFileName,(const char *)strFileNoPath);
						InfoLog(37,strLog);

					}
				}
			}

			int nRet = SendHTTPNoFileResponse(td->sockfd, strError, strHTML);
			if (nRet > 0)
				*nTotalProxiedBytes += nRet;

			// Setting nKeepAliveReturnValue = 0 will close the keep alive connection when the request could not be fulfilled
			// The browser (IE anyway) always closes it - but theoretically it does not have to and other browser may not
			// nKeepAliveReturnValue = 0;
		}
	}
	return nKeepAliveReturnValue;
}




void ProxyLog(const char *szConfigSectionName, int nThreadID, char *pData, int nBytes, const char *prefix )
{
	GString strLogFile (    GetProfile().GetPath(szConfigSectionName,"LogPath",false)   );
	if (strLogFile.Length())
	{
		char pzJoulDate[16];
		char pzTime[64];
		struct tm *newtime;
		time_t ltime;
		time(&ltime);
		newtime = gmtime(&ltime);
		strftime(pzJoulDate, 16, "%j%Y", newtime);
		strftime(pzTime, 64, "%H:%M:%S", newtime);
		
		
		#ifdef _WIN32
			_mkdir(strLogFile);
		#else
			mkdir(strLogFile,777);
		#endif


		strLogFile += pzJoulDate;
		#ifdef _WIN32
			mkdir(strLogFile);
		#else
			mkdir(strLogFile,777);
		#endif

		// where to log it
		strLogFile += g_chSlash;
		strLogFile += szConfigSectionName;
		strLogFile += ".txt";

		// log prefix header
		GString strPrefix("\n\n\n");
		strPrefix << prefix;
		strPrefix.write(pzTime,8);
		strPrefix << "-";
		char pzBytes[16];
		sprintf(pzBytes,"%06d",(int)nBytes);
		strPrefix.write(pzBytes,6);
		strPrefix << ">\n";

		// log binary
		if (GetProfile().GetBool(szConfigSectionName,"LogBinary",false))
		{
			GString strBinary(strPrefix);
			strBinary.FormatBinary((unsigned char *)pData,nBytes);
			strBinary.ToFileAppend(strLogFile);
		}
		else // or log text
		{
			FILE *fp = fopen((const char *)strLogFile,"a");
			if (fp)
			{
				fwrite((const char *)strPrefix,1,(int)strPrefix.Length(),fp);
				fwrite(pData,1,nBytes,fp);
				fclose(fp);
			}
		}
	}
}



int PrepForServer(unsigned short sAttribs, int bIsTunnel,CipherData *pcd, char *sockBuffer,int nDataSize,char *cryptDest, int *newLength)
{
	if (nDataSize > MAX_DATA_CHUNK)
	{
		InfoLog(40,"protocol error - packet too big");
		return 0;
	}
	

	int bCipher = ( sAttribs & ATTRIB_CRYPTED_2FISH ) ? 1 : 0;
	int bZip	= ( sAttribs & ATTRIB_COMPRESSED_BZIP ) ? 1 : 0;
	int gZip	= ( sAttribs & ATTRIB_COMPRESSED_GZIP ) ? 1 : 0;


	GString strMessage;
	int nBitsCrypted;
	if (bIsTunnel)
	{
		// Compress
		if (bZip | gZip)
		{
			// in the worst case (data is already compressed) the output
			// may actually expand a little (~500 bytes on a 65535 byte buffer)
			*newLength = MAX_DATA_CHUNK;
			
			if (nDataSize > 64)
			{
				// true indicates that the packet uses compression
				cryptDest[0] = 1;

				// done
				try{

				int cRet;
				if (gZip)
				{
				  #ifndef _NO_GZIP_
					cRet = GZipBuffToBuffCompress(
										&cryptDest[1], // destination
									   (unsigned int*)newLength,	
									   sockBuffer, // source
									   nDataSize					   );
				  #endif
				}
				else // bZip
				{
					 
					cRet = BZ2_bzBuffToBuffCompress( &cryptDest[1], // destination
									   (unsigned int*)newLength,	
									   sockBuffer, // source
									   nDataSize,
									   9, //blockSize100k, 
									   0,// verbosity, 
									   30);//workFactor )
				}




				if (cRet != 0)
				{
					GString strErr("Compress failed. point 1, code:");
					strErr << cRet;
					InfoLog(39,strErr);
					*newLength = 0;
					return 0;
				}
				else
				{
					*newLength += 1;
				}
				}catch(...)
				{
					GString strErr("Compress failed. point 2");
					InfoLog(39,strErr);
					*newLength = 0;
					return 0;
				}
			}
			else
			{
				// false indicates that the packet does not use compression, it's too small to benefit
				cryptDest[0] = 0;
				memcpy(&cryptDest[1],sockBuffer,nDataSize);
				*newLength = nDataSize + 1;
			}
			
			// hang onto running total for runtime statistical info
			g_PreZipBytes += nDataSize;
			g_PostZipBytes += *newLength;

			if (bCipher) // put it where the cipher input will pick it up
			{
				memcpy(sockBuffer,cryptDest,*newLength);
				nDataSize = *newLength;
			}

			if (g_DataTrace) DataTrace("After Compress", cryptDest, *newLength);
		}
		// Encrypt
		if (bCipher)
		{
			// "Encrypted - Sent to tunnel server:

			// bytes required to pad the last block to a 128 bit boundary
			unsigned char nPad = (16 - nDataSize % 16);
			nPad = (nPad == 16) ? 0 : nPad;

			// this is the size (in bytes) to encrypt.
			*newLength = nDataSize + nPad + 16;

			// create a trailer block of 128 bits
			// write the pad size in the first trailer byte 
			sockBuffer[nDataSize + nPad] = nPad;
			// null the rest of the last block
			memset(&sockBuffer[nDataSize + nPad+1],0,15);

			try
			{
				nBitsCrypted = pcd->blockEncrypt((unsigned char *)sockBuffer,(*newLength)*8,(unsigned char *)cryptDest);
			}
			catch(...)
			{
				return 0;
			}
			if (nBitsCrypted != (*newLength)*8)
			{
				return 0;
			}
			if (g_DataTrace) DataTrace("After Encrypt", cryptDest, *newLength);
		}
	}
	else
	{
		if (bCipher)
		{
			// Decrypted - Sent to Application server

			if (nDataSize*8 % BLOCK_SIZE)
			{
				InfoLog(40,"protocol error.");
				return 0;
			}

			try
			{
				nBitsCrypted = pcd->blockDecrypt((unsigned char *)sockBuffer,nDataSize*8,(unsigned char *)cryptDest);
			}
			catch(...) // this can happen when using a 128 key to undo 256 data
			{
				return 0;
			}
			if (nBitsCrypted != nDataSize*8)
			{
				return 0;
			}

			// get the first byte of the last 128 bit block
			unsigned char chPad = (unsigned char)cryptDest[(nBitsCrypted/8) - 16];
			// ASSERTION, the byte pad count must be less than a block size
			if (chPad > 16)
			{
				InfoLog(41,"Wrong Password.");
				return 0;
			}
			// and the new size removes the last block and padding bits added to the second to last block.
			*newLength = (nBitsCrypted/8) - 16 - chPad;
			if (g_DataTrace) DataTrace("After Decrypt", cryptDest, *newLength);
		}

		// Decompress
		if (bZip || gZip) 
		{

			if (bCipher) // move cipher output to compression input
			{
				memcpy(sockBuffer,cryptDest,*newLength);
				nDataSize = *newLength;
			}

			if ( sockBuffer[0] )
			{
				try{
				*newLength = MAX_DATA_CHUNK;
				int cRet;
				if (gZip)
				{
				  #ifndef _NO_GZIP_
					cRet = GZipBuffToBuffDecompress
										   ( cryptDest, // destination
											 (unsigned int *)newLength, // decompressed length
											 &sockBuffer[1], // source
											 nDataSize - 1); // source len
				  #endif
				}
				else // bZip
				{
					cRet = BZ2_bzBuffToBuffDecompress 
										   ( cryptDest, // destination
											 (unsigned int *)newLength, // decompressed length
											 &sockBuffer[1], // source
											 nDataSize - 1, // source len
											 1,// or 0 small,
											 0);//verbosity )
				}


				if (cRet != 0)
				{
					//14=Decompress failed.  Wrong password-key attempt?
					GString strLog;
					strLog.LoadResource(g_pzCoreErrorSection, 14);
					InfoLog(42,strLog);

					*newLength = 0;
					return 0;
				}
				}catch(...)
				{
					InfoLog(42,"Decompress failed - unknown error");
					*newLength = 0;
					return 0;
				}
			}
			else
			{
				memcpy(cryptDest,&sockBuffer[1],nDataSize-1);
				*newLength = nDataSize-1;
			}

			if (g_DataTrace) DataTrace("After Decompress", cryptDest, *newLength);
		}

	}

	// for packet sync support
	if (!bCipher && (!(bZip || gZip)))
	{
		memcpy(cryptDest,sockBuffer,nDataSize);
		*newLength = nDataSize;
	}

	return 1;
}


int PrepForClient(unsigned short sAttribs, int bIsTunnel,CipherData *pcd, char *sockBuffer,int nDataSize,char *cryptDest, int *newLength)
{
	 
	int bCipher = ( sAttribs & ATTRIB_CRYPTED_2FISH ) ? 1 : 0;
	int bZip	= ( sAttribs & ATTRIB_COMPRESSED_BZIP ) ? 1 : 0;
	int gZip	= ( sAttribs & ATTRIB_COMPRESSED_GZIP ) ? 1 : 0;

	
	if (bIsTunnel)
	{
		if (bCipher)
		{
			if (g_DataTrace) DataTrace("Before Decrypt", sockBuffer, nDataSize);
			// Decrypted - Sent to Client Application:
			if (nDataSize*8 % BLOCK_SIZE)
			{
				//15=protocol error - invalid block size[%d]
				GString strLog;
				strLog.LoadResource(g_pzCoreErrorSection, 15, nDataSize);
				InfoLog(43,strLog);
				return 0;
			}

			int nBitsCrypted;
			try
			{
				nBitsCrypted = pcd->blockDecrypt((unsigned char *)sockBuffer,nDataSize*8,(unsigned char *)cryptDest);
			}
			catch(...) // this can happen when using a 128 key to undo 256 data
			{
				return 0;
			}
			if (nBitsCrypted != nDataSize*8)
			{
				//16=Wrong password-key attempt?
				GString strLog;
				strLog.LoadResource(g_pzCoreErrorSection, 16);
				InfoLog(44,strLog);
				return 0;
			}
			// get the first byte of the last 128 bit block
			unsigned char chPad = (unsigned char)cryptDest[(nBitsCrypted/8) - 16];
			// the byte pad count must be less than a block size if the decrypt was a success
			if (chPad > 16)
			{
				//16=Wrong password-key attempt?
				GString strLog;
				strLog.LoadResource(g_pzCoreErrorSection, 16);
				InfoLog(45,strLog);
				return 0;
			}

			// and the new size removes the last block and padding bits added to the second to last block.
			*newLength = (nBitsCrypted/8) - 16 - chPad;
			if (g_DataTrace) DataTrace("After Decrypt", cryptDest, *newLength);
		}

		// Decompress
		if (bZip || gZip)
		{
			if (bCipher) // move cipher output to compression input
			{
				memcpy(sockBuffer,cryptDest,*newLength);
				nDataSize = *newLength;
			}

			if ( sockBuffer[0] ) // first byte is a flag indicating if this packet is compressed
			{
				try{
				*newLength = MAX_DATA_CHUNK;
				int cRet;
				if (gZip)
				{
				  #ifndef _NO_GZIP_
					 cRet = GZipBuffToBuffDecompress
										   ( cryptDest, // destination
											 (unsigned int *)newLength, // decompressed length
											 &sockBuffer[1], // source
											 nDataSize - 1); // source len
				  #endif
				}
				else // bZip
				{
						cRet = BZ2_bzBuffToBuffDecompress 
										   ( cryptDest, // destination
											 (unsigned int *)newLength, // decompressed len
											 &sockBuffer[1], // source
											 nDataSize - 1, // source len
											 1,// or 0 small,
											 0);//verbosity )
				}
				if (cRet != 0)
				{

					//14=Decompress failed.  Wrong password-key attempt?
					GString strLog;
					strLog.LoadResource(g_pzCoreErrorSection, 14);
					InfoLog(46,strLog);

					*newLength = 0;
					return 0;
				}
				}catch(...)
				{
					InfoLog(46,"Decompress failed - unknown error");
					*newLength = 0;
					return 0;
				}
			}
			else
			{
				memcpy(cryptDest,&sockBuffer[1],nDataSize-1);
				*newLength = nDataSize-1;
			}
			if (g_DataTrace) DataTrace("Decompressed", cryptDest, *newLength);
		}
	}
	else
	{
		// Compress
		if (bZip || gZip)
		{
			if (g_DataTrace) DataTrace("Before Compress", sockBuffer, nDataSize);

			*newLength = MAX_DATA_CHUNK;
			if (nDataSize > 64)
			{
				// true indicates that the packet uses compression
				cryptDest[0] = 1;

				try{
				int cRet;
				if (gZip)
				{
				  #ifndef _NO_GZIP_
					cRet = GZipBuffToBuffCompress(
										&cryptDest[1], // destination
									   (unsigned int*)newLength,	
									   sockBuffer, // source
									   nDataSize					   );
				  #endif
				}
				else // bZip
				{
					cRet = BZ2_bzBuffToBuffCompress( &cryptDest[1],
									   (unsigned int*)newLength,
									   sockBuffer, // raw data
									   nDataSize,
									   9, //blockSize100k, 
									   0,// verbosity, 
									   30);//workFactor )
				}

				if (cRet != 0)
				{
					GString strErr("Compression failed - code:");
					strErr << cRet;
					InfoLog(47,strErr);
					*newLength = 0;
				}
				else
				{
					*newLength += 1;

				}
				}catch(...)
				{
					InfoLog(47,"Compression failed - unknown error");
					*newLength = 0;
				}
				
			}
			else
			{
				// false indicates that the packet does not use compression
				cryptDest[0] = 0;
				memcpy(&cryptDest[1],sockBuffer,nDataSize);
				*newLength = nDataSize + 1;
			}
			
			// hang onto running total for statistical info
			g_PreZipBytes += nDataSize;
			g_PostZipBytes += *newLength;

			if (bCipher) // put it where the cipher input will pick it up
			{
				memcpy(sockBuffer,cryptDest,*newLength);
				nDataSize = *newLength;
			}
			if (g_DataTrace) DataTrace("After Compress", cryptDest, *newLength);

		}
		
		// Encrypt
		if (bCipher)
		{
			// "Encrypted - Sent to Client Tunnel:";
			// bytes required to pad the last block to a 128 bit boundary
			unsigned char nPad = (16 - nDataSize % 16);
			nPad = (nPad == 16) ? 0 : nPad;

			// this is the size (in bytes) to encrypt.
			*newLength = nDataSize + nPad + 16;

			// create a trailer block of 128 bits
			// write the pad size in the first trailer byte 
			sockBuffer[nDataSize + nPad] = nPad;
			// null the rest of the last block
			memset(&sockBuffer[nDataSize + nPad+1],0,15);
			
			int nBitsCrypted;
			try
			{
				nBitsCrypted = pcd->blockEncrypt((unsigned char *)sockBuffer,(*newLength)*8,(unsigned char *)cryptDest);
			}
			catch(...)
			{
				return 0;
			}
			if (nBitsCrypted != (*newLength)*8)
			{
				return 0;
			}
			if (g_DataTrace) DataTrace("After Crypt", cryptDest, *newLength);
		}
	}
	
	// for packet sync support
	if (!bCipher && (!(bZip || gZip)))
	{
		memcpy(cryptDest,sockBuffer,nDataSize);
		*newLength = nDataSize;
	}
	return 1;
}


// after calling ReadPacketHTTP(), this code:
// int nHTTPHeaderLen = pContent - pzHTTPHeader; 
// will get you the HTTP header len IN THE FIRST packet read.
int ReadPacketHTTP(int fd, int *pnBytesInBuf, char *Buf, unsigned int *nPktIndex, unsigned long *pnContentLen = 0, char **ppContent = 0, int nTimeOutSec = 60, int nTimeOutMicroSec = 0)
{
	int nHeaderLength = 0;
	int nBytes = 0;
	char *pContent = 0;

	//
	//  if we're reading the next chunk of content then *nPktIndex equals the bytes left of Content-Length to read 
	if (*nPktIndex > 0)
	{
		// int nBufIndex = 0; // read-ahead not supported in HTTP mode, start at the beginning of Buf
		int nRemaining = *nPktIndex;
		if( nRemaining  > MAX_RAW_CHUNK)
		{
			nRemaining = MAX_RAW_CHUNK;
		}

		int rslt = readableTimeout(fd, nTimeOutSec, nTimeOutMicroSec);
		// failure during wait || time limit expired
		if (rslt == -1 || rslt == 0)
		{
			if(g_TraceConnection && rslt == 0)
			{
				GString strErr;
				strErr.Format("ReadPacketHTTP SUB timeout(%d seconds) code %d",(int)nTimeOutSec,(int)SOCK_ERR);
				InfoLog(47,strErr);
			}
			return rslt; 
		}

		int nRcvRetryCount = 0;
SUB_RCV_RETRY:		
		nBytes = recv(fd, Buf, nRemaining, 0 ); // take what ever is available and return to caller
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
		*nPktIndex -= nBytes;
		*pnBytesInBuf += nBytes;
		return 1; // process the data
	}


	//
	// otherwise read the primary packet (sub packets are subsequent packet data )
	//
	*pnBytesInBuf = 0; // read-ahead not supported in HTTP mode, start at the beginning of Buf
	int nRcvRetryCount = 0;
	int rslt = readableTimeout(fd, nTimeOutSec, nTimeOutMicroSec);
	// failure during wait || time limit expired
	if (rslt == -1 || rslt == 0)
	{
		return rslt;
	}

	// enters a byte by byte read while bReadingHTTPHeader is true.  
	int bReadingHTTPHeader = 1;


	// honest HTTP headers are not 64k, RFC 2616 for HTTP 1.1 makes no specifications for a URL length
	// Microsoft Internet Explorer as of version 7, (in July 2007) supports a maximum URL length of 2,083
	// Opera's URL limit is 4,050, and Netscape 6 is 2000, if we pull in more than 4096 of HTTP header it's probably junk.
	while(bReadingHTTPHeader && (*pnBytesInBuf < MAX_RAW_CHUNK) && (*pnBytesInBuf < g_nMaxHTTPHeaderSize) )
	{
RCV_RETRY:
		nBytes = recv(fd, &Buf[*pnBytesInBuf], 1, 0 );	// read 1 byte
		if (nBytes < 1)
		{
			if(nBytes == 0)
			{
				return -1;
			}

			// 10035 = Resource temporarily unavailable
			if (SOCK_ERR == 10035)
			{
				goto RCV_RETRY; 
			}
			if(nRcvRetryCount++ < 3 && SOCK_ERR != 10054 && SOCK_ERR != 10038)
			{
				gsched_yield();
				goto RCV_RETRY; 
			}
			// The client aborted, or timed out, or g_bPartialHeaderWait is OFF and 0 bytes was returned during the header read.
			return -1; 
		}
		
		// look for the "\r\n\r\n" that terminates the HTTP headers
		if (Buf[*pnBytesInBuf] == 10 && *pnBytesInBuf > 3)
		{
			if ( Buf[(*pnBytesInBuf)-1] == 13 &&  Buf[(*pnBytesInBuf)-2] == 10 &&  Buf[(*pnBytesInBuf)-3] == 13)
			{
				bReadingHTTPHeader = 0; // we got the complete header, stop reading header bytes.
				pContent = &Buf[(*pnBytesInBuf) + 1];
				*pContent = 0;
				if (ppContent)
					*ppContent = pContent;
				nHeaderLength = (pContent) - Buf;
			}
		}
		*pnBytesInBuf += nBytes; // nBytes is always 1

		// The user supplied timeout was met before entering the read of this HTTP header,
		// Between the first byte of header and the last, duration of time between each byte may not exceed N seconds.
		// Generally the entire header is on the socket buffer if any of it is there at all, but extreme conditions do exist.
		// if g_bPartialHeaderWait is true - wait for the remainder of the TCP data, otherwise the connection will get dropped.
		if (bReadingHTTPHeader && g_bPartialHeaderWait)
		{
			// todo: check the I/O stats of buffered size, this call could be avoided for each byte of header data.
			rslt = readableTimeout(fd, g_nPartialHeaderWaitSec, 0);
			if (rslt == -1 || rslt == 0)
			{
				return -1; // note: not "return rslt;" so the caller closes the connection and does not wait any longer.
			}
		}
	}
	
	if (bReadingHTTPHeader == 1) // if we bailed because we hit g_nMaxHTTPHeaderSize hang up the connection
	{
		return -1;
	}

	// get the content length
	int nCL = 0;
	char *pCL = stristr(Buf,"Content-Length:");
	if (pCL)
	{
		pCL += 15; // first byte of content length in the HTTP header
		nCL = atoi(pCL);
	}
	if(pnContentLen)
		*pnContentLen = nCL;

	// read the packet data content too
	if (nCL)
	{
		*nPktIndex = nCL;
		int nReadSize = nCL;
		if( nReadSize + *pnBytesInBuf > MAX_RAW_CHUNK)
		{
			nReadSize = MAX_RAW_CHUNK - *pnBytesInBuf;
		}
	

		// now read the remainder of the sockets buffered data
		rslt = readableTimeout(fd, nTimeOutSec, nTimeOutMicroSec);
		// failure during wait || time limit expired waiting for content data
		if (rslt == -1 || rslt == 0)
		{
			return rslt; 
		}
RETRY:
		
		nBytes = recv(fd, &Buf[*pnBytesInBuf], nReadSize, 0 );
		// now we have the entire header + nBytes of the content length - the total of which does not exceed MAX_RAW_CHUNK

		if (nBytes < 1)
		{
			// 10035 = Resource temporarily unavailable
			if (SOCK_ERR == 10035)
			{
				// gsched_yield();
				goto RETRY; 
			}
			if(nRcvRetryCount++ < 3 && SOCK_ERR != 10054 && SOCK_ERR != 10038) 
			{
				gsched_yield();
				goto RETRY; 
			}

			// The client aborted 
			return -1; 
		}
		*nPktIndex -= nBytes;
		*pnBytesInBuf += nBytes;
	}
	else
	{
		// we got the whole HTTP message that has no content length
		*nPktIndex = 0;
	}

	if (g_DataTrace) DataTrace("raw rx", Buf, *pnBytesInBuf);

	return 1; // process the data
}



// ReadPacket() notes:  
// --------------------
// When bIsHTTP is true then:
// Protocol is expected to be standard HTTP, bPacketSync should be false.
// The message can be in command form (GET / HTTP 1.1) or use a content 
// length that may span packets, as the packets returned by this fn max at MAX_DATA_CHUNK.
// ppPkt will always == Buf in this case, and nPktIndex will be bytes remaining if the
// HTTP content-length > MAX_DATA_CHUNK it will span one packet, it(nPktIndex) must be 
// supplied in subsequent calls to get the next data packet from the current HTTP message.

// When bPacketSync is true then:  --   bIsHTTP should be false
// Protocol =  #*DATA Packet 1 ... #*DATA Packet 2 ... #*DATA Packet 3
// where # is a 16 bit numeric byte count of the DATA packet that follows, 
// and * is a 16 bit flag field of data attributes (compression and encryption attributes)
// Buf may contain 1 packet, a partial packet, or multiple packets so (ppPkt) should be
// considered the result value for the size (nPktLen), and nPktIndex must be handed back for
// subsequent packet reads.

// When bPacketSync is false and bIsHTTP is false then:
// Data is read 'raw' with no transport markup expected - reading from a localhost application
// (POP3, SMTP, Telnet, or other).  Packets may or not retain the same length as the source 
// application.  This is used to transfer raw TCP data into a tunnel, or to bounce either
// raw or ciphered data across this machine, either way the packets will be re-aligned to
// the origin packet length at the client application(browser, telnet viewer, mail reader) 
// or at the tunnel exit point if the data is ciphered.
// ------------
// Returns: 0=No data nothing to do,   1=Packet has arrived - output params are set   or   -1 network/connection error
int ReadPacket(int fd, int nTimeOutSec, int nTimeOutMicro, int *pnBytesInBuf,char *Buf,char **ppPkt,int *nPktLen, int *nPktIndex, unsigned short *psAttrFlags, unsigned short sAttribsExpected, int bPacketSync, int bIsHTTP)
{
READ_PACKET_BEGIN:
	// if the beginning of the next packet was in the last recv()
	if (*nPktIndex > -1 && !bIsHTTP)
	{
		unsigned short theNumber = 0;				// storage on platform architecture byte boundry
		memcpy(&theNumber,&Buf[*nPktIndex],2);		// storage in network byte order moved to architecture byte boundry storage 
		unsigned short nNextPktSize = ntohs( theNumber );    // convert to host byte order

		memcpy(&theNumber,&Buf[(*nPktIndex) + sizeof(short)],2); // the same thing here.
		*psAttrFlags = ntohs( theNumber );



		if (nNextPktSize < 0)
		{
			GString strLog = "Invalid packet size (misconfigured?)";
			InfoLog(48,strLog);
			return -1;
		}

		if (*psAttrFlags != sAttribsExpected)
		{
			// within the second block of 16 bits recv()'d, If any of the reserved protocol 
			// bits (bits 5-16) are on or all bits(1-16) are off, this is raw(non-packeted) data.
			// This machine is expecting packeted data.  
			if ( ( *psAttrFlags & ATTRIB_UNUSED_BIT9 ) || ( *psAttrFlags & ATTRIB_UNUSED_BIT10) ||
				 ( *psAttrFlags & ATTRIB_UNUSED_BIT11) || ( *psAttrFlags & ATTRIB_UNUSED_BIT12) ||
				 ( *psAttrFlags & ATTRIB_UNUSED_BIT13) || ( *psAttrFlags & ATTRIB_UNUSED_BIT14) ||
				 ( *psAttrFlags & ATTRIB_UNUSED_BIT15) || ( *psAttrFlags & ATTRIB_UNUSED_BIT16) ||
				 *psAttrFlags == 0) //there is always > 0 attributes when synchronized
			{
				// 17=Protocol error. If you are attempting to proxy or 'bounce' TCP, enable RawPacketProxy=yes in the correct subsection of the config file.
				GString strLog;
				strLog.LoadResource(g_pzCoreErrorSection, 17);
				InfoLog(49,strLog);
				return -1;
			}
		}

		int nBufBytesNeeded = *nPktIndex + nNextPktSize + (2 * sizeof(short));
		// If the next packet is the complete remainder of the recv() buffer
		if (nBufBytesNeeded == *pnBytesInBuf)
		{
			*ppPkt = &Buf[*nPktIndex + (2 * sizeof(short))]; // 4 bytes of header, then data
			*nPktLen = nNextPktSize;
			*nPktIndex = -1; // last packet
			return 1;
		}
		// if the next packet is in the recv() buffer with more data after it
		else if (nBufBytesNeeded < *pnBytesInBuf)
		{
			*ppPkt = &Buf[*nPktIndex + (2 * sizeof(short))]; // 4 bytes of header, then data
			*nPktLen = nNextPktSize;
			*nPktIndex = nBufBytesNeeded; // index to the first byte of the next packet
			return 1;
		}
		// otherwise, we need to recv() the remainder of this packet
		else 
		{
			int rslt = readableTimeout(fd, nTimeOutSec, nTimeOutMicro);
			if ( rslt > 0)
			{
				// slide the data to the front of the buffer to make room for this packet
				if (*nPktIndex > 0)
				{
					memmove(Buf,&Buf[*nPktIndex],*pnBytesInBuf - *nPktIndex); // ANSI <string.h>
					*pnBytesInBuf = *pnBytesInBuf - *nPktIndex;
					*nPktIndex = 0;
				}

				// test the buffer for necessary free space now
				if (MAX_DATA_CHUNK - *pnBytesInBuf < 1)
				{
					// 18=ReadPacket buffer overrun.
					GString strLog;
					strLog.LoadResource(g_pzCoreErrorSection, 18);
					InfoLog(50,strLog);

					return -1;
				}

				// read data from the server
				int nBytes = recv(fd, &Buf[*pnBytesInBuf], MAX_DATA_CHUNK - *pnBytesInBuf, 0 );
				if (nBytes > 0)
				{
					char *pNewDataBegin = &Buf[*pnBytesInBuf];
					*pnBytesInBuf = *pnBytesInBuf + nBytes;
					if (g_DataTrace) DataTrace("raw rx", pNewDataBegin, nBytes);
					goto READ_PACKET_BEGIN;
				}
				else // nBytes = -1 || 0
				{
					// recv() failed (connection closed by server)
					if(g_TraceConnection)
					{
						InfoLog(51,"111 ReadPacket::recv() failed (connection closed by server)");
					}
					return -1;
				}
			}
			else if (rslt == -1)
			{
				// failure during wait (connection closed by server)
				if(g_TraceConnection)
				{
					InfoLog(52,"ReadPacket::failure during wait");
				}
				return -1;
			}
			else 
			{
				// rslt = 0, the time limit expired.
			}
			
			// no data or only a partial packet available - nothing to do.
			return 0;
		}
	}
	else
	{
		int rslt = readableTimeout(fd, nTimeOutSec, nTimeOutMicro );
		if ( rslt > 0)
		{
			if (bIsHTTP)
			{
				// Buf will contain the HTTP packet
				int nBytes  = 0;
				*pnBytesInBuf = nBytes; // no preread bytes exist in the destination buffer
				*ppPkt = Buf;
				int nRet = ReadPacketHTTP(fd, pnBytesInBuf, Buf, (unsigned int *)nPktIndex, 0, 0, nTimeOutSec, nTimeOutMicro);
				*nPktLen = *pnBytesInBuf; // HTTP reads: nPktLen always == nBytesInBuf
				return nRet;
			}


			// read data from the server (tunnel server, or app server depending on bPacketSync)
			int nMaxRead = (bPacketSync) ? MAX_DATA_CHUNK : MAX_RAW_CHUNK;

			// read in some data, 4 bytes at a minimum(if bPacketSync) to obtain attributes(cipher/zip...)
			int nMinRead = (bPacketSync) ? 4 : 1;
			int nRetryOnErrCount = 0;
			int nBufi = 0;
RETRY_ON_ERR: 
			int nBytes = recv(fd, &Buf[nBufi], nMaxRead, 0 );
			if (nBytes > 0)
			{
				if (nBytes < nMinRead) // if we got 1,2 or 3 bytes but we need a min of 4
				{
					// this will most likely never happen, but in theory it could
					nMinRead -= nBytes;
					nBufi += nBytes;
					*pnBytesInBuf += nBytes;
					readableTimeout(fd, nTimeOutSec, nTimeOutMicro );
					goto RETRY_ON_ERR;
				}
				*pnBytesInBuf = nBytes;
			}
			else if (nBytes == -1)
			{
				// 10035 = Resource temporarily unavailable
				if (SOCK_ERR == 10035)
				{
					// gsched_yield();
					goto RETRY_ON_ERR; 
				}
				if (nRetryOnErrCount++ > 2 || SOCK_ERR == 10054 || SOCK_ERR == 10038 )
				{
					// recv() failed or session complete (connection closed by server)
					return -1;
				}
				else
				{
					PortableSleep(0,100000);
					gsched_yield();
					goto RETRY_ON_ERR;
				}
			}
			else if (nBytes == 0) 
			{
				//InfoLog("ReadPacket returns 0. Drop connection.");
				return -1;
			}


			// if ciphered/zipped proxy, we may have > 1 packet in Buf, or < 1 full packet - go remedy that
			if (bPacketSync) 
			{
				// a packet from the tunnel server / tunnel client has the protocol headers
				*nPktIndex = 0;
				goto READ_PACKET_BEGIN;
			}
			else // raw data proxy - we'll take whatever we got and use that.  It's usually
				 // a raw packet from an application server (pop3 server, telnet server etc)
			{
				if (g_DataTrace) DataTrace("raw rx", Buf, nBytes);
				*ppPkt = Buf;
				*nPktLen = *pnBytesInBuf;
				*psAttrFlags = 0;	// no attributes - raw data
				return 1; // process the data now
			}
		}
		else if (rslt == -1)
		{
			// failure during wait (connection closed by server)
			return -1;
		}
		
		// no data or only a partial packet available - nothing to do.
		return 0;
	}
}

 
unsigned short BuildAttributesHeader(const char *pzCfgSection, int nProtocol, int nProtocolAction, int *bReadHTTP, int nDirection)
{
	unsigned short sRet = 0;
	*bReadHTTP = 0;
	if (nProtocol == 3)
	{
		return sRet; // 0 is a raw proxy
	}
	if (nProtocol == 9)
	{
		sRet |= ATTRIB_CRYPTED_2FISH;
		sRet |= ATTRIB_COMPRESSED_GZIP;
		return sRet; 
	}


	if ( GetProfile().GetBoolean(pzCfgSection,"UseHTTP",false) )
	{
		*bReadHTTP = 1;
	}

	if ( GetProfile().GetBoolean(pzCfgSection,"CipherPass",false) )
	{
		sRet |= ATTRIB_CRYPTED_2FISH;
	}
	if ( GetProfile().GetBoolean(pzCfgSection,"CompressEnabled",false) )
	{
		sRet |= ATTRIB_COMPRESSED_GZIP;
	}
	if ( GetProfile().GetBoolean(pzCfgSection,"PacketSyncEnabled",false) )
	{
		sRet |= ATTRIB_SYNCRONIZED;
	}

	// if no attributes were specified and a raw(non-synchronized) was not specified.
	if (!sRet && !GetProfile().GetBoolean(pzCfgSection,"RawPacketProxy",false))
	{
		sRet |= ATTRIB_SYNCRONIZED;
	}
	
	// remove the ATTRIB_SYNCRONIZED for raw HTTP proxy for messaging
	if (nProtocol == 1)
	{
		sRet = 0;
	}
	
	// Dynamic HTTP Proxy
	if (nProtocol == 11)
	{
		*bReadHTTP = 1;
		sRet = 0;
	}

	// OpenSurf
	if (nProtocol == 12)
	{
		if (nDirection == 1)  // The machine that connected to this machine is sending through this machine
		{
			*bReadHTTP = 1;
			sRet |= ATTRIB_CRYPTED_2FISH;
		}
		else // the machine connected to by this machine is sending back to the initiating machine through this machine
		{
			sRet = 0; 
		}
	}
	return sRet;
}





int	DoConnect(const char *pzConnectTo, int nPort, int nFromID, int nToID)
{
	if (!pzConnectTo || !pzConnectTo[0])
	{
		if(g_TraceConnection)
		{
			GString strInfo;
			strInfo.Format("*******FAILED Outbound[%d:(null)]---(tid:%d<->%d)", (int)nPort,(int)nFromID, (int)nToID);
			InfoLog(57,strInfo);
		}
		return -1;
	}
	
	// if the first byte of pzConnectTo is a '~' - make an inbound-connect.  
	// wait for 2 minutes for the named connection to be posted in (connection opened by the destination).
	if (pzConnectTo[0] == '~')
	{
		GString strInternalConnect(&pzConnectTo[1]);
		strInternalConnect.MakeUpper();
		// returns -1 or the socket handle
		return g_SwitchBoard.ConnectToServer( strInternalConnect );
	}

	if(g_TraceConnection)
	{
		GString strInfo;
		strInfo.Format("Outbound[%d:%s]---(tid:%d<->%d)", (int)nPort,pzConnectTo, (int)nFromID, (int)nToID);
		InfoLog(58,strInfo);
	}
	int fd = -1;

	// else make an outbound-connect....
	struct sockaddr_in their_addr; 
	their_addr.sin_family = AF_INET;
	their_addr.sin_port = htons(nPort);
	their_addr.sin_addr.s_addr = inet_addr (pzConnectTo);
	if (their_addr.sin_addr.s_addr == -1)
	{
		// resolve a DNS server name if inet_addr() came up empty.
		struct hostent *pHE = (struct hostent *)gethostbyname(pzConnectTo);
		if (pHE == 0)
		{ 
			// 27=Info:Failed to resolve[%s].
//			GString strInfo;
//			strInfo.Format("Info:Failed to resolve[%s]", pzConnectTo);
//			InfoLog(59,strInfo);

			// we failed to resolve, not due to DNS error but due to the
			// inability to obtain a network connection, likely on a mobile
			// device that is 'sleeping'
			goto NO_CONNECT;
		}
		memcpy((char *)&(their_addr.sin_addr), pHE->h_addr,pHE->h_length); 
	}
	memset(&(their_addr.sin_zero),0, 8);//zero the rest of the (unused) struct

	if ((fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) 
	{	
		if(g_TraceConnection)
		{
			GString strInfo;
			strInfo.Format("*******FAILED: Outbound[%d:%s]---(tid:%d<->%d)  NO SOCKET", (int)nPort,pzConnectTo, (int)nFromID, (int)nToID);
			InfoLog(60,strInfo);
		}
		goto NO_CONNECT;
	}
	
	if (connect(fd, (struct sockaddr *)&their_addr, sizeof(struct sockaddr)) != 0)
	{
		if(g_TraceConnection)
		{
			GString strInfo;
			strInfo.Format("*******FAILED: Outbound[%d:%s]---(tid:%d<->%d)  NO CONNECT", (int)nPort,pzConnectTo, (int)nFromID, (int)nToID);
			InfoLog(61,strInfo);
		}

		PortableClose(fd,"Core6");
		fd = -1;

		goto NO_CONNECT;
	}

	AddDebugSocket(fd);

	if (g_TraceSocketHandles)
	{
		GString strTrace("Connect Socket:");
		strTrace << fd << "   on port:" << nPort << "  to:" << pzConnectTo << "  " << nFromID << ":" << nToID;
		InfoLog(888,strTrace);
	}

NO_CONNECT:
	return fd;
}






// Convert															TO
// -------------------------------------------------------------------------------------------------------------------
// GET http://www.Domain.com/Index.html HTTP/1.0					GET Index.html HTTP/1.0
// GET http://www.Domain.com/Index HTTP/1.1							GET Index HTTP/1.1
// GET http://www.Domain.com/images/pi_startc.jpg HTTP/1.1			GET /images/pic.jpg HTTP/1.1
// -------------------------------------------------------------------------------------------------------------------
// subsequent commands on Keep-Alive connections copy pIn to pOut as the data may look like this:
// GET /menu.js HTTP/1.0
// GET /images/pic.jpg HTTP/1.1
// GET \menu.js HTTP/7.0
// -------------------------------------------------------------------------------------------------------------------
int ConvertHTTPProxy(char *pIn, int nInLen, char *pOut, GString &strPxyConnectTo, int &nConvertedLen, int *pbIsCONNECT)
{
	//	note: pIn must not == pOut, memory may not overlap   

	if ( memcmp(pIn,"CONNECT",7) == 0 || memcmp(pIn,"connect",7) == 0) 
	{
		GString strPxyToPort;
		char *pPort = strpbrk(pIn+8,":");
		if (pPort)
		{
			strPxyToPort.SetFromUpTo(pPort+1, " ");
			if ( strPxyToPort.Compare("443") )
				return 0; // note: limit to only allow port 443 blocks potential attack
			strPxyConnectTo = "443@";
		}
		strPxyConnectTo.AppendFromUpTo(pIn+8, ":");
		nConvertedLen = 0;
		
		if (pbIsCONNECT)
			*pbIsCONNECT = 1;

		return 1; // Proxy conversion success
	}
	if (pbIsCONNECT)
		*pbIsCONNECT = 0;

	char *pSpace = strpbrk(pIn," "); // HTTP verb left of space
	if (pSpace)
	{
		if (pSpace[1] == '/' || pSpace[1] == '\\')
		{
			memcpy(pOut,pIn,nInLen);
			nConvertedLen = nInLen;
			return 1; // Proxy conversion success
		}
		else if ( memcmp(pSpace+1,"http://",7) == 0  ||  memcmp(pSpace+1,"HTTP://",7) == 0)
		{
			nConvertedLen = pSpace-pIn+1;
			memcpy(pOut,pIn,nConvertedLen);  // "GET " or "POST " moved to destination


			char *pSlash = strpbrk(pSpace+8,"/"); // first / after "http://" (End of server name)
			if (pSlash)
			{
				int nCpySize = pSlash-pIn;//  nCpySize = size of  "GET http://www.Server.com/"
				memcpy(&pOut[nConvertedLen],pSlash,nInLen-nCpySize);
				nConvertedLen += nInLen-nCpySize;
				strPxyConnectTo.WriteOn(pSpace+8,pSlash - (pSpace+8)); // copy the server name "www.Server.com"
				
				char *pColon = strpbrk(strPxyConnectTo.Buf(),":");
				if (pColon)
				{
					// if the ConnectTo is 1.2.3.4:81 convert it to 81@1.2.3.4
					*pColon	= 0;
					GString strTemp;
					strTemp << (pColon + 1) << '@' << strPxyConnectTo.Buf();
					strPxyConnectTo = strTemp;
				}
				return 1; // Proxy conversion success
			}
		}
		else
		{
			// todo: investigate how this can happen.
			InfoLog(1,"********* HTTPProxy FAILURE");
		}
	}
	return 0; // no proxy conversion
}


/*
This structure is stored in BaseInterface.h for maintenance reasons
because every language driver needs the definition also.
  
struct CBDataStruct
{
	int nState;			    // currently unused - may track transactional state
	int nReserved;			// currently unused - the size of this struct cannot change or it will break lots of code so maybe this will be needed someday
	void *pstrDest;		    // a GString * - may be used directly if you link with the same foundation the invoking process is built with.
	CBfn fn;				// 2nd level handler function on the calling process, 1st level is in the driver.
	char *piBuf;			// set by the invoking process, that the called method may indicate processing instructions on the results, assume 1024 bytes
	unsigned char *wkBuf;   // set by the invoking process, a region of memory as a working buffer for the called method, size is set by invoker, nWKBufferSize bytes(normally 64000).
	unsigned char *wkBuf2;  // set by the invoking process, a region of memory as a working buffer for the called method, size is set by invoker, nWKBufferSize bytes(normally 64000).
	void *pCBArg;		    // a void * user defined by the SetDriverOption("callbackArg");
	void *pDriverInterface; // pointer to instance of driver interface (for example, a CStdCallInterface, CJavaInterface, CPerlInterface)
	int nWKBufferSize;		// size of memory allocated in "wkBuf"

	union UArg
	{
		__int64 Int64;
		__int64 *PInt64;
		int Int;
		int *PInt;
		const char *CChar;
		char *Char;
		void *Void;
	};
	UArg arg[7];
};
*/







// this struct can be modified without breaking backward compatibility 
// IF any additions are added at the bottom of the structure
struct DriverCallBackArg
{
	// always valid
	int nProtocol; // 1 for HTTP, 2 for Xfer, 3 for Messaging
	int fd;
	int tid;
	int nCompleteSize;
	int nObtainedCount;
	GString strUser;
	unsigned short sDataFlags;

	int *pnKeepAlive;

	// only valid for Xfer
	CipherData *en;
	CipherData *de;
	void *XCmd; // an XferCommand *

	int nBuf1Size;
	GString strBoundary;
	GString strArgs;
};

//
// This is how plugins interface with the kernel functions through the driver that loads the plugin.
// Xfer plugins and HTTP Server plugins and Messaging plugins use this same callback handler.
// 
void DriverCallBack(const char *pzCmd, void *pCBArg)
{
	GString strOption(pzCmd);
	CBDataStruct *pcb = (CBDataStruct *)pCBArg;
	DriverCallBackArg *sysState = (DriverCallBackArg *)pcb->pCBArg;

	if (strOption.CompareNoCase("msg") == 0)
	{
		if (sysState->nProtocol == 3)
		{
			GStringList lstArgs("/",sysState->strArgs);
			GString *p = lstArgs.GetStrAt(pcb->arg[0].Int);
			strncpy(pcb->arg[1].Char, p->Buf(), pcb->arg[2].Int  );
			pcb->arg[1].Char[pcb->arg[2].Int - 1] = 0;
		}
	}
	else if (strOption.CompareNoCase("usr") == 0)
	{
		strncpy(pcb->arg[0].Char, sysState->strUser.Buf(), pcb->arg[1].Int  );
		pcb->arg[0].Char[pcb->arg[1].Int - 1] = 0;
	}
	else if (strOption.CompareNoCase("siz") == 0)  // get full size of data to expect
	{
		pcb->arg[0].Int = sysState->nCompleteSize;
	}
	else if (strOption.CompareNoCase("tot") == 0)  // get full size of data to expect
	{
		pcb->arg[0].Int = sysState->nObtainedCount;
	}
	else if (strOption.CompareNoCase("xtx") == 0)  // Transmit
	{
		if (sysState->nProtocol == 1 || sysState->nProtocol == 3)
		{
			// note: the caller MUST correctly setup the HTTP Headers in the first block sent back 
			if ( nonblocksend(sysState->fd,pcb->arg[0].CChar,pcb->arg[1].Int) == pcb->arg[1].Int)
			{
				pcb->arg[0].Int = 1;// success
			}
			else
			{
				pcb->arg[0].Int = 0;// fail
			}
		}
		else if (sysState->nProtocol == 2)
		{
		#ifdef ___XFER
			// note: this comment ONLY applies to Xfer protocol, sending from the system buffers for HTTP is always safe and always recommended.
			//
			// pcb->arg[0].CChar should be memory other than pcb->wkBuf, just to be safe
			// pcb->arg[0].CChar must never be pcb->wkBuf2
			//
			// Do NOT place data to send in the System Buffer, normally it will send fine from pcb->wkBuf, but if it spans multiple
			// packets(aka > 8192) AND the compression hits worst case scenarios(pre compressed data expands a little), then 
			// the beginning of the second block will be stepped on by end of the first, this is all our memory
			// so no error will occur, but your data will be trashed when you receive it.  Note that it will 
			// successfully decrypt and decompress on the client side because it's getting exactly what the server sent
			// but that's not what the developer of the plugin intended, so be safe and use your own memory unless you
			// know for sure that this situation could never occur in your application.
			GString strAttached(pcb->arg[0].CChar, pcb->arg[1].Int, pcb->arg[1].Int, 0); // Uses the Attach Constructor

			// note: The caller MUST correctly setup the header in the first call to "xtx" on every connection
			// each subsequent call, if any, is a continuation of the data stream initiated in the first call
			// so the beginning of the first packet must look like this: "OK nnnnnnn:DDDDD"
			// where nnnn is an alpha numeric length of the data(DDDDD) that begins after the ':'
			LocalData LD((XferCommand *)sysState->XCmd);
			LD.fd = sysState->fd;
			LD.buf1 = pcb->wkBuf;
			LD.buf2 = pcb->wkBuf2;
			LD.sDataFlags = sysState->sDataFlags;
			LD.en = sysState->en;
			pcb->arg[0].Int = LD.XCmd->SendXferResults(&LD, &strAttached);
		#endif // #ifdef ___XFER
		}

		
		// add the Process Instruction if it's not already added
		if (stristr(pcb->piBuf,"DataSent") == 0)
			strcpy(pcb->piBuf,"DataSent");
	
	}
	else if (strOption.CompareNoCase("xrx") == 0)  // Xfer Receive
	{
		if (sysState->nProtocol == 1 || sysState->nProtocol == 3)
		{
			if (pcb->arg[0].Int == -1)
			{
				pcb->arg[0].Int = nonblockrecvAny(sysState->fd,pcb->arg[1].Char,pcb->arg[2].Int, pcb->arg[3].Int, pcb->arg[4].Int);
			}
			else
			{
				pcb->arg[0].Int = nonblockrecvAny(sysState->fd,(char *)pcb->wkBuf,sysState->nBuf1Size, 60, 0);
			}
			if (pcb->arg[0].Int > 0)
			{
				sysState->nObtainedCount += pcb->arg[0].Int;
				pcb->arg[1].Int = pcb->arg[0].Int; // move the size into arg 1
				pcb->arg[0].Int = 1; // indicate success 

				if (sysState->nProtocol == 3)
				{
					//int nToWrite = pcb->arg[1].Int;
					if (sysState->nCompleteSize - sysState->nObtainedCount < 256)
					{
						// walk back from the end and stop at a null or 256
						int nEndIndex = pcb->arg[1].Int - 1;
						while(pcb->wkBuf[nEndIndex] && pcb->arg[1].Int - nEndIndex < 256)
							nEndIndex--;	

						char *pEnd = strstr(&pcb->wkBuf[nEndIndex+1],sysState->strBoundary.Buf());
						if (pEnd)
						{
							pcb->arg[1].Int = (pEnd - pcb->wkBuf) - 4; // four bytes behind the boundary marker
							sysState->nCompleteSize = sysState->nObtainedCount;
						}
					}
				}

			}
		}
		else if (sysState->nProtocol == 2)
		{
		#ifdef ___XFER
			// exec result into [0], size result into [1]
			pcb->arg[0].Int = DoXferPacketRead(sysState->fd, (char *)pcb->wkBuf2, (char *)pcb->wkBuf, &pcb->arg[1].Int, sysState->de, 60);
			if ( pcb->arg[0].Int == 1 )
			{
				sysState->nObtainedCount += pcb->arg[1].Int;
			}
		#endif // ___XFER
		}
	}
	else if (strOption.CompareNoCase("pre") == 0)  
	{
		((GString *)pcb->pstrDest)->SetPreAlloc(pcb->arg[0].Int);
	}
	else if (strOption.CompareNoCase("cat") == 0)  // concatenate data "to send"
	{
		if (pcb->arg[0].CChar)
			*((GString *)pcb->pstrDest) << pcb->arg[0].CChar;
	}
	else if (strOption.CompareNoCase("wrtSnd") == 0) 
	{
		// send it straight out of the plugin-memory
		// assumes user management of HTTP headers.
		pcb->arg[2].Int = nonblocksend(sysState->fd,pcb->arg[1].CChar,pcb->arg[0].Int);
	}
	else if (strOption.CompareNoCase("wrt") == 0)
	{
		if (pcb->arg[1].CChar)
			((GString *)pcb->pstrDest)->write(pcb->arg[1].CChar,pcb->arg[0].Int);
	}
	else if (strOption.CompareNoCase("ip") == 0)
	{
		struct sockaddr_in other_addr;
		int nSize = sizeof(sockaddr_in);
		getpeername(sysState->fd,(struct sockaddr *) &other_addr, (socklen_t *)&nSize);
		strcpy(pcb->arg[0].Char,inet_ntoa(other_addr.sin_addr));
	}
	else if (strOption.CompareNoCase("tid") == 0)
	{
		pcb->arg[0].Int = sysState->tid;
	}
	else if (strOption.CompareNoCase("keepalive") == 0)  // return connection keep alive preference
	{
		pcb->arg[0].PInt = sysState->pnKeepAlive;
	}
	else if (strOption.CompareNoCase("dis") == 0)
	{
		PortableClose(sysState->fd,"Core7");
	}
	else if (strOption.CompareNoCase("fd") == 0)  // return socket file descriptor
	{
		pcb->arg[0].Int = sysState->fd;
	}
	else if (strOption.CompareNoCase("log") == 0)  // write to system log file
	{
		InfoLog(63,pcb->arg[0].CChar);
	}
}

int Plugin(ThreadData *td, char *sockBuffer, int nBytes, int nContentLen, char *pContent, int *pnKeepAlive, char *workBuffer, int *nTotalProxiedBytes, unsigned int nBytesRemaining)
{
	// Design note: This function is designed to assume that the sockBuffer value(the first read off the TCP buffers on the HTTP port)  
	// does NOT map to any plugin, it must avoid as much logic execution as possible until a plugin map has been matched
	// because this "filter" will execute for every HTTP request - we need to add as little overhead as possible.

	// This function has two main logic branches, one for POST and one for GET, as they each handle the data somewhat differently

	GListIterator itPlgIn( &g_lstActivePlugins ); // a list(plugins) of lists(each plugins arguments), they are all very small
	while(itPlgIn())
	{
		GStringIterator itPlugInArgs( (GStringList *)itPlgIn++ );		// lists are pre-constructed and pre-parsed to reduce overhead
		GString strPlugInPath( !itPlugInArgs() ? 0 : itPlugInArgs++);
		

		// check for a wildcard method match (user assigned method name by operator permission only)
		GString strMethod;
		const char *pzMethod = 0;
		if ( strPlugInPath.GetAt(strPlugInPath.Length()-1) == '*' )  // test the one bytes that indicates a wildcard
		{
			char *pSp = strpbrk(sockBuffer," ");
			if (pSp)
			{
				if ( strPlugInPath.CompareNoCase(pSp+1,strPlugInPath.Length()-1) == 0)
				{
					char *pTerm = strpbrk(pSp+strPlugInPath.Length(),"/\\ \r\n");
					if (!pTerm) pTerm = pSp+1;
					strMethod.WriteOn(pSp+2,pTerm - (pSp+2));
					pzMethod = strMethod;
					strPlugInPath   = "/";				// (this here code....
					strPlugInPath  += strMethod;		// retains path match length...
					strPlugInPath  += "/";				// so that arguments are parsed out correctly)
				}
			}
		}
		
		// this test is executed once for each configured plugin
		if ( memcmp(sockBuffer,"POST",4) == 0 && (pzMethod || strPlugInPath.CompareNoCase(&sockBuffer[5],strPlugInPath.Length()) == 0 ) )
		{
			// ok we got a match, now none of this code is overhead to standard HTTP transactions

			const char *pzLanguage = ( !itPlugInArgs() ) ?  0 : itPlugInArgs++;
			const char *pzComponent = ( !itPlugInArgs() ) ?  0 : itPlugInArgs++;
			const char *pzInterface = ( !itPlugInArgs() ) ?  0 : itPlugInArgs++;
			if (!pzMethod) pzMethod = ( !itPlugInArgs() ) ?  0 : itPlugInArgs++;


			// determine (next transaction) connection attributes
			// default to "Keep Alive, unless directed to close"
			GString strConnectionType("Keep-Alive");
			int nIsKeepAlive = 1;
			char *pCon = strstr((char *)sockBuffer,"Connection:");
			if (pCon)
			{
				char *pEnd = strpbrk(pCon + 12, " \r\n");
				if (pEnd)
				{
					char chSave = *pEnd;
					*pEnd = 0;
					strConnectionType = pCon + 12;
					*pEnd = chSave;
					if (strConnectionType.CompareNoCase("close") == 0)
					{
						nIsKeepAlive = 0;
					}
				}
			}
			if (!nIsKeepAlive) // if the requestor did not want this connection to be kept alive 
			{
				*pnKeepAlive = 0;
			}
			else
			{
				(*pnKeepAlive)--;
			}
			td->m_pnKeepAlive = pnKeepAlive;


			InterfaceInstance *pII = GetInterfaceInstance(pzLanguage);
			if (!pII)
			{
				GString strError;
				strError << pzLanguage << " is not configured correctly or is not a supported component type on this machine.";
				InfoLog(64,strError);
				*pnKeepAlive = 0;
				return 0;
			}

			const char *pDataStart = &sockBuffer[5 + strPlugInPath.Length()];


			// get the argument names, if arg1 is "Header" and arg2 is "Content", then don't parse arguments.
			// in that case, parsing is the responsibility of the plug-in.
			int bParse = 1;
			GStringList *pL = pII->GetMethodParams(pzComponent, pzInterface, pzMethod);
			int nReqArgCount = pL->Size();
			GStringIterator itArgNames(pL);
			if (itArgNames())
			{
				char *p = stristr(itArgNames++,"Header");
				if (p && itArgNames())
				{
					p = stristr(itArgNames++,"Content");
					if (p && !itArgNames())
						bParse = 0;
				}
			}
			itArgNames.reset();

			

			if (nBytesRemaining && ((nBytes + nBytesRemaining) < MAX_RAW_CHUNK ) && bParse )
			{
				// we need the rest of the POST data before we can parse out the arguments required for this method invocation
				if ( (unsigned int)nonblockrecv(td->sockfd,&sockBuffer[nBytes],nBytesRemaining) != nBytesRemaining)
				{
					*pnKeepAlive = 0; 
					return 0;
				}
				nBytesRemaining = 0;
				nBytes += nBytesRemaining;
				sockBuffer[nBytes] = 0;
			}
			else if ( nBytesRemaining && (nBytes < MAX_RAW_CHUNK ) && bParse )
			{
				// we can't fit the entire POST into the system buffer but we can hold more than we have, we'll get all we can
				// then see if we can parse out the arguments from what we have, this allows the plugin to map as much as possible
				// and callback for the remainder of the data stream that follows the mapped arguments
				if ( nonblockrecv(td->sockfd,&sockBuffer[nBytes],MAX_RAW_CHUNK - nBytes) != MAX_RAW_CHUNK - nBytes)
				{
					*pnKeepAlive = 0; 
					return 0;
				}
				nBytesRemaining -= (MAX_RAW_CHUNK - nBytes);
				nBytes += (MAX_RAW_CHUNK - nBytes);
				sockBuffer[nBytes] = 0;
			}


			// prepare the information available to the driver callback
			DriverCallBackArg prd;
			prd.XCmd = 0;
			prd.de = 0; // cipher N/A on anonymous HTTP
			prd.en = 0;
			prd.fd = td->sockfd;
			prd.nProtocol = 1;
			prd.tid = td->nThreadID;
			prd.strUser = "Anonymous HTTP";
			prd.pnKeepAlive = td->m_pnKeepAlive;
			prd.nCompleteSize = nContentLen;
			prd.nObtainedCount = nContentLen - nBytesRemaining;
			prd.sDataFlags = 0; // N/A
			prd.nBuf1Size = MAX_DATA_CHUNK;
			

			// setup the driver options
			char pzPIBuf[1024];
			pzPIBuf[0] = 0;
			pII->SetOption("piBuf",pzPIBuf);
			pII->SetOption("callback",(void *)DriverCallBack);
			pII->SetOption("callbackArg",&prd );
			pII->SetOption("wkBuf",sockBuffer);
			pII->SetOption("wkBuf2",workBuffer);
			int n = MAX_DATA_CHUNK;
			pII->SetOption("wkBufSize",(void *)n);
			if ( GetProfile().GetBool("HTTP","UnloadPlugins",false) )
			{
				int yes = 1;
				pII->SetOption("unload",&yes); 
			}


			// get the POSTed content type 
			GString strContentType;
			char *pCT = stristr(sockBuffer,"Content-Type:");
			if (pCT)
			{
				pCT += 13;
				char *pEnd = strpbrk(pCT, "\r\n");
				char ch = *pEnd;
				*pEnd = 0;
				strContentType = pCT;
				strContentType.TrimLeftWS(); // remove leading space if there is one.
				*pEnd = ch;
			}
			// see if this content type needs to be decoded
			int bDoDecode = 0;
			if ( strContentType.CompareNoCase("application/x-www-form-urlencoded") == 0) 
			{
				bDoDecode = 1;
			}
				

			int nHeadLen = pContent - pDataStart;



			


			//
			// prepare arguments for Invoke() call
			//
			GString strArgSizes;




			// special case, pass the HTTP headers as Arg1, and the content as Arg2
			if (!bParse /*&& (nReqArgCount == 2) */) 
			{
				if (bDoDecode)
				{
					nContentLen = unEscapeData(pContent,pContent);
					pContent[nContentLen] = 0; // terminate what we just unescaped
				}
				strArgSizes << nHeadLen << "|" << nContentLen;

			}
			// parse out the HTML form data - map to method arguments
			else if (bParse)
			{
				int nCurrentIndex = 0;
				pContent[nContentLen] = 0;
				GStringList lstHTMLArgs("&",pContent); // pContent = "One=1sValue&Two=2sValue&Three=3sValue&B1=Submit" where One, Two, and Three are the input control names in the HTML
				GStringIterator itHTMLArgs(&lstHTMLArgs);
				
				GString strHTMLArgName;
				int nNewArgLen;
				while(itArgNames()) // loop through the required arguments
				{
					const char *pRegArg = itArgNames++;
					int bFound = 0;
					while(itHTMLArgs()) // loop through the supplied arguments
					{
						const char *pzHTMLArg = itHTMLArgs++;
						char *pzSep = (char *)strpbrk(pzHTMLArg, "=");
						if (pzSep)
						{
							*pzSep = 0;
							strHTMLArgName = pzHTMLArg;
							if (strHTMLArgName.CompareNoCase(pRegArg) == 0)
							{
								*pzSep = '=';
								pzSep++;
								nNewArgLen = unEscapeData(pzSep,&sockBuffer[nCurrentIndex]);
								nCurrentIndex += nNewArgLen + 1; // size plus NULL
								if (strArgSizes.Length())
									strArgSizes << "|";
								strArgSizes << (nNewArgLen+1);
								bFound = 1;
								break;
							}
							else
							{
								*pzSep = '=';
							}
						}
					}
					if (!bFound)
					{
						if (nBytesRemaining)
						{
							// Maybe we didn't find this argument because it was after an argument that came in much larger than expected.
							// In this case, either the data posted is invalid so dropping the connection is the proper thing to do.
							// It's not impossible for a name, email, phone or the like to come in at 50,000 bytes.
							// If this is legitimate data the plugin needs to handle this type of transaction by mapping directly to the 
							// HTTP Header and Content and using the argument parsing utilities to extract the arguments.  
							GString strError;
							
							strError << pzComponent << "::" << pzInterface << "::" << pzMethod << " dropped connection posting:" << nContentLen << " bytes.";
							InfoLog(64,strError);
						}
						else // this is an error in the HTML or the mapping between the plugin and the HTML is out of date
						{
							GString strInfo;
							strInfo << "ERROR: [" << pRegArg << "]" << "not found for call:" << pzComponent << "::" << pzInterface << "::" << pzMethod;
							InfoLog(66,strInfo);
						}
						*pnKeepAlive = 0; 
						return 0;
					}
					itHTMLArgs.reset();
				}
				pDataStart = sockBuffer;
			}
			else
			{
				GString strInfo;
				strInfo << "ERROR: Arg count must be 2 when using 'HTTPHeader/Content' but you have " << nReqArgCount << "---"  << pzComponent << "::" << pzInterface << "::" << pzMethod;
				InfoLog(67,strInfo);
			}

		

			int nOutResultSize = 0;
			const char *pzResults = pII->InvokeEx( pzComponent, pzInterface, pzMethod, pDataStart,strArgSizes,&nOutResultSize);


			if (stristr(pzPIBuf,"DataSent") != 0)
			{
				// the plugin sent the data directly
				return 1; // HTTP request handled.
			}
			if (nOutResultSize < 1 || stristr(pzPIBuf,"NoKeepAlive") != 0)
			{
				*pnKeepAlive = 0;
				return 1; // HTTP request handled, no results from user created handler.
			}

			// currently this is the only processing instruction available, in the future this will be a pipe separated list.
			// todo: add processing instruction support "NoKeepAlive"
			if (stristr(pzPIBuf,"HasHeaders") != 0)
			{
				if (nOutResultSize)
					if (nonblocksend(td->sockfd,pzResults,nOutResultSize) == nOutResultSize)
						*nTotalProxiedBytes += nOutResultSize;
			}
			else
			{
				// todo: support multiple content types by setting "Ext", currently NULL is "text/html"
				BuildHTTPHeader(sockBuffer, 0/*FileName*/, strConnectionType, nIsKeepAlive, *pnKeepAlive, 0/*ETag*/, 0 /*Ext*/, nOutResultSize, -1, -1);
				int nHL = strlen(sockBuffer);

				if (nHL + nOutResultSize < MAX_DATA_CHUNK)
				{
					memcpy(&sockBuffer[nHL],pzResults,nOutResultSize);
					if (nonblocksend(td->sockfd,sockBuffer,nOutResultSize+nHL) == nOutResultSize+nHL)
						*nTotalProxiedBytes += nOutResultSize+nHL;
				}
				else
				{
					if (nonblocksend(td->sockfd,sockBuffer,nHL) == nHL)
					{
						*nTotalProxiedBytes += nHL;
						// todo: chunk up the send if > 4 meg - it may cause socket() errors on Windows.
						if (nonblocksend(td->sockfd,pzResults,nOutResultSize) == nOutResultSize)
						{
							*nTotalProxiedBytes += nOutResultSize;
						}
					}
				}
			}
			return 1;
		}
		else if ( memcmp(sockBuffer,"GET",3) == 0 && (pzMethod || strPlugInPath.CompareNoCase(&sockBuffer[4],strPlugInPath.Length()) == 0 ) )
		{
			// RFC 2616, "Hypertext Transfer Protocol -- HTTP/1.1," does not specify any requirement for URL length. 
			// Microsoft Internet Explorer as of version 7, (in June 2007) supports a maximum URL length of 2,083
			const char *pzLanguage = ( !itPlugInArgs() ) ?  0 : itPlugInArgs++;
			const char *pzComponent = ( !itPlugInArgs() ) ?  0 : itPlugInArgs++;
			const char *pzInterface = ( !itPlugInArgs() ) ?  0 : itPlugInArgs++;
			if (!pzMethod) pzMethod = ( !itPlugInArgs() ) ?  0 : itPlugInArgs++;


			// determine (next transaction) connection attributes
			// default to "Keep Alive, unless directed to close"
			GString strConnectionType("Keep-Alive");
			int nIsKeepAlive = 1;
			char *pCon = strstr((char *)sockBuffer,"Connection:");
			if (pCon)
			{
				char *pEnd = strpbrk(pCon + 12, " \r\n");
				if (pEnd)
				{
					char chSave = *pEnd;
					*pEnd = 0;
					strConnectionType = pCon + 12;
					*pEnd = chSave;
					if (strConnectionType.CompareNoCase("close") == 0)
					{
						nIsKeepAlive = 0;
					}
				}
			}
			if (!nIsKeepAlive) // if the requestor did not want this connection to be kept alive 
			{
				*pnKeepAlive = 0;
			}
			else
			{
				(*pnKeepAlive)--;
			}
			td->m_pnKeepAlive = pnKeepAlive;  // todo: I think nobody uses m_pnKeepAlive in ThreadData, remove it if possible

			
			
			// prepare the information available to the driver callback			
			DriverCallBackArg prd;
			prd.XCmd = 0;
			prd.de = 0; // cipher N/A on anonymous HTTP
			prd.en = 0;
			prd.fd = td->sockfd;
			prd.nProtocol = 1;
			prd.tid = td->nThreadID;
			prd.strUser = "Anonymous HTTP";
			prd.pnKeepAlive = td->m_pnKeepAlive;
			prd.nCompleteSize = nBytes;
			prd.nObtainedCount = nBytes;
			prd.sDataFlags = 0; // N/A
			prd.nBuf1Size = MAX_DATA_CHUNK;
			

			// setup the driver options
			InterfaceInstance *pII = GetInterfaceInstance( pzLanguage);
			if (!pII)
			{
				GString strError;
				strError << pzLanguage << " is not configured correctly or is not a supported component type on this machine.";
				InfoLog(68,strError);
			}
			pII->SetOption("callback",(void *)DriverCallBack);
			pII->SetOption("callbackArg",&prd);
			char pzPIBuf[1024];
			pzPIBuf[0] = 0;
			pII->SetOption("piBuf",pzPIBuf);
			pII->SetOption("wkBuf",workBuffer);
			pII->SetOption("wkBuf2",sockBuffer);
			int n = MAX_DATA_CHUNK;
			pII->SetOption("wkBufSize",(void *)n);
			if ( GetProfile().GetBool("HTTP","UnloadPlugins",false) )
			{
				int yes = 1;
				pII->SetOption("unload",&yes); 
			}

			const char *pDataStart = &sockBuffer[4 + strPlugInPath.Length()]; // 4 = strlen("GET ")
			nBytes -= (pDataStart - sockBuffer);

			
			// get the argument names, if arg1 is "Header" and arg2 is "Content", then don't parse arguments.
			// in that case, parsing is the responsibility of the plug-in.
			int bParse = 1;
			GStringList *pL = pII->GetMethodParams(pzComponent, pzInterface, pzMethod);
			int nReqArgCount = pL->Size();
			GStringIterator itArgNames(pL);
			if (itArgNames())
			{
				char *p = stristr(itArgNames++,"Header");
				if (p && itArgNames())
				{
					p = stristr(itArgNames++,"Content");
					if (p && !itArgNames())
						bParse = 0;
				}
			}
			itArgNames.reset();


			
			// "GET /Path/PageAAA-BBB-CCC HTTP/1.1\r\n" 
			// where "/Path/Page" is the strPlugInPath, all remaining data to the space is the argument.
			// an argument is optional, the call could be "GET /Path/Page HTTP/1.1\r\n"
			int nArgsLen = 0;
			if (*pDataStart != 32)
			{
				char *pArgEnd = (char *)strpbrk(pDataStart," \r\n");
				if (pArgEnd && *pArgEnd == 32)
				{
					nArgsLen = pArgEnd - pDataStart;
				}
				else
				{
					nArgsLen = strlen(pDataStart);
				}
			}

			
			GString strArgSizes;
			// special case, pass the HTTP headers as Arg1, and the content as Arg2
			if (!bParse && nReqArgCount == 2) 
			{
				int nNewArgsLen = nArgsLen;
				if (nArgsLen) 
				{
					if (nArgsLen + nBytes < MAX_DATA_CHUNK)
					{
						int n = (pDataStart - sockBuffer);
						// unescape and place the arguments at the end of the buffer.
						nNewArgsLen = unEscapeData(pDataStart,&sockBuffer[nBytes+n]);
						sockBuffer[nBytes+n+nNewArgsLen] = 0; // terminate what we just copied
					}
				}
				pDataStart += (nArgsLen + 1);
				nBytes -= (nArgsLen + 1);
				strArgSizes << nBytes << "|" << nNewArgsLen;
			}
			
			// parse out the arguments supplied from the browser IN EXECUTION ORDER unlike the
			// POST logic, while under the GET logic the order that arguments are supplied in MATTERS.
			else 
			{
				char pzDelimit[2];
				pzDelimit[0] = pDataStart[0]; // set the user defined delimiter byte
				pzDelimit[1] = 0;
				GStringList lstHTMLArgs(pzDelimit,&pDataStart[1]);
				
				if (nReqArgCount > lstHTMLArgs.Size())
				{
					// todo: error
				}

				GStringIterator itHTMLArgs(&lstHTMLArgs);
				int nCurrentIndex = 0;
				while(itHTMLArgs())
				{
					int nNewArgLen = unEscapeData(itHTMLArgs++,&sockBuffer[nCurrentIndex]);
					nCurrentIndex += nNewArgLen + 1; // size plus NULL
					if (strArgSizes.Length())	
						strArgSizes << "|";
					strArgSizes << (nNewArgLen+1);
				}
				pDataStart = sockBuffer;
			}

			int nOutResultSize = 0;
			GString strRslt;
			pII->SetOption("rsltGStr",&strRslt);
			const char *pzResults = pII->InvokeEx( pzComponent, pzInterface, pzMethod, 
													pDataStart,strArgSizes,&nOutResultSize);
			if (stristr(pzPIBuf,"DataSent") != 0)
			{
				// the plugin sent the data directly
				return 1; // HTTP request handled.
			}
			if (nOutResultSize < 1 || stristr(pzPIBuf,"NoKeepAlive") != 0)
			{
				*pnKeepAlive = 0;
				return 1; // HTTP request handled, no results from user created handler.
			}

			// currently this is the only processing instruction available, in the future this will be a pipe separated list.
			// todo: add processing instruction support "NoKeepAlive"
			if (stristr(pzPIBuf,"HasHeaders") != 0)
			{
				if (nOutResultSize)
					if (nonblocksend(td->sockfd,pzResults,nOutResultSize) == nOutResultSize)
						*nTotalProxiedBytes += nOutResultSize;
			}
			else
			{
				// todo: support multiple content types by setting "Ext", currently NULL is "text/html"
				BuildHTTPHeader(sockBuffer, 0, strConnectionType, nIsKeepAlive, *pnKeepAlive, 0/*ETag*/, 0 /*Ext*/, nOutResultSize, -1, -1);
				int nHL = strlen(sockBuffer);

				if (nHL + nOutResultSize < MAX_DATA_CHUNK)
				{
					memcpy(&sockBuffer[nHL],pzResults,nOutResultSize);
					if (nonblocksend(td->sockfd,sockBuffer,nOutResultSize+nHL) == nOutResultSize+nHL)
						*nTotalProxiedBytes += nOutResultSize+nHL;
				}
				else
				{
					if (nonblocksend(td->sockfd,sockBuffer,nHL) == nHL)
					{
						*nTotalProxiedBytes += nHL;
						// todo: chunk up the send if > 4 meg 
						if (nonblocksend(td->sockfd,pzResults,nOutResultSize) == nOutResultSize)
						{
							*nTotalProxiedBytes += nOutResultSize;
						}
					}
				}
			}
			return 1;
		}
	}
	return 0;
}




void *ProxyHelperThread(void *arg)
{
	ThreadData *td = (ThreadData *)arg;

//	if (g_TraceThread)
//	{
//		GString strThreadTrace;
//		strThreadTrace << "++++Created new ProxyHelperThread    tid:" << td->nThreadID;
//		InfoLog(111,strThreadTrace);
//	}


	CipherData *pcdToServer = 0;
	CipherData *pcdToClient = 0;

	int fd;
	int nBufBytes;
	int nPacketIndex;
	int nPacketLen;
	int nCloseCode;
	int nReadWait;
	int nIdleReads;
	int nTotalTransmit = 0;
	char *pPacket;
	const int off = 0;
	int nFirstThreadUse = 1;

	GStringList lstRollOver;
	GString strMessagingSender;

	char *cryptDest = 0;
	char *sockBuffer = 0;
	if (g_nLockThreadBuffers) 
	{
		// this makes the allocation before the transaction begins, making the txn a little faster
		int n = MAX_DATA_CHUNK;
		cryptDest = new char[(n * 2) + 1024];
		sockBuffer = &cryptDest[n + 512];

		sockBuffer[0] = 0;
		cryptDest[0] = 0;
		
		// The only external reference of this memory is in KillTid(), search this file for KillTidMemoryNote
		td->Buf1 = cryptDest;
		td->Buf2 = sockBuffer;
	}

	struct sockaddr_in their_addr; 
	their_addr.sin_family = AF_INET;
	
	// seed the random number generator once on each thread with a unique seed.
	g_nRrandSeed += (777 * (g_nRrandSeed % 100)) ;
	srand( g_nRrandSeed );

BACK_TO_THE_POOL:

	nBufBytes = 0;
	nPacketIndex = -1;
	nPacketLen = 0;
	pPacket = 0;
	nCloseCode = 2100; // errors in 21nn range are Secondary Thread errors, started by the primary thread (range [11nn - 12nn] for this connection
	nReadWait = g_nProxyReadWait;
	nIdleReads = 0;
	
	
	if (!nFirstThreadUse)
	{
		#ifdef _WIN32
			InterlockedDecrement(&g_nProxyThreadsInUse);
		#else
			g_nProxyThreadsInUse--;
		#endif
	}

	// ----------------------------------------------------------------------
	// wait here until the clientthread wakes up this thread.
	// ----------------------------------------------------------------------
	if ( td->m_bUseThreads )
	{
		td->nThreadIsBusy = 0;
		gthread_cond_wait(&td->cond, &td->lock); 
	}
	td->starttime = getTimeMS();
	if (g_ServerIsShuttingDown)
	{
		PortableClose(td->sockfd,"Core8");

		if (g_nLockThreadBuffers == 0 && cryptDest)
		{
			delete cryptDest;
			td->Buf1 = 0;
			td->Buf2 = 0;
		}
		gthread_exit(0);
		return 0;
	}


	#ifdef _WIN32
		InterlockedIncrement(&g_nProxyThreadsInUse);
	#else
		g_nProxyThreadsInUse++;
	#endif
	if (g_nProxyThreadsInUse > g_nProxyThreadsPeakUse)
		g_nProxyThreadsPeakUse = g_nProxyThreadsInUse;

	nFirstThreadUse = 0;


	if ( g_ThreadPing )
	{
		GString strLog;
		strLog.Format("H%d",(int)td->nThreadID);
		InfoLog(69,strLog);

		goto BACK_TO_THE_POOL;
	}
	
	if (cryptDest == 0)
	{
		int n = MAX_DATA_CHUNK;
		cryptDest = new char[(n * 2) + 1024];
		sockBuffer = &cryptDest[n + 512];

		sockBuffer[0] = 0;
		cryptDest[0] = 0;
		// The only external reference of this memory is in KillTid(), search this file for KillTidMemoryNote
		td->Buf1 = cryptDest;
		td->Buf2 = sockBuffer;
	}


	// the 16 bit flag field indicating data attributes
	int bReadHTTP;

	unsigned short sAttribs = BuildAttributesHeader(td->pTSD->szConfigSectionName,td->pTSD->nProtocol, td->nAction, &bReadHTTP, 0);
	int bIsTunnel = td->pTSD->bIsTunnel;


	const char *pKey128 = 0;
	if ( sAttribs & ATTRIB_CRYPTED_2FISH )
	{
		pKey128 = GetProfile().GetString(td->pTSD->szConfigSectionName,"CipherPass",false);
	}
	CipherData en(pKey128,DIR_ENCRYPT);
	CipherData de(pKey128,DIR_DECRYPT);

	if ( sAttribs & ATTRIB_CRYPTED_2FISH )
	{
		if (bIsTunnel)
		{
			pcdToClient = &de;
			pcdToServer = &en;
		}
		else
		{
			pcdToClient = &en;
			pcdToServer = &de;
		}
	}
	
	// determine the destination server.
	GString strProxyToServer;
	int nProxyToPort = td->pTSD->nProxyToPort;
	if (td->pTSD->nProtocol == 3)
	{
		char *pSep = strpbrk(td->pzIP,"@");
		if ( pSep )
		{
			strProxyToServer = pSep + 1;
			*pSep = 0;
			nProxyToPort = atoi(td->pzIP);
			*pSep = '@';
		}
		else
		{
			strProxyToServer = td->pzIP;
			nProxyToPort = 10777;
		}
	}
	// standard HTTP proxy
	else if( td->pTSD->nProtocol == 11 )
	{
		char *pSep = strpbrk(td->pzConnectRoute,"@");
		if ( pSep )
		{
			*pSep = 0;
			nProxyToPort = atoi(td->pzConnectRoute);
			strProxyToServer = pSep + 1;
		}
		else
		{
			strProxyToServer = td->pzConnectRoute; 
			nProxyToPort = 80; 
		}

	}

	else if (td->pTSD->nProtocol == 1 ) // HTTP messaging proxy
	{
		strProxyToServer = td->pzIP; // always a "~Name"
	}
	else
	{
		nProxyToPort = td->pTSD->nProxyToPort;
		strProxyToServer = td->pTSD->szProxyToServer; // default

		if (td->pTSD->szConfigSectionName && td->pTSD->szConfigSectionName[0])
		{
			// parse the comma separated "on fail to connect - roll over list"
			const char *pzRollOver = GetProfile().GetString(td->pTSD->szConfigSectionName,"RollOver",false);
			lstRollOver.DeSerialize(",",pzRollOver);

			// See if there is a 'load balanced' connection order specified:
			//
			// [Proxy2]
			// Balance=50,www.Domain.com,25,www.UBTInc.com,25,www.5Loaves.com
			// 
			// then the "RemoteMachine" is overridden with 50% routed to www.Domain.com
			// 25% routed to www.UBTInc.com and 25% routed to www.5Loaves.com
			const char *pzBalance = GetProfile().GetString(td->pTSD->szConfigSectionName,"Balance",false);
			// strProxyToServer will be changed to the new randomly balanced location.
			GetBalanceTo(pzBalance, strProxyToServer);
		}
	}

ROLL_OVER_CONNECT:	


	fd = DoConnect(strProxyToServer, nProxyToPort,td->nParentThreadID, td->nThreadID); // in ProxyThread
	if (fd == -1)
	{
		// roll over to a backup server if configured
		if (lstRollOver.Size())
		{
			if(g_TraceConnection)
			{
				InfoLog(70,"Rollover Connection");
			}
			strProxyToServer = lstRollOver.RemoveFirst();
			goto ROLL_OVER_CONNECT;
		}

		*td->pnProxyClosed = 1;
		nCloseCode = 2101;	
		goto THREAD_EXIT;
	}

    // turn the OS kernel buffering off.
	if (g_StopDoubleBuffer)
	{
		setsockopt(fd, SOL_SOCKET, SO_SNDBUF, (char *)&off, sizeof(off));
		int one = 1;
		setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (char *)&one, sizeof(one));
	}
	

	*(td->pnProxyfd) = fd;					// Give the base thread(clientthread) this connection handle
	gthread_cond_signal( td->pcondHelper ); // and notify it that the connection is ready

	// ProxyHelperThread main loop - reading from [strProxyToServer]
	while( !(*td->pnProxyClosed)  )
	{
		// if what we are about to recv() is expected to be (bCipher || bZip || bSync)
		int bPacketSync = ( bIsTunnel && sAttribs ) ? 1 : 0;
		

		if (bReadHTTP) // this can be removed - producing no change in server behavior
		{
			bReadHTTP = 0;  // the base thread is reading HTTP, this thread can too but does not since it's faster to proxy data as it is available.
			bPacketSync = 0; 
		}
		unsigned short sPktAttributes = 0;

		int rslt = ReadPacket(fd,0,nReadWait,&nBufBytes,sockBuffer,&pPacket,&nPacketLen, &nPacketIndex,&sPktAttributes, sAttribs, bPacketSync, bReadHTTP);
		if (*td->pnProxyClosed)
		{
			nCloseCode = 2103; 
			goto THREAD_EXIT;
		}
		if (rslt == 0)
		{
			if (nIdleReads++ > g_nProxyIdleAt)
			{
				nReadWait = g_nProxyIdleReadWait; // slow this thread down until more data shows up
			}
			// no data or only a partial packet available - nothing to do.
			if (*td->plLastActivityTime)
			{
				if ( (time(0) - *td->plLastActivityTime ) > (unsigned long)td->pTSD->nProxyTimeout )
				{
					GString strInfo;
					strInfo << "Protocol [" << GetProtocolName( td->pTSD->nProtocol ) << "] inactivity time out for:" << td->pzIP;
					InfoLog(73,strInfo);

					nCloseCode = 2104; 
					goto THREAD_EXIT;
				}
			}
			else
			{
				*td->plLastActivityTime = time(0);
			}
		}
		else if (rslt == -1) // failure during wait (connection closed by server)
		{ 
			nCloseCode = 2105; 
			goto THREAD_EXIT;
		}
		else // process this packet on the Proxy Helper thread
		{
			*td->plLastActivityTime = time(0);
			nIdleReads = 0;
			nReadWait = g_nProxyReadWait;

			char *pSendData = pPacket;
			int nSendDataLen = nPacketLen;
			if (g_DataTrace)
			{
				GString strLabel; 
				strLabel.Format("tid%d rx from s[%s]",(int)td->nThreadID,(const char *)strProxyToServer);
				DataTrace(strLabel, pSendData, nSendDataLen);
			}


			if ( sAttribs ) // bCipher || bZip || bSync
			{
				int newLength;
				if (!bIsTunnel) // if we are about to encrypt/compress data from the application to send into the tunnel
				{
					cryptDest += 4; // move the pointer ahead 4 bytes where we will put the header
				}

				// log the raw send event
				if (GetProfile().GetBoolean(td->pTSD->szConfigSectionName,"LogEnabled",false))
				{
					ProxyLog(td->pTSD->szConfigSectionName, td->nParentThreadID, pSendData, nSendDataLen, "raw>c");
				}


				// compress/crypt or decompress/decrypt depending on bIsTunnel
				if ( PrepForClient(sAttribs,bIsTunnel,pcdToClient,pSendData,nSendDataLen,cryptDest,&newLength) )
				{
					// if (sending to client tunnel) add the attribute header otherwise sending to client application 
					if (!bIsTunnel) 
					{
						cryptDest -= 4; // move the pointer back 4 bytes to make room for the header
						unsigned short sh = htons(newLength); // packet length in network byte order
						memcpy(&cryptDest[0],&sh,sizeof(short));
						sh = htons(sAttribs); // attributes in network byte order
						memcpy(&cryptDest[2],&sh,sizeof(short));
						newLength += 4; // account for the header in the packet size
					}
					pSendData = cryptDest;
					nSendDataLen = newLength;
				}
				else
				{
					nCloseCode = 2106; 
					goto THREAD_EXIT;
				}
			}

			if (*td->pnProxyClosed)
			{
				nCloseCode = 2107; 
				goto THREAD_EXIT;
			}

			// transmit to client
			if (nonblocksend(td->sockfd,pSendData, nSendDataLen) != nSendDataLen)
			{
				nCloseCode = 2108; 
				goto THREAD_EXIT;
			}
			nTotalTransmit += nSendDataLen;
			
			// log the send event from helper thread
			if (GetProfile().GetBoolean(td->pTSD->szConfigSectionName,"LogEnabled",false))
			{
				ProxyLog(td->pTSD->szConfigSectionName, td->nParentThreadID, pSendData, nSendDataLen, "tx->c");
			}
			if (g_DataTrace)
			{
				GString strLabel;
				strLabel.Format("tid%d tx to c",(int)td->nThreadID);
				DataTrace(strLabel, pSendData, nSendDataLen);
			}

		}
	} 
THREAD_EXIT:

	if ( *td->pnProxyClosed != 777 ) // 777 signifies a "detach" so the connection must not be closed
	{
		if(g_TraceConnection && !g_ServerIsShuttingDown)
		{
			GString strTime;
			int nElapsed = getTimeMS() - td->starttime;
			if (nElapsed > 300000)
				strTime << (nElapsed / 1000) / 60 << "min"; // minutes over 5
			else if (nElapsed > 5000 && nElapsed < 300000)
				strTime << nElapsed / 1000 << "sec"; // seconds between 5 seconds and 300 seconds
			else
				strTime << nElapsed << "ms"; // milliseconds less than 5000
			
			// note: on Windows - transactions that complete in the < 50 millisecond range on slower CPU's have been seen to report an elapsed time of 0.
			// Use GPerformanceTimer in GPerformanceTimer.h if you want to count clock ticks per transaction.  
			GString strInfo;
			strInfo.Format("Closed Outbound [%d:%s] (tid:%d  %s) code[%d:%d]",(int)nProxyToPort,(const char *)strProxyToServer,td->nThreadID,strTime.Buf(),(int)nCloseCode,(int)SOCK_ERR);
			InfoLog(74,strInfo);
		}

		// In many cases *td->pnProxyClosed was already set to '1' a few steps earlier, do not assume it was '0' at this point
		*td->pnProxyClosed = 1; // 1 == Shut down in progress.

		PortableClose(td->sockfd,"Core9");
	
	}
	*td->pbHelperThreadIsRunning = 0; // helper shutdown complete.

	if (g_nLockThreadBuffers == 0 && cryptDest)
	{
		delete cryptDest;
		cryptDest = 0;
		sockBuffer = 0;
		td->Buf1 = 0;
		td->Buf2 = 0;
	}
 	else
	{
		sockBuffer[0] = 0;
		cryptDest[0] = 0;
	}

	if (g_ServerIsShuttingDown)
	{
		gthread_exit(0);
	}
	else
	{
		*td->pnProxyClosed = 2; // 2 = Closed and in pool.
		goto BACK_TO_THE_POOL;
	}
	return 0;
}









int stopListeners(int nPort = -1)
{
	// stop the listeners
	int nRet = 0;
	ThreadStartupData *pTSD = &g_TSD;
	while(pTSD)
	{
		if (nPort == -1 || nPort == pTSD->nPort)
		{
			pTSD->bThreadShuttingDown = 1;
			nRet++;
		}
		pTSD = pTSD->Next();
	}
	return nRet;
}

// nAction  1=Create socket and Bind  2=Start Listener Thread  3=Both
int startListeners(int nAction, int nPort = -1) 
{
	int nRet = 0; 

	ThreadStartupData *pTSD;
	struct sockaddr_in  serv_addr;

	// create and bind to a socket on port(s) specified that this server handles
	if (nAction == 1 || nAction == 3)
	{
		pTSD = &g_TSD;
		while(pTSD)
		{
			if (nPort == -1 || nPort == pTSD->nPort)
			{
				if (pTSD->nPort > -1)
				{
					if((pTSD->sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
					{
						pTSD->nState = -1; 
						InfoLog(90,"Can't open socket. Check permissions. ");
						return 0;
					}
					memset((char *) &serv_addr, 0, sizeof(serv_addr));
					serv_addr.sin_family = AF_INET;
					serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
					serv_addr.sin_port = htons( pTSD->nPort );
					
					if(bind(pTSD->sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
					{
						pTSD->nState = -2;
						GString strErr;																// that big arrow makes this stick out in a log file
						strErr.Format("Cannot use port[%d] (try [netstat -o] or [netstat -lp | grep %d]  ---------------------------->  N\\A[%d]",(int)pTSD->nPort, (int)pTSD->nPort,(int)pTSD->nPort);
						InfoLog(91,strErr);
					}
					else
					{
						GString strInfo;
						strInfo.Format("Listening on port[%d]",(int)pTSD->nPort);
						InfoLog(92,strInfo);
						pTSD->nState = 1; // ready
					}

					setsockopt(pTSD->sockfd,SOL_SOCKET,SO_RCVBUF,(char *)&g_nKernelSocketRcvBuf, sizeof(g_nKernelSocketRcvBuf));

					nRet++;
				}
			}
			pTSD = pTSD->Next();
		}
	}
	if (nAction == 2 || nAction == 3)
	{
		// create a listener thread for each port we handle
		pTSD = &g_TSD;
		while(pTSD)
		{
			if (nPort == -1 || nPort == pTSD->nPort)
			{
				if (pTSD->nState == 1) // we've already done a bind()
				{
					pTSD->bThreadShuttingDown = 0;
					gthread_t listen_thr;


					gthread_create(&listen_thr,	NULL, listenThread, (void *)pTSD );
				}
			}
			if (nAction == 2 )
			{
				nRet++;
			}

			pTSD = pTSD->Next();
		}
	}
	return nRet;
}



void showActiveThreads(GString *pG = 0);

#ifdef ___XFER
	#include "../../Libraries/Xfer/Core/XferLocalCommands.cpp"
#endif

#ifdef __CUSTOM_CORE__
	#include "ServerCoreCustomGlobalInternals.cpp" 
#endif


class MessagingArgs
{
public:
	GString strMessagingHeader;
	GString strMessagingSender;
	GString strMessagingReceiver;
	GString strMessagingDataInfo;
	GString strMessagingTitle;
	GString strMessagingHash;
};



// This thread services all connection types.
void *clientThread(void *arg)
{
	ThreadData *td = (ThreadData *)arg;

//	if (g_TraceThread)
//	{
//		GString strThreadTrace;
//		strThreadTrace << "++++Created new clientThread   tid:" << td->nThreadID;
//		InfoLog(111,strThreadTrace);
//	}


	unsigned long lBytesRemaining;
	char *pCommandParam;
	const char *pSent = 0;
	int nYielded = 0;
	const char *pzHome = 0;
	int bHelperThreadIsRunning = 0;
	int nCloseCode = 1100; // default close code for Branch 1   error range 11nn(1st thread, 1nd branch)
	int nInitialRawSend = 0;
	char *pInitialProxySend = 0;
	const int off = 0;
	int nFirstThreadUse = 1;

		
	int bisHead = 0;  // is the HTTP HEAD verb
	int nKeepAlive = g_nMaxKeepAlives; // counts down to 0 with each kept alive transaction

	MessagingArgs MA;
	ThreadConnectionData TCD;

	int nTotalProxiedBytes = 0;
	
	int nProxyfd = -1;
	
	// KillTidMemoryNote:
	// -------------------------
	// A reference to this threads memory is stored external to the thread.  ServerCore does not make other heap 
	// allocations.  If this thread was to be killed by the administrator there will be no memory loss.
	// Keep in mind that this thread services many different kinds of protocols each protocol may or may not
	// make it's own heap allocations that an untimely 'kill' would leave 'lost' until the whole process ends.
	// Future development should maintain the guarantee that no heap allocations are made at the 
	// 'system transactional level' but rather only in the protocol implementation, for example a plugin may make
	// heap allocations (on this thread) then should the thread be killed the admin may have the peace of
	// mind that any ill effect of the kill is localized to the logic of the plugin.

	char *sockBuffer = 0;
	char *cryptDest = 0;
	if (g_nLockThreadBuffers) 
	{

		// make the allocation before the transaction begins, making the txn a little faster
		int n = MAX_DATA_CHUNK;
		cryptDest = new char[(n * 2) + 1024];	// note: only 1 allocation for both buffers
		sockBuffer = &cryptDest[n + 512];

		sockBuffer[0] = 0;
		cryptDest[0] = 0;
		
		// external reference of this memory is in KillTid() and in the plugins, search this file for KillTidMemoryNote
		td->Buf1 = sockBuffer;
		td->Buf2 = cryptDest;
	}


	MessagingArgs *pMA = &MA;


	// stream read in (used for HTTP & TXML requests only)
	SERVER_MEMORY_EXTENSION;


	// seed the system random number generator on each thread with a unique number
	g_nRrandSeed += (777 * (g_nRrandSeed % 100)) ;
	srand( g_nRrandSeed );


	GString strConnectedIPAddress;


	int nBufBytes;
	int nPacketIndex;
	int nPacketLen;
	int nProxyMaxBytes;
	int nProxyClosed = 1;
	GString strCertifiedMsgResponse;

	char *pPacket;
	unsigned long lLastActivityTime;


	ExceptionHandlerScope durationOfThread;

MAIN_THREAD_BODY:

#ifdef _XMLF_JIT_
	XML_TRY		// note: comment this line out so the JIT debugging can record the call stack It should be in the final build
#endif
	{
		// pre-caches Transaction objects in the server build
		OBJECT_PRECACHE;

		pCommandParam = 0;
		lBytesRemaining = 0;
		pSent = 0;
		nYielded = 0;
		pzHome = 0;
		nBufBytes = 0;
		nPacketIndex = -1;
		nPacketLen = 0;
		pPacket = 0;
		lLastActivityTime = 0;
		bHelperThreadIsRunning = 0;
		nProxyMaxBytes = -1;
		
		if (g_TraceThread)
		{
			GString strLog;
			strLog.Format("Thread %d Waiting",(int)td->nThreadID);
			InfoLog(127,strLog);
		}


		if (!nFirstThreadUse)
		{
			#ifdef _WIN32
				InterlockedDecrement(&g_nClientthreadsInUse);
			#else
				g_nClientthreadsInUse--;
			#endif
		}
		// ----------------------------------------------------------------------
		// wait here until we accept a new connection and hand off to this thread
		// ----------------------------------------------------------------------
		if ( td->m_bUseThreads )
		{
			td->nThreadIsBusy = 0;
			gthread_cond_wait(&td->cond, &td->lock); 
		}

		// ----------------------------------------------------------------------
		// We have just begun to service a new request.  We are "On the clock" now.
		// ----------------------------------------------------------------------
		td->starttime = getTimeMS();
		if (g_ServerIsShuttingDown)
		{
			nCloseCode = 1101;
			goto SHUTTING_DOWN;
		}
		td->nSequence = 0;


	#ifdef _WIN32
		InterlockedIncrement(&g_nClientthreadsInUse);
	#else
		g_nClientthreadsInUse++;
	#endif
	if (g_nClientthreadsInUse > g_nClientthreadsPeakUse)
		g_nClientthreadsPeakUse = g_nClientthreadsInUse;

		nFirstThreadUse = 0;

		if ( g_ThreadPing )
		{
			GString strLog;
			strLog.Format("W%d",(int)td->nThreadID);
			InfoLog(128,strLog);
			goto MAIN_THREAD_BODY;
		}

		if (sockBuffer == 0)
		{
			int n = MAX_DATA_CHUNK;
			cryptDest = new char[(n * 2) + 1024];
			if (!cryptDest)
			{
				InfoLog(111,"********* Out of Memory *********");
			}
			sockBuffer = &cryptDest[n + 512];

			sockBuffer[0] = 0;
			cryptDest[0] = 0;
			// external reference of this memory is in KillTid() and in the plugins, search this file for KillTidMemoryNote
			td->Buf1 = sockBuffer;
			td->Buf2 = cryptDest;
		}


		// get the address of the connected client into strConnectedIPAddress;
		// this address cannot be trusted for any type of access restrictions.
		strConnectedIPAddress = td->pzClientIP;


		if(g_TraceConnection)
		{
			GString strMessage;
			strMessage.Format("Inbound [%d:%s] (fd:%d tid:%d type:%s)",(int)td->pTSD->nPort,(const char *)strConnectedIPAddress,(int)td->sockfd, (int)td->nThreadID,GetProtocolName( td->pTSD->nProtocol ));
			InfoLog(129,strMessage);
		}
		if (g_TraceThread)
		{
			GString strLog;
			strLog.Format("Thread %d Working",(int)td->nThreadID);
			InfoLog(130,strLog);
		}

	    // turn the OS kernel buffering off
		if (g_StopDoubleBuffer)
		{
			setsockopt(td->sockfd, SOL_SOCKET, SO_SNDBUF, (char *)&off, sizeof(off));
			int one = 1;
			setsockopt(td->sockfd, IPPROTO_TCP, TCP_NODELAY, (char *)&one, sizeof(one));
		}



		nProxyMaxBytes = -1;
		pInitialProxySend = 0;
		nInitialRawSend = 0;


		if (td->pTSD->nProtocol == 3) // Xfer or remote console
		{
#ifdef ___XFER
#include "../../Libraries/Xfer/Core/XferCoreIntegration.cpp"
#endif
		}


		//////////////////////////////////////////////////////////////////////////		
		//		
		// Following this comment are the two main logic branches.
		// Branch 1 - forwards the request/data to another location (switchboard, hop, or final destination)
		// Branch 2 - reads HTTP headers, and generates an HTTP response 
		//
		//////////////////////////////////////////////////////////////////////////		
		

		// if ( tunnel(in)/proxy(out)  or        raw(bounce)        or   HTTP conversion bounce		or    PrivateSurf
		if (td->pTSD->nProtocol == 2  || td->pTSD->nProtocol == 3  ||   td->pTSD->nProtocol == 11   || td->pTSD->nProtocol == 9 || td->pTSD->nProtocol == 12)
		{
		//////////////////////////////////////////////////////////////////////////		
		// Branch 1 - forwards the request/data to another location (switchboard, hop, or final destination)
		//////////////////////////////////////////////////////////////////////////		
		nProxyfd = -1;
		nProxyClosed = 0; // it's not exactly open yet but it will be unless someone changes this flag to stop it

		// Tie the threadData(td) of 'this' thread to the thread that will proxy on our behalf
		TCD.pTSD = td->pTSD;
		TCD.sockfd = td->sockfd;
		TCD.pnProxyClosed = &nProxyClosed;
		TCD.pcondHelper = &td->condHelper;
		TCD.nParentThreadID = td->nThreadID;
		TCD.pnProxyfd = &nProxyfd;
		TCD.plLastActivityTime = &lLastActivityTime;
		TCD.pbHelperThreadIsRunning = &bHelperThreadIsRunning;
		strcpy(TCD.pzPollSectionName, td->pzPollSectionName);

		// the 16 bit flag field indicating data attributes
		int bReadHTTP;
		unsigned short sAttribs = BuildAttributesHeader(td->pTSD->szConfigSectionName, td->pTSD->nProtocol, TCD.nAction, &bReadHTTP, 1);
		int bIsTunnel = td->pTSD->bIsTunnel;


		const char *pKey128 = 0;
		if (  sAttribs & ATTRIB_CRYPTED_2FISH  )
		{
			pKey128 = GetProfile().GetString(td->pTSD->szConfigSectionName,"CipherPass",false);
		}
		int nDir = 0;
		if(bIsTunnel)
		{
			nDir = DIR_ENCRYPT;
		}
		else
		{
			nDir = DIR_DECRYPT;
		}
		CipherData cdToServer(pKey128,nDir);


		// HOOK: Ciphered Tunnel Setup


		// assign the "pcd" so that worker thread can obtain a pointer to this 
		// key and use it rather than rebuilding the same key.  currently unused.
		TCD.pTSD->pcd = &cdToServer;	
		
		////////////////////////////////////////////////////////////////////////////
		//
		// Here we have a protocol initialized, and ready to go.
		// The sAttribs flag has been set by BuildAttributesHeader()
		// and cipher keys have been prepared with the correct password.
		//
		//
		//  Hook - protocol preprocessor   
		//
		////////////////////////////////////////////////////////////////////////////



		// HTTP Proxy Server
		int bIsCONNECT = 0;
		if ( td->pTSD->nProtocol == 11)
		{
			//unsigned short sPktAttributes = 0;
			//unsigned short sUnusedAttribs = 0;
			unsigned int nPktIndex = 0;
			int rslt = ReadPacketHTTP(td->sockfd, &nInitialRawSend ,sockBuffer,&nPktIndex);
			
			if (rslt == 0)
			{
				// no data available
				nCloseCode = 1102;
				goto CLOSE_CONNECT; 
			}
			else if (rslt == -1) // failure during wait (connection closed by client)
			{
				nCloseCode = 1103; 
				goto CLOSE_CONNECT; 
			}
			else // process this initial packet
			{
				pInitialProxySend = sockBuffer; // (nInitialRawSend was set by ReadPacketHTTP() )

				GString strPxyConnectTo;
				if ( ConvertHTTPProxy(pInitialProxySend,nInitialRawSend,cryptDest,strPxyConnectTo, nInitialRawSend, &bIsCONNECT) )
				{
					td->strAction = "HTTP Proxy to ";
					td->strAction << strPxyConnectTo;

					pInitialProxySend = cryptDest;
					td->pTSD->nProxyTimeout = 60;
					strcpy(TCD.pzConnectRoute,(const char *)strPxyConnectTo);
					TCD.nConnectRouteSize = strlen(TCD.pzConnectRoute) + 1;

				}
				else
				{
					nCloseCode = 1104; 
					goto CLOSE_CONNECT; 
				}
			}
		}

		


		//	HOOK: Main protocol pre-implementation




		//////////////////////////////////////////////////////////////////////////////////////////////////
		//
		// This is the main loop in clientthread.
		//
		//////////////////////////////////////////////////////////////////////////////////////////////////
		IOBlocking(td->sockfd, 1);   // Set non-blocking IO incase some pre-handler forgot to set it back  
		nTotalProxiedBytes = 0;
		int bHasAwakenHelper = 0;
		do
		{
			////////////////////////////////////////////////////////////////////////////////////////////////
			// on the first pass through the loop start the ProxyHelperThread, make the remote connection
			// and send any initial protocol data that has been prepared in pInitialProxySend when nInitialRawSend bytes has been set to the length of pInitialProxySend.
			////////////////////////////////////////////////////////////////////////////////////////////////
			if (bHasAwakenHelper == 0) 
			{
				if(g_DataTrace)
				{
					GString strMessage("\n\nWaking Helper - Thread ID(tid):");
					strMessage << td->nThreadID;
					strMessage.ToFileAppend(GetProfile().GetString("Trace","DataTraceFile",false));
				}

				// awaken our helper thread and have it connect to the remote side of this hop or the switchboard
				AttachToClientThread(&TCD,g_PoolProxyHelperThread,g_ProxyPoolSize,&g_nProxyPoolRoundRobin);
				bHasAwakenHelper = 1;

				nProxyClosed = 0;
				// hang here until the forwardTo connection has been established 
				gtimespec ts;
				ts.tv_sec=time(0) + td->pTSD->nProxyTimeout; 
				ts.tv_nsec=0;
				int nWaitRslt = gthread_cond_timedwait(&td->condHelper, &td->lockHelper, &ts);
				if (nWaitRslt == ETIMEDOUT)  // if we timed out 
				{
					GString strLog;
					strLog.Format("Timeout [%d] seconds   ip[%s], port[%d]",(int)td->pTSD->nProxyTimeout, td->pzClientIP, (int)td->pTSD->nPort);
					InfoLog(131,strLog);
					nCloseCode = 1109;
					break;
				}
				if (nProxyClosed)
				{
					// the [balanced|roll-over] connection could not be established 
					nCloseCode = 1110; // no alternate specified
					break;
				}
				if (bIsCONNECT && td->pTSD->nProtocol == 11)
				{
					GString strConnectResponse;
					strConnectResponse = "HTTP/1.1 200 Connection established\r\nProxy-agent: FiveLoaves\r\n\r\n";
					nonblocksend(td->sockfd,strConnectResponse,(int)strConnectResponse.Length());
				}

				// except this assumes any header/markup is already prepared in pInitialProxySend
				if (nInitialRawSend)
				{
					//	send the first packet of proxy
					if (nonblocksend(nProxyfd,pInitialProxySend,nInitialRawSend) != nInitialRawSend)
					{
						nCloseCode = 1111; 
						break;
					}
					else if (g_DataTrace)
					{
						GString strLabel;
						strLabel.Format("tid%d tx to s",(int)td->nThreadID);
						DataTrace(strLabel, pInitialProxySend, nInitialRawSend);
					}
					nTotalProxiedBytes += nInitialRawSend;
					nInitialRawSend = 0;
				}
			}

			////////////////////////////////////////////////////////////////////////////////////////////////
			// ^^^^^^^^^^^^ End of "first pass through the main loop (started ProxyHelperThread)^^^^^^^^^^^
			////////////////////////////////////////////////////////////////////////////////////////////////







			// recv from: (bIsTunnel) ? client application : client tunnel
			// essentially---> int bPacketSync = ( (!bIsTunnel) && (bCipher || bZip || bSync) ) ? 1 : 0;
			int bPacketSync = ( (!bIsTunnel) && sAttribs ) ? 1 : 0;
			unsigned short sPktAttributes = 0;


// HOOK: Special protocol processing





			if (nProxyClosed) // If our worker thread has been stopped
			{
				nCloseCode = 1113; // end this thread too
				break;
			}

			// for httpProxy when on very slow host or network
			if (td->pTSD->nProtocol == 11)
			{
				PortableSleep(1,0);
				if (bIsCONNECT)
					bReadHTTP = 0;
			}

			int rslt = ReadPacket(td->sockfd,1,0,&nBufBytes,sockBuffer,&pPacket,&nPacketLen, &nPacketIndex,&sPktAttributes,sAttribs,bPacketSync,bReadHTTP);

			if (nProxyClosed) // If our worker thread has been stopped
			{
				nCloseCode = 1113; // end this thread too
				break;
			}

			if (rslt == 0)
			{
				// no data or only a partial packet available - nothing to do.
				if (lLastActivityTime)
				{
					if ( (time(0) - lLastActivityTime ) > (unsigned long)td->pTSD->nProxyTimeout )
					{
						//Helper Thread Wait Expired
						nCloseCode = 1114; 
						break;
					}
				}
				else
				{
					lLastActivityTime = time(0);
				}
			}
			else if (rslt == -1) // failure during wait (connection closed by client)
			{
				nCloseCode = 1115; 
				break;
			}
			else // process this packet
			{
				// keep alive on clientthread
				lLastActivityTime = time(0);

				char *pSendData = pPacket;
				int nSendDataLen = nPacketLen;
				if (g_DataTrace)
				{
					GString strLabel;
					strLabel.Format("tid%d rx from c",(int)td->nThreadID);
					DataTrace(strLabel, pSendData, nSendDataLen);
				}


				if (GetProfile().GetBoolean(td->pTSD->szConfigSectionName,"LogEnabled",false))
				{
					ProxyLog(td->pTSD->szConfigSectionName, td->nParentThreadID, pSendData, nSendDataLen, "raw>s");
				}

				// if we need to apply data attributes(Cipher || Zip || Sync)
				if ( sAttribs )
				{
					int newLength;
					if (bIsTunnel) // if we are about to encrypt/compress, make room for the packet header
					{
						cryptDest += 4; // move the pointer ahead 4 bytes
					}

					// compress/decompress/crypt/decrypt or custom manipulation
					if (PrepForServer(sAttribs,bIsTunnel,&cdToServer,pSendData,nSendDataLen,cryptDest,&newLength))
					{
						if (bIsTunnel) // if we just encrypted/compressed
						{
							cryptDest -= 4; // move the pointer back 4 bytes
							short sh = htons(newLength); // packet length in network byte order
							memcpy(&cryptDest[0],&sh,sizeof(short));
							sh = htons(sAttribs); // attributes in network byte order
							memcpy(&cryptDest[2],&sh,sizeof(short));
							newLength += 4; // account for the header in the packet size
						}
						pSendData = cryptDest;
						nSendDataLen = newLength;
					}
					else
					{
						//PrepForServer failed in Branch2
						nCloseCode = 1116; 
						break;
					}
				}


				
				////////////////////////////////////////////////////////////////////////////
				//
				// Here we have the data read in (by HTTP or by System Tunnel) from the client.
				// (client is: the application or host that made the connection to this machine)
				// It is about to be sent to the server(the machine we connected out to)
				// pSendData is set to the current data and nSendDataLen is it's length.
				//
				// Hook - Dynamic Proxy Client to Server Packet Conversion
				//
				////////////////////////////////////////////////////////////////////////////
				// if HTTP proxy
				if ( td->pTSD->nProtocol == 11)
				{

					if ( td->pTSD->nProtocol == 11 && bIsCONNECT )
					{
						// pSendData and nSendDataLen will be passed in the raw
						nSendDataLen = nSendDataLen;
					}
					else
					{
						GString strPxyConnectTo(TCD.pzConnectRoute);

						if ( ConvertHTTPProxy(pSendData,nSendDataLen,cryptDest,strPxyConnectTo,nSendDataLen, &bIsCONNECT) )
						{
							pSendData = cryptDest;
							nBufBytes = 0;
							nPacketLen = 0;
							if (strPxyConnectTo.CompareNoCase(TCD.pzConnectRoute) != 0)
							{
								// we want to connect to strPxyConnectTo but we have a cached connection to TCD.pzConnectRoute

								// hangup the ProxyHelperThread
								nProxyClosed = 1;
								while (nProxyClosed == 1) // it will be 2 when it's back in the pool
								{
									PortableSleep(0,250000);
								}
								strcpy(TCD.pzConnectRoute,(const char *)strPxyConnectTo);
								TCD.nConnectRouteSize = strlen(TCD.pzConnectRoute) + 1;


							    // awaken our new helper thread and have it connect to the remote side of this hop or the switchboard
								AttachToClientThread(&TCD,g_PoolProxyHelperThread,g_ProxyPoolSize,&g_nProxyPoolRoundRobin/*,ProxyHelperThread*/);

								// hang here until the forwardTo connection has been established 
								gtimespec ts;
								ts.tv_sec=time(0) + td->pTSD->nProxyTimeout; 
								ts.tv_nsec=0;
								int nWaitRslt = gthread_cond_timedwait(&td->condHelper, &td->lockHelper, &ts);
								if (nWaitRslt == ETIMEDOUT)  // if we timed out 
								{
									GString strLog;
									strLog.Format("Timeout [%d] seconds   ip[%s], port[%d]",(int)td->pTSD->nProxyTimeout, td->pzClientIP, (int)td->pTSD->nPort);
									InfoLog(131,strLog);
									nCloseCode = 1109;
									break;
								}
								nProxyClosed = 0;
							}
						}
						else
						{
							nCloseCode = 1117; 
							break;
						}
					}
				}

// HOOK: protocol implementation


				
//				if (nProxyMaxBytes > 0) // Protocol = 8
//				{
//					// Currently this only happens when protocol = 8, messaging is truncating the HTTP multipart trailer
//					nSendDataLen = ( (nTotalProxiedBytes + nSendDataLen) > nProxyMaxBytes) ? nProxyMaxBytes - nTotalProxiedBytes : nSendDataLen;
//					
//					// if the proxy is about to send the last chunk
//					if (nTotalProxiedBytes + nSendDataLen >= nProxyMaxBytes)  
//					{
//						nProxyClosed = 777; // detach the nProxyfd and end the ProxyHelperThread
//						while (nProxyClosed != 2) // it will be 2 when it's back in the pool
//						{
//							gsched_yield();
//						}
//					}
//				}

				if (nonblocksend(nProxyfd,pSendData, nSendDataLen) != nSendDataLen)
				{
					nCloseCode = 1120; 
					break;
				}
				nTotalProxiedBytes += nSendDataLen; 
				
				// log the send event
				if (GetProfile().GetBoolean(td->pTSD->szConfigSectionName,"LogEnabled",false))
				{
					ProxyLog(td->pTSD->szConfigSectionName, td->nParentThreadID, pSendData, nSendDataLen, "tx->s");
				}
				if (g_DataTrace)
				{
					GString strLabel; 
					strLabel.Format("tid%d tx to s",(int)td->nThreadID);
					DataTrace(strLabel, pSendData, nSendDataLen);
				}

				
				// this tests for the end of proxy for Protocol 8(Messaging)
//				if (nProxyMaxBytes > 0 && nTotalProxiedBytes >= nProxyMaxBytes)  
//				{
//					g_SwitchBoard.WaitForResponse(td->sockfd, strCertifiedMsgResponse);
//
//					td->sockfd = nProxyfd; // swap fd's - keep this one alive
//					
//					// put the thread back in the pool but DON'T CLOSE HIS FD, move it to this thread
//					goto KEEP_ALIVE;
//				}

			}
		}while( !nProxyClosed );


		// this is where it would fall through to anyhow.  the goto is for readability.
		goto CLOSE_CONNECT; 

		
		}// end of Branch 1						
		else if (td->pTSD->nProtocol == 1  ||	// HTTP Server			
				 td->pTSD->nProtocol == 4  ||	// TXML
				 td->pTSD->nProtocol == 5 )// ||	// Both TXML and HTTP
		{
		//////////////////////////////////////////////////////////////////////////		
		// Branch 2 - reads HTTP headers, and generates an HTTP response 
		//////////////////////////////////////////////////////////////////////////		
		nKeepAlive = g_nMaxKeepAlives;
		int nHeaderLength;
		int nBytes;
		int nCumulativeBytes;
		bisHead = 0;
		nCloseCode = 1200; // default close code for Branch 2   error range 12nn(1st thread, 2nd branch)
		// Set non-blocking IO 
		IOBlocking(td->sockfd, 1);
KEEP_ALIVE:
		td->nSequence++;
		nHeaderLength = 0;
		nBytes = -1;
		nCumulativeBytes = 0;

		int rslt = readableTimeout(td->sockfd, g_nKeepAliveTimeout, 0);
		// failure during wait || time limit expired
		if (rslt == -1 || rslt == 0)
		{
			//	[HTTP] Keep Alive Time Out met=(1201) or Client Disconnect=(1202)
			nCloseCode = (!rslt) ?  1201 : 1202;
			goto CLOSE_CONNECT;
		}

		// recv an HTTP command
		unsigned int nPktIndex = 0;	// you may need to use this value to fetch next block of content.
		unsigned long nContentLen = 0;
		char *pContent = 0;


		rslt = ReadPacketHTTP( td->sockfd, &nBytes, sockBuffer,&nPktIndex, &nContentLen, &pContent);
		if (rslt < 1)
		{
			// The client aborted, 1203=Timeout 1204=Socket level
			nCloseCode = (!rslt) ?  1203 : 1204;
			goto CLOSE_CONNECT;
		}

		//
		// These are useful values for the ServerCoreCustomHTTP.cpp hook
		int nHTTPHeaderLength = pContent - sockBuffer;
		int nHTTPContentAcquired = nBytes - nHTTPHeaderLength;



		sockBuffer[nBytes] = 0;
		if (g_HTTPHeaderTrace)
		{
			if (sockBuffer[nBytes-4] == '\r')
			{
				sockBuffer[nBytes-4] = 0;	
				InfoLog(196,sockBuffer);
				sockBuffer[nBytes-4] = '\r';	
			}
			else
			{
				InfoLog(197,sockBuffer);
			}
		}
		if(g_TraceConnection && nKeepAlive != g_nMaxKeepAlives)
		{
			GString strInfo;
			strInfo.Format("Inbound [%d:%s] (fd:%d tid:%d seq:%d type:%s)",(int)td->pTSD->nPort,(const char *)strConnectedIPAddress,(int)td->sockfd,(int)td->nThreadID,(int)td->nSequence,GetProtocolName( td->pTSD->nProtocol ));
			InfoLog(136,strInfo);
		}


		///////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// Check 1 - POST (dynamic connection for protocol 8 - Messaging) - only when the switchboard server is active
		//////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// look for a special post to root "POST / HTTP"
		if ( g_bEnableMessagingSwitchboard && memcmp(sockBuffer,"POST / ",7) == 0 )
		{

			GString strBoundary;
			char *pBoundary = strstr(sockBuffer,"boundary=");
			if (pBoundary)
			{
				char *pEnd = strpbrk(pBoundary,"\r\n ");
				if (pEnd)
				{
					char ch = *pEnd;
					*pEnd = 0;
					strBoundary = pBoundary + 9;
					*pEnd = ch;
				}
			}

			char *pContent= strstr(sockBuffer,"\r\n\r\n");
			int nContentTermLen = 4;
			if(pContent)
			{
				pContent += nContentTermLen;
			}
			char *pContent1 = pContent;

			// if all we got was the primary header in the first read - we need more data now
			if (pContent1 - sockBuffer == nBytes)
			{
				IOBlocking(td->sockfd, 0);
				int nMoreHeader = recv(td->sockfd, &sockBuffer[nBytes], MAX_DATA_CHUNK-nBytes, 0 );
				IOBlocking(td->sockfd, 1);
				 if (nMoreHeader > 0)
				 {
					nBytes += nMoreHeader;
				 }
				 else
				 {
					GString str;
					str.Format("Read of more header failed %d\n",(int)SOCK_ERR);
					InfoLog(137,str);
					pContent1 = 0;
				 }
			}
				
			if (pContent1)
			{
				// find the second 'end of headers'
				nContentTermLen = 4;
				pContent = strstr(pContent1,"\r\n\r\n");
				if(pContent)
				{
					pContent += nContentTermLen;
				}
			}
			int nSecondHeaderLen = pContent - pContent1;
			
			// int nFooterLen = 2 + (int)strBoundary.Length() + 2; // "--Boundary--"

			if ( pContent && pContent[0] == 'O' && pContent[1] == 'K')
			{
				int nContentArgLength = atoi(pContent+2);
				int nLenlen = 0;
				while ( pContent[2+nLenlen] >= '0' &&  pContent[2+nLenlen] <= '9')
					nLenlen++;

				// get the 'content arguments'
				GString strContentArgs(pContent+2+nLenlen, nContentArgLength);
				GStringList lstSBAction("/",strContentArgs);
				GStringIterator itArgs(&lstSBAction);
				if (itArgs())
					pMA->strMessagingReceiver = itArgs++;
				if (itArgs())
					pMA->strMessagingSender = itArgs++;
				if (itArgs())
					pMA->strMessagingTitle = itArgs++;
				if (itArgs())
					pMA->strMessagingDataInfo = itArgs++;
				if (itArgs())
					pMA->strMessagingHash = itArgs++;


				GString strHashSource;
				SwitchEntry *pSE2 = g_SwitchBoard.Pending (pMA->strMessagingReceiver, 1);
				if (pSE2)
				{
					strHashSource = pSE2->strRandomData;
				}
				else
				{
					GString strMessage;
					strMessage << "["  << pMA->strMessagingReceiver << "] Not awaiting message.";
					InfoLog(1,strMessage);
					goto CLOSE_CONNECT;
				}
				

				// todo: If this SwitchBoard connection requires certified connections it will now be certified.
				// pMA->strMessagingHash contains a ciphered version of pSE2->strRandomData based on the 
				// password for pMA->strMessagingSender, so decipher pMA->strMessagingHash and verify that it
				// matches pSE2->strRandomData, if so - this sender is certified to be who he says he is
				
				// this is only a placeholder test
				pMA->strMessagingHash.MakeReverse();

				if (pMA->strMessagingHash.Compare(pSE2->strRandomData) == 0)
				{
					// printf("PASSED Certification test\n");
				}
				else
				{
					// printf("FAILED Certification test\n");
				}

				int fdServer = pSE2->fdServer;
				int bResponse = pSE2->bResponse;

				delete pSE2;


				GString strCertificationData;
				GenerateCertificationData(strCertificationData);


				GString strSendArgs;
				strSendArgs << pMA->strMessagingReceiver << '/' << pMA->strMessagingSender << '/' 
							<< pMA->strMessagingTitle <<  '/' << pMA->strMessagingDataInfo << '/' 
							<< strCertificationData;

				sprintf(TCD.pzIP,"~%s",(const char *)pMA->strMessagingReceiver);
				td->pTSD->nProxyTimeout = 120;

				// get the content length
				int nCL = 0;
				char *pCL = stristr(sockBuffer,"Content-Length:");
				if (pCL)
				{
					pCL += 15; // first byte of content length in the HTTP header
					nCL = atoi(pCL);
				}

				// convert the POST header to a "200 OK" header.
				pMA->strMessagingHeader = "";
				int nNewContentLen = nCL - nSecondHeaderLen - nContentArgLength + (int)strSendArgs.Length();
				MakeHTTPHeader(pMA->strMessagingHeader, nNewContentLen);

				int nHTTPHeaderLen1 = (int)pMA->strMessagingHeader.Length();  

				pMA->strMessagingHeader << "OK";
				pMA->strMessagingHeader << strSendArgs.Length(); 

				
				pMA->strMessagingHeader.write(strSendArgs, strSendArgs.Length());
				
				
				// and append any message content that came too
				char *pDataStart = pContent+2+nLenlen+nContentArgLength;
				int nAllHeadersLen = (pDataStart - sockBuffer);

				if (nBytes > nAllHeadersLen)
				{
					pMA->strMessagingHeader.write(pDataStart, nBytes - nAllHeadersLen);
				}



				pInitialProxySend = (char *)(const char *)pMA->strMessagingHeader;
				nInitialRawSend = (int)pMA->strMessagingHeader.Length();


				// #of bytes to exchange before shutting down the proxy.  The proxy will shut down prior to sending the last 100 bytes or so
				// because the last few bytes are residue from the HTTP multi-part POST that has been changed into an HTTP 200 response.
				// They were read in from the POST, and they are simply thrown away.
				nProxyMaxBytes = nHTTPHeaderLen1 + nNewContentLen;

				strCertifiedMsgResponse = strCertificationData;


				
				// fill up the first block and send it to the server
				int nPostedPassthrough = nInitialRawSend;
				memcpy(sockBuffer,pInitialProxySend,nInitialRawSend);
				int nChunkSize = (nProxyMaxBytes - nInitialRawSend < MAX_DATA_CHUNK ) ? nProxyMaxBytes - nInitialRawSend : MAX_DATA_CHUNK - nInitialRawSend;
				int nGot = 0;
				if (nChunkSize)
				{
					nGot = nonblockrecv(td->sockfd,&sockBuffer[nInitialRawSend],nChunkSize);
					if (nGot < 1)
					{
						GString strMessage("failed to read in POST1:");
						strMessage << nGot << ":" << SOCK_ERR;
						InfoLog(1,strMessage);
						goto CLOSE_CONNECT;
					}
					nPostedPassthrough += nGot;
				}
				if (nonblocksend(fdServer,sockBuffer,nPostedPassthrough) != nPostedPassthrough)
				{
					GString strMessage("Failed to send message to server");
					InfoLog(1,strMessage);
					goto CLOSE_CONNECT;
				}


				// now send anything that is remaining
				while(nPostedPassthrough < nProxyMaxBytes)
				{
					nChunkSize = (nProxyMaxBytes - nPostedPassthrough < MAX_DATA_CHUNK) ? nProxyMaxBytes - nPostedPassthrough : MAX_DATA_CHUNK;
				
					nGot = nonblockrecv(td->sockfd,sockBuffer,nChunkSize);
					if (nGot < 1)
					{
						GString strMessage("failed to read in POST2");
						InfoLog(1,strMessage);
						goto CLOSE_CONNECT;
					}
					if (nonblocksend(fdServer,sockBuffer,nGot) != nGot)
					{
						GString strMessage("Failed to send message to server");
						InfoLog(1,strMessage);
						goto CLOSE_CONNECT;
					}
					nPostedPassthrough += nGot;
				}

				// now do one last read from the sender and throw it away, it's the HTTP multipart footer 
				nonblockrecvAny(td->sockfd,sockBuffer,MAX_DATA_CHUNK);


				if (bResponse == 0)
				{
					// the 1st time around
					g_SwitchBoard.WaitForResponse(td->sockfd, strCertifiedMsgResponse);
				}
				else // the 2nd time around
				{
					HTTPSend(td->sockfd, "OK", 2 );
					
					PortableClose(td->sockfd,"Core11");
				}


				// we detached the sender (td->sockfd) from this thread, he is back in the switchboard now
				td->sockfd = fdServer; // now attach the receiver onto this thread
				goto KEEP_ALIVE;
			}
		}

		//
		// Extension #1 - Plugin executive loader - executes user written Java, COM, C++, Perl functions
		//
		if (g_lstActivePlugins.Size())
		{
			try // todo: embed nested OS level exception handling since we're passing control off to an external developer, see the XML_TRY macro.
			{
				// todo: pass nCloseCode - set in 1000 range
				td->strAction = "Plugin";
				if (Plugin(td, sockBuffer, nBytes, nContentLen, pContent, &nKeepAlive, cryptDest, &nTotalProxiedBytes, nPktIndex))
				{
					goto HANDLER_FINISH;
				}
				else
				{
					td->strAction = "";
					// else give another handler a crack at it - fall through
				}
			}
			catch( GException &rErr )
			{
				// This may not be an error, for example we're mapped to a wildcard method name of
				// big*, mapped to an object that contains big1, big2 and big3 - but the HTTP request 
				// asked for BigWave.html, so log the info and allow the file handler a chance to service the request.
				GString strMsg("Plugin Message:");
				strMsg << rErr.GetDescription();
				InfoLog(138,strMsg);
			}
			catch(...)
			{
				GString strMsg("Fatal error in plugin. Debug dump follows:");
				strMsg << sockBuffer;
				strMsg << "\r\n\r\n";
				InfoLog(139,strMsg);
				nCloseCode = 1000;
				goto HANDLER_FINISH;
			}
		}

#ifdef ___XFER 
	#include "../../Libraries/Xfer/Core/XferHTTPHook.cpp"
#endif

// __CUSTOM_CORE__ includes all Core hooks,  SERVERCORE_CUSTOM_HTTP includes only this HTTP hook
// This will be the most commonly used hook into ServerCore, as it can be used to build simple HTTP Services
#if defined(__CUSTOM_CORE__) && !defined(SERVERCORE_CUSTOM_HTTP) 
	#include "ServerCoreCustomHTTP.cpp"
#endif


		//////////////////////////////////////////
		// Check 2 - GET (static file - or dynamic connection)
		//////////////////////////////////////////			
		// look for an HTTP GET, it MIGHT look something like this:
		/*
			GET /Index.html HTTP/1.1
			Accept: image/gif, image/x-xbitmap, image/jpeg, image/pjpeg, application/vnd.ms-excel, application/msword
			Accept-Language: en-us
			Accept-Encoding: gzip, deflate
			User-Agent: Mozilla/4.0 (compatible; MSIE 6.0; Windows NT 5.0)
			Host: 192.168.1.102
			Connection: Keep-Alive
		*/
		// but it could be a CGI or ISAPI call as well.
		// it could also beta GET for a connection at the switchboard

		bisHead = (memcmp(sockBuffer,"HEAD",4) == 0) ? 1 : 0;
		if ( memcmp(sockBuffer,"GET",3) == 0 || bisHead )
		{

			////////////////////////////////////////////////////////////////////////////
			//  Checks for a Messaging switchboard connection
			////////////////////////////////////////////////////////////////////////////
			// Check the path of the "file" that this HTTP get is looking for.
			// If we are looking for switchboard requests....
			// and the path of this request matches the special path that identifies this as the switchboard ---
			if ( g_bEnableMessagingSwitchboard && g_strMessagingSwitchPath.Length() )
			{
				char ch = sockBuffer[4 + g_strMessagingSwitchPath.Length()]; // save and null the first byte past the path
				sockBuffer[4 + g_strMessagingSwitchPath.Length()] = 0;
				// if the path to the "file" this HTTP GET is looking in matches
				// the "name" entry under [SwitchBoardServer], which might be a value
				// something like this "/MySwitchServerPath/"......
				if ( g_strMessagingSwitchPath.CompareNoCase(&sockBuffer[4]) == 0)
				{
					// then this is a server checking for a remote-waiting client connection on this machine.

					// restore first byte past the path since the comparison is done.
					sockBuffer[4 + g_strMessagingSwitchPath.Length()] = ch;

					// sockBuffer = "/PublicPath/put/DOMAIN 
					// being indexed at the word put;
					if (sockBuffer[4 + g_strMessagingSwitchPath.Length()] == 'p' && sockBuffer[5 + g_strMessagingSwitchPath.Length()] == 'u' && sockBuffer[6 + g_strMessagingSwitchPath.Length()] == 't')
					{
						
						// User sends "When can I put the Message?" as
						// GET /PublicPath/put/UBTsAccountForYou HTTP/1.1
						char *pCmdEnd = strstr(&sockBuffer[8 + g_strMessagingSwitchPath.Length()]," HTTP/");
						if (pCmdEnd)
							*pCmdEnd = 0;

					
						GString strRandomData;
						GenerateCertificationData(strRandomData);

						// wait for an entry in the SwitchBoard with the "DOMAIN" of 
						// 	"GET /PublicPath/put/DOMAIN HTTP/1.?" sent here on port 80.

						GString strConnectName;
						if (g_SwitchBoard.WaitForConnection (&sockBuffer[8 + g_strMessagingSwitchPath.Length()],strConnectName, strRandomData))
						{
							// found and placed in a special table of connections.
							
							// Server sends "Now", and here's some random data that you will
							// need to encrypt or hash and send with the file if you must "certify"
							// your connection before sending to this recipient.

							// make http message, send it, close connection and return to pool.
							GString strMsg;
							strMsg.Format("OK%s/%s",(const char *)strRandomData, (const char *)strConnectName);
							HTTPSend(td->sockfd, strMsg, (int)strMsg.Length() );
						}
						else
						{
							HTTPSend(td->sockfd, "NO", 2 ); // Server sends "Not yet."
						}
						
						goto KEEP_ALIVE;
					}

					char *pStart = &sockBuffer[4 + g_strMessagingSwitchPath.Length()];
					char *pEnd = strstr(pStart,"HTTP/");
					if (pEnd)
					{
						pEnd--;
						*pEnd = 0;

						// see if there is a client waiting to connect to this request.
						// Returns true if handoff happens otherwise false.
						// It is asking: "Is a [pStart] ready to send?"

						if (g_SwitchBoard.HandOffConnection (td->sockfd, pStart))
						{

							// there was a client waiting, td->sockfd is now being serviced
							// by another thread, so this thread goes back to the pool.
							// The connection does NOT get closed on this thread.
							goto SHUTTING_DOWN;
						}
						else
						{
							// no connections are waiting, close this connection then go back to the pool.
							// an HTTP response 'NO' must be sent to this client notifying him that there was no pickup.
							HTTPSend(td->sockfd, "NO", 2 );

							goto CLOSE_CONNECT;
						}
					}
				}
				else
				{
					// put [ch] back where the string was temporarily null terminated for the comparison.
					// this is not a switchboard request.
					sockBuffer[4 + g_strMessagingSwitchPath.Length()] = ch;
				}
			}
		
		
			////////////////////////////////////////////////////////////////////////////
			//  If this is not a CGI or ISAPI call then return a static file
			////////////////////////////////////////////////////////////////////////////
			// look for something like this (a CGI or ISAPI call)
			// GET /XMLServer/UBT.dll?TransactXML=......"
			// the ? must be in the first line of the HTTP header
			char *p = 0;
			char *pGETEnd = strpbrk(sockBuffer,"\r\n");
			if (pGETEnd)
			{
				char pSave = *pGETEnd;
				*pGETEnd = 0;
				p = strpbrk(sockBuffer,"?");  
				*pGETEnd = pSave;
			}
			
			// if there's no ?, this is not a CGI or ISAPI call
			// for a static document/image/javascript etc...
			if (!p) 
			{
				// If we're configured to look for HTTP GET's on this port go-on
				if (td->pTSD->nProtocol == 1 ||	td->pTSD->nProtocol == 5)
				{
					pzHome = GetProfile().GetString("HTTP","Home",false);

					// get the file (from memory or disk), assemble the HTTP response, and send it back.
					nKeepAlive = ReturnFile(td, pzHome, &sockBuffer[4] ,nBytes - 4,strConnectedIPAddress, nKeepAlive, bisHead, sockBuffer, cryptDest, &nTotalProxiedBytes);
					goto HANDLER_FINISH;
				}
			}
			
			// When posting data through a GET, from a browser, the 
			// URL Data has certain characters hex encoded (a space becomes %20 for example)
			// CGI or ISAPI calls get their param values unescaped at this point.
			/*int nNewDataLen = */unEscapeData(p + 1,p + 1);
			
			goto HANDLER_FINISH;
		}
		
		//////////////////////////////////////////
		// Check 3 - default POST handler 
		//////////////////////////////////////////			
		if ( memcmp(sockBuffer,"POST",4) == 0 )
		{
			GString strPostedTo;
			char *pSep = strpbrk(&sockBuffer[5],"\r\n ");
			if (pSep)
			{
				char ch = *pSep;
				*pSep = 0;
				strPostedTo = &sockBuffer[5];
				*pSep = ch;
			}
			nKeepAlive = 0;
			GString str;
			str.Format("Posted %u bytes, ignored - no POST handler registered for [%s].\n",(int)nContentLen,(const char *)strPostedTo);
			InfoLog(140,str);
		}
HANDLER_FINISH:
	
		// there has to be some reasonable limit to prevent denial of service attack
		if (nKeepAlive > 0 && nKeepAlive != g_nMaxKeepAlives)
		{
			goto KEEP_ALIVE;
		}

		if (nCloseCode == 1200) // unless otherwise specified, the reason we are closing this connection is:
			nCloseCode = 1277; // "1. Not using Keep alives, or 2. Max keep alives per connection reached or 3. Closed by transaction handler."

		
#ifdef ___TXML // UBT Server extension for TransactXML
	    #include "ServerDispatch.cpp"
#endif

		} // end of branch2
		
	}
#ifdef _XMLF_JIT_
	XML_CATCH(ex)
	{
		GString strLog;
		strLog.Format("XML_CATCH=%s\n",ex.GetDescription());
		InfoLog(141,strLog);

		PROPIGATE_SERVER_EXCEPTION;
	}
#endif // _XMLF_JIT_

CLOSE_CONNECT:  // this goto tag was previously named SOCKET_ERR_ABORT - it was renamed because it does not necessarily indicate error
	nProxyClosed = 1; // shared memory flag if a worker thread was started - stop it.


	if (td->sockfd != -1)
		PortableClose(td->sockfd,"Core12");

	
	// ----------------------------------------------------------------------------------------------------------
	// The connection is closed and the client has the response(or it's in transit).  We are "off the clock" now
	// ----------------------------------------------------------------------------------------------------------
	if (g_TraceThread)
	{
		GString strLog;
		strLog.Format("Thread %d  Service time ~:%d ms",(int)td->nThreadID, (int)getTimeMS() - td->starttime);
		InfoLog(142,strLog);
	}
	if(g_TraceConnection)
	{
		GString strTime;
		int nElapsed = getTimeMS() - td->starttime;
		if (nElapsed > 120000)
			strTime << ((getTimeMS() - td->starttime) / 1000) / 60 << "min"; // minutes over 2
		else if (nElapsed > 5000 && nElapsed < 120000)
			strTime << (getTimeMS() - td->starttime) / 1000 << "sec"; // seconds between 5 seconds and 120 seconds
		else
			strTime << (getTimeMS() - td->starttime) << "ms"; // milliseconds less than 5000

		GString strMessage;
		strMessage.Format("Closed Inbound [%d:%s] (fd:%d tid:%d  %s) code:[%d:%d]",
			(int)td->pTSD->nPort,(const char *)strConnectedIPAddress, (int)td->sockfd, (int)td->nThreadID, strTime.Buf(), (int)nCloseCode, (int)SOCK_ERR);
		InfoLog(143,strMessage);
	}

SHUTTING_DOWN:

	SERVER_MEMORY_RESET;
	if (cryptDest)
	{
		if (g_nLockThreadBuffers == 0)
		{
			delete cryptDest;
			cryptDest = 0;
			sockBuffer = 0;
			td->Buf1 = 0;
			td->Buf2 = 0;
		}
		else 
		{
			sockBuffer[0] = 0;
			cryptDest[0] = 0;
		}
	}


	td->strAction = "";

	// decrement the connection resource monitor
	// clientThread() has finished handling this connection
	if (strConnectedIPAddress.Length())
		ConnectionCounter(strConnectedIPAddress,-1); 

	unsigned long timenow = time(0);
	while (bHelperThreadIsRunning)
	{
		if (time(0) - timenow > 10)
		{
			// This is a development safety net.  
			// Under no known conditions can this code be reached.
			InfoLog(144,"Thread Hung.  You need to stop then restart this application to correct this problem");
			
			// Threads should not kill other threads.  Fix the problem.
		}
		gsched_yield();
	}

	// this thread and it's helper are finished, signal the event
	gthread_cond_signal( &td->condFinished ); 

	// don't free the memory, just go back and wait for another transaction.
	if (!g_ServerIsShuttingDown && td->m_bUseThreads )
	{
		goto MAIN_THREAD_BODY;
	}

	// release any additional "per protocol" transactional memory and exit this thread.
	// delete inboundXML;
	SERVER_MEMORY_CLEANUP;
	if (td->m_bUseThreads)
	{
		gthread_exit(0);
	}
	return 0;
}




int MessageRouteInfo(int fd,char *pzConnectRoute,char *pzIP,int *nAction, int *nConnectRouteSize,char *pzPassword, char *pRouteData, int nRouteDataLen )
{
	if (nRouteDataLen < 5)
		return 0; // invalid protocol

	int nRouteDataIndex = 0;

	unsigned short theNumber = 0;					  // storage on platform architecture byte boundary
	memcpy(&theNumber,&pRouteData[nRouteDataIndex],2);// storage in network byte order moved to architecture byte boundary storage 
	short nParams = ntohs( theNumber );				  // convert to host byte order

	nRouteDataIndex = 2;
	
	memcpy(&theNumber,&pRouteData[nRouteDataIndex],2);
	short nRouteMsgSize = ntohs( theNumber );
	nRouteDataIndex += 2;

	if (nRouteDataLen < nRouteMsgSize+nRouteDataIndex)
		return 0; // invalid protocol

	GString strRoute;
	strRoute.write(&pRouteData[nRouteDataIndex],nRouteMsgSize);
	nRouteDataIndex += nRouteMsgSize;

	/////////////////////////////////////////////////////////////////////////////////////////////////
	GStringList lstDeliveryIPs("|",(const char *)strRoute );

	if (nParams == 2) // password only sent to local host - not over the network
	{
		if (nRouteDataLen < nRouteDataIndex+2)
			return 0; // invalid protocol

		unsigned short theNumber = 0;						// storage on platform architecture byte boundary
		memcpy(&theNumber,&pRouteData[nRouteDataIndex],2);	// storage in network byte order moved to architecture byte boundary storage 
		short nPassSize = ntohs( theNumber );				// convert to host byte order
		nRouteDataIndex += 2;

		if (nRouteDataLen < nRouteDataIndex+nPassSize || nPassSize > 1024)
			return 0; // invalid protocol

		
		memcpy(pzPassword, &pRouteData[nRouteDataIndex], nPassSize);

		pzPassword[nPassSize] = 0;

	}
	else if (nParams != 1)
	{
		return 0; // protocol error, only 1 or 2 params (local or not-local)
	}

	GString strFirstIP(lstDeliveryIPs.PeekFirst());
	if (strFirstIP.GetAt(0) == '~')
	{
		lstDeliveryIPs.RemoveFirst();		  //  remove the ~connectionName
		lstDeliveryIPs.AddFirst("127.0.0.1"); // 'this' will get popped off
		strFirstIP = "127.0.0.1";
	}


	// if the IP is prefixed with the port "777@ThisHost"
	char *pSep = (char *)strpbrk((const char *)strFirstIP,"@");
	const char *pMeTest = (pSep) ? pSep + 1 : strFirstIP;

	if ( IsMe( pMeTest  ) )
	{
		if ( lstDeliveryIPs.Size() > 1 )
		{
			// pop this hop
			lstDeliveryIPs.RemoveFirst();

			__int64 nipLen = 0;
			const char *ip = lstDeliveryIPs.PeekFirst(&nipLen);
			if (nipLen > 1023)
				return 0;
			strcpy(pzIP, ip);
			
			const char *pzRoute = lstDeliveryIPs.Serialize("|");
			int nRouteLen = strlen(pzRoute);
			if (nRouteLen > 1019)
				return 0; // invalid protocol

			strcpy(&pzConnectRoute[4],pzRoute);
			unsigned short sh = htons(nRouteLen); 
			memcpy(&pzConnectRoute[2],&sh,sizeof(short));
			sh = htons(1); 
			memcpy(&pzConnectRoute[0],&sh,sizeof(short));
			*nConnectRouteSize = nRouteLen + 4;

			*nAction = 2;
		}
		else
		{
			// destination (proxy-->server), decrypted from tunnel and serviced by this machine
			*nConnectRouteSize = 0;
			*nAction = 3;
			pzIP[0] = 0;
		}
	}
	else
	{
		// tunnel (client-->proxy), raw VNC into tunnel crypted and sent to proxy
		__int64 nipLen = 0;
//		const char *ip = lstDeliveryIPs.PeekFirst(&nipLen);
		if (nipLen > 1023)
			return 0; // invalid protocol
		strcpy(pzIP,lstDeliveryIPs.PeekFirst());

		const char *pzRoute = lstDeliveryIPs.Serialize("|");
		int nRouteLen = strlen(pzRoute);
		if (nRouteLen > 1019)
			return 0; // invalid protocol
		strcpy(&pzConnectRoute[4],pzRoute);
		unsigned short sh = htons(nRouteLen); 
		memcpy(&pzConnectRoute[2],&sh,sizeof(short));
		sh = htons(1); 
		memcpy(&pzConnectRoute[0],&sh,sizeof(short));
		*nConnectRouteSize = nRouteLen + 4;
		
		*nAction = 1;
	}
	return 1;
}



int g_nUniqueSequence = 0; //only used by BuildConnectionPostData()
void BuildConnectionPostData(int tid, GString &strDestination, const char *pzConnectionFileName, const char *pzCfgSection, const char *pzProxyFirewall = 0,const char *pzHTTPAuth = 0, const char *pzHost = 0)
{
	
	// The strNonCacheTag is appended onto the URL for two reasons.
	// First, it guarantees that every requested URL is unique and therefore will not be cached by either a corporate HTTP proxy or by the ISP as many ISPs now manage caching for you.
	// Second, it includes this thread id, and the current Queue depth - information that the switchboard can use to detect if this connection supersedes a previous connection since TCP
	// has no way of notifying the remote side that a connection has been closed without a KeepAlive ping, that is expensive and not supported within a pure HTTP protocol.
	// %25 is a URL encoded '%'
	GString strNonCacheTag("%25");
	strNonCacheTag << tid << "/"  << g_nUniqueSequence++ << "/" <<  time(0);


	GString strAuth;
	if (pzHTTPAuth && pzHTTPAuth[0])
	{
		BUFFER b;
		BufferInit(&b);
		uuencode((unsigned char *)pzHTTPAuth, strlen(pzHTTPAuth), &b); 
		GString strEncoded((char *)b.pBuf, b.cLen);
		BufferTerminate(&b);
		strAuth.Format("Authorization: Basic %s\r\n",(const char *)strEncoded);

	}
	
	
	// get the host 
	GString strHost(pzHost);

	
	GString strProxtDestPort;
	int nProxyDestPort = GetProfile().GetInt(pzCfgSection,"Port",false);
	if (nProxyDestPort > 0)
		strProxtDestPort << ":" << nProxyDestPort;


	GString strProxyFirewall;
	if (pzProxyFirewall && pzProxyFirewall[0])
	{
		strProxyFirewall = pzProxyFirewall;
		strHost = pzProxyFirewall;
	}
	else
		strProxyFirewall = GetProfile().GetString(pzCfgSection,"ProxyTo",false);



	if ( strProxyFirewall.Length() )
	{
		// Note: This is a sample header between NS and a standard software proxy
		// ----------------------------------------------------------------------
		// GET http://www.Domain.com/Index HTTP/1.1
		// Host: www.Domain.com
		// User-Agent: Mozilla/5.0 (Windows; U; Windows NT 5.0; en-US; rv:0.9.4) Gecko/20011019 Netscape6/6.2
		// Accept: text/xml, application/xml, application/xhtml+xml, text/html;q=0.9, image/png, image/jpeg, image/gif;q=0.2, text/plain;q=0.8, text/css, */*;q=0.1
		// Accept-Language: en-us
		// Accept-Encoding: gzip, deflate, compress;q=0.9
		// Accept-Charset: ISO-8859-1, utf-8;q=0.66, *;q=0.66
		// Keep-Alive: 300
		// Proxy-Connection: keep-alive

		// Note: This is a sample header between IE and a standard software proxy
		// ----------------------------------------------------------------------
		// GET http://www.Domain.com/Index.html HTTP/1.0
		// Accept: */*
		// Accept-Language: en-us
		// User-Agent: Mozilla/4.0 (compatible; MSIE 6.0; Windows NT 5.0)
		// Host: www.Domain.com
		// Proxy-Connection: Keep-Alive

		strDestination.Format(
			"GET http://%s%s%s%s HTTP/1.1\r\n"
			"Host: %s\r\n"
			"User-Agent: Xfer\r\n"
			"Keep-Alive: 300\r\n"
			"%s" // HTTP Authentication 
			"Proxy-Connection: keep-alive\r\n\r\n",(const char *)strProxyFirewall,strProxtDestPort.Buf(),pzConnectionFileName,(const char *)strNonCacheTag, (const char *)strProxyFirewall,(const char *)strAuth);
		
	}
	else
	{
		///////////////////////////////////////////////////////////////////////////////////////////////////
		//				Sample HTTP GET from Netscape 6.2 (http headers say it's still 6.1)
		// If they fix that - you'll never know what version of 6.2 you have by looking at the HTTP headers.
		///////////////////////////////////////////////////////////////////////////////////////////////////
		//GET /index.html HTTP/1.1												
		//Host: 192.168.1.104													
		//User-Agent: Mozilla/5.0 (Windows; U; Windows NT 5.0; en-US; rv:0.9.2) Gecko/20010726 Netscape6/6.1	
		//Accept: text/xml, application/xml, application/xhtml+xml, text/html;q=0.9, image/png, image/jpeg, image/gif;q=0.2, text/plain;q=0.8, text/css, */*;q=0.1 
		//Accept-Language: en-us												
		//Accept-Encoding: gzip,deflate,compress,identity						
		//Accept-Charset: ISO-8859-1, utf-8;q=0.66, *;q=0.66					
		//Keep-Alive: 300														
		//Connection: keep-alive												

		///////////////////////////////////////////////////////////////////////////////////////////////////
		//								Sample HTTP GET from IE 6.0
		///////////////////////////////////////////////////////////////////////////////////////////////////
		//GET /Index.html HTTP/1.1												
		//Accept: image/gif, image/x-xbitmap, image/jpeg, image/pjpeg, application/vnd.ms-excel, application/msword, */*			
		//Accept-Language: en-us												
		//Accept-Encoding: gzip, deflate										
		//User-Agent: Mozilla/4.0 (compatible; MSIE 6.0; Windows NT 5.0)		
		//Host: 127.0.0.1														
		//Connection: Keep-Alive												

		strDestination.Format(
			"GET %s%s HTTP/1.1\r\n"		
			"Host: %s\r\n"
			"User-Agent: Xfer\r\n"
			"Keep-Alive: 300\r\n"
			"Accept: */*\r\n"
			"%s" // HTTP Authentication 
			"Connection: keep-alive\r\n\r\n",pzConnectionFileName, (const char *)strNonCacheTag, (const char *)strHost,(const char *)strAuth);

	}
}


class MessagePollData
{
public:
	// always filled
	GString strServer; 
	int nPort;
	GString strProxyTo; 
	GString strSwitchBoardPath; 

	// if using HTTP authentication the make a request outside the lcoal network
	GString strLocalNetworkAuthenticate; // "user:password"

	// a valid value if this structure resides in a g_msgPollSenders
	GString strSender;
	GString strPassword;
	GString strUserName; 
	GString strPollPathAndName; 
	GString strSectionName; 

	// empty unless this structure reside in a g_msgPollSites
	GStringList lstPollFor;
};


// a MessagePollData per unique(Server + Port + ProxyTo)
GBTree g_msgPollSites;

// a MessagePollData per known 'send to'
GBTree g_msgPollSenders;


void *messagePollUpdateThread(void *arg)
{
	// load defaults for all locations
	GString strServer;
	int nPort;
	GString strProxyTo;
	if ( GetProfile().GetBoolean("Messaging","UseBrowserProxy",false) )
	{
		strServer = GetProfile().GetString("Messaging","BrowserProxy",false); 
		nPort = GetProfile().GetInt("Messaging","BrowserProxyPort",false);
		strProxyTo = GetProfile().GetString("Messaging","DefaultSwitchBoardServer",false); 
	}
	else
	{
		strServer = GetProfile().GetString("Messaging","DefaultSwitchBoardServer",false); 
		nPort = GetProfile().GetInt("Messaging","DefaultSwitchBoardPort",false);
	}

	GStringList lstSenders(",",GetProfile().GetString("Messaging","AcceptFrom",false)); 
	GStringIterator it(&lstSenders);
	while (it())
	{
		MessagePollData *pMPD = new MessagePollData;

		pMPD->strSender = it++;
		pMPD->strSectionName = "MsgFrom-";
		pMPD->strSectionName += pMPD->strSender;
		
		if ( !GetProfile().GetBoolean(pMPD->strSectionName,"Enable",false) || !GetProfile().GetBoolean(pMPD->strSectionName,"CheckAtSwitchBoard",false) )
		{
			delete pMPD;
			continue;
		}

	
		pMPD->strLocalNetworkAuthenticate = GetProfile().GetString("Messaging","LocalNetworkAuthenticate",false); 


		int bUseProxy = GetProfile().GetBoolean("Messaging","UseBrowserProxy",false);
		if (bUseProxy)
		{
			pMPD->strServer = strServer;
			
			GString str = GetProfile().GetString(pMPD->strSectionName,"SwitchBoardServer",false); 
			if (str.Length())
				pMPD->strProxyTo = str;
			else
				pMPD->strProxyTo = strProxyTo;


			pMPD->nPort = nPort;

		}
		else
		{
			GString str = GetProfile().GetString(pMPD->strSectionName,"SwitchBoardServer",false); 
			if (str.Length())
				pMPD->strServer = str;
			else
				pMPD->strServer = strServer;
			
			pMPD->nPort = GetProfile().GetInt(pMPD->strSectionName,"SwitchBoardPort",false);
			if (pMPD->nPort < 1)
				pMPD->nPort = nPort;
		
			pMPD->strProxyTo = "";
		}

		// load the specific recipient override settings
		pMPD->strPollPathAndName = GetProfile().GetString(pMPD->strSectionName,"Name",false); 

		pMPD->strPassword = GetProfile().GetString(pMPD->strSectionName,"Password",false); 

		// build poll location key (Server + Port + ProxyTo)
		GString strPollLocation;
		strPollLocation << pMPD->strServer << pMPD->nPort << pMPD->strProxyTo;
		
		if (pMPD->strPollPathAndName.Length())
		{
			const char *pPollName = GDirectory::LastLeaf( pMPD->strPollPathAndName );
			
			MessagePollData *pMPDList = (MessagePollData *) g_msgPollSites.search(strPollLocation);
			if (pMPDList)
			{
				
				pMPDList->lstPollFor.AddLast(pPollName);
			}
			else
			{
				pMPD->lstPollFor.AddLast(pPollName);
				
				char *pzPollPathAndName = (char *)(const char *)pMPD->strPollPathAndName;
				int i=strlen(pzPollPathAndName)-1;
				for(;pzPollPathAndName[i]; i-- )
					if (pzPollPathAndName[i] == '\\' || pzPollPathAndName[i] == '/')
						break;
				char ch = pzPollPathAndName[i+1];
				pzPollPathAndName[i+1] = 0;
				pMPD->strSwitchBoardPath = pzPollPathAndName;
				pzPollPathAndName[i+1] = ch;

				g_msgPollSites.insert(strPollLocation, (void *)pMPD);
			}
		}

		g_msgPollSenders.insert(pMPD->strSender, (void *)pMPD);
	}
return 0;
}


class MessageSendArgs
{
public:
	// address of Switchboard server UNLESS sending through a proxy-firewall
	// in that case "strServer" should contain the address of the proxy-firewall.
	GString strServer; 
	
	// port of Switchboard server (80) UNLESS sending through a proxy-firewall
	// in that case "nPort" should contain the port of the proxy-firewall. (80,81,8080,3128,???)
	int nPort;

	// EMPTY, unless (strServer & nPort) refer to a proxy-firewall.  If they do
	// then "strSwitchBoardServer" is the address of the strSwitchBoardServer that
	// "strServer" should pass the message on to.
	GString strSwitchBoardServer;


	// If the internal network is configured to prompt a browser for a user name/password
	// and it's using the "Basic" authentication of HTTP, then this value should be in the form
	// "user:password" 
	GString strBasicHTTPAuth;

	//////////////////////////////////
	// This is the data that is the content of the message, it my be supplied either as a file
	// on disk as fully qualified path and file name to send: like "c:\Path\File.doc" or "/usr/bin/file.doc"
	GString strFileToSend;
	// or you may load the message into this GString
	GString strMessageToSend;
	//////////////////////////////////


	// a value like (/PublicPath/), value is defined at the [SwitchboardServer] config file
	// and must be known by both ends polling into it.  Value must begin AND end with a '/'
	GString strPublicPathName;

	// the name that you will exchange the file under, If the 'poller' is configured
	// to poll for "/PublicPath/eSocks.com" then you will meet under the name 'eSocks.com'
	// This value should NOT contain the path (/PublicPath/) just the name.
	GString strConnectName;


	// name of the 'user' sending, "strConnectName" is like the domain or company name
	// and "strUserName" may specify the purpose of the file or decoding procedures for the
	// receiver.
	GString strUserName;
	

	// the title of the message like "file.doc" that will be written on the remote machine.
	// if the remote machine has specifically granted access for this connection then you
	// can use a value like this "\\\\CorpServer\\Dept21\\Project777\\PutThis.doc".
	// To enable unrestricted values the set "[MsgFrom-???] LetSenderPlaceFile=yes" in 
	// config file of the polling machine.
	GString strRemoteTitle;

	// after all the data has been sent from the switchboard to the receiver
	// this value is sent back not to confirm delivery but to confirm transmit.
	// This value can be used to later certify the delivery after the receiver
	// has acknowledged that the data arrived and was in the expected format
	//GString strDeliveryID;


	// Any value in strResopnse, guarantees that the message was delivered.
	GString strResponse;


	// when set to any non-zero number MessageSend() will immediately return
	// and the caller is responsible for checking the results at a later time.
	int bExecuteAsync;

	/////////////////////////////////////////////////////////////////////////////
	// the following arguments are 'out' args that are assigned by MessageSend()
	/////////////////////////////////////////////////////////////////////////////
	// updated realtime to indicate # of bytes sent
	int nTotalDataSent;
	
	// the number of bytes being sent can be watched to show progress
	int nFileBytesToSend;

	// 1 - in progress
	// 2 - done
	// any other value is an error code
	int nResult;
	GString strErrorDescription;
	MessageSendArgs()
	{
		bExecuteAsync = 0;
		nTotalDataSent = 0;
	}
};

// POSTS args->strFileToSend to the switchboard
int POSTMessage(int sockfd, MessageSendArgs	*args, GString &strData)
{
	// build the POST header

	// determine the size of the data to send
	long lBytes = (long)args->strMessageToSend.Length();
	FILE *fp = 0;
	if (args->strFileToSend.Length())
	{
		fp = fopen(args->strFileToSend,"rb");
		if (!fp)
		{
			args->strErrorDescription.Format("Failed to open local file[%s] to send.",args->strFileToSend.Buf());
			args->nResult = 8;
			return 0; 
		}

		// get the size of the file
		fseek(fp,0,SEEK_END);
		lBytes = ftell(fp);
		args->nFileBytesToSend = lBytes;
		fseek(fp,0,SEEK_SET);
	}




	GString strBoundary;
	strBoundary.Format("---------------------------%010d",time(0));

	GString strBoundaryHeader = "--";
	strBoundaryHeader += strBoundary;
	strBoundaryHeader += "\r\n";
	strBoundaryHeader += "Content-Disposition: form-data; name=\"File\"; filename=\"";
	strBoundaryHeader += time(0); strBoundaryHeader +="\"\r\n";
	strBoundaryHeader += "Content-Type: application/octet-stream\r\n\r\n";
	

	GString strBoundaryFooter;
	strBoundaryFooter.Format("\r\n--%s\r\nContent-Disposition: form-data; name=\"Submit\"\r\n\r\nSubmit\r\n--%s--\r\n",(const char *)strBoundary,(const char *)strBoundary);


	GString strDataInfo;
	strDataInfo.Format("%d|%s",(int)lBytes+strBoundaryFooter.Length(),(const char *)strBoundary);


	// todo: strData contains a value that needs to be hashed or ciphered, reversing it is not much security - but it does sufficiently test the process.
	strData.MakeReverse();


	// build the header
	GString strHeader;
	GString strSubSubHeader = args->strConnectName << '/' << args->strUserName << '/' << args->strRemoteTitle << '/' << strDataInfo << '/' << strData;
	GString strSubHeader("OK");
	strSubHeader += strSubSubHeader.Length();
	strSubHeader += strSubSubHeader;

	// file + subheader + boundary header + boundary footer (with leading -- and trailing -- (4) )
	int nContentPostLen = lBytes + (int)strSubHeader.Length() + (int)strBoundaryHeader.Length() + (int)strBoundary.Length() + 4;

	// http://www.w3.org/Protocols/rfc1341/7_2_Multipart.html

	GString strAuth;
	if (args->strBasicHTTPAuth.Length())
	{
		BUFFER b;
		BufferInit(&b);
		uuencode((unsigned char *)(const char *)args->strBasicHTTPAuth, (int)args->strBasicHTTPAuth.Length(), &b); 
		GString strEncoded((char *)b.pBuf, b.cLen);
		BufferTerminate(&b);
		strAuth.Format("Authorization: Basic %s\r\n",(const char *)strEncoded);
	}


	GString strIPHost;
	char pzHostName[512];
	gethostname(pzHostName,512); //uname() on some systems
	struct hostent *pHELocal = (struct hostent *)gethostbyname((const char *)pzHostName);
	struct sockaddr_in my_addr; 
	memcpy((char *)&(my_addr.sin_addr), pHELocal->h_addr,pHELocal->h_length); 
	memset(&(my_addr.sin_zero),0, 8);// zero the rest of the (unused) struct
	strIPHost = inet_ntoa(my_addr.sin_addr);


	if ( args->strSwitchBoardServer.Length() )
	{
		// todo: this assumes we always connect on port 80 after passing through the HTTP proxy - it should be allowed to go anywhere

		strHeader = "POST http://";
		strHeader += args->strSwitchBoardServer;
		strHeader += "/ HTTP/1.0\r\n";
		strHeader += "Accept: image/gif, image/x-xbitmap, image/jpeg, image/pjpeg, application/vnd.ms-excel, application/msword, */*\r\n";
		strHeader += "Accept-Language: en-us\r\n";
		strHeader += "Content-Type: multipart/form-data; boundary="; strHeader += strBoundary; strHeader += "\r\n";
		strHeader += "Proxy-Connection: Keep-Alive\r\n";
		strHeader += "User-Agent: Mozilla/4.0 (compatible; MSIE 6.0; Windows NT 5.0)\r\n";
		strHeader += "Host: "; strHeader += args->strSwitchBoardServer; strHeader +="\r\n";
		strHeader += "Content-Length: "; strHeader += nContentPostLen; strHeader +="\r\n";
		strHeader += strAuth;
		strHeader += "Pragma: no-cache\r\n\r\n";
	}
	else
	{
		strHeader = "POST / HTTP/1.0\r\n";
		strHeader += "Accept: image/gif, image/x-xbitmap, image/jpeg, image/pjpeg, application/vnd.ms-excel, application/msword, */*\r\n";
		strHeader += "Accept-Language: en-us\r\n";
		strHeader += "Content-Type: multipart/form-data; boundary="; strHeader += strBoundary; strHeader += "\r\n";
		strHeader += "Proxy-Connection: Keep-Alive\r\n";
		strHeader += "Connection: Keep-Alive\r\n";
		strHeader += "User-Agent: Mozilla/4.0 (compatible; MSIE 6.0; Windows NT 5.0)\r\n";
		strHeader += "Host: "; strHeader += strIPHost; strHeader +="\r\n";
		strHeader += "Content-Length: "; strHeader += nContentPostLen; strHeader +="\r\n";
		strHeader += strAuth;
		strHeader += "Pragma: no-cache\r\n\r\n";
	}
	strHeader += strBoundaryHeader;
	strHeader += strSubHeader;

	char pFileSendBuffer[8192];


	// send it all in a single send() for small messages
	if (strHeader.Length() + lBytes < 8192)
	{
		if (args->strMessageToSend.Length())
		{
			strHeader << args->strMessageToSend;
		}
		else
		{
			fread(pFileSendBuffer,sizeof(char),lBytes,fp);
			strHeader.write(pFileSendBuffer,lBytes);
		}
		
		
		strHeader += strBoundaryFooter;

		int nHeadSent = nonblocksend(sockfd, (const char *)strHeader, (int)strHeader.Length());
		if ( nHeadSent != strHeader.Length() )
		{
			args->strErrorDescription.Format("Failed to send Message code:%d:%d",(int)nHeadSent,(int)SOCK_ERR);
			args->nResult = 9;
			if (fp)
				fclose(fp);
			return 0; 
		}
	}
	else // chop up the message and send it
	{
		// now send the header
		int nHeadSent = nonblocksend(sockfd, (const char *)strHeader, (int)strHeader.Length());
		if ( nHeadSent != strHeader.Length() )
		{
			args->strErrorDescription.Format("Header send failed %d:%d \n\n",(int)nHeadSent,(int)SOCK_ERR);
			args->nResult = 10;
			fclose(fp);
			return 0; 
		}
		else
		{
			// now send the data, in this case a file. 
			int nSendStringIndex = 0;
			while(lBytes > 0)
			{
				int nChunkSize = (lBytes < 8192) ? lBytes : 8192; 

				if (args->strMessageToSend.Length())
				{
					memcpy(pFileSendBuffer,args->strMessageToSend.StartingAt(nSendStringIndex),nChunkSize);
					nSendStringIndex += nChunkSize;
				}
				else
				{
					if (fread(pFileSendBuffer,sizeof(char),nChunkSize,fp) != (unsigned int)nChunkSize)
					{
						InfoLog(146,"Failed to read from source send file\n");
						break; 
					}
				}
				lBytes -= nChunkSize;


				
				int nRslt = nonblocksend(sockfd, pFileSendBuffer, nChunkSize);
				if ( nRslt < 1 )
				{
					args->strErrorDescription.Format("Failed to send file chunk %d:%d",(int)nRslt,(int)SOCK_ERR);
					args->nResult = 11;
					break;
				}
				else if ( nRslt != nChunkSize )
				{
					// if this ever happens
					args->strErrorDescription.Format("Failed to send %d:%d:%d",(int)nRslt,(int)SOCK_ERR,(int)nChunkSize);
					args->nResult = 12;
					break;
				}
				else
				{
					// just sent [nChunkSize] bytes of file
					args->nTotalDataSent += nChunkSize;
				}
			}
			
			// send boundary footer, if that's all that's left to send
			if (lBytes == 0)
			{
				nonblocksend(sockfd, (const char *)strBoundaryFooter, (int)strBoundaryFooter.Length());
			}
			else
			{
				if (fp)
					fclose(fp);

				return 0; // error
			}
		}
	}
	if (fp)
		fclose(fp);
	
	return 1;
}

int MessagingPlugin(DriverCallBackArg *prd, const char *pzData, int nLength, char *pBuf, int nBufSize, const char *pzPluginhandler, GString *pstrReply)
{
	GStringList lstArgs("::",pzPluginhandler);
	GStringIterator it(&lstArgs);
	const char *pzLanguage = 0;
	const char *pzComponent = 0;
	const char *pzInterface = 0;
	const char *pzMethod = 0;
	if (it())
		pzLanguage = it++;
	if (it())
		pzComponent = it++;
	if (it())
		pzInterface = it++;
	if (it())
		pzMethod = it++;

	if (!pzLanguage || !pzComponent || !pzInterface || !pzMethod)
	{
		InfoLog(64,"Messaging Plugin Handler is misconfigured.  Should be... Language::Component::Interface::Method");
		return 0;
	}

	InterfaceInstance *pII = GetInterfaceInstance(pzLanguage);
	if (!pII)
	{
		GString strError;
		strError << pzLanguage << " is not configured correctly or is not a supported component type on this machine.";
		InfoLog(64,strError);
		return 0;
	}


	// setup the driver options
	char pzPIBuf[1024];
	pzPIBuf[0] = 0;
	pII->SetOption("piBuf",pzPIBuf);
	pII->SetOption("callback",(void *)DriverCallBack);
	pII->SetOption("callbackArg",prd );
	pII->SetOption("wkBuf",pBuf);
	pII->SetOption("wkBuf2",0);
	int n = nBufSize;
	pII->SetOption("wkBufSize",(void *)n);


	//
	// prepare arguments for Invoke() call
	//
	GString strArgSizes;
	strArgSizes << nLength;

	try
	{
		int nOutResultSize = 0;
		const char *pzResults = pII->InvokeEx( pzComponent, pzInterface, pzMethod, pzData,strArgSizes,&nOutResultSize);
		if (pstrReply)
			pstrReply->write(pzResults,nOutResultSize);

		if (stristr(pzPIBuf,"DataSent") != 0)
		{
			// the plugin sent the data directly
			if (pstrReply)
			{
				pstrReply->Empty();
				(*pstrReply) << "DataSent";
			}
		}
		return 1;
	}
	catch( GException &rErr )
	{
		InfoLog(161,rErr.GetDescription());
	}
	return 0;
}



int ParseMessagingResponse(int fd, int bSenderPlaceFile, const char *pzOutFile, char *pContentData, int nContentLen, char *Buf, int nBufSize, int nBytes, GString &strCertifyData, GString *pstrDest, const char *pzPluginhandler, GString *pstrReply )
{
	GString strSender;
	GString strRecvr;
	GString strDataInfo;
	GString strTitle;


	int nSubHeaderLen = atoi(&pContentData[2]); // past the "OK" is the size

	// walk past the Length, point at the first non-numerical byte 
	// counting the numerical bytes
	int nNumericals=0;
	while (pContentData[nNumericals+2] >= '0' && pContentData[nNumericals+2] <= '9')
		nNumericals++;

	// data starts past "OK" NNN and subheader
	char *pDataStart = &pContentData[2+nNumericals+nSubHeaderLen];

	int AllHeadersLen = pDataStart - Buf;

	// read in the whole sub-header if we didn't get it in the previous recv
	if (AllHeadersLen > nBytes)
	{
		// get more data
		int nMore = nonblockrecv(fd, &Buf[nBytes],nBufSize - nBytes);
		if (nMore > 0)
		{
			nBytes += nMore;
		}
		else
		{
			// printf("POLL result- Read of more header failed %d\n",SOCK_ERR);
			nBytes = -1;
		}

	}


	int nTotalMessageSize = nContentLen - 2 - nNumericals - nSubHeaderLen;
	int nDataWithHeader = nBytes - AllHeadersLen;
	int nBytesToGet = nTotalMessageSize - nDataWithHeader;




	// "OK28recvr/sender/title/data info/certify/data"
	GString strArgs(&pContentData[2+nNumericals],nSubHeaderLen);

	GStringList lstArgs("/",strArgs);
	GStringIterator itArgs(&lstArgs);
	if (itArgs())
		strRecvr = itArgs++;
	if (itArgs())
		strSender = itArgs++;
	if (itArgs())
		strTitle = itArgs++;
	if (itArgs())
		strDataInfo = itArgs++;
	if (itArgs())
		strCertifyData = itArgs++;


	GString strOutFile; // the disk location.


	// if we let the trusted sender decide where to put the file
	if (bSenderPlaceFile)
	{
		strOutFile = strTitle;  // could be "\\CorpServer\department21\project777\myfile.doc"
	}
	else // or we control where the file goes
	{
		if (pzOutFile)
		{
			// pTCD->pzPollSectionName = "MsgFrom-Domain"
			strOutFile = pzOutFile; //GetProfile().GetPath(pTCD->pzPollSectionName,"DiskLocation",false);

			// make the destination directory if it does not exist
			#ifdef _WIN32
				mkdir(ReSlash(strOutFile));
			#else
				mkdir(ReSlash(strOutFile),777);
			#endif


			// to stop access behind the configured root like "../../autoexec.bat"
			// and to keep from specifying subfolders like "/subdir/file.dat"
			// and to keep the file name valid....
			char *pBuf = (char *)(const char *)strTitle;
			char *pBadChar = strpbrk(pBuf,"\r\n\\/:<>|");
			while (pBadChar)
			{
				*pBadChar = 'X'; // modify a const
				pBadChar = strpbrk(pBuf,"\r\n\\/:<>|");
			}
			// and it cannot start with a .
			if (pBuf[0] == '.')
				pBuf[0] = 'X';

			strOutFile += strTitle;
		}
	}


	//
	// Prepare the message destination
	//

	FILE *fp = 0;
	if (strOutFile.Length() && !(pzPluginhandler && pzPluginhandler[0]) )
	{
		fp = fopen(strOutFile,"wb"); // open local file
		if (!fp)
		{
			GString strLog;
			strLog.Format("failed to open file [%s] to receive message\n",(const char *)strOutFile);
			InfoLog(149,strLog);
			return 0; // error
		}
	}



	//int nFullLen = atoi(strDataInfo);
	char *pzBoundary = (char *)strpbrk((const char *)strDataInfo,"|");
	if (pzBoundary)
		pzBoundary++;


	if (pzPluginhandler && pzPluginhandler[0])
	{
		
		int nDataLen = nDataWithHeader;
		if (nDataWithHeader)
		{
			//int nToWrite = nDataWithHeader;
			if (pzBoundary)
			{
				char *pEnd = strstr(pDataStart,pzBoundary);
				if (pEnd)
				{
					nDataLen = (pEnd - pDataStart) - 4; // four bytes behind the boundary marker
					nBytesToGet = nDataLen;
				}
			}
		}
		else
		{
			int nChunk = (nBytesToGet > nBufSize) ? nBufSize : nBytesToGet;
			nBytes = nonblockrecv(fd, Buf, nChunk);
			if (nBytes > 0)
			{
				pDataStart = Buf;
				nDataLen = nBytes;
			}
			else
			{
				GString strLog;
				strLog.Format("error reading message %d:%d\n",(int)nBytes,(int)SOCK_ERR);
				InfoLog(151,strLog);
				return 0;
			}
		}

		// prepare the information available to the driver callback
		DriverCallBackArg prd;
		prd.de = 0; // cipher N/A on anonymous HTTP
		prd.en = 0;
		prd.fd = fd;
		prd.nProtocol = 3;
		prd.tid = 7;			// N/A in Messaging
		prd.strUser = strSender;
		prd.pnKeepAlive = 0;	// N/A in Messaging
		prd.nCompleteSize = nBytesToGet;
		prd.nObtainedCount = nDataLen;
		prd.sDataFlags = 0;		// N/A in Messaging
		prd.nBuf1Size = nBufSize;
		prd.strBoundary = pzBoundary;
		prd.strArgs = strArgs;

		try 
		{
			MessagingPlugin(&prd, pDataStart, nDataLen, Buf, nBufSize, pzPluginhandler, pstrReply);
		}
		catch( GException &rErr )
		{
			GString strMsg("MessagingPlugin Message:");
			strMsg << rErr.GetDescription();
			InfoLog(138,strMsg);
		}
		catch(...)
		{
			GString strMsg("Fatal error in MessagingPlugin.");
			InfoLog(139,strMsg);
		}


		return 1;	
	}


	// if data arrived with the header in the first read, write it now.
	if (nDataWithHeader)
	{
		
		int nToWrite = nDataWithHeader;
		if (pzBoundary)
		{
			char *pEnd = strstr(pDataStart,pzBoundary);
			if (pEnd)
			{
				nToWrite = (pEnd - pDataStart) - 4; // four bytes behind the boundary marker
			}
		}
		
		// put the data somewhere
		if (fp)
		{
			fwrite(pDataStart,1,nToWrite,fp);
		}
		else if (pstrDest)
		{
			pstrDest->write(pDataStart,nToWrite);
		}
	}

	while(nBytesToGet > 0)
	{
		int nChunk = (nBytesToGet > nBufSize) ? nBufSize : nBytesToGet;
		nBytes = nonblockrecv(fd, Buf, nChunk);

		if (nBytes > 0)
		{
			
			// shorten the last packet
			int nToWrite = nBytes;
			if (pzBoundary && nBytesToGet-nBytes < 256)
			{
				// walk back from the end and stop at a null or 256
				int nEndIndex = nBytes -1;
				while(Buf[nEndIndex] && nBytes - nEndIndex < 256)
					nEndIndex--;	

				char *pEnd = strstr(&Buf[nEndIndex+1],pzBoundary);
				if (pEnd)
				{
					nToWrite = (pEnd - Buf) - 4; // four bytes behind the boundary marker
				}
			}
			
			// put the data somewhere
			if (fp)
			{
				fwrite(Buf,1,nToWrite,fp);
			}
			else if (pstrDest)
			{
				pstrDest->write(Buf,nToWrite);
			}

			nBytesToGet -= nBytes;
		}
		else
		{
			GString strLog;
			strLog.Format("error reading message %d:%d\n",(int)nBytes,(int)SOCK_ERR);
			InfoLog(151,strLog);
			break;
		}
		
	}
	if (fp)
		fclose(fp);
	return 1;
}



void *MessageSend(void *argin)
{
	MessageSendArgs	*args = (MessageSendArgs *)argin;
	if (args->bExecuteAsync)
	{
		gthread_t Async_thr;
		args->bExecuteAsync = 0;
		gthread_create(&Async_thr,	NULL, MessageSend, argin );
		return 0;
	}
	
	args->nResult = 1; // in progress

	GString strIPHost;
	char pzHostName[512];
	gethostname(pzHostName,512); //uname() on some systems
	struct hostent *pHELocal = (struct hostent *)gethostbyname((const char *)pzHostName);
	struct sockaddr_in my_addr; 
	memcpy((char *)&(my_addr.sin_addr), pHELocal->h_addr,pHELocal->h_length); 
	memset(&(my_addr.sin_zero),0, 8);// zero the rest of the (unused) struct
	strIPHost = inet_ntoa(my_addr.sin_addr);

//	FILE *fp = 0;
	GString strConnected;
	GString strData;

	int nRetries = 0;
LOOP_IT:
	args->nTotalDataSent = 0;
	int sockfd = DoConnect(args->strServer, args->nPort,5555,5555);
	if (sockfd == -1)
	{
		args->strErrorDescription.Format("Failed to connect to %s on port %d",args->strServer.Buf(), (int)args->nPort);
		args->nResult = 3;
		return 0;
	}


	// "/PublicPath/put/Domain"
	GString strCommand;
	strCommand.Format("%sput/%s",(const char *)args->strPublicPathName,(const char *)args->strConnectName);

	GString strAuth;
	if (args->strBasicHTTPAuth.Length())
	{
		BUFFER b;
		BufferInit(&b);
		uuencode((unsigned char *)(const char *)args->strBasicHTTPAuth, (int)args->strBasicHTTPAuth.Length(), &b); 
		GString strEncoded((char *)b.pBuf, b.cLen);
		BufferTerminate(&b);
		strAuth.Format("Authorization: Basic %s\r\n",(const char *)strEncoded);
	}



	GString strConnectionName;

	// generate a "Browser Issued" GET command requesting a connection.
	if ( args->strSwitchBoardServer.Length() )
	{
		// GET http://www.Domain.com/PublicPath/put/Domain%nnnnnnnnnn HTTP/1.1
		strConnectionName.Format(
		"GET http://%s%s%%%d HTTP/1.1\r\n"
		"Accept: image/gif, image/x-xbitmap, image/jpeg, image/pjpeg, application/vnd.ms-excel, application/msword, */*\r\n"
		"Accept-Language: en-us\r\n"
		"Accept-Encoding: gzip, deflate\r\n"
		"User-Agent: Mozilla/4.0 (compatible; MSIE 6.0; Windows NT 5.0)\r\n"
		"Host: %s\r\n" 
		"%s"
		"Connection: Keep-Alive\r\n\r\n",(const char *)args->strSwitchBoardServer,(const char *)strCommand, time(0),(const char *)strAuth );
	}
	else
	{
		strConnectionName.Format(
			"GET %s%%%d HTTP/1.1\r\n"
			"Accept: image/gif, image/x-xbitmap, image/jpeg, image/pjpeg, application/vnd.ms-excel, application/msword, */*\r\n"
			"Accept-Language: en-us\r\n"
			"Accept-Encoding: gzip, deflate\r\n"
			"User-Agent: Mozilla/4.0 (compatible; MSIE 6.0; Windows NT 5.0)\r\n"
			"Host: %s\r\n"
			"%s"
			"Connection: Keep-Alive\r\n\r\n",(const char *)strCommand,time(0),(const char *)strIPHost,(const char *)strAuth);
	}


	if ( nonblocksend(sockfd, (const char *)strConnectionName, (int)strConnectionName.Length()) == -1 )
	{ 
		args->strErrorDescription.Format("nonblocksend() failed on Poll Thread code:%d",(int)SOCK_ERR);
		args->nResult = 4;
		PortableClose(sockfd,"Core13");
		return 0;
	}

	
	char Buf[8192];
	char *pContentData = Buf;
	int nContentLen = 0;
	int nBytes = nonblockrecvHTTP(sockfd, Buf, 8192, &pContentData, &nContentLen,0,90,0);

	if (nBytes > 0 && !pContentData[0] && nContentLen) 
	{
		IOBlocking(sockfd, 0);
		int nMore = recv(sockfd, pContentData,8192 - nBytes,0);
		IOBlocking(sockfd, 1);
		if (nMore > 0)
		{
			nBytes += nMore;
		}
		else
		{
			GString str;
			str.Format("Failed to read in data following header[%d]\n",(int)SOCK_ERR);
			InfoLog(145,str);

			nBytes = -1;
		}
	}


	if (nBytes != -1)
	{
		if (pContentData[0] == 'O' && pContentData[1] == 'K')
		{
			char *pzSlash = strpbrk(pContentData,"/");
			if (pzSlash)
			{
				*pzSlash = 0;
				strConnected = pzSlash + 1;
				strData = &pContentData[2];
			}
		}
		else if (pContentData[0] == 'N' && pContentData[1] == 'O')
		{
			if (nRetries++ < 5)
				goto LOOP_IT; 

			args->strErrorDescription = "No pickup at SwitchBoardServer";
			args->nResult = 13;
			PortableClose(sockfd,"Core14");

			return 0;

		}
		else
		{
			// probably an HTTP page from a proxy-firewall indicating 
			// that the server could not be connected to, or a 404 from the
			// switchboard indicating a bad switch path.
			
			char *pEnd = strpbrk(pContentData,"\r\n");
			if (pEnd)
				*pEnd = 0;
			
			// HTTP 1.1 404 Not Found
			// HTTP 1.1 401 Unauthorized
			args->strErrorDescription = pContentData;

			args->nResult = 5;
			PortableClose(sockfd,"Core15");
			return 0;
		}
	}
	else
	{
		args->strErrorDescription.Format("nonblocksend() failed on Poll Thread code:%d",(int)SOCK_ERR);
		args->nResult = 6;
		PortableClose(sockfd,"Core16");
		return 0;
	}

// ************************************************************
// * To be sure that this is truly connectionless we can 
// * hang up and reconnect here without transaction failure.
// * this code will not cause the transaction to fail. - (leave it commented out anyhow).
// ************************************************************
//
//	PortableClose(sockfd);
//	sockfd = DoConnect(args->strServer, args->nPort,4444,4444);
///////////////////////////////////////////////////////////////

	
// ************************************************************
// * Now send the POST data
// ************************************************************
	int nPOSTRetries = 0;
POSTRetry:
	int nRet = POSTMessage(sockfd, args, strData);			// POSTS args->strFileToSend
	if (nRet == 0)
	{	// failed
		PortableClose(sockfd,"Core17");

		if (nPOSTRetries++ < 2)
		{
			sockfd = DoConnect(args->strServer, args->nPort,4444,4444);
			if (sockfd == -1)
			{
				args->strErrorDescription.Format("Failed to connect to %s on port %d",args->strServer.Buf(), (int)args->nPort);
				args->nResult = 7;
				return 0;
			}
			goto POSTRetry;
		}
	}
	else
	{

	// ************************************************************
	// ****************** now read the REPLY **********************
	// ************************************************************

		nBytes = nonblockrecvHTTP(sockfd, Buf, 8000, &pContentData, &nContentLen,0,90,0);
		if (nBytes > 0)
		{
			GString strCertifyData;
			GString strDest;
			ParseMessagingResponse(sockfd, 0, 0, pContentData, nContentLen, Buf, 8192, nBytes, strCertifyData, &args->strResponse, 0 , 0);

			args->nResult = 2;
			args->strErrorDescription = "Success";
		}
		else
		{
			args->strErrorDescription = "No delivery confirmation";
			args->nResult = 50;
		}
	}


	// todo: cache sockfd with a key of  args->strServer + args->nPort
	PortableClose(sockfd,"Core18");


	return 0;
}

int g_PollNow = 0;
void PollExecSleep(int nSeconds)
{
	int nSlept = 0;
	while(nSlept++ < nSeconds)
	{
		PortableSleep(1,0);
		if (g_PollNow)
		{
			g_PollNow = 0;
			return;
		}
	}
}


int g_PollThreadID = 4999; // start poll threadID at 5000
long g_PollThreadsConnected = 0; 
// issued by SwitchBoardPoll()
void *PollExecThread(void *arg)
{
	ThreadConnectionData *pTCD = (ThreadConnectionData *)arg;

	time_t tLastFailure = 0;
	time_t tCase2Timer = 0;
	int nFailureCounter = 0;
	int nFailureCounter2 = 0;
	int nFailureCase = 0;

	pTCD->nPollThreadID = ++g_PollThreadID;


POLL_POOL:

	// if this is the 2nd, 3rd, or 4th etc in a queue that has not made a connection to the 
	// switchboard  on the 1st or has had no connection pickup on the 1st that has made it to the switchboard
	if ( pTCD->pBaseTCD != pTCD && (time(0) - pTCD->pBaseTCD->lLastActivity > 10) ) 
	{
		PortableSleep(2,0);
		goto POLL_POOL;
	}


	int bFoundConnection = 0;
	char Buf[8192];

	int nPort = pTCD->pTSD->nPort; // pTCD->pTSD->nPort will be -777 to identify this connection as having been polled rather than accept()ed
	if (nPort < 1)
	{
		nPort = GetProfile().GetInt(pTCD->pzPollSectionName,"Port",false);
		if (nPort < 1)
			nPort = 80;
		int nHTTPProxyPort = GetProfile().GetIntOrDefault(pTCD->pzPollSectionName,"HTTPProxyPort",0);
		if (nHTTPProxyPort)
		{
			nPort = nHTTPProxyPort;
		}
	}


	int fd = DoConnect(pTCD->pTSD->szProxyToServer, nPort, pTCD->nPollThreadID ,pTCD->nPollThreadID);
	if (fd == -1)
	{
		pTCD->nLastPollConnectStatus = 700;
		// failed connect falls through to close this connection, same as 'goto CLOSE_POLL;'
	}
	else
	{
		pTCD->nLastPollConnectStatus = 1;

		// set the connection attributes
		IOBlocking(fd, 1);	// Set non blocking IO

		GString strConnectionName; // contains results of BuildConnectionPostData()
		
		// generate a GET command requesting a connection.
		BuildConnectionPostData(pTCD->nPollThreadID, strConnectionName, pTCD->pTSD->pzPostConnectPathAndName, pTCD->pzPollSectionName, pTCD->pTSD->szHTTPProxyFirewall, pTCD->strLocalNetworkAuthenticate,pTCD->pTSD->szProxyToServer );

		if ( nonblocksend(fd, (const char *)strConnectionName, (int)strConnectionName.Length()) == -1 )
		{ 
			pTCD->nLastPollConnectStatus = -1;
			InfoLog(147,"nonblocksend() failed on Poll Thread");
			goto CLOSE_POLL;
		}

		Buf[0] = 0;
		char *pContentData = Buf;
		int nContentLen = 0;

#ifdef _WIN32
		InterlockedIncrement(&g_PollThreadsConnected);
#else
		g_PollThreadsConnected++;
#endif
		// note: nBytes will equal -10035 when [pTCD->nPollSrvWait] times out
		int nBytes = nonblockrecvHTTP(fd, Buf, 8192, &pContentData, &nContentLen, 0, pTCD->nPollSrvWait, 1);
#ifdef _WIN32
		InterlockedDecrement(&g_PollThreadsConnected);
#else
		g_PollThreadsConnected--;
#endif
		if (nBytes == -1)
		{
			pTCD->nLastPollConnectStatus = 701;
		}
		if (nBytes > -1)
		{
			// check the HTTP status code
			GString strStatus;
			strStatus.SetFromUpTo(&Buf[9],"\r\n");  //Gets a string like "503 Service Unavailable" or "401 Unauthorized"

			
			// 204 is not an error, it's a negative response to the poll that cam from the switchboard server - nobody is there to pick it up
			if (strStatus.CompareNoCase("204 No Content") == 0)
			{
				pTCD->nLastPollConnectStatus = 204;
				goto CLOSE_POLL;
			}
			else if (strStatus.CompareNoCase("200 OK") != 0)
			{
				//  look for a message like this: (a reply from the cisco pix firewall indicating a request with a bad password)
				//  ------------------------------
				//	HTTP/1.1 401 Unauthorized
				//	WWW-Authenticate: Basic realm="UBT Authenticate  (ID777)"
				//	Connection: close
				// 
				//  or like this: (a reply from the switchboard indicating a misconfigured connection path)
				//  ------------------------------
				//  HTTP/1.1 404 Object Not Found
				//
				//  or like this: (a reply from a microsoft proxy without an internet connection)
				//  ------------------------------
				//  HTTP/1.1 503 The service is unavailable
				//
				GString strErr("Poll Connect to [");
				strErr << pTCD->pTSD->szProxyToServer << "] failed [" << strStatus << "]";
				InfoLog(148,strErr);
				
				pTCD->nLastPollConnectStatus = atoi(strStatus);
				goto CLOSE_POLL;
			}
		}

		if (nBytes > 0 && (pTCD->pTSD->nProtocol == 3)) // File Transfer or Remote Console
		{
			GString strReplyTo;
			char *p = strstr(Buf,"XferReplyTo: ");
			if (p)
			{
				pTCD->pBaseTCD->lLastActivity = time(0); // if we have multiple threads in this poll queue - make sure the others are ready to accept connections too

				strReplyTo.SetFromUpTo(p+13,"\r\n");

				// This writes into the TCD, each polling thread has it's own TCD
				pTCD->pzClientIP[0] = '~';
				strcpy(&pTCD->pzClientIP[1],pTCD->pTSD->szProxyToServer);
				strcpy(pTCD->pzIP,strReplyTo.Buf());
				pTCD->sockfd = fd;

				AttachToClientThread(pTCD,g_PoolClientThread,g_ThreadPoolSize,&g_nThreadPoolRoundRobin);


				// complete this polling, dont close the connection, it is being serviced
				// by the clientThread that it was just attached to. 
				bFoundConnection = 1;
				goto FINISH_POLL;
			}
		}

		if (nBytes > -1 && !pContentData[0])
		{
			// if all we got was header - no content, wait for some content.
			if (!pContentData[0] && nContentLen) 
			{
				int nMore = nonblockrecvAny(fd, pContentData,8192 - nBytes);
				if (nMore > 0)
				{
					nBytes += nMore;
				}
				else
				{
					nBytes = -1;
				}
			}
		}

		if (nBytes > -1)
		{
			if (pContentData[0] == 'O' && pContentData[1] == 'K')
			{
				bFoundConnection = 1;



				if (pTCD->pTSD->nProtocol == 8) // [Messaging] poll
				{
					int bSenderPlaceFile = GetProfile().GetBool(pTCD->pzPollSectionName,"LetSenderPlaceFile",false);
					GString strOutFileFolder = GetProfile().GetPath(pTCD->pzPollSectionName,"DiskLocation",false);
					
					
					GString strHandler;
					if ( GetProfile().GetBool("Messaging","UsePluginHandler",false) )
					{
						strHandler = GetProfile().GetString("Messaging","PluginHandler",false);
					}

					MessageSendArgs args;


					// extract the message content, and finish reading it in if necessary
					GString strCertifyData;
					ParseMessagingResponse(fd, bSenderPlaceFile,strOutFileFolder, pContentData, nContentLen, Buf, 8192, nBytes , strCertifyData, 0, strHandler, &args.strMessageToSend);


					// send an auto-reply, if no handler created a custom reply
					if (args.strMessageToSend.Compare("DataSent") != 0)
					{
						if (args.strMessageToSend.IsEmpty())
							args.strMessageToSend.Format("Your transmission was received in full at[%s]",g_strThisHostName.Buf());

						if ( GetProfile().GetBool("Messaging","UseBrowserProxy",0 ) )
						{
							// pTCD->pzPollSectionName is the "MsgFrom-*" for the account we are dealing with
							args.strSwitchBoardServer = GetProfile().GetString(pTCD->pzPollSectionName,"SwitchBoardServer",false);
							args.strBasicHTTPAuth = pTCD->strLocalNetworkAuthenticate;
						}
						args.strPublicPathName = GetProfile().GetString(pTCD->pzPollSectionName,"Name",false);
						args.strPublicPathName.SetLength(args.strPublicPathName.ReverseFind('/')  + 1);
						args.strConnectName = strCertifyData;
						args.strUserName = "UBT";
						args.strRemoteTitle = "Confirmation";


						int nPOSTMessageRetries = 0;
						while( !POSTMessage(fd, &args, strCertifyData) )
						{
							if (nPOSTMessageRetries++ > 1)
								break;

							// if the POST fails, try in on a new connection - it may be that a proxy server did not keep the connection alive
							PortableClose(fd,"Core19");


							fd = DoConnect(pTCD->pTSD->szProxyToServer, nPort, pTCD->nParentThreadID ,pTCD->nParentThreadID);
							IOBlocking(fd, 1);
						}
					}


					// read the response to the post, and do nothing with it.
					nonblockrecvHTTP(fd, Buf, 8000, &pContentData, &nContentLen, 0, 120, 1);
				}
				else
				{
					GString strLog;
					strLog.Format("Protocol [%d] in PollExternalConnect must be 3 or 6",(int)pTCD->pTSD->nProtocol);
					InfoLog(153,strLog);
					goto CLOSE_POLL;
				}
				
				// complete this polling, dont close the connection, it is being serviced
				// by the clientThread that it was just attached to. 
				goto FINISH_POLL;
			}
			else
			{
				// there is no client waiting for the type of connection just requested.
				goto CLOSE_POLL;
			}
		}
	}
CLOSE_POLL:
	PortableClose(fd,"Core20");

FINISH_POLL:

	
	if (bFoundConnection)
	{
		pTCD->nLastPollConnectStatus = 2; // connection accepted

		// this resets the failure throttle
		nFailureCounter = 0;
		nFailureCounter2 = 0;
		nFailureCase = 0;
		tLastFailure = 0;
	}
	else
	{
		// if the value is 200-599 it's the HTTP error code - so keep the error detail for debugging
		// the 700 range is a local defined error
		// 0 indicates 'in progress' and -1 is 'unspecified error' - we may not be set at -1 yet it is now set because it is no longer 'in progress' 
		if (pTCD->nLastPollConnectStatus < 200)	
			pTCD->nLastPollConnectStatus = -1;

		// We need to be careful not to get stuck in a tight loop here.  The polled connection failed, for one of nearly countless reasons.
		// In many cases we need to immediately try again, and in others if we do we'll immediately and indefinitely get the exact same error putting this thread in a tight loop.


		// we also get here because of reaching configured timeouts.  It could be the timeout at the switchboard, then [pTCD->nLastPollConnectStatus] equals 204
		// It could also be a timeout due to local polling timeout configuration, in that case [nBytes] will equal -10035.
		// When we pass through this failure throttle due to hitting a configured timeout, we must never slow down - because that's how we are configured.


		// CASE 1: - if we have 2 failures in less than 2 seconds - slow down
		if (nFailureCase < 2) // nFailureCase 0 is the first time on error after a success, nFailureCase 1 we've already been doing this.
		{
			if (time(0) - tLastFailure < 2) // this value 2 is has dependency logic in CASE 2, if you adjust it adjust the CASE 2 calculation also
			{
				PollExecSleep(10);// this value 10 is has dependency logic in CASE 2, if you adjust it adjust the CASE 2 calculation also
				nFailureCase = 1;
			}
			else
			{
				tLastFailure = time(0);
			}
		}


		// CASE 2: - every 15 failures check to see if we are in a CASE 1 failure loop - if so slow down even more
		else if (nFailureCounter++ > 15 || nFailureCase == 2)
		{
			if (!tCase2Timer)
			{
				tCase2Timer = time(0); // this causes a longer period for the first loop, since the application was just started the other side may be starting too and we should remain quicker to retry
			}
			if ( time(0) - tCase2Timer < 120)
			{
				// 15 immediate back to back failures from a failed connection always takes exactly 98 seconds with the CASE 1 throttling.
				// 15 HTTP 404's will take a little longer depending on network latencies so we'll say anything less than 120 seconds constitutes a tight loop of CASE 1 conditions
				// if we hit 15 errors in under 2 minutes - this problem may not ever correct itself - but it certainly could still
				PollExecSleep(60);
				nFailureCase = 2;
				nFailureCounter2++;
			}
		}

		// CASE 3: - if we've been in a back to back CASE 2 failure for an hour at 1 minute intervals - slow down even more
		else if (nFailureCounter2 > 60 )
		{
			nFailureCase = 3;
			PollExecSleep(1800); // try every 30 minutes
		}
	}
	pTCD->bInUse = 0;


goto POLL_POOL;

	gthread_exit(0);
	return 0;
}


void *SystemTrace(void *arg)
{
	int nTraceOutInterval = 60; // every minute
	time_t LastTraceTime = time(0);
	while (!g_ServerIsShuttingDown)
	{
		PortableSleep(5,0);
		if (time(0) - LastTraceTime > nTraceOutInterval)
		{
			LastTraceTime = time(0);
			
			if (g_TraceSocketHandles)
			{
				// list all of the currently open socket handles
				
				GString strOpenHandles("Currently Open Socket Handles:");
				int n = ListOpenSockets(strOpenHandles);
				if (n)
				{
					InfoLog(888,strOpenHandles);
				}
			}
		}
	}
	return 0;
}




void *SwitchBoardPoll(void *arg)
{
	// first just get the nTotalPollEntryCount by iterating the config sections
	int nTotalPollEntryCount = 0;
	int nActivePollCount = 0;
	while(1)
	{
		GString strSection;
		strSection.Format("PollExternalConnect%d",(int)nTotalPollEntryCount+1); // == "PollExternalConnect1" .. "PollExternalConnectN"
		const char *pzCountTest = GetProfile().GetString(strSection,"Server",false);
		if ( pzCountTest && pzCountTest[0] )
		{
			nTotalPollEntryCount++; // including disabled entries, they will be skipped later
		}
		else
		{
			break;
		}
	}

//GString strThreadTrace;
//strThreadTrace << "---------nTotalPollEntryCount--------:" << nTotalPollEntryCount;
//InfoLog(111,strThreadTrace);


	// see if [Messaging] is enabled
	if ( GetProfile().GetBoolean("Messaging","Enable",false) )
	{
		messagePollUpdateThread(0);
		nTotalPollEntryCount += g_msgPollSites.getNodeCount();
	}


	// loop through the (PollExternalConnectN) config sections 
	// copying all data into thread startup structures.
	ThreadConnectionData *TCD = new ThreadConnectionData[nTotalPollEntryCount];
	int i;
	for(i=0; i<nTotalPollEntryCount - (int)g_msgPollSites.getNodeCount();i++)
	{
		sprintf(TCD[i].pzPollSectionName, "PollExternalConnect%d",(int)i+1);

		const char *pzIP = GetProfile().GetString(TCD[i].pzPollSectionName,"Server",false); 
		GString strFullPath(GetProfile().GetString(TCD[i].pzPollSectionName,"Path",false));
		GString strPollName(GetProfile().GetString(TCD[i].pzPollSectionName,"Name",false));

		// if is not properly configured, or specifically disabled
		if (!pzIP || !pzIP[0] || strFullPath.IsEmpty() || strPollName.IsEmpty() ||	!GetProfile().GetBoolean(TCD[i].pzPollSectionName,"Enable",false) )
		{
			TCD[i].sockfd = -7; // used below to skip this 'disabled' entry
			continue;
		}
		strFullPath << strPollName;

		nActivePollCount++;

		TCD[i].pTSD = new ThreadStartupData;
		TCD[i].pTSD->nProxyTimeout = 120;
		
		// 1 HTTP, 2 Proxy/Tunnel, 3 Xfer, 4 TXML, 5 TXML&HTTP, 6 Remote-WS, 7 Reserved
		TCD[i].pTSD->nProtocol = GetProfile().GetInt(TCD[i].pzPollSectionName,"Protocol",false);;

		TCD[i].nPollInterval = GetProfile().GetIntOrDefault(TCD[i].pzPollSectionName,"PollIntervalSeconds",15);
		TCD[i].nPollQueueMax = GetProfile().GetIntOrDefault(TCD[i].pzPollSectionName,"PollQueue",3);
		TCD[i].nPollSrvWait = GetProfile().GetIntOrDefault(TCD[i].pzPollSectionName,"PollSrvWait",300);
		

		GString strHTTPProxyServer = GetProfile().GetStringOrDefault(TCD[i].pzPollSectionName,"HTTPProxyServer","");
		if (strHTTPProxyServer.Length())
		{
			TCD[i].strLocalNetworkAuthenticate = GetProfile().GetStringOrDefault(TCD[i].pzPollSectionName,"HTTPBasicAuthenticate","");
			strcpy(TCD[i].pTSD->szHTTPProxyFirewall,pzIP);
			strcpy(TCD[i].pTSD->szProxyToServer,strHTTPProxyServer.Buf());
		}
		else
		{
			strcpy(TCD[i].pTSD->szProxyToServer,pzIP);
		}
		strcpy(TCD[i].pTSD->pzPostConnectPathAndName,strFullPath.Buf()); // like: /MySwitchPath/MyVNCSwitchName.html


		// any non valid port number, since this connection really comes through port 80 on another machine
		// this is not used anywhere except in debug tracing, to distinguish 'listen' and 'poll'.
		// the thread that services the request is unaware of how the connection was initiated (poll-in vs listen-in).
		TCD[i].pTSD->nPort = -777;
	}

	

	// if [Messaging] is enabled
	if ( GetProfile().GetBoolean("Messaging","Enable",false) )
	{
		// 1 = Alphabetical, 0 = Reverse Alphabetical, 2 = Insertion Order
		GBTreeIterator it(&g_msgPollSites,2);

		for (int nLoop = nTotalPollEntryCount - g_msgPollSites.getNodeCount(); nLoop < nTotalPollEntryCount; nLoop++)
		{
			// the first MessagePollData matches to the nLoop index of the TCD array
			MessagePollData *pMPD = (MessagePollData *)it++;

			GetProfile().GetString("Messaging","Name",false);  


			strcpy(TCD[nLoop].pzPollSectionName,(const char *)pMPD->strSectionName);
			TCD[nLoop].pTSD = new ThreadStartupData;
			TCD[nLoop].pTSD->nProxyTimeout = 120;
			
			// 1 HTTP, 2 Proxy/Tunnel, 3 Xfer, 4 TXML, 5 TXML&HTTP, 6 Remote-WS, 7 Reserved
			TCD[nLoop].pTSD->nProtocol = 8;
			TCD[nLoop].pTSD->nPort = pMPD->nPort;

			strcpy(TCD[nLoop].pTSD->szProxyToServer,(const char *)pMPD->strServer);
			strcpy(TCD[nLoop].pTSD->szHTTPProxyFirewall,(const char *)pMPD->strProxyTo);
			
			const char *pSerialized = pMPD->lstPollFor.Serialize("&&");
			strcpy(TCD[nLoop].pTSD->pzPostConnectPathAndName,(const char *)pMPD->strSwitchBoardPath);
			strcat(TCD[nLoop].pTSD->pzPostConnectPathAndName,pSerialized); 

			TCD[nLoop].nPollInterval = GetProfile().GetInt(TCD[nLoop].pzPollSectionName,"PollIntervalSeconds",false);
			if (TCD[nLoop].nPollInterval < 1)
				TCD[nLoop].nPollInterval = 30;

			TCD[nLoop].strLocalNetworkAuthenticate = pMPD->strLocalNetworkAuthenticate;

			
			nActivePollCount++;
		}
	}


	if (nActivePollCount == 0)
	{
		// end this thread, there is nothing to poll
		return 0;
	}


	// loop through thread startup structures, TCD[i], there is one for each [PollExternalConnectN] section.
	// Within each TCD in the array, there is a list of TCD's corresponding to each connection in the Queue
	// for this [PollExternalConnectN] section by walking TCD[i].pNext->pNext->pNext
//	time_t lTimeOnError = 0;

//	int bLongDelay = 0;
	for(i=0; i<nTotalPollEntryCount;i++) 
	{

//GString strThreadTrace;
//strThreadTrace << "---------looping--------:" << i;
//InfoLog(111,strThreadTrace);

		if (TCD[i].sockfd == -7) // if this is not enabled, skip this one
			continue;

		for(int j=0;j < TCD[i].nPollQueueMax; j++)
		{
			// find a ThreadConnectData struct to use
			ThreadConnectionData *pUse = 0;
			if (TCD[i].bHasStartedThread)
			{
				ThreadConnectionData **pNext = &TCD[i].pNext; // check the next in the list
				while(*pNext)
				{
					pNext = &((*pNext)->pNext);
				}
				*pNext = new ThreadConnectionData; // we'll never have more than TCD[i].nPollQueueMax of these
				pUse = *pNext;
				pUse->Copy(&TCD[i]);
			}
			else
			{
				pUse = &TCD[i];
			}

			pUse->pBaseTCD = &TCD[i];

			gthread_t poll_thr;
			gthread_create(&poll_thr,	NULL, PollExecThread, (void *)pUse );
			pUse->bHasStartedThread = 1;
		}
	}
		
	return 0;
}


void *listenThread(void *arg)
{
	ThreadConnectionData TCD;
	TCD.pTSD = (ThreadStartupData *)arg;
	
//	if (g_TraceThread)
//	{
//		GString strThreadTrace;
//		strThreadTrace << "++++Created new listenThread  port:" << TCD.pTSD->nPort;
//		InfoLog(111,strThreadTrace);
//	}


	GString strConnectedIPAddress;
	g_listenThreadsRunning++;
	g_strListeningOnPorts << "[" << TCD.pTSD->nPort << "]";

	int clilen;
	struct sockaddr_in cli_addr;
	int nCount = 0;
	long lLastPing = 0;
	
	listen(TCD.pTSD->sockfd, g_nListenQueue );


	// Set non-blocking IO
	IOBlocking(TCD.pTSD->sockfd, 1);


	for( ;; )
	{
		TCD.pTSD->nState = 2; // waiting
		nCount++;
		clilen = sizeof(cli_addr);

		int RetryForLinux = 0;
RETRY_FOR_LINUX:		
		int rslt = readableTimeout(TCD.pTSD->sockfd, 3/*seconds*/, 0/*microseconds*/);
		if ( rslt == -1)
		{
			if (RetryForLinux++ < 10)
			{
				gsched_yield();gsched_yield();gsched_yield();gsched_yield();gsched_yield();gsched_yield();
				goto RETRY_FOR_LINUX;
			}
			goto LISTEN_THREAD_EXIT; 
		}
		if (g_ServerIsShuttingDown || TCD.pTSD->bThreadShuttingDown)
		{
			goto LISTEN_THREAD_EXIT; 
		}
		ThreadData *pTD = 0;
		
		// if there is a connection, pick it up.
		// Otherwise check to see if the server is shutting down, of if this thread is ending.
		if ( rslt > 0 )
		{
			TCD.sockfd = accept(TCD.pTSD->sockfd, (struct sockaddr *) &cli_addr, (socklen_t *)&clilen);
			if(TCD.sockfd < 0)
			{
				// a 10038 is possible here "Connection reset by Peer"
				continue;
			}
			TCD.pTSD->nState = 3; // servicing
			TCD.pTSD->nCount++;

			AddDebugSocket(TCD.sockfd);

			if (g_TraceSocketHandles)
			{
				GString strTrace("Accepted Socket:");
				strTrace << TCD.sockfd  << "   on port:" << TCD.pTSD->nPort;
				InfoLog(888,strTrace);
			}

			// get the address of the connected client
			struct sockaddr_in other_addr;
			int nSize = sizeof(sockaddr_in);
			int nPeerRet = getpeername(TCD.sockfd,(struct sockaddr *) &other_addr, (socklen_t *)&nSize);
			if ( nPeerRet != 0 )
			{
				PortableClose(TCD.sockfd,"Core21");
				continue; 
			}

			// get his string n.n.n.n IP address
			strConnectedIPAddress = inet_ntoa(other_addr.sin_addr);
			strcpy(TCD.pzClientIP,(const char *)strConnectedIPAddress);


			//if there is a list of IP's that this system will refuse to service
			if ( g_FirstRefusedIP ) 
			{
				// if this ip IS in the list, drop him.
				if (QueryServicePermission( other_addr.sin_addr.s_addr , 0 ))
				{
					TCD.pTSD->nRefused++;
					PortableClose(TCD.sockfd,"Core22");
					GString strMessage;
					strMessage << "Refused ip [" << strConnectedIPAddress << "] found in global reject list";
					InfoLog(111, strMessage);
					continue;
				}
			}

			// if this system is configured to only allow connections from a specified IP range
			if ( g_FirstAllowedIP )
			{
				// if this ip IS NOT in the list, drop him.
				if (!QueryServicePermission( other_addr.sin_addr.s_addr , 1 ))
				{
					TCD.pTSD->nRefused++;
					PortableClose(TCD.sockfd,"Core23");
					GString strMessage;
					strMessage << "Refused ip [" << strConnectedIPAddress << "] NOT found in global permission list";
					InfoLog(111, strMessage);
					continue;
				}
			}


			// if he's connecting on a blocked port, add him to the list of IP's that this system will refuse to service then disconnect him
			if (TCD.pTSD->nProtocol == 10)
			{
				TCD.pTSD->nRefused++;

				PortableClose(TCD.sockfd,"Core24");

				// likely a bogus IP, or the IP of a probing machine.
				// note: this blocks him now on all ports because he has no business on this one
				GString strErr("[System] BlockedPorts:");
				strErr << TCD.pTSD->nPort;
				AddRefuseService(strConnectedIPAddress,g_nBlockedPortPenalty,strErr);
				continue;
			}


			// if the FrequencyTimeLimit option is enabled and 
			// this guy visits too often flag him as trouble.
			if ( g_nTimeLimit )
			{
				if ( AddTime( &g_LogRoot, strConnectedIPAddress ) )
				{
					TCD.pTSD->nTooFrequent++;
					// he's hitting the server too hard, drop this connection
					PortableClose(TCD.sockfd,"Core25");
					continue;
				}
			}


			// if you do "too much"(per config file) connection traffic
			// slow down this guys transaction rate by dropping the connection.
			if ( ConnectionCounter(strConnectedIPAddress,1) > g_nMaxConnections ) 
			{
				TCD.pTSD->nTooMany++;

				GString strLog;
				strLog.Format("***Machine at[%s] attempted more than %d connections.",(const char *)strConnectedIPAddress,(int)g_nMaxConnections);
				InfoLog(155,strLog);

				// we could send a "HTTP/1.1 5xx \n\n" like IIS or just quit 
				// wasting cycles on this guy by ending the connection right now.
				PortableClose(TCD.sockfd,"Core26");

				// decrement this connection
				ConnectionCounter(strConnectedIPAddress,-1);
				continue;
			}



			// *********************************************************************
			//                       service the connection
			// *********************************************************************
			pTD = AttachToClientThread(&TCD,g_PoolClientThread,g_ThreadPoolSize,&g_nThreadPoolRoundRobin);




			if (TCD.pTSD->nInstanceLimit > 0) 
			{
				gthread_cond_wait(&pTD->condFinished, &pTD->lockFinished);
			}
		}
		if (g_PingListenThread)
		{
			if (time(0) > lLastPing + 3 )
			{
				GString strLog;
				strLog.Format("%d",(int)TCD.pTSD->nPort);
				InfoLog(156,strLog);
				lLastPing = time(0);
			}
		}

		if (g_ServerIsShuttingDown || TCD.pTSD->bThreadShuttingDown)
			break;

	}
LISTEN_THREAD_EXIT:
	g_listenThreadsRunning--;


	GString strRemove;
	strRemove << "[" << TCD.pTSD->nPort << "]";
	int nIndex = (int)g_strListeningOnPorts.Find(strRemove);
	g_strListeningOnPorts.Remove(nIndex,strRemove.Length());



	GString strLog;
	strLog.Format("Stopped port %d",(int)TCD.pTSD->nPort);
	InfoLog(157,strLog);

	PortableClose(TCD.pTSD->sockfd,"Core28");


	TCD.pTSD->sockfd = -1;
	TCD.pTSD->nState = 0; // stopped

	gthread_exit(0);
	return 0;
}



MessageSendArgs arg;


int server_started_port(int nPort)
{
	// loop through defined ports
	ThreadStartupData *pTSD = &g_TSD;
	while(pTSD)
	{
		if(nPort == pTSD->nPort)
		{
			if ( pTSD->nState > 0 )
			{
				return 1; // Ready, Waiting or Servicing
			}
			else
			{
				return 0; // port stopped(pTSD->nState == 0) or never started (pTSD->nState == -1)
			}
		}
		pTSD = pTSD->Next();
	}
	return -1; // port not defined
}




// WARNING:Never set a monitored value inside the GProfile from inside this callback or you may be in infinite recursion.
// This will be called automatically for GProfile entries registered with RegisterChangeNotification()
int GProfileMonitor(const char *pzSection, const char *pzEntry, const char *pzNewValue)
{
	GString strEntry(pzEntry);
	GString strSection(pzSection);

	// note string types may be directly assigned.  Boolean must go through GProfile translation to convert (yes/on/1/off/no/0)


	// [Trace]
	if (strSection.CompareNoCase("Trace") == 0)
	{
		if (strEntry.CompareNoCase("ConnectTrace") == 0)
		{
			g_TraceConnection = GetProfile().GetBoolean("Trace","ConnectTrace",false);
#ifdef ___XFER
			g_TraceXferConnection = g_TraceConnection;
#endif
		}
		else if (strEntry.CompareNoCase("ThreadTrace") == 0)
		{
			g_TraceThread = GetProfile().GetBoolean("Trace","ThreadTrace",false);
		}
		else if (strEntry.CompareNoCase("HTTPHeaderTrace") == 0)
		{
			g_HTTPHeaderTrace = GetProfile().GetBoolean("Trace","HTTPHeaderTrace",false);
		}
		else if (strEntry.CompareNoCase("HTTPFilesTrace") == 0)
		{
			g_TraceHTTPFiles = GetProfile().GetBoolean("Trace","HTTPFilesTrace",false);
		}
		else if (strEntry.CompareNoCase("SocketHandles") == 0)
		{
			g_TraceSocketHandles = GetProfile().GetBoolOrDefault("Trace","SocketHandles",0);
		}
		else if (strEntry.CompareNoCase("TransferSizes") == 0)
		{
			g_TraceTransferSizes = GetProfile().GetBoolOrDefault("Trace","TransferSizes",0);
		}
		else if (strEntry.CompareNoCase("DataTraceFile") == 0)
		{
			if (strlen(pzNewValue))
				g_DataTrace = 1;
			else
				g_DataTrace = 0;
		}
		else if (strEntry.CompareNoCase("PreLog") == 0)
		{
			g_strPreLog = GetProfile().GetStringOrDefault(	"Trace","PreLog","" );
		}

#ifdef ___XFER
		else if (strEntry.CompareNoCase("XferTrace") == 0)
		{
			g_TraceXfer = GetProfile().GetBoolean("Trace","XferTrace",false);
		}
		else if (strEntry.CompareNoCase("XferTraceBinary") == 0)
		{
			g_TraceXferBin = GetProfile().GetBoolean("Trace","XferTraceBinary",false);
		}
#endif // ___XFER



	} 
	// end of [Trace] - beginning of [HTTP]
	else if (strSection.CompareNoCase("HTTP") == 0)
	{
		// [HTTP]
		if (strEntry.CompareNoCase("KeepAliveMax") == 0)
		{
			g_nMaxKeepAlives = GetProfile().GetIntOrDefault("HTTP","KeepAliveMax",150);
		}
		else if (strEntry.CompareNoCase("HitLog") == 0)
		{
			g_strHTTPHitLog = pzNewValue; 
		}
		else if (strEntry.CompareNoCase("UseKeepAlives") == 0)
		{
			g_bUseHTTPKeepAlives = GetProfile().GetBoolOrDefault("HTTP","UseKeepAlives",1);
		}
		else if (strEntry.CompareNoCase("KeepAliveTimeOut") == 0)
		{
			g_nKeepAliveTimeout = GetProfile().GetIntOrDefault("HTTP","KeepAliveTimeOut",300); 
		}


	}
	// end of [HTTP] - beginning of [System]
	else if (strSection.CompareNoCase("System") == 0)
	{
		// [System]
		if (strEntry.CompareNoCase("BlockedPortPenalty") == 0)
		{
			g_nBlockedPortPenalty = GetProfile().GetIntOrDefault("System","BlockedPortPenalty",777);
		}
		else if (strEntry.CompareNoCase("DisableLog") == 0)
		{
			g_bDisableLog = GetProfile().GetBoolOrDefault("System","DisableLog",0);
		}
		else if (strEntry.CompareNoCase("LogFile") == 0)
		{
			g_strLogFile = pzNewValue; 
			g_NoLog = 0;
		}
		else if (strEntry.CompareNoCase("LogCache") == 0)
		{
			g_LogCache = GetProfile().GetBoolOrDefault("System","LogCache",1);
			g_NoLog = 0;
		}
		else if (strEntry.CompareNoCase("RefuseService") == 0)
		{
			BuildServicePermissionList(0);
		}
		else if (strEntry.CompareNoCase("MaxOpenConnectionsPerIP") == 0)
		{
			g_nMaxConnections = GetProfile().GetIntOrDefault("System","MaxOpenConnectionsPerIP",500);
		}
		else if (strEntry.CompareNoCase("FrequencyTimeLimit") == 0)
		{
			g_nTimeLimit = GetProfile().GetIntOrDefault("System","FrequencyTimeLimit",0);
		}


	}
#ifdef ___XFER
	// end of [System] - beginning of [Xfer]
	else if (strSection.CompareNoCase("Xfer") == 0)
	{
		// Section [Xfer]
		if (strEntry.CompareNoCase("ProxyAuthSystemName") == 0)
		{
			g_strProxyAuthSysName = pzNewValue; 
		}
		else if (strEntry.CompareNoCase("RequireTxnSequence") == 0)
		{
			g_nRequireTxnSeq = GetProfile().GetBoolean("Xfer","RequireTxnSequence",false);
		}
		else if (strEntry.CompareNoCase("RequireSysId") == 0)
		{
			g_bRequireSysID = GetProfile().GetBoolean("Xfer","RequireSysId",false);
		}
		else if (strEntry.CompareNoCase("RequireProxyAuth") == 0)
		{
			g_bRequireProxyAuth = GetProfile().GetBoolean("Xfer","RequireProxyAuth",false);
		}
		else if (strEntry.CompareNoCase("TxnIDPrecision") == 0)
		{
			g_nXferTxnIDPrecision = GetProfile().GetIntOrDefault("Xfer","TxnIDPrecision",3);
		}
		else if (strEntry.CompareNoCase("RequireDataFlags") == 0)
		{
			g_bRequireFlags = GetProfile().GetIntOrDefault("Xfer","RequireDataFlags",10);
		}
		else if (strEntry.CompareNoCase("EnableSwitchBoard") == 0)
		{
			g_bEnableXferSwitchBoard = GetProfile().GetBoolOrDefault("Xfer","EnableSwitchBoard",0);
		}
		else if (strEntry.CompareNoCase("SwitchBoardWaitForServer") == 0)
		{
			g_nServerQWaitTime = GetProfile().GetIntOrDefault("Xfer","SwitchBoardWaitForServer",240);;
		}
		else if (strEntry.CompareNoCase("SwitchBoardWaitForClient") == 0)
		{
			g_nClientQWaitTime = GetProfile().GetIntOrDefault("Xfer","SwitchBoardWaitForClient",120);;
		}
		else if (strEntry.CompareNoCase("SwitchBoardMaxQ") == 0)
		{
			g_nMaxQ = GetProfile().GetIntOrDefault("Xfer","SwitchBoardMaxQ",5);;
		}
	}
#endif // #ifdef ___XFER
#ifdef __CUSTOM_CORE__
	#include "ServerCoreCustomProfileUpdate.cpp"
#endif

	return 0;
}


void AssignSystemProfileValues()
{
	// set the global values	
	g_TraceConnection = GetProfile().GetBoolOrDefault(	"Trace","ConnectTrace",0);
	g_TraceSocketHandles = GetProfile().GetBoolOrDefault("Trace","SocketHandles",0);
	g_strPreLog = GetProfile().GetStringOrDefault(		"Trace","PreLog","" );
	g_TraceThread = GetProfile().GetBoolOrDefault(		"Trace","ThreadTrace",0);
	g_HTTPHeaderTrace = GetProfile().GetBoolOrDefault(	"Trace","HTTPHeaderTrace",0);
	g_TraceHTTPFiles = GetProfile().GetBoolOrDefault(	"Trace","HTTPFilesTrace",0);
	g_DataTrace =	GetProfile().ValueLength(			"Trace","DataTraceFile");
	g_TraceTransferSizes = GetProfile().GetBoolOrDefault("Trace","TransferSizes",0);

	g_nMaxKeepAlives = GetProfile().GetIntOrDefault(	"HTTP","KeepAliveMax",150);
	g_strHTTPHitLog = GetProfile().GetStringOrDefault(	"HTTP","HitLog","");
	g_bUseHTTPKeepAlives = GetProfile().GetBoolOrDefault("HTTP","UseKeepAlives",1);
	g_nKeepAliveTimeout = GetProfile().GetIntOrDefault("HTTP","KeepAliveTimeOut",300); 

	g_nBlockedPortPenalty = GetProfile().GetIntOrDefault("System","BlockedPortPenalty",777);;
	g_nLockThreadBuffers = GetProfile().GetBoolOrDefault ("System","LockThreadBuffers",1 );
	g_bDisableLog = GetProfile().GetBoolOrDefault(		"System","DisableLog",0);
	g_strLogFile = GetProfile().GetStringOrDefault(		"System","LogFile","");
	g_LogCache = GetProfile().GetBoolOrDefault(			"System","LogCache",1);
	g_nMaxConnections = GetProfile().GetIntOrDefault(	"System","MaxOpenConnectionsPerIP",10000);


#ifdef ___XFER
	g_TraceXfer = GetProfile().GetBoolOrDefault(		"Trace","XferTrace",0);
	g_TraceXferBin = GetProfile().GetBoolOrDefault(		"Trace","XferTraceBinary",0);
	g_TraceXferConnection = g_TraceConnection;
	g_bEnableXferSwitchBoard = GetProfile().GetBoolOrDefault("Xfer","EnableSwitchBoard",0);
	g_nServerQWaitTime = GetProfile().GetIntOrDefault("Xfer","SwitchBoardWaitForServer",240);;
	g_nClientQWaitTime = GetProfile().GetIntOrDefault("Xfer","SwitchBoardWaitForClient",120);;
	g_nMaxQ = GetProfile().GetIntOrDefault("Xfer","SwitchBoardMaxQ",5);
	g_strProxyAuthSysName = GetProfile().GetStringOrDefault("Xfer","ProxyAuthSystemName","sys");
	g_bRequireSysID = GetProfile().GetBoolOrDefault("Xfer","RequireSysId",0);
#endif // ___XFER
	
	g_strMessagingSwitchPath = GetProfile().GetPath("SwitchBoardServer","Name",false);
	g_bEnableMessagingSwitchboard = GetProfile().GetBoolean("SwitchBoardServer","Enable",false);

	g_LogStdOut = GetProfile().GetBoolean("Trace","LogStdOut",false);
	g_ThreadPoolSize = GetProfile().GetInt("System","Pool",false);
	if (GetProfile().GetBoolOrDefault("HTTP","Enable",0) && !g_ThreadPoolSize)
		g_ThreadPoolSize = 10;
	g_ProxyPoolSize = GetProfile().GetInt("System","ProxyPool",false);

	g_nTimeLimit = GetProfile().GetInt("System","FrequencyTimeLimit",false);
}

int g_HasRegisteredMonitorValues = 0;
void RegisterRealTimeProfileMonitorValues()
{
	if (g_HasRegisteredMonitorValues)
		return;

	g_HasRegisteredMonitorValues = 1;
	// monitor for realtime changes to the global values that may be set through the GUI admin 
	// tool for realtime kernel behavior changes and also set through the MC (MasterConfig) in the server console.
	// just add and entry here, and add a handler in GProfileMonitor()
	//
	// todo: there is lots to add to this list of global variables that may be changed in a running server.
	// everything that might possibly want to be changed should be added so that a server must never be stopped
	// and started for a behavior change.  Mostly add values to change when under attack or when performance tuning.
	// here's a few: find more and make more.
	//	int g_nPartialHeaderWaitSec
	//	int g_nProxyReadWait
	//	int g_nProxyIdleReadWait
	//  int g_nMaxHTTPHeaderSize;
	GetProfile().RegisterChangeNotification("Trace", "ConnectTrace",	GProfileMonitor);
	GetProfile().RegisterChangeNotification("Trace", "ThreadTrace",		GProfileMonitor);
	GetProfile().RegisterChangeNotification("Trace", "HTTPHeaderTrace",	GProfileMonitor);
	GetProfile().RegisterChangeNotification("Trace", "HTTPFilesTrace",	GProfileMonitor);
	GetProfile().RegisterChangeNotification("Trace", "DataTraceFile",	GProfileMonitor);
	GetProfile().RegisterChangeNotification("Trace", "SocketHandles",	GProfileMonitor);
	GetProfile().RegisterChangeNotification("Trace", "XferTrace",		GProfileMonitor);
	GetProfile().RegisterChangeNotification("Trace", "XferTraceBinary",	GProfileMonitor);
	GetProfile().RegisterChangeNotification("Trace", "TransferSizes",	GProfileMonitor);
	GetProfile().RegisterChangeNotification("Trace", "PreLog",			GProfileMonitor);



	GetProfile().RegisterChangeNotification("HTTP",  "KeepAliveMax",			GProfileMonitor);
	GetProfile().RegisterChangeNotification("HTTP",  "HitLog",					GProfileMonitor);
	GetProfile().RegisterChangeNotification("HTTP",  "UseKeepAlives",			GProfileMonitor);
	GetProfile().RegisterChangeNotification("HTTP",  "KeepAliveTimeOut",		GProfileMonitor);

	GetProfile().RegisterChangeNotification("System","BlockedPortPenalty",		GProfileMonitor); 
	GetProfile().RegisterChangeNotification("System","DisableLog",				GProfileMonitor); 
	GetProfile().RegisterChangeNotification("System","LogFile",					GProfileMonitor); 
	GetProfile().RegisterChangeNotification("System","LogCache",				GProfileMonitor); 
	GetProfile().RegisterChangeNotification("System","RefuseService",			GProfileMonitor); 
	GetProfile().RegisterChangeNotification("System","MaxOpenConnectionsPerIP",	GProfileMonitor); 
	GetProfile().RegisterChangeNotification("System","FrequencyTimeLimit",		GProfileMonitor); 

#ifdef ___XFER
	GetProfile().RegisterChangeNotification("Xfer","TxnIDPrecision",			GProfileMonitor); 
	GetProfile().RegisterChangeNotification("Xfer","RequireDataFlags",			GProfileMonitor); 
	GetProfile().RegisterChangeNotification("Xfer","RequireTxnSequence",		GProfileMonitor); 
	GetProfile().RegisterChangeNotification("Xfer","RequireSysId",				GProfileMonitor); 
	GetProfile().RegisterChangeNotification("Xfer","RequireProxyAuth",			GProfileMonitor); 
	GetProfile().RegisterChangeNotification("Xfer","SwitchBoardPath",			GProfileMonitor); 
	GetProfile().RegisterChangeNotification("Xfer","EnableSwitchBoard",			GProfileMonitor); 
	GetProfile().RegisterChangeNotification("Xfer","SwitchBoardWaitForServer",	GProfileMonitor); 
	GetProfile().RegisterChangeNotification("Xfer","SwitchBoardWaitForClient",	GProfileMonitor); 
	GetProfile().RegisterChangeNotification("Xfer","SwitchBoardMaxQ",			GProfileMonitor); 
#endif
#ifdef __CUSTOM_CORE__
	#include "ServerCoreCustomProfileSetNotify.cpp"
#endif
}




//////////////////////////////////////////////////////////////////////////////
int server_start(const char *pzStartupMessage = 0)
{
	g_ServerIsShuttingDown = 0;
	g_SwitchBoard.Startup();
	memset(HashOfActiveConnections,0,ACTIVE_CONN_HASH_SIZE ); // reset connection counts if this is a restart.

	// turn off stdout buffering so we never need to flush();
	#ifndef WINCE
		setbuf (stdout, NULL);
	#endif
	#ifdef _WIN32
		_gthread_processInitialize();
	#endif

	GString strStartup("Server starting at:");
	strStartup << g_strThisIPAddress;
	InfoLog(158,strStartup);

	char pzTime[128];
	struct tm *newtime;
	time_t ltime;
	time(&ltime);
	newtime = gmtime(&ltime);
	strftime(pzTime, 128, "Date: %a, %d %b %Y %H:%M:%S GMT\r\n", newtime);
	InfoLog(159,pzTime);

	g_SwitchBoard.SetInfoLog( InfoLog );

#ifdef ___XFER
	g_XferSwitchBoard.SetInfoLog( InfoLog );
	SetXferSwitchBoardConnect( SBConnect );
	SetXferWaitForReply( SBWaitForReply );
	SetXferInfoLog( InfoLog );
#endif
#ifdef __CUSTOM_CORE__
	#include "ServerCoreCustomServerStart.cpp"
#endif

try
{

	BuildServicePermissionList(0); // "System","RefuseService"
	BuildServicePermissionList(1); // "System","GrantService"


	AssignSystemProfileValues();
	RegisterRealTimeProfileMonitorValues();

	// one time initialization of the socket library
#ifdef _WIN32
	WSADATA wsaData;
	WSAStartup(MAKEWORD( 2, 2 ),&wsaData );
#endif

	gthread_mutex_init(&g_csMtxConnCache, NULL);
	gthread_mutex_init(&PoolExpansionLock,0);

	g_LogStdOut = 1; // Startup info always goes to std out and the file

	if (pzStartupMessage)
	{
		InfoLog(160,pzStartupMessage);
	}

	if ( GetProfile().GetBoolean("System","UltraHighSecure",false) )
		InitRandomNumbers();
	else
		QuickInitRandomNumbers();

	// gather info about all listen threads that will be started
	ThreadStartupData *pTSD = &g_TSD;


	if (GetProfile().GetBool("HTTP","Enable",false))
	{
		int nIndex = 1;
		GString strPluginEntry;
		while (1)
		{
			strPluginEntry << "ServerPlugin" << nIndex++;
			const char *pzVal = GetProfile().GetString("HTTP",strPluginEntry,false);
			if (pzVal)
			{
				GStringList *plstArgs = new GStringList("|",pzVal);
				g_lstActivePlugins.AddLast(plstArgs);
			}
			else
				break;
			strPluginEntry = "";
		}
	}




	// Lock any 'blocked' ports upfront
	GString strPortBlock = GetProfile().GetString("System","BlockedPorts",false);
	if ( !strPortBlock.IsEmpty() )
	{
		GStringList ports(",",strPortBlock);
		GStringIterator itPorts(&ports);
		while (itPorts())
		{
			const char *pzPort = itPorts++;
			pTSD = pTSD->Next(1);
			pTSD->nPort = atoi(pzPort);
			pTSD->nProtocol = 10;

			strcpy(pTSD->szConfigSectionName,"System");

			GString strLog;
			strLog.Format("%d   ",(int)pTSD->nPort);
		}
	}



	//
	// The HTTP/TXML service can share a port or each be on it's own port.....
	//

	// HTTP Service
	if ( GetProfile().GetBoolean("HTTP","Enable",false) )
	{
		pTSD->nPort = GetProfile().GetInt("HTTP","Port",false);
		if (!pTSD->nPort)
			pTSD->nPort = 80;
		pTSD->nProtocol = 1;
		
		strcpy(pTSD->szConfigSectionName,"HTTP");
		
		GString str;
		str.Format("%d   ",(int)pTSD->nPort);
	}
	// TXML Service
	if ( GetProfile().GetBoolean("TXML","Enable",false) )
	{
		if ( GetProfile().GetBoolean("HTTP","Enable",false) )
		{
			if (pTSD->nPort == GetProfile().GetInt("TXML","Port",false))
			{
				// Running TXML and HTTP on the same port (modify the current pTSD)
				pTSD->nProtocol = 5; 
			}
			else
			{
				// Create a new pTSD for the TXML port
				pTSD = pTSD->Next(1);
				pTSD->nPort = GetProfile().GetInt("TXML","Port",false);
				pTSD->nProtocol = 4;

				GString strLog;
				strLog.Format("%d   ",(int)pTSD->nPort);

			}
			strcpy(pTSD->szConfigSectionName,"TXML");
		}
	}


	if ( GetProfile().GetBoolean("HTTPProxy","Enable",false) )
	{
		pTSD = pTSD->Next(1);
		pTSD->nPort = GetProfile().GetInt("HTTPProxy","Port",false);
		if (pTSD->nPort < 1)
			pTSD->nPort = 8080;
		pTSD->nProtocol = 11;
		strcpy(pTSD->szConfigSectionName,"HTTPProxy");
	}

// HOOK:protocol init
#ifdef __CUSTOM_CORE__
	#include "ServerCoreCustomProtocolInit.cpp"
#endif



	// Xfer - Port 10777
	int nXferPort = 0;
	if ( GetProfile().GetBoolean("Xfer","Enable",false) )
	{
		pTSD = pTSD->Next(1);
		pTSD->nPort = GetProfile().GetInt("Xfer","Port",false);
		if (pTSD->nPort < 1)
			pTSD->nPort = 10777;
		nXferPort = pTSD->nPort;
		pTSD->nProtocol = 3;
		strcpy(pTSD->szConfigSectionName,"Xfer");
	}


	// Remote Console Service - Port 10111
	if ( GetProfile().GetBoolean("RemoteConsole","Enable",false) )
	{
		int nRemoteConsolePort = GetProfile().GetInt("RemoteConsole","Port",false);
		
		// if Xfer is running, and RemoteConsole is on the same port then don't do anything, otherwise start Remote Console on the port configured
		if (nXferPort && nRemoteConsolePort != nXferPort)
		{
			pTSD = pTSD->Next(1);
			pTSD->nPort = GetProfile().GetInt("RemoteConsole","Port",false);
			if (pTSD->nPort < 1)
				pTSD->nPort = 10111;
			strcpy(pTSD->szConfigSectionName,"RemoteConsole");
			pTSD->nProtocol = 3;
		}
	}


	// loop through the [Tunnel1]...[TunnelN] and [Proxy1]..[ProxyN] sections in the config file
	int bAdvanceTSD = 1;
	int nEnabledCount = 0;
	for(int nTunnelOrProxy = 0; nTunnelOrProxy < 2; nTunnelOrProxy++)
	{
		const char *pSection = (nTunnelOrProxy) ? "Proxy" : "Tunnel";

		// Priming Section Read
		int nEntry = 1;
		GString strNextSection;
		strNextSection.Format("%s1",pSection); // == "Proxy1" or "Tunnel1"
		const char *pzLocalPort = GetProfile().GetString(strNextSection,"LocalPort",false);
			
		while(pzLocalPort && pzLocalPort[0])
		{
			if (GetProfile().GetBoolean(strNextSection,"Enable"))
			{
				nEnabledCount++;
				if (bAdvanceTSD)
					pTSD = pTSD->Next(1);
				try // missing required profile sections throw exceptions
				{
					pTSD->nProtocol = 2;
					pTSD->nPort = atoi(pzLocalPort);
					strcpy(pTSD->szProxyToServer, GetProfile().GetString(strNextSection,"RemoteMachine") );
					pTSD->nProxyToPort = GetProfile().GetInt(strNextSection,"RemotePort"); 
					pTSD->nInstanceLimit = GetProfile().GetInt(strNextSection,"InstanceLimit",false); 
					pTSD->nProxyTimeout = GetProfile().GetInt(strNextSection,"Timeout");
					pTSD->bIsTunnel = (nTunnelOrProxy) ? 0 : 1;

					// so the thread can go get more info from this section later
					strcpy(pTSD->szConfigSectionName,(const char *)strNextSection);

					bAdvanceTSD = 1;
				}
				catch( GException &rErr )
				{
					// skip this one, it's not correctly setup in the config file.
					GString strLog;
					strLog.Format("Port[%s] not started.  Configuration error[%s]",pzLocalPort,rErr.GetDescription());
					InfoLog(161,strLog)
						;
					bAdvanceTSD = 0;
				}
			}

			// Next Section Read
			nEntry++;
			strNextSection.Format("%s%d",pSection,(int)nEntry); // "ProxyN" or "TunnelN"
			pzLocalPort = GetProfile().GetString(strNextSection,"LocalPort",false);
		}
	}
	if (nEnabledCount && !g_ProxyPoolSize)
	{
		g_ProxyPoolSize = 10;
	}

	// bind to a socket on each port that this server handles
	startListeners(1);



	// create the worker threads 
	CreateThreadPool(&g_PoolClientThread, g_ThreadPoolSize,clientThread);
	CreateThreadPool(&g_PoolProxyHelperThread, g_ProxyPoolSize,ProxyHelperThread);


	// create a listener thread for each port we handle
	startListeners(2);

	InfoLog(162,"All bound ports are now being serviced");



	gthread_t listen_thr;
	gthread_create(&listen_thr,	NULL, SwitchBoardPoll, (void *)0 );

	gthread_create(&listen_thr,	NULL, SystemTrace, (void *)0 );
// HOOK:protocol init
#ifdef __CUSTOM_CORE__
	#include "ServerCoreCustomServerStarted.cpp"
#endif

}

catch( GException &rErr )
{
	printf( rErr.GetDescription() );
	InfoLog(1, rErr.GetDescription());
}
catch( ... )
{
	printf("Fatal error starting server.\n");
	InfoLogFlush();
	return 0;
}

	InfoLogFlush();
	return 1;
}


void viewPorts(GString *pG = 0)
{
	// defined ports
	GString strPort("All Defined Ports:");
	ThreadStartupData *pTSD = &g_TSD;
	int n = 0;
	while(pTSD)
	{
		if (pTSD->nPort == -1)
		{
			pTSD = pTSD->Next();
			continue;
		}
		strPort << pTSD->nPort << "   ";
		n++;
		pTSD = pTSD->Next();
	}
	if (!n)
		strPort << "None.";

	
	// errored ports
	pTSD = &g_TSD;
	GString str("\n\nErrored Ports:");
	n = 0;
	while(pTSD)
	{
		if (pTSD->nPort == -1)
		{
			pTSD = pTSD->Next();
			continue;
		}
		if ( pTSD->nState < 0 )
		{
			str << pTSD->nPort << "   ";
			n++;
		}
		pTSD = pTSD->Next();
	}
	if (n)
	{
		strPort << str;
	}

	// stopped ports
	pTSD = &g_TSD;
	str = "\n\nStopped Ports:";
	n = 0;
	while(pTSD)
	{
		if (pTSD->nPort == -1)
		{
			pTSD = pTSD->Next();
			continue;
		}
		if ( pTSD->nState == 0 )
		{
			str << pTSD->nPort << "   ";
			n++;
		}
		pTSD = pTSD->Next();
	}
	if (n)
	{
		strPort << str;
	}


	// active ports
	pTSD = &g_TSD;
	str ="\n\nPorts [R]eady [W]aitng and [S]ervicing:";
	n = 0;
	while(pTSD)
	{
		if (pTSD->nPort == -1)
		{
			pTSD = pTSD->Next();
			continue;
		}

		if ( pTSD->nState > 0 )
		{
			str << pTSD->nPort << "[";
			if(pTSD->nState == 1)
				str << "R";
			else if(pTSD->nState == 2)
				str << "W";
			else if(pTSD->nState == 3)
				str << "S";
			else 
				str << "?";
			str << "]   ";
			n++;
		}
		pTSD = pTSD->Next();
	}
	if (n)
	{
		strPort << str;
	}


	// display it
	int nPrevLogSetting = g_LogStdOut;
	int nPrevLogSetting2 = g_NoLog;
	g_LogStdOut = 1; 
	g_NoLog = 0; 

	InfoLog(163,strPort);
	if (pG)
		*pG << strPort;

	g_LogStdOut = nPrevLogSetting; 
	g_NoLog = nPrevLogSetting2;
}


// maybe the server was started and only 4/5 of the desired listen ports
// were available.  The server starts with a warning.  Any time in the
// future when the desired port becomes available. Calling listenerReboot()
// will close all of the active listeners then re-attempt for 5/5.
// All active transactions will complete normally.  New connections will be
// briefly paused.
void listenerReboot()
{
	stopListeners();
	
	PortableSleep(3,0);

	startListeners(3);
}



// a diagnostic to briefly wake every thread and have it print it's threadID.
// fyi: each thread is assigned a numerical ID beginning at 1  (easier to look at than real thread handles)
void pingPools()
{
	int nPrevLogSetting = g_LogStdOut;
	int nPrevLogSetting2 = g_NoLog;
	g_LogStdOut = 1; 
	g_NoLog = 0;

	InfoLog(164,"Main Pool:");
	PingPool(g_PoolClientThread,g_ThreadPoolSize);
	InfoLog(165,"Proxy Pool:");
	PingPool(g_PoolProxyHelperThread,g_ProxyPoolSize);

	g_PingListenThread = 1;
	InfoLog(166,"Listening on ports:");
	PortableSleep(3,0);
	g_PingListenThread = 0;


	g_LogStdOut = nPrevLogSetting; 
	g_NoLog = nPrevLogSetting2;
}




void showActiveThreads(GString *pG/* = 0*/)
{
//	if (pG)
//	{
//		GString Msg;
//		Msg << g_nClientthreadsInUse << "," << g_ThreadUsageMax << "," << g_TotalHits << "," <<  g_TotalTooBusy;
//		*pG << Msg;
//	}
	
	
	int nPrevLogSetting = g_LogStdOut;
	int nPrevLogSetting2 = g_NoLog;
	g_LogStdOut = 1; 
	g_NoLog = 0; 
	if( g_ThreadPoolSize > 1 )
	{
		for(int i=0; i < g_ThreadPoolSize; i++)  
		{
			ThreadData *td = (ThreadData *)(g_PoolClientThread->m_arrObjects[i].m_pValue);

			if (td->nThreadIsBusy)
			{
				// tid,protocol,action,outPort,inPort,IP,time,"Action"\r\n
				GString Msg;
				Msg << td->nThreadID << "," << td->pTSD->nProtocol
					<< "," << td->nAction << "," << td->pTSD->nProxyToPort
					<< "," << td->pTSD->nPort << "," << td->pzClientIP
					<< "," << (getTimeMS() - td->starttime) / 1000 << "," << td->nSequence << "," << td->strAction;

				if (pG)
				{
					if (pG->Length())
						*pG << "\r\n";
					*pG << Msg;
				}
				else
				{
					InfoLog(167,Msg);
				}
			}
		}
	}
	g_LogStdOut = nPrevLogSetting; 
	g_NoLog = nPrevLogSetting2;
}

#include	<memory.h>
#include	<assert.h>

// This routine was designed to verify algorithms after a new operating system port.
void server_build_check()
{
	InfoLog(168,"Starting build check");

	// view data sizes
	GString str;
	str.Format("Size of--- Short:%d   Integer:%d    Long:%d   VeryLong: %d",sizeof(short),sizeof(int),sizeof(long),sizeof(__int64));
	InfoLog(169,str);

	// view more data sizes
	GString str2;
	str2.Format("Size of--- BYTE:%d   DWORD:%d    char:%d   VeryLong: %d",sizeof(BYTE),sizeof(DWORD),sizeof(char),sizeof(__int64));
	InfoLog(169,str2);
	

	// view byte order (1-0-0-0 is Intel,  0-0-0-1 is most others)

	// thoughts from the author......
	// byte order is like the slash - some people think backwards....
	// Some even divide about it(the slash) - with religious like convictions and loyalty to their slash of choice.
	// but who will divide by 0? (that is illegal - we ALL agree - no matter which way we slash)
	// There is a time an a place for everything, search for "breakin the law" in GException.cpp. 
	// Jesus said, "I have come not to break the law, but to fulfill it"
	
	int i = 1;
	char *pi = (char *)&i;
	str = "Byte Order ";
	for(int nByteOrderIndex = 0; nByteOrderIndex < sizeof(int); nByteOrderIndex++)
	{
		str += (int)pi[nByteOrderIndex];
		str += "-";
	}
	InfoLog(170,str);




	///////////////////////////////////////// Make Test Data ///////////////////////////////////////////////
	char pData1[256];
	char pWorkBuffer[1024];
	char pDataMatch[256];
	memset(pData1,0,256);
	memset(pDataMatch,0,256);
	for(i=0; i<32; i++)
	{
		pData1[i] = 65+i;		// A thru a followed by repeating nulls
		pDataMatch[i] = 65+i;
	}

	///////////////////////////////////////// SHA256 ////////////////////////////////////////////////////
	GString strMsgHash("Testing SHA256 Hash:");
	unsigned char pDigest[32];
	SHA256Hash((unsigned char *)pData1, 32, pDigest);

	for(i=0;i<32;i++)
	{
		GString strLog;
		strLog.Format("%d-",(int)(unsigned char)pDigest[i]);
		strMsgHash << strLog;
	}
	InfoLog(171,strMsgHash);


	///////////////////////////////// TwoFish Encrypt/Decrypt  ///////////////////////////////////////
	CipherData en("This is the pass-key",DIR_ENCRYPT);
	CipherData de("This is the pass-key",DIR_DECRYPT);
	InfoLog(172,"Init Cipher Keys");
	int nBitsCrypted;
	try
	{												// uncrypted in				// crypted out
		GString strMsgCipher("Testing Encrypt:");
		nBitsCrypted = en.blockEncrypt((unsigned char *)pData1,32*8,(unsigned char *)pWorkBuffer);
		for(i=0;i<32;i++)
		{
			GString strLog;
			strLog.Format("%d-",(int)(unsigned char)pWorkBuffer[i]);
			strMsgCipher << strLog;
		}
		InfoLog(174,strMsgCipher);
	}
	catch(...)
	{
		InfoLog(175,"blockEncrypt failed.");
		return;
	}
	if (nBitsCrypted != 32*8)
	{
		InfoLog(176,"blockEncrypt failed point 2.");
		return;
	}
	try
	{
		InfoLog(177,"Testing Decrypt");
		memset(pData1,0,256);							// crypted in					 // uncrypted out	
		nBitsCrypted = de.blockDecrypt((unsigned char *)pWorkBuffer,32*8,(unsigned char *)pData1);
		
		if (memcmp(pData1,pDataMatch,256) == 0)
		{
			InfoLog(178,"Decrypted OK.");
		}
		else
		{
			InfoLog(179,"Decrypted Failed!");
		}
	}
	catch(...) 
	{
		InfoLog(180,"Decrypt Failed, point 1");
		return;
	}
	if (nBitsCrypted != 32*8)
	{
		InfoLog(181,"Decrypt Failed, point 2");
		return;
	}

	
	/////////////////////////////// Zip/UnZip ////////////////////////////////////////////////////////
	InfoLog(182,"Testing Compress");
	int nZippedLength = 256;
	int cRet;
	if (1)
	{
		cRet = GZipBuffToBuffCompress(
										pWorkBuffer, // destination
									   (unsigned int*)&nZippedLength,	
									   pData1, // source
									   256					   );
	}
	else
	{
		cRet = BZ2_bzBuffToBuffCompress( pWorkBuffer, // destination
							   (unsigned int*)&nZippedLength,	
							   pData1, // source
							   256,						// zip the nulls + the data
							   9, //blockSize100k, 
							   0,// verbosity, 
							   30);//workFactor )
	}

	if (cRet != 0)
	{
		GString strLog;
		strLog.Format("Compress failed. code[%d]",(int)cRet);
		InfoLog(183,strLog);
		return;
	}
	else
	{
		GString strLog;
		strLog.Format("Compressed 256 bytes into %d bytes OK.",(int)nZippedLength);
		InfoLog(184,strLog);
	}


	InfoLog(185,"Testing Decompress");
	int nUnZippedLength = 256;
	memset(pDataMatch,0,256);
	if (1)
	{
				cRet = GZipBuffToBuffDecompress
										   ( pDataMatch, // destination
											 (unsigned int *)&nUnZippedLength, // decompressed length
											 pWorkBuffer, // source
											 nZippedLength); // source len
	} else
	{

			cRet = BZ2_bzBuffToBuffDecompress 
										   ( pDataMatch,		// destination
											 (unsigned int *)&nUnZippedLength,
											 pWorkBuffer,		// source
											 nZippedLength,
											 1,// or 0 small,
											 0);//verbosity )
	}

	// see if the decompressed data matches the source data
	if (memcmp(pDataMatch,pData1,256) == 0)
	{
		InfoLog(187,"Decompress OK.");
	}else
	{
		InfoLog(188,"Decompress Failed!");
		return;
	}



	/////////////////////////////////// UU En/DeCode /////////////////////////////////////////////
	// uuencode into a GString
	InfoLog(189,"Testing UUEncode");
	BUFFER b;
	BufferInit(&b);
	uuencode((unsigned char *)pData1, 256, &b);
	GString strEncoded((char *)b.pBuf, b.cLen);
	BufferTerminate(&b);
	
	GString strLog;
	strLog.Format("UUEncoded=%s",(const char *)strEncoded);
	InfoLog(190,strLog);


	InfoLog(191,"Testing UUDecode");
	BUFFER b2;
	BufferInit(&b2);
	unsigned int nDecoded;
	uudecode((char *)(const char *)strEncoded, &b2, &nDecoded, false);
	if (memcmp(b2.pBuf,pData1,256) == 0 )
	{
		InfoLog(192,"UUDecoded OK");
	}
	else
	{
		InfoLog(193,"UUDecode Failed!");
		return;
	}
	BufferTerminate(&b2);

	InfoLog(194,"Done Checking Build.");
}

void server_stop()
{
	// stop receiving new connections, end the listen threads
	InfoLogFlush();
	g_ServerIsShuttingDown = 1;
	g_SwitchBoard.ShutDown();
	
#ifdef _WIN32
	Sleep(300);
#else
	PortableSleep(3,0);
#endif

	// drain the pools
	InfoLog(195,"Destroying thread pools");
	DestroyThreadPool(g_PoolProxyHelperThread,g_ProxyPoolSize);
	DestroyThreadPool(g_PoolClientThread,g_ThreadPoolSize);
	g_ProxyPoolSize = 0;
	g_ThreadPoolSize = 0;

#ifdef ___XFER
	DestroyXferCommandThreadPool();
	DestroyXferProxyThreadPool();
#endif 
#ifdef __CUSTOM_CORE__
	#include "ServerCoreCustomServerStop.cpp"
#endif

	gthread_mutex_destroy(&g_csMtxConnCache);

#ifdef _WIN32
	Sleep(2000);
	_gthread_processTerminate();
#else
	PortableSleep(2,0);
#endif


	InfoLog(195,"ShutDown Complete");
	InfoLogFlush();

#ifdef _WIN32
	WSACleanup();
#endif
}


void DeHexUUCipher(GString *pStrIn, GString *pStrDest, const char *pzPass)
{
	pStrIn->Replace('@','+');	// faster, constant size

	//uudecode
	BUFFER b;
	BufferInit(&b);
	unsigned int nDecoded;
	uudecode((char *)pStrIn->StrVal(), &b, &nDecoded, false);

	char *pWkBuf = new char[(int)pStrIn->Length()+16];

	//decrypt
	CipherData de(pzPass,DIR_DECRYPT);
	int nBitsCrypted;
	nBitsCrypted = de.blockDecrypt((unsigned char *)b.pBuf,nDecoded *8,(unsigned char *)pWkBuf);
	BufferTerminate(&b);


	pStrDest->WriteOn(pWkBuf,nBitsCrypted/8);
//	pStrDest->TrimRight('¿');      // this was the code prior to Nov 3, 2010 replaced with...
	pStrDest->TrimRight((char)168);// For Chinese character support the ¿ was represented in its ascii base 10 numerical equivalent.

	delete pWkBuf;
}

// cipher it, then uuencode it, then hex escape URL reserved
void EnHexUUCipher(GString *pStrIn, GString *pStrDest, const char *pzPass)
{
	// cipher it
	unsigned char nPad = (16 - ((int)pStrIn->Length()) % 16);
	nPad = (nPad == 16) ? 0 : nPad;
	// pad it
	pStrIn->PadRight(pStrIn->Length() + nPad,(char)168); // 168 is seen as'¿'
	char *pWkBuf = new char[(int)pStrIn->Length()+16];

	CipherData en(pzPass,DIR_ENCRYPT);
	int nBitsCrypted = en.blockEncrypt ((unsigned char *)pStrIn->StrVal(),((int)pStrIn->Length())*8,(unsigned char *)pWkBuf);

//	pStrIn->TrimRight('¿',nPad); // lop off the pad so the input is 'const' - this was the code prior to Nov 3, 2010 replaced with...
	pStrIn->TrimRight((char)168,nPad);  // For Chinese character support the ¿ was represented in its ascii base 10 numerical equivalent.

	// uuencode it
	BUFFER b;	
	BufferInit(&b);
	uuencode((unsigned char *)pWkBuf, nBitsCrypted/8, &b);
	*pStrDest = (const char *)b.pBuf;

	BufferTerminate(&b);

	// hex escape URL reserved
	pStrDest->Replace('+','@');		// faster, constant size

	delete pWkBuf;
}



