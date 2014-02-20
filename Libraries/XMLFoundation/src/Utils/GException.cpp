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
#undef new

#include "GException.h"
#include "GString.h"
#include <stdlib.h> //for: atoi()
#include <stdarg.h> //for: va_start(), va_end 
#include <string.h> //for: strlen(), memcpy()

#include "ErrorsEnglish.cpp"

static char *g_pzStaticErrMap = 0;
static GString g_strErrorFile;
static GString *g_pstrErrorFileContents = 0;

void SetErrorDescriptions(const char *pzErrData)
{

	if (g_pzStaticErrMap)
		delete[] g_pzStaticErrMap;
	g_pzStaticErrMap = new char[strlen(pzErrData)+1];
	memcpy(g_pzStaticErrMap,pzErrData,strlen(pzErrData)+1);
}

GlobalErrorLanguage::GlobalErrorLanguage()
{
	SetErrorDescriptions(pzBoundErrorDescriptions);
}
GlobalErrorLanguage::~GlobalErrorLanguage() 
{
	if (g_pzStaticErrMap)
	{
		delete g_pzStaticErrMap;
		g_pzStaticErrMap = 0;
	}
	if (g_pstrErrorFileContents)
	{
		delete g_pstrErrorFileContents;
		g_pstrErrorFileContents = 0;
	}
}
GlobalErrorLanguage _GEL;





GProfile &GetErrorProfile()
{
	if (g_strErrorFile.IsEmpty())
	{
		if (g_pzStaticErrMap)
		{
			g_strErrorFile = "Static Load";
			g_pstrErrorFileContents = new GString(g_pzStaticErrMap,strlen(g_pzStaticErrMap));
		}
		else
		{
			const char *pzErrFile = GetProfile().GetString("System", "Errors", 0);
			if (pzErrFile && pzErrFile[0])
			{
				g_pstrErrorFileContents = new GString();
				if (!g_pstrErrorFileContents->FromFile(pzErrFile,0))
				{
					pzErrFile = 0;
				}
				g_strErrorFile = pzErrFile;
			}
			// if the error file could not be found, default to a minimal error file
			if (!pzErrFile)
			{
				g_pstrErrorFileContents = new GString();
				(*g_pstrErrorFileContents) = "[Exception]\nSubSystem=0\n[Profile]\nSubSystem=1\n0=[%s.%s]Error Description File Not loaded.\n1=[%s]Error Description File Not loaded.  Call SetErrorDescriptions().\n";
				g_strErrorFile = "Static";
			}
		}
	}
	static GProfile ErrorProfile(g_pstrErrorFileContents->StrVal(), (int)g_pstrErrorFileContents->Length());
	return ErrorProfile;
}


#if  !defined(WINCE) && !defined(__WINPHONE) // Not available on Windows CE or Windows Phone
#if defined _DEBUG && _WIN32

#ifndef BORLANDC
	#include <eh.h>
#endif

#include <string>
#include <vector>
#pragma comment( lib, "imagehlp" )
//#pragma comment(lib, "dbghelp.lib")  


///////////////////////////////////////////////////////////////////////////
/////////////////////// Win32 stack tracing ///////////////////////////////


#define gle (GetLastError())
#define lenof(a) (sizeof(a) / sizeof((a)[0]))
#define MAXNAMELEN 1024 // max name length for found symbols
#define IMGSYMLEN ( sizeof (IMAGEHLP_SYMBOL) )
#define TTBUFLEN 65536 // for a temp buffer

void _stack_se_translator(unsigned int e, _EXCEPTION_POINTERS* p)
{
	HANDLE hThread;

	DuplicateHandle( GetCurrentProcess(), GetCurrentThread(),
		GetCurrentProcess(), &hThread, 0, false, DUPLICATE_SAME_ACCESS );

	GCallStack stk(hThread, *(p->ContextRecord));
	
	CloseHandle( hThread );

	throw stk;
}


GCallStack::GCallStack(const GCallStack &src)
{
	_stk = src._stk;
}

