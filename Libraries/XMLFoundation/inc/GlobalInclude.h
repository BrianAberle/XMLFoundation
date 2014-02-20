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
//
// Common Global Include 
//
#pragma once
#pragma warning (disable:4996)
#pragma warning (disable:4267)
#pragma warning (disable:4244)
#pragma warning (disable:4100)	// Unreferenced formal parameter
#pragma warning (disable:4702)	// Unreachable Code
#pragma warning (disable:4127)	// conditional expression is constant




#ifndef new

	#define NEWER_NEW new(SOURCE_FILE, __LINE__, 'X')

	#ifdef _WIN64
		void * operator new( unsigned __int64 n, const char *pzFile, int nLine, char chUniqueOverload);
	#elif _LINUX64
		#include <stddef.h>
		void * operator new( size_t n, const char *pzFile, int nLine, char chUniqueOverload);
	#elif _IOS
		#include <string>
		void * operator new( size_t n, const char *pzFile, int nLine, char chUniqueOverload);
	#else
		void * operator new( unsigned int n, const char *pzFile, int nLine, char chUniqueOverload);
	#endif

	void operator delete( void* p, const char *pzFile, int nLine, char chUniqueOverload );

#endif

// Uncomment this for the overloaded new/delete in memory.cpp to be called.
// After you uncomment it, you need to rebuild XMLFoundation, 
// AND your application AND any other libs that you use like Xfer.lib or XMLServer.lib
//
// #define new NEWER_NEW





