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

#ifndef _XML_GENERIC_LIST_ABSTRACTION
#define _XML_GENERIC_LIST_ABSTRACTION


#include "ListAbstraction.h"
#include "GList.h"
#include "GArray.h"
#include "GString.h"
#include "GStringList.h"
#include "GHash.h"
#include "GBTree.h"
#include "StringAbstraction.h"
#include "GString.h"



// there is only 1 instance of this class.
class GenericStringAbstract : public StringAbstraction
{
public:
	int isEmpty(userString pString)
	{
		return (((GString *)pString)->IsEmpty());
	}
	__int64 length(userString pString)
	{
		return ((GString *)pString)->Length();
	}
	void assign(userString pString, const char *pzValue)
	{
		*((GString *)pString) = pzValue;
	}
	void append(userString pString, const char *pzValue)
	{
		*((GString *)pString) += pzValue;
	}
	const char *data(userString pString)
	{
		return (const char *)*((GString *)pString);
	}
};




//
// ****  You need only one ListAbstraction instance per list type.
// The ListAbstraction class is stateless so a single global 
// instance can handle all your lists without any thread safety 
// concerns.  
class GenericListAbstraction : public ListAbstraction
{
public:
	void create(xmlObjectList pList)
	{
		// if create() is called, it is called 'sometime' after List 
		// construction and before the List Object's destructor.

		// if the List has never had an object added to it by
		// the XMLObjectFactory, create() will NOT be called.

		// create() may be called more than once during a List's duration.

		// If the List was filled by an application or process 
		// other than XMLObjectFactory, and contains NOT a single
		// instance of an object created by the XMLObjectFactory
		// create() will NOT be called, unless ModifyObjectBehavior()
		// has been called to guarantee the call.
		
		GList *pTypedList = (GList *)pList;
		pTypedList->DeferDestruction();


	}
	void destroy(xmlObjectList pList)
	{
		// destroy() is called a short while AFTER the Lists destructor.  uh huh.
		// destroy() is only called once.	

		GList *pTypedList = (GList *)pList;

		pTypedList->Destruction();

		// the object memory has not been deleted yet, 
		// but the destructor has already been executed
		// and the memory should be released soon.

	}
	
	__int64 itemCount(xmlObjectList pList)
	{
		GList *pTypedList = (GList *)pList;
		return pTypedList->Size();
	}
	void append(xmlObjectList pList, XMLObject *pObject)
	{
		create(pList);
		GList *pTypedList = (GList *)pList;
		pTypedList->AddLast(pObject);
	}
	
	XMLObject *getLast(xmlObjectList pList)
	{
		GList *pTypedList = (GList *)pList;
		return (XMLObject *)pTypedList->Last();

	}
	
	void removeObject(xmlObjectList pList, xmlObjectIterator Iterator)
	{
		GList *pTypedList = (GList *)pList;
		GListIterator *pIter = (GListIterator *)Iterator;
		pTypedList->SetCurrent(pIter->iCurrentNode);
		pTypedList->RemoveCurrent();
	}

	XMLObject *getFirst(xmlObjectList pList, xmlObjectIterator *pIterator)
	{
		GList *pTypedList = (GList *)pList;
		XMLObject *pObject = 0;
		if ( pTypedList->Size() )
		{
			GListIterator *pIter = new GListIterator(pTypedList);
			*pIterator = pIter;
			if ( (*pIter)() )
			{
				pObject = (XMLObject *)((*pIter)++);
			}
			else
			{
				delete pIter;
				pIter = 0;
			}
		}
		return pObject;
	}
	XMLObject *getNext(xmlObjectList /*pList*/, xmlObjectIterator Iterator)
	{
		GListIterator *pIter = (GListIterator *)Iterator;
		XMLObject *pObject = 0;
		if ( (*pIter)() )
		{
			pObject = (XMLObject *)((*pIter)++);
		}
		else
		{
			delete pIter;
			pIter = 0;
		}
		return pObject;
	}
	void releaseIterator(xmlObjectIterator Iterator)
	{
		delete (GListIterator *)Iterator;
		Iterator = 0;
	}
};

