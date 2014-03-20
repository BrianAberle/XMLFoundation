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

#ifndef __HASH_TABLE_H__
#define __HASH_TABLE_H__

#include "GString.h"
#include "GBTree.h"

// The GHash "Data Structure Algorithm" uses a "Hash" to index an "Array" of "Balanced Trees", it's fast.
class GHash
{
protected:
	   // m_table is a "Static Array" of "Balanced Binary-Trees" - not a Dynamic Array like GArray
		GBTree *m_table;			
	   __int64	m_nCount;
	   __int64	m_nHashTableSize;
	   __int64	m_nDeferDestruction;

	   void InitTable();
	   friend class GHashIterator; // GHash Iterator Class
public:
	   // number of elements in the hash table
	   __int64  GetCount() const;
	   bool IsEmpty() const;

	   // The memory address Object 
inline void *Lookup(unsigned __int64 key, __int64 nOccurance = 1) const { return Lookup(   (unsigned int)(key % (unsigned int)-1),      nOccurance    ); } 
	   void *Lookup(unsigned int     key, __int64 nOccurance = 1) const;
	   void *Lookup(const char *     key, __int64 nOccurance = 1) const;
	   void *Lookup(void *           key, __int64 nOccurance = 1) const;
	   // You must use a unique key to search/lookup with operator [] since you always get the first/only occurrence
	   void *operator[](const char * key) const;


	// add a new (key, value) pair
inline void Insert(unsigned __int64 key, void *value) const { Insert(   (unsigned int)(key % (unsigned int)-1),      value    ); } 
	   void Insert(unsigned int key, void *value);
	   void Insert(const char *key, void *value);
	   void Insert(void *key, void *value);


	// removing existing (key, value) pair
	// when nOccurance = 0, all occurrences of the key are removed, otherwise nOccurance = must be 1 or > to remove the first or the second occurrence etc..
inline void *RemoveKey(unsigned __int64	key, __int64 nOccurance = 0) const { return RemoveKey(   (unsigned int)(key % (unsigned int)-1),      nOccurance    ); } 
	   void *RemoveKey(unsigned int		key, __int64 nOccurance = 0);
	   void *RemoveKey(const char *		key, __int64 nOccurance = 0);
	   void *RemoveKey(void *			key, __int64 nOccurance = 0);
	   void RemoveAll();

	void reassignKeyPair(const char*key, void *pOld, void*pNew);

	// creates a hash index
	unsigned __int64 HashKey(const char * key) const;

	void DeferDestruction();
	void Destruction();

	// Construct the Hash with a prime number.  For very large data sets you may want
	// one of these primes: 4441, 8123, 11777, 17017, 170777
	// or a smaller prime :    7, 101, 503, 1009
	GHash(unsigned int nPrime = 2503);
	~GHash();
};


#pragma warning (disable:4512)	// 'GHashIterator' : assignment operator could not be generated

class GHashIterator
{
	GBTreeIterator m_it;
	int m_itOK;

	const GHash &m_hash;
	__int64 m_nHashIndex;
	void *m_pLookAhead;
	void *Increment();
public:

	GHashIterator(const GHash *hash);
	~GHashIterator(){};
	int operator ()(void);
	void * operator ++ (int);
};

#endif // __HASH_TABLE_H__
