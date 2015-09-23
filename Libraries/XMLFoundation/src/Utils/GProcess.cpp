// --------------------------------------------------------------------------
//						United Business Technologies
//			  Copyright (c) 2015  All Rights Reserved.
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

#ifdef _WIN32		// ****** Windows and Windows CE
	#ifdef WINCE
		#include  "WinCERuntimeC.h"
	#endif
	#include <windows.h>	
	#ifndef WINCE
		#include <process.h>		// for _spawnv()
		#include <sys/stat.h>
		#include <io.h>			// for _chmod() in _WIN32
		#include <Winsvc.h>		// for SC_HANDLE, OpenSCManager, and all SCM calls
		#include <direct.h>		// for mkdir()
		#include <pdh.h>		// for process stats used in GetProcessStats()
		#include <shlwapi.h>	// for SHDeleteKey()
		#include <iphlpapi.h>   // for GetIpAddrTable()  // within <iphlpapi>, this documents the undocumented:  http://www.pinvoke.net/default.aspx/iphlpapi/GetExtendedTcpTable.html
		#include <Iprtrmib.h>   // for MIB_TCPTABLE_OWNER_PID structure needed by GetExtendedTcpTable() and in some versions PMIB_TCP6TABLE_OWNER_PID Argument 1 struct for GetExtendedTcpTable()


// VC6 requires Winsock libraries newer than what was available in Service Pack 6,  For VC6 builds, AF_INET6 was already defined in <winsock.h> in the VC6Winsock2 folder of /XMLFoundation
#if defined(_MSC_VER) && _MSC_VER > 1200		// for Visual Studio newer than VC6
	#include <ws2tcpip.h>   // for AF_INET6
//	#include <tcpmib.h>     // PMIB_TCP6TABLE_OWNER_PID was defined in <Iprtrmib.h> VS2003-VS2005, in VS2013 its in <tcpmib.h>, and by default not compiled in by preprocessor directive.
//	#include <udpmib.h>     // PMIB_UDP6TABLE_OWNER_PID was defined in <Iprtrmib.h> VS2003-VS2005, in VS2013 its in <tcpmib.h>, and by default not compiled in by preprocessor directive.

	///////////////////////////////////////////////////// from <tcpmib.h>
	typedef struct _MIB_TCP6ROW_OWNER_PID
	{
		UCHAR           ucLocalAddr[16];
		DWORD           dwLocalScopeId;
		DWORD           dwLocalPort;
		UCHAR           ucRemoteAddr[16];
		DWORD           dwRemoteScopeId;
		DWORD           dwRemotePort;
		DWORD           dwState;
		DWORD           dwOwningPid;
	} MIB_TCP6ROW_OWNER_PID, *PMIB_TCP6ROW_OWNER_PID;

	typedef struct _MIB_TCP6TABLE_OWNER_PID
	{
		DWORD                   dwNumEntries;
		_Field_size_(dwNumEntries)
			MIB_TCP6ROW_OWNER_PID   table[ANY_SIZE];
	} MIB_TCP6TABLE_OWNER_PID, *PMIB_TCP6TABLE_OWNER_PID;

	/////////////////////////////////////////////////////    // from <udpmib.h>
	typedef struct _MIB_UDP6ROW_OWNER_PID {
		UCHAR           ucLocalAddr[16];
		DWORD           dwLocalScopeId;
		DWORD           dwLocalPort;
		DWORD           dwOwningPid;
	} MIB_UDP6ROW_OWNER_PID, *PMIB_UDP6ROW_OWNER_PID;

	typedef struct _MIB_UDP6TABLE_OWNER_PID
	{
		DWORD                   dwNumEntries;
		_Field_size_(dwNumEntries)
		MIB_UDP6ROW_OWNER_PID   table[ANY_SIZE];
	} MIB_UDP6TABLE_OWNER_PID, *PMIB_UDP6TABLE_OWNER_PID;
#else
	#include <winsock.h>   // for getnameinfo(), AF_INET,  struct 'in_addr' (and AF_INET6 (using vc6.sp7))
#endif

		#pragma comment(lib, "iphlpapi.lib")
		#pragma comment(lib, "shlwapi.lib") // for SHDeleteKey()
		#pragma comment(lib, "ADVAPI32.lib")	
		#pragma message("Automatically linking with ADVAPI32.lib and SHLWAPI.lib and iphlpapi.lib")
	#endif

	#define socklen_t int

#else // Linux, Unix
	#include <sys/types.h>	
	#include <sys/socket.h>	
	#include <sys/stat.h>		// for chmod()
	#include <arpa/inet.h>		// for ntohl.h
	#include <netdb.h>			// for gethostbyname()
	#include <unistd.h>			// for unlink()
	#include <netinet/tcp.h>	// for no reason other than the #define of TCP_NODELAY (==1)
	#ifdef _ANDROID
		#include <GSocketHelpersAndroid.h> // <ifaddrs.h> is not part of Android, so GSocketHelpersAndroid.h implements that and other missing bits in Android
	#else
		#include <ifaddrs.h>    // for ifaddrs and getifaddrs()
	#endif

#endif


#include "GDirectory.h"
#include "GProcess.h"
#include "Base64.h"
#include "GHash.h"

#include <string.h> // strpbrk
#include <stdio.h> // FILE
#include <stdlib.h> // atoi
#include <errno.h>			// for no reason other than the #define of EBUSY (==16)



//--------------------------------------------------------------------------------------------------------------------

#ifdef _WIN32
#include <tlhelp32.h>
typedef BOOL (CALLBACK *PROCENUMPROC)( DWORD, DWORD, WORD, LPSTR, LPARAM ) ;

struct ProcessWindowTitle
{
	DWORD pid;
	GString strTitle;
	HWND hWnd;
};

