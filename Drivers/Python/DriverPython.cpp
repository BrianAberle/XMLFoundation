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

// This file contains the Language Driver Interface entry points.
// Language drivers have a simple interface requiring only 10
// entry points to be implemented:
//
//					Construct()
//					GetDriverName()
//					DriverOperation()
//					IsAvailable() 
//					GetComponents()
//					GetInterfaces()
//					GetMethods()
//					GetMethodParams()
//					Invoke()
//					Destruct()
//
//

#ifdef _WIN32
	// Microsoft builds
	#define MSFT_EXPORT __declspec(dllexport)
#else
	// UNIX builds
	#define MSFT_EXPORT
#endif

#include "IntegrationPython.h"



// Return the Driver name as it will appear in DesignerXML
// Also used to index the [Python] section in txml.txt
extern "C" MSFT_EXPORT char *GetDriverName(void)
{
	static char pzInterface[] = "Python";
	return pzInterface;
}


//--------------------------------------------
// BYTE 1	//language driver operation modes:
//--------------------------------------------
// 1 Single Threaded- Single instance. TXML Server Blocking. 
//					  No two methods will ever be simultaneous executed.  
//					  TXML Server threads wait in line for the single instance of this object.
// 2 Multi Threaded	- Single instance. All server threads into a single instance. 
//					  Handle your own thread safety issues.
// 3 Multi Object	- Multi instance - Object pool.
//					  Each instance will only be used by a single thread at any moment,
//					  while multiple instances prevent the server from blocking.
//					  This relieves the driver implementation of thread blocking routines.
// 4 As necessary	- Multi instance. Created and destroyed with each use. No caching.
//
//--------------------------------------------
// BYTE 2	DLL or .SO Unload Behavior :
//--------------------------------------------
// 1 Never unload the driver (faster).
// 0 Unload the driver after each call to Destruct()
//--------------------------------------------
// BYTE 3	ICU char width of param values (available in select builds):
//--------------------------------------------
// 0 8 bit chars.
// 1 16 bit chars
extern "C" MSFT_EXPORT char *DriverOperation()
{
	static char pzDriverOperation[] = "310";
	return pzDriverOperation;
}



// pzComponentPath = value from txml.txt. "Path" entry for [Python] section.
// pzComponentSettings = value from txml.txt. "Settings" entry for [Python] section.  
extern "C" MSFT_EXPORT void *Construct(const char *pzComponentPath, const char *pzComponentSettings)
{
	return new PythonInterface(pzComponentPath,pzComponentSettings);
}

// Check to see if a specified method is available in the requested component
extern "C" MSFT_EXPORT int IsAvailable(void *pInst, 
									   const char *pzComponent, 
									   const char *pzInterface, 
									   const char *pzMethodName)
{
	PythonInterface *pI = (PythonInterface *)pInst;
	return pI->IsAvailable(pzComponent,pzInterface,pzMethodName);
}

// In the case of Python, return a list of .pl files
extern "C" MSFT_EXPORT const char *GetComponents(void *pInst)
{
	PythonInterface *pI = (PythonInterface *)pInst;
	GStringList *pList = pI->GetComponents();
	return pList->Serialize("&"); //looks like this "1.pl&2.pl&3.pl"
}

// In the case of Python, return a list of .pl files
extern "C" MSFT_EXPORT const char *GetInterfaces(void *pInst, const char *pzComponent)
{
	PythonInterface *pI = (PythonInterface *)pInst;
	GStringList *pList = pI->GetInterfaces(pzComponent);
	return pList->Serialize("&");//looks like this "1&2&3"
}

// In the case of Python, return the methods specified in "ExposedMethods"
extern "C" MSFT_EXPORT const char *GetMethods(void *pInst, const char *pzComponent, const char *pzInterface)
{
	PythonInterface *pI = (PythonInterface *)pInst;
	GStringList *pList = pI->GetMethods(pzComponent,pzInterface);
	return pList->Serialize("&");
}

// In the case of Python, return the details specified in "ExposedMethods"
extern "C" MSFT_EXPORT const char *GetMethodParams(void *pInst,
													const char *pzComponent, 
													const char *pzInterface, 
													const char *pzMethodName)
{
	PythonInterface *pI = (PythonInterface *)pInst;
	GStringList *pList = pI->GetMethodParams(pzComponent, pzInterface, pzMethodName);
	pI->m_strMethodParamsSuperSet = pList->Serialize("&");		// param names list
	pI->m_strMethodParamsSuperSet += "!";						// seperator
	pList = pI->GetMethodParamTypes();				
	pI->m_strMethodParamsSuperSet += pList->Serialize("&");		// param types list
	pI->m_strMethodParamsSuperSet += "!";						// seperator
	pI->m_strMethodParamsSuperSet += pI->GetMethodReturnType();	// return type
	return pI->m_strMethodParamsSuperSet;
}


extern "C" MSFT_EXPORT const char *Invoke(void *pInst,
										   const char *pzComponent, 
										   const char *pzInterface, 
										   const char *pzMethodName, 
										   const char *pzArgs 
										   )
{
	PythonInterface *pI = (PythonInterface *)pInst;
	GStringList list("&",pzArgs);
	GStringIterator it(&list);
	return pI->Invoke(pzComponent,pzInterface,pzMethodName,it);
}

extern "C" MSFT_EXPORT void Destruct(void *pInst)
{
	PythonInterface *pI = (PythonInterface *)pInst;
	delete pI;
}


extern "C" MSFT_EXPORT int SetOption(void *pInst,const char *pzOption, void *pzValue)
{
	// set any global driver operation settings here
	PythonInterface *pI = (PythonInterface *)pInst;
	return pI->SetOption(pzOption, pzValue);
}


extern "C" MSFT_EXPORT void *GetOption(void *pInst,const char *pzOption)
{
	PythonInterface *pI = (PythonInterface *)pInst;
	return pI->GetOption(pzOption);
}

