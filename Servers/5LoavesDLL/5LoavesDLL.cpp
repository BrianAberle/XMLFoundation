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


//  NOTE:   Because a DLL is actually a library and not an application it is required
//          that the initialization be done manually.  It must be managed by the dymanic library.
//			The solution is to BUILD the XMLFoundation with _NO_SELF_INIT defined, then manage 
//			the init when this library(the DLL or SO) is loaded.
//			
//			The manual init is done when the DLL loads, under  case DLL_PROCESS_ATTACH
//			pthread_win32_process_attach_np ();
//
//			This was a design decision made based on the fact that the majority of applications
//			are not Dynamic Link Libraries.
//			
//			1. Add _NO_SELF_INIT to the "preprocessor" definitions in the XMLFoundation Project
//			2. Rebuild the XMLFoundation and link with that library
//			3. Rebuild this .dll to link with the new XMLFoundation
//			4. remove the _NO_SELF_INIT directive and rebuild the XMLFoundation, or every other app thet links with it will crash.



#include "stdafx.h"
#include "5LoavesDLL.h"
#include "GProfile.h"
#include "GException.h"
#include "GThread.h"

#include <winsock.h>
#include <fcntl.h>
#include <stdlib.h>

// The "pascal" calling convention is out dated, 
// this allows the DLL to be built as 16/32/64 bit.



GString g_strLastError;
BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved
					 )
{
    switch (ul_reason_for_call)
	{
		case DLL_PROCESS_ATTACH:
//			pthread_win32_process_attach_np ();
			break;
		case DLL_THREAD_ATTACH:
			break;
		case DLL_THREAD_DETACH:
//			pthread_win32_thread_detach_np ();
			break;
		case DLL_PROCESS_DETACH:
			break;
    }
    return TRUE;
}
int isRunning=0;






void server_stop();
long pascal ServerStop(void)
{
	try
	{
		server_stop();
	}
	catch ( GException &e)
	{
		g_strLastError = e.GetDescription();
		return 0;
	}
	catch(...)
	{
		g_strLastError = "Unknown shutdown error";
		return 0;
	}
	isRunning = 0;
	return 1;
}

// call this before calling ServerStart() if you want to allow
// the user of your application to change the runtime settings.
// or call SetConfigEntry() to set them manually.
long pascal SetConfigFile(const char *pzStartConfigFile)
{
	try
	{
		SetProfile(new GProfile(pzStartConfigFile,0));
	}
	catch ( GException &e )
	{
		g_strLastError = e.GetDescription();
		return 0;
	}
	catch(...)
	{
		g_strLastError = "Unknown error";
		return 0;
	}
	return 1;
}

long pascal SetConfigEntry(const char *pzSection, const char *pzEntry, const char *pzValue)
{
	try
	{
		GetProfile().SetConfig(pzSection, pzEntry, pzValue);
	}
	catch ( GException &e )
	{
		g_strLastError = e.GetDescription();
		return 0;
	}
	catch(...)
	{
		g_strLastError = "Unknown shutdown error";
		return 0;
	}
	return 1;
}


int server_start(const char *);
long pascal ServerStart()
{
	if (!isRunning)
	{
		try
		{
			char pzName[512];
			memset(pzName,0,512);
			GetModuleFileName(NULL,pzName,512);

			GString strStartupMessage;
			strStartupMessage << "5Loaves Windows DLL [" << pzName << "]";


			int nRet = server_start(strStartupMessage);
			if (nRet)
				isRunning = 1;
			return 1;
		}
		catch ( GException &e)
		{
			if (e.GetError() == 2)		
			{
				g_strLastError = "Startup Password required";
			}
			else
			{
				g_strLastError = e.GetDescription();
			}
			return 0;
		}
		catch ( ... )
		{
			g_strLastError = "Unknown startup error";
			return 0;
		}
	}
	return 1;
}

const char * pascal GetLast5LoavesError()
{
	return g_strLastError;
}