BOOL CALLBACK EnumWindowsProc(   HWND hwnd,    DWORD lParam   )
{ 
    DWORD             pid = 0; 
    TCHAR             buf[64]; 
    ProcessWindowTitle*  pwt = (ProcessWindowTitle*)lParam; 
    
    // get the processid for this window 
    if (!GetWindowThreadProcessId(hwnd, &pid)) 
	{ 
        return TRUE; 
    } 
    // look for the task in the task list for this window 
    if (pwt->pid == pid) 
    {
        pwt->hWnd = hwnd;
		int nCnt = GetWindowText(hwnd, buf, 63);
		if (nCnt > 0)
		{
			buf[nCnt] = '\0';
			pwt->strTitle.Empty();
			pwt->strTitle << "[" << buf << "] ";
		}
		return FALSE;
    } 
    return TRUE; // continue the enumeration 
}
#endif


//#ifdef WINCE
//  typedef BOOL (CALLBACK *PROCENUMPROC)( DWORD, DWORD, WORD, LPSTR, LPARAM ) ;
//#endif

#if defined(_WIN32) && !defined(WINCE)
BOOL WINAPI EnumProcs( PROCENUMPROC lpProc, LPARAM lParam ) ;
#include <vdmdbg.h>
typedef struct
{
	DWORD          dwPID ;
	PROCENUMPROC   lpProc ;
	DWORD          lParam ;
	BOOL           bEnd ;
} EnumInfoStruct ;

void GetProcessStats(GString *pG,const char *pzProcessName, HINSTANCE dllHandle)
{
	typedef BOOL (CALLBACK* pdh_Open)(LPCSTR,  DWORD ,  HQUERY);
	typedef BOOL (CALLBACK* pdh_Add)(HQUERY, LPCSTR,  DWORD, HCOUNTER );
	typedef BOOL (CALLBACK* pdh_Get)(HQUERY);
	typedef BOOL (CALLBACK* pdh_Fmt)(HCOUNTER, DWORD, LPDWORD, PPDH_FMT_COUNTERVALUE);
	typedef BOOL (CALLBACK* pdh_Close)(HQUERY);
	DWORD dwPriority = 0;
	pdh_Open fn = 0;;
	pdh_Add fn2 = 0;
	pdh_Fmt fnFmt = 0;
	GString strExecutablePath("N/A");
	GString strQueryBase("\\Process(");
	strQueryBase << pzProcessName << ")\\";
	ProcessWindowTitle pwt;

	HQUERY hQuery;
	fn = (pdh_Open)GetProcAddress(dllHandle,"PdhOpenQueryA");
	if (fn) 
	{
		if (0 != fn(NULL, 1, &hQuery)) 
		{
			goto PDB_FAIL_EARLY;
		}
	}
	else
	{
		goto PDB_FAIL_EARLY;
	}

    HCOUNTER hCounter0;
    HCOUNTER hCounter1;
    HCOUNTER hCounter2;
    HCOUNTER hCounter3;
    HCOUNTER hCounter4;
    HCOUNTER hCounter5;

	fn2 = (pdh_Add)GetProcAddress(dllHandle,"PdhAddCounterA");
	if (fn2) 
	{
		GString strQuery;
		strQuery = strQueryBase;
		strQuery << "ID Process";
		if (0 != fn2(hQuery, (const char *)strQuery, 0, &hCounter0))
		{
			goto PDB_FAIL;
		}
		strQuery = strQueryBase;
		strQuery << "Elapsed Time";
		if (0 != fn2(hQuery, (const char *)strQuery, 0, &hCounter1))
		{
			goto PDB_FAIL;
		}

		strQuery = strQueryBase;
		strQuery << "% Processor Time";
		if (0 != fn2(hQuery, (const char *)strQuery, 0, &hCounter2))
		{
			goto PDB_FAIL;
		}
		strQuery = strQueryBase;
		strQuery << "Working Set";
		if (0 != fn2(hQuery, (const char *)strQuery, 0, &hCounter3))
		{
			goto PDB_FAIL;
		}
		
		strQuery = strQueryBase;
		strQuery << "Thread Count";
		if (0 != fn2(hQuery, (const char *)strQuery, 0, &hCounter4))
		{
			goto PDB_FAIL;
		}

		strQuery = strQueryBase;
		strQuery << "Handle Count";
		if (0 != fn2(hQuery, (const char *)strQuery, 0, &hCounter5))
		{
			goto PDB_FAIL;
		}
	}
	else
	{
		goto PDB_FAIL;
	}

	// Execute Query twice, Sleep inbetween, or the Process/CPU % is always 0, -- This was not in the Documentation.
	if (0 != ((pdh_Get)GetProcAddress(dllHandle,"PdhCollectQueryData"))(hQuery))
	{
		goto PDB_FAIL;
	}
	Sleep(50);
	if (0 != ((pdh_Get)GetProcAddress(dllHandle,"PdhCollectQueryData"))(hQuery))
	{
		goto PDB_FAIL;
	}


	fnFmt = (pdh_Fmt)GetProcAddress(dllHandle,"PdhGetFormattedCounterValue");
    PDH_FMT_COUNTERVALUE pdhFormattedValue0;
    PDH_FMT_COUNTERVALUE pdhFormattedValue1;
    PDH_FMT_COUNTERVALUE pdhFormattedValue2;
    PDH_FMT_COUNTERVALUE pdhFormattedValue3;
    PDH_FMT_COUNTERVALUE pdhFormattedValue4;
    PDH_FMT_COUNTERVALUE pdhFormattedValue5;
	if (0 != fnFmt(hCounter0,PDH_FMT_LONG,NULL,&pdhFormattedValue0))
	{
		goto PDB_FAIL;
	}
	if (0 != fnFmt(hCounter1,PDH_FMT_LONG,NULL,&pdhFormattedValue1))
	{
		goto PDB_FAIL;
	}
	if (0 != fnFmt(hCounter2,PDH_FMT_LONG,NULL,&pdhFormattedValue2))
	{
		goto PDB_FAIL;
	}
	if (0 != fnFmt(hCounter3,PDH_FMT_LARGE,NULL,&pdhFormattedValue3)) // 64 bit PDH_FMT_LARGE 
	{
		goto PDB_FAIL;
	}
	if (0 != fnFmt(hCounter4,PDH_FMT_LONG,NULL,&pdhFormattedValue4))
	{
		goto PDB_FAIL;
	}
	if (0 != fnFmt(hCounter5,PDH_FMT_LONG,NULL,&pdhFormattedValue5))
	{
		goto PDB_FAIL;
	}



	// Get Full Pathname of the binary for this process
	HINSTANCE      hInstLib ;
	hInstLib = LoadLibraryA( "PSAPI.DLL" ) ;
	if (hInstLib)
	{
		HANDLE hProcess = OpenProcess(	PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,FALSE,  pdhFormattedValue0.longValue) ;
		if (hProcess)
		{
			dwPriority = GetPriorityClass(hProcess);

			BOOL (WINAPI *lpfEnumProcessModules)( HANDLE, HMODULE *,DWORD, LPDWORD );
			lpfEnumProcessModules = (BOOL(WINAPI *)(HANDLE, HMODULE *,DWORD, LPDWORD)) GetProcAddress( hInstLib,"EnumProcessModules" ) ;
			if (lpfEnumProcessModules)
			{
				HMODULE        hMod ;
				DWORD		   dwSize;
				if( lpfEnumProcessModules( hProcess, &hMod,	sizeof( hMod ), &dwSize ) )
				{
					char szFileName[512];

					DWORD (WINAPI *fnFileName)( HANDLE, HMODULE,char *, DWORD );
					fnFileName = (DWORD(WINAPI *)(HANDLE, HMODULE, char *, DWORD)) GetProcAddress(hInstLib, "GetModuleFileNameExA");



					if( fnFileName( hProcess, hMod, szFileName, sizeof( szFileName ) ) )
					{
						strExecutablePath = szFileName;
					}
				}
			}
			CloseHandle( hProcess ) ;
		}
	}

	// get the window title for this process
	pwt.pid = pdhFormattedValue0.longValue;
	EnumWindows((WNDENUMPROC) EnumWindowsProc, (LPARAM)&pwt);


	if(pG->Length())
		*pG << "\n";
	pdhFormattedValue3.largeValue = (pdhFormattedValue3.largeValue) ? pdhFormattedValue3.largeValue : 1024; // don't divide by 0, it's illegal.

	*pG << pzProcessName														// process name
		<< "*" << pdhFormattedValue0.longValue									// Process ID
		<< "*" << pdhFormattedValue2.longValue									// % Processor Time
		<< "*" << (__int64)(pdhFormattedValue3.largeValue / (__int64)1024)      // Working Set
		<< "*" << pdhFormattedValue1.longValue									// Elapsed Time
		<< "*" <<  pwt.strTitle << strExecutablePath							// binary executable file
		<< "*" << dwPriority													// process priority
		<< "*" << pdhFormattedValue4.longValue									// Thread Count
		<< "*" << pdhFormattedValue5.longValue;									// Handle Count

	((pdh_Close)GetProcAddress(dllHandle,"PdhCloseQuery"))(hQuery);
	return;

PDB_FAIL:
	((pdh_Close)GetProcAddress(dllHandle,"PdhCloseQuery"))(hQuery);
PDB_FAIL_EARLY:
	if(pG->Length())
		*pG << "\n";
	*pG << pzProcessName << "*" << "N/A" << "*" << "N/A" << "*" << "N/A" << "*" << "N/A" << "*" << "N/A" << "*" << "N/A"<< "*" << "N/A"<< "*" << "N/A";
	return;
}

