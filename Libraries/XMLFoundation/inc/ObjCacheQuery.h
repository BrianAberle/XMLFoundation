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

#ifndef _OBJ_CACHE_UPDATE_H_
#define _OBJ_CACHE_UPDATE_H_

#include "ProcedureCall.h"

template<class TYPE> class  ObjCacheQuery 
								: virtual public XMLProcedureCall,
								  virtual public GList
{
protected:
	long		m_requestedCount;
	GString m_strTagId;

	// Object factory to be used when creating TYPE objects.  If 
	// m_factory is null when MapXMLTagsToMembers() is called then 
	// the factory will be retrieved from the Factory Manager and 
	// the new object will be put into the Cache Manager.
	ObjectFactory m_factory;
public:

	virtual ~ObjCacheQuery(){}

	void Execute()
	{
		XMLProcedureCall::Execute();
	}

	// called by the XMLProcedureCall when generating XML 
	// request message in segmented bulk fetching implementations.
	virtual long GetRowCount()	{ return m_requestedCount;}

	ObjCacheQuery(	CXMLDataSource *pSource,
					const char * pzStoredProcedureName, 
					long nMaxObjectsReturnedPerWANCall = (long)-1
				  );


	virtual void MapXMLTagsToMembers();
};

// inline template implementation 
template<class TYPE> ObjCacheQuery<TYPE>::
ObjCacheQuery(	CXMLDataSource *pSource,
				const char * pzStoredProcedureName, 
				long nMaxObjectsReturnedPerWANCall
			  )
{
	m_factory = 0;
	SetProcedureName( pzStoredProcedureName );
	ModifyObjectBehavior(NO_MEMBER_MAP_PRECOUNT);
	SetDataSource(pSource);
	m_strTagId << TYPE::GetStaticTag() << '\0';
	m_requestedCount = nMaxObjectsReturnedPerWANCall;
}



// no longer needs to be templatized to the base.
template<class TYPE> void ObjCacheQuery<TYPE>::
MapXMLTagsToMembers()
{
	// map our members if we are recieving results from the Procedure call previously executed
	MapMember( (GList *)this, (const char *)m_strTagId, 0, m_factory  );
	SetMemberSerialize((const char *)m_strTagId, false);
	XMLProcedureCall::MapXMLTagsToMembers();
}

#endif //_OBJ_CACHE_UPDATE_H_

