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
#ifndef _LIST_ABSTRACTION_RWTPTR_SLIST
#define _LIST_ABSTRACTION_RWTPTR_SLIST

#include "ListAbstraction.h"

#include "StringAbstraction.h"

class RWStringAbstract : public StringAbstraction
{
	int isEmpty(userString pString)
	{
		return ((RWCString *)pString)->isNull();
	}
	__int64 length(userString pString)
	{
		return ((RWCString *)pString)->length();
	}
	void assign(userString pString, const char *pzValue)
	{
		*((RWCString *)pString) = pzValue;
	}
	void append(userString pString, const char *pzValue)
	{
		*((RWCString *)pString) += pzValue;
	}
	const char *data(userString pString)
	{
		return ((RWCString *)pString)->data();
	}
};



// This class implements the ListAbstraction for an RWTPtrSlist.
// Your high level objects may contain lists of other objects.
// For example an Order has a list of Order-Items.  The actual
// type of list can be anything you choose, but the 
// xmlObjectFramework needs to be able to append new objects
// to your list and iterate your list to serialize your objects 
// to XML.  This implementation of a ListAbstraction is provided
// to the virtual MapXMLTagsToMembers() method when you map the 
// list.   
// ****  You need only one ListAbstraction instance per list type.
// The ListAbstraction class is stateless so a single global 
// instance can handle all your lists without any thread safety 
// concerns.  
template <class TYPE> class RWList : public  ListAbstraction
{
public:
	void removeObject(xmlObjectList pList, xmlObjectIterator Iterator)
	{
	}
	void releaseIterator(xmlObjectIterator Iterator)
	{
		delete (RWTPtrSlistIterator<TYPE> *)Iterator;
	}
	__int64 itemCount(void *pList)
	{
		return ((RWTPtrSlist<TYPE> *)pList)->entries();
	}
	void append(void *pList, void *pObject)
	{
		RWTPtrSlist<TYPE> *pTypedList = (RWTPtrSlist<TYPE> *)pList;
		pTypedList->append( (TYPE *)pObject );
	}
	XMLObject *getLast(xmlObjectList pList)
	{
		RWTPtrSlist<TYPE> *pTypedList = (RWTPtrSlist<TYPE> *)pList;
		return (TYPE *)pTypedList->last();
	}
	XMLObject *getFirst(void *pList, xmlObjectIterator *ppIterator)
	{
		RWTPtrSlist<TYPE> *pTypedList = (RWTPtrSlist<TYPE> *)pList;
		TYPE *pObject = 0;
		if (pTypedList->entries())
		{
			RWTPtrSlistIterator<TYPE> *pIter = new RWTPtrSlistIterator<TYPE>(*pTypedList);
			*ppIterator = pIter;
			pObject = (*pIter)();
		}
		return pObject;
	}
	XMLObject *getNext(void *, xmlObjectIterator ppIterator)
	{
		RWTPtrSlistIterator<TYPE> *pIter = (RWTPtrSlistIterator<TYPE> *)ppIterator;
		TYPE *pObject = (*pIter)();
		if (!pObject)
		{
			delete pIter;
		}
		return pObject;
	}
};


#endif //_LIST_ABSTRACTION_RWTPTR_SLIST