void ListProcs(GString *pG)
{
	typedef BOOL (CALLBACK* pdh_EnumOb)(LPCSTR, LPCSTR, LPCSTR, LPSTR, LPDWORD, LPSTR ,LPDWORD, DWORD ,DWORD);
    HINSTANCE dllHandle = LoadLibraryA("pdh");
	if (!dllHandle)
		return;
	
	pdh_EnumOb fn;
	fn = (pdh_EnumOb)GetProcAddress(dllHandle,"PdhEnumObjectItemsA");
	if (!fn) 
	{
		return;
	}

	
	PDH_STATUS status;

    CHAR mszCounterList[4096];
	mszCounterList[0] = 0;
    DWORD cchCounterList = sizeof(mszCounterList) / sizeof(TCHAR);
    CHAR mszInstanceList[4096];
	mszInstanceList[0] = 0;
    DWORD cchInstanceList = sizeof(mszInstanceList) / sizeof(TCHAR);
    
    status = fn(NULL,NULL,"Process",mszCounterList,&cchCounterList,mszInstanceList,&cchInstanceList,400,0 );

    if ( ERROR_SUCCESS != status )
        return;

    if ( cchInstanceList > 2 )
    {
		GString strLastInstance;
		int nCount = 0;
		char *pszInstance = mszInstanceList;
        while ( *pszInstance )
        {
			if (strLastInstance.CompareNoCase(pszInstance) == 0)
			{
				nCount++;
				GString str(pszInstance);
				str << "#" << nCount;
				GetProcessStats(pG,str,dllHandle);
			}
			else
			{
				nCount=0;
				GetProcessStats(pG,pszInstance,dllHandle);

			}
			strLastInstance = pszInstance;

//            pszInstance += lstrlen(pszInstance) + 1;
			pszInstance += strlen(pszInstance) + 1;
		}
    }
	FreeLibrary(dllHandle);
}
#endif ////#if defined(_WIN32) && !defined(WINCE)


