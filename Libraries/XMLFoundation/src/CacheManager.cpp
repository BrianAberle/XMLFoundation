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

#include "GHash.h"
#include "GString.h"
#include "GStringList.h"
#include "CacheManager.h"
#include "FactoryManager.h"
#include "xmlObject.h"
#include "FrameworkAuditLog.h"

#include "DynamicXMLObject.h"
int DynamicXMLObject::nInstanceCount = 0;




XMLObjectCache::XMLObjectCache()
{
	m_cache = new GHash();
	m_cacheState = new GHash();
	m_cacheForeign = new GHash();
	m_cacheForeignAlternate = new GHash();
	m_cacheData = new GHash();
	_gthread_processInitialize();
	XML_INIT_MUTEX(&m_cs);
	gthread_mutex_init(&m_csState, NULL);
	gthread_mutex_init(&m_csForeign, NULL);
	gthread_mutex_init(&m_csData, NULL);
}

XMLObjectCache::~XMLObjectCache()
{
	try
	{
		// Note: You can remove the following 6 lines of code if you have you have 
		// a situation that leaves an invalid object reference in the cash.  Free'ing this
		// memory is non-essential because the process is ending if we are executing this code.
		// Therefore, any memory access exceptions on those references can be safely ignored.
		dumpCache();
		dumpStateCache();
		dumpForeignCache();
		delete m_cache;
		delete m_cacheState;
		delete m_cacheForeign;
	}
	catch (...)
	{
		// continue normal processing after the first chance exception of an Access Violation
	}

	delete m_cacheForeignAlternate;
	delete m_cacheData;
	m_cache = 0;
	m_cacheState = 0;
	m_cacheForeign = 0;
//	gthread_mutex_destroy(&m_cs);
	XML_DESTROY_MUTEX(&m_cs);

	gthread_mutex_destroy(&m_csState);
	gthread_mutex_destroy(&m_csForeign);
	gthread_mutex_destroy(&m_csData);
	
	_gthread_processTerminate();
}

// findObject() returns an object from the cache or null if the 
// object was not found in the cache.
XMLObject *XMLObjectCache::findObject( const char* hashKey )
{
//	gthread_mutex_lock(&m_cs);
	XML_LOCK_MUTEX(&m_cs);

	XMLObject* pObject = 0;
	pObject = (XMLObject *)m_cache->Lookup(hashKey);
	//gthread_mutex_unlock(&m_cs);
	XML_UNLOCK_MUTEX(&m_cs);

	return pObject;
}


// findObject() returns an object from the cache or null if the 
// object was not found in the cache.
XMLObject *XMLObjectCache::findObject(const char *oid, 
									  const char *pzClassName )
{
	GString strKey;
	strKey << oid << pzClassName;
	return findObject( strKey );
}


void XMLObjectCache::dumpCache()
{
	TRACE_MESSAGE("****** Begin Object Instance Cache Dump ******");
//	gthread_mutex_lock(&m_cs);
	XML_LOCK_MUTEX(&m_cs);

	GHashIterator iter(m_cache);
	while (iter())
	{
		XMLObject *pObj = (XMLObject *)iter++;
		GString str;
		str.Format("Object In Cache - %s[%s](%s)RefCount=%d\n", pObj->GetObjectType(), pObj->GetObjectTag(), pObj->getOID(), pObj->GetRefCount());
		TRACE_MESSAGE((const char *)str);
	}
//	gthread_mutex_unlock(&m_cs);
	XML_UNLOCK_MUTEX(&m_cs);

	TRACE_MESSAGE("****** End Object Instance Cache Dump ******");
}



void XMLObjectCache::cacheStatus(GStringList *pDest)
{
//	gthread_mutex_lock(&m_cs);
	XML_LOCK_MUTEX(&m_cs);


	GHashIterator iter(m_cache);
	
	GString strCacheEntry;
	while (iter())
	{
		XMLObject* pObject = (XMLObject*)iter++;
		if (pObject)
		{
			strCacheEntry = pObject->GetObjectType();
			strCacheEntry += "  ";
			strCacheEntry += pObject->GetObjectTag();
			strCacheEntry += "  RefCount=";
			strCacheEntry += pObject->GetRefCount();
			strCacheEntry += "  ";
			strCacheEntry += pObject->getOID();
			strCacheEntry += "\n";
		}
		pDest->AddLast((const char *)strCacheEntry);
	}
//	gthread_mutex_unlock(&m_cs);
	XML_UNLOCK_MUTEX(&m_cs);

}




