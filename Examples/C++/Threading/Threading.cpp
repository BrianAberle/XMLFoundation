// --------------------------------------------------------------------------
//						United Business Technologies
//			  Copyright (c) 2016  All Rights Reserved.
//
// Source in this file is released to the public under the following license:
// --------------------------------------------------------------------------
// This toolkit may be used free of charge for any purpose including corporate
// and academic use.  For profit, and Non-Profit uses are permitted.
//
// This source code and any work derived from this source code must retain 
// this copyright at the top of each source file.
// --------------------------------------------------------------------------
#include <XMLFoundation.h>
#include "GPerformanceTimer.h"


#ifdef _WIN32
#include <conio.h> // for getch()
#endif
#include <errno.h>		// for definition of EBUSY (16)

// Note: Debug memory managers are an order of magnitude slower than release and have lower limitations.


// This example uses large files and memory allocations, the test file size can be set in increments of 93912 bytes
// therefore when g_SizeFactor = 222 the input file will be (93912 * 222) = about 20MB (20,848,487 bytes exactly).
// therefore when g_SizeFactor = 777 the input file will be (93912 * 777) = about 70MB (72,969,624 bytes exactly).
// that input file will be parsed into a fully instanciated object model per thread.
// Adjust g_SizeFactor according to your available memory resources.  Execution times are phenomenal.

#if defined(_WIN64)
	#ifdef DEBUG
		// 64 bit Debug - with 16MB of RAM these values work well.
		int g_SizeFactor = 333;
		int g_nThreadCount = 5;
	#else
		// 64 bit Release - with 16MB of RAM these values work well.
		int g_SizeFactor = 777;
		int g_nThreadCount = 7;
	#endif
#else
	#ifdef DEBUG
		//32 bit debug
		int g_SizeFactor = 111;
		int g_nThreadCount = 3;
	#else
		// 32 bit release
		int g_SizeFactor = 222;
		int g_nThreadCount = 6;
	#endif
#endif

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// in a threaded environment we have the option to speed things up even more by removing auto-memory-cleanup 
// when the proper conditions allow it as they do here.  Search for where [g_TrustThreadHeaps] is used in this file.
int g_TrustThreadHeaps = 1;
// Note: the following results are from an Intel i5 with 16 MB of RAM
//
// -------------- 32 bit release build with [g_SizeFactor = 222] and [g_nThreadCount = 6] -----------
// TOTAL==============Parsed 125Mb of XML in 2550 milliseconds  <--- with g_TrustThreadHeaps = 1
// TOTAL==============Parsed 125Mb of XML in 3777 milliseconds  <--- with g_TrustThreadHeaps = 0  
// 2550 is 67.7% of 3777 or 32.3% faster.
//
// -------------- 64 bit release build with [g_SizeFactor = 777] and [g_nThreadCount = 7] ------------
// TOTAL ============== Parsed 510Mb of XML in 8200 milliseconds <--- with g_TrustThreadHeaps = 1
// TOTAL ============== Parsed 510Mb of XML in 26335 milliseconds <--- with g_TrustThreadHeaps = 0   
// Execution times vary -  OVER TWICE AS FAST is constant and with the given input THREE TIMES FASTER 
//////////////////////////////////////////////////////////////////////////////////////////////////////


// this parses XML so fast, that it's almost unbelievable - so if you set [g_bWriteProof] to 1, each thread 
// re-creates and writes the XML from Data.xml to DataDataN.xml where N is the thread that did the work.
// When set, the output files will equal the input file so you can see and are not forced to believe.
int g_bWriteProof = 0;



class MyThreadedObject : public XMLObject
{
public:

	char m_pzElementary[32];
	char m_pzAttributary[32];
	virtual void MapXMLTagsToMembers()
	{
		MapMember(m_pzElementary, "Elementary", sizeof(m_pzElementary));

		// Elements parse faster than attributes in the XMLFoundation - because after the first attribute has been detected in the XML
		// XMLObject needs to allocate and create a GHash which also constructs internal GBTrees.  This construction time should be 
		// considered in algorithm comparison data along with lookup, insert, delete, for example compare GHash and GSparshHash in this respect.
		// so we could make our numbers even better with an all Element no-Attribute design, but we dont handle attributes slow
		// - we just handle elements faster
		MapAttribute(m_pzAttributary, "Attributary", sizeof(m_pzAttributary));
	}

	DECLARE_FACTORY(MyThreadedObject, EgyptianCotton)

