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

#include "GBTree.h"
#include "GString.h" // only needed for stricmp() definition under WinCE
#include <string.h> // for:strlen(), strcpy(), strcmp()

#ifdef _WIN32
	#define strcasecmp	_stricmp
#else
	#include <strings.h> // for strcasecmp()
#endif


int GBTree::TreeStrCmp(const char *p1, const char *p2)
{
	if (m_bCaseInsensetive)
	{
		return strcasecmp(p1,p2);
	}
	return strcmp(p1,p2);
}


void GBTree::DeferDestruction()
{
	m_nDeferDestruction = 1;
}

void GBTree::Destruction()
{
	if (m_nDeferDestruction)
	{
		if (m_root) 
			clear(); 
	}
}

GBTree::~GBTree()
{
	if (!m_nDeferDestruction)
	{
		if (m_root) 
			clear(); 
	}
}


GBTree::GBTree(int bCaseInsensetive ) :
	m_root(0),
	m_numNodes(0),
	m_Last(0),
	m_First(0),
	m_nDeferDestruction(0)

{
	m_bCaseInsensetive = bCaseInsensetive;
}

void GBTree::clear() 
{ 
	GBTreeStruct *curr = m_First;
	while(curr)
	{
		delete [] curr->m_szKey;
		GBTreeStruct *d = curr;
		curr = curr->m_Next;
		delete d;
	}

	m_First = 0;
	m_root = 0;
	m_numNodes = 0;
}

void GBTree::rightRotate(GBTreeStruct * &rootPtr)
{
	GBTreeStruct *ptr2, *ptr3;

	ptr2 = rootPtr->m_rightPtr;
	if (ptr2->m_balance == rightTilt)
	{
		// rotate once
		rootPtr->m_rightPtr = ptr2->m_leftPtr;
		ptr2->m_leftPtr = rootPtr;
		rootPtr->m_balance = noTilt;
		rootPtr = ptr2;
	}
	else
	{
		// rotate twice
		ptr3 = ptr2->m_leftPtr;
		ptr2->m_leftPtr = ptr3->m_rightPtr;
		ptr3->m_rightPtr = ptr2;
		rootPtr->m_rightPtr = ptr3->m_leftPtr;
		ptr3->m_leftPtr = rootPtr;
		ptr2->m_balance = (ptr3->m_balance == leftTilt) ? rightTilt : noTilt;
		rootPtr->m_balance = (ptr3->m_balance == rightTilt) ? leftTilt : noTilt;
		rootPtr = ptr3;
	}
	rootPtr->m_balance = noTilt;
}

void GBTree::leftRotate(GBTreeStruct * &rootPtr)
{
	GBTreeStruct *ptr2, *ptr3;

	ptr2 = rootPtr->m_leftPtr;
	if (ptr2->m_balance == leftTilt)
	{
		// rotate once
		rootPtr->m_leftPtr = ptr2->m_rightPtr;
		ptr2->m_rightPtr = rootPtr;
		rootPtr->m_balance = noTilt;
		rootPtr = ptr2;
	}
	else
	{
		// rotate twice
		ptr3 = ptr2->m_rightPtr;
		ptr2->m_rightPtr = ptr3->m_leftPtr;
		ptr3->m_leftPtr = ptr2;
		rootPtr->m_leftPtr = ptr3->m_rightPtr;
		ptr3->m_rightPtr = rootPtr;
		ptr2->m_balance = (ptr3->m_balance == rightTilt) ? leftTilt : noTilt;
		rootPtr->m_balance = (ptr3->m_balance == leftTilt) ? rightTilt : noTilt;
		rootPtr = ptr3;
	}
	rootPtr->m_balance = noTilt;
}