struct ModuleEntry
{
	std::string imageName;
	std::string moduleName;
	DWORD baseAddress;
	DWORD size;
};
typedef std::vector< ModuleEntry > ModuleList;
typedef ModuleList::iterator ModuleListIter;

// miscellaneous toolhelp32 declarations; we cannot #include the header
// because not all systems may have it
#define MAX_MODULE_NAME32 255
#define TH32CS_SNAPMODULE   0x00000008
#pragma pack( push, 8 )
typedef struct tagMODULEENTRY32
{
    DWORD   dwSize;
    DWORD   th32ModuleID;       // This module
    DWORD   th32ProcessID;      // owning process
    DWORD   GlblcntUsage;       // Global usage count on the module
    DWORD   ProccntUsage;       // Module usage count in th32ProcessID's context
    BYTE  * modBaseAddr;        // Base address of module in th32ProcessID's context
    DWORD   modBaseSize;        // Size in bytes of module starting at modBaseAddr
    HMODULE hModule;            // The hModule of this module in th32ProcessID's context
    char    szModule[MAX_MODULE_NAME32 + 1];
    char    szExePath[MAX_PATH];
} MODULEENTRY32;
typedef MODULEENTRY32 *  PMODULEENTRY32;
typedef MODULEENTRY32 *  LPMODULEENTRY32;
#pragma pack( pop )

bool fillModuleListTH32( ModuleList& modules, DWORD pid )
{
	// CreateToolhelp32Snapshot()
	typedef HANDLE (__stdcall *tCT32S)( DWORD dwFlags, DWORD th32ProcessID );
	// Module32First()
	typedef BOOL (__stdcall *tM32F)( HANDLE hSnapshot, LPMODULEENTRY32 lpme );
	// Module32Next()
	typedef BOOL (__stdcall *tM32N)( HANDLE hSnapshot, LPMODULEENTRY32 lpme );

	// I think the DLL is called tlhelp32.dll on Win9X, so we try both
	const char *dllname[] = { "kernel32.dll", "tlhelp32.dll" };
	HINSTANCE hToolhelp;
	tCT32S pCT32S;
	tM32F pM32F;
	tM32N pM32N;

	HANDLE hSnap;
	MODULEENTRY32 me = { sizeof me };
	bool keepGoing;
	ModuleEntry e;
	int i;

	for ( i = 0; i < lenof( dllname ); ++ i )
	{
		hToolhelp = LoadLibrary( dllname[i] );
		if ( hToolhelp == 0 )
			continue;
		pCT32S = (tCT32S) GetProcAddress( hToolhelp, "CreateToolhelp32Snapshot" );
		pM32F = (tM32F) GetProcAddress( hToolhelp, "Module32First" );
		pM32N = (tM32N) GetProcAddress( hToolhelp, "Module32Next" );
		if ( pCT32S != 0 && pM32F != 0 && pM32N != 0 )
			break; // found the functions!
		FreeLibrary( hToolhelp );
		hToolhelp = 0;
	}

	if ( hToolhelp == 0 ) // nothing found?
		return false;

	hSnap = pCT32S( TH32CS_SNAPMODULE, pid );
	if ( hSnap == (HANDLE) -1 )
		return false;

	keepGoing = !!pM32F( hSnap, &me );
	while ( keepGoing )
	{
		// here, we have a filled-in MODULEENTRY32
		e.imageName = me.szExePath;
		e.moduleName = me.szModule;
		e.baseAddress = (DWORD) me.modBaseAddr;
		e.size = me.modBaseSize;
		modules.push_back( e );
		keepGoing = !!pM32N( hSnap, &me );
	}

	CloseHandle( hSnap );

	FreeLibrary( hToolhelp );

	return modules.size() != 0;
}

// miscellaneous psapi declarations; we cannot #include the header
// because not all systems may have it
typedef struct _MODULEINFO {
    LPVOID lpBaseOfDll;
    DWORD SizeOfImage;
    LPVOID EntryPoint;
} MODULEINFO, *LPMODULEINFO;