void XMLObjectCache::emptyCache()
{
//	gthread_mutex_lock(&m_cs);
	XML_LOCK_MUTEX(&m_cs);


	GHashIterator iter(m_cache);
	while (iter())
	{
		try
		{
			delete (XMLObject *)iter++;
		}
		catch(...)
		{
			TRACE_ERROR("Reference counting error.\n");
		}
	}

//	gthread_mutex_unlock(&m_cs);
	XML_UNLOCK_MUTEX(&m_cs);

}


void XMLObjectCache::enterObject(const char *oid, const char *pzClassName, XMLObject *pObject )
{
	if ( !(pObject->GetObjectBehaviorFlags() & PREVENT_AUTO_CACHE) )
	{
		GString strKey;
		strKey << oid << pzClassName;
//		gthread_mutex_lock(&m_cs);
		XML_LOCK_MUTEX(&m_cs);

		m_cache->Insert((const char *)strKey, pObject);
//		gthread_mutex_unlock(&m_cs);
		XML_UNLOCK_MUTEX(&m_cs);

	}
}


XMLObject *XMLObjectCache::findObjectByTag(const char *pzXMLTag, const char* oid)
{
	XMLObject* pObject = 0;
//	gthread_mutex_lock(&m_cs);
	XML_LOCK_MUTEX(&m_cs);

	XMLObjectFactoryManager &rFM = XMLObjectFactoryManager::getFactoryManager();
	const char *pzClassName = rFM.GetSDKClassName(pzXMLTag);
	if (pzClassName && oid && oid[0])
	{
		GString strKey;
		strKey << oid << pzClassName;
		pObject = (XMLObject*)m_cache->Lookup((const char *)strKey);
	}
//	gthread_mutex_unlock(&m_cs);
	XML_UNLOCK_MUTEX(&m_cs);

	if (pObject)
		pObject->IncRef();
	return pObject;
}

XMLObject *XMLObjectCache::findObjectByType(const char *pzClassName, const char* oid)
{
	XMLObject* pObject = 0;
//	gthread_mutex_lock(&m_cs);
	XML_LOCK_MUTEX(&m_cs);

	if (pzClassName && pzClassName[0] && oid && oid[0])
	{
		GString strKey;
		strKey << oid << pzClassName;
		pObject = (XMLObject*)m_cache->Lookup((const char *)strKey);
	}
//	gthread_mutex_unlock(&m_cs);
	XML_UNLOCK_MUTEX(&m_cs);

	if (pObject)
		pObject->IncRef();
	return pObject;
}


// getObject() will always return an object for the given criteria
// If the object is found in the cache, a reference to the cached 
// instance is returned, otherwise a new instance is created.

// The oid is generally not known yet,[in the case of objects using MapObjectID()].
// For high performance implementations to avert building a temp object, the OID
// will be known prior to creation allowing the Factory to "Attach" to a cached object.
XMLObject *XMLObjectCache::getObject(const char *pzXMLTag, const char* oid, 
									 const char *pzUpdateTime,int *bDidCreate /*= 0*/ )
{
	// Synchronized with releaseObject
//	gthread_mutex_lock(&m_cs);
	XML_LOCK_MUTEX(&m_cs);

	XMLObject* pObject = 0;
		

	// There should always be a class name, but if the application pgmr
	// forgot to call REG_FACTORY() it will be null.
	XMLObjectFactoryManager &rFM = XMLObjectFactoryManager::getFactoryManager();
	const char *pzClassName = rFM.GetSDKClassName(pzXMLTag);
	if (pzClassName)
	{
		if ( oid && oid[0] )
		{
			GString strKey;
			strKey << oid << pzClassName;
			pObject = (XMLObject*)m_cache->Lookup((const char *)strKey);
			if (pObject)
			{
				// set flag indicating that the object was in the cache.
				if (bDidCreate)
					*bDidCreate = 0;
			}
		}
		if ( !pObject )
		{
			// set flag indicating that the object was not in the cache, a new one was added
			if (bDidCreate)
				*bDidCreate = 1;

			// create a new object 
			pObject = (*(rFM.GetFactory(pzXMLTag)))();

			if (oid && oid[0])
			{
				pObject->Init( oid, pzUpdateTime );
				if ( !(pObject->GetObjectBehaviorFlags() & PREVENT_AUTO_CACHE) )
				{
					GString strKey;
					strKey << oid << pzClassName;
					m_cache->Insert((const char *)strKey, pObject);
				}
			}
		}
		else
		{
			// GString strTrace;
			// strTrace.Format("Attaching to cached Instance of <%s> [ %s ]", pzClassName, oid);
			// TRACE_MESSAGE ((const char *)strTrace);
		}
	}

//	gthread_mutex_unlock(&m_cs);
	XML_UNLOCK_MUTEX(&m_cs);


	// return the object - caller is responsible
	// for calling AddRef() on this object (if not NULL)!
	// 
	return pObject; 
}