	MyThreadedObject() {};
	~MyThreadedObject() {};
};
IMPLEMENT_FACTORY(MyThreadedObject, EgyptianCotton)





class MyListOfObjects : public XMLObject
{
public:

	char  m_pzStringNumber[7];
	int	  m_nNumber;
	GList m_lstMyObjects; // holds instances of type MyThreadedObject

	virtual void MapXMLTagsToMembers()
	{
		MapMember(m_pzStringNumber, "AlphaNumber", sizeof(m_pzStringNumber));
		MapMember(&m_nNumber, "Number");
		MapMember(&m_lstMyObjects, MyThreadedObject::GetStaticTag());
	}

	void MakeData(int nListID)
	{
		sprintf(m_pzStringNumber, "%i", nListID);
		m_nNumber = nListID;

		for (int i = 0; i<777; i++)
		{
			MyThreadedObject *pO = new MyThreadedObject();

			// 	create objects with No OID for this example
			sprintf(pO->m_pzElementary, "Element%d in %d", i + 1, nListID);
			sprintf(pO->m_pzAttributary, "Attribute%d in %d", i + 1, nListID);
			m_lstMyObjects.AddLast(pO);
		}
	}


public:
	DECLARE_FACTORY(MyListOfObjects, AnotherList)
	MyListOfObjects() {}

	void ForceCleanup()
	{
		GListIterator it(&m_lstMyObjects);
		while (it())
		{
			delete (MyThreadedObject *)it++;
		}
	}

	~MyListOfObjects()
	{
		// Normally cleanup happens automatically
		if (g_TrustThreadHeaps)
		{
			ModifyObjectBehavior(PREVENT_AUTO_DESTRUCTION, 0);
		}
	};
};
IMPLEMENT_FACTORY(MyListOfObjects, AnotherList)



class MyCustomObjectContainer : public XMLObject
{
public:

	int m_nThreadID;
	GList m_lstMyObjects; // holds instances of type MyListOfObjects


	virtual void MapXMLTagsToMembers()
	{
		MapMember(&m_lstMyObjects, MyListOfObjects::GetStaticTag());
	}

	void MakeData()
	{
		for (int i = 0; i < g_SizeFactor; i++)
		{
			MyListOfObjects *pO = new MyListOfObjects();
			pO->MakeData(i + 1);
			m_lstMyObjects.AddLast(pO);
		}
	}


public:
	DECLARE_FACTORY(MyCustomObjectContainer, Owner)
	MyCustomObjectContainer(int nTID)
	{
		m_nThreadID = nTID;
	}
	MyCustomObjectContainer() {}

	void ForceCleanup()
	{
		GListIterator it(&m_lstMyObjects);
		while (it())
		{
			MyListOfObjects *pO = (MyListOfObjects *)it++;
			pO->ForceCleanup();
			delete pO;
		}
	}

	~MyCustomObjectContainer()
	{
		// Normally cleanup happens automatically
		if (g_TrustThreadHeaps)
		{
			ModifyObjectBehavior(PREVENT_AUTO_DESTRUCTION, 0);
		}
	};
};
IMPLEMENT_FACTORY(MyCustomObjectContainer, Owner)

//////////////////////////////////////////////////////////////////////////
// Globals used to transfer state info between XMLParseThread and it's parent
int XMLParseThreadFlags[256];
gthread_mutex_t lock[32];
gthread_cond_t	cond[32];
//////////////////////////////////////////////////////////////////////////
void *XMLParseThread(void *arg)
{
	// each thread of execution gets a number between 0 and g_nThreadCount
	int nThreadIndex = (int)arg;

	// ----------------------------------------------------------------------
	// wait here until we get the command to start
	// ----------------------------------------------------------------------
	gthread_cond_wait(&cond[nThreadIndex], &lock[nThreadIndex]);


	MyCustomObjectContainer *ObjectFromFile = new MyCustomObjectContainer(nThreadIndex);

	printf("%din+\n", nThreadIndex);
	ObjectFromFile->FromXMLFile("Data.xml");
	printf("%din-\n", nThreadIndex);


	// ObjectFromFile was created right now and if we write it out to as proof.
	if (g_bWriteProof)
	{
		// it writes Data.xml back out as DataDataN.xml where N is nThreadIndex that did the work.

		// add the thread # into the output file name so we can see where it came from
		GString strOutFile("DataData");
		strOutFile << nThreadIndex << ".xml";
		printf("%dout+\n", nThreadIndex);
		ObjectFromFile->ToXMLFile(94000 * g_SizeFactor, strOutFile);
		printf("%dout-\n", nThreadIndex);
	}

	// if we unconditionally end this thread - we dont need to release all the little bits of memory we used - 
	// as we terminate the thread, the threads heap space is destroyed in 1 single faster release as the thread terminates, 
	// therefore we can safely skip the memory cleanup to build more efficient software when we also know that there is
	// no destructor logic that must happen in our custom ObjectFromFile - we can skip all those method calls as well.
	if (!g_TrustThreadHeaps)
	{
		printf("%d~+\n", nThreadIndex);
		delete ObjectFromFile;
		printf("%d~-\n", nThreadIndex);
	}

	XMLParseThreadFlags[nThreadIndex] = 1;

	GString strLabel;
	strLabel << nThreadIndex << " Exit\n";
	printf(strLabel);
	return 0;
}


