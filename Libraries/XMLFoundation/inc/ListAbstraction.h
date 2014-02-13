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
#ifndef XML_LIST_ABSTRACTION
#define XML_LIST_ABSTRACTION

typedef void * xmlObjectIterator;
typedef void * xmlObjectList;
typedef void * userStringList;
typedef void * userStringListIterator;
typedef void * userArray;
typedef void * userHashIterator;
typedef void * KeyedDataStructure;

#include "GString.h"

class XMLObject;
class ListAbstraction	// abstract object container interface
{
public:
	// remove the last object issued by getFirst() or getNext()
	virtual void removeObject(xmlObjectList pList, xmlObjectIterator Iterator) = 0;
	virtual __int64 itemCount(xmlObjectList pList) = 0;
	virtual void append(xmlObjectList pList, XMLObject *pObject) = 0;
	virtual XMLObject *getLast(xmlObjectList pList) = 0;
	virtual XMLObject *getFirst(xmlObjectList pList, xmlObjectIterator *pIterator) = 0;
	virtual XMLObject *getNext(xmlObjectList pList, xmlObjectIterator Iterator) = 0;
	virtual void releaseIterator(xmlObjectIterator Iterator) = 0;
	virtual void destroy(xmlObjectList pList) = 0;
	virtual void create(xmlObjectList pList) = 0;
};

///////////////////////////////////////////////////////////////////////////////////////////
// hashes, binary-trees, etc...
///////////////////////////////////////////////////////////////////////////////////////////
// Unlike all the other *Abstraction* base classes, this one contains state (members).
// Therefore it becomes necessary to create one instance of this deravitive for each
// KeyedDataStructure that is used.  You cannot use a global instance for all handlers
// as is typical for all other *Abstraction* base classes.
///////////////////////////////////////////////////////////////////////////////////////////
// For example, 
// class foo
// {
//	GHashAbstraction m_htblUsersAbs;
//	hashTbl m_htblUsers;
//	GHashAbstraction m_htblOtherAbs;
//	hashTbl m_htblOther;
//	....
//
//	virtual void MapXMLTagsToMembers()
//	{
//		MapMember(&m_htblUsers,	&m_htblUsersAbs,
//				  CUser::GetStaticTag(), 
//				  "User");
//		MapMember(&m_htblOther,	&m_htblOtherAbs,
//				  COther::GetStaticTag(), 
//				  "Other");
//
///////////////////////////////////////////////////////////////////////////////////////////
class KeyedDataStructureAbstraction
{
public:
	virtual void insert(KeyedDataStructure kds, XMLObject *pObj, const char *pzKey) = 0;
	virtual XMLObject *search(KeyedDataStructure kds, const char *pzKey) = 0;
	virtual XMLObject *getFirst(KeyedDataStructure kds, userHashIterator *pIterator) = 0;
	virtual XMLObject *getNext(KeyedDataStructure kds, userHashIterator Iterator) = 0;
	virtual __int64 itemCount(KeyedDataStructure kds) = 0;
	virtual void releaseIterator(xmlObjectIterator Iterator) = 0;
	virtual void destroy(KeyedDataStructure kds) = 0;
	virtual void create(KeyedDataStructure kds) = 0;
	virtual void remove(KeyedDataStructure kds, userHashIterator Iterator, const char *pzKey, XMLObject *pObj) = 0;
};



// Abstract container interface:  This is an interface to an xml collection(all collections are strings)
// the application may be a list, tree, array, or anything you desire to implement containing any type
// of data that you convert to and from a character string representation for the xml.  For example,
// look at CDoubleArrayAbstraction in AbstractionsMFC.h to see this used as a base class abstraction
// used to store double's in an MFC CArray.
class StringCollectionAbstraction	
{
public:
	// append a new string to your string collection implementation
	virtual void append(userStringList pList, const char *pString) = 0;
	virtual const char *getFirst(userStringList pList, userStringListIterator *pIterator) = 0;
	virtual const char *getNext(userStringList pList, userStringListIterator Iterator) = 0;
	virtual __int64 itemCount(userStringList pList) = 0;
};

class IntegerArrayAbstraction	// abstract dynamic allocation integer array interface
{
public:
	// append a new integer to your dynamic array implementation
	virtual void append(userArray pArray, __int64 nValue) = 0;
	virtual __int64 getAt(userArray pArray, __int64 nIndex, int *bIsValidIndex) = 0;
	virtual __int64 itemCount(userArray pList) = 0;
};


#endif //XML_LIST_ABSTRACTION
