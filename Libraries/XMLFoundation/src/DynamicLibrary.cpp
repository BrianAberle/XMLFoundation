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

#include "DynamicLibrary.h"
#include "GString.h"

#ifdef _WIN32
#ifdef _UNICODE

#include "windows.h"
HINSTANCE DL_LoadLibrary(const char *a)
{
	char buf[1024]; // imposes a 512 byte MAX_PATH
	return LoadLibrary( DLAsciiToUnicode(a, buf) );
}
int DL_FreeLibrary(HMODULE a)
{
	return FreeLibrary(a);
}
FARPROC DL_GetProcAddress( HINSTANCE a, const char *b)
{
	char buf[1024]; // imposes a 512 byte MAX_PATH
	return GetProcAddress(a, DLAsciiToUnicode(b, buf));
}


#else

HINSTANCE DL_LoadLibrary(const char *a)
{
#ifdef __WINPHONE
//	int mbstowcs(wchar_t *out, const char *in, size_t size);
	const size_t WCHARBUF = 256;
	wchar_t  wszDest[WCHARBUF];
	MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, a, -1, wszDest, WCHARBUF);
	return LoadPackagedLibrary(wszDest,0);
#else
	return LoadLibrary(a);
#endif
}
int DL_FreeLibrary(HMODULE a)
{
	return FreeLibrary(a);
}
FARPROC DL_GetProcAddress( HINSTANCE a, const char *b)
{
	return GetProcAddress(a, b);
}

#endif
#endif




void GetLoadFailureReason(GString *pReason)
{
#ifdef __WINPHONE
	DWORD dwError = GetLastError();
	char pzMsgBuf[256];
	FormatMessage( 
					FORMAT_MESSAGE_FROM_SYSTEM | 
					FORMAT_MESSAGE_IGNORE_INSERTS,
					0,
					dwError,
					MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
					pzMsgBuf,
					0,
					NULL 
				);

		*pReason = pzMsgBuf;
		return;
#else 
	#ifdef _WIN32
		DWORD dwError = GetLastError();
		LPVOID lpMsgBuf;
		FormatMessage( 
						FORMAT_MESSAGE_ALLOCATE_BUFFER | 
						FORMAT_MESSAGE_FROM_SYSTEM | 
						FORMAT_MESSAGE_IGNORE_INSERTS,
						0,
						dwError,
						MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
						(LPTSTR) &lpMsgBuf,
						0,
						NULL 
					);

		if (lpMsgBuf)
		{
			*pReason = (const char *)lpMsgBuf;
			LocalFree( lpMsgBuf );
		}
	#else
		*pReason = dlerror();
		return;
	#endif
#endif

}
