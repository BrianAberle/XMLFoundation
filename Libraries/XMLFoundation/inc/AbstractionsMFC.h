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

#ifndef _XML_MFC_ABSTRACTIONS
#define _XML_MFC_ABSTRACTIONS


#include "StringAbstraction.h"


// To use MFC Strings in your objects, include this header file from your StdAfx.h file, 
// then add the following line of code to stdafx.cpp or any one of your project source files:
//
// CStringAbstraction gC;

// likewise for other MFC data types:
// CStringListAbstraction gL;
// CDWordArrayAbstraction gD;
// CListAbstraction<CMyXMLObject> gMyO;

//
//
// In your implementation of MapXMLTagsToMembers() you can now use 
// this single handler for all your MFC CStrings like this:
//
//	MapMember("FirstName", m_strFirstName, &gMFCStrHandler)
//
class CStringAbstraction;
extern CStringAbstraction gC;


class CStringAbstraction : public StringAbstraction
{
	int isEmpty(userString pString)
	{
		return ((CString *)pString)->IsEmpty();
	}
	__int64 length(userString pString)
	{
		return ((CString *)pString)->GetLength();
	}
	void assign(userString pString, const char *pzValue)
	{
		*((CString *)pString) = pzValue;
		((CString *)pString)->TrimRight();
	}
	void append(userString pString, const char *pzValue)
	{
		*((CString *)pString) += pzValue;
		((CString *)pString)->TrimRight();
	}
	const char *data(userString pString)
	{
		return  *((CString *)pString);
	}
};



//
// This class implements the ListAbstraction for an CPtrList.
// Your high level objects may contain lists of other objects.
// For example an Order has a list of Order-Items.  The actual
// type of list can be anything you choose, but the 
// XMLFoundation needs to be able to append new objects
// to your list and iterate your list to serialize your objects 
// to XML.  This implementation of a ListAbstraction is provided
// to the virtual MapXMLTagsToMembers() method when you map the 
// list.   
// ****  You need only one ListAbstraction instance per list type.
// The ListAbstraction class is stateless so a single global 
// instance can handle all your lists without any thread safety 
// concerns.  




#include "ListAbstraction.h"
#include "GList.h"

