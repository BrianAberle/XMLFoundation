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

#include "MemberDescriptor.h"
#include "FactoryManager.h"
#include "FrameworkAuditLog.h"
#include "xmlObject.h"
#include "GProfile.h"
#include <stdio.h>  // for: sprintf()
#include <string.h> // for: strlen() strcpy()

#ifdef _WIN32
	#define strcasecmp	_stricmp
#endif


int XMLObjectFactoryManager::m_UIDCounter = 0;


XMLObjectFactoryManager::XMLObjectFactoryManager() : m_factoryList(32,32), m_IFacefactoryList(32,32)
{
}

XMLObjectFactoryManager::~XMLObjectFactoryManager()
{
	__int64 nCount = m_factoryList.GetItemCount();
	for( __int64 nArrayIndex=0; nArrayIndex<nCount; nArrayIndex++ )
	{
		MemberDescriptor *pMD = (MemberDescriptor *) m_factoryList.GetAt(nArrayIndex);
		delete[] (char *)((pMD->m_Member).pClassName);
		// specify the global delete 
		::delete pMD;
	}
}


//*********************************************************    int nArraySize = GetMemberMapCount(0);
// Description:
//		called by macro REG_FACTORY to register this object and it's factory
void XMLObjectFactoryManager::Add( ObjectFactory pFactory, const char * szTag, const char * szClassName)
{
	MemberDescriptor *pMD = GetRootableMapEntry(szTag); 
	if ( pMD ) 
	{
		char szTemp[128];
		sprintf(szTemp, "Object <%s> and <%s> both map to tag <%s>.  <%s> takes precesedence.", 
			(const char *)pMD->m_Member.pClassName, szClassName, szTag, (const char *)pMD->m_Member.pClassName);
		if ( GetProfile().GetBool("Debug", "MultiMapWarnings", false))
		{
			TRACE_WARNING(szTemp);
		}
	}
	else
	{
		char *pHeapedClassName = new char[strlen(szClassName) + 1];
		strcpy(pHeapedClassName,szClassName);
		m_factoryList.AddElement(::new MemberDescriptor(0, szTag,pFactory,pHeapedClassName));
	}
}


// called by macro IMP_BOL_RELATION to register this interface
void XMLObjectFactoryManager::Add(	
							IFaceFactory Iff, 
							ObjectFactory Of, 
							const char *iFaceClassName , 
							const char *BusObjClassName)
{
	structIFaceCreate *sIFC = new structIFaceCreate();
	sIFC->Iff = Iff;
	sIFC->Of = Of;
	sIFC->strBusObjClassName = BusObjClassName;
	sIFC->strIFaceObjClassName = iFaceClassName;
	m_IFacefactoryList.AddElement(sIFC);

}

XMLObjectFactoryManager::structIFaceCreate *XMLObjectFactoryManager::FindIFaceInfo( const char *pzBOLClassName )
{
	__int64 nCount = m_IFacefactoryList.GetItemCount();
	for( __int64 nArrayIndex=0; nArrayIndex < nCount; nArrayIndex++ )
	{
		structIFaceCreate *pIFC = (structIFaceCreate *)m_IFacefactoryList.GetAt(nArrayIndex);
		if( strcmp(pzBOLClassName,(const char *)pIFC->strBusObjClassName) == 0 )
		{
			return pIFC; // we found a match
		}
	}
	return 0; // no match
}

	

//*********************************************************
// Description:
//		Check all the objects registered with REG_FACTORY using tag name as search key
// 
// Preconditions:
//		.
// 
// Postconditions:
//		. 
// 
//*********************************************************
MemberDescriptor *XMLObjectFactoryManager::GetRootableMapEntry(const char * pzTag)
{
	__int64 nCount = m_factoryList.GetItemCount();
	for( __int64 nArrayIndex=0; nArrayIndex<nCount; nArrayIndex++ )
	{
		MemberDescriptor *pMD = (MemberDescriptor *) m_factoryList.GetAt(nArrayIndex);
		if( strcasecmp((const char *)pMD->strTagName, pzTag) == 0 )
		{
			return pMD; // we found a match
		}
	}
	return 0; // No Root element matches pzTag
}

ObjectFactory XMLObjectFactoryManager::GetFactory(const char * pzTag)
{
	__int64 nCount = m_factoryList.GetItemCount();
	for( __int64 nArrayIndex=0; nArrayIndex<nCount; nArrayIndex++ )
	{
		MemberDescriptor *pMD = (MemberDescriptor *) m_factoryList.GetAt(nArrayIndex);
		if( strcasecmp((const char *)pMD->strTagName, pzTag) == 0 )
		{
			return pMD->m_pfnFactoryCreate; // we found a match
		}
	}
	return 0; // No Root element matches pzTag
}


const char *XMLObjectFactoryManager::GetSDKClassName(const char * pzTag)
{
	__int64 nCount = m_factoryList.GetItemCount();
	for( __int64 nArrayIndex=0; nArrayIndex<nCount; nArrayIndex++ )
	{
		MemberDescriptor *pMD = (MemberDescriptor *) m_factoryList.GetAt(nArrayIndex);
		if( strcasecmp((const char *)pMD->strTagName, pzTag) == 0 )
		{
			return pMD->m_Member.pClassName; // we found a match
		}
	}
	return 0; // No Root element matches pzTag
}


//*********************************************************
// Description:
//		Check all the objects registered with REG_FACTORY 
//		using factory function as search key
// 
// Preconditions:
//		.
// 
// Postconditions:
//		. 
// 
//*********************************************************
MemberDescriptor *XMLObjectFactoryManager::GetRootableMapEntry(ObjectFactory pFn)
{
	__int64 nCount = m_factoryList.GetItemCount();
	for( __int64 nArrayIndex=0; nArrayIndex<nCount; nArrayIndex++ )
	{
		MemberDescriptor *pMD = (MemberDescriptor *) m_factoryList.GetAt(nArrayIndex);
		if( pMD->m_pfnFactoryCreate == pFn )
		{
			return pMD; // we found a match
		}
	}
	return 0; // No Root element matches pFn
}

const char *XMLObjectFactoryManager::GetTag(ObjectFactory pFn)
{
	__int64 nCount = m_factoryList.GetItemCount();
	for( __int64 nArrayIndex=0; nArrayIndex<nCount; nArrayIndex++ )
	{
		MemberDescriptor *pMD = (MemberDescriptor *) m_factoryList.GetAt(nArrayIndex);
		if( pMD->m_pfnFactoryCreate == pFn )
		{
			return (const char *)pMD->strTagName; // we found a match
		}
	}
	return 0; // No Root element matches pFn

}


RegisterBusinessObject::RegisterBusinessObject(	ObjectFactory Of, 
												const char * szXMLTagIdentifier, 
												const char * szBusObjClassName)
{
	XMLObjectFactoryManager &r = XMLObjectFactoryManager::getFactoryManager();
	r.Add( Of, szXMLTagIdentifier, szBusObjClassName);
}

RegisterInterface::RegisterInterface(	IFaceFactory If,
										ObjectFactory Of, 
										const char * szIFaceClassName, 
										const char * szBusObjClassName)
{
	XMLObjectFactoryManager &r = XMLObjectFactoryManager::getFactoryManager();
	r.Add(	If,
			Of,
			szIFaceClassName,
			szBusObjClassName);	
}

XMLObjectFactoryManager &XMLObjectFactoryManager::getFactoryManager()
{
	static XMLObjectFactoryManager factoryManager;
	return factoryManager;
}