void MyAppInit()
{
	memset(XMLParseThreadFlags, 0, sizeof(XMLParseThreadFlags));

	// We could have a thread pool with [g_nThreadCount] threads already running and waiting 
	// for an event to immediately begin the work to be done, before going back to the pool..... 
	// See [AttachToClientThread] and [g_PoolClientThread] and [CreateThreadPool] in ServerCore.cpp for 
	// a thread pool implementation that would complicate this mostly simple example too much.
	// Therefore we will use a "Ready - Set - (awaiting go)" design approach.
	for (int i = 0; i<g_nThreadCount; i++)
	{
		gthread_mutex_init(&lock[i], 0); // handle all this init code at application startup.
		gthread_cond_init(&cond[i], 0);
		gthread_mutex_lock(&lock[i]); // lock mutex so the XMLParseThread hangs at gthread_cond_wait() 

									  // Creating threads is VERY slow (relative to a non-threaded function call) as the OS allocates a new heap, 
									  // sets up the stack, registers handles, and makes the function call on the new thread.
									  // This "Ready - Set - (awaiting go)" design approach avoids the penalty for all of that without 
									  // complicating this example with a thread pool which is a preferrable design with respect to overall flexibility.
		gthread_t parse_thread;
		gthread_create(&parse_thread, NULL, XMLParseThread, (void *)i);
	}

}

int main(int argc, char* argv[])
{
	MyAppInit();

	try
	{
		// create an XML input file that we will run the test with
		GPerformanceTimer *profLoad = new GPerformanceTimer("Create and Write XML");
		MyCustomObjectContainer *O = new MyCustomObjectContainer;
		O->MakeData();
		O->ToXMLFile(94000 * g_SizeFactor, "Data.xml");
		delete profLoad;
		GPerformanceTimer *freeTime = new GPerformanceTimer("Releasing memory from Create and Write");
		O->ForceCleanup();
		delete O;
		delete freeTime;


		// start the timer then tell the "ready, set" worker threads to "go" parse XML
		__int64 lTime = 0;
		GPerformanceTimer *profTotal = new GPerformanceTimer(&lTime);
		for (int i = 0; i<g_nThreadCount; i++)
		{
			gthread_cond_signal(&cond[i]);
		}



		while (1)
		{
			// this should use gthread_cond_signal(), but for the purpose of the example simplification
			// it is counting up to [g_nThreadCount] every 50 milliseconds to check a thread completion flag.
			Sleep(50);
			int nThreadsFinished = 0;
			for (int i = 0; i<g_nThreadCount; i++)
			{
				if (XMLParseThreadFlags[i] == 1)
					nThreadsFinished++;
			}

			// when all the threads are finished break out of the while(1)
			if (nThreadsFinished == g_nThreadCount)
				break;
		}

		// All the threads are finished now.
		delete profTotal; // stops the timer and sets [lTime]

		// display the results.
		GString strFinalMsg("\n\nTOTAL==============");
		GString strFinalBytes;
		strFinalBytes << 93912 * g_SizeFactor * g_nThreadCount;
		strFinalMsg << "Parsed " << strFinalBytes.AbbreviateNumeric();
		if (g_bWriteProof)
		{
			strFinalMsg << " AND created/wrote " << strFinalBytes.AbbreviateNumeric();
		}
		strFinalMsg << " of XML in " << lTime << " milliseconds";

		printf(strFinalMsg);
	}
	catch (GException &rErr)
	{
		//Run TaskManager while you run this example and watch the memory use....
		printf(rErr.GetDescription());
	}

	printf("\n\nPress any key to begin calling global destructors ...\n");
	fflush(stdout);

#ifndef _WIN32
	getchar();
#else
	_getch();
#endif

	return 0;
}


