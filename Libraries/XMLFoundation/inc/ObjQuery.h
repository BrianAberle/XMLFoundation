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
#ifndef _OBJ_QUERY_H_
#define _OBJ_QUERY_H_

#include "ProcedureCall.h"
#include "ObjCacheQuery.h"

template <class TYPE> class ObjResultSet;
template<class TYPE> class  ObjQuery : public ObjCacheQuery<TYPE>
{
	// Flag that causes the ObjResultSet to Re-Execute this query 
	// to get the remaining results from the TXML Server.
	bool m_bUseChunkFetching;
	int m_requestedCount;

	// mutual friendship to the ObjResultSet
	friend class ObjResultSet<TYPE>;
	
	ObjResultSet<TYPE> *m_pResultSet;

	// Flag that indicates if our friend has requested freedom() of self
	int m_bHasBeenFreed;

	const char *getResultObjectTag()
	{
		return TYPE::GetStaticTag();
	}
protected:

	// because friendship is not inherited, this allows our derivatives
	// to use the friendship priveledges of this class.  You MUST call 
	// freedom() on the object returned to release it.
	ObjResultSet<TYPE> &MakeNewResultSet()
	{
		return *( new ObjResultSet<TYPE>( this ) );
	}
public:
	// 'this' must not go out of scope to keep the iterator(ObjResultSet) valid.
	virtual ~ObjQuery()
	{
		if (!m_bHasBeenFreed && m_pResultSet )
		{
			m_pResultSet->destruction();
			
			// PREVENT_AUTO_DESTRUCTION added 3/7/2008, because our base class XMLProcedureCall under 
			// the ObjCacheQuery was decrementing the reference count on the result set objects.
			// 2014 Note: Search for "This was bad.   Fixed 12/21/2013" in ProcedureCall.cpp.
			// That fix likely eliminates the need for this - need to research
			XMLObject *pThis = (XMLObject *)this;
			pThis->ModifyObjectBehavior(PREVENT_AUTO_DESTRUCTION,0); 

			delete m_pResultSet;
		}
	}

	ObjResultSet<TYPE> &Execute()
	{
		XMLProcedureCall::Execute();
		m_pResultSet = new ObjResultSet<TYPE>( this );
		return *m_pResultSet;
	}

	// Overrides the nMaxObjectsReturnedPerWANCall parameter from construction.
	// The ObjResultSet may use this method to alternate WAN chunk sizes.
	void SetReturnRowCount(int n);

	bool IsUsingChunkFetching()	{ return m_bUseChunkFetching; };


	ObjQuery(	CXMLDataSource *pSource,
				const char * pzStoredProcedureName 
		    );
};


template<class TYPE> ObjQuery<TYPE>::
ObjQuery( CXMLDataSource *pSource,  const char * pzStoredProcedureName ) : 
  ObjCacheQuery<TYPE>(pSource, pzStoredProcedureName, -1)
{  
	m_pResultSet = 0;
	m_bHasBeenFreed = 0;
	m_bUseChunkFetching = false;
}


template<class TYPE> void ObjQuery<TYPE>::
SetReturnRowCount(int n) 
{
	if (n > 0)
	{
		m_requestedCount = n;
	}
	else
	{
		m_bUseChunkFetching = false;
	}
}




#endif //_OBJ_QUERY_H_

