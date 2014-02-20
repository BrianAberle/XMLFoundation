// --------------------------------------------------------------------------
//						United Business Technologies
//			  Copyright (c) 2000 - 2014  All Rights Reserved.
//
// Source in this file is released to the public under the following license:
// --------------------------------------------------------------------------
// This toolkit may be used free of charge for any purpose including corporate
// and academic use.  For profit, and Non-Profit uses are permitted.
//
// This source code and any work derived from this source code must retain 
// this copyright at the top of each source file.
// --------------------------------------------------------------------------

#ifndef _GLIST_NODECACHE_H____
#define _GLIST_NODECACHE_H____


#include "GThread.h" // for GListNodeCache's XML_MUTEX which needs CRITICAL_SECTION
#include "GStack.h"  // for GListNodeCache

#ifdef _IOS
	#include <stdlib.h> // for malloc()
#else
	#include <malloc.h> // for malloc()
#endif




#include "stdio.h" // ONLY FOR DEBUGGING   //////////////////////////////////////////////////


//#define NODES_PER_ALLOC		4096
#define NODES_PER_ALLOC		8192
//#define NODES_PER_ALLOC		16384
//#define NODES_PER_ALLOC		32768
//#define NODES_PER_ALLOC		65536



class GListNodeCache
{
	GStack *_pStackAllocBlocks;
	GStack *_pStackAvailable;
	__int64 _nNeedsCompressCount;
	__int64 _nPutCount;
	int _nLockedBlocks;
	int _nBaseBlock;
	__int64 _nAutoGarbageCollect;
	XML_MUTEX _cs;

public:
	__int64 m_Init;
	void AutoGarbageCollect(bool bAuto){ _nAutoGarbageCollect = (bAuto) ? 777 : 0; }