bool fillModuleListPSAPI( ModuleList& modules, DWORD pid, HANDLE hProcess )
{
	// EnumProcessModules()
	typedef BOOL (__stdcall *tEPM)( HANDLE hProcess, HMODULE *lphModule, DWORD cb, LPDWORD lpcbNeeded );
	// GetModuleFileNameEx()
	typedef DWORD (__stdcall *tGMFNE)( HANDLE hProcess, HMODULE hModule, LPSTR lpFilename, DWORD nSize );
	// GetModuleBaseName() -- redundant, as GMFNE() has the same prototype, but who cares?
	typedef DWORD (__stdcall *tGMBN)( HANDLE hProcess, HMODULE hModule, LPSTR lpFilename, DWORD nSize );
	// GetModuleInformation()
	typedef BOOL (__stdcall *tGMI)( HANDLE hProcess, HMODULE hModule, LPMODULEINFO pmi, DWORD nSize );

	HINSTANCE hPsapi;
	tEPM pEPM;
	tGMFNE pGMFNE;
	tGMBN pGMBN;
	tGMI pGMI;

	DWORD i;
	ModuleEntry e;
	DWORD cbNeeded;
	MODULEINFO mi;
	HMODULE *hMods = 0;
	char *tt = 0;

	hPsapi = LoadLibrary( "psapi.dll" );
	if ( hPsapi == 0 )
		return false;

	modules.clear();

	pEPM = (tEPM) GetProcAddress( hPsapi, "EnumProcessModules" );
	pGMFNE = (tGMFNE) GetProcAddress( hPsapi, "GetModuleFileNameExA" );
	pGMBN = (tGMFNE) GetProcAddress( hPsapi, "GetModuleBaseNameA" );
	pGMI = (tGMI) GetProcAddress( hPsapi, "GetModuleInformation" );
	if ( pEPM == 0 || pGMFNE == 0 || pGMBN == 0 || pGMI == 0 )
	{
		FreeLibrary( hPsapi );
		return false;
	}

	hMods = new HMODULE[TTBUFLEN / sizeof(HMODULE)];
	tt = new char[TTBUFLEN];

	// check for errors and cleanup
	if ( ! pEPM( hProcess, hMods, TTBUFLEN, &cbNeeded ) )
	{
		goto cleanup;
	}

	if ( cbNeeded > TTBUFLEN )
	{
		goto cleanup;
	}

	for ( i = 0; i < cbNeeded / sizeof hMods[0]; ++ i )
	{
		// for each module, get:
		// base address, size
		pGMI( hProcess, hMods[i], &mi, sizeof mi );
		e.baseAddress = (DWORD) mi.lpBaseOfDll;
		e.size = mi.SizeOfImage;
		// image file name
		tt[0] = '\0';
		pGMFNE( hProcess, hMods[i], tt, TTBUFLEN );
		e.imageName = tt;
		// module name
		tt[0] = '\0';
		pGMBN( hProcess, hMods[i], tt, TTBUFLEN );
		e.moduleName = tt;

		modules.push_back( e );
	}

cleanup:
	if ( hPsapi )
		FreeLibrary( hPsapi );
	delete [] tt;
	delete [] hMods;

	return modules.size() != 0;
}

bool fillModuleList( ModuleList& modules, DWORD pid, HANDLE hProcess )
{
	// try toolhelp32 first
	if ( fillModuleListTH32( modules, pid ) )
		return true;
	// nope? try psapi, then
	return fillModuleListPSAPI( modules, pid, hProcess );
}

void enumAndLoadModuleSymbols( HANDLE hProcess, DWORD pid )
{
	ModuleList modules;
	ModuleListIter it;
	char *img, *mod;

	// fill in module list
	fillModuleList( modules, pid, hProcess );

	for ( it = modules.begin(); it != modules.end(); ++ it )
	{
		// unfortunately, SymLoadModule() wants writeable strings
		img = new char[(*it).imageName.size() + 1];
		strcpy( img, (*it).imageName.c_str() );
		mod = new char[(*it).moduleName.size() + 1];
		strcpy( mod, (*it).moduleName.c_str() );

		SymLoadModule( hProcess, 0, img, mod, (*it).baseAddress, (*it).size );

		delete [] img;
		delete [] mod;
	}
}