// Removes the obect from the cache.
// Called by XMLObjects whose ref count has reached zero.
//
bool XMLObjectCache::releaseObject(const char* className,const char* oid)
{
	if (!m_cache)
		return false;
	if (!m_cache->GetCount())
		return false;

	// Synchronized with getObject
//	gthread_mutex_lock(&m_cs);
	XML_LOCK_MUTEX(&m_cs);


	bool bSuccess = false;

	if (className && oid)
	{
		GString strKey;
		strKey << oid << className;
		
		// take the object out of the cache, but don't delete it yet. 
		bSuccess = ( m_cache->RemoveKey((const char *)strKey) != 0 );
	}

//	gthread_mutex_unlock(&m_cs);
	XML_UNLOCK_MUTEX(&m_cs);


	return bSuccess;
}




///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
/////////////////////   Object State Cache  ///////////////////////////
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////


GString *XMLObjectCache::getState(const char *pzXMLTag, const char* oid)
{
	// Synchronized with releaseState
	gthread_mutex_lock(&m_csState);

	GString* pObject = 0;

	if (oid && oid[0])
	{
		XMLObjectFactoryManager &rFM = XMLObjectFactoryManager::getFactoryManager();
		const char *pzClassName = rFM.GetSDKClassName(pzXMLTag);
		if (pzClassName)
		{
			if ( oid && oid[0] )
			{
				GString strKey;
				strKey << oid << pzClassName;
				pObject = (GString*)m_cacheState->Lookup((const char *)strKey);
			}
		}
	}

	gthread_mutex_unlock(&m_csState);
	return pObject; 
}



void XMLObjectCache::enterState(const char *oid, const char *pzClassName, const char *pState )
{
	releaseState(pzClassName,oid);
	GString strKey;
	strKey << oid << pzClassName;
	gthread_mutex_lock(&m_csState);
	m_cacheState->Insert((const char *)strKey, (void *)new GString(pState));
	gthread_mutex_unlock(&m_csState);
}

// remove a state entry from the cache
bool XMLObjectCache::releaseState(const char* className,const char* oid)
{
	// Synchronized with getState
	gthread_mutex_lock(&m_csState);

	bool bSuccess = false;

	if (className && oid)
	{
		GString strKey;
		strKey << oid << className;
		
		// take the object out of the cache, but don't delete it yet. 
		bSuccess = ( m_cacheState->RemoveKey((const char *)strKey) != 0 );
	}

	gthread_mutex_unlock(&m_csState);

	return bSuccess;
}

void XMLObjectCache::emptyStateCache()
{
	gthread_mutex_lock(&m_csState);
	GHashIterator iter(m_cacheState);
	while (iter())
	{
		delete (GString *)iter++;
	}
	gthread_mutex_unlock(&m_csState);
}


GString *XMLObjectCache::findState( const char* hashKey )
{
	gthread_mutex_lock(&m_csState);
	GString* pObject = 0;
	pObject = (GString *)m_cacheState->Lookup(hashKey);
	gthread_mutex_unlock(&m_csState);
	return pObject;
}

// findState() returns an object's state from the cache or null if the 
// state was not found. 
GString *XMLObjectCache::findState( const char* oid, const char *pzClassName )
{
	GString strKey;
	strKey << oid << pzClassName;
	return findState( strKey );
}

