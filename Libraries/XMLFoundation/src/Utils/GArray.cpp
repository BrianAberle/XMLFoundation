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

#include "GArray.h"
#include "GException.h"
#include <string.h> // for: strlen(), strcpy(), strcmp()

__int64 GArray::FindFirstNon(const void *p) const
{
	for (__int64 i = 0; i < m_nNext; i++)
	{
		if (m_arrObjects[i].m_pValue != p)
			return i;
	}
	return GA_NOT_FOUND;
}

__int64 GArray::FindFirst(const void *p) const
{
	for (__int64 i = 0; i < m_nNext; i++)
	{
		if (m_arrObjects[i].m_pValue == p)
			return i;
	}
	return GA_NOT_FOUND;
}

bool GArray::Grow(unsigned long nGrowBy)
{
	bool bRet = false;

	ObjectKey *p;

	p = new ObjectKey[m_nSize + nGrowBy];

	if (p != 0)
	{
//		memset(p, 0, sizeof(ObjectKey) * (m_nSize + nGrowBy));
		memcpy(p, m_arrObjects, sizeof(ObjectKey) * m_nSize);
		delete [] m_arrObjects;

		m_nSize += nGrowBy;
		m_arrObjects = p;

		bRet = true;
	}
	else
	{
		throw GException("GenericArray", 1);
	}

	return bRet;
}

char *GArray::MakeKey(const char *szKey)
{
	int nKeyLen = strlen(szKey) + 1;
	char *pCurrentKeyBlock = (char *)m_KeySpace.Top();
	
	char *pReturnValue = 0;
	
	// this only happens 1 time - when the first key is created then the first block is allocated
	if (!pCurrentKeyBlock)
	{
		m_nCurrentKeySpaceSize = (nKeyLen > 4096) ? nKeyLen * 32 : 4096;
		pCurrentKeyBlock = new char[m_nCurrentKeySpaceSize];
		m_KeySpace.Push(pCurrentKeyBlock);
	}
	// if the next key(and +1 for a null termination) will fit in the current key space block
	if ( m_nCurrentKeySpaceIndex + nKeyLen < m_nCurrentKeySpaceSize )
	{
		memcpy(&pCurrentKeyBlock[m_nCurrentKeySpaceIndex],szKey,nKeyLen);
		pReturnValue = &pCurrentKeyBlock[m_nCurrentKeySpaceIndex];
		m_nCurrentKeySpaceIndex += nKeyLen;
	}
	else
	{
		// subsequent new key space blocks are created here
		m_nCurrentKeySpaceSize = (nKeyLen > 4096) ? nKeyLen * 32 : 4096;
		pCurrentKeyBlock = new char[m_nCurrentKeySpaceSize];
		m_KeySpace.Push(pCurrentKeyBlock);
		m_nCurrentKeySpaceIndex = 0;

		// now add the new key - the exact same three lines of code are just above the "else" we are in
		memcpy(&pCurrentKeyBlock[m_nCurrentKeySpaceIndex],szKey,nKeyLen);
		pReturnValue = &pCurrentKeyBlock[m_nCurrentKeySpaceIndex];
		m_nCurrentKeySpaceIndex += nKeyLen;
	}
	return pReturnValue;
}

__int64 GArray::AddElement(void *Element, const char *szKey /* = 0 */)
{
	// check to see if the array size need to grow
	// -------------------------------------------
	if (m_nNext >= m_nSize)
	{
		Grow(m_nGrowBy);
	}

	// add the element to the end of the array
	// ---------------------------------------
	(*(m_arrObjects + m_nNext)).m_pValue = Element;
	if (szKey)
	{
		(*(m_arrObjects + m_nNext)).m_pszKey = MakeKey(szKey); //  = new char[strlen(szKey) + 1];
//		strcpy((*(m_arrObjects + m_nNext)).m_pszKey, szKey);
	}

	m_nNext++;

	m_bSorted = false;

	// return the index where the element was added
	// --------------------------------------------
	return m_nNext - 1;
}

// returns a pointer to the array element if
// the index is in the array size.
// -----------------------------------------
void *&GArray::operator[](unsigned long nIdx)
{
	if (nIdx >= m_nNext)
		throw GException("GenericArray", 0);
	m_bSorted = false;
	return (*(m_arrObjects + nIdx)).m_pValue;
}
void *&GArray::operator[](__int64 nIdx)
{
	if (nIdx >= m_nNext)
		throw GException("GenericArray", 0);
	m_bSorted = false;
	return (*(m_arrObjects + nIdx)).m_pValue;
}
void *&GArray::GetAt(__int64 nIdx)
{
	if (nIdx >= m_nNext)
		throw GException("GenericArray", 0);
	m_bSorted = false;
	return (*(m_arrObjects + nIdx)).m_pValue;
}



void GArray::Assign(__int64 nIdx, void *pValue, const char *szKey /* = 0 */)
{
	if (nIdx >= m_nNext)
		throw GException("GenericArray", 0);

	(*(m_arrObjects + nIdx)).m_pValue = pValue;

//	delete (*(m_arrObjects + m_nNext)).m_pszKey;
	if (szKey)
	{
		(*(m_arrObjects + m_nNext)).m_pszKey = MakeKey(szKey);       // = new char[strlen(szKey) + 1];
//		strcpy((*(m_arrObjects + m_nNext)).m_pszKey, szKey);
	}
	else
	{
		(*(m_arrObjects + m_nNext)).m_pszKey = 0;
	}

	m_bSorted = false;
}

void GArray::Initialize()
{
	m_bSorted = false;
	m_nNext = 0;
	m_nDeferDestruction = 0;

	m_arrObjects = new ObjectKey[m_nSize];
//	memset(m_arrObjects, 0, sizeof(ObjectKey) * m_nSize);
}

