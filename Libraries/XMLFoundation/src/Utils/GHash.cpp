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
#include "GlobalInclude.h"
static char SOURCE_FILE[] = __FILE__;

#include "GHash.h"
#include "GException.h"
#include <string.h> // for strlen

// CityHash and Spooky are so close to the same that I decided to use SpookyHash
// Since they both slow down GHash indexing and are NOT enabled by default or in 
// any production code, therefore it does not matter which one I choose.
// I have never found any case where changing from that default.speeds up GHash.  Maybe you will.
#include "SpookyV2.cpp"  
// #include "../src/Utils/SpookyV2.cpp" 



// Here is an excellent Hash index algorithm comparison online:    
// http://www.burtleburtle.net/bob/hash/doobs.html
unsigned __int64 GHash::HashKey(const char *key) const
{
	// Dec 2013 - Changing to SpookyHash/CityHash will slow down insert and lookup.
	// It will reduce collisions depending on the variable value of the "const char *key"
	// I found no test case there the reduction in collisions.

	// circumstances will (very slightly) slow down despite the potential for performance improvement
	// This is why, by default, SpookyHash is NOT used.
//	if (m_nCollisionReduction)	
//		return SpookyHash::Hash32(key, strlen(key), 0);

	// The following VERY simple hash normally(maybe always) produces slightly faster lookups... 

	// 64 bit FNV1 hash
    unsigned __int64 nHash = 14695981039346656037;
    while(*key)
	{
        nHash = (nHash * 1099511628211) ^ *key++;
    }
    return nHash;


	// Rotating Hash
//	unsigned int nHash = 0;
	while (*key)
		nHash = (nHash<<5) + nHash + *key++;// <----------- this hash was used from 1999-2013
//		nHash = (nHash<<4)^(nHash>>28)^(*key++);// in Dec 2013 this line of code became the hash index
												// that line of code can be further optimized and changed
												// to find the best use of the CPU to convert 
												// an array of bytes into an integer(aka binary).
	return nHash;								// For the data structure design within GHash that will use 
												// this integer, collisions are expected and handled efficiently
												// by fragmenting (into a B-Tree) which was avoided by "disk"
}												// minded folks(where fragmentation IS bad) while designing their 
												// indexing scheme, however their underestimation of a reCursive 
												// in memory(not "disk") de-fragmentation algorithm prevented
// number of elements							// them from finding the faster solution.  GBTree uses an iterator.
__int64  GHash::GetCount() const						
{
	return m_nCount;
}

bool GHash::IsEmpty() const
{
	return (m_nCount == 0);
}

// use the memory address as the hash key
void *GHash::Lookup(void *key, __int64 nOccurance/* = 1*/) const
{
	union CAST_THIS_TYPE_SAFE_COMPILERS
	{
		// this union has this variable sized type 'void *' = 32 or 64 bits
		void *			 mbrVoid;
		unsigned __int64 mbrInt64;
		unsigned int	 mbrInt;
	}Member;
	Member.mbrVoid = key;

#if defined(_LINUX64) || defined(_WIN64)  || defined(_IOS)
    // The memory address is a 64 bit integer - convert to a 32 bit integer using modulo/modulus
	// as opposed to throwing out the high order bytes by using a static cast from a 64 to a 32 bit integer
	unsigned int nHashKey = Member.mbrInt64 % (unsigned int)-1;
	return Lookup( nHashKey, nOccurance);
#else
	return Lookup(Member.mbrInt, nOccurance);
#endif


}

// Lookup
void *GHash::Lookup(const char *key, __int64 nOccurance/* = 1*/) const
{
	if (!m_table)
	{
		// throw an exception - could not allocate hash table
		throw GException("Hash", 0);
	}

	unsigned int nHashKey = HashKey(key);
	unsigned int nHash = nHashKey % m_nHashTableSize;


	GBTree &rAssoc = m_table[nHash];
	return rAssoc.search(key,nOccurance);
}

void *GHash::Lookup(unsigned int nKey, __int64 nOccurance/* = 1*/) const
{
	if (!m_table)
	{
		// throw an exception - could not allocate hash table
		throw GException("Hash", 0);
	}

	unsigned int nHash = nKey % m_nHashTableSize;
	GBTree &rAssoc = m_table[nHash];
	GString strKey;
	strKey << nKey;
	return rAssoc.search(strKey,nOccurance);
}

void *GHash::operator[](const char *key) const
{
	return Lookup(key);
}


void GHash::Insert(void *key, void *value)
{
	union CAST_THIS_TYPE_SAFE_COMPILERS
	{
		void *       mbrVoid;
		unsigned __int64 mbrInt64;
		unsigned int mbrInt;
	}Member;
	Member.mbrVoid = key;

#if defined(_LINUX64) || defined(_WIN64) || defined(_IOS)
    // The memory address is a 64 bit integer - convert to a 32 bit integer using modulo/modulus
	// as opposed to throwing out the high order bytes by using a static cast from a 64 to a 32 bit integer
	unsigned int nHashKey = Member.mbrInt64 % (unsigned int)-1;
	Insert(nHashKey, value);
#else
	Insert(Member.mbrInt, value);
#endif
	
}


// add a new (key, value) pair
void GHash::Insert(const char *key, void *value)
{
	if (!m_table)
	{
		// throw an exception - could not allocate hash table
		throw GException("Hash", 0);
	}

	unsigned int nHashKey = HashKey(key);
	unsigned int nHash = nHashKey % m_nHashTableSize;

	GBTree &rAssoc = m_table[nHash];
	rAssoc.insert(key, value);

	m_nCount++;
}