void GBTree::insertNode(GBTreeStruct * &rootPtr, const char *szKey, void *value)
{
	if (!rootPtr)
	{
		rootPtr = new GBTreeStruct;
		if (!rootPtr)
		{
			// throw GException();
		}

		// copy data
		rootPtr->m_dataNode = value;
		rootPtr->m_leftPtr  = 0;
		rootPtr->m_rightPtr = 0;
		rootPtr->m_balance  = noTilt;
		rootPtr->m_szKey    = new char[strlen(szKey) + 1];
		strcpy(rootPtr->m_szKey, szKey);

		if (!m_First)
			m_First = rootPtr;

		rootPtr->m_Prev = m_Last;
		rootPtr->m_Next = 0;
		if (m_Last)
			m_Last->m_Next = rootPtr;
		m_Last = rootPtr;

		m_numNodes++;
		m_insertedOK = 1;
	}
	else if (TreeStrCmp(szKey, rootPtr->m_szKey) < 0)
	{
		insertNode(rootPtr->m_leftPtr, szKey, value);
		if (m_insertedOK)
		{
			switch (rootPtr->m_balance)
			{
			case leftTilt :
				leftRotate(rootPtr);
				m_insertedOK = 0;
				break;
			case noTilt :
				rootPtr->m_balance = leftTilt;
				break;
			case rightTilt :
				rootPtr->m_balance = noTilt;
				m_insertedOK = 0;
				break;
			}
		}
	}
	else
	{
		insertNode(rootPtr->m_rightPtr, szKey, value);
		if (m_insertedOK)
		{
			switch (rootPtr->m_balance)
			{
			case leftTilt :
				rootPtr->m_balance = noTilt;
				m_insertedOK = 0;
				break;
			case noTilt :
				rootPtr->m_balance = rightTilt;
				break;
			case rightTilt :
				rightRotate(rootPtr);
				m_insertedOK = 0;
				break;
			}
		}
	}
}


__int64 GBTree::getOccurCount(const char *szKey)
{
	__int64 count = 0;
	GBTreeStruct *p = 0;
	p = m_root;
	while (p)
	{
		int nDir = TreeStrCmp(szKey, p->m_szKey);
		if (nDir > 0)
		{
			p = p->m_rightPtr;
		}
		else if (nDir < 0)
		{
			p = p->m_leftPtr;
		}
		else
		{
			// found a match
			count++;
			p = p->m_leftPtr;
		}
	}
	return count;
}

int GBTree::reassignKeyPair(const char *szKey, void *pOldData, void*pNewData)
{
	GBTreeStruct *p = 0;

	p = m_root;
	while (p)
	{
		int nDir = TreeStrCmp(szKey, p->m_szKey);
		if (nDir > 0)
		{
			p = p->m_rightPtr;
		}
		else if (nDir < 0)
		{
			p = p->m_leftPtr;
		}
		else
		{
			// found a match
			if (pOldData != p->m_dataNode)
			{
				p = p->m_leftPtr; // handle duplicate keys
			}
			else
			{
				p->m_dataNode = pNewData;
				
				// if we are re-assigning the key pair data to -1, it is marked as 'free' or 'deleted' but remains in the tree.
				if (pNewData == (void *)-1)
					m_numNodes--;
				if (m_numNodes == 0)
					m_root = 0;
				return 1;
			}
		}
	}

	return 0;
}

void *GBTree::searchTree(const char *szKey, __int64 occur /* = 1 */)
{
	__int64 count = 0;

	GBTreeStruct *p = 0;

	occur = (occur == 0) ? 1 : occur;
	if (occur > m_numNodes)
	{
		return 0;
	}

	p = m_root;
	while (p && count < occur)
	{
		int nDir = TreeStrCmp(szKey, p->m_szKey);
		if (nDir > 0)
		{
			p = p->m_rightPtr;
		}
		else if (nDir < 0)
		{
			p = p->m_leftPtr;
		}
		else
		{
			// found a match
			count++;
			if (count < occur)
			{
				p = p->m_leftPtr;
			}
		}
	}

	if (p)
		return p->m_dataNode;
	return 0;
}

