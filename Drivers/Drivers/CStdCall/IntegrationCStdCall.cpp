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
#include "GDirectory.h"
#include "GException.h"
#include "DynamicLibrary.h"


CStdCallInterface::CStdCallInterface(const char *pzComponentPath, const char *pzComponentSettings)
	:	BaseInterface(pzComponentPath, pzComponentSettings)
{
	m_Delimit = "&~&";
	m_strComponentPath = pzComponentPath;
	m_strComponentSettings = pzComponentSettings;
	m_PIBuffer = 0;
	m_WKBuffer = 0;
	m_WKBuffer2 = 0;
	m_nWKBufferSize = 0;
	m_bUnload = 0;
	m_dllHandle = 0;
	m_DataMap = 0;
}

CStdCallInterface::~CStdCallInterface()
{
}

GStringList* CStdCallInterface::GetComponents()
{
	m_pObjectList.RemoveAll();
	GDirectory dir( m_strComponentPath );
	GStringIterator it(&dir);
	
	int nObjCount = 0;
	while (it())
	{
		nObjCount++;
		m_pObjectList.AddLast( it++ );
	}

	if (!nObjCount)
	{
		throw GException("CStdCallIntegration", 1,(const char *)m_strComponentPath);
	}
	return &m_pObjectList;
}

GStringList* CStdCallInterface::GetInterfaces(const char *pzDynamicLib)
{
	// remove the .DLL or .SO from the load lib and return the base name
	m_pInterfaceList.RemoveAll();
	char *ch = (char *)strpbrk(pzDynamicLib,".");
	if (ch)
		*ch = 0;
	m_pInterfaceList.AddLast(pzDynamicLib);
	return &m_pInterfaceList;
}

const char *CStdCallInterface::FullPathDynamicLib(const char *pzDynamicLib)
{
	m_strPathQualifiedDynamicLib = (const char *)m_strComponentPath;
	m_strPathQualifiedDynamicLib += pzDynamicLib;
	return (const char *)m_strPathQualifiedDynamicLib;
}


GStringList* CStdCallInterface::GetMethods(const char *pzDynamicLib, const char *pzInterface)
{
	m_pMethodList.RemoveAll();
	InitTypeInfo(pzDynamicLib);
	GListIterator it( m_iC.GetMethods() );
	while (it())
	{
		iMethod *p = (iMethod *)it++;
		m_pMethodList.AddLast( p->GetName() );
	}
	
	return &m_pMethodList;
}

GStringList* CStdCallInterface::GetMethodParams(const char *pzDynamicLib, const char *pzInterface, const char *pzMethodName)
{
	m_pParamNameList.RemoveAll();
	m_pParamTypeList.RemoveAll();
	InitTypeInfo(pzDynamicLib);

	GString strFindMethod(pzMethodName);
	GListIterator it( m_iC.GetMethods() );
	while (it())
	{
		iMethod *p = (iMethod *)it++;
		if (strFindMethod.CompareNoCase( p->GetName() ) == 0)
		{
			GListIterator itParams( p->GetParams() );
			while (itParams())
			{
				iParam *iP = (iParam *)itParams++;
				m_pParamNameList.AddLast(iP->GetName());
				m_pParamTypeList.AddLast(iP->GetType());
			}
		}
	}
	return &m_pParamNameList;
}
// return detailed information about the last method described by GetMethodParams()
GStringList* CStdCallInterface::GetMethodParamTypes()
{
	return &m_pParamTypeList;
}

const char * CStdCallInterface::GetMethodReturnType()
{
	// the only type allowed
	m_strReturnType = "char *";
	return m_strReturnType;
}




int CStdCallInterface::IsAvailable(const char *pzObject, const char *pzInterface, const char *pzMethodName)
{
	InitTypeInfo(pzObject);
	return m_iC.IsMethodAvailable(pzMethodName);
}

const char * CStdCallInterface::Invoke(const char *pzDynamicLib, const char *pzInterface, const char *pzMethodName, GStringIterator &varArgs)
{
	InitTypeInfo(pzDynamicLib);

	GString strErrorMessage;
	const char *pzPathedDynamicLib = FullPathDynamicLib(pzDynamicLib);
	if (!m_dllHandle)
		m_dllHandle = _OPENLIB( pzPathedDynamicLib );

	if (m_dllHandle)
	{
		void *pfn = (void *)_PROCADDRESS(m_dllHandle, pzMethodName);
		if (pfn)
		{
			InnerInvoke(pfn,
						pzMethodName,
						pzDynamicLib,
						varArgs,
						m_iC.GetRequiredParamCount(pzMethodName));
		}
		else
		{
			GString strMsg(pzPathedDynamicLib);
			strMsg << " method:" << pzMethodName;
			throw GException("CStdCallIntegration", 2,(const char *)strMsg);
		}
		if (m_bUnload)
		{
			_CLOSELIB(m_dllHandle);
			m_dllHandle = 0;
		}
	}
	else
	{
#ifndef _WIN32
		// UNIX environments only
		throw GException("CStdCallIntegration", 7,dlerror(),pzDynamicLib);
#else
		// NT could not load the DLL.
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

		throw GException("CStdCallIntegration", 4,(const char *)pzDynamicLib,dwError,(lpMsgBuf) ? lpMsgBuf : "");
#endif
	}
	return m_pRsltStr->StrVal();
}

