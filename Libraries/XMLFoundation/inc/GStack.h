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

#ifndef __GSTACK_H___
#define __GSTACK_H___

#include "xmlDefines.h"

class GStack
{
// For fastest access, use the accessor macros following this class declaration
// xmlLex.cpp shows how the accessor macros are used.
public:
	bool grow();
	unsigned __int64 m_nNext;
	void **m_arrPtr;
	__int64 m_nGrowBy; // by leaving it signed, opposed to unsigned __int64, we reserve special negative values for special behavior in special cases.  See how negatives are used in CFileDataSource::SetConnectionInfo()
	unsigned __int64 m_nSize;
public:
	// Construct an empty stack - with no allocation - if nothing is ever added, no memory is ever used.
	GStack(__int64 growBy) :  m_nGrowBy(growBy)
	{
		m_arrPtr = 0;
		m_nSize = m_nNext = 0;
	};

	// Construct an empty stack - with an initial memory pre-allocation
	GStack(__int64 initialSize, __int64 growBy);


	// Destroy the stack, does not do memory cleanup of the items in the stack, only the stack itself.
	~GStack() 
	{
		delete [] m_arrPtr;
		m_arrPtr = 0;
		m_nSize = m_nNext = 0;
	};

	// Empty the stack, does not do memory cleanup of the items in the stack.
	void RemoveAll(void) { m_nNext = 0; }
	
	// Returns the number of entries in the stack
	inline __int64 Size(void) const { return m_nNext; } 
	
	// Returns TRUE if the stack is empty, otherwise FALSE
	inline bool isEmpty(void) const { return m_nNext == 0; } 
	
	// Removes and returns the item at the top of the stack, or returns NULL if the stack is empty
	inline void * Pop(void)
	{
		register void *p;

		if (m_nNext > 0)
		{
			m_nNext--;
			p = m_arrPtr[m_nNext];
			m_arrPtr[m_nNext] = 0;
		}
		else
		{
			p = 0;
		}

		return p;
	}
 
	// Adds an item to the top of the stack
	inline void Push(void *p)
	{ 
		if (m_nNext >= m_nSize)
		{
			grow();
		}

		m_arrPtr[m_nNext] = p;
		m_nNext++;
	} 
	
	// Returns the item at the top of the stack, or returns NULL if the stack is empty
	inline void * Top(void)
	{ 
		register void *p;

		if (m_nNext > 0)
		{
			p = m_arrPtr[(m_nNext - 1)];
		}
		else
		{
			p = 0;
		}
		return p;
	} 

	inline void * Bottom(void)
	{
		return m_arrPtr[0];
	}
};



// ----------- Accessor macros ------------------

#define GStackPush(stack,item)					\
	if (stack.m_nNext >= stack.m_nSize)			\
		stack.grow();							\
	stack.m_arrPtr[stack.m_nNext] = item;		\
	stack.m_nNext++;							\


#define GStackPopType(stack,item,type)			\
		type item;								\
		if (stack.m_nNext > 0)					\
		{										\
			stack.m_nNext--;					\
			item = (type)stack.m_arrPtr[stack.m_nNext];	\
			stack.m_arrPtr[stack.m_nNext] = 0;	\
		}										\
		else									\
		{										\
			item = (type)0;						\
		}								

#define GStackPop(stack)						\
		if (stack.m_nNext > 0)					\
		{										\
			stack.m_nNext--;					\
			stack.m_arrPtr[stack.m_nNext] = 0;	\
		}										


#define GStackTopType(stack,item,type)			\
		if (stack.m_nNext > 0)					\
			item = (type)stack.m_arrPtr[(stack.m_nNext - 1)];	\
		else									\
		{										\
			item = (type)0;						\
		}										


#define GStackRemoveAll(stack)					\
		{										\
			stack.m_nNext = 0;					\
		}										

#endif // __GSTACK_H___