#ifdef _WIN32
BOOL WINAPI EnumProcs( PROCENUMPROC lpProc, LPARAM lParam )
{
	HINSTANCE      hInstLib = 0;
	HANDLE         hSnapShot ;
	PROCESSENTRY32 procentry ;
	BOOL           bFlag ;
	HANDLE (WINAPI *lpfCreateToolhelp32Snapshot)(DWORD,DWORD) ;
	BOOL (WINAPI *lpfProcess32First)(HANDLE,LPPROCESSENTRY32) ;
	BOOL (WINAPI *lpfProcess32Next)(HANDLE,LPPROCESSENTRY32) ;
#ifdef WINCE
	lpfCreateToolhelp32Snapshot= CreateToolhelp32Snapshot;
	lpfProcess32First = Process32First;
	lpfProcess32Next  = Process32Next;
#else
	hInstLib = LoadLibraryA("Kernel32.DLL" );
	if( hInstLib == NULL )
		return FALSE ;

	lpfCreateToolhelp32Snapshot=(HANDLE(WINAPI *)(DWORD,DWORD))GetProcAddress( hInstLib,"CreateToolhelp32Snapshot" ) ;
	lpfProcess32First= (BOOL(WINAPI *)(HANDLE,LPPROCESSENTRY32))GetProcAddress( hInstLib, "Process32First" ) ;
	lpfProcess32Next= (BOOL(WINAPI *)(HANDLE,LPPROCESSENTRY32))GetProcAddress( hInstLib, "Process32Next" ) ;
#endif
	if( lpfProcess32Next == NULL || lpfProcess32First == NULL || lpfCreateToolhelp32Snapshot == NULL )
	{
		FreeLibrary( hInstLib ) ;
		return FALSE ;
	}

	hSnapShot = lpfCreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0 ) ;
	if( hSnapShot == INVALID_HANDLE_VALUE )
	{
		FreeLibrary( hInstLib ) ;
		return FALSE ;
	}

	procentry.dwSize = sizeof(PROCESSENTRY32) ;
	bFlag = lpfProcess32First( hSnapShot, &procentry ) ;

	// While there are processes, keep looping.
	while( bFlag )
	{
			GString strConvertName; // converts from unicode if necessary
			strConvertName << procentry.szExeFile;

		// Call the enum func with the filename and ProcID.
		if(lpProc( procentry.cntThreads, procentry.th32ProcessID, 0,strConvertName.Buf(), lParam ))
		{
			procentry.dwSize = sizeof(PROCESSENTRY32) ;
			bFlag = lpfProcess32Next( hSnapShot, &procentry );
		}
		else
			bFlag = FALSE ;
	}
	if (hInstLib)    
		FreeLibrary( hInstLib ) ;
	return TRUE ;
}

void MakeProcessName(GString &rDest,const char *pzIn)
{
	int nLen = strlen(pzIn);
	// walk backwards to the first slash
	for(int n = nLen; n > 0; n--)
	{
		if (pzIn[n] == '\\' || pzIn[n] == '/')
		{
			rDest = &pzIn[n+1];
			break;
		}
	}
	if (rDest.Length() == 0)
	{
		rDest = pzIn;
	}
	

	// lop off the .exe, if the name ends with ".exe"
	if (rDest.Length() > 4)
	{
		if (rDest.CompareSubNoCase(".exe",rDest.Length() - 4) == 0)
		{
			rDest.SetLength( rDest.Length() - 4 );
		}
	}
}


BOOL CALLBACK ProcessListCallBack( DWORD cntThreads, DWORD pid, WORD zero, LPSTR pzName,  LPARAM lParam)
{
	if (pzName && pzName[0] && pid > 0)
	{
		DWORD dwPriority = 3;
#ifndef WINCE
		HANDLE hProcess = OpenProcess( PROCESS_ALL_ACCESS, FALSE, pid );
		if (hProcess) 
		{
			dwPriority = GetPriorityClass(hProcess);
			CloseHandle( hProcess ) ;
		}
#endif
		
		ProcessWindowTitle pwt;
		pwt.pid = pid;
		EnumWindows((WNDENUMPROC) EnumWindowsProc, (LPARAM)&pwt);

		GString *pG = (GString *)lParam;
		if(pG->Length())
			*pG << "\n";

		GString strProcessName;
		MakeProcessName(strProcessName,pzName);

		*pG << strProcessName << '*' << pid  << '*';
		*pG << "N/A*N/A*N/A";
		*pG << '*' << pwt.strTitle << pzName << '*' << dwPriority << "*" << cntThreads << "*N/A";
		}
	return 1;
}
#endif

struct MoreTreadInfo
{
	int nCount;
	GString strUser;
	MoreTreadInfo()
	{
		nCount = 0;
	}
};


#ifdef _ANDROID


void ParseAndroidProcessInfo(GString &g,GString &strResult)
{
	GString strUser;
	GString strPID;
	GString strPPID;
	GString strVSize;
	GString strRSS;
	GString strWCHAN;
	GString strPC;
	GString strName;

	__int64 term = strUser.SetFromUpTo(g," ");
	g.TrimLeftBytes(term);
	g.TrimLeftWS();

	term = strPID.SetFromUpTo(g," ");
	g.TrimLeftBytes(term);
	g.TrimLeftWS();

	term = strPPID.SetFromUpTo(g," ");
	g.TrimLeftBytes(term);
	g.TrimLeftWS();

	term = strVSize.SetFromUpTo(g," ");
	g.TrimLeftBytes(term);
	g.TrimLeftWS();

	term = strRSS.SetFromUpTo(g," ");
	g.TrimLeftBytes(term);
	g.TrimLeftWS();

	term = strWCHAN.SetFromUpTo(g," ");
	g.TrimLeftBytes(term);
	g.TrimLeftWS();

	term = strPC.SetFromUpTo(g," ");
	g.TrimLeftBytes(term);
	g.TrimLeftWS();

	term = strPC.SetFromUpTo(g," ");
	g.TrimLeftBytes(term);
	g.TrimLeftWS();
	
	strName = g;


//  we have this (unwanted) process information
//	GString strPPID; // process parent id
//	GString strVSize; // some heap memory
//	GString strWCHAN;// Memory address of the event the process is waiting for 
//	GString strPC; // either R, S, or D - dunno what it is

	// ---- we want this - and give it what we have -----
	GString c("0");// c= % Processor Time
	GString e(strPPID);		// e= Elapsed Time
	GString g2(strWCHAN);// g= Process priority
	GString h(strPC);	// h= Thread Count 


	if (strPID != "PID")
	{
		if (strResult.Length())
			strResult << "\n";
		strResult << GDirectory::LastLeaf(strName) << '*' << strPID << '*' << c << '*' << strRSS << '*' << e << '*' << strName << '*' << g2 << '*' << h << '*' << strUser;
	}
}

#endif


extern void JavaShellExec(GString &strCommand, GString &strResult); // in GAppServer.cpp