// add a new (key, value) pair
void GHash::Insert(unsigned int nHashKey, void *value)
{
	if (!m_table)
	{
		// throw an exception - could not allocate hash table
		throw GException("Hash", 0);
	}

	unsigned int nHash = nHashKey % m_nHashTableSize;

	GBTree &rAssoc = m_table[nHash];
	GString strKey;
	strKey << nHashKey;
	rAssoc.insert(strKey, value);

	m_nCount++;
}

void GHash::reassignKeyPair(const char*key, void *pOld, void*pNew)
{
	if (!m_table)
	{
		// throw an exception - could not allocate hash table
		throw GException("Hash", 0);
	}
	unsigned int nHashKey = HashKey(key);
	unsigned int nHash = nHashKey % m_nHashTableSize;

	GBTree &rAssoc = m_table[nHash];

	rAssoc.reassignKeyPair(key, pOld, pNew);
}


// removing existing (key, ?) pair
void *GHash::RemoveKey(const char *key, __int64 nOccurance/* = 0*/)
{
	if (!m_table)
	{
		// throw an exception - could not allocate hash table
		throw GException("Hash", 0);
	}

	unsigned int nHashKey = HashKey(key);
	unsigned int nHash = nHashKey % m_nHashTableSize;

	GBTree &rAssoc = m_table[nHash];

	if (nOccurance == 0) // remove them all, return the 1st of them if there was 1 or >
	{
		void *pRet = rAssoc.search(key);
		void *pCur = pRet;
		while (pCur)
		{
			pCur = rAssoc.removeGet(key);
			if (pCur)
				m_nCount--;
		}
		return pRet;
	}
	else // remove exactly what is specified
	{
		return rAssoc.removeGet(key, nOccurance);
	}
	return 0;
}


void *GHash::RemoveKey(void *key, __int64 nOccurance/* = 0*/)
{
	union CAST_THIS_TYPE_SAFE_COMPILERS
	{
		void *       mbrVoid;
		unsigned __int64 mbrInt64;
		unsigned int mbrInt;
	}Member;
	Member.mbrVoid = key;
#if defined(_LINUX64) || defined(_WIN64)  || defined(_IOS)
    // The memory address is a 64 bit integer - convert to a 32 bit integer using modulo/modulus
	// as opposed to throwing out the high order bytes by using a static cast from a 64 to a 32 bit integer
	unsigned int nHashKey = Member.mbrInt64 % (unsigned int)-1;
	return RemoveKey(nHashKey, nOccurance);
#else
	return RemoveKey(Member.mbrInt, nOccurance);
#endif

}

void *GHash::RemoveKey(unsigned int nHashKey, __int64 nOccurance/* = 0*/)
{
	if (!m_table)
	{
		// throw an exception - could not allocate hash table
		throw GException("Hash", 0);
	}

	unsigned int nHash = nHashKey % m_nHashTableSize;

	GBTree &rAssoc = m_table[nHash];
	GString strKey;
	strKey << nHashKey;
	void *pRet = rAssoc.search(strKey);
	if (pRet)
	{
		rAssoc.remove(strKey);
		m_nCount--;
	}
	return pRet;
}

void GHash::InitTable()
{
	if (m_table)
		delete [] m_table;
	m_table = new GBTree[m_nHashTableSize];
	m_nCount = 0;
}

void GHash::RemoveAll()
{
	InitTable();
}

GHash::GHash(unsigned int nPrime /* = 2503 */) :
	m_nHashTableSize(nPrime),
	m_table(0),
	m_nDeferDestruction(0),
	m_nCount(0)
{
	InitTable();
}

GHash::~GHash()
{
	if (!m_nDeferDestruction)
	{
		delete [] m_table;
		m_table = 0;
	}

}

void GHash::DeferDestruction()
{
	m_nDeferDestruction = 1;
}

void GHash::Destruction()
{
	if (m_table)
	{
		delete [] m_table;
		m_table = 0;
	}

}

GHashIterator::GHashIterator(const GHash *hash) :
	m_hash(*hash),
	m_nHashIndex(0),
	m_pLookAhead(0),
	m_itOK(0)
{
}


void *GHashIterator::Increment()
{
	if (m_pLookAhead)
	{
		void *pRet = m_pLookAhead;
		m_pLookAhead = 0;
		return pRet;
	}

	// return the next in the tree at this hash index
	if ( m_itOK && m_it() )
		return m_it++;

	// we're at the end of this tree's iterator
	m_itOK = 0;

	// look at the next hash index
	for (; m_nHashIndex < m_hash.m_nHashTableSize;)
	{
		if (m_hash.m_table[m_nHashIndex].getNodeCount())
		{
			// move to the beginning of the next tree's iterator
			m_it.Reset(&m_hash.m_table[m_nHashIndex], 2);
			m_itOK = 1;
			m_nHashIndex++;
			return m_it++;
		}
		m_nHashIndex++;
	}
	return 0;
}

int GHashIterator::operator ()(void)
{
	if (m_pLookAhead)
		return 1;
	else
		m_pLookAhead = (*this)++;
	return (m_pLookAhead == 0) ? 0 : 1;
}

void * GHashIterator::operator ++ (int)
{
	void *pRet = Increment();
	while(pRet == (void *)-1)
		Increment();
	return pRet;
}
