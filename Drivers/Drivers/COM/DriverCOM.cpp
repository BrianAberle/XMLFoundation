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

// DriverCOM.cpp : Defines the entry point for the DLL application.
//
// NOTE: this project does NOT link the XMLFoundation - but it directly compiles several
// XMLFoundation utilities. It simplifies driver builds that can get complex when linking 
// things like the Perl runtime or PHP interpreter due to threading models.  Since the base
// driver template does not link to anything it makes it easier to link to anything during
// driver development.

#include "stdafx.h"
#include "DriverCOM.h"
#include "IntegrationCOM.h"

/*
BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved
					 )
{
    switch (ul_reason_for_call)
	{
		case DLL_PROCESS_ATTACH:
		case DLL_THREAD_ATTACH:
		case DLL_THREAD_DETACH:
		case DLL_PROCESS_DETACH:
			break;
    }
    return TRUE;
}
*/
extern "C" DRIVERCOM_API char *GetDriverName(void)
{
	static char pzInterface[] = "COM";
	return pzInterface;
}


//--------------------------------------------
// BYTE 1	//language driver operation modes:
//--------------------------------------------
// 1 Single Threaded- 1 instance. TXML Server Blocking. 
//					  No two methods will ever be simultaneous executed.  
//					  TXML Server threads wait in line for the single instance of this object.
// 2 Multi Threaded	- 1 instance. All server threads into a single instance. 
//					  Handle you own thread safety issues.
// 3 Multi Object	- Multi instance - Object pool. (DEFAULT)
//					  Interface Objects are accessed by any thread as necessary,
//					  A single instance will only be used by a single thread at any moment.
//					  this relieves the driver implementation of thread blocking routines.
// 4 As necessary	- Multi instance. Created and destroyed with each use. No caching.
//
//--------------------------------------------
// BYTE 2	DLL or .SO Unload Behavior :
//--------------------------------------------
// 1 Never unload the driver.
// 0 Unload the driver after each call to Destruct()
extern "C" DRIVERCOM_API char *DriverOperation()
{
	static char pzDriverOperation[] = "31";
	return pzDriverOperation;
}




// pzComponentPath = value from settings.xml. "CStdCallPath" entry for "CStdCall" driver.  
// pzComponentSettings = value from settings.xml. "CStdCallSettings" entry for "CStdCall" driver.  
extern "C" DRIVERCOM_API void *Construct(const char *pzComponentPath, const char *pzComponentSettings)
{
	// create an object and set some state in it.
	return new COMInterface(pzComponentPath,pzComponentSettings);
}

extern "C" DRIVERCOM_API const char *GetComponents(void *pInst)
{
	COMInterface *pI = (COMInterface *)pInst;
	GStringList *pList = pI->GetComponents();
	return pList->Serialize("&");
}

extern "C" DRIVERCOM_API const char *GetInterfaces(void *pInst, const char *pzComponent)
{
	COMInterface *pI = (COMInterface *)pInst;
	GStringList *pList = pI->GetInterfaces(pzComponent);
	return pList->Serialize("&");
}

extern "C" DRIVERCOM_API const char *GetMethods(void *pInst, const char *pzComponent, const char *pzInterface)
{
	COMInterface *pI = (COMInterface *)pInst;
	GStringList *pList = pI->GetMethods(pzComponent,pzInterface);
	return pList->Serialize("&");
}

extern "C" DRIVERCOM_API const char *GetMethodParams(void *pInst,
													const char *pzComponent, 
													const char *pzInterface, 
													const char *pzMethodName)
{
	COMInterface *pI = (COMInterface *)pInst;
	GStringList *pList = pI->GetMethodParams(pzComponent, pzInterface, pzMethodName);
	pI->m_strMethodParamsSuperSet = pList->Serialize("&");		// param names list
	pI->m_strMethodParamsSuperSet += "!";						// seperator
	pList = pI->GetMethodParamTypes();				
	pI->m_strMethodParamsSuperSet += pList->Serialize("&");		// param types list
	pI->m_strMethodParamsSuperSet += "!";						// seperator
	pI->m_strMethodParamsSuperSet += pI->GetMethodReturnType();	// return type

	return pI->m_strMethodParamsSuperSet;
}


extern "C" DRIVERCOM_API int IsAvailable(void *pInst, 
									   const char *pzComponent, 
									   const char *pzInterface, 
									   const char *pzMethodName)
{
	COMInterface *pI = (COMInterface *)pInst;
	return pI->IsAvailable(pzComponent,pzInterface,pzMethodName);
}


extern "C" DRIVERCOM_API const char *Invoke(void *pInst,
										   const char *pzComponent, 
										   const char *pzInterface, 
										   const char *pzMethodName, 
										   const char *pzArgs 
										   )
{
	COMInterface *pI = (COMInterface *)pInst;
	GStringList list("&",pzArgs);
	GStringIterator it(&list);
	return pI->Invoke(pzComponent,pzInterface,pzMethodName,it);
}

extern "C" DRIVERCOM_API void Destruct(void *pInst)
{
	COMInterface *pI = (COMInterface *)pInst;
	delete pI;
}