void XMLObjectCache::dumpStateCache()
{
	TRACE_MESSAGE("****** Begin Object State Cache Dump ******");
	gthread_mutex_lock(&m_csState);
	GHashIterator iter(m_cacheState);
	while (iter())
	{
		GString *pObj = (GString *)iter++;
		GString str;
		str.Format("State In Cache - %s", pObj->StrVal());
		TRACE_MESSAGE((const char *)str);
	}
	gthread_mutex_unlock(&m_csState);
	TRACE_MESSAGE("****** End Object State Cache Dump ******");
}
void XMLObjectCache::stateCacheStatus(GStringList *pDest)
{
	gthread_mutex_lock(&m_csState);

	GHashIterator iter(m_cacheState);
	
	GString strCacheEntry;
	while (iter())
	{
		GString* pObject = (GString*)iter++;
		if (pObject)
		{
			strCacheEntry += pObject->StrVal();
			strCacheEntry += "\n";
		}
		pDest->AddLast((const char *)strCacheEntry);
	}
	gthread_mutex_unlock(&m_csState);
}





///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
/////////////////////   Foreign Object Cache  /////////////////////////
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
void XMLObjectCache::enterForeign( __int64 nTempKey, DynamicXMLObject *pO )
{
	GString strKey;
	strKey << nTempKey;

	gthread_mutex_lock(&m_csForeign);
	m_cacheForeign->Insert((const char *)strKey, pO);
	gthread_mutex_unlock(&m_csForeign);

}

void XMLObjectCache::enterForeign( DynamicXMLObject *pO )
{
	GString strKey;
	strKey << (__int64)(void *)pO;

	gthread_mutex_lock(&m_csForeign);
	m_cacheForeign->Insert((const char *)strKey, pO);
	gthread_mutex_unlock(&m_csForeign);

}

DynamicXMLObject *XMLObjectCache::DXOFromNative( __int64 ptr )
{
	DynamicXMLObject *pReturn;
	GString strKey;
	strKey << ptr;
	
	gthread_mutex_lock(&m_csForeign);
	pReturn = (DynamicXMLObject *)m_cacheForeignAlternate->Lookup(strKey);
	gthread_mutex_unlock(&m_csForeign);

	return pReturn;
}

DynamicXMLObject *XMLObjectCache::isForeign( DynamicXMLObject *pO )
{
	DynamicXMLObject *pReturn = 0;

	GString strKey;
	strKey << (__int64)(void *)pO;

	gthread_mutex_lock(&m_csForeign);
	pReturn = (DynamicXMLObject *)m_cacheForeign->Lookup(strKey);
	gthread_mutex_unlock(&m_csForeign);
	return pReturn;
}


bool XMLObjectCache::tempToAlternate( __int64 nKey )
{
	GString strKey;
	strKey << nKey;


	bool bSuccess = false;

	gthread_mutex_lock(&m_csForeign);
	
	DynamicXMLObject *pDXO = (DynamicXMLObject *)m_cacheForeign->Lookup(strKey);
	
	if (pDXO->getUserLanguageObject())
	{
		GString strAltKey;
		strAltKey << (__int64)(void *)pDXO->getUserLanguageObject();

		m_cacheForeignAlternate->Insert((const char *)strAltKey,pDXO);
	}

	bSuccess = ( m_cacheForeign->RemoveKey((const char *)strKey) != 0 );
	gthread_mutex_unlock(&m_csForeign);

	return bSuccess;
}

void XMLObjectCache::addAlternate( DynamicXMLObject *pDXO )
{
	if (pDXO->getUserLanguageObject())
	{
		GString strAltKey;
		strAltKey << (__int64)(void *)pDXO->getUserLanguageObject();

		gthread_mutex_lock(&m_csForeign);
		m_cacheForeignAlternate->Insert((const char *)strAltKey,pDXO);
		gthread_mutex_unlock(&m_csForeign);
		
		enterForeign( pDXO );
	}
}


bool XMLObjectCache::releaseForeign( __int64 nUserObj )
{
	GString strAltKey;
	strAltKey << nUserObj;
	gthread_mutex_lock(&m_csForeign);
	m_cacheForeignAlternate->RemoveKey((const char *)strAltKey);
	gthread_mutex_unlock(&m_csForeign);
	return true;
}