class GHashAbstraction : public KeyedDataStructureAbstraction
{
public:
	XMLObject *search(KeyedDataStructure kds, const char *pzKey)
	{
		GHash *pTyped = (GHash *)kds;
		return (XMLObject *)pTyped->Lookup(pzKey);
	}
	void destroy(KeyedDataStructure kds)
	{
		GHash *pTyped = (GHash *)kds;
		pTyped->Destruction();
	};
	void create(KeyedDataStructure kds)
	{
		GHash *pTyped = (GHash *)kds;
		pTyped->DeferDestruction();
	};
	void insert(KeyedDataStructure kds, XMLObject *pObj, const char *pzKey)
	{
		GHash *pTyped = (GHash *)kds;
		pTyped->DeferDestruction();
		pTyped->Insert(pzKey, pObj);
	}
	XMLObject *getFirst(KeyedDataStructure kds, userHashIterator *pIterator)
	{
		XMLObject *pRet = 0;
		GHash *pTyped = (GHash *)kds;
		if ( pTyped->GetCount() )
		{
			GHashIterator *pIter = new GHashIterator(pTyped);
			*pIterator = pIter;
			pRet = (XMLObject *)(*pIter)++;
			if (!pRet)
			{
				delete pIter;
				pIter = 0;
			}
		}
		return pRet;
	}
	XMLObject *getNext(KeyedDataStructure /*kds*/, userHashIterator Iterator)
	{
		GHashIterator *pIter = (GHashIterator *)Iterator;

		XMLObject *pRet = (XMLObject *)(*pIter)++;
		if (!pRet)
		{
			delete pIter;
			pIter = 0;
		}
		return pRet;
	}
	void remove(KeyedDataStructure kds, userHashIterator Iterator, const char *pzKey, XMLObject *pObj)
	{
		GHashIterator *pIter = (GHashIterator *)Iterator;
		GHash *pTyped = (GHash *)kds;
		
		(*pIter)(); // advances the internal integrator position before we delete the current node
		pTyped->reassignKeyPair(pzKey,pObj, (void *)-1);
	
	}
	__int64 itemCount(KeyedDataStructure kds)
	{
		const GHash *pTyped = (GHash *)kds;
		return pTyped->GetCount();
	}
	void releaseIterator(xmlObjectIterator Iterator)
	{
		delete (GHashIterator *)Iterator;
		Iterator = 0;
	}
};



class GStringListAbstraction : public StringCollectionAbstraction
{
public:
	void append(userStringList pList, const char *pString)
	{
		GStringList *pTypedList = (GStringList *)pList;
		pTypedList->AddLast(pString);
	}
	const char *getFirst(xmlObjectList pList, xmlObjectIterator *pIterator)
	{
		GStringList *pTypedList = (GStringList *)pList;
		const char *pzRetVal = 0;
		if ( pTypedList->Size() )
		{
			GStringIterator *pIter = new GStringIterator(pTypedList);
			*pIterator = pIter;

			pzRetVal = ((*pIter)++);
			
		}
		return pzRetVal;
	}
	const char *getNext(xmlObjectList/*pList*/, xmlObjectIterator Iterator)
	{
		GStringIterator *pIter = (GStringIterator *)Iterator;
		const char *pzReturnValue = 0;
		if ( (*pIter)() )
		{
			pzReturnValue = ((*pIter)++);
		}
		else
		{
			delete pIter;
			pIter = 0;
		}
		return pzReturnValue;
	}
	__int64 itemCount(userStringList pList)
	{
		GStringList *pTypedList = (GStringList *)pList;
		return pTypedList->Size();
	}
};


class GArrayAbstraction : public IntegerArrayAbstraction
{
public:	
	void append(userArray pArray, __int64 nValue)
	{
		GArray *pTypedArray = (GArray *)pArray;
		pTypedArray->AddElement( (void *)nValue );
	}
	__int64 getAt(userArray pArray, __int64 nIndex, int *bIsValidIndex)
	{
		GArray *pTypedArray = (GArray *)pArray;
		if (nIndex < pTypedArray->GetItemCount())
		{
			*bIsValidIndex = 1;
			union CAST_THIS_TYPE_SAFE_COMPILERS
			{
				void *      mbrVoid;
				__int64		mbrInt;
			}Member;
			Member.mbrInt = XMLF_LONG_ZERO;

			Member.mbrVoid = (*pTypedArray)[nIndex];
			return Member.mbrInt;
		}
		*bIsValidIndex = 0;
		return 0;
	}
	__int64 itemCount(userArray pArray)
	{
		GArray *pTypedArray = (GArray *)pArray;
		return pTypedArray->GetItemCount();
	}
};


