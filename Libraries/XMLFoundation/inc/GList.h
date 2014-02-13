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

#ifndef _GLIST_H____
#define _GLIST_H____

class XMLObject;
#include "xmlDefines.h"  // for __int64 definition on non-Windows builds

// this is only for building MFC apps
#ifdef new
	#undef new
#endif

#ifndef _WIN32
#include <stddef.h> // for size_t in new()
#endif


class GListNodeCache;
class GList
{
public:
	// doubly linked list structure
	class Node 
	{
	public	:
		Node * PrevNode; // pointer to previous node    [ <<- | <T> |  -> ]
		void * Data;  // pointer to data
		Node * NextNode; // pointer to next node        [ <-  | <T> | ->> ]

		void init(Node *pN, bool InsertAfter = 1) 
		{
			// Node constructor -- if 'pN' is NULL, this is the first node in
			// the list.  InsertAfter is a boolean indicating where this node
			// is to be inserted, i.e. before | after the node 'pN'.
			if (pN) 
			{
				if (InsertAfter)
				{
					// procedure to add 'this' node after the node 'pN'
					NextNode = pN->NextNode;
					if (pN->NextNode) 
						pN->NextNode->PrevNode = this;
					PrevNode = pN;
					pN->NextNode = this;
				}
				else 
				{
					// procedure to add 'this' node before the node 'pN'
					PrevNode = pN->PrevNode;
					if (pN->PrevNode) 
						pN->PrevNode->NextNode = this;
					NextNode = pN;
					pN->PrevNode = this;
				}
			}
			else 
			{
				NextNode = PrevNode = 0;
			}
		}
		
		// ONLY the GListNodeCache can use this ctor because it will also call init() when appropriate
		// any other needed initializations could be done here.
		Node(){} 

		Node(Node *pN, bool InsertAfter = 1) 
		{
			init(pN, InsertAfter);
		};
		
		~Node() 
		{
			// procedure to remove 'this' node from the list, and re-link the list
			if (PrevNode) PrevNode->NextNode = NextNode;
			if (NextNode) NextNode->PrevNode = PrevNode;
		};
		// operator delete(1 argument) - puts memory back it into the GListNodeCache
		void operator delete (void *p); 
		// notice new() - we don't allocate we invoke the 0 arg Node 'ctor.
		void* operator new(size_t , void*memoryAddress) {return memoryAddress;}
	};

protected:

	Node  * FirstNode,    // First element in list
		  * LastNode,     // Last element in list
		  * CurrentNode;  // Current element in list
	__int64  iSize;           // number of nodes in list
	
	int nDeferDestruction;

	friend class GListIterator; // List Iterator Class

	GListNodeCache *pNodeCache;
public:
	GListNodeCache *GetNodeCache(){return pNodeCache;}
	void SetNodeCache(GListNodeCache *p) {pNodeCache = p;}
	void EnableCache(bool bAutoGarbageCollect = 0);

	// nodal operations
	void SetCurrent(Node *p) {	CurrentNode = p; }
	Node *GetCurrent() { return CurrentNode; }

	GList(); // list constructor - initialize Node *'s
	GList(const GList &);
	virtual ~GList(void);// Calls RemoveAll for list clean-up

	void operator=(const GList &);

	// copy the contents from the parameter list into the 'this' list
	void AppendList(const GList *);	
										
    // returns a reference to the data in the first node of the list. 
	void * First(void) const;

	// returns a reference to the data in the last node of the list.
	void * Last(void) const;

	// returns a reference to the data in the current node of the list.  
	void * Current(void) const;
					  
	// returns a reference to the data in the next node of the list.  
	// An exception is thrown if the list is empty.
	//  The next node is the node following the current node.  
	// The current node becomes the next node.
	void * Next(void) const;

	// returns a reference to the data in the previous node of the list.  
	// An exception is thrown if the list is empty.  
	// The previous node is the node before the current node.  
	// The current node becomes the previous node.
	void * Previous(void) const;

	// returns a reference to the data in the next node of the list.  
	// An exception is thrown if the list is empty.  
	// The next node is the node following the current node.  
	// The current node does not become the next node.
	void * Tail(void) const;

	// AddBeforeCurrent() adds data before the current node. The current node does not change.
	// Add maintains list integrity by adding the new node before the current node
	// if the current node is the first node, the new node becomes the first node.
	void AddBeforeCurrent( void *data );

	// adds data after the current node.
	// The current node does not change.
	void AddAfterCurrent( void *data );

	// add data to the beginning of the list.
	// The first node becomes the new node.
	void AddHead( void * );
	void AddHead( XMLObject *pO) {AddHead((void *)pO);}

	// add data to the end of the list.
	// The last node becomes the new node.
	void AddLast( void * );
	void AddLast( XMLObject *pO ) {AddLast((void *)pO);}

	// returns True if the list is empty, False otherwise
	bool isEmpty(void) const { return iSize == 0; };

	// returns the number of nodes in the list
	__int64 Size(void) const { return iSize; };
	__int64 GetCount(void) { return iSize;}; 
					  
	// removes all nodes from the list, making an empty list
	void RemoveAll(void);
	
	// returns 1 if found and removed, otherwise Data was not in the list
	int Remove(void *Data); 

	// removes the first node from the list
	void *RemoveFirst(void);
					  
	// removes the last node from the list
	void *RemoveLast(void);
					  
	// removes the current node from the list
	void RemoveCurrent(void);
					  
	// removes the next node from the list,
	// i.e.  the node following the current node
	void RemoveNext(void);

	// removes the previous from the list,
	// i.e.  the node before the current node
	void RemovePrevious(void);

	void DeferDestruction(); // defer ~dtor execution until Destruction()
	void Destruction();
};



class GListIterator
{
	GList::Node *iDataNode;    // used to return reference to data
	GList *pTList;
public:
	GList::Node *iCurrentNode;    // used to return reference to data

	// List Iterator Constructor, iList is a pointer to the list to be iterated
	// IterateFirstToLast indicates that the list will be traversed from "First Node to Last Node" or from "Last Node to First Node".
	// If IterateFirstToLast == 1, use the operator++ to iterate list, otherwise, use the operator-- to iterate list.
	GListIterator(const GList *iList, int IterateFirstToLast = 1);
	~GListIterator() {};

	// reset to the beginning by default, otherwise reset to the end
	void reset(int IterateFirstToLast = 1);

	// returns True if there are more node's to iterate in the list, False otherwise.
	int operator () (void) const;

	// returns a reference to the current node's data, advances the current node to the next node.  
	void * operator ++ (int); 

	// returns a reference to the current node's data, positions the current node to the previous node.  
	void * operator -- (int); 
};


#endif // _GLIST_H____