// Note: The template TYPE is necessary, to ensure the pointer is widened to 
// include the CORBA or COM interface if the object is multiply derived from 
// Component Object support.
template <class TYPE> class CListAbstraction : public ListAbstraction
{
public:
	__int64 itemCount(xmlObjectList pList)
	{
		CPtrList *pTypedList = (CPtrList *)pList;
		return pTypedList->GetCount();
	}
	void append(xmlObjectList pList, XMLObject *pObject)
	{
		CPtrList *pTypedList = (CPtrList *)pList;
		pTypedList->AddTail(pObject);
	}
	XMLObject *getLast(xmlObjectList pList)
	{
		CPtrList *pTypedList = (CPtrList *)pList;
		if (pTypedList->GetCount())
			return (TYPE*)pTypedList->GetTail();
		return 0;
	}
	XMLObject *getFirst(xmlObjectList pList, xmlObjectIterator *pIterator)
	{
		CPtrList *pTypedList = (CPtrList *)pList;
		TYPE *pObject = 0;
		if ( pTypedList->GetCount() )
		{
			POSITION *pIter = new POSITION;
			*pIter = pTypedList->GetHeadPosition();
			*pIterator = pIter;
			pObject = (TYPE *)(pTypedList->GetNext(*pIter));
			if (!pObject)
			{
				delete pIter;
				pIter = 0;
			}
		}
		return pObject;
	}
	XMLObject *getNext(xmlObjectList pList, xmlObjectIterator Iterator)
	{
		POSITION *pIter = (POSITION *)Iterator;
		CPtrList *pTypedList = (CPtrList *)pList;
		TYPE *pObject = 0;
		if ( *pIter )
		{
			pObject = (TYPE *)pTypedList->GetNext(*pIter);
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
		delete (POSITION *)Iterator;
		Iterator = 0;
	}
	void removeObject(xmlObjectList pList, xmlObjectIterator Iterator)
	{
	}
	void destroy(xmlObjectList pList)
	{
		// destroy() is called a short while AFTER the Lists destructor.  uh huh.  It is called after the ~dtor()
		// destroy() is only called once.
		// the object memory has not been deleted yet, so the call still executes in safe instruction space.
	}
	void create(xmlObjectList pList)
	{
		// depending on list usage create() may not be called.
		// if create() is called, it is called after List construction, after the `ctor is called.
		// The call is guaranteed to happen prior the ~dtor(), and destroy()

		// If the List has never had an object added to it by the XMLObjectFactory, create() will NOT be called and 
		// the list can behave normally.

		// create() may be called more than once during a List's duration, implementation must determine if this 
		// is a duplicate call - ( a potential side effect from caching and object pooling )

		// If the List contains items added directly by an application programmer, NOT by the XMLObjectFactory
		// and the list does not conatin a single instance of an object created by the XMLObjectFactory
		// create() will NOT be called, unless ModifyObjectBehavior() has been called to guarantee the call.

		// Here the code must instruct the List NOT to free the nodes during destruction in the ~dtor()
		// the work normally done in the ~dtor(), will be done in destroy().

		// IF THIS IS NOT IMPLEMENTED - Everything works fine except reference counting and automatic object 
		// destruction.  Using this list handler you must clean up yourself or cache infinitely.  Both are 
		// acceptable solutions in certain cases.
	}
};

#include <afxcoll.h> // necessary for CStringList definition


class CStringListAbstraction : public StringCollectionAbstraction
{
public:
	void append(userStringList pList, const char *pString)
	{
		CStringList *pTypedList = (CStringList *)pList;
		pTypedList->AddTail( pString );
	}
	const char *getFirst(userStringList pList, userStringListIterator *pIterator)
	{
		CStringList *pTypedList = (CStringList *)pList;

		const char *pzRetVal = 0;
		if ( pTypedList->GetCount() )
		{
			POSITION *pos = new POSITION;
			*pos = pTypedList->GetHeadPosition();
			pzRetVal = pTypedList->GetNext( *pos );
			*pIterator = pos;
		}
		return pzRetVal;
	}
	const char *getNext(userStringList pList, userStringListIterator Iterator)
	{
		CStringList *pTypedList = (CStringList *)pList;
		POSITION *pos = (POSITION *)Iterator;

		if(pos && *pos == 0)
		{
			delete pos;
			pos = 0;
			return 0;
		}
		return pTypedList->GetNext( *pos );
	}
	__int64 itemCount(userStringList pList)
	{
		CStringList *pTypedList = (CStringList *)pList;
		return pTypedList->GetCount();
	}
};

#include <afxtempl.h> // necesasary for CArray


// This class shows the infinite extendibility of the StringCollectionAbstraction
// which also serves as the more obvious base class to C and G StringList implementations
// such as GStringListAbstraction, CStringListAbstraction.  Here you can see that the
// concept of a collection of XML strings, is just as easily mapped to list or array
// of any datatype that the string data is representing.  Here StringCollectionAbstraction
// is the ideal abstraction interface to store an array of doubles in an MFC CArray.
// This class is used in MFCTypesFromXMLDlg.cpp in the samples directory.
// This implementation serves as a template to copy for any integer iterated array or vector of any type.
class CDoubleArrayAbstraction : public StringCollectionAbstraction
{
	// use one instance of 'this' per array that you map to if you will 
	// access the mapped array simultaneously from multiple threads through this
	// abstraction because access to g_strConversion is neither in a Mutex nor a 
	// Critical Section or implement your own that is.  The C and G stringlist
	// implementations do not have this issue because they return a direct pointer
	// to the data they contain that does not need any conversion, alternately you
	// could keep a list of these temporary conversion storage spaces so that 
	// no Mutex or Critical Section is necessary - the tradeoff being performance
	// or memory use.  This simple implementation will work perfectly in most cases.
	GString g_strConversion; 

public:
	void append(userStringList pList, const char *Value)
	{
		// pairs to this code in your implementation class declaration:
		//		CArray<double,double> m_arrDbl;
		// and to this in the MapXMLTagsToMembers() where gAA is an instance of CDoubleArrayAbstraction
		//                         xml tag			   // optional outer xml tag
		//	MapMember(&m_arrDbl,  "Double",	  &gAA,	  "CArrayOfDoubles");
		//
		CArray<double,double> *pTypedArray = (CArray<double,double> *)pList;

		// convert string Value to double then add it to the array
		pTypedArray->Add( atof( Value ) ); 
	}
	const char *getFirst(userStringList pList, userStringListIterator *pIterator)
	{
		CArray<double,double> *pTypedArray = (CArray<double,double> *)pList;
		if ( pTypedArray->GetSize() )
		{
			// create and return a new iterator, an int * that gets passed into getNext()
			int *pIter = new int;  
			*pIter = 0;
			*pIterator = pIter;
			g_strConversion = "";
			g_strConversion << (*pTypedArray)[*pIter];
			(*pIter)++;
		}
		else
		{
			return 0;
		}
		return g_strConversion;
	}
	const char *getNext(userStringList pList, userStringListIterator Iterator)
	{
		CArray<double,double> *pTypedArray = (CArray<double,double> *)pList;
		int *pIter = (int *)Iterator;

		if ( pTypedArray->GetSize() > *pIter)
		{
			g_strConversion = "";
			g_strConversion << (*pTypedArray)[*pIter];
			(*pIter)++;
			
			return g_strConversion;
		}
		else
		{
			// automatic delete of iterator after end of collection is reached
			delete pIter;
			pIter = 0;
		}
		return 0;
	}
	__int64 itemCount(userStringList pList)
	{
		CArray<double,double> *pTypedArray = (CArray<double,double> *)pList;
		return pTypedArray->GetSize();
	}
};



class CDWordArrayAbstraction : public IntegerArrayAbstraction
{
public:	
	void append(userArray pArray, __int64 nValue)
	{
		CDWordArray *pTypedArray = (CDWordArray *)pArray;
		pTypedArray->Add( (DWORD)nValue );
	}
	__int64 getAt(userArray pArray, __int64 nIndex, int *bIsValidIndex)
	{
		CDWordArray *pTypedArray = (CDWordArray *)pArray;
		if (nIndex < (unsigned int)pTypedArray->GetSize())
		{
			*bIsValidIndex = 1;
			return (*pTypedArray)[(int)nIndex];
		}
		*bIsValidIndex = 0;
		return 0;
	}
	__int64 itemCount(userArray pArray)
	{
		CDWordArray *pTypedArray = (CDWordArray *)pArray;
		return pTypedArray->GetSize();
	}
};



template <class TYPE> class CPtrArrayAbstraction : public ListAbstraction
{
public:
	__int64 itemCount(xmlObjectList pList)
	{
		CPtrArray *pTypedList = (CPtrArray *) pList;
		return pTypedList->GetSize();
	}

	void append(xmlObjectList pList, XMLObject *pObject)
	{
		CPtrArray *pTypedList = (CPtrArray *) pList;
		pTypedList->Add(pObject);
	}

	void removeObject(xmlObjectList pList, xmlObjectIterator Iterator)
	{
		CPtrArray *pTypedList = (CPtrArray *) pList;
		int *pIter = (int *)Iterator;

		if (*pIter < pTypedList->GetSize())
		{
			pTypedList->RemoveAt(*pIter);
		}
	}

	XMLObject *getLast(xmlObjectList pList)
	{
		CPtrArray *pTypedList = (CPtrArray *) pList;
		TYPE *pObject = NULL;

		INT_PTR nIndex = pTypedList->GetSize();
		if( nIndex > 0)
		{
			nIndex--;
			pObject = (TYPE *)(pTypedList->ElementAt(nIndex));
		}

		return pObject;
	}

	XMLObject *getFirst(xmlObjectList pList, xmlObjectIterator *pIterator)
	{
		CPtrArray *pTypedList = (CPtrArray *) pList;
		TYPE *pObject = NULL;

		if ( pTypedList->GetSize() > 0)
		{
			pObject = (TYPE *)(pTypedList->ElementAt(0));
			if(pObject)
			{
				int *pIter = new int;
				*pIter = 1;
				*pIterator = pIter;
			}
		}

		return pObject;
	}

	XMLObject *getNext(xmlObjectList pList, xmlObjectIterator Iterator)
	{
		CPtrArray *pTypedList = (CPtrArray *) pList;
		TYPE *pObject = NULL;

		int *pIter = (int *)Iterator;
		if (*pIter < pTypedList->GetSize())
		{
			pObject = (TYPE *)pTypedList->ElementAt(*pIter);
			*pIter = *pIter + 1;
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
		delete (int *)Iterator;
		Iterator = 0;
	}
	void create(xmlObjectList pList)
	{
		// Not implemented
	}

	void destroy(xmlObjectList pList)
	{
		// Not implemented
	}
};


template <class TYPE, class ARG_TYPE = const TYPE&> class CArrayAbstraction : public ListAbstraction
{
public:

	__int64 itemCount(xmlObjectList pList)
	{
		CArray<TYPE, ARG_TYPE> *pTypedList = (CArray<TYPE, ARG_TYPE> *) pList;
		return pTypedList->GetSize();
	}
	void append(xmlObjectList pList, XMLObject *pObject)
	{
		CArray<TYPE, ARG_TYPE> *pTypedList = (CArray<TYPE, ARG_TYPE> *) pList;
		pTypedList->Add((TYPE) pObject);
	}
	XMLObject *getLast(xmlObjectList pList)
	{
		CArray<TYPE, ARG_TYPE> *pTypedList = (CArray<TYPE, ARG_TYPE> *) pList;
		TYPE *pObject = NULL;

		if(pTypedList->GetSize() > 0)
		{
			pObject = (TYPE *)(pTypedList->ElementAt(pTypedList->GetSize()-1));
		}

		return (TYPE)pObject;
	}
	XMLObject *getFirst(xmlObjectList pList, xmlObjectIterator *pIterator)
	{
		CArray<TYPE, ARG_TYPE> *pTypedList = (CArray<TYPE, ARG_TYPE> *) pList;
		TYPE *pObject = NULL;

		if ( pTypedList->GetSize() > 0)
		{
			pObject = (TYPE *)(pTypedList->ElementAt(0));
			if(pObject)
			{
				int *pIter = new int;
				*pIter = 1;
				*pIterator = pIter;
			}
		}

		return (TYPE)pObject;
	}
	XMLObject *getNext(xmlObjectList pList, xmlObjectIterator Iterator)
	{
		CArray<TYPE, ARG_TYPE> *pTypedList = (CArray<TYPE, ARG_TYPE> *) pList;
		TYPE *pObject = NULL;

		int *pIter = (int *)Iterator;

		if (*pIter < pTypedList->GetSize())
		{
			pObject = (TYPE *)pTypedList->ElementAt(*pIter);
			*pIter = *pIter + 1;
		}
		else
		{
			delete pIter;
			pIter = 0;
		}
		
		return (TYPE)pObject;
	}
	void releaseIterator(xmlObjectIterator Iterator)
	{
		delete (int *)Iterator;
		Iterator = 0;
	}
	void removeObject(xmlObjectList pList, xmlObjectIterator Iterator)
	{
		CArray<TYPE, ARG_TYPE> *pTypedList = (CArray<TYPE, ARG_TYPE> *) pList;
		int *pIter = (int *)Iterator;

		if (*pIter < pTypedList->GetSize())
		{
			pTypedList->RemoveAt(*pIter);
		}
	}
	void create(xmlObjectList pList)
	{
		// Not implemented
	}
	void destroy(xmlObjectList pList)
	{
		// Not implemented
	}
};



class CStringArrayAbstraction : public StringCollectionAbstraction
{
public:
	void append(userStringList pList, const char *pString)
	{
		CStringArray *pTypedList = (CStringArray *)pList;
		pTypedList->Add( pString );
	}

	CString getAt(userStringList pList, unsigned int nIndex, int *bIsValidIndex)
	{
		CStringArray *pTypedArray = (CStringArray *)pList;

		if (nIndex < (unsigned int) pTypedArray->GetSize())
		{
			*bIsValidIndex = 1;
			return (*pTypedArray)[nIndex];
		}
		*bIsValidIndex = 0;
		return "";
	}

	const char *getFirst(userStringList pList, xmlObjectIterator *pIterator)
	{
		CStringArray *pTypedArray = (CStringArray *)pList;
		const char *pObject = NULL;

		if ( pTypedArray->GetSize() > 0)
		{
			pObject = (const char *)(pTypedArray->ElementAt(0));
			if(pObject)
			{
				int *pIter = new int;
				*pIter = 1;
				*pIterator = pIter;
			}
		}

		return (const char *)pObject;
	}

	const char *getNext(userStringList pList, xmlObjectIterator Iterator)
	{
		CStringArray *pTypedArray = (CStringArray *)pList;
		const char *pObject = NULL;

		int *pIter = (int *)Iterator;

		if (*pIter < pTypedArray->GetSize())
		{
			pObject = (const char *)pTypedArray->ElementAt(*pIter);
			*pIter = *pIter + 1;
		}
		else
		{
			delete pIter;
			pIter = 0;
		}
		
		return (const char *)pObject;
	}

	__int64 itemCount(userStringList pList)
	{
		CStringArray *pTypedArray = (CStringArray *)pList;
		return (unsigned int)pTypedArray->GetSize();
	}

};



template <class TYPE> class CMapStringToPtrAbstraction : public KeyedDataStructureAbstraction
{
public:
	XMLObject *search(KeyedDataStructure kds, const char *pzKey)
	{
		CMapStringToPtr *pTyped = (CMapStringToPtr *)kds;
		void *pRet;
		pTyped->Lookup(pzKey,pRet);
		return (XMLObject *)pRet;
	}
	void destroy(KeyedDataStructure kds)
	{
	};
	void create(KeyedDataStructure kds)
	{
	};
	void remove(KeyedDataStructure kds, userHashIterator Iterator, const char *pzKey, XMLObject *pObj)
	{
	}

	void insert(KeyedDataStructure kds, XMLObject *pObj, const char *pzKey)
	{
		CMapStringToPtr *pTyped = (CMapStringToPtr *)kds;
		pTyped->SetAt(pzKey, pObj);

	}
	XMLObject *getFirst(KeyedDataStructure kds, userHashIterator *pIterator)
	{
		void *pRet = 0;
		CMapStringToPtr *pTyped = (CMapStringToPtr *)kds;
		

		if ( pTyped->GetCount() > 0)
		{
			CString key;
			POSITION *pIter = new POSITION;
			*pIter = pTyped->GetStartPosition();
			*pIterator = pIter;
			pTyped->GetNextAssoc(*pIter, key, pRet);
			if (!pRet)
			{
				delete pIter;
				pIter = 0;
			}

		}
		return (TYPE *)pRet;
	}
	XMLObject *getNext(KeyedDataStructure kds, userHashIterator Iterator)
	{
		POSITION *pIter = (POSITION *)Iterator;
		CMapStringToPtr *pTyped = (CMapStringToPtr *) kds;

		void *pObject = 0;
		if ( *pIter )
		{
			CString key;
			pTyped->GetNextAssoc(*pIter, key, pObject);
		}
		else
		{
			delete pIter;
			pIter = 0;
		}
		return (TYPE *)pObject;

	}
	__int64 itemCount(KeyedDataStructure kds)
	{
		const CMapStringToPtr *pTyped = (CMapStringToPtr *)kds;
		return pTyped->GetCount();
	}
	void releaseIterator(xmlObjectIterator Iterator)
	{
		delete (int *)Iterator;
		Iterator = 0;
	}
};



#endif //_XML_MFC_ABSTRACTIONS