void GBTree::rightBalance(GBTreeStruct * &rootPtr, bool &delOK)
{
	GBTreeStruct *ptr2, *ptr3;
	balanceState balance, balnc3;

	switch (rootPtr->m_balance)
	{
	case leftTilt :
		rootPtr->m_balance = noTilt;
		break;
	case noTilt :
		rootPtr->m_balance = rightTilt;
		delOK = 0;
		break;
	case rightTilt :
		ptr2 = rootPtr->m_rightPtr;
		balance = ptr2->m_balance;
		if (balance != leftTilt)
		{
			rootPtr->m_rightPtr = ptr2->m_leftPtr;
			ptr2->m_leftPtr = rootPtr;
			if (balance == noTilt)
			{
				rootPtr->m_balance = rightTilt;
				ptr2->m_balance = leftTilt;
				delOK = 0;
			}
			else
			{
				rootPtr->m_balance = noTilt;
				ptr2->m_balance = noTilt;
			}
			rootPtr = ptr2;
		}
		else
		{
			ptr3 = ptr2->m_leftPtr;
			balnc3 = ptr3->m_balance;
			ptr2->m_leftPtr = ptr3->m_rightPtr;
			ptr3->m_rightPtr = ptr2;
			rootPtr->m_rightPtr = ptr3->m_leftPtr;
			ptr3->m_leftPtr = rootPtr;
			ptr2->m_balance = (balnc3 == leftTilt) ? rightTilt : noTilt;
			rootPtr->m_balance = (balnc3 == rightTilt) ? leftTilt : noTilt;
			rootPtr = ptr3;
			ptr3->m_balance = noTilt;
		}
		break;
	}
}

void GBTree::leftBalance(GBTreeStruct * &rootPtr, bool &delOK)
{
	GBTreeStruct *ptr2, *ptr3;
	balanceState balance, balnc3;

	switch (rootPtr->m_balance)
	{
	case rightTilt :
		rootPtr->m_balance = noTilt;
		break;
	case noTilt :
		rootPtr->m_balance = leftTilt;
		delOK = 0;
		break;
	case leftTilt :
		ptr2 = rootPtr->m_leftPtr;
		balance = ptr2->m_balance;
		if (balance != rightTilt)
		{
			rootPtr->m_leftPtr = ptr2->m_rightPtr;
			ptr2->m_rightPtr = rootPtr;
			if (balance == noTilt)
			{
				rootPtr->m_balance = leftTilt;
				ptr2->m_balance = rightTilt;
				delOK = 0;
			}
			else
			{
				rootPtr->m_balance = noTilt;
				ptr2->m_balance = noTilt;
			}
			rootPtr = ptr2;
		}
		else
		{
			ptr3 = ptr2->m_rightPtr;
			balnc3 = ptr3->m_balance;
			ptr2->m_rightPtr = ptr3->m_leftPtr;
			ptr3->m_leftPtr = ptr2;
			rootPtr->m_leftPtr = ptr3->m_rightPtr;
			ptr3->m_rightPtr = rootPtr;
			ptr2->m_balance = (balnc3 == rightTilt) ? leftTilt : noTilt;
			rootPtr->m_balance = (balnc3 == leftTilt) ? rightTilt : noTilt;
			rootPtr = ptr3;
			ptr3->m_balance = noTilt;
		}
		break;
	}
}

void GBTree::deleteBothChildren(GBTreeStruct * &rootPtr, 
								 GBTreeStruct * &ptr, 
								 bool &delOK)
{
	if (!ptr->m_rightPtr)
	{
		rootPtr->m_dataNode = ptr->m_dataNode;
		
		delete rootPtr->m_szKey;
		rootPtr->m_szKey = ptr->m_szKey;
		ptr->m_szKey = 0;

		m_nodeMarker = ptr;

		if (ptr == m_First) 
			m_First = ptr;
		if (ptr == m_Last) 
			m_Last = ptr->m_Prev;

		if (ptr->m_Prev) 
			ptr->m_Prev->m_Next = ptr->m_Next;

		if (ptr->m_Next) 
			ptr->m_Next->m_Prev = ptr->m_Prev;

		ptr = ptr->m_leftPtr;

		delOK = 1;
	}
	else
	{
		deleteBothChildren(rootPtr, ptr->m_rightPtr, delOK);
		if (delOK)
		{
			leftBalance(ptr, delOK);
		}
	}
}

