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

#ifndef _DYNAMIC_LIB_H_
#define _DYNAMIC_LIB_H_

#ifdef _WIN32
	#include <windows.h>
	HINSTANCE DL_LoadLibrary(const char *a);
	int DL_FreeLibrary(HMODULE a);
	FARPROC DL_GetProcAddress( HINSTANCE a, const char *b);
	#define _OPENLIB(a) DL_LoadLibrary(a);	
	#define _CLOSELIB(a) DL_FreeLibrary((HMODULE)a)
	#define _PROCADDRESS(a, b) DL_GetProcAddress((HINSTANCE)a, b);
#elif _HPUX
	void *dlopenhp(const char *fname, int mode);
	#define _OPENLIB(a) dlopenhp(a, RTLD_LAZY)
	#define _CLOSELIB(a) dlclose(a)
	#define _PROCADDRESS(a, b) dlsym(a, b);
#elif _LINUX
	#include <dlfcn.h>
	#define _OPENLIB(a) dlopen(a, RTLD_LAZY)
	#define _CLOSELIB(a) dlclose(a)			
	#define _PROCADDRESS(a, b) dlsym(a, b);
#elif __sun
	#include <dlfcn.h>
	#define _OPENLIB(a) dlopen(a, RTLD_LAZY)
	#define _CLOSELIB(a) dlclose(a)			
	#define _PROCADDRESS(a, b) dlsym(a, b);
#elif _IOS
	#include <dlfcn.h>
	#define _OPENLIB(a) dlopen(a, RTLD_LAZY)
	#define _CLOSELIB(a) dlclose(a)			
	#define _PROCADDRESS(a, b) dlsym(a, b);
#endif

class GString;
void GetLoadFailureReason(GString *pReason);

#endif //_DYNAMIC_LIB_H_