void GetProcessList( GString *pstrResults )
{
	GString strResult(32767);
#ifdef _WIN32
	#ifndef WINCE 
		OSVERSIONINFO  osver ;
		osver.dwOSVersionInfoSize = sizeof( osver ) ;
		GetVersionEx( &osver );
		if( osver.dwPlatformId == VER_PLATFORM_WIN32_NT ) // If Windows NT, 2K, XP, Vista, 7, 8 or 10
		{
			ListProcs(pstrResults);
		}
	#endif
	if (pstrResults->IsEmpty()) // 95, 98, ME or failure in NT (NT will fail without Admin privilege)
	{
		EnumProcs( ProcessListCallBack, (long)pstrResults ); 
	}
#else

	//	note: use "ps -eLf"  to get additional process info - thread count per process and user name ("root" or "brian")
	// store the value in a keyed data structure indexed by ProcessID, that we will tie into the full process info with.

	GBTree tree;
	FILE *fpipe1;
	char *command1="ps -eLf";
	char line1[256];

	if ( (fpipe1 = (FILE*)popen(command1,"r")) )
	{  
		fgets( line1, sizeof(line1), fpipe1); // the first row is headers
		while ( fgets( line1, sizeof(line1), fpipe1) )
		{

			GString a;
			a.SetFromUpTo(line1," ");		// assign 'a'(the name) up to the first space
			
			char *pFirstNumeric = (char *)strpbrk(line1,"0123456789"); 
			GString b;	// Process ID
			if (pFirstNumeric)
				b.SetFromUpTo(pFirstNumeric," "); 


			MoreTreadInfo *pVal = (MoreTreadInfo *)tree.search(b.Buf());
			if (!pVal)
			{
				pVal = new MoreTreadInfo;
				pVal->nCount++;
				pVal->strUser = a;
				tree.insert(b.Buf(),pVal);
			}
			else
			{
				pVal->nCount++;
			}
		}
		pclose(fpipe1);
	}
#ifdef _ANDROID
	
	// Android has no popen() implemented, we will JNI up to the Java bootloader application
	// which executes the same via code in the JVM.
	GString strCommand("ps");
//	GString strCommand("ps -eo comm:15,pid:7,pcpu:5,sz:8,etime:12,command:25,pri:4");

	GString strCmdResult;
	JavaShellExec(strCommand, strCmdResult); // executes any command that the ADB Debugger can.

	int nStart = 0;
	for(int i=0; i<strCmdResult.Length(); i++)
	{
		if (strCmdResult.GetAt(i) == '\n' )
		{
			GString g;
			g.SetFromUpTo(strCmdResult.StartingAt(nStart),"\n");
			nStart = i+1;
			ParseAndroidProcessInfo(g,*pstrResults);
		}
		if (i == pstrResults->Length()-1)
		{
			GString g(strCmdResult.StartingAt(nStart));
			ParseAndroidProcessInfo(g,*pstrResults);
		}
	}
#else

	FILE *fpipe;
	char *command="ps -eo comm:15,pid:7,pcpu:5,sz:8,etime:12,command:25,pri:4";
	char line[256];

	if ( (fpipe = (FILE*)popen(command,"r")) )
	{  
		fgets( line, sizeof(line), fpipe); // the first row is headers
		while ( fgets( line, sizeof(line), fpipe) )
		{
			GString a(line					,15);	// process name
			GString b(line+15+1				,7);	// Process ID
			GString c(line+15+7+2			,5);	// % Processor Time
			GString d(line+15+7+5+3			,8);	// memory
			GString e(line+15+7+5+8+4		,12);	// Elapsed Time
			GString f(line+15+7+5+8+12+5	,25);	// binary executable file
			GString g(line+15+7+5+8+12+25+6 ,4);	// Process priority
													// Thread Count "N/A"
													// Handle Count "N/A"
			a.TrimRightWS(); a.TrimLeftWS();
			b.TrimRightWS(); b.TrimLeftWS();
			c.TrimRightWS(); c.TrimLeftWS();
			d.TrimRightWS(); d.TrimLeftWS();
			e.TrimRightWS(); e.TrimLeftWS();
			f.TrimRightWS(); f.TrimLeftWS();
			g.TrimRightWS(); g.TrimLeftWS();

			GString h("N/A");
			GString i("N/A");

			MoreTreadInfo *pVal = (MoreTreadInfo *)tree.search(b.Buf());
			if (pVal)
			{
				h = pVal->nCount;
				i = pVal->strUser;
			}

			
			GString strExclude( f.Buf(), strlen("ps -eo") );
			if (strExclude != "ps -eo")
			{
				if (pstrResults->Length())
					*pstrResults << "\n";
				*pstrResults << a << '*' << b << '*' << c << '*' << d << '*' << e << '*' << f << '*' << g << '*' << h << '*' << i;
			}
		}
		pclose(fpipe);
	}
#endif

#endif

}


#if defined(_MSC_VER) && _MSC_VER <= 1200		// for Visual Studio VC6
typedef enum 

{
  TCP_TABLE_BASIC_LISTENER,
  TCP_TABLE_BASIC_CONNECTIONS,
  TCP_TABLE_BASIC_ALL,
  TCP_TABLE_OWNER_PID_LISTENER,
  TCP_TABLE_OWNER_PID_CONNECTIONS,
  TCP_TABLE_OWNER_PID_ALL,
  TCP_TABLE_OWNER_MODULE_LISTENER,
  TCP_TABLE_OWNER_MODULE_CONNECTIONS,
  TCP_TABLE_OWNER_MODULE_ALL
} TCP_TABLE_CLASS, *PTCP_TABLE_CLASS;
#endif
/*
typedef enum _UDP_TABLE_CLASS {
    UDP_TABLE_BASIC,
    UDP_TABLE_OWNER_PID,
    UDP_TABLE_OWNER_MODULE
} UDP_TABLE_CLASS, *PUDP_TABLE_CLASS;
*/


// from <winbase.h> on Windows platforms
#ifndef _WIN32
	#define NORMAL_PRIORITY_CLASS       0x00000020
	#define IDLE_PRIORITY_CLASS         0x00000040
	#define HIGH_PRIORITY_CLASS         0x00000080
	#define REALTIME_PRIORITY_CLASS     0x00000100
