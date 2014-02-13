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

#ifndef __GSTRINGSTACK_H___
#define __GSTRINGSTACK_H___

#include "GList.h"
#include "GStack.h"
#include "GString.h"

class GStringStack
{
	GStack m_stk;
	GList  m_lst;

public:

	// Construct an empty stack
	GStringStack(unsigned long growBy) :
	  m_stk(growBy)
	{
	};
	
	// Destroy the stack
	~GStringStack() 
	{
		GListIterator it(&m_lst);
		while (it())
			delete (GString *)it++;
	};
	
	// Returns the number of entries in the stack
	inline __int64 Size(void) const { return m_stk.Size(); } 
	
	// Returns TRUE if the stack is empty, otherwise FALSE
	inline bool isEmpty(void) const { return m_stk.isEmpty(); } 
	
	// Removes and returns the item at the top of the stack, or returns NULL if the stack is empty
	inline GString & Pop(void)
	{
		return *(GString *)m_stk.Pop();
	}
 
	inline void Push(const char *p)
	{
		GString *strNew = new GString(p);
		m_stk.Push(strNew);
		m_lst.AddLast(strNew);
	}

	inline void Push(GString &p)
	{
		Push((const char *)p);
	}

	// Adds an iten to the top of the stack
	inline void Push(GString *p)
	{
		Push(*p);
	} 
	
	// Returns the item at the top of the stack, or returns NULL if the stack is empty
	inline GString &Top(void)
	{ 
		return *(GString *)m_stk.Top();
	} 

	inline GString &Bottom(void)
	{
		return *(GString *)m_stk.Bottom();
	}
};

#endif // __GSTRINGSTACK_H___