	// Because GList is used in Global objects ....
	// Get() may be called before the 'ctor - so we need to manage the mutex init here - not in the 'ctor
	GList::Node *Get( GList::Node *pN, bool bInsertAfter)
	{
		// We only want to initialize 1 time. We must assume that all member variables are uninitialized.  They will likely 
		// be initialized to 0, but we must not assume so.  If m_Init happens to have a random, uninitialized value of 777 
		// AND a global object is calling this - then this will CRASH before the first line in main() - it is unlikely since 
		// m_Init will be 0 on most operating systems rendering the situation impossible.  If it's not impossible to encounter
		// this situation - it will be very rare. The odds are exactly a 1 in 18,446,744,073,709,551,615 chance it will 
		// randomly be 777.  Luck will have nothing to do with it.
		if (m_Init != 777)
		{
			m_Init = 777;
			_pStackAvailable = new GStack(NODES_PER_ALLOC, NODES_PER_ALLOC);
			_pStackAllocBlocks  = new GStack(64, 64);
			_nNeedsCompressCount = 0;
			_nBaseBlock = 10;
			_nLockedBlocks = 0;

			XML_INIT_MUTEX(&_cs);
		}

		// XML_LOCK_MUTEX is #defined as a CRITICAL_SECTION in Windows - faster than a Mutex
		// notice that there is a single LOCK and a single UNLOCK in this function  
		XML_LOCK_MUTEX(&_cs);
DO_IT_AGAIN:
		while(_pStackAvailable->m_nNext)
		{
			// this is a GStackPopType();
			_pStackAvailable->m_nNext--;
			GList::Node * pNewNode = (GList::Node *)_pStackAvailable->m_arrPtr[_pStackAvailable->m_nNext];

			// if we were able to pop an available node from the stack - init it and return it
			if (pNewNode)
			{
				XML_UNLOCK_MUTEX(&_cs);
				pNewNode->init(pN, bInsertAfter);
				return pNewNode;
			}
			
			// otherwise, keep popping the stack as we "compress" it very efficiently here and remove Nodes marked for deletion.
			_nNeedsCompressCount--;
		}
		
		// if we make it to here - 
		
		// There are no free nodes in our available stack of cached Nodes - its time to fill up the cache
		// allocate a bunch of Nodes at once, a block of them
		void *pBlock = malloc(sizeof(GList::Node) * NODES_PER_ALLOC); 
		// and keep track - in a stack - of all the blocks - we allocated.
		GStackPush((*_pStackAllocBlocks),pBlock);
		// now walk the array backwards as we break that block into pieces sizeof(GList::Node)
		// Walking the list backwards causes us to (initially) issue the Node memory addresses in ascending order. 
		// The memory address of the second Node we return from Get() will be [sizeof(GList::Node)] larger than the first.
		for(int nBlockIndex = NODES_PER_ALLOC - 1; nBlockIndex > -1; nBlockIndex--)
		{
			// get a Node from this pBlock, there will be NODES_PER_ALLOC in 1 pBlock
			// we know where in this block each issued Node is, the address is every sizeof(GList::Node) bytes offset.

			// note: this code is the same - its more readable - but uses an unnecessary temp variable
			// void *pVoid = ((char *)pBlock) + ( sizeof(GList::Node) * nBlockIndex );
			// GList::Node *pNodeInBlock = (GList::Node *)pVoid;
			// this is the fastest - a type cast after a little pointer arithmetic - calculated at compile time.
			GList::Node *pNodeInBlock = (GList::Node *)((void *)(((char *)pBlock) + (sizeof(GList::Node) * nBlockIndex)));
			// that is one deep line of code - and not a single unnecessary parenthesis in the mix.

			// and we could add the extra call stack frames and call the 'ctor - still using our pre-allocated memory.
			// this approach still reaps huge performance benefits.  We reserve the right to call Node 'ctor in the future
			// GList::Node *pNodeInBlock = new( ((char *)pBlock) + (sizeof(GList::Node) * nBlockIndex)  ) GList::Node();
			
			// so we have then and we are looping through the block of them we acquired 
			// - push them on the stack of available Nodes  ( this is a GStackPush() expanded inline)  
			if (_pStackAvailable->m_nNext >= _pStackAvailable->m_nSize)
				_pStackAvailable->grow();
			_pStackAvailable->m_arrPtr[_pStackAvailable->m_nNext] = pNodeInBlock;
			_pStackAvailable->m_nNext++;
		}
		
		// we made this allocation, broke it into new Nodes, and filled the available stack because the application
		// requested a Node and our available stack was empty.  It is full now, so go back to the top and fulfill the request.
		goto DO_IT_AGAIN;

		return 0; // this never happens because the goto always happens.
	}


	void Put( GList::Node *pN )
	{
		// When m_Init is 21 it tells us to safely ignore this request to return a Node.
		// It can only happen when another global object empties a GList or GStringList in it's destructor.
		// The other global object needs not be aware that ~GListNodeCache() was executed first and the memory was already returned to the OS
		if (m_Init != 21)
		{
			XML_LOCK_MUTEX(&_cs);

			// procedure to remove 'this' node from the list, and re-link the list
			if (pN->PrevNode) pN->PrevNode->NextNode = pN->NextNode;
			if (pN->NextNode) pN->NextNode->PrevNode = pN->PrevNode;

			GStackPush((*_pStackAvailable),pN);
			XML_UNLOCK_MUTEX(&_cs);

			// We could attempt an automatic garbage collection scheme.....(but we do not by default)
			// There are many ways to 'guess' when it's time.  There are many many variables in "generalized" memory management.
			// The variables are defined by YOUR CUSTOM application.  For example, The XMLFoundation examples work on an OLD
			// Android 2.2 phone and a NEW 64 bit machine with multiple CPU's and enough memory to remember the ancient and the future. 
			// "customized" memory management does much less 'guessing'.  It's better to KNOW and best to customize.
			//
			// On a 64 bit build, we will assume that we have more resources.  
			// Bigger numbers improve performance and use more memory until the numbers get too big - then they slow you down.
			// The variables also include your application of this "custom" memory manager.  Not all applications do the same 
			// thing - thats a big variable.  In MANY applications (not all(i love exceptions)) we MUST Garbage collect.
			// The timing of the Garbage collection can be critical - you can customize it, but a default will work well in many applications.
			// Working well == raising many size XML document size limits and improving performance by managing the burden that would otherwise
			// be on the operating system memory manager.  Just know this: Never rely on a default when you KNOW that you will contain the exception.

			// If we NEVER garbage collect, and our instance of this - a GListNodeCache - applies to one instance of a GList - sweet! ....
			// ..That is simple stuff. We can SPEED over the generalized memory manger in the OS.  We just nuke 'em quick as an iteration 
			// AFTER our GList destructor is called.
			if (_nAutoGarbageCollect == 777)
			{
				#if defined(_LINUX64) || defined(_WIN64) 
					int nFrequency = 21; // higher value = less frequent automatic GarbageCollect, the more we cache(faster), more memory is used.
				#else
					int nFrequency = 3;
				#endif

				if ( ++_nPutCount  %  (NODES_PER_ALLOC * nFrequency) == 0)   // 21
				{
//					__int64 nBlocks = _pStackAllocBlocks->Size();
					if ( _pStackAvailable->Size() >  NODES_PER_ALLOC * 21)
					{

						_nPutCount = 0;
					
						GarbageCollect();

					}
				}
			}
		}
	}

