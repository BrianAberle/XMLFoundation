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
#ifndef __GARRAY_H__
#define __GARRAY_H__

#define GA_NOT_FOUND 0xFFFF
#include "GString.h"
#include "GStack.h"

class GArray
{
	GStack m_KeySpace;
	int m_nCurrentKeySpaceIndex;
	int m_nCurrentKeySpaceSize;
	char *MakeKey(const char *szKey);
protected:

	struct ObjectKey
	{
		char *m_pszKey;
		void *m_pValue;
	};
	
	unsigned long m_nGrowBy;
	__int64 m_nSize;
	__int64 m_nNext;

	bool m_bSorted;
	int	 m_nDeferDestruction;

	void InsertSort(__int64 lb, __int64 ub);
	__int64 Partition(__int64 lb, __int64 ub);
	void QSort(__int64 nFirst, __int64 nLast);

public:
	
	// This member is exposed for performance reasons, you can use it for object access like this:
	// MyObject *pO = (MyObject *)(MyGArray.m_arrObjects[n].m_pValue);
	ObjectKey *m_arrObjects;

	void Initialize(void);

	// the number of items in the array.

	bool IsSorted() const 
	{
		return m_bSorted;
	}

	// # of items in the array.  If GetItemCount() == 5 then the 
	// largest valid index is 4, the 5th item (0,1,2,3,4)
	// note: GArray::Size() is now GetItemCount()
	__int64 GetItemCount(void) const
	{
		return m_nNext; 
	}

	// Generally GetSize() is not useful information.  This returns the entire allocated size of the array.  
	// Generally GetItemCount() is what you want to return the number of items in the array.
	__int64 GetSize(void) const 
	{ 
		return m_nSize; 
	}

	// will expand array to new size
	// -----------------------------
	bool Grow(unsigned long nGrowBy);

	// adds the element to the m_nNext position
	// ---------------------------------------
	__int64 AddElement(void *pObject, const char *szKey = 0);
	const void *Search(const char *szKey) const;
	
	// same as Search() and AddElement() added to match GBHash interface
	const void *Lookup(const char *szKey) const {return Search(szKey);};
	__int64 Insert(const char *szKey, void *pObject) {return AddElement(pObject,szKey);};

	void Sort();

	// these methods are equivalent
	void *& GetAt(__int64 nIdx);
	void *&operator[](__int64 nIdx);
	void *&operator[](unsigned long nIdx);


	void Assign(__int64 nIdx, void *pValue, const char *szKey = 0);

	// returns the first index that matches pFind or GA_NOT_FOUND
	__int64 FindFirst(const void *pFind) const;

	// returns the first non matching index or GA_NOT_FOUND
	__int64 FindFirstNon(const void *pFind) const;

	void DeferDestruction();
	void Destruction();

	// construct with a specified size, and increments to grow by
	GArray(__int64 nSize, unsigned long nGrowBy);
	// construct by default with a size of 10 that grows by blocks of 10
	GArray(unsigned long nGrowBy = 10);
	virtual ~GArray();
};

// this class is "identical" to GArray, the base class contains the Qsort algorithm
// this exists because XMLObject::MapMember() distinguishes between mapping a simple
// array of integers and also a keyed data structure, and they both are contained in
// GArray, however using the GQSortArray indicates that the indexed structure will 
// be used to contain objects - not an array of integers.
class GQSortArray : public GArray
{
public:
	GQSortArray(__int64 nSize, unsigned long nGrowBy) : GArray(nSize,nGrowBy)	{	}
	GQSortArray(unsigned long nGrowBy = 10) : GArray(nGrowBy) {}
	virtual ~GQSortArray(){};
};


#endif // __GARRAY_H__
