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
#ifdef _WIN32
	#define MSFT_EXPORT __declspec(dllexport)
#else
	#define MSFT_EXPORT
#endif
#include "IntegrationJava.h"


extern "C" MSFT_EXPORT char *GetDriverName(void)
{
	static char pzInterface[] = "Java";
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
extern "C" MSFT_EXPORT char *DriverOperation()
{
	static char pzDriverOperation[] = "31";
	return pzDriverOperation;
}



// pzComponentPath = value from settings.xml. "CStdCallPath" entry for "CStdCall" driver.  
// pzComponentSettings = value from settings.xml. "CStdCallSettings" entry for "CStdCall" driver.  
extern "C" MSFT_EXPORT void *Construct(const char *pzComponentPath, const char *pzComponentSettings)
{
	// create an object and set some state in it.
	return new JavaInterface(pzComponentPath,pzComponentSettings);
}

extern "C" MSFT_EXPORT const char *GetComponents(void *pInst)
{
	JavaInterface *pI = (JavaInterface *)pInst;
	GStringList *pList = pI->GetComponents();
	return pList->Serialize("&&");
}

extern "C" MSFT_EXPORT const char *GetInterfaces(void *pInst, const char *pzComponent)
{
	JavaInterface *pI = (JavaInterface *)pInst;
	GStringList *pList = pI->GetInterfaces(pzComponent);
	return pList->Serialize("&&");
}

extern "C" MSFT_EXPORT const char *GetMethods(void *pInst, const char *pzComponent, const char *pzInterface)
{
	JavaInterface *pI = (JavaInterface *)pInst;
	GStringList *pList = pI->GetMethods(pzComponent,pzInterface);
	return pList->Serialize("&&");
}

extern "C" MSFT_EXPORT const char *GetMethodParams(void *pInst,
													const char *pzComponent, 
													const char *pzInterface, 
													const char *pzMethodName)
{
	JavaInterface *pI = (JavaInterface *)pInst;
	GStringList *pList = pI->GetMethodParams(pzComponent, pzInterface, pzMethodName);
	pI->m_strMethodParamsSuperSet = pList->Serialize("&&");		// param names list
	pI->m_strMethodParamsSuperSet += "&&&&";						// seperator
	pList = pI->GetMethodParamTypes();				
	pI->m_strMethodParamsSuperSet += pList->Serialize("&&");		// param types list
	pI->m_strMethodParamsSuperSet += "&&&&";						// seperator
	pI->m_strMethodParamsSuperSet += pI->GetMethodReturnType();	// return type

	return pI->m_strMethodParamsSuperSet;
}


extern "C" MSFT_EXPORT int IsAvailable(void *pInst, 
									   const char *pzComponent, 
									   const char *pzInterface, 
									   const char *pzMethodName)
{
	JavaInterface *pI = (JavaInterface *)pInst;
	return pI->IsAvailable(pzComponent,pzInterface,pzMethodName);
}

extern "C" MSFT_EXPORT const char *Invoke(void *pInst,
										   const char *pzComponent, 
										   const char *pzInterface, 
										   const char *pzMethodName, 
										   const char *pzArgs 
										   )
{
	JavaInterface *pI = (JavaInterface *)pInst;
	GStringList list("&&",pzArgs);
	GStringIterator it(&list);
	return pI->Invoke(pzComponent,pzInterface,pzMethodName,it);
}

extern "C" MSFT_EXPORT void Destruct(void *pInst)
{
	JavaInterface *pI = (JavaInterface *)pInst;
	delete pI;
}