#endif 


GProcessListRow::GProcessListRow(const char *pzRow)
{
	strUnparsedData = pzRow;

	GStringList lSub("*",pzRow);				// each column divided by "*"
	GStringIterator itSub(&lSub);

	if( itSub() )
		strName  = itSub++;

	if(itSub())
		strPID = itSub++;		 // PID
	
	if(itSub())
		strCPU = itSub++;		 // % CPU

	if(itSub())
		strMem = itSub++;		 // Memory
	strMem.CommaNumeric();
	
	if(itSub())
		strRunTime = itSub++;   // run time on the clock

	if(itSub())
		strBinaryPath = itSub++; // Binary Executable Path

	if(itSub())
	{
		unsigned int dw = atol(itSub++);		 // Process Priority
		switch (dw)
		{
			case NORMAL_PRIORITY_CLASS: strPriority = "Normal"; break;
			case IDLE_PRIORITY_CLASS:	strPriority = "Idle"; break;
			case HIGH_PRIORITY_CLASS:	strPriority = "High"; break;
			case REALTIME_PRIORITY_CLASS: strPriority="RealTime"; break;
			default : strPriority << dw;
		}
	}
	
	if(itSub())
		strThreads = itSub++;      // # threads

	if(itSub())				// # handles
		strHandles = itSub++;

}