bool XMLObjectCache::releaseForeign( DynamicXMLObject *pO )
{
	GString strKey;
	strKey << (__int64)(void *)pO;

	bool bSuccess = false;

	gthread_mutex_lock(&m_csForeign);

	bSuccess = ( m_cacheForeign->RemoveKey((const char *)strKey) != 0 );
	void *pLangObject = pO->getUserLanguageObject();
	if (pLangObject)
	{
		if (pLangObject != (void *)-1)
		{
			GString strAltKey;
			strAltKey << (__int64)pLangObject;
			m_cacheForeignAlternate->RemoveKey((const char *)strAltKey);
		}
	}

	gthread_mutex_unlock(&m_csForeign);

	return bSuccess;
}

void XMLObjectCache::emptyForeignCache()
{
	GHashIterator iter(m_cacheForeign);
	while (iter())
	{
		delete (DynamicXMLObject*)iter++;
	}
}
void XMLObjectCache::dumpForeignCache()
{
	TRACE_MESSAGE("****** Begin Foreign Object Instance Cache Dump ******");
	gthread_mutex_lock(&m_csForeign);

	
	GHashIterator iter2(m_cacheForeignAlternate);
	while (iter2())
	{
		DynamicXMLObject *pObj = (DynamicXMLObject *)iter2++;
		GString str;
		str.Format("Object In Alt-Index Cache - %s[%s]\n", pObj->GetObjectType(), pObj->GetObjectTag() );
		TRACE_MESSAGE((const char *)str);
	}
	
	TRACE_MESSAGE("-----------------------------------------------\n");
	
	
	GHashIterator iter(m_cacheForeign);
	while (iter())
	{
		DynamicXMLObject *pObj = (DynamicXMLObject *)iter++;
		GString str;
		str << "Object In Cache - " << (__int64)(void *)pObj;
		TRACE_MESSAGE((const char *)str);
	}

	gthread_mutex_unlock(&m_csForeign);
	TRACE_MESSAGE("****** End Foreign Object Instance Cache Dump ******");
}

void XMLObjectCache::foreignCacheStatus(GStringList *pDest)
{
	gthread_mutex_lock(&m_csForeign);

	GHashIterator iter(m_cacheForeign);
	
	GString strCacheEntry;
	while (iter())
	{
		DynamicXMLObject* pObject = (DynamicXMLObject*)iter++;
		if (pObject)
		{
			strCacheEntry += pObject->GetObjectType();
			strCacheEntry += "  ";
			strCacheEntry += pObject->GetObjectTag();
			strCacheEntry += "\n";
		}
		pDest->AddLast((const char *)strCacheEntry);
	}
	gthread_mutex_unlock(&m_csForeign);
}


// ****************************************************************
// Applies to the Data cache (web pages, images, or other data)
// ****************************************************************
void XMLObjectCache::EnterCacheData(long length, void *pData, const char *pzKey)
{
	gthread_mutex_lock(&m_csData);
	m_cacheData->Insert(pzKey, pData);
	GString strKey;
	strKey << (__int64)pData;
	m_cacheData->Insert((const char *)strKey, (void *)length);
	gthread_mutex_unlock(&m_csData);
}
void *XMLObjectCache::UncacheData(const char* pzKey)
{
	gthread_mutex_lock(&m_csData);
	bool bSuccess = 0;
	void *pData = m_cacheData->Lookup(pzKey);
	if (pData)
	{
		bSuccess =  ( m_cacheData->RemoveKey(pzKey) != 0 );
	}
	gthread_mutex_unlock(&m_csData);
	return pData;
}
void *XMLObjectCache::GetCacheData(const char *pzKey, long *length)
{
	void *pRet;
	gthread_mutex_lock(&m_csData);
	pRet = m_cacheData->Lookup(pzKey);
	if (pRet && length)
	{
		GString strKey;
		strKey << (__int64)pRet;

		*length = (long)m_cacheData->Lookup((const char *)strKey);
	}
	gthread_mutex_unlock(&m_csData);
	return pRet;
}



void XMLObjectCache::EmptyDataCache()
{
	gthread_mutex_lock(&m_csData);
	m_cacheData->RemoveAll();
	gthread_mutex_unlock(&m_csData);
}


XMLObjectCache cacheManager;