void GBTree::removeNode(GBTreeStruct * &rootPtr,  __int64 &occur, 	 bool &delOK,	 const char *szKey)
{
	GBTreeStruct *p;

	if (!rootPtr)
	{
		delOK = 0;
	}
	else
	{
		int nMatched = TreeStrCmp(szKey, rootPtr->m_szKey);
		if (nMatched == 0)
		{
			occur--;
		}

		if (occur > 0 && nMatched <= 0)
		{
			removeNode(rootPtr->m_leftPtr, occur, delOK, szKey);
			if (delOK)
			{
				rightBalance(rootPtr, delOK);
			}
		}
		else if (nMatched > 0)
		{
			removeNode(rootPtr->m_rightPtr, occur, delOK, szKey);
			if (delOK)
			{
				leftBalance(rootPtr, delOK);
			}
		}
		else
		{
			p = rootPtr;
			if (!rootPtr->m_rightPtr)
			{
				rootPtr = rootPtr->m_leftPtr;
				delOK = 1;

				if (p == m_First) 
					m_First = p->m_Next;
				if (p == m_Last) 
					m_Last = p->m_Prev;
				if (p->m_Prev) 
					p->m_Prev->m_Next = p->m_Next;
				if (p->m_Next) 
					p->m_Next->m_Prev = p->m_Prev;

				delete p->m_szKey;
				p->m_szKey = 0;
				delete p;
				p = 0;
			}
			else if (!rootPtr->m_leftPtr)
			{
				rootPtr = rootPtr->m_rightPtr;
				delOK = 1;

				if (p == m_First) 
					m_First = p->m_Next;
				if (p == m_Last) 
					m_Last = p->m_Prev;
				if (p->m_Prev) 
					p->m_Prev->m_Next = p->m_Next;
				if (p->m_Next) 
					p->m_Next->m_Prev = p->m_Prev;
				
				delete p->m_szKey;
				p->m_szKey = 0;
				delete p;
				p = 0;
			}
			else
			{
				deleteBothChildren(rootPtr, rootPtr->m_leftPtr, delOK);

				if (delOK)
				{
					rightBalance(rootPtr, delOK);
				}
				delete m_nodeMarker;
			}
			m_numNodes--;
		}
	}
}

void GBTree::removeNode(GBTreeStruct * &rootPtr,  bool &delOK,	 const char *szKey, void *pData)
{
	GBTreeStruct *p;

	if (!rootPtr)
	{
		delOK = 0;
	}
	else
	{
		int nMatched = TreeStrCmp(szKey, rootPtr->m_szKey);

		if (nMatched <= 0 && rootPtr->m_dataNode != pData)
		{
			removeNode(rootPtr->m_leftPtr, delOK, szKey, pData);
			if (delOK)
			{
				rightBalance(rootPtr, delOK);
			}
		}
		else if (nMatched > 0)
		{
			removeNode(rootPtr->m_rightPtr, delOK, szKey, pData);
			if (delOK)
			{
				leftBalance(rootPtr, delOK);
			}
		}
		else
		{
			p = rootPtr;
			if (!rootPtr->m_rightPtr)
			{
				rootPtr = rootPtr->m_leftPtr;
				delOK = 1;

				if (p == m_First) 
					m_First = p->m_Next;
				if (p == m_Last) 
					m_Last = p->m_Prev;
				if (p->m_Prev) 
					p->m_Prev->m_Next = p->m_Next;
				if (p->m_Next) 
					p->m_Next->m_Prev = p->m_Prev;

				delete p->m_szKey;
				p->m_szKey = 0;
				delete p;
				p = 0;
			}
			else if (!rootPtr->m_leftPtr)
			{
				rootPtr = rootPtr->m_rightPtr;
				delOK = 1;

				if (p == m_First) 
					m_First = p->m_Next;
				if (p == m_Last) 
					m_Last = p->m_Prev;
				if (p->m_Prev) 
					p->m_Prev->m_Next = p->m_Next;
				if (p->m_Next) 
					p->m_Next->m_Prev = p->m_Prev;
				
				delete p->m_szKey;
				p->m_szKey = 0;
				delete p;
				p = 0;
			}
			else
			{
				deleteBothChildren(rootPtr, rootPtr->m_leftPtr, delOK);

				if (delOK)
				{
					rightBalance(rootPtr, delOK);
				}
				delete m_nodeMarker;
			}
			m_numNodes--;
		}
	}
}


