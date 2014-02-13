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

#ifndef __STACK_FRAME_CHK_H__
#define __STACK_FRAME_CHK_H__

#include "xmlDefines.h"

#include "GStack.h"

class StackFrameCheck
{
private:
	GStack *m_pStack;

	bool m_bOwner;
	bool m_bNestedFrame;
public:
	bool isNestedStackFrame()	{	return m_bNestedFrame;		}
	__int64 getStackSize()		{	return m_pStack->Size();	}


	StackFrameCheck( void *pCurrentFrame, StackFrameCheck *pStack = 0 );
	~StackFrameCheck(  );
};

#endif //__STACK_FRAME_CHK_H__
