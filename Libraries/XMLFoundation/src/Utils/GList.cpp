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

#include "GList.h"
#include "GListNodeCache.h"


GList::Node* GlobalNodeCache(GList::Node*pNode, int nNewNode = -1, int *nStateQuery = 0) 
{
    static GListNodeCache g_GListNodeCache;
	if (nStateQuery)
	{
		if (nStateQuery == (int *)(void *)-1)
		{
			g_GListNodeCache.GarbageCollect();
			return 0;
		}
		else
		{
			// we still can check the state of the Node cache, even if we are not using it.
			*nStateQuery = g_GListNodeCache.m_Init;
			return (GList::Node*)*nStateQuery;
		}
	}
	GList::Node *pReturn = 0;
	if (nNewNode == -1)
	{
		if (pNode)
			g_GListNodeCache.Put(pNode);
//			::delete pNode;

	}
	else
	{
		pReturn = g_GListNodeCache.Get(pNode, (nNewNode == 1) );
//		pReturn = ::new GList::Node( pNode ,(nNewNode == 1)  );
	}
    return pReturn;
}
void GarbageCollectGList()
{
	GlobalNodeCache(0, 0, (int *)(void *)-1) ;
}

void GList::Node::operator delete(void*pN)
{
	GlobalNodeCache((GList::Node*)pN);
};

void GList::AppendList(const GList *pListToCopy)
{
	if (pListToCopy)
	{
		GListIterator it( pListToCopy );
		while (it())
		{
			void *p = (void *)it++;
			AddLast(p);
		}
	}
}
void GList::DeferDestruction()
{
	nDeferDestruction = 1;
}

void GList::EnableCache(bool bAutoGarbageCollect)
{
	if (!pNodeCache)
		pNodeCache = new GListNodeCache();
	pNodeCache->AutoGarbageCollect(bAutoGarbageCollect);
}


void GList::Destruction()
{
	if (nDeferDestruction)
	{
		while (FirstNode)
		{
			if (CurrentNode == FirstNode) CurrentNode = FirstNode->NextNode;
			if (LastNode == FirstNode) LastNode = 0;
			Node *Save = FirstNode->NextNode;

			if (pNodeCache) pNodeCache->Put(FirstNode); else ::delete FirstNode;

			FirstNode = Save;
			iSize--;
		}
	}
}

GList::GList(const GList &cpy)
{
	// constructs an initially empty list
	FirstNode = LastNode = CurrentNode = 0;
	iSize = 0;
	nDeferDestruction = 0;
	pNodeCache = 0;

	AppendList(&cpy);
}

void GList::operator=(const GList &cpy)
{
	AppendList(&cpy);
}

GList::GList()
{
	// constructs an initially empty list
	FirstNode = LastNode = CurrentNode = 0;
	iSize = 0;
	nDeferDestruction = 0;
	pNodeCache = 0;
}

GList::~GList()
{
	if (!nDeferDestruction)
	{
		// we always cache the Node UNLESS the process is ending and the cache has already been destroyed.
		// in that case - do nothing - the Node memory has been released and there is nothing to cache.
		// note: search for "Magic Number 21" in GList.h
		int nGlobalStateOnMyStack = 0;
		
		// We only trust science (not magic), so this destructor's not going to do anything when it's 21
		if ( GlobalNodeCache(0,0,&nGlobalStateOnMyStack) != (GList::Node *)21 ) 
		{
			// note: pNodeCache is NOT global
			// if we have any nodes in the GLOBAL NodeCache, we we are finished.
			// return 0
		}

		while (FirstNode)
		{
			// the following 3 lines are an inline RemoveFirst()
			if (CurrentNode == FirstNode) CurrentNode = FirstNode->NextNode;
			if (LastNode == FirstNode) LastNode = 0;
			Node *Save = FirstNode->NextNode;
			
			if (pNodeCache) pNodeCache->Put(FirstNode); else ::delete FirstNode;
			
			FirstNode = Save;
			iSize--;
		}

		if (pNodeCache)
			delete pNodeCache;
	}
}