GCallStack::GCallStack(HANDLE hThread, CONTEXT& c)
{
	DWORD imageType = IMAGE_FILE_MACHINE_I386;
	HANDLE hProcess = GetCurrentProcess();

	int frameNum; // counts walked frames

#if defined(_LINUX64) || defined(_WIN64) || defined(_IOS)
	unsigned __int64 offsetFromSymbol; // tells us how far from the symbol we were
#else
	DWORD offsetFromSymbol; // tells us how far from the symbol we were
#endif



	DWORD symOptions; // symbol handler settings
	IMAGEHLP_SYMBOL *pSym = (IMAGEHLP_SYMBOL *) malloc( IMGSYMLEN + MAXNAMELEN );
	char undFullName[MAXNAMELEN]; // undecorated name with all shenanigans

	IMAGEHLP_MODULE Module;
	IMAGEHLP_LINE Line;
	std::string symSearchPath;
	char *tt = 0, *p;

	STACKFRAME s; // in/out stackframe
	memset( &s, '\0', sizeof s );

	tt = new char[TTBUFLEN];

	// build symbol search path from:
	symSearchPath = "";
	// current directory
	if ( GetCurrentDirectory( TTBUFLEN, tt ) )
		symSearchPath += tt + std::string( ";" );
	// dir with executable
	if ( GetModuleFileName( 0, tt, TTBUFLEN ) )
	{
		for ( p = tt + strlen( tt ) - 1; p >= tt; -- p )
		{
			// locate the rightmost path separator
			if ( *p == '\\' || *p == '/' || *p == ':' )
				break;
		}
		// if we found one, p is pointing at it; if not, tt only contains
		// an exe name (no path), and p points before its first byte
		if ( p != tt ) // path sep found?
		{
			if ( *p == ':' ) // we leave colons in place
				++ p;
			*p = '\0'; // eliminate the exe name and last path sep
			symSearchPath += tt + std::string( ";" );
		}
	}
	// environment variable _NT_SYMBOL_PATH
	if ( GetEnvironmentVariable( "_NT_SYMBOL_PATH", tt, TTBUFLEN ) )
		symSearchPath += tt + std::string( ";" );
	// environment variable _NT_ALTERNATE_SYMBOL_PATH
	if ( GetEnvironmentVariable( "_NT_ALTERNATE_SYMBOL_PATH", tt, TTBUFLEN ) )
		symSearchPath += tt + std::string( ";" );
	// environment variable SYSTEMROOT
	if ( GetEnvironmentVariable( "SYSTEMROOT", tt, TTBUFLEN ) )
		symSearchPath += tt + std::string( ";" );

	if ( symSearchPath.size() > 0 ) // if we added anything, we have a trailing semicolon
		symSearchPath = symSearchPath.substr( 0, symSearchPath.size() - 1 );

	// why oh why does SymInitialize() want a writeable string?
	strncpy( tt, symSearchPath.c_str(), TTBUFLEN );
	tt[TTBUFLEN - 1] = '\0'; // if strncpy() overruns, it doesn't add the null terminator

	// init symbol handler stuff (SymInitialize())
	if ( ! SymInitialize( hProcess, tt, false ) )
	{
		goto tagCleanUp;
	}

	symOptions = SymGetOptions();
	symOptions |= SYMOPT_LOAD_LINES;
	symOptions &= ~SYMOPT_UNDNAME;
	SymSetOptions( symOptions ); // SymSetOptions()

	enumAndLoadModuleSymbols( hProcess, GetCurrentProcessId() );

	// init STACKFRAME for first call
	// Notes: will have to be #ifdef-ed for Alphas; MIPSes are dead anyway,
#ifndef _WIN64
	s.AddrPC.Offset = c.Eip;
	s.AddrPC.Mode = AddrModeFlat;
	s.AddrFrame.Offset = c.Ebp;
	s.AddrFrame.Mode = AddrModeFlat;
#endif

	memset( pSym, '\0', IMGSYMLEN + MAXNAMELEN );
	pSym->SizeOfStruct = IMGSYMLEN;
	pSym->MaxNameLength = MAXNAMELEN;

	memset( &Line, '\0', sizeof Line );
	Line.SizeOfStruct = sizeof Line;

	memset( &Module, '\0', sizeof Module );
	Module.SizeOfStruct = sizeof Module;

	offsetFromSymbol = 0;

	for ( frameNum = 0; ;)
	{
		// get next stack frame (StackWalk(), SymFunctionTableAccess(), SymGetModuleBase())
		// if this returns ERROR_INVALID_ADDRESS (487) or ERROR_NOACCESS (998), you can
		// assume that either you are done, or that the stack is so hosed that the next
		// deeper frame could not be found.
		if ( ! StackWalk( imageType, hProcess, hThread, &s, &c, NULL,
			SymFunctionTableAccess, SymGetModuleBase, NULL ) )
			break;

		if ( s.AddrPC.Offset != 0 )
		{ // we seem to have a valid PC
			// show procedure info (SymGetSymFromAddr())
			if ( ! SymGetSymFromAddr( hProcess, s.AddrPC.Offset, &offsetFromSymbol, pSym ) )
			{
				break;
			}
			else
			{
				// UnDecorateSymbolName()
				UnDecorateSymbolName( pSym->Name, undFullName, MAXNAMELEN, UNDNAME_COMPLETE );
				if (!frameNum && strstr(undFullName, "Exception"))
					continue;
				_stk += undFullName;
			}
		} // we seem to have a valid PC

		// no return address means no deeper stackframe
		if ( s.AddrReturn.Offset == 0 )
		{
			// avoid misunderstandings in the printf() following the loop
			SetLastError( 0 );
			break;
		}
		++frameNum;
	}

	// de-init symbol handler etc. (SymCleanup())
	SymCleanup( hProcess );
	free( pSym );
tagCleanUp:;	
	delete [] tt;
}

