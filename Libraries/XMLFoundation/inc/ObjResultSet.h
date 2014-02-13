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
#ifndef __OBJ_RESULT_H__
#define __OBJ_RESULT_H__

#include "GList.h"

template <class TYPE> class ObjQuery;

// Private inheritance
template <class TYPE> class ObjResultSet : private GListIterator
{
	// Our only friend, has the power to create an instance of ObjResultSet
	// Our constructor is private, our friend will create this on the heap.
	friend class ObjQuery<TYPE>;
	
	// Count of TYPE objects, this iterator has returned
	int m_nReturned;  

	// The m_pCreatorQuery that generated the results we are iterating
	ObjQuery<TYPE> *m_pCreatorQuery;

	// Private destructor for reassurance of our trusted friend creator
	~ObjResultSet()	{ }

	// 'ctor for ObjQuery.  Our mutual friend.
	ObjResultSet( ObjQuery<TYPE>* s ) :
		GListIterator( s ) 
	{ 
		m_nReturned = 0;
		m_pCreatorQuery = s;
	}

	// Automatically issued by the ObjQuery when the scope of existence has 
	// been reached. All objects end here in destruction() unless a 
	// call to freedom() or AddRef() has been requested before scope is met.
	void destruction()
	{
		GListIterator innerResults( m_pCreatorQuery );
		while (innerResults())
			((TYPE *)innerResults++)->DecRef();
	}

public:
	// Get rid of the resultset container, but keep the objects
	void freedom()
	{
		// write in the memory of our creator that we have been freed.
		m_pCreatorQuery->m_bHasBeenFreed = true;

		// With trust in our creator, destruction is not fatal because
		// we know we're on the heap. freedom() releases us from our 
		// own cleanup responsibility, and we bypass the second and 
		// final destruction() in ~ObjQuery.
		delete this;
	}

	bool hasMoreToFetch()
	{
		 return m_pCreatorQuery->hasMoreUnfetchedResults();
	}

	// Sets the number of objects that should be retrieved per WAN call.
	// Any value < 0, causes next() to return NULL when it has reached
	// the end of the current block of objects.
	void setNextBulkFetchCount( int nValue )
	{
		m_pCreatorQuery->SetReturnRowCount ( nValue );
	}

	// Get the next object out of the result set.  Every Nth call to
	// next() will cause a WAN call back to the database. 
	TYPE*	next()
	{
		// if we need to fetch the next chunk, do so before calling 
		// the base class operator ()
		TYPE *pRet = 0;
		if ( ( m_pCreatorQuery->IsUsingChunkFetching())			&&
			 ( m_nReturned % m_pCreatorQuery->GetRowCount() == 0 ) &&  
			 ( m_nReturned > 0 )
		   ) 
			 
		{
			// fetch the next chunk of data from the database
			m_pCreatorQuery->Execute();
		}
		if ( GListIterator::operator()() )
		{
			pRet = (TYPE *)GListIterator::operator++(0);
		}
		m_nReturned++;
		return pRet;
	}
};

#endif //__OBJ_RESULT_H__
