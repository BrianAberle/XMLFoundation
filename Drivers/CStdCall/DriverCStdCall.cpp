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
#include "IntegrationCStdCall.h"
#include "GException.h"

#include <stdlib.h> // for atoi()


extern "C" MSFT_EXPORT char *GetDriverName(void)
{
	static char pzInterface[] = "CStdCall";
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
static int g_bErrorDesc = 0;
extern "C" MSFT_EXPORT void *Construct(const char *pzComponentPath, const char *pzComponentSettings)
{
	if (!g_bErrorDesc)
	{
		g_bErrorDesc = 1;	
	}
	// create an object and set some state in it.
	return new CStdCallInterface(pzComponentPath,pzComponentSettings);
}

extern "C" MSFT_EXPORT const char *GetComponents(void *pInst)
{
	CStdCallInterface *pSCI = (CStdCallInterface *)pInst;
	GStringList *pList = pSCI->GetComponents();
	return pList->Serialize("&&");
}

extern "C" MSFT_EXPORT const char *GetInterfaces(void *pInst, const char *pzComponent)
{
	CStdCallInterface *pSCI = (CStdCallInterface *)pInst;
	GStringList *pList = pSCI->GetInterfaces(pzComponent);
	return pList->Serialize("&");
}

extern "C" MSFT_EXPORT const char *GetMethods(void *pInst, const char *pzComponent, const char *pzInterface)
{
	CStdCallInterface *pSCI = (CStdCallInterface *)pInst;
	GStringList *pList = pSCI->GetMethods(pzComponent,pzInterface);
	return pList->Serialize("&");
}

extern "C" MSFT_EXPORT const char *GetMethodParams(void *pInst,
													const char *pzComponent, 
													const char *pzInterface, 
													const char *pzMethodName)
{
	CStdCallInterface *pSCI = (CStdCallInterface *)pInst;
	GStringList *pList = pSCI->GetMethodParams(pzComponent, pzInterface, pzMethodName);
	if (pList->Size())
	{
		pSCI->m_strMethodParamsSuperSet = pList->Serialize("&");		// param names list
		pSCI->m_strMethodParamsSuperSet += "!";						// seperator
		pList = pSCI->GetMethodParamTypes();				
		pSCI->m_strMethodParamsSuperSet += pList->Serialize("&");		// param types list
		pSCI->m_strMethodParamsSuperSet += "!";						// seperator
		pSCI->m_strMethodParamsSuperSet += pSCI->GetMethodReturnType();	// return type
	}
	else
	{
		return 0;
	}
	return pSCI->m_strMethodParamsSuperSet;
}

extern "C" MSFT_EXPORT int IsAvailable(void *pInst, 
									   const char *pzComponent, 
									   const char *pzInterface, 
									   const char *pzMethodName)
{
	CStdCallInterface *pSCI = (CStdCallInterface *)pInst;
	return pSCI->IsAvailable(pzComponent,pzInterface,pzMethodName);
}

extern "C" MSFT_EXPORT int SetOption(void *pInst,const char *pzOption, void *pzValue)
{
	// set any global driver operation settings here
	CStdCallInterface *pSCI = (CStdCallInterface *)pInst;
	return pSCI->SetOption(pzOption, pzValue);
}


extern "C" MSFT_EXPORT void *GetOption(void *pInst,const char *pzOption)
{
	CStdCallInterface *pSCI = (CStdCallInterface *)pInst;
	return pSCI->GetOption(pzOption);
}


extern "C" MSFT_EXPORT const char *Invoke(void *pInst,
										   const char *pzComponent, 
										   const char *pzInterface, 
										   const char *pzMethodName, 
										   const char *pzArgs 
										  )
{
	CStdCallInterface *pSCI = (CStdCallInterface *)pInst;
	
	if (pSCI->m_DataMap.IsEmpty())
	{
		GStringList list(pSCI->m_Delimit,pzArgs);
		GStringIterator it(&list);
		return pSCI->Invoke(pzComponent,pzInterface,pzMethodName,it);
	}
	else
	{
		// datamap is size|size|size for a 3 argument pzArgs
		GStringList listDataMap("|",pSCI->m_DataMap); 
		GStringIterator itDM(&listDataMap);
		char chNULL = 0;
		GStringList list;
		unsigned long nTotal = 0;
		while(itDM())
		{
			int nSize = atoi(itDM++);
			if (nSize == 0)
			{
				list.AddLastUserConstructed(new GString(&chNULL,1,1,0));
			}
			else
			{
				// leave the memory is it's source buffer, do not copy it.
				list.AddLastUserConstructed(new GString(&pzArgs[nTotal],nSize,nSize,0));
			}
			nTotal += nSize;
		}

		GStringIterator it(&list);
		return pSCI->Invoke(pzComponent,pzInterface,pzMethodName,it);
	}

	return 0; // never hits this code
}

extern "C" MSFT_EXPORT void Destruct(void *pInst)
{
	CStdCallInterface *pSCI = (CStdCallInterface *)pInst;
	delete pSCI;
}

