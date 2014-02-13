// --------------------------------------------------------------------------
//						United Business Technologies
//			  Copyright (c) 2000 - 2012  All Rights Reserved.
//
// Source in this file is released to the public under the following license:
// --------------------------------------------------------------------------
// This toolkit may be used free of charge for any purpose including corporate
// and academic use.  For profit, and Non-Profit uses are permitted.
//
// This source code and any work derived from this source code must retain 
// this copyright at the top of each source file.
// --------------------------------------------------------------------------

#ifndef _XML_STD_ABSTRACTION
#define _XML_STD_ABSTRACTION


#include "ListAbstraction.h"
#include "StringAbstraction.h"

#include <list>
#include <string>
using namespace std ;


// there is only 1 instance of this class.
class STDStringAbstract : public StringAbstraction
{
public:
	int isEmpty(userString pString)
	{
		return (((string *)pString)->empty());
	}
	__int64 length(userString pString)
	{
		return ((string *)pString)->length();
	}
	void assign(userString pString, const char *pzValue)
	{
		((string *)pString)->assign(pzValue);
	}
	void append(userString pString, const char *pzValue)
	{
		((string *)pString)->append(pzValue);
	}
	const char *data(userString pString)
	{
		return ((string *)pString)->c_str();
	}
};




template <class TYPE> class STDListAbstraction : public ListAbstraction
{
public:
	void create(xmlObjectList pList){}
	void destroy(xmlObjectList pList){}
	
	__int64 itemCount(xmlObjectList pList)
	{
		list<TYPE>*pTypedList = (list<TYPE> *)pList;
		return pTypedList->size();
	}
	void append(xmlObjectList pList, XMLObject *pObject)
	{
		list<TYPE> *pTypedList = (list<TYPE> *)pList;
		pTypedList->push_back((TYPE)pObject->GetUntypedThisPtr());
	}
	XMLObject *getLast(xmlObjectList pList)
	{
		list<TYPE> *pTypedList = (list<TYPE> *)pList;
		TYPE p = (pTypedList->size()) ? pTypedList->back() : 0;
		return p;

	}



	// this is about as complex as pointer indirection can get:
	// Iterator, of type xmlObjectIterator (which is a typedef for void *),
	// is a pointer.  Because there is no * between "xmlObjectIterator Iterator"  below,
	void removeObject(xmlObjectList pList,          xmlObjectIterator Iterator)
	// you must be aware that the * is there when you expand the definition of xmlObjectIterator in your mind
	// that is why the next two lines of code that follow do nothing whatsoever except cast to the actual
	// data type in it's native cast after passing through the preexisting abstraction in the XMLFoundation
	{

		list<TYPE> *pTypedList = (list<TYPE> *)pList;
		
		// the following line of code will not compile under the g++ and I dont know why yet (12/11/2012)
		list<TYPE>::iterator *pi;
		pi= (list<TYPE>::iterator *)Iterator;

	// to add to the situation of this being about as complex as pointer indirection can get we have the 
	// added complexity of this code existing in a C++ template <class TYPE> where type is 
	// actually a pointer of all things, so in your mind see TYPE as something with a * after it.
	
		++(*pi); // I could have put      ++(*(list<TYPE>::iterator *)Iterator);      but that adds complexity
		TYPE p = *(*pi);

		if(p)
		{
			pTypedList->remove( p );
		}
	}



	XMLObject *getFirst(xmlObjectList pList, xmlObjectIterator *pIterator)  //pIterator is a pointer to a pointer
	{
		list<TYPE> *pTypedList = (list<TYPE> *)pList;
		TYPE pObject = 0;
		if ( pTypedList->size() )
		{
			list<TYPE>::iterator *i = new list<TYPE>::iterator;
			*i = pTypedList->begin();
			*pIterator = i;
			pObject = *(*i);

		}
		return pObject;
	}
	XMLObject *getNext(xmlObjectList pList, xmlObjectIterator Iterator)
	{
		list<TYPE> *pTypedList = (list<TYPE> *)pList;
		list<TYPE>::iterator *pIter = (list<TYPE>::iterator *)Iterator;
		TYPE pObject = 0;
		++(*pIter);
		if(*pIter == pTypedList->end())
		{
			delete pIter;
			pIter = 0;
			pObject = 0;
		}
		else
		{
			pObject = *(*pIter);
		}
		return pObject;
	}
	void releaseIterator(xmlObjectIterator Iterator)
	{
		list<TYPE>::iterator *pIter = (list<TYPE>::iterator *)Iterator;
		delete pIter;
		Iterator = 0;
	}
};



class STDStringListAbstraction : public StringCollectionAbstraction
{
	string _s;
public:
	void append(userStringList pList, const char *pString)
	{
		list<string> *pTypedList = (list<string> *)pList;
		pTypedList->push_back(pString);
	}
	const char *getFirst(xmlObjectList pList, xmlObjectIterator *pIterator)
	{
		list<string> *pTypedList = (list<string> *)pList;
		const char *pRet = 0;
		if ( pTypedList->size() )
		{
			list<string>::iterator *i = new list<string>::iterator;
			*i = pTypedList->begin();
			*pIterator = i;

		
#	if defined(_MSC_VER) && _MSC_VER >= 1400
			// this is faster, and a much preferred approach to using member _s to store the string
			// this eliminates multithreading issues that may arise in some cases.
			// it compiles in vs2012 and not in VC6
			pRet = ((*i)._Ptr->_Myval).data();		
#else
			_s = (*(*i));   
			pRet = _s.data();
#endif

		}
		else
		{
			return 0;		
		}
		return pRet;
	}
	const char *getNext(xmlObjectList pList, xmlObjectIterator Iterator)
	{
		list<string> *pTypedList = (list<string> *)pList;
		list<string>::iterator *pIter = (list<string>::iterator *)Iterator;
		const char *pRet = 0;
		++(*pIter);
		if(*pIter == pTypedList->end())
		{
			delete pIter;
			pIter = 0;
			return 0;
		}
		else
		{
#	if defined(_MSC_VER) && _MSC_VER >= 1400
			// this is faster, and a much preferred approach to using member _s to store the string
			// this eliminates multithreading issues that may arise in some cases.
			// it compiles in vs2012 and not in VC6
			pRet = ((*pIter)._Ptr->_Myval).data();		
#else
			_s = (*(*pIter));   
			pRet = _s.data();
#endif

		}
		return pRet;
	}
	__int64 itemCount(userStringList pList)
	{
		list<string>*pTypedList = (list<string> *)pList;
		return pTypedList->size();
	}
};

#endif //_XML_STD_ABSTRACTION