	GListNodeCache(){ }


	// Note: The Order is Critical here.
	~GListNodeCache()
	{
		// if m_Init is not 777, then this object has never had a single Node requested from it, so there is nothing to destroy
		if (m_Init == 777)
		{
			// Note: search for "Magic number 21" in GList.cpp
			m_Init = 21;
			// In this state, ALL memory underneath ALL GList::Nodes is about to be 0xBAD when we delete it in the next loop.


			for(int i = 0; i < _pStackAllocBlocks->m_nNext-1; i++)
			{
				if (_pStackAllocBlocks->m_arrPtr[i])
					delete _pStackAllocBlocks->m_arrPtr[i];
			}
		

			// finally...... destroy the stacks.
			delete _pStackAvailable;
			delete _pStackAllocBlocks;

			XML_DESTROY_MUTEX(&_cs);
		}

	}
	// Note: There are many possible implementations and situations.  This function will NEVER get called unless you specify.  
	// I found cases where its very fast and other cases where it does not work at all due to worst case scenarios.  
	// Keep in mind that a lists destructor is the perfect place to do all this in just a few lines of code with no guesswork.
	// This would only be needed in a "Long Living List", you would need to test this for yourself if you have a situation like that.
	// It always works well in one application, that is most certainly not true of all applications.
	void GarbageCollect()
	{
		if (_pStackAllocBlocks->Size() < 2)
		{
			return;
		}
		bool nReverseOptimize = 0;
		XML_LOCK_MUTEX(&_cs);

		if (_nNeedsCompressCount > 0)
		{
			// compress out the nodes marked for deletion
			GStack *pCompressed = new GStack(_pStackAvailable->m_nSize,_pStackAvailable->m_nGrowBy);
			for (int i=0;i<_pStackAvailable->m_nNext;i++)
			{
				if (_pStackAvailable->m_arrPtr[i])
				{
					GStackPush(  (*pCompressed),  _pStackAvailable->m_arrPtr[i]  );
				}
			}
			delete _pStackAvailable;
			_pStackAvailable = pCompressed;


			// compress out pBlocks marked for deletion
			pCompressed = new GStack(_pStackAllocBlocks->m_nSize,_pStackAllocBlocks->m_nGrowBy);
			for(int i2 = 0; i2 < _pStackAllocBlocks->m_nNext-1; i2++)
			{
				if (_pStackAllocBlocks->m_arrPtr[i2])
				{
					GStackPush(  (*pCompressed),  _pStackAllocBlocks->m_arrPtr[i2]  );
				}
			}
			delete _pStackAllocBlocks;
			_pStackAllocBlocks = pCompressed;

			_nNeedsCompressCount = 0;
		}

		// we are going to loop through all the "pBlocks" which are blocks of memory that are large enough to hold NODES_PER_ALLOC instances of a GList::Node
		// Top is _pStackAllocBlocks->m_nNext-1.  Bottom is 0.  Bottom blocks were issued first, they are older and more likely to be free.
		// The very top(the highest valid index is 1 less than m_nNext), is most certainly not free so we wont ever check it.  
		// The Next N blocks are nearly never going to contain any Nodes that the garbage collection will be able to free because they are used in the "Base" of the application that will be in use all the time. 
		__int64 nBlockIndexInArray = _pStackAllocBlocks->m_nNext - _nBaseBlock;
		_nBaseBlock = 10; // we will add on to it the optimization for the next call to GarbageCollect();

CHECK_NEXT_BLOCK:
		while(nBlockIndexInArray > -1)
		{
			char *pBlock = (char *)_pStackAllocBlocks->m_arrPtr[nBlockIndexInArray];
			nBlockIndexInArray--;

			//
			// All these variables  are running counts (or current values like nBlockOffset) during the attempt to destroy 1 pBlock (of N number of them in [_pStackAllocBlocks])
			//
			
			int nBlockOffest = NODES_PER_ALLOC - 1;

			int nFreeFound = 0;

			// the previous iterations index into the available stack where it was found
			__int64 nLastFreeFound = 0;

			__int64 nTryOptimize = -1;
			__int64 nLastTryOptimize = -1;

			int nFailedOptimizations = 0; 
			int nIterations = 0; 
			int nMaxDiff = -3;
			__int64 nExceptionOptimizes = 0;
			__int64 nLastExceptionOptimize = 0;


CHECK_NEXT_ADDRESS:
			while (nBlockOffest > -1) 
			{
			// we know where in this block each issued Node is, the address is every sizeof(GList::Node) bytes offset.

			// note: this code is the same - its more readable - but uses an unnecessary temp variable
			//
			// void *pVoid = ((char *)pBlock) + ( sizeof(GList::Node) * nBlockOffest );
			// GList::Node *pNodeInBlock = (GList::Node *)pVoid;
			//
			// this is the fastest - a type cast after a little pointer arithmetic.
			GList::Node *pNode = (GList::Node *)((void *)(((char *)pBlock) + (sizeof(GList::Node) * nBlockOffest)));
			//
			// or we could add the extra call stack frames and call the 'ctor - we reserve the right to call it in the future
			//
			// GList::Node *pNode = new( ((char *)pBlock) + (sizeof(GList::Node) * nBlockOffest)  ) GList::Node();


				__int64 nIndexSearch;
RECHECK_THIS_ADDRESS:
				if (nReverseOptimize)
					nIndexSearch = (nTryOptimize == -1) ? -1 : nTryOptimize;
				else
					nIndexSearch = (nTryOptimize == -1) ? _pStackAvailable->m_nNext : nTryOptimize;
				
				nIterations = 0; // and reset this one so we know how well or poor the optimization is working

				// ***********************************************************************************************
				// BEGIN loop through _pStackAvailable[nIndexSearch];  Count how many free Nodes are in this pBlock
				while(1)
				{
					// inc/dec nIndexSearch then decide if the looping has reached termination - the bottom or the top of _pStackAvailable
					bool nKeepGoing;
					if (nReverseOptimize) // nIndexSearch is incrementing from 0 to _pStackAvailable->m_nNext
					{
						nIndexSearch++;
						nKeepGoing = (nIndexSearch < (__int64)_pStackAvailable->m_nNext) ? 1 : 0;
					}
					else
					{
						nIndexSearch--;
						nKeepGoing = (nIndexSearch > -1) ? 1 : 0;
						if (nIndexSearch > (__int64)_pStackAvailable->m_nNext - 1)
						{
							nIndexSearch = _pStackAvailable->m_nNext - 1; 
						}
					}

					if(!nKeepGoing)
					{
						// bad news.  we looped a long way to find that this was not here.  Avoid this as much as possible.
						// if we have not even found a block that contains free Nodes, this may be still part of the applications 'Base' memory use.
						if(nIterations == _pStackAvailable->m_nNext)
						{
							if (!_nNeedsCompressCount)
								_nBaseBlock++;

							_nLockedBlocks++;
//printf("#");
						}

						break;   // we reached the top, or bottom of [_pStackAvailable] 
					}



					// count how many time we loop until we either find [pNode] in the stack of Available Nodes 
					// or know for certain that [pNode] does NOT exist in [_pStackAvailable].  
					// This is the central focus in optimizations.
					nIterations++;

//Break: if(nIterations > _pStackAvailable->m_nNext){	int debug =1;  debug++; }
					
					
					// if we FOUND it!
					if (_pStackAvailable->m_arrPtr[nIndexSearch] == (void *)pNode)
					{
						nFreeFound++;
						nBlockOffest--;
						if(nFreeFound == NODES_PER_ALLOC)
						{
							// excellent - this is what we are looking for - a whole block of free nodes.
							break; 
						}
							
						// we are guessing that the next Node issued from this pBlock will be found just below this one.
						// set [nTryOptimize] to the index where we want to begin iterating.  This value will still
						// inc/dec one more time and will continue until it is found or the top/bottom of _pStackAvailable is found.
						int nDifference = (int)(nLastTryOptimize - nIndexSearch);


						// if this hit wasnt very optimized - then determine out next optimization
						if(nIterations > 1000)
						{
							// we guessed wrong, thats why we see lots of [nIterations]
							if (nFailedOptimizations++ > 10)
							{
								int debug =1;  debug++;
							}

							// We optimized expecting it below us.  This if says: Is it above us?
							if(nLastTryOptimize != -1 && nIndexSearch > nLastTryOptimize)
							{
								if (nDifference < 0)
								{
									// This optimization is complex.  The nodes were put() on the stack recursively - a list objects destructor destroyed a list.
									// this sets the frame in which we are likely to find that which we seek to destroy 
									//   - there are still exceptions but this very commonly makes the search find in as few as [nMaxDiff] iterations.

// testing what would this does to the ratio of iterations per hit
//	// now lets also guess that we found this somewhat freakish occurrence in this set they may be another even 3 stack frames bigger
nDifference += -3;

									// note: both numbers are negative and nMaxdiff will now go even further negative.
									nMaxDiff += nDifference;


// This line of code never gets hit.....
// Just about all hits, are found in under 10 iterations!!!!!!  (except for the exceptions, which are generally a single digit value per 4096 sizes block.)
// does the variance ever get more than 50 off?  thats a deep stack, or something unexpected.
// Break: if (nMaxDiff * -1 > 50) { printf("$"); }  

								}
							}
							else if (nLastTryOptimize != -1 && nIndexSearch < nLastTryOptimize)
							{

								// nTryOptimize will be set to nLastTryOptimize not nIndexSearch because we are predicting that this is an exception
								// Keep track of these, very relatively few freaks from somewhere else in the stack that failed the early level guessing.
								nExceptionOptimizes++;

								// we are about to set nIndexSearch to the next place we want to start looking next time.
								// hang on to our last hit index. 
								nLastFreeFound = nIndexSearch;

								if(nLastExceptionOptimize)
								{
//	int nReset = nIndexSearch - nLastExceptionOptimize;
									nLastExceptionOptimize = nIndexSearch;

//BREAK:		if (nReset < 10)	
//						{	int debug =1;  debug++;  }
								}

								if (nReverseOptimize)
									nTryOptimize = nLastTryOptimize + nMaxDiff;  
								else
									nTryOptimize = nLastTryOptimize - nMaxDiff;

								nLastTryOptimize = nIndexSearch;


								goto CHECK_NEXT_ADDRESS;

							}
						}


						nLastTryOptimize = (nTryOptimize == -1) ? nLastTryOptimize : nTryOptimize;

// stop here - This pattern search is not working - resort the whole mess in a more typical (less optimal but possible in all cases) algorithms.
// this algorithm punishes you for giving it non iteratively/recursively placed deletes.  
//if ( nExceptionOptimizes > 77 )
//BREAK:						{	int debug =1;  debug++;  }

						
						//note: nMaxDiff is negative
						if (nReverseOptimize)
							nTryOptimize = nIndexSearch + nMaxDiff;  
						else
							nTryOptimize = nIndexSearch - nMaxDiff;


						// this pNode is free.  It does not disqualify the pBlock of being released.
						// Now go check the next Node issued from this in this block.  Iterate to the next pNode
						goto CHECK_NEXT_ADDRESS;  
					}
				}
				// END loop through the stack of available Nodes, _pStackAvailable[nIndexSearch]
				// ***********************************************************************************************


				// in the loop immediately above - we tallied the count of free Nodes in pBlock
				if(nFreeFound != NODES_PER_ALLOC)  
				{
					// if we tried to optimize the search we exited with - but did not find it - try again without optimization
					if (nTryOptimize != -1)
					{
						nTryOptimize = -1;
						goto RECHECK_THIS_ADDRESS;
					}

					// if any 1 address in a block is not free, then the block will not be destroyed - 
					// we dont need to check any more addresses in this block so start checking the next block
					goto CHECK_NEXT_BLOCK; 
				}
				else  // DELETE - this is where the 'real' memory allocation will be freed()
				{

					_nNeedsCompressCount += NODES_PER_ALLOC;
					nTryOptimize = - 1;
					// first remove ALL the (soon to be) invalid _pStackAvailable pointers from this block
					// loop through pBlock again - this time we know that every one must be removed from the stack

		// now for those who seek to understand.....
		// The "upper loop" is optimized, this loop is not.  Both loops do the same thing.
		// This loop will eventually become optimized like the loop above it and XMLFoundation we will be even yet FASTER++;
		// Here we just start at the top of each pBlock and run down until we find it - the run top down on the next...
		// running up Billions more iterations in this simple little loop....  That serves well for .edu purposes
		// Look how simple this loop is - (we do the same thing up above) it's an order of magnitude slower here.....
		// We do it fast above currently still SLOW here and the net performance gain over using the OS memory manager is very good.
		// So look how simple it really is to be faster than the OS.  Here we seek to be the fastest so the same complexities
		// in the upper loop will be applied here, but until then look how simple it is just to "get it done". kind of fast.

					int nBlockOffest = NODES_PER_ALLOC - 1;
CHECK_NEXT_ADDRESS_DELETE:
					while (nBlockOffest > -1) 
					{
						char *pRemove = &pBlock[nBlockOffest * sizeof(GList::Node)];
RECHECK_THIS_ADDRESS_DELETE:
						nIndexSearch = (nTryOptimize != -1) ? nTryOptimize : _pStackAvailable->m_nNext-1;
						for(; nIndexSearch > -1; nIndexSearch--)
						{
							if (_pStackAvailable->m_arrPtr[nIndexSearch] == (void *)pRemove)
							{
								// this is now 'marked as free' - but still in the stack search for "compress" for more comment about it
								_pStackAvailable->m_arrPtr[nIndexSearch] = 0;  

								// we are guessing that the next pNode issued from the pBlock be found just below this one.
								nTryOptimize = nIndexSearch - 1; 
								nBlockOffest--; 
								goto CHECK_NEXT_ADDRESS_DELETE;  // this pNode is free, check the next 
							}
						}

						// if we tried to optimize - but did not find it - try again without optimization
						if (nTryOptimize != -1)
						{
							nTryOptimize = - 1;
							nFailedOptimizations++;
							goto RECHECK_THIS_ADDRESS_DELETE;
						}
					}

					// OK - all the pNodes that were issued from this pBlock are removed from the available stack, 
					// now remove this block from _pStackAllocBlocks
					for(int i = 0; i < _pStackAllocBlocks->m_nNext-1; i++)
					{
						if (_pStackAllocBlocks->m_arrPtr[i] == pBlock)
						{
							_pStackAllocBlocks->m_arrPtr[i] = 0; // we compress it out later
							break;
						}
					}
					// finally....... free the memory
					free(pBlock);
						
				}
			}
		}
		XML_UNLOCK_MUTEX(&_cs);
	}
	


};
extern void GarbageCollectGList();

#endif // _GLIST_NODECACHE_H____