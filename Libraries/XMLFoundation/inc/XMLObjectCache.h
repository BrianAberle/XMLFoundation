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


// An object may be cached in one of two forms if the object has called
// MapObjectID() in the MapXMLTagsToMembers() method.

// ***************************** m_cache ***********************************
// An inter-related cache of Object/Interface instances.  That is, the COM or 
// CORBA object and it's state. Allows a stateful app (sometimes a server, 
// always a client) to correctly inter-relate objects.  If a 'Customer' 
// relates to an address object and an 'Order' relates to the same 'Address' 
// object, we only need  one instance of that object with a RefCount of 2.  
// Both the 'Customer' and the 'Order' can point to the same 'Address' if 
// that 'Address' has an OID defined by MapObjectID().

// ***************************** m_cacheState********************************
// Alternatively Objects may be cached in the most basic form, that is a buffer
// of XML with no 'object' instance or Interface(CORBA or COM) since all that
// can be created on demand when necessary.

// Currently, for optimal performance no cache synchronization is enforced.
// You could cache the state of a CORBA/COM Object then never destroy your
// interface object.  This would cause the same object to exist in both 
// cached forms, and most likely cause you a memory leak.  Don't do that.

// ***************************** m_cacheForeign *****************************
// non-native objects, like Java Objects use a TIE approach that associates
// state to the actual handle of the jobject, VBobject, or any other
// interpreter/byte code executor object handle.  This solution handles all
// languages since the handle type is cast down to void, providing java & VB
// programmers a pure java/VB coding experience.


#ifndef _CACHE_MGR_
#define _CACHE_MGR_

#include "GThread.h"

typedef int (*lpForeignCallback)(void *pObj);

#include "xmlDefines.h"

class XMLObject;
class GHash;
class GString;
class GStringList;
class DynamicXMLObject;

class XMLObjectCache
{
	GHash *m_cache;			// cache of object instances
	GHash *m_cacheState;	// cache of XML object state
	GHash *m_cacheForeign;	
	GHash *m_cacheForeignAlternate;
	GHash *m_cacheData;	
//	gthread_mutex_t m_cs;
	XML_MUTEX m_cs;

	gthread_mutex_t m_csState;
	gthread_mutex_t m_csForeign;
	gthread_mutex_t m_csData;
public:
	XMLObjectCache();
	~XMLObjectCache();

	// ****************************************************************
	// Applies to the Object Instance cache
	// ****************************************************************

	// getObject() will always return an object for the given criteria
	// If the object is found in the cache, a reference to the cached 
	// instance is returned, otherwise a new instance is created, placed
	// in the cache then returned. This allows access to an object when 
	// the data type is unknown. 
	XMLObject *getObject(const char *pzXMLTag, const char* oid, 
						 const char *pzUpdateTime, int *bDidCreate = 0);

	void enterObject(const char *oid, const char *pzClassName, XMLObject *pO );
	
	// findObjectByType() and findObjectByTag() returns an object from the cache 
	// or null if the object was not found. YOU MUST CALL DecRef() when you are 
	// finished using the object returned by either of these methods
	XMLObject *findObjectByType(const char *pzClassName, const char* oid);
	XMLObject *findObjectByTag(const char *pzXMLTag, const char* oid);


	// findObject() returns an object from the cache or null if the object was not found. 
	// findObject() Does not affect the Ref Count. This is used by XMLObjectFactory and XMLDataServer.
	// findObject() is NOT recommended for use in the application layer because the ref count 
	// is not incremented and therefore the scope of duration is undefined
	XMLObject *findObject( const char* oid, const char *ClassName );
	XMLObject *findObject( const char* hashKey );

	virtual bool releaseObject(const char* className,const char* oid);
	void emptyCache();
	void dumpCache();
	void cacheStatus(GStringList *pDest);

	// ****************************************************************
	// Applies to the State cache
	// ****************************************************************
	GString *getState(const char *pzXMLTag, const char* oid);
	void enterState(const char *oid, const char *pzClassName, const char *pState );

	// findState() returns an object's state from the cache or null if the 
	// state was not found. 
	GString *findState( const char* oid, const char *ClassName );
	GString *findState( const char* hashKey );

	// remove a state entry from the cache
	virtual bool releaseState(const char* className,const char* oid);
	void emptyStateCache();
	void dumpStateCache();
	void stateCacheStatus(GStringList *pDest);

	// ****************************************************************
	// Applies to the Foreign Object Instance cache
	// ****************************************************************
	void enterForeign( DynamicXMLObject *pO );
	void enterForeign( __int64 nTempKey, DynamicXMLObject *pO );

	// remove a state entry from the cache
	DynamicXMLObject *isForeign( DynamicXMLObject *pO );
	DynamicXMLObject *DXOFromNative( __int64 ptr );
	bool releaseForeign( DynamicXMLObject *pO );
	bool releaseForeign( __int64 l );
	bool tempToAlternate( __int64 l );
	void addAlternate( DynamicXMLObject *pO );
	void emptyForeignCache();
	void dumpForeignCache();
	void foreignCacheStatus(GStringList *pDest);

	// ****************************************************************
	// Applies to the Data cache (web pages, images, or other data)
	// ****************************************************************
	void EnterCacheData(long length, void *pData, const char *pzKey);
	void *GetCacheData(const char *pzKey, long *length = 0 );
	void *UncacheData(const char *pzKey);
	void EmptyDataCache();


};


extern XMLObjectCache cacheManager;


#endif //_CACHE_MGR_