GCallStack::~GCallStack()
{
}

const GStringList *GCallStack::GetStack()
{
	return &_stk;
}

const char *GCallStack::GetStackAsString() 
{
	_strStk.Empty();
	GStringIterator it(&_stk);
	
	while (it())
	{
		if (!_strStk.IsEmpty())
			_strStk += "\n";
		_strStk += it++;
	}

	return (const char *)_strStk;
}

#endif
#endif

GException::GException(int nError, int nSubSystem, const char *pzText, GStringList *pStack)
{
	_error = nError;
	_subSystem = nSubSystem;
	_cause = 0;
	this->Format("%s",pzText);

	if (pStack)
	{
		GStringIterator it(pStack);
		while (it())
			_stk.AddLast(it++);
	}
}

void GException::AddErrorDetail(int nError, const char *pzErrorText)
{
	GString strTemp;
	strTemp.Format("[Error %ld] %s\n",(int)nError,pzErrorText);
	_ErrorDetail.AddLast(strTemp);
}


void GException::AddErrorDetail(const char* szSystem, int error, ...)
{
	int nSubSystem = 0;
	int nError = error;
	try
	{

		GProfile &ErrorProfile = GetErrorProfile();
		nSubSystem = atoi(ErrorProfile.GetString(szSystem, "SubSystem"));

		GString strKey;
		strKey.Format("%ld", (int)error);
		strKey = ErrorProfile.GetString(szSystem, (const char *)strKey);

		GString strTemp;
		va_list argList;
		va_start(argList, error);
		strTemp.FormatV((const char *)strKey, argList);
		va_end(argList);


		GString strAddedToUserStack;
		strAddedToUserStack.Format("[Error %ld:%ld] %s\n", (int)nSubSystem, (int)nError, (const char *)strTemp);
		_ErrorDetail.AddLast(strAddedToUserStack);

	}
	catch (GException &e)
	{
		_subSystem = e._subSystem;
		_error = e._error;

		Format("%s", (const char *)e);
		return;
	}
}