// OK, few people pass 64 arguments and if they would I would ask them why they did it that way.
// But - Limitations suck, and Only the C++ driver has a limitation of the number of arguments that can be passed
// all the other languages have a late binding / dynamic binding way of handling this
// supporting insane amounts of arguments does not hinder performance or memory use for normal amounts
// so the limit was set at 64 arguments.
typedef void(*C0)(void *, void (*CB)(void *pCBArg, const char *pzCmd, const char *pzData));
typedef void(*C1)(void *, void (*CB)(void *pCBArg, const char *pzCmd, const char *pzData), char *);
typedef void(*C2)(void *, void (*CB)(void *pCBArg, const char *pzCmd, const char *pzData), char *,char *);
typedef void(*C3)(void *, void (*CB)(void *pCBArg, const char *pzCmd, const char *pzData), char *,char *,char *);
typedef void(*C4)(void *, void (*CB)(void *pCBArg, const char *pzCmd, const char *pzData), char *,char *,char *,char *);
typedef void(*C5)(void *, void (*CB)(void *pCBArg, const char *pzCmd, const char *pzData), char *,char *,char *,char *,char *);
typedef void(*C6)(void *, void (*CB)(void *pCBArg, const char *pzCmd, const char *pzData), char *,char *,char *,char *,char *,char *);
typedef void(*C7)(void *, void (*CB)(void *pCBArg, const char *pzCmd, const char *pzData), char *,char *,char *,char *,char *,char *,char *);
typedef void(*C8)(void *, void (*CB)(void *pCBArg, const char *pzCmd, const char *pzData), char *,char *,char *,char *,char *,char *,char *,char *);
typedef void(*C9)(void *, void (*CB)(void *pCBArg, const char *pzCmd, const char *pzData), char *,char *,char *,char *,char *,char *,char *,char *,char *);
typedef void(*C10)(void *, void (*CB)(void *pCBArg, const char *pzCmd, const char *pzData), char *,char *,char *,char *,char *,char *,char *,char *,char *,char *);
typedef void(*C11)(void *, void (*CB)(void *pCBArg, const char *pzCmd, const char *pzData), char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *);
typedef void(*C12)(void *, void (*CB)(void *pCBArg, const char *pzCmd, const char *pzData), char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *);
typedef void(*C13)(void *, void (*CB)(void *pCBArg, const char *pzCmd, const char *pzData), char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *);
typedef void(*C14)(void *, void (*CB)(void *pCBArg, const char *pzCmd, const char *pzData), char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *);
typedef void(*C15)(void *, void (*CB)(void *pCBArg, const char *pzCmd, const char *pzData), char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *);
typedef void(*C16)(void *, void (*CB)(void *pCBArg, const char *pzCmd, const char *pzData), char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *);
typedef void(*C17)(void *, void (*CB)(void *pCBArg, const char *pzCmd, const char *pzData), char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *);
typedef void(*C18)(void *, void (*CB)(void *pCBArg, const char *pzCmd, const char *pzData), char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *);
typedef void(*C19)(void *, void (*CB)(void *pCBArg, const char *pzCmd, const char *pzData), char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *);
typedef void(*C20)(void *, void (*CB)(void *pCBArg, const char *pzCmd, const char *pzData), char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *);
typedef void(*C21)(void *, void (*CB)(void *pCBArg, const char *pzCmd, const char *pzData), char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *);
typedef void(*C22)(void *, void (*CB)(void *pCBArg, const char *pzCmd, const char *pzData), char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *);
typedef void(*C23)(void *, void (*CB)(void *pCBArg, const char *pzCmd, const char *pzData), char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *);
typedef void(*C24)(void *, void (*CB)(void *pCBArg, const char *pzCmd, const char *pzData), char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *);
typedef void(*C25)(void *, void (*CB)(void *pCBArg, const char *pzCmd, const char *pzData), char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *);
typedef void(*C26)(void *, void (*CB)(void *pCBArg, const char *pzCmd, const char *pzData), char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *);
typedef void(*C27)(void *, void (*CB)(void *pCBArg, const char *pzCmd, const char *pzData), char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *);
typedef void(*C28)(void *, void (*CB)(void *pCBArg, const char *pzCmd, const char *pzData), char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *);
typedef void(*C29)(void *, void (*CB)(void *pCBArg, const char *pzCmd, const char *pzData), char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *);
typedef void(*C30)(void *, void (*CB)(void *pCBArg, const char *pzCmd, const char *pzData), char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *);
typedef void(*C31)(void *, void (*CB)(void *pCBArg, const char *pzCmd, const char *pzData), char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *);
typedef void(*C32)(void *, void (*CB)(void *pCBArg, const char *pzCmd, const char *pzData), char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *);
typedef void(*C33)(void *, void (*CB)(void *pCBArg, const char *pzCmd, const char *pzData), char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *);
typedef void(*C34)(void *, void (*CB)(void *pCBArg, const char *pzCmd, const char *pzData), char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *);
typedef void(*C35)(void *, void (*CB)(void *pCBArg, const char *pzCmd, const char *pzData), char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *);
typedef void(*C36)(void *, void (*CB)(void *pCBArg, const char *pzCmd, const char *pzData), char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *);
typedef void(*C37)(void *, void (*CB)(void *pCBArg, const char *pzCmd, const char *pzData), char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *);
typedef void(*C38)(void *, void (*CB)(void *pCBArg, const char *pzCmd, const char *pzData), char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *);
typedef void(*C39)(void *, void (*CB)(void *pCBArg, const char *pzCmd, const char *pzData), char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *);
typedef void(*C40)(void *, void (*CB)(void *pCBArg, const char *pzCmd, const char *pzData), char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *);
typedef void(*C41)(void *, void (*CB)(void *pCBArg, const char *pzCmd, const char *pzData), char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *);
typedef void(*C42)(void *, void (*CB)(void *pCBArg, const char *pzCmd, const char *pzData), char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *);
typedef void(*C43)(void *, void (*CB)(void *pCBArg, const char *pzCmd, const char *pzData), char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *);
typedef void(*C44)(void *, void (*CB)(void *pCBArg, const char *pzCmd, const char *pzData), char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *);
typedef void(*C45)(void *, void (*CB)(void *pCBArg, const char *pzCmd, const char *pzData), char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *);
typedef void(*C46)(void *, void (*CB)(void *pCBArg, const char *pzCmd, const char *pzData), char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *);
typedef void(*C47)(void *, void (*CB)(void *pCBArg, const char *pzCmd, const char *pzData), char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *);
typedef void(*C48)(void *, void (*CB)(void *pCBArg, const char *pzCmd, const char *pzData), char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *);
typedef void(*C49)(void *, void (*CB)(void *pCBArg, const char *pzCmd, const char *pzData), char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *);
typedef void(*C50)(void *, void (*CB)(void *pCBArg, const char *pzCmd, const char *pzData), char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *);
typedef void(*C51)(void *, void (*CB)(void *pCBArg, const char *pzCmd, const char *pzData), char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *);
typedef void(*C52)(void *, void (*CB)(void *pCBArg, const char *pzCmd, const char *pzData), char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *);
typedef void(*C53)(void *, void (*CB)(void *pCBArg, const char *pzCmd, const char *pzData), char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *);
typedef void(*C54)(void *, void (*CB)(void *pCBArg, const char *pzCmd, const char *pzData), char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *);
typedef void(*C55)(void *, void (*CB)(void *pCBArg, const char *pzCmd, const char *pzData), char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *);
typedef void(*C56)(void *, void (*CB)(void *pCBArg, const char *pzCmd, const char *pzData), char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *);
typedef void(*C57)(void *, void (*CB)(void *pCBArg, const char *pzCmd, const char *pzData), char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *);
typedef void(*C58)(void *, void (*CB)(void *pCBArg, const char *pzCmd, const char *pzData), char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *);
typedef void(*C59)(void *, void (*CB)(void *pCBArg, const char *pzCmd, const char *pzData), char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *);
typedef void(*C60)(void *, void (*CB)(void *pCBArg, const char *pzCmd, const char *pzData), char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *);
typedef void(*C61)(void *, void (*CB)(void *pCBArg, const char *pzCmd, const char *pzData), char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *);
typedef void(*C62)(void *, void (*CB)(void *pCBArg, const char *pzCmd, const char *pzData), char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *);
typedef void(*C63)(void *, void (*CB)(void *pCBArg, const char *pzCmd, const char *pzData), char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *);
typedef void(*C64)(void *, void (*CB)(void *pCBArg, const char *pzCmd, const char *pzData), char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *,char *);