#ifdef _WIN32
bool GetNetworkConnections(GString &strResults, int nFlags)
{

	// ------------------  Get a snapshot all processes, parse them into hashProcessSnapshot keyed on PID  ----------------
	//                        we will tie this information to the IP Address and port va PID (Process ID) -----------------
	GString strRunningProcessData;
	GetProcessList( &strRunningProcessData );
	GHash hashProcessSnapshot;
	GStringList l("\n",strRunningProcessData); // each row divided by "\n"
	GStringIterator it(&l);
	while(it())
	{
		GProcessListRow *pRow = new GProcessListRow(it++);
		hashProcessSnapshot.Insert((unsigned int)atoi(pRow->strPID),pRow); 
	}
	// --------------------------------------------------------------------------------------------------------------------
	// Go figure, some ports are open by fantom processes.  Either the process was there and is not or has somehow evaded the process list
	// When we tie our Network data to the Process data keyed on PID, any non-paired PIDS get this default where strName = "X"
	GProcessListRow defaultRow;
	defaultRow.strName	="X";
	defaultRow.strPID   = 0;
	defaultRow.strCPU   = 0;
	defaultRow.strMem   = 0;
	defaultRow.strRunTime = 0;
	defaultRow.strBinaryPath = "";
	defaultRow.strPriority = 0;
	defaultRow.strThreads = 0;
	defaultRow.strHandles = 0;
	// --------------------------------------------------------------------------------------------------------------------



	char buffer[32767];
	unsigned long nSize = 32767;
	static long null_address[4] = { 0, 0, 0, 0 };

	typedef PSTR (NTAPI *_RtlIpv4AddressToStringA)(struct in_addr *, PSTR /* __out_ecount(16) */);
	typedef PSTR (NTAPI *_RtlIpv6AddressToStringA)(struct in6_addr *,PSTR /* __out_ecount(65) */);
	_RtlIpv6AddressToStringA rtlIpv6AddressToStringA;
	_RtlIpv4AddressToStringA rtlIpv4AddressToStringA;

    HINSTANCE ntdll = LoadLibrary(TEXT("ntdll.dll"));
	if (ntdll)
	{
		rtlIpv4AddressToStringA = (_RtlIpv4AddressToStringA)GetProcAddress(ntdll,"RtlIpv4AddressToStringA");
		rtlIpv6AddressToStringA = (_RtlIpv6AddressToStringA)GetProcAddress(ntdll,"RtlIpv6AddressToStringA");
	}

	// load this "special" Iphlpapi function at runtime, most other Iphlpapi methods can use early binding.
	HINSTANCE hstLibrary = LoadLibraryA("C:\\Windows\\System32\\Iphlpapi.dll");

	if(!hstLibrary)
	{
		FreeLibrary(hstLibrary);
		return false;
	}
	// load function address from dll
	typedef DWORD (WINAPI *GetExtendedUdpTableX)(PVOID, PDWORD, BOOL, ULONG, UDP_TABLE_CLASS, ULONG);   GetExtendedUdpTableX GetUDPTable;
	typedef DWORD (WINAPI *GetExtendedTcpTableX)(PVOID, PDWORD, BOOL, ULONG, TCP_TABLE_CLASS, ULONG);   GetExtendedTcpTableX GetTCPTable;
	GetTCPTable = (GetExtendedTcpTableX)GetProcAddress(hstLibrary, "GetExtendedTcpTable");
	GetUDPTable = (GetExtendedUdpTableX)GetProcAddress(hstLibrary, "GetExtendedUdpTable");



	if ( ( nFlags & NET_FLAG_NO_TCP4 ) == 0)
	{

		int res = GetTCPTable(buffer, &nSize, true, AF_INET, TCP_TABLE_OWNER_PID_ALL, 0);
		if (res != 0) //If there is not enough memory to execute function
		{
			FreeLibrary(hstLibrary);
			return false;
		}

		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//										Get TCP4 Table
		PMIB_TCPTABLE_OWNER_PID tcp4Table = (PMIB_TCPTABLE_OWNER_PID)(void *)&buffer[0];

		for (unsigned int i = 0; i < tcp4Table->dwNumEntries; i++)
		{
			int nPID = tcp4Table->table[i].dwOwningPid;
			
			GString strConnectedIPAddress;
			GString strConnectedIPAddress2;

			GProcessListRow *pRow = (GProcessListRow *)hashProcessSnapshot.Lookup((unsigned int)nPID);
			if(!pRow)
				pRow = &defaultRow;

			if (tcp4Table->table[i].dwLocalAddr != 0)
			{
				struct in_addr addr;
				addr.S_un.S_addr = tcp4Table->table[i].dwLocalAddr;
				//rtlIpv4AddressToStringA(&addr, addressBufferLocal);// we can use this new interface or the old inet_ntoa()
				strConnectedIPAddress = inet_ntoa(addr); // get his string n.n.n.n IP address

			}
			// On Windows <= XP, remote addr is filled even if socket is in LISTEN mode in which case we just ignore it.
			if (  tcp4Table->table[i].dwRemoteAddr != 0 )
			{
				struct in_addr addr;
				addr.S_un.S_addr = tcp4Table->table[i].dwRemoteAddr;
				//rtlIpv4AddressToStringA(&addr, addressBufferRemote); // we can use this new interface or the old inet_ntoa()
				strConnectedIPAddress2 = inet_ntoa(addr); // get his string n.n.n.n IP address
			}

			if (tcp4Table->table[i].dwState != 2) // they all have a remote port of 0 and no other info except a local port
			{
				if (nFlags & NET_FLAG_REDUCE_INFO)
				{
					strResults	<< "TCP4 pid:" << nPID << "          TO=" << strConnectedIPAddress2 << " port:" << tcp4Table->table[i].dwRemotePort  << "              FROM=" << strConnectedIPAddress << " port: "<< tcp4Table->table[i].dwLocalPort
								<< "       ["  <<  pRow->strName << "]" << pRow->strBinaryPath << "\n";; 
				}
				else
				{
					strResults	<< "TCP4," << nPID << "," << tcp4Table->table[i].dwLocalPort << "," << strConnectedIPAddress << "," << tcp4Table->table[i].dwRemotePort << "," << strConnectedIPAddress2 
								<< "," 	   <<  pRow->strName << "," <<  pRow->strCPU << "," << pRow->strMem << "," << pRow->strRunTime << "," << pRow->strThreads 
								<< ","     << pRow->strHandles << "," << pRow->strPriority << "," << pRow->strBinaryPath << "\n";
				}
			}
		}
	}
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//										Get UDP4 Table
	if ( ( nFlags & NET_FLAG_NO_UDP4 ) == 0)
	{
		nSize = 32767;
		int res = GetUDPTable(buffer, &nSize, false, AF_INET, UDP_TABLE_OWNER_PID, 0);
		if (res != 0)
		{
			FreeLibrary(hstLibrary);
			return false;
		}
		PMIB_UDPTABLE_OWNER_PID udp4Table = (PMIB_UDPTABLE_OWNER_PID)(void *)&buffer[0];
		for (unsigned int i = 0; i < udp4Table->dwNumEntries; i++)
		{
			int nPID = udp4Table->table[i].dwOwningPid;
			GProcessListRow *pRow = (GProcessListRow *)hashProcessSnapshot.Lookup((unsigned int)nPID);
			if(!pRow)
				pRow = &defaultRow;

			GString strConnectedIPAddress;
			if (udp4Table->table[i].dwLocalAddr != 0 ||  udp4Table->table[i].dwLocalPort != 0)
			{
				struct in_addr addr;
				addr.S_un.S_addr = udp4Table->table[i].dwLocalAddr;
				//rtlIpv4AddressToStringA(&addr, addressBufferLocal);
				strConnectedIPAddress = inet_ntoa(addr); // get the string n.n.n.n IP address

				if (nFlags & NET_FLAG_REDUCE_INFO)
				{
					strResults	<< "UDP4 pid:" << nPID << "      IP=" << strConnectedIPAddress << " port:" << udp4Table->table[i].dwLocalPort 
								<< "       ["  <<  pRow->strName << "]" << pRow->strBinaryPath << "\n"; 
				}
				else
				{
					strResults	<< "UDP4," << nPID << "," << udp4Table->table[i].dwLocalPort << "," << strConnectedIPAddress << ",(0),(no remote)" 
								<< "," 	   <<  pRow->strName << ","  <<  pRow->strCPU << "," << pRow->strMem << "," << pRow->strRunTime << "," << pRow->strThreads 
								<< ","     << pRow->strHandles << "," << pRow->strPriority << "," << pRow->strBinaryPath << "\n";
				}
			}
		}
	}
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//										Get TCP6 Table
	if ( ( nFlags & NET_FLAG_NO_TCP6 ) == 0)
	{
		nSize = 32767;
		if (GetTCPTable(buffer, &nSize, false, AF_INET6,  TCP_TABLE_OWNER_PID_ALL, 0) == 0)
		{
			PMIB_TCP6TABLE_OWNER_PID tcp6Table = (PMIB_TCP6TABLE_OWNER_PID)(void *)&buffer[0];
			for (unsigned int i = 0; i < tcp6Table->dwNumEntries; i++)
			{
				int nPID = tcp6Table->table[i].dwOwningPid;
				GProcessListRow *pRow = (GProcessListRow *)hashProcessSnapshot.Lookup((unsigned int)nPID);
				if(!pRow)
					pRow = &defaultRow;

				char addressBufferLocal[256];
				char addressBufferRemote[256];

				if (memcmp(tcp6Table->table[i].ucLocalAddr, null_address, 16) != 0 || tcp6Table->table[i].dwLocalPort != 0)
				{
					struct in6_addr addr;
					memcpy(&addr, tcp6Table->table[i].ucLocalAddr, 16);
					rtlIpv6AddressToStringA(&addr, addressBufferLocal);
				}

            
				if ((memcmp(tcp6Table->table[i].ucRemoteAddr, null_address, 16) != 0 ||  tcp6Table->table[i].dwRemotePort != 0) )
				{
					struct in6_addr addr;
					memcpy(&addr, tcp6Table->table[i].ucRemoteAddr, 16);
					rtlIpv6AddressToStringA(&addr, addressBufferRemote);


					if (nFlags & NET_FLAG_REDUCE_INFO)
					{
						strResults	<< "TCP6 pid:" << nPID << "          TO=" << addressBufferRemote << " port:" << tcp6Table->table[i].dwRemotePort  << "              FROM=" << addressBufferLocal << " port: "<< tcp6Table->table[i].dwLocalPort
									<< "       ["  <<  pRow->strName << "]" << pRow->strBinaryPath << "\n";

					}
					else
					{
						strResults	<< "TCP6," << nPID << "," << tcp6Table->table[i].dwLocalPort << "," << addressBufferLocal << "," << tcp6Table->table[i].dwRemotePort << "," <<  addressBufferRemote
									<< "," 	   <<  pRow->strName << "," << pRow->strCPU << "," << pRow->strMem << "," << pRow->strRunTime << "," << pRow->strThreads 
									<< ","     <<  pRow->strHandles << "," << pRow->strPriority << "," << pRow->strBinaryPath << "\n";
					}
				}
			}
		}
	}
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//											Get UDP6 Table
	if (( nFlags & NET_FLAG_NO_UDP6 ) == 0)
	{
		nSize = 32767;
		int res = GetUDPTable(buffer, &nSize, false, AF_INET, UDP_TABLE_OWNER_PID, 0);
		if (res != 0)
		{
			FreeLibrary(hstLibrary);
			return false;
		}
		PMIB_UDP6TABLE_OWNER_PID udp6Table = (PMIB_UDP6TABLE_OWNER_PID)(void *)&buffer[0];
		char addressBufferLocal[256];
		for (unsigned int i = 0; i < udp6Table->dwNumEntries; i++)
		{
			int nPID = udp6Table->table[i].dwOwningPid;
			GProcessListRow *pRow = (GProcessListRow *)hashProcessSnapshot.Lookup((unsigned int)nPID);
			if(!pRow)
				pRow = &defaultRow;

			if (memcmp(udp6Table->table[i].ucLocalAddr, null_address, 16) != 0  && udp6Table->table[i].dwLocalPort != 0)
			{
				struct in6_addr addr;
				memcpy(&addr, udp6Table->table[i].ucLocalAddr, 16);
				rtlIpv6AddressToStringA(&addr, addressBufferLocal);
				if (nFlags & NET_FLAG_REDUCE_INFO)
				{
					strResults	<< "UDP6 pid:" << nPID << "      IP=" << addressBufferLocal << " port:" << udp6Table->table[i].dwLocalPort
								<< "   name["  <<  pRow->strName << "]" << pRow->strBinaryPath << "\n";; 
				}
				else
				{
					strResults	<< "UDP6," << nPID << "," << udp6Table->table[i].dwLocalPort << "," << addressBufferLocal << ",(0),(no remote ip)"
								<< "," 	   <<  pRow->strName <<  "," <<  pRow->strCPU << "," << pRow->strMem << "," << pRow->strRunTime << "," << pRow->strThreads 
								<< ","     <<  pRow->strHandles << "," << pRow->strPriority << "," << pRow->strBinaryPath << "\n";
				}
			}

		}
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	GHashIterator itH (&hashProcessSnapshot);
	while(itH())
	{
		delete (GProcessListRow *)itH++;
	}


	return true;
}
#else
bool GetNetworkConnections(GString &strResults, int nFlags)
{
	return 0;
}
#endif // #ifdef _WIN32


void InternalIPs(GStringList*plstBoundIPAddresses)
{
	//////////////////////////////////////////////////////////////////////////////////
	// enumerate all IPs bound to each network interface	
	//////////////////////////////////////////////////////////////////////////////////
#ifdef _WIN32

	DWORD dwSize = 0;
	MIB_IPADDRTABLE *pIPAddrTable = (MIB_IPADDRTABLE *) malloc(sizeof (MIB_IPADDRTABLE));


	// Make an initial call to GetIpAddrTable to get the necessary size into the dwSize variable
	if (GetIpAddrTable(pIPAddrTable, &dwSize, 0) ==  ERROR_INSUFFICIENT_BUFFER) 
	{
        free(pIPAddrTable);
		pIPAddrTable = (MIB_IPADDRTABLE *) malloc(dwSize);
    }

	// Make a second call to GetIpAddrTable to get the actual data we want
	if ( GetIpAddrTable( pIPAddrTable, &dwSize, 0 ) == NO_ERROR ) 
	{ 
		IN_ADDR IPAddr;
		
		for (int i=0; i < (int) pIPAddrTable->dwNumEntries; i++) 
		{
			IPAddr.S_un.S_addr = (u_long) pIPAddrTable->table[i].dwAddr;

			GString g;
			g << "Type[" << pIPAddrTable->table[i].wType << "]:" << inet_ntoa(IPAddr);
			plstBoundIPAddresses->AddFirst(g);
		}
	}

	if (pIPAddrTable) 
	{
		free(pIPAddrTable);
		pIPAddrTable = NULL;
	}

#else //Linux

	struct ifaddrs * ifAddrStruct=NULL;
	struct ifaddrs * ifa=NULL;
	void * tmpAddrPtr=NULL;

	GString strLocalHost("127.0.0.1");
	getifaddrs(&ifAddrStruct);

	for (ifa = ifAddrStruct; ifa != NULL; ifa = ifa->ifa_next) 
	{
		if (!ifa->ifa_addr) 
		{
			continue;
		}
		if (ifa->ifa_addr->sa_family == AF_INET) 
		{ 
			// check if this is a valid IP4 Address
			tmpAddrPtr=&((struct sockaddr_in *)ifa->ifa_addr)->sin_addr;
			char addressBuffer[INET_ADDRSTRLEN];
			inet_ntop(AF_INET, tmpAddrPtr, addressBuffer, INET_ADDRSTRLEN);
			if (strLocalHost != addressBuffer)
				plstBoundIPAddresses->AddFirst(addressBuffer);
		} 
		// check it is IP6
		else if (ifa->ifa_addr->sa_family == AF_INET6) 
		{ 
			// is a valid IP6 Address
			tmpAddrPtr=&((struct sockaddr_in6 *)ifa->ifa_addr)->sin6_addr;
			char addressBuffer[INET6_ADDRSTRLEN];
			inet_ntop(AF_INET6, tmpAddrPtr, addressBuffer, INET6_ADDRSTRLEN);
			plstBoundIPAddresses->AddFirst(addressBuffer);
		} 
	}
	if (ifAddrStruct!=NULL) 
		freeifaddrs(ifAddrStruct);
  
	
#endif

}