GException::GException(const char* szSystem, int error, ...)
{
	_subSystem = 0;
	_error = error;

	try
	{
		GProfile &ErrorProfile = GetErrorProfile();
		if (ErrorProfile.DoesExist(szSystem, "SubSystem"))
		{
			_subSystem = atoi(ErrorProfile.GetStringOrDefault(szSystem, "SubSystem","0"));
		}
		else
		{
			_subSystem = 0;
		}

		if (_subSystem == 0) // string resources not loaded
		{
			_subSystem = 0;
			_error = error;
			Format("%s", "String resource descriptions not loaded");
			return;
		}

		GString strKey;
		strKey.Format("%ld", (int)error);
		if (ErrorProfile.DoesExist(szSystem, (const char *)strKey))
		{
			strKey = ErrorProfile.GetStringOrDefault(szSystem, (const char *)strKey, "String Resource Not Found");
		}
		else
		{
			strKey = "String Resource Not Found";
		}

		va_list argList;
		va_start(argList, error);
		FormatV((const char *)strKey, argList);
		va_end(argList);
	}
	catch (GException &e)
	{
		_subSystem = e._subSystem;
		_error = e._error;

		Format("%s", (const char *)e);
		return;
	}

#if defined GCallStack && _WIN32 // record the callstack.
	_se_translator_function	f;
	f = _set_se_translator(_stack_se_translator);
	try
	{
		int div = 0;
		#ifdef _MyOwnSlash // breakin the law.
		int crash = 1\div;
		#else
		int crash = 1/div;
		#endif
	}
	catch (GCallStack &gcs)
	{
		_stk = *gcs.GetStack();
	}
	_set_se_translator(f);
#endif
}

GException::GException(const GException &cpy)
{
	_subSystem = cpy._subSystem;
	_error = cpy._error;
	_ErrorDetail = cpy._ErrorDetail;
	Format("%s", (const char *)cpy);
}

GException::GException(int error, const char *str)
{
	_subSystem = 0;
	_error = error;
	Format("%s", str);
}

GException::GException()
{
	_subSystem = 0;
}

GException::~GException()
{
}

const char *GException::GetDescription() 
{
	// Get the user stack into strUserStack
	GString strUserStack;
	GStringIterator it(&_ErrorDetail);
	while (it())
	{
		strUserStack += it++;
		if (it())
			strUserStack += "\n";
	}

	_ret.Format("[Error %ld:%ld] %s\n%s", (int)_subSystem, (int)_error, (const char *)*this, (const char *)strUserStack);
	return (const char *)_ret; 
}


const char *GException::ToXML(const char *pzExceptionParent/* = "TransactResultSet"*/) 
{
	_ret.Empty();

	// Get the user stack into strUserStack
	GString strUserStack;
	GStringIterator it(&_ErrorDetail);
	while (it())
	{
		strUserStack += "\t\t<UserStack>";
		strUserStack.AppendEscapeXMLReserved(it++);
		strUserStack += "</UserStack>";
		if (it())
			strUserStack += "\n";
	}


	GString strDescription = (const char *)*this;
	strDescription.EscapeXMLReserved();
	

	_ret.Format("<%s>\n\t<Exception>\n\t\t<Description>%s</Description>\n\t\t"
				"<ErrorNumber>%ld</ErrorNumber>\n\t\t<SubSystem>%d</SubSystem>\n%s\t</Exception></%s>",
				pzExceptionParent,
				(const char *)strDescription,
				(int)_error,
				(int)_subSystem,
				(const char *)strUserStack,
				pzExceptionParent);


	return (const char *)_ret; 
}

long GException::GetCause()
{ 
	_cause = _subSystem;
	_cause <<= 4;
	_cause |= _error;

	return _cause;
}

const GStringList *GException::GetStack()
{
	return &_stk;
}

const char *GException::GetStackAsString()
{
	_ret.Empty();

	if (_stk.isEmpty())
	{
		_ret = "Call stack unavailable.";
	}
	else
	{
		GStringIterator it(&_stk);
		
		while (it())
		{
			if (!_ret.IsEmpty())
				_ret += "\n";
			_ret += it++;
		}
	}

	return (const char *)_ret;
}
