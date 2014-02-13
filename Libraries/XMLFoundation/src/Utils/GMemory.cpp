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
#include "GString.h"

#include <stdlib.h>

#ifndef _IOS
#include <malloc.h>
#endif

// You must #define new to NEWER_NEW in GlobalInclude.h for this code to be called
//
// char chUniqueOverload is not used, it's purpose is so that this memory overload does not 
// cause link conflicts with other memory tools like that from MicroQuill or the overloaded new
// within MFC.
//
// This may be used to find memory leaks by entering every alloc in a list or hash then removing it when it is free'd
// anything left over is technically a memory leak, however not all leaks are bad.  Bad leaks grow
// during the course of the application.  Good leaks are one time allocations that never grow and they are never
// released when the application shuts down because there is no real purpose since the process is ending and the process
// memory will be released by the operating system.
//
//
// This was added to find performance errors, such as adding 1Meg into a GString that was never pre-allocated.
// Such an error will cause the GString to alloc many times over and over until it has grown to the needed size.
// Calling GString::PreAlloc(1024000) will be much faster, and this handy overload will help you find such errors.


#ifdef _WIN64
void * operator new( unsigned __int64 n, const char *pzFile, int nLine, char chUniqueOverload )
#elif defined(_IOS)
void * operator new( size_t n, const char *pzFile, int nLine, char chUniqueOverload )
#else
void * operator new( unsigned int n, const char *pzFile, int nLine, char chUniqueOverload )
#endif
{
	void *p = malloc(n);
/*	
	GString strLeakDebug;
	strLeakDebug.Format("Alloc % 10d bytes [",n);
	strLeakDebug << pzFile << " line " << nLine << "]:" << (unsigned long)p << "\r\n";
	strLeakDebug.ToFileAppend("d:\\mem.txt");
*/
	return p;
}

void operator delete( void *p, const char *pzFile, int nLine, char chUniqueOverload )
{
/*
	GString strLeakDebug("Free  ");
	strLeakDebug << "[" << pzFile << " line " << pzFile << "]" << (unsigned long)p << "\r\n";
	strLeakDebug.ToFileAppend("d:\\mem.txt");
*/
	free(p);
}