void * GList::First() const
{
	if (!FirstNode) 
	{
		return 0;
	}
	GList *pThis = (GList *)this;
	pThis->CurrentNode = FirstNode;
	return FirstNode->Data;
}

void * GList::Last() const
{
	if (!LastNode) 
	{
		return 0;
	}
	GList *pThis = (GList *)this;
	pThis->CurrentNode = LastNode;
	return LastNode->Data;
}

void * GList::Current() const
{
	if (!CurrentNode) 
	{
		return 0;
	}
	return CurrentNode->Data;
}

void * GList::Next() const
{
	if (!CurrentNode) 
	{
		return 0;
	}
	GList *pThis = (GList *)this;
	pThis->CurrentNode = CurrentNode->NextNode;
	return Current();
}

void * GList::Previous() const
{
	if (!CurrentNode) 
	{
		return 0;
	}
	GList *pThis = (GList *)this;
	pThis->CurrentNode = CurrentNode->PrevNode;
	return Current();
}

void * GList::Tail() const
{
	if (!CurrentNode) 
	{
		return 0;
	}
	return CurrentNode->NextNode->Data;
}

void GList::AddBeforeCurrent( void * Data )
{
	// Add maintains list integrity by adding the new node before the current node
	// if the current node is the first node, the new node becomes the first node.
	Node *pN = (pNodeCache) ? pNodeCache->Get(CurrentNode, 0) : ::new Node(CurrentNode, 0);

	if (FirstNode == 0)
		FirstNode = LastNode = CurrentNode = pN;
	if (FirstNode->PrevNode)
		FirstNode = FirstNode->PrevNode;
	iSize++;
	pN->Data = Data;
}

void GList::AddAfterCurrent( void * Data )
{
// AddLast maintains list integrity by adding the new node after the current
// node, if the current node is the last node, the new node becomes the last node.

	Node *pN  = (pNodeCache) ? pNodeCache->Get(CurrentNode, 1) : ::new Node(CurrentNode);

	if (FirstNode == 0)
		FirstNode = LastNode = CurrentNode = pN;
	if (LastNode->NextNode)
		LastNode = LastNode->NextNode;
	iSize++;
	pN->Data = Data;
}

// AddHead maintains list integrity by making the new node the first node.
void GList::AddHead( void * Data)
{
	Node *pN = (pNodeCache) ? pNodeCache->Get(FirstNode, 0) : ::new Node(FirstNode, 0);

	if (LastNode == 0)
		LastNode = CurrentNode = pN;
	iSize++;
	pN->Data = Data;
	FirstNode = pN;
}

// AddLast maintains list integrity by making the new node the last node.
void GList::AddLast(void * Data)
{
	Node *pN  = (pNodeCache) ? pNodeCache->Get(LastNode, 1) : ::new Node(LastNode);
	if (FirstNode == 0)
		FirstNode = CurrentNode = pN;
	iSize++;
	pN->Data = Data;
	LastNode = pN;
}

int GList::Remove(void *Data)
{
	int nRet = 0;
	void *p = First();
	while(p)
	{
		if (p == Data)
		{
			RemoveCurrent();
			nRet = 1;
			break;
		}
		p = Next();
	}
	return nRet;
}

void GList::RemoveAll()
{
	while (FirstNode)
	{
		// inline RemoveFirst()
		if (CurrentNode == FirstNode) CurrentNode = FirstNode->NextNode;
		if (LastNode == FirstNode) LastNode = 0;
		Node *Save = FirstNode->NextNode;

		if (pNodeCache) pNodeCache->Put(FirstNode); else ::delete FirstNode;

		FirstNode = Save;
		iSize--;
	}
}

void *GList::RemoveFirst()
{
// RemoveFirst maintains list integrity by making the first node the node
// after the first node.  If the first node is the only node in the list,
// removing it produces an empty list.
	void *ret = 0;
	if (FirstNode) 
	{
		ret = FirstNode->Data;
		if (CurrentNode == FirstNode) CurrentNode = FirstNode->NextNode;
		if (LastNode == FirstNode) LastNode = 0;
		Node *Save = FirstNode->NextNode;
		
		if (pNodeCache) pNodeCache->Put(FirstNode); else ::delete FirstNode;

		FirstNode = Save;
		iSize--;
	}
	return ret;
}