int CStdCallInterface::SetOption(const char *pzOption, void *pzValue)
{
	GString strOption(pzOption);
	if (strOption.CompareNoCase("unload") == 0)
	{
		m_bUnload = *(int *)pzValue;
		return 1;
	}
	if (strOption.CompareNoCase("rsltGStr") == 0)
	{
		m_pRsltStr = (GString *)pzValue;
		return 1;
	}
	if (strOption.CompareNoCase("Delimit") == 0)
	{
		m_Delimit = (const char *)pzValue;
		return 1;
	}
	if (strOption.CompareNoCase("DataMap") == 0)
	{
		m_DataMap = (const char *)pzValue;
		return 1;
	}
	if (strOption.CompareNoCase("callback") == 0)
	{
		m_CB = (CBfn)pzValue;
		return 1;
	}
	if (strOption.CompareNoCase("callbackArg") == 0)
	{
		m_CBArg = pzValue;
		return 1;
	}
	if (strOption.CompareNoCase("piBuf") == 0)
	{
		m_PIBuffer = (char *)pzValue;
		return 1;
	}
	if (strOption.CompareNoCase("wkBufSize") == 0)
	{
		m_nWKBufferSize = (int)pzValue;
	}
	if (strOption.CompareNoCase("wkBuf") == 0)
	{
		m_WKBuffer = (unsigned char *)pzValue;
		return 1;
	}
	if (strOption.CompareNoCase("wkBuf2") == 0)
	{
		m_WKBuffer2 = (unsigned char *)pzValue;
		return 1;
	}
	return 0;
}


void *CStdCallInterface::GetOption(const char *pzOption)
{
	GString strOption(pzOption);

	if (strOption.CompareNoCase("rsltGStr") == 0)
	{
		return m_pRsltStr;
	}
	if (strOption.CompareNoCase("Delimit") == 0)
	{
		return (void *)(const char *)m_Delimit;
	}
	if (strOption.CompareNoCase("DataMap") == 0)
	{
		return (void *)(const char *)m_DataMap;
	}

	return 0;
}



