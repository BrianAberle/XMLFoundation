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
#ifndef __GB_TREE_H__
#define __GB_TREE_H__

#include "GStack.h"

enum balanceState { leftTilt, noTilt, rightTilt };

class GBTree
{
private:
	int m_bCaseInsensetive;
	int TreeStrCmp(const char *p1, const char *p2);
	struct GBTreeStruct
	{
		GBTreeStruct    *m_leftPtr;
		GBTreeStruct    *m_rightPtr;
		GBTreeStruct	*m_Next;
		GBTreeStruct	*m_Prev;
		char			*m_szKey;
		void			*m_dataNode;
		balanceState m_balance;
	};
protected:

	bool m_insertedOK;
	int m_nDeferDestruction;

	// special node pointer used in deletion
	GBTreeStruct *m_nodeMarker;

	void rightRotate(GBTreeStruct * &rootPtr);
	void leftRotate(GBTreeStruct * &rootPtr);
	void rightBalance(GBTreeStruct * &rootPtr, bool &delOK);
	void leftBalance(GBTreeStruct * &rootPtr, bool &delOK);
	void deleteBothChildren(GBTreeStruct * &rootPtr, GBTreeStruct * &Ptr, bool &delOK);
	void insertNode(GBTreeStruct * &rootPtr, const char *szKey, void *value);
	void removeNode(GBTreeStruct * &rootPtr,  bool &delOK,	 const char *szKey, void *pData);
	void removeNode(GBTreeStruct * &rootPtr, __int64 &occur, bool &delOK, const char *szKey);
	void *searchTree(const char *szKey, __int64 occur = 1);
	bool delSubTree(GBTreeStruct * &rootPtr);

	__int64 m_numNodes;

	GBTreeStruct *m_nodePtr, *m_root, *m_Last, *m_First;
	friend class GBTreeIterator;

public:

	GBTree(int bCaseInsensetive = 0);
	~GBTree();

	__int64 getNodeCount() const { return m_numNodes; }
	__int64	getOccurCount(const char *szKey);

	void insert(const char *szKey, void *value);
	void *search(const char *szKey, __int64 occur = 1)	{ return searchTree(szKey, occur); }
	bool remove(const char *szKey, void *pOldData);
	bool remove(const char *szKey, __int64 occur = 1);
	int reassignKeyPair(const char *szKey, void *pOldData, void*pNewData);
	void *removeGet(const char *szKey, __int64 occur = 1) {void *p=search(szKey,occur);if(p)remove(szKey,occur); return p;}
	void clear();
	bool isEmpty(){return (m_numNodes == 0) ? true : false; }

	void DeferDestruction();
	void Destruction();

};



class GBTreeIterator
{
	void *m_pLookAhead;
	int m_Direction;
	GBTree *m_pTree;

	// note that we do NOT walk this  - we index(),add(),and remove() @ the tail.
	GStack m_strTreeStack;
	GStack m_strFrameLocStack;

	GBTree::GBTreeStruct *m_pInsertionOrderCurrent;
	void *IncrementIterator();
public:
	// nDirection values:  1 = Alphabetical, 0 = Reverse Alphabetical, 2 = Insertion Order
	// stackSize can be pre-allocated for HBase/Cassandra sized datasets.
	// Note: stackSize is NOT == to the count of items in the BTree, it is far less than that.
	//       It is merely the count of items in the longest path from Root to Leaf.
	//		 Also consider that the tree IS balanced so the longest path is NEVER a 
	//       "B-Tree Worst Case Scenario" which essentially degrades a B-Tree to a recursively linked list.
	GBTreeIterator(GBTree *p = 0, int nDirection = 2, __int64 stackSize = 1024, __int64 stackGrowBy = 8192);
	void Reset(GBTree *p, int nDirection);

	int operator () (void);
	void * operator ++ (int);
};



#endif //  __GB_TREE_H__