void GArray::InsertSort(__int64 lb, __int64 ub) 
{
    ObjectKey swapElem;
    __int64 i, j;

   /**************************
    *  sort array a[lb..ub]  *
    **************************/
    for (i = lb + 1; i <= ub; i++) 
	{
        swapElem = *(m_arrObjects + i);

        /* Shift elements down until */
        /* insertion point found.    */
        for (j = i-1; j >= lb && strcmp((m_arrObjects + j)->m_pszKey, swapElem.m_pszKey) > 0; j--)
            *(m_arrObjects + j + 1) = *(m_arrObjects + j);

        /* insert */
        *(m_arrObjects + j + 1) = swapElem;
    }
}

__int64 GArray::Partition(__int64 lb, __int64 ub) 
{
    ObjectKey t, pivot;
    __int64 i, j, p;

   /*******************************
    *  partition array a[lb..ub]  *
    *******************************/

    /* select pivot and exchange with 1st element */
    p = lb + ((ub - lb)>>1);
    pivot = *(m_arrObjects + p);
    *(m_arrObjects + p) = *(m_arrObjects + lb);

    /* sort lb+1..ub based on pivot */
    i = lb+1;
    j = ub;
    while (1) 
	{
        while (i < j && strcmp(pivot.m_pszKey, (m_arrObjects + i)->m_pszKey) > 0) i++;
        while (j >= i && strcmp((m_arrObjects + j)->m_pszKey, pivot.m_pszKey) > 0) j--;
        if (i >= j) break;
        t = *(m_arrObjects + i);
        *(m_arrObjects + i) = *(m_arrObjects + j);
        *(m_arrObjects + j) = t;
        j--; i++;
    }

    /* pivot belongs in a[j] */
    *(m_arrObjects + lb) = *(m_arrObjects + j);
    *(m_arrObjects + j) = pivot;

    return j;
}

void GArray::QSort(__int64 lb, __int64 ub)
{
	__int64 m;
	while (lb < ub) 
	{
        /* quickly sort short lists */
        if (ub - lb <= 12) 
		{
            InsertSort(lb, ub);
            return;
        }
		
        /* partition into two segments */
        m = Partition(lb, ub);

        /* sort the smallest partition    */
        /* to minimize stack requirements */
        if (m - lb <= ub - m) 
		{
            QSort(lb, m - 1);
            lb = m + 1;
        } 
		else 
		{
            QSort(m + 1, ub);
            ub = m - 1;
        }
    }
}

void GArray::Sort()
{
	QSort(0, m_nNext - 1);
	m_bSorted = true;
}

const void *GArray::Search(const char *key) const
{
	if (m_bSorted)
	{
        const void *base = m_arrObjects;
        __int64 num = m_nNext;
        size_t width = sizeof(ObjectKey);

        char *lo = (char *)base;
        char *hi = (char *)base + (num - 1) * width;
        char *mid;
        __int64 half;
        int result;

        while (lo <= hi)
		{
            half = num / 2;
			if (half)
            {
                    mid = lo + (num & 1 ? half : (half - 1)) * width;
                    if (!(result = strcmp(key,(const char *)((ObjectKey *)mid)->m_pszKey)))
                        return(((ObjectKey *)mid)->m_pValue);
                    else if (result < 0)
                    {
                            hi = mid - width;
                            num = num & 1 ? half : half-1;
                    }
                    else    {
                            lo = mid + width;
                            num = half;
                    }
            }
            else if (num)
                    return(strcmp(key,(const char *)((ObjectKey *)lo)->m_pszKey) ? NULL : ((ObjectKey *)lo)->m_pValue);
            else
                    break;
		}

		return 0;
	}

	for (__int64 l = 0; l < m_nNext; l++)
	{
		ObjectKey *p = (m_arrObjects + l);
		if (strcmp(p->m_pszKey, key) == 0)
			return p->m_pValue;
	}

	return 0;
}

GArray::GArray(__int64 nSize, unsigned long nGrowBy) : m_KeySpace(64)
{
	m_nGrowBy = nGrowBy;
	m_nSize = nSize;
	m_arrObjects = 0;
	m_nCurrentKeySpaceIndex = 0;
	
	Initialize();
}

GArray::GArray(unsigned long nGrowBy)  : m_KeySpace(64)
{
	m_nGrowBy = nGrowBy;
	m_nSize = m_nGrowBy;
	m_arrObjects = 0;
	m_nCurrentKeySpaceIndex = 0;

	Initialize();
}

GArray::~GArray()
{
	if (!m_nDeferDestruction)
	{
//		for (__int64 i = 0; i < m_nNext; i++)
//			delete [] (*(m_arrObjects + i)).m_pszKey;
		char *pKeyBlock = (char *)m_KeySpace.Pop();
		while(pKeyBlock)
		{
			delete pKeyBlock;
			pKeyBlock = (char *)m_KeySpace.Pop();
		}

		delete [] m_arrObjects;
		m_nNext = 0;
	}
}
void GArray::DeferDestruction()
{
	m_nDeferDestruction = 1;
}

void GArray::Destruction()
{
	if (m_nDeferDestruction)
	{
//		for (__int64 i = 0; i < m_nNext; i++)
//			delete [] (*(m_arrObjects + i)).m_pszKey;
		char *pKeyBlock = (char *)m_KeySpace.Pop();
		while(pKeyBlock)
		{
			delete pKeyBlock;
			pKeyBlock = (char *)m_KeySpace.Pop();
		}


		delete [] m_arrObjects;
		m_nNext = 0;
	}
}