void DrvCallBack(void *pCBArg, const char *pzCmd, const char *)
{
	CBDataStruct *pcb = (CBDataStruct *)pCBArg;
	CStdCallInterface *pThis = (CStdCallInterface *)pcb->pDriverInterface;

	if (pzCmd && pzCmd[0] == 'm' && pzCmd[1] == 'a' && pzCmd[2] == 'p')
	{
		if (pThis && pThis->m_DataMap)
		{
			int nIdx = pcb->arg[0].Int;
			GStringList lst("|",pThis->m_DataMap);
			if (nIdx == 0 || nIdx > lst.Size()) // 1 based index
			{
				pcb->arg[0].Int = -1; // invalid index
			}
			else
			{
				GStringIterator it(&lst);
				for(int i=0; i < nIdx; i++)
				{
					if (i == (nIdx - 1))
					{
						pcb->arg[0].Int = atoi(it++); 
					}
					else
					{
						it++; // advance 1 more
					}
				}
			}
		}
		else
		{
			pcb->arg[0].Int = -2; // this cant happen
		}
		return;
	}


	if (pcb->fn)
	{
		pcb->fn(pzCmd,pcb);
	}
}
void CStdCallInterface::InnerInvoke(void *pfn, const char *pzMethodName, const char *pzModuleName, 
									GStringIterator &varArgs, int nRequiredParamCount)
{
	int nPassedParams = 0;
	char *pArg[100];
	while (varArgs())
	{
		pArg[nPassedParams++] = (char *)(const char *)varArgs++;
	}
	
	if (nPassedParams != nRequiredParamCount)
	{
		throw GException("CStdCallIntegration", 5,pzMethodName,pzModuleName,nRequiredParamCount,nPassedParams);
	}
	
	
	CBDataStruct cbd;
	cbd.pstrDest = m_pRsltStr; 
	cbd.pCBArg = m_CBArg;
	cbd.fn = m_CB;
	cbd.piBuf = m_PIBuffer;
	cbd.wkBuf = (char *)m_WKBuffer;
	cbd.wkBuf2 = (char *)m_WKBuffer2;
	cbd.nWKBufferSize = m_nWKBufferSize;
	cbd.pDriverInterface = this;
	

	switch(nRequiredParamCount)
	{
	case 0:   ((C0)pfn)(&cbd, DrvCallBack); break;
	case 1:   ((C1)pfn)(&cbd, DrvCallBack, pArg[0]); break;
	case 2:   ((C2)pfn)(&cbd, DrvCallBack, pArg[0],pArg[1]); break;
	case 3:   ((C3)pfn)(&cbd, DrvCallBack, pArg[0],pArg[1],pArg[2]); break;
	case 4:   ((C4)pfn)(&cbd, DrvCallBack, pArg[0],pArg[1],pArg[2],pArg[3]); break;
	case 5:   ((C5)pfn)(&cbd, DrvCallBack, pArg[0],pArg[1],pArg[2],pArg[3],pArg[4]); break;
	case 6:   ((C6)pfn)(&cbd, DrvCallBack, pArg[0],pArg[1],pArg[2],pArg[3],pArg[4],pArg[5]); break;
	case 7:   ((C7)pfn)(&cbd, DrvCallBack, pArg[0],pArg[1],pArg[2],pArg[3],pArg[4],pArg[5],pArg[6]); break;
	case 8:   ((C8)pfn)(&cbd, DrvCallBack, pArg[0],pArg[1],pArg[2],pArg[3],pArg[4],pArg[5],pArg[6],pArg[7]); break;
	case 9:   ((C9)pfn)(&cbd, DrvCallBack, pArg[0],pArg[1],pArg[2],pArg[3],pArg[4],pArg[5],pArg[6],pArg[7],pArg[8]); break;
	case 10: ((C10)pfn)(&cbd, DrvCallBack, pArg[0],pArg[1],pArg[2],pArg[3],pArg[4],pArg[5],pArg[6],pArg[7],pArg[8],pArg[9]); break;
	case 11: ((C11)pfn)(&cbd, DrvCallBack, pArg[0],pArg[1],pArg[2],pArg[3],pArg[4],pArg[5],pArg[6],pArg[7],pArg[8],pArg[9],pArg[10]); break;
	case 12: ((C12)pfn)(&cbd, DrvCallBack, pArg[0],pArg[1],pArg[2],pArg[3],pArg[4],pArg[5],pArg[6],pArg[7],pArg[8],pArg[9],pArg[10],pArg[11]); break;
	case 13: ((C13)pfn)(&cbd, DrvCallBack, pArg[0],pArg[1],pArg[2],pArg[3],pArg[4],pArg[5],pArg[6],pArg[7],pArg[8],pArg[9],pArg[10],pArg[11],pArg[12]); break;
	case 14: ((C14)pfn)(&cbd, DrvCallBack, pArg[0],pArg[1],pArg[2],pArg[3],pArg[4],pArg[5],pArg[6],pArg[7],pArg[8],pArg[9],pArg[10],pArg[11],pArg[12],pArg[13]); break;
	case 15: ((C15)pfn)(&cbd, DrvCallBack, pArg[0],pArg[1],pArg[2],pArg[3],pArg[4],pArg[5],pArg[6],pArg[7],pArg[8],pArg[9],pArg[10],pArg[11],pArg[12],pArg[13],pArg[14]); break;
	case 16: ((C16)pfn)(&cbd, DrvCallBack, pArg[0],pArg[1],pArg[2],pArg[3],pArg[4],pArg[5],pArg[6],pArg[7],pArg[8],pArg[9],pArg[10],pArg[11],pArg[12],pArg[13],pArg[14],pArg[15]); break;
	case 17: ((C17)pfn)(&cbd, DrvCallBack, pArg[0],pArg[1],pArg[2],pArg[3],pArg[4],pArg[5],pArg[6],pArg[7],pArg[8],pArg[9],pArg[10],pArg[11],pArg[12],pArg[13],pArg[14],pArg[15],pArg[16]); break;
	case 18: ((C18)pfn)(&cbd, DrvCallBack, pArg[0],pArg[1],pArg[2],pArg[3],pArg[4],pArg[5],pArg[6],pArg[7],pArg[8],pArg[9],pArg[10],pArg[11],pArg[12],pArg[13],pArg[14],pArg[15],pArg[16],pArg[17]); break;
	case 19: ((C19)pfn)(&cbd, DrvCallBack, pArg[0],pArg[1],pArg[2],pArg[3],pArg[4],pArg[5],pArg[6],pArg[7],pArg[8],pArg[9],pArg[10],pArg[11],pArg[12],pArg[13],pArg[14],pArg[15],pArg[16],pArg[17],pArg[18]); break;
	case 20: ((C20)pfn)(&cbd, DrvCallBack, pArg[0],pArg[1],pArg[2],pArg[3],pArg[4],pArg[5],pArg[6],pArg[7],pArg[8],pArg[9],pArg[10],pArg[11],pArg[12],pArg[13],pArg[14],pArg[15],pArg[16],pArg[17],pArg[18],pArg[19]); break;
	case 21: ((C21)pfn)(&cbd, DrvCallBack, pArg[0],pArg[1],pArg[2],pArg[3],pArg[4],pArg[5],pArg[6],pArg[7],pArg[8],pArg[9],pArg[10],pArg[11],pArg[12],pArg[13],pArg[14],pArg[15],pArg[16],pArg[17],pArg[18],pArg[19],pArg[20]); break;
	case 22: ((C22)pfn)(&cbd, DrvCallBack, pArg[0],pArg[1],pArg[2],pArg[3],pArg[4],pArg[5],pArg[6],pArg[7],pArg[8],pArg[9],pArg[10],pArg[11],pArg[12],pArg[13],pArg[14],pArg[15],pArg[16],pArg[17],pArg[18],pArg[19],pArg[20],pArg[21]); break;
	case 23: ((C23)pfn)(&cbd, DrvCallBack, pArg[0],pArg[1],pArg[2],pArg[3],pArg[4],pArg[5],pArg[6],pArg[7],pArg[8],pArg[9],pArg[10],pArg[11],pArg[12],pArg[13],pArg[14],pArg[15],pArg[16],pArg[17],pArg[18],pArg[19],pArg[20],pArg[21],pArg[22]); break;
	case 24: ((C24)pfn)(&cbd, DrvCallBack, pArg[0],pArg[1],pArg[2],pArg[3],pArg[4],pArg[5],pArg[6],pArg[7],pArg[8],pArg[9],pArg[10],pArg[11],pArg[12],pArg[13],pArg[14],pArg[15],pArg[16],pArg[17],pArg[18],pArg[19],pArg[20],pArg[21],pArg[22],pArg[23]); break;
	case 25: ((C25)pfn)(&cbd, DrvCallBack, pArg[0],pArg[1],pArg[2],pArg[3],pArg[4],pArg[5],pArg[6],pArg[7],pArg[8],pArg[9],pArg[10],pArg[11],pArg[12],pArg[13],pArg[14],pArg[15],pArg[16],pArg[17],pArg[18],pArg[19],pArg[20],pArg[21],pArg[22],pArg[23],pArg[24]); break;
	case 26: ((C26)pfn)(&cbd, DrvCallBack, pArg[0],pArg[1],pArg[2],pArg[3],pArg[4],pArg[5],pArg[6],pArg[7],pArg[8],pArg[9],pArg[10],pArg[11],pArg[12],pArg[13],pArg[14],pArg[15],pArg[16],pArg[17],pArg[18],pArg[19],pArg[20],pArg[21],pArg[22],pArg[23],pArg[24],pArg[25]); break;
	case 27: ((C27)pfn)(&cbd, DrvCallBack, pArg[0],pArg[1],pArg[2],pArg[3],pArg[4],pArg[5],pArg[6],pArg[7],pArg[8],pArg[9],pArg[10],pArg[11],pArg[12],pArg[13],pArg[14],pArg[15],pArg[16],pArg[17],pArg[18],pArg[19],pArg[20],pArg[21],pArg[22],pArg[23],pArg[24],pArg[25],pArg[26]); break;
	case 28: ((C28)pfn)(&cbd, DrvCallBack, pArg[0],pArg[1],pArg[2],pArg[3],pArg[4],pArg[5],pArg[6],pArg[7],pArg[8],pArg[9],pArg[10],pArg[11],pArg[12],pArg[13],pArg[14],pArg[15],pArg[16],pArg[17],pArg[18],pArg[19],pArg[20],pArg[21],pArg[22],pArg[23],pArg[24],pArg[25],pArg[26],pArg[27]); break;
	case 29: ((C29)pfn)(&cbd, DrvCallBack, pArg[0],pArg[1],pArg[2],pArg[3],pArg[4],pArg[5],pArg[6],pArg[7],pArg[8],pArg[9],pArg[10],pArg[11],pArg[12],pArg[13],pArg[14],pArg[15],pArg[16],pArg[17],pArg[18],pArg[19],pArg[20],pArg[21],pArg[22],pArg[23],pArg[24],pArg[25],pArg[26],pArg[27],pArg[28]); break;
	case 30: ((C30)pfn)(&cbd, DrvCallBack, pArg[0],pArg[1],pArg[2],pArg[3],pArg[4],pArg[5],pArg[6],pArg[7],pArg[8],pArg[9],pArg[10],pArg[11],pArg[12],pArg[13],pArg[14],pArg[15],pArg[16],pArg[17],pArg[18],pArg[19],pArg[20],pArg[21],pArg[22],pArg[23],pArg[24],pArg[25],pArg[26],pArg[27],pArg[28],pArg[29]); break;
	case 31: ((C31)pfn)(&cbd, DrvCallBack, pArg[0],pArg[1],pArg[2],pArg[3],pArg[4],pArg[5],pArg[6],pArg[7],pArg[8],pArg[9],pArg[10],pArg[11],pArg[12],pArg[13],pArg[14],pArg[15],pArg[16],pArg[17],pArg[18],pArg[19],pArg[20],pArg[21],pArg[22],pArg[23],pArg[24],pArg[25],pArg[26],pArg[27],pArg[28],pArg[29],pArg[30]); break;
	case 32: ((C32)pfn)(&cbd, DrvCallBack, pArg[0],pArg[1],pArg[2],pArg[3],pArg[4],pArg[5],pArg[6],pArg[7],pArg[8],pArg[9],pArg[10],pArg[11],pArg[12],pArg[13],pArg[14],pArg[15],pArg[16],pArg[17],pArg[18],pArg[19],pArg[20],pArg[21],pArg[22],pArg[23],pArg[24],pArg[25],pArg[26],pArg[27],pArg[28],pArg[29],pArg[30],pArg[31]); break;
	case 33: ((C33)pfn)(&cbd, DrvCallBack, pArg[0],pArg[1],pArg[2],pArg[3],pArg[4],pArg[5],pArg[6],pArg[7],pArg[8],pArg[9],pArg[10],pArg[11],pArg[12],pArg[13],pArg[14],pArg[15],pArg[16],pArg[17],pArg[18],pArg[19],pArg[20],pArg[21],pArg[22],pArg[23],pArg[24],pArg[25],pArg[26],pArg[27],pArg[28],pArg[29],pArg[30],pArg[31],pArg[32]); break;
	case 34: ((C34)pfn)(&cbd, DrvCallBack, pArg[0],pArg[1],pArg[2],pArg[3],pArg[4],pArg[5],pArg[6],pArg[7],pArg[8],pArg[9],pArg[10],pArg[11],pArg[12],pArg[13],pArg[14],pArg[15],pArg[16],pArg[17],pArg[18],pArg[19],pArg[20],pArg[21],pArg[22],pArg[23],pArg[24],pArg[25],pArg[26],pArg[27],pArg[28],pArg[29],pArg[30],pArg[31],pArg[32],pArg[33]); break;
	case 35: ((C35)pfn)(&cbd, DrvCallBack, pArg[0],pArg[1],pArg[2],pArg[3],pArg[4],pArg[5],pArg[6],pArg[7],pArg[8],pArg[9],pArg[10],pArg[11],pArg[12],pArg[13],pArg[14],pArg[15],pArg[16],pArg[17],pArg[18],pArg[19],pArg[20],pArg[21],pArg[22],pArg[23],pArg[24],pArg[25],pArg[26],pArg[27],pArg[28],pArg[29],pArg[30],pArg[31],pArg[32],pArg[33],pArg[34]); break;
	case 36: ((C36)pfn)(&cbd, DrvCallBack, pArg[0],pArg[1],pArg[2],pArg[3],pArg[4],pArg[5],pArg[6],pArg[7],pArg[8],pArg[9],pArg[10],pArg[11],pArg[12],pArg[13],pArg[14],pArg[15],pArg[16],pArg[17],pArg[18],pArg[19],pArg[20],pArg[21],pArg[22],pArg[23],pArg[24],pArg[25],pArg[26],pArg[27],pArg[28],pArg[29],pArg[30],pArg[31],pArg[32],pArg[33],pArg[34],pArg[35]); break;
	case 37: ((C37)pfn)(&cbd, DrvCallBack, pArg[0],pArg[1],pArg[2],pArg[3],pArg[4],pArg[5],pArg[6],pArg[7],pArg[8],pArg[9],pArg[10],pArg[11],pArg[12],pArg[13],pArg[14],pArg[15],pArg[16],pArg[17],pArg[18],pArg[19],pArg[20],pArg[21],pArg[22],pArg[23],pArg[24],pArg[25],pArg[26],pArg[27],pArg[28],pArg[29],pArg[30],pArg[31],pArg[32],pArg[33],pArg[34],pArg[35],pArg[36]); break;
	case 38: ((C38)pfn)(&cbd, DrvCallBack, pArg[0],pArg[1],pArg[2],pArg[3],pArg[4],pArg[5],pArg[6],pArg[7],pArg[8],pArg[9],pArg[10],pArg[11],pArg[12],pArg[13],pArg[14],pArg[15],pArg[16],pArg[17],pArg[18],pArg[19],pArg[20],pArg[21],pArg[22],pArg[23],pArg[24],pArg[25],pArg[26],pArg[27],pArg[28],pArg[29],pArg[30],pArg[31],pArg[32],pArg[33],pArg[34],pArg[35],pArg[36],pArg[37]); break;
	case 39: ((C39)pfn)(&cbd, DrvCallBack, pArg[0],pArg[1],pArg[2],pArg[3],pArg[4],pArg[5],pArg[6],pArg[7],pArg[8],pArg[9],pArg[10],pArg[11],pArg[12],pArg[13],pArg[14],pArg[15],pArg[16],pArg[17],pArg[18],pArg[19],pArg[20],pArg[21],pArg[22],pArg[23],pArg[24],pArg[25],pArg[26],pArg[27],pArg[28],pArg[29],pArg[30],pArg[31],pArg[32],pArg[33],pArg[34],pArg[35],pArg[36],pArg[37],pArg[38]); break;
	case 40: ((C40)pfn)(&cbd, DrvCallBack, pArg[0],pArg[1],pArg[2],pArg[3],pArg[4],pArg[5],pArg[6],pArg[7],pArg[8],pArg[9],pArg[10],pArg[11],pArg[12],pArg[13],pArg[14],pArg[15],pArg[16],pArg[17],pArg[18],pArg[19],pArg[20],pArg[21],pArg[22],pArg[23],pArg[24],pArg[25],pArg[26],pArg[27],pArg[28],pArg[29],pArg[30],pArg[31],pArg[32],pArg[33],pArg[34],pArg[35],pArg[36],pArg[37],pArg[38],pArg[39]); break;
	case 41: ((C41)pfn)(&cbd, DrvCallBack, pArg[0],pArg[1],pArg[2],pArg[3],pArg[4],pArg[5],pArg[6],pArg[7],pArg[8],pArg[9],pArg[10],pArg[11],pArg[12],pArg[13],pArg[14],pArg[15],pArg[16],pArg[17],pArg[18],pArg[19],pArg[20],pArg[21],pArg[22],pArg[23],pArg[24],pArg[25],pArg[26],pArg[27],pArg[28],pArg[29],pArg[30],pArg[31],pArg[32],pArg[33],pArg[34],pArg[35],pArg[36],pArg[37],pArg[38],pArg[39],pArg[40]); break;
	case 42: ((C42)pfn)(&cbd, DrvCallBack, pArg[0],pArg[1],pArg[2],pArg[3],pArg[4],pArg[5],pArg[6],pArg[7],pArg[8],pArg[9],pArg[10],pArg[11],pArg[12],pArg[13],pArg[14],pArg[15],pArg[16],pArg[17],pArg[18],pArg[19],pArg[20],pArg[21],pArg[22],pArg[23],pArg[24],pArg[25],pArg[26],pArg[27],pArg[28],pArg[29],pArg[30],pArg[31],pArg[32],pArg[33],pArg[34],pArg[35],pArg[36],pArg[37],pArg[38],pArg[39],pArg[40],pArg[41]); break;
	case 43: ((C43)pfn)(&cbd, DrvCallBack, pArg[0],pArg[1],pArg[2],pArg[3],pArg[4],pArg[5],pArg[6],pArg[7],pArg[8],pArg[9],pArg[10],pArg[11],pArg[12],pArg[13],pArg[14],pArg[15],pArg[16],pArg[17],pArg[18],pArg[19],pArg[20],pArg[21],pArg[22],pArg[23],pArg[24],pArg[25],pArg[26],pArg[27],pArg[28],pArg[29],pArg[30],pArg[31],pArg[32],pArg[33],pArg[34],pArg[35],pArg[36],pArg[37],pArg[38],pArg[39],pArg[40],pArg[41],pArg[42]); break;
	case 44: ((C44)pfn)(&cbd, DrvCallBack, pArg[0],pArg[1],pArg[2],pArg[3],pArg[4],pArg[5],pArg[6],pArg[7],pArg[8],pArg[9],pArg[10],pArg[11],pArg[12],pArg[13],pArg[14],pArg[15],pArg[16],pArg[17],pArg[18],pArg[19],pArg[20],pArg[21],pArg[22],pArg[23],pArg[24],pArg[25],pArg[26],pArg[27],pArg[28],pArg[29],pArg[30],pArg[31],pArg[32],pArg[33],pArg[34],pArg[35],pArg[36],pArg[37],pArg[38],pArg[39],pArg[40],pArg[41],pArg[42],pArg[43]); break;
	case 45: ((C45)pfn)(&cbd, DrvCallBack, pArg[0],pArg[1],pArg[2],pArg[3],pArg[4],pArg[5],pArg[6],pArg[7],pArg[8],pArg[9],pArg[10],pArg[11],pArg[12],pArg[13],pArg[14],pArg[15],pArg[16],pArg[17],pArg[18],pArg[19],pArg[20],pArg[21],pArg[22],pArg[23],pArg[24],pArg[25],pArg[26],pArg[27],pArg[28],pArg[29],pArg[30],pArg[31],pArg[32],pArg[33],pArg[34],pArg[35],pArg[36],pArg[37],pArg[38],pArg[39],pArg[40],pArg[41],pArg[42],pArg[43],pArg[44]); break;
	case 46: ((C46)pfn)(&cbd, DrvCallBack, pArg[0],pArg[1],pArg[2],pArg[3],pArg[4],pArg[5],pArg[6],pArg[7],pArg[8],pArg[9],pArg[10],pArg[11],pArg[12],pArg[13],pArg[14],pArg[15],pArg[16],pArg[17],pArg[18],pArg[19],pArg[20],pArg[21],pArg[22],pArg[23],pArg[24],pArg[25],pArg[26],pArg[27],pArg[28],pArg[29],pArg[30],pArg[31],pArg[32],pArg[33],pArg[34],pArg[35],pArg[36],pArg[37],pArg[38],pArg[39],pArg[40],pArg[41],pArg[42],pArg[43],pArg[44],pArg[45]); break;
	case 47: ((C47)pfn)(&cbd, DrvCallBack, pArg[0],pArg[1],pArg[2],pArg[3],pArg[4],pArg[5],pArg[6],pArg[7],pArg[8],pArg[9],pArg[10],pArg[11],pArg[12],pArg[13],pArg[14],pArg[15],pArg[16],pArg[17],pArg[18],pArg[19],pArg[20],pArg[21],pArg[22],pArg[23],pArg[24],pArg[25],pArg[26],pArg[27],pArg[28],pArg[29],pArg[30],pArg[31],pArg[32],pArg[33],pArg[34],pArg[35],pArg[36],pArg[37],pArg[38],pArg[39],pArg[40],pArg[41],pArg[42],pArg[43],pArg[44],pArg[45],pArg[46]); break;
	case 48: ((C48)pfn)(&cbd, DrvCallBack, pArg[0],pArg[1],pArg[2],pArg[3],pArg[4],pArg[5],pArg[6],pArg[7],pArg[8],pArg[9],pArg[10],pArg[11],pArg[12],pArg[13],pArg[14],pArg[15],pArg[16],pArg[17],pArg[18],pArg[19],pArg[20],pArg[21],pArg[22],pArg[23],pArg[24],pArg[25],pArg[26],pArg[27],pArg[28],pArg[29],pArg[30],pArg[31],pArg[32],pArg[33],pArg[34],pArg[35],pArg[36],pArg[37],pArg[38],pArg[39],pArg[40],pArg[41],pArg[42],pArg[43],pArg[44],pArg[45],pArg[46],pArg[47]); break;
	case 49: ((C49)pfn)(&cbd, DrvCallBack, pArg[0],pArg[1],pArg[2],pArg[3],pArg[4],pArg[5],pArg[6],pArg[7],pArg[8],pArg[9],pArg[10],pArg[11],pArg[12],pArg[13],pArg[14],pArg[15],pArg[16],pArg[17],pArg[18],pArg[19],pArg[20],pArg[21],pArg[22],pArg[23],pArg[24],pArg[25],pArg[26],pArg[27],pArg[28],pArg[29],pArg[30],pArg[31],pArg[32],pArg[33],pArg[34],pArg[35],pArg[36],pArg[37],pArg[38],pArg[39],pArg[40],pArg[41],pArg[42],pArg[43],pArg[44],pArg[45],pArg[46],pArg[47],pArg[48]); break;
	case 50: ((C50)pfn)(&cbd, DrvCallBack, pArg[0],pArg[1],pArg[2],pArg[3],pArg[4],pArg[5],pArg[6],pArg[7],pArg[8],pArg[9],pArg[10],pArg[11],pArg[12],pArg[13],pArg[14],pArg[15],pArg[16],pArg[17],pArg[18],pArg[19],pArg[20],pArg[21],pArg[22],pArg[23],pArg[24],pArg[25],pArg[26],pArg[27],pArg[28],pArg[29],pArg[30],pArg[31],pArg[32],pArg[33],pArg[34],pArg[35],pArg[36],pArg[37],pArg[38],pArg[39],pArg[40],pArg[41],pArg[42],pArg[43],pArg[44],pArg[45],pArg[46],pArg[47],pArg[48],pArg[49]); break;
	case 51: ((C51)pfn)(&cbd, DrvCallBack, pArg[0],pArg[1],pArg[2],pArg[3],pArg[4],pArg[5],pArg[6],pArg[7],pArg[8],pArg[9],pArg[10],pArg[11],pArg[12],pArg[13],pArg[14],pArg[15],pArg[16],pArg[17],pArg[18],pArg[19],pArg[20],pArg[21],pArg[22],pArg[23],pArg[24],pArg[25],pArg[26],pArg[27],pArg[28],pArg[29],pArg[30],pArg[31],pArg[32],pArg[33],pArg[34],pArg[35],pArg[36],pArg[37],pArg[38],pArg[39],pArg[40],pArg[41],pArg[42],pArg[43],pArg[44],pArg[45],pArg[46],pArg[47],pArg[48],pArg[49],pArg[50]); break;
	case 52: ((C52)pfn)(&cbd, DrvCallBack, pArg[0],pArg[1],pArg[2],pArg[3],pArg[4],pArg[5],pArg[6],pArg[7],pArg[8],pArg[9],pArg[10],pArg[11],pArg[12],pArg[13],pArg[14],pArg[15],pArg[16],pArg[17],pArg[18],pArg[19],pArg[20],pArg[21],pArg[22],pArg[23],pArg[24],pArg[25],pArg[26],pArg[27],pArg[28],pArg[29],pArg[30],pArg[31],pArg[32],pArg[33],pArg[34],pArg[35],pArg[36],pArg[37],pArg[38],pArg[39],pArg[40],pArg[41],pArg[42],pArg[43],pArg[44],pArg[45],pArg[46],pArg[47],pArg[48],pArg[49],pArg[50],pArg[51]); break;
	case 53: ((C53)pfn)(&cbd, DrvCallBack, pArg[0],pArg[1],pArg[2],pArg[3],pArg[4],pArg[5],pArg[6],pArg[7],pArg[8],pArg[9],pArg[10],pArg[11],pArg[12],pArg[13],pArg[14],pArg[15],pArg[16],pArg[17],pArg[18],pArg[19],pArg[20],pArg[21],pArg[22],pArg[23],pArg[24],pArg[25],pArg[26],pArg[27],pArg[28],pArg[29],pArg[30],pArg[31],pArg[32],pArg[33],pArg[34],pArg[35],pArg[36],pArg[37],pArg[38],pArg[39],pArg[40],pArg[41],pArg[42],pArg[43],pArg[44],pArg[45],pArg[46],pArg[47],pArg[48],pArg[49],pArg[50],pArg[51],pArg[52]); break;
	case 54: ((C54)pfn)(&cbd, DrvCallBack, pArg[0],pArg[1],pArg[2],pArg[3],pArg[4],pArg[5],pArg[6],pArg[7],pArg[8],pArg[9],pArg[10],pArg[11],pArg[12],pArg[13],pArg[14],pArg[15],pArg[16],pArg[17],pArg[18],pArg[19],pArg[20],pArg[21],pArg[22],pArg[23],pArg[24],pArg[25],pArg[26],pArg[27],pArg[28],pArg[29],pArg[30],pArg[31],pArg[32],pArg[33],pArg[34],pArg[35],pArg[36],pArg[37],pArg[38],pArg[39],pArg[40],pArg[41],pArg[42],pArg[43],pArg[44],pArg[45],pArg[46],pArg[47],pArg[48],pArg[49],pArg[50],pArg[51],pArg[52],pArg[53]); break;
	case 55: ((C55)pfn)(&cbd, DrvCallBack, pArg[0],pArg[1],pArg[2],pArg[3],pArg[4],pArg[5],pArg[6],pArg[7],pArg[8],pArg[9],pArg[10],pArg[11],pArg[12],pArg[13],pArg[14],pArg[15],pArg[16],pArg[17],pArg[18],pArg[19],pArg[20],pArg[21],pArg[22],pArg[23],pArg[24],pArg[25],pArg[26],pArg[27],pArg[28],pArg[29],pArg[30],pArg[31],pArg[32],pArg[33],pArg[34],pArg[35],pArg[36],pArg[37],pArg[38],pArg[39],pArg[40],pArg[41],pArg[42],pArg[43],pArg[44],pArg[45],pArg[46],pArg[47],pArg[48],pArg[49],pArg[50],pArg[51],pArg[52],pArg[53],pArg[54]); break;
	case 56: ((C56)pfn)(&cbd, DrvCallBack, pArg[0],pArg[1],pArg[2],pArg[3],pArg[4],pArg[5],pArg[6],pArg[7],pArg[8],pArg[9],pArg[10],pArg[11],pArg[12],pArg[13],pArg[14],pArg[15],pArg[16],pArg[17],pArg[18],pArg[19],pArg[20],pArg[21],pArg[22],pArg[23],pArg[24],pArg[25],pArg[26],pArg[27],pArg[28],pArg[29],pArg[30],pArg[31],pArg[32],pArg[33],pArg[34],pArg[35],pArg[36],pArg[37],pArg[38],pArg[39],pArg[40],pArg[41],pArg[42],pArg[43],pArg[44],pArg[45],pArg[46],pArg[47],pArg[48],pArg[49],pArg[50],pArg[51],pArg[52],pArg[53],pArg[54],pArg[55]); break;
	case 57: ((C57)pfn)(&cbd, DrvCallBack, pArg[0],pArg[1],pArg[2],pArg[3],pArg[4],pArg[5],pArg[6],pArg[7],pArg[8],pArg[9],pArg[10],pArg[11],pArg[12],pArg[13],pArg[14],pArg[15],pArg[16],pArg[17],pArg[18],pArg[19],pArg[20],pArg[21],pArg[22],pArg[23],pArg[24],pArg[25],pArg[26],pArg[27],pArg[28],pArg[29],pArg[30],pArg[31],pArg[32],pArg[33],pArg[34],pArg[35],pArg[36],pArg[37],pArg[38],pArg[39],pArg[40],pArg[41],pArg[42],pArg[43],pArg[44],pArg[45],pArg[46],pArg[47],pArg[48],pArg[49],pArg[50],pArg[51],pArg[52],pArg[53],pArg[54],pArg[55],pArg[56]); break;
	case 58: ((C58)pfn)(&cbd, DrvCallBack, pArg[0],pArg[1],pArg[2],pArg[3],pArg[4],pArg[5],pArg[6],pArg[7],pArg[8],pArg[9],pArg[10],pArg[11],pArg[12],pArg[13],pArg[14],pArg[15],pArg[16],pArg[17],pArg[18],pArg[19],pArg[20],pArg[21],pArg[22],pArg[23],pArg[24],pArg[25],pArg[26],pArg[27],pArg[28],pArg[29],pArg[30],pArg[31],pArg[32],pArg[33],pArg[34],pArg[35],pArg[36],pArg[37],pArg[38],pArg[39],pArg[40],pArg[41],pArg[42],pArg[43],pArg[44],pArg[45],pArg[46],pArg[47],pArg[48],pArg[49],pArg[50],pArg[51],pArg[52],pArg[53],pArg[54],pArg[55],pArg[56],pArg[57]); break;
	case 59: ((C59)pfn)(&cbd, DrvCallBack, pArg[0],pArg[1],pArg[2],pArg[3],pArg[4],pArg[5],pArg[6],pArg[7],pArg[8],pArg[9],pArg[10],pArg[11],pArg[12],pArg[13],pArg[14],pArg[15],pArg[16],pArg[17],pArg[18],pArg[19],pArg[20],pArg[21],pArg[22],pArg[23],pArg[24],pArg[25],pArg[26],pArg[27],pArg[28],pArg[29],pArg[30],pArg[31],pArg[32],pArg[33],pArg[34],pArg[35],pArg[36],pArg[37],pArg[38],pArg[39],pArg[40],pArg[41],pArg[42],pArg[43],pArg[44],pArg[45],pArg[46],pArg[47],pArg[48],pArg[49],pArg[50],pArg[51],pArg[52],pArg[53],pArg[54],pArg[55],pArg[56],pArg[57],pArg[58]); break;
	case 60: ((C60)pfn)(&cbd, DrvCallBack, pArg[0],pArg[1],pArg[2],pArg[3],pArg[4],pArg[5],pArg[6],pArg[7],pArg[8],pArg[9],pArg[10],pArg[11],pArg[12],pArg[13],pArg[14],pArg[15],pArg[16],pArg[17],pArg[18],pArg[19],pArg[20],pArg[21],pArg[22],pArg[23],pArg[24],pArg[25],pArg[26],pArg[27],pArg[28],pArg[29],pArg[30],pArg[31],pArg[32],pArg[33],pArg[34],pArg[35],pArg[36],pArg[37],pArg[38],pArg[39],pArg[40],pArg[41],pArg[42],pArg[43],pArg[44],pArg[45],pArg[46],pArg[47],pArg[48],pArg[49],pArg[50],pArg[51],pArg[52],pArg[53],pArg[54],pArg[55],pArg[56],pArg[57],pArg[58],pArg[59]); break;
	case 61: ((C61)pfn)(&cbd, DrvCallBack, pArg[0],pArg[1],pArg[2],pArg[3],pArg[4],pArg[5],pArg[6],pArg[7],pArg[8],pArg[9],pArg[10],pArg[11],pArg[12],pArg[13],pArg[14],pArg[15],pArg[16],pArg[17],pArg[18],pArg[19],pArg[20],pArg[21],pArg[22],pArg[23],pArg[24],pArg[25],pArg[26],pArg[27],pArg[28],pArg[29],pArg[30],pArg[31],pArg[32],pArg[33],pArg[34],pArg[35],pArg[36],pArg[37],pArg[38],pArg[39],pArg[40],pArg[41],pArg[42],pArg[43],pArg[44],pArg[45],pArg[46],pArg[47],pArg[48],pArg[49],pArg[50],pArg[51],pArg[52],pArg[53],pArg[54],pArg[55],pArg[56],pArg[57],pArg[58],pArg[59],pArg[60]); break;
	case 62: ((C62)pfn)(&cbd, DrvCallBack, pArg[0],pArg[1],pArg[2],pArg[3],pArg[4],pArg[5],pArg[6],pArg[7],pArg[8],pArg[9],pArg[10],pArg[11],pArg[12],pArg[13],pArg[14],pArg[15],pArg[16],pArg[17],pArg[18],pArg[19],pArg[20],pArg[21],pArg[22],pArg[23],pArg[24],pArg[25],pArg[26],pArg[27],pArg[28],pArg[29],pArg[30],pArg[31],pArg[32],pArg[33],pArg[34],pArg[35],pArg[36],pArg[37],pArg[38],pArg[39],pArg[40],pArg[41],pArg[42],pArg[43],pArg[44],pArg[45],pArg[46],pArg[47],pArg[48],pArg[49],pArg[50],pArg[51],pArg[52],pArg[53],pArg[54],pArg[55],pArg[56],pArg[57],pArg[58],pArg[59],pArg[60],pArg[61]); break;
	case 63: ((C63)pfn)(&cbd, DrvCallBack, pArg[0],pArg[1],pArg[2],pArg[3],pArg[4],pArg[5],pArg[6],pArg[7],pArg[8],pArg[9],pArg[10],pArg[11],pArg[12],pArg[13],pArg[14],pArg[15],pArg[16],pArg[17],pArg[18],pArg[19],pArg[20],pArg[21],pArg[22],pArg[23],pArg[24],pArg[25],pArg[26],pArg[27],pArg[28],pArg[29],pArg[30],pArg[31],pArg[32],pArg[33],pArg[34],pArg[35],pArg[36],pArg[37],pArg[38],pArg[39],pArg[40],pArg[41],pArg[42],pArg[43],pArg[44],pArg[45],pArg[46],pArg[47],pArg[48],pArg[49],pArg[50],pArg[51],pArg[52],pArg[53],pArg[54],pArg[55],pArg[56],pArg[57],pArg[58],pArg[59],pArg[60],pArg[61],pArg[62]); break;
	case 64: ((C64)pfn)(&cbd, DrvCallBack, pArg[0],pArg[1],pArg[2],pArg[3],pArg[4],pArg[5],pArg[6],pArg[7],pArg[8],pArg[9],pArg[10],pArg[11],pArg[12],pArg[13],pArg[14],pArg[15],pArg[16],pArg[17],pArg[18],pArg[19],pArg[20],pArg[21],pArg[22],pArg[23],pArg[24],pArg[25],pArg[26],pArg[27],pArg[28],pArg[29],pArg[30],pArg[31],pArg[32],pArg[33],pArg[34],pArg[35],pArg[36],pArg[37],pArg[38],pArg[39],pArg[40],pArg[41],pArg[42],pArg[43],pArg[44],pArg[45],pArg[46],pArg[47],pArg[48],pArg[49],pArg[50],pArg[51],pArg[52],pArg[53],pArg[54],pArg[55],pArg[56],pArg[57],pArg[58],pArg[59],pArg[60],pArg[61],pArg[62],pArg[63]); break;
	default : break;
	}

}