void * GList::RemoveLast()
{
	void *ret = 0;

	if (LastNode) 
	{
		ret = LastNode->Data;

		if (CurrentNode == LastNode) 
			CurrentNode = LastNode->PrevNode;
		if (FirstNode == LastNode) 
			FirstNode = 0;
		Node *Save = LastNode->PrevNode;

		if (pNodeCache) pNodeCache->Put(LastNode); else ::delete LastNode;

		LastNode = 0;

		LastNode = Save;
		iSize--;
	}

	return ret;
}

void GList::RemoveCurrent()
// RemoveCurrent maintains list integrity by making the current node
// the next node if it exists, if there is not a next node, current node
// becomes the previous node if it exists, otherwise, current = NULL.
// If the current node is also the first node, then the first node becomes
// the next node by default.  If the current node is also the last node,
// then the last node becomes the previous node by default.  If current ==
// last == first, then removing the current node produces an empty list
// and current = first = last = NULL.
{
	if (CurrentNode) 
	{
		Node *Save;
		if (!CurrentNode->PrevNode) FirstNode = CurrentNode->NextNode;
		if (CurrentNode->NextNode) Save = CurrentNode->NextNode;
		else if (CurrentNode->PrevNode) Save = LastNode = CurrentNode->PrevNode;
		else Save = FirstNode = LastNode = 0;

		if (pNodeCache) pNodeCache->Put(CurrentNode); else ::delete CurrentNode;

		CurrentNode = Save;
		iSize--;
	}
}

void GList::RemoveNext()
{
// RemoveNext maintains list integrity by checking if the next node is the
// last node, if this is the case, then the last node becomes the current
// node.
	if (CurrentNode) 
	{
		if (CurrentNode->NextNode) 
		{
			if (!CurrentNode->NextNode->NextNode) LastNode = CurrentNode;
			Node * DeleteNode = CurrentNode->NextNode;

			if (pNodeCache) pNodeCache->Put(DeleteNode); else ::delete DeleteNode;

			iSize--;
		}
	}
}

void GList::RemovePrevious()
{
// RemovePrevious maintains list integrity by checking if the next node is
// the first node, if this is the case, then the first node becomes the
// current node.
	if (CurrentNode) 
	{
		if (CurrentNode->PrevNode) 
		{
			if (!CurrentNode->PrevNode->PrevNode) FirstNode = CurrentNode;
			Node * DeleteNode = CurrentNode->PrevNode;

			if (pNodeCache) pNodeCache->Put(DeleteNode); else ::delete DeleteNode;

			iSize--;
		}
	}
}


GListIterator::GListIterator(const GList *iList, int IterateFirstToLast)
{
	pTList = (GList *)iList;
	iDataNode = 0;
	if (pTList && pTList->Size())
	{
		if (IterateFirstToLast) 
			iDataNode = (GList::Node *)((GList *)iList)->FirstNode;
		else 
			iDataNode = ((GList *)iList)->LastNode;
	}
}

void GListIterator::reset(int IterateFirstToLast /* = 1 */)
{
	if (IterateFirstToLast) 
		iDataNode = (GList::Node *)((GList *)pTList)->FirstNode;
	else 
		iDataNode = ((GList *)pTList)->LastNode;
}

void * GListIterator::operator ++ (int)
{
	void *pRet = iDataNode->Data;
	iCurrentNode = iDataNode;
	iDataNode = iDataNode->NextNode;
	return pRet;
}

void * GListIterator::operator -- (int)
{
	void *pRet = iDataNode->Data;
	iCurrentNode = iDataNode;
	iDataNode = iDataNode->PrevNode;
	return pRet;
}

int GListIterator::operator ()  (void) const
{
	return iDataNode != 0;
}