bool GBTree::remove(const char *szKey, void *pOldData)
{
	__int64 oldCount = m_numNodes;
	bool delOK = 0;
	removeNode(m_root, delOK, szKey, pOldData);
	return (oldCount > m_numNodes) ? 1 : 0;
}


bool GBTree::remove(const char *szKey, __int64 occur /* = 1 */)
{
	__int64 oldCount = m_numNodes;
	bool delOK = 0;

	occur = (occur == 0) ? 1 : occur;
	if (occur > m_numNodes)
		return 0;

	removeNode(m_root, occur, delOK, szKey);

	return (oldCount > m_numNodes) ? 1 : 0;
}

bool GBTree::delSubTree(GBTreeStruct * &rootPtr)
{
	bool delThisNode = 0;

	if (!rootPtr->m_leftPtr && !rootPtr->m_rightPtr)
	{
		delThisNode = 1;
	}
	else
	{
		if (rootPtr->m_leftPtr)
		{
			if (delSubTree(rootPtr->m_leftPtr))
			{
				delete rootPtr->m_leftPtr->m_szKey;
				delete rootPtr->m_leftPtr;
				rootPtr->m_leftPtr = 0;
			}
		}

		if (rootPtr->m_rightPtr)
		{
			if (delSubTree(rootPtr->m_rightPtr))
			{
				delete rootPtr->m_rightPtr->m_szKey;
				delete rootPtr->m_rightPtr;
				rootPtr->m_rightPtr = 0;
			}
		}

		delThisNode = (!rootPtr->m_leftPtr && !rootPtr->m_rightPtr) ? 1 : 0;
	}

	return delThisNode;
}

void GBTree::insert(const char *szKey, void *value)
{
	m_insertedOK = 0;
	insertNode(m_root, szKey, value);
}


//
//
// The remainder of this Source file is the implementation of GBTreeIterator
//
//


#if defined(_LINUX64) || defined(_WIN64)  || defined(_IOS)
	static const __int64 _GStack0 = 0;
	static const __int64 _GStack1 = 1;
	static const __int64 _GStack2 = 2;
	static const __int64 _GStack3 = 3;
	static const __int64 _GStack4 = 4;
#else
	static const int _GStack0 = 0;
	static const int _GStack1 = 1;
	static const int _GStack2 = 2;
	static const int _GStack3 = 3;
	static const int _GStack4 = 4;
#endif



void GBTreeIterator::Reset(GBTree *p, int nDirection)
{
	m_pTree = p;
	m_Direction = nDirection;
	m_pInsertionOrderCurrent = 0;
	m_pLookAhead =0;
	
	GStackRemoveAll(m_strTreeStack);
	GStackRemoveAll(m_strFrameLocStack);
	GStackPush(m_strTreeStack,m_pTree->m_root);
	GStackPush(m_strFrameLocStack,(void *)_GStack0 );
}

// supply a tree, or reset() this iterator before re-using it.
GBTreeIterator::GBTreeIterator(GBTree *p, int nDirection, __int64 stackSize, __int64 stackGrowBy) 
	: m_strTreeStack(stackSize, stackGrowBy), m_strFrameLocStack(stackSize, stackGrowBy)
{
	m_pTree = p;
	m_Direction = nDirection;
	m_pInsertionOrderCurrent = 0;
	m_pLookAhead =0;
	
	if (m_pTree) 
	{
		GStackPush(m_strTreeStack,m_pTree->m_root);
		GStackPush(m_strFrameLocStack,(void *)_GStack0 );
	}
}

int GBTreeIterator::operator ()  (void)
{
	if (m_pLookAhead)
		return 1;
	else
		m_pLookAhead = (*this)++;
	return (m_pLookAhead == 0) ? 0 : 1;
}