void iComponent::UnCache()
{
	GListIterator it( &m_lstMethods );
	while (it())
	{
		iMethod *p = (iMethod *)it++;
		delete p;
	}
	m_lstMethods.RemoveAll();
}

int iComponent::GetRequiredParamCount(const char *pzMethod)
{
	GString strFindMethod(pzMethod);
	GListIterator it( GetMethods() );
	while (it())
	{
		iMethod *p = (iMethod *)it++;
		if (strFindMethod.CompareNoCase( p->GetName() ) == 0)
		{
			return p->GetParams()->Size();
		}
	}
	
	throw GException("CStdCallIntegration", 2,pzMethod);

}

iMethod *iComponent::AddMethod(const char *pzMethodName, const char *pzMethodRetType)
{
	iMethod *pRet = new iMethod(pzMethodName, pzMethodRetType);
	m_lstMethods.AddLast(pRet);
	return pRet;
}

int iComponent::IsMethodAvailable(const char *pzMethod)
{
	GString strFindMethod(pzMethod);
	GListIterator it( GetMethods() );
	while (it())
	{
		iMethod *p = (iMethod *)it++;
		if (strFindMethod.CompareNoCase( p->GetName() ) == 0)
		{
			return 1;
		}
	}
	return 0;;
}


typedef char *(*ExposedMethods)(void);
void CStdCallInterface::InitTypeInfo(const char *pzDynamicLib)
{
	if (m_strLastLoadedTypeLib.CompareNoCase(pzDynamicLib) == 0)
		return;
	
	m_iC.UnCache();
	m_strLastLoadedTypeLib = pzDynamicLib;
	
	const char *pzPathedDynamicLib = FullPathDynamicLib(pzDynamicLib);
	GString strErrorMessage;
	if (!m_dllHandle)
		m_dllHandle = _OPENLIB(pzPathedDynamicLib);

	if (m_dllHandle)
	{
		void *pfn = (void *)_PROCADDRESS(m_dllHandle, "ExposedMethods");
		if (pfn)
		{
			char *pzData = ((ExposedMethods)pfn)();
			m_iC.LoadTypeLib(pzData);
		}
		else
		{
			throw GException("CStdCallIntegration", 6,(const char *)pzPathedDynamicLib);
		}
		if (m_bUnload)
		{
			_CLOSELIB(m_dllHandle);
			m_dllHandle = 0;
		}
	}
	else
	{
#ifndef _WIN32
		// UNIX environments only
		throw GException("CStdCallIntegration", 7,dlerror(),(const char *)pzPathedDynamicLib);
#else
		// NT could not load the DLL.
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

		throw GException("CStdCallIntegration", 4,(const char *)pzDynamicLib,dwError,(lpMsgBuf) ? lpMsgBuf : "");
#endif
	}
}


void iComponent::LoadTypeLib(const char *pzRawTypeInfo)
{
// Data comes in the following format.
//		"SayHello&varName&char *!"
//		"OtherFunction&Var1&char *&Var2&char *!";
	GStringList listOfLists("!",pzRawTypeInfo);
	GStringIterator it(&listOfLists);
	while(it())
	{
		GStringList listRow("&",it++);
		GStringIterator itR(&listRow);
		if (itR())
		{
			const char *pzMethod = itR++;
			iMethod *iM = AddMethod(pzMethod,"char *");
			while (itR())
			{
				const char *pzVarName = itR++;
				const char *pzVarType = 0;
				if(itR())
				{
					pzVarType = itR++;
				}
				iM->AddParam(pzVarName,pzVarType);
			}
		}
	}
}


