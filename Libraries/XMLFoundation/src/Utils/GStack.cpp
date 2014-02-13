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

#include "GStack.h"
#include <string.h> // for: memset(), memcpy()


bool GStack::grow()
{
	bool bRet = false;

	void **p;
	m_nSize += m_nGrowBy;
	p = new void * [m_nSize];

	if (p != 0)
	{	
		if (m_arrPtr)
		{
			// here we initialize the new memory from the previous - the remaining new space is uninitialized
			memcpy(p, m_arrPtr, (m_nSize - m_nGrowBy) * sizeof(void *) );

			// delete the previous space
			delete [] m_arrPtr;
		}
		// now set m_arrPtr pointing to the new space
		m_arrPtr = p;
		bRet = true;
	}

	return bRet;
}

GStack::GStack(__int64 initialSize, __int64 growBy)
{
	m_nGrowBy = growBy;
	m_nSize = initialSize;
	m_arrPtr = new void * [m_nSize];
	m_nNext = 0;
 }