class GQSortAbstraction : public KeyedDataStructureAbstraction
{
public:
	XMLObject *search(KeyedDataStructure kds, const char *pzKey)
	{
		GArray *pTyped = (GArray *)kds;
		return (XMLObject *)pTyped->Lookup(pzKey);
	}
	void destroy(KeyedDataStructure kds)
	{
		GArray *pTyped = (GArray *)kds;
		pTyped->Destruction();
	};
	void create(KeyedDataStructure kds)
	{
		GArray *pTyped = (GArray *)kds;
		pTyped->DeferDestruction();
	};
	void remove(KeyedDataStructure kds, userHashIterator Iterator, const char *pzKey, XMLObject *pObj)
	{
		GArray *pTyped = (GArray *)kds;
		unsigned int *nIndex = (unsigned int *)Iterator;
		pTyped->Assign(*nIndex, (void *)-1, "");
	}
	void insert(KeyedDataStructure kds, XMLObject *pObj, const char *pzKey)
	{
		create(kds);
		GArray *pTyped = (GArray *)kds;
		pTyped->AddElement (pObj,pzKey);
	}
	XMLObject *getFirst(KeyedDataStructure kds, userHashIterator *pIterator)
	{
		GArray *pTyped = (GArray *)kds;
		*pIterator = new __int64;
		__int64 *nIndex = (__int64 *)*pIterator;
		*nIndex = 0;
		

		__int64 nItemCount = pTyped->GetItemCount();
		if (*nIndex < nItemCount)
		{
			// iterate past nodes marked as removed with an object pointer of 0xffffffff
			while( (*nIndex < nItemCount-1) && ( ((*pTyped)[*nIndex]) == (void *)-1) ) { (*nIndex)++;}

			XMLObject *pO =  (XMLObject *)(*pTyped)[*nIndex];
			if ( pO  &&  (pO != (void *)-1)  )
				return pO;
		}
		delete nIndex;
		nIndex = 0;
		return 0;
	}
	XMLObject *getNext(KeyedDataStructure kds, userHashIterator Iterator)
	{
		__int64 *nIndex = (__int64 *)Iterator;
		GArray *pTyped = (GArray *)kds;
		(*nIndex)++;

		if (*nIndex < pTyped->GetItemCount())
		{
			// iterate past nodes marked as removed
			while( (*nIndex < pTyped->GetItemCount()) && ( ((*pTyped)[*nIndex]) == (void *)-1) ) {(*nIndex)++;}

			XMLObject *pO =  (XMLObject *)(*pTyped)[*nIndex];
			if (pO)
				return pO;
		}
		delete nIndex;
		nIndex = XMLF_LONG_ZERO;
		return 0;
	}
	__int64 itemCount(KeyedDataStructure kds)
	{
		GArray *pTyped = (GArray *)kds;
		return pTyped->GetItemCount();
	}
	void releaseIterator(xmlObjectIterator Iterator)
	{
		delete (int *)Iterator;
		Iterator = 0;
	}
};


class GBTreeAbstraction : public KeyedDataStructureAbstraction
{
public:
	XMLObject *search(KeyedDataStructure kds, const char *pzKey)
	{
		GBTree *pTyped = (GBTree *)kds;
		return (XMLObject *)pTyped->search(pzKey);
	}
	void destroy(KeyedDataStructure kds)
	{
		GBTree *pTyped = (GBTree *)kds;
		pTyped->Destruction();
	};
	void create(KeyedDataStructure kds)
	{
		GBTree *pTyped = (GBTree *)kds;
		pTyped->DeferDestruction();
	};
	void insert(KeyedDataStructure kds, XMLObject *pObj, const char *pzKey)
	{
		create(kds);
		GBTree *pTyped = (GBTree *)kds;
		pTyped->insert (pzKey,  pObj);
	}
	void remove(KeyedDataStructure kds, userHashIterator, const char *pzKey, XMLObject *pObj)
	{
		GBTree *pTyped = (GBTree *)kds;
		pTyped->reassignKeyPair(pzKey, pObj, (void *)-1);
	}
	
	XMLObject *getFirst(KeyedDataStructure kds, userHashIterator *pIterator)
	{
		GBTreeIterator *pIter = new GBTreeIterator((GBTree *)kds, 2);
		*pIterator = pIter;		
		XMLObject *pRet = (XMLObject *)((*pIter)++);
		if (!pRet)
		{
			delete pIter;
			pIter = 0;
		}
		return pRet;
	}

	XMLObject *getNext(KeyedDataStructure/* kds*/, userHashIterator Iterator)
	{
		GBTreeIterator *pIter = (GBTreeIterator *)Iterator;
		XMLObject *pRet = (XMLObject *)((*pIter)++);
		if (!pRet)
		{
			delete (GBTreeIterator *)Iterator;
			Iterator = 0;
		}
		return pRet;
	}
	__int64 itemCount(KeyedDataStructure kds)
	{
		GBTree *pTyped = (GBTree *)kds;
		return pTyped->getNodeCount();
	}
	void releaseIterator(xmlObjectIterator Iterator)
	{
		delete (GBTreeIterator *)Iterator;
		Iterator = 0;
	}
};

#endif //_XML_GENERIC_LIST_ABSTRACTION