// stacked recursion
void *GBTreeIterator::IncrementIterator()
{
	if (m_pLookAhead)
	{
		void *pRet = m_pLookAhead;
		m_pLookAhead = 0;
		return pRet;
	}

	if (m_Direction == 2 )
	{
		if (!m_pInsertionOrderCurrent)
			m_pInsertionOrderCurrent = m_pTree->m_First;
		else
			m_pInsertionOrderCurrent = m_pInsertionOrderCurrent->m_Next;
		return (m_pInsertionOrderCurrent) ? m_pInsertionOrderCurrent->m_dataNode : 0;
	}

	// load the current GBTreeStruct for this stack frame
	GStackPopType(m_strTreeStack, btRoot, GBTree::GBTreeStruct *);


	if (!btRoot)
		return 0;

	// load the integer that tells us where to start or resume this stack frame
	union CAST_THIS_TYPE_SAFE_COMPILERS
	{
		void *			 mbrVoid;
		unsigned int	 mbrInt;
		unsigned __int64 mbrInt64;
	}Member;  


//	..............  This is GStackPopType() manually expanded ..............
	if (m_strFrameLocStack.m_nNext > 0)
	{
		m_strFrameLocStack.m_nNext--;
		Member.mbrVoid = (void *)m_strFrameLocStack.m_arrPtr[m_strFrameLocStack.m_nNext];
		m_strFrameLocStack.m_arrPtr[m_strFrameLocStack.m_nNext] = 0;
	}
	else
	{
		Member.mbrVoid = (void *)_GStack0;	
	}								
//	..............  This is GStackPopType() manually expanded ..............




	switch (Member.mbrInt) 
	{
	case 1: goto LOCATION1;
	case 2: goto LOCATION2;
	case 3: goto LOCATION3;
	case 4: goto LOCATION4;
	}

LOCATION1:
	
	// Move either Left or Right down the tree depending on [Pos.m_Direction]
	if (m_Direction != 0) 
	{
		if( btRoot->m_leftPtr )
		{
			GStackPop(m_strFrameLocStack);					// update the frame position location
			GStackPush(m_strFrameLocStack,(void *)_GStack2 );
			GStackPush(m_strTreeStack,btRoot->m_leftPtr);	// prepare the next frame
			GStackPush(m_strFrameLocStack,(void *)_GStack1 );

			return (*this)++;								// load the next frame
		}
	}
	else 
	{
		if( btRoot->m_rightPtr )
		{
			GStackPop(m_strFrameLocStack);					// update the frame position location
			GStackPush(m_strFrameLocStack,(void *)_GStack2 );
			GStackPush(m_strTreeStack,btRoot->m_rightPtr);	// prepare the next frame
			GStackPush(m_strFrameLocStack,(void *)_GStack1 );

			return (*this)++;								// load the next frame
		}
	}
LOCATION2:
	if (m_Direction == 1 || m_Direction == 0)
	{
		GStackPop(m_strFrameLocStack);		// update the frame position location
		GStackPush(m_strFrameLocStack,(void *)_GStack3 );

		return btRoot->m_dataNode;			// unwind all frames, return the result
	}

LOCATION3:
	// Move either Left or Right down the tree depending on [Pos.m_Direction]
	if (m_Direction != 0)
	{
		if( btRoot->m_rightPtr )
		{
			GStackPop(m_strFrameLocStack);		// same as above, but opposite direction
			GStackPush(m_strFrameLocStack,(void *)_GStack4 );
			GStackPush(m_strTreeStack,btRoot->m_rightPtr); 
			GStackPush(m_strFrameLocStack,(void *)_GStack1 );

			return (*this)++;
		}
	}
	else 
	{
		if( btRoot->m_leftPtr )
		{
			GStackPop(m_strFrameLocStack);
			GStackPush(m_strFrameLocStack,(void *)_GStack4 );
			GStackPush(m_strTreeStack,btRoot->m_leftPtr); 
			GStackPush(m_strFrameLocStack,(void *)_GStack1 );

			return (*this)++;
		}
	}
LOCATION4:
	
	// pop this frame, and resume the previous frame in it's last state
	GStackPop(m_strTreeStack);
	GStackPop(m_strFrameLocStack);		

	return (*this)++;
}


void * GBTreeIterator::operator ++ (int)
{
	void *pRet = IncrementIterator();
	while(pRet == (void *)-1)
	{
		pRet = IncrementIterator();
	}
	return pRet;

}





