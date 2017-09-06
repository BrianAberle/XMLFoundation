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


//***************************************************************************
// This example reads in a text file one word at a time.
// For each word a simple indexed object is created.  
// Multiple data structures are used to manage the objects.
//***************************************************************************


// NOTE: if this application crashes, you need to:
//
// A) Try it in Release, not Debug - your memory manager will be faster and have higher limits
// B) set bSmallTest to 1, which actually is a very large object indexing test
// C) Test it on a machine with more memory.


///////////////////
// GSparseHash and GHash are interesting to compare.  For a Lookup() GSparseHash and GHash are very close.
// The big difference is the time it takes to iterate the entire collection.
// The iteration times VARY(differ) VERY(lots) GREATLY between debug and release builds.

// --------  Iteration Times  --------
// GHash DEBUG   		  269,400 microseconds
// GHash Release 		   98,753 microseconds
// GSparesHash DEBUG	7,112,693 microseconds
// GSparesHash RELEASE	    8,216 microseconds

// We should mostly disregard debug build performance numbers, even though they are so interesting:
// GSparseHash can be iterated much faster than GHash(in a Release build where it matters)
// Both GList and GQSortArray can be iterated over 2x as fast as GSparseHash 
//    ( GSparseHash and GHash greatest attribute is Lookup speed.)
// A GBTree is the only structure to use if you need to sort alphabetically because it is 
//  the only keyed data structure iterator that does that.
///////////////////



// you may want to set this to 'true' or 1 if you are going to run this in the debugger.
bool bSmallTest = 0;

#include "XMLFoundation.h"
#include "GPerformanceTimer.h"
#include "GStringCipherZip.h"

#ifdef _WIN32
	#include <conio.h> // for getch()
#else
	#include <strings.h> // for: strcmp() strlen()
	#include <string.h> 
#endif
#include <stdlib.h> // for: getenv()
#include <stdio.h>



#ifndef _WIN32
	int g_UseSparseHash = 0;
	#define NO_SPARSE_HASH  1
	// see notes in "GSparseHash.h".
#else
	#if defined(_MSC_VER) && _MSC_VER <= 1200 

		// VC6 will NOT use SparseHash
		int g_UseSparseHash = 0;
		#define NO_SPARSE_HASH  1

	#else
// removed for VS2017 compatibility due to missing std::uninitialized_copy
int g_UseSparseHash = 0;
#define NO_SPARSE_HASH  1

//		int g_UseSparseHash = 1;

		// The Google SparseHash project contains several hash-map implementations in use at Google.
		// Sparse Hash is very fast.  GHash and xmlLex are very fast too.
		// XMLFoundation uses templates too. Look at ObjQuery<> to see how templates can deliver data to the application layer.
//		#include "GSparseHash.h"

	#endif
#endif



// go see where this is used before you change it to 1.
int g_bAddMembermapTime = 0;
__int64 g_nWordObjInstances = 0;


GString *g_xml = 0;




// **********************  The Word Object *****************************
class TheWord : public XMLObject
{
public:
	
	//note: i proposes that k denotes a member is a key part.  They are special members with special rules.
	//note: all your memory is consumed by this variable.  32 bytes per instance of this object = Gigabytes
	//note: {P}ointer to a {Z}ero terminated {K}ey part - TheWord
	char m_pzkTheWord[32];

	// Note that we cannot use a GString inplace of m_pzkTheWord, because a GString makes a 64 byte 
	//    default allocation.   That would double the application memory use, plus add GString object overhead.
	// We would conserve much memory by using a GString32 over a GString, if there are many instances 
	// that never contain > 32 bytes. 
	// The direct char[] map to m_pzkTheWord has no overhead, we can use that if we dont need GString.
	


	__int64  m_nkCount;
	unsigned char m_Bits;

	int m_nInt;
	virtual void MapXMLTagsToMembers()
	{
		MapObjectID("theWord", 1,	"Count", 1);
		MapMember(m_pzkTheWord,		"theWord", sizeof(m_pzkTheWord) );
		MapMember(&m_nkCount,		"Count");
		MapMember(&m_nInt,			"Integer");

// Deeply technical note: Notice the time it takes for the first [Releasing memory].  We are waiting on the 
// memory manager in the OS (BIG difference between 32 and 64 bit memory managers) to "delete" every last single 
// bit  of "new" memory that it gave us. The memory manager keeps track of it with a lookup in a hash-table 
// kind of data structure that manages it all.  64 bit numbers are HUGE.  
// This example program IS AN INDEXING STRESS TEST.  Each word becomes an OBJECT. Instantiated (aka new'd) MANY times.
// Memory managers, by design, optimize new over delete.  The high end of this test will stress 64 bit memory managers......
// so as expected, the 32 bit memory managers will be knocked off first.  32 bit builds will crash in all kinds 
// of places when new() returns 0.  Each call to MapMember introduces another call to new().
//
// 32 bit memory manager limits will quickly be reached in a stress test designed for 64 bit - yet this is portable 
// to 32 bit where we perform very well.  Future XMLFoundation enhancements will likely consolidate even more
// calls to new by consolidating the "new" space for multiple objects into 1 call to new - then explicitly calling
// the constructors.  Note: DEBUG memory managers will break down before RELEASE memory managers.
// Any accurate performance numbers must be obtained with a release build.
//
// Prior to the custom memory management added to the XMLFoundation in 2014 The additional calls to MapMember() would 
// break down a 32 bit memory manger, under the stress of every word in "TheWholeTruth.txt" being represented as 
// a unique XMLObject. - There would be other ways to model that data.  However - 2014 comes with higher limits for
// 32 bit builds now - they can now handle the stress of Objects instantiated from "TheWholeTruth.txt" - and no longer
// be so close to the ceiling that they need to start using all their bits like this:  I raised the roof.
		if(g_bAddMembermapTime)
		{
			MapMemberBit(&m_Bits,		"EvenOddWords", 7, "False",  "True");
		}

	}

	DECLARE_FACTORY(TheWord, WordObj)
	// FromXML() factory created objects use this ctor 
	TheWord(){g_nWordObjInstances++;}
	// User created objects use this ctor 
	TheWord(const char *pzWord, __int64 nCount, int bOdd) 
	{
		g_nWordObjInstances++;
		m_nkCount = nCount;  
		m_nInt = 7; 
		m_Bits = 0; 
		strncpy(m_pzkTheWord,pzWord,sizeof(m_pzkTheWord)-1); 
		
		// stores the Evenness or Oddness of TheWord that fills our memory.
		// 8 bytes uses more memory than 8 bits, a lot more in some cases, a little more in most.
		// We could store 7 more booleans in this object "for free"
		if (bOdd)
		{
			m_Bits |= 0x01; // turn the 1st bit on
			m_Bits |= 0x04; // turn the 3rd bit on
			m_Bits |= 0x08; // turn the 4th bit on
			m_Bits |= 0x10; // turn the 5th bit on
			m_Bits |= 0x20; // turn the 6th bit on
			m_Bits |= 0x40; // turn the 7th bit on
			m_Bits |= 0x80; // turn the 8th bit on
		}
		else
		{
			m_Bits &= ~(1<<0); //turn the 1st bit off
			m_Bits &= ~(1<<1); //turn the 2nd bit off
			m_Bits &= ~(1<<2); //turn the 3rd bit off
			m_Bits &= ~(1<<3); //turn the 4th bit off
			m_Bits &= ~(1<<4); //turn the 5th bit off
			m_Bits &= ~(1<<5); //turn the 6th bit off
			m_Bits &= ~(1<<6); //turn the 7th bit off
			m_Bits &= ~(1<<7); //turn the 8th bit off
		}
	}
	~TheWord()
	{
		// in DEBUG(where the memory manager is very slow) - printf() so we know everything is OK
		// and we are not stuck in a loop - printf() every 10000th destroyed
#ifdef _DEBUG
		if (!bSmallTest)
		{
			if ((g_nWordObjInstances-- % 10000) == 0)
				printf(".");
		}
#endif
	}
	__int64 GetCount() {return m_nkCount;}
};
IMPLEMENT_FACTORY(TheWord, WordObj)
// **********************  The Word Object *****************************





// **********************  The Controller Object ***********************
class AllTheWords: public XMLObject
{
	int m_nSort;  

	GList					m_listWords;
	GQSortArray				m_quickSortWords;
	GBTree					m_binaryTreeWords;
	GHash					m_hashTableWords;
#ifndef NO_SPARSE_HASH
	GSparseHash   m_SparseHash;
#endif

	void MapXMLTagsToMembers()
	{
		if (m_nSort == 0)	MapMember(&m_listWords,		  TheWord::GetStaticTag());
		if (m_nSort == 1)	MapMember(&m_quickSortWords,  TheWord::GetStaticTag()); 
		if (m_nSort == 2)	MapMember(&m_binaryTreeWords, TheWord::GetStaticTag());  
		if (m_nSort == 3)	MapMember(&m_hashTableWords,  TheWord::GetStaticTag());
#ifndef NO_SPARSE_HASH 
		if (m_nSort == 4)	MapMember(&m_SparseHash, (KeyedDataStructureAbstraction *)&g_SparseHashAbstraction, TheWord::GetStaticTag());
#endif

	}

public:
	DECLARE_FACTORY(AllTheWords, AllWords)
	AllTheWords() {m_nSort = 0;}
	AllTheWords(int nSort) : m_quickSortWords(500000,10000), m_hashTableWords(777001)  { m_listWords.EnableCache(); m_nSort = nSort;  ModifyObjectBehavior(NO_OBJECT_DATA); }
	virtual ~AllTheWords(){  }
	__int64 Iterate()
	{
		__int64 nCount = 0;
		if (m_nSort == 0)
		{
			GListIterator it(&m_listWords);
			while (it())
			{
				TheWord *p = (TheWord *)it++;
				nCount++;
			}
		}
		else if (m_nSort == 1)
		{
			__int64 n = m_quickSortWords.GetItemCount();
			for(__int64 i = 0;i < n; i++)
			{
				TheWord *p = (TheWord *)m_quickSortWords[i];
				nCount++;
			}
		}
		else if (m_nSort == 2)
		{
			// 1 = Alphabetical, 0 = Reverse Alphabetical, 2 = Insertion Order
			GBTreeIterator it(&m_binaryTreeWords,2);
			while(it())
			{
				TheWord *p = (TheWord *)it++;
				nCount++;
			}
		}
		else if (m_nSort == 3)
		{
			GHashIterator it(&m_hashTableWords);
			while(it())
			{
				TheWord *p = (TheWord *)it++;
				nCount++;
			}
		}
#ifndef NO_SPARSE_HASH
		else if (m_nSort == 4)
		{
			for (sparse_hash_map<__int64,void *>::const_iterator it = m_SparseHash.begin();      it != m_SparseHash.end(); ++it)
			{
				TheWord *p = (TheWord *)it->second;
				nCount++;
			}
		}
#endif
		return nCount;
	}

	TheWord *Search(const char *pzKey)
	{
		if (m_nSort == 0)
		{
			GListIterator it(&m_listWords);
			while (it())
			{
				TheWord *pSearchResult = (TheWord *)it++;
				const char *pzOID = pSearchResult->getOID();
				if (strcmp(pzOID,pzKey) == 0)
				{
					return pSearchResult;
				}
			}
		}
		else if (m_nSort == 1)
			return (TheWord *)m_quickSortWords.Search( pzKey );
		else if (m_nSort == 2)
			return (TheWord *)m_binaryTreeWords.search( pzKey );
		else if (m_nSort == 3)
			return (TheWord *)m_hashTableWords[pzKey];
#ifndef NO_SPARSE_HASH
		else if (m_nSort == 4)
			return (TheWord *)m_SparseHash.Search(pzKey);
#endif
		return 0;			
	}
	GList *GetList()  {return &m_listWords;}
	GBTree *GetTree()  {return &m_binaryTreeWords;}
};
IMPLEMENT_FACTORY(AllTheWords, AllWords)
// **********************  The Controller Object *****************************

int CreateInputFile(GString &strMiddleWord);


char *Algorithms[] ={ "GList", "GQSortArray", "GBTree", "GHash", "GSparseHash" };
int main(int argc, char* argv[])
{
	try
	{
		GString strSearchKey;// A word from "Truth.txt" or "TheWholeTruth.txt" to search for.
		if (CreateInputFile(strSearchKey)) 
		{

//			for(int i=0; i < 1; i++) // if you want to test only  - GList
//			for(int i=1; i < 2; i++) // if you want to test only  - GQSort
//			for(int i=2; i < 3; i++) // if you want to test only  - GBTree
//			for(int i=3; i < 4; i++) // if you want to test only  - GHash
//			for(int i=4; i < 5; i++) // if you want to test only  - GSparseHash
//			for(int i=3; i < 5; i++) // GHash and GSparseHash
			for(int i=0; i < 5; i++) // ---------------------------------------->Run all of them
			{
			if (!g_UseSparseHash && i==4)
				continue;

				printf("\n--------  %s -------- \n", Algorithms[i]);
				AllTheWords Example(i); 
				
				// Load the XML Document into memory
				GPerformanceTimer *profLoad = new GPerformanceTimer("InsertObjects");
				Example.FromXMLFile("DebugInput.xml");
				delete profLoad;// done loading


				// Walk through all the memory objects, output the object count
				GPerformanceTimer *profIter = new GPerformanceTimer("Iterate All  ",0);
				printf("%lld objects in ",Example.Iterate());
				delete profIter;


				// Search for an existing object in memory - then write it to disk
				// todo: a more diverse keyset comparison must be made 10 (Honestly) random keys will be enough.  It will take far longer to execute it than it will to write the code.
				GPerformanceTimer *profSearch = new GPerformanceTimer("Search Find  ",0);
				TheWord *pTheFoundWord = Example.Search(strSearchKey);
				delete profSearch;



				GString strTestReslutsFileName(Algorithms[i]); strTestReslutsFileName += "Debug.xml";
				if ( pTheFoundWord ) 
				{	
					pTheFoundWord->ToXMLFile(1024,strTestReslutsFileName);
//					pTheFoundWord will be represented in the XML like this
//					-----------------------------------------------------
//					<WordObj>
//						<theWord>the</theWord>
//						<Count>13620</Count>
//						<Integer>7</Integer>
//					</WordObj>

					// MappedOID Update = "Late Bound" OID is defined by MapObjectID()
					// -----------------
					// Update an existing object instance in memory from a "standard" XML input - change <Integer> from 7 to 777
					// THE KEY(aka the oid) CANNOT CHANGE (or you'll make a new object instance) 
					// todo:put that loop of a more diverse keyset around this and all relevant data structure actions.
					GString strUpdate;
					strUpdate << "<AllWords><WordObj><theWord>" << pTheFoundWord->m_pzkTheWord << "</theWord><Count>";
					strUpdate << pTheFoundWord->m_nkCount << "</Count><Integer>777</Integer></WordObj></AllWords>";


					GPerformanceTimer *profUpdate = new GPerformanceTimer("Update Object",0);
					Example.FromXMLX(strUpdate);
					delete profUpdate;


					// search for the object we just updated - to test the XMLFactory indexing of the objects.  
					// It will find a memory address that we already have so we can be sure is the same object - not another instance.
					TheWord *pUpdatedAndFound = Example.Search(strSearchKey);
					if (pUpdatedAndFound != pTheFoundWord)
					{
						// the memory addresses must be the same - they both point to the same object instance.
						printf("SEARCH %d for updated object FAILED!\n",i);
					}
					else
					{
						strTestReslutsFileName.Insert(0,"Update",strlen("Update"));;

						pTheFoundWord->ToXMLFile(1024,strTestReslutsFileName);
	//					pTheFoundWord represented in the XML
	//					-----------------------------------
	//					<WordObj>
	//						<theWord>the</theWord>
	//						<Count>13620</Count>
	//						<Integer>777</Integer>
	//					</WordObj>


						// MappedOID Update = "Early Bound" OID is defined by the "oid" attribute in the XML.
						// -----------------
						// Update an existing object instance in memory - change Integer from 777 to 21
						// THE KEY(aka the oid) CANNOT CHANGE (or you'll make a new object instance)
						GString strOID;  
						strOID << pTheFoundWord->m_pzkTheWord << pTheFoundWord->m_nkCount;

						strUpdate.Empty();
						strUpdate << "<AllWords><WordObj oid='" << strOID << "'><theWord>" << pTheFoundWord->m_pzkTheWord
                                  << "</theWord><Count>" << pTheFoundWord->m_nkCount << "</Count><Integer>21</Integer></WordObj></AllWords>";


						GPerformanceTimer *profUpdate2 = new GPerformanceTimer("Update Faster",0);
						Example.FromXMLX(strUpdate);
						delete profUpdate2;


						// now search for the object we just updated
						pUpdatedAndFound = Example.Search(strSearchKey);
						if (pUpdatedAndFound != pTheFoundWord)
						{
							// the memory addresses must be the same - they both point to the same object instance.
							printf("SEARCH %d for updated object FAILED!\n",i);
						}
						else
						{
							strTestReslutsFileName.Insert(0,"2nd",3);

							pUpdatedAndFound->ToXMLFile(1024,strTestReslutsFileName);
	//						<WordObj>
	//							<theWord>the</theWord>
	//							<Count>13620</Count>
	//							<Integer>21</Integer>
	//						</WordObj>
					
						}
					}
				}
				else
				{
					printf("SEARCH %d FAILED!\n",i);
				}


				// Walk through all the memory objects(again), output the object count see that the count did not change
				GPerformanceTimer *profIter2 = new GPerformanceTimer("Iterate All  ",0);
				printf("%lld objs in ",Example.Iterate());
				delete profIter2;


				// Search for a memory object that will not be found - 
				// todo: add to this test a set of searches for a more diverse keyset comparison.
				GPerformanceTimer *profSearch2 = new GPerformanceTimer("Search NoFind",0);
				TheWord *pNull = Example.Search("Non Existing Key");   
				delete profSearch2;
				if (pNull) 
				{	
					printf("SEARCH %d FAILED!  EXPECTING NULL.\n",i);
				}


				// Create the XML then Save it. Time each task.
				GString strOutFile(Algorithms[i]); strOutFile += "OutDebug.xml";
				g_xml->Empty();
				GPerformanceTimer *profSave  = new GPerformanceTimer("Create XML   "); 
				Example.ToXML(g_xml);
				delete profSave;


				GPerformanceTimer *profSave2  = new GPerformanceTimer("XML To Disk  "); 
				g_xml->ToFile(strOutFile);
				delete profSave2;

				// iterate the objects, and delete them
				GPerformanceTimer *profRelease = new GPerformanceTimer("Free Memory  ");
				Example.DecRef(true);
				delete profRelease;


				// store the XML compressed
				GString GZip;
				GZip.Compress(g_xml);
				strOutFile += ".zipped";
				GZip.ToFile(strOutFile);

				GString strCompressionResults;
				GString strLen1;
				strLen1 << g_xml->Length();
				GString strLen2;
				strLen2 << GZip.Length();
				strCompressionResults << "---------------------- Compressed " << strLen1.CommaNumeric() << " bytes of XML to "  << strLen2.CommaNumeric() << "\n";
				printf( strCompressionResults.Buf() );
				
				// Decompress the stored file, compare it to the Origin prior to compression
				GZip.DeCompress(strOutFile);
				if ( g_xml->Compare(GZip._str) == 0 )
				{
					// as expected GZip contains the same data g_xml
				}
				else
				{
					printf("Error - Decompression match failed!!!!\n");
				}
			}
		}
	}
	catch( GException &rErr)
	{
		printf(rErr.GetDescription());
	}
	
	if (g_xml)	
		delete g_xml;

	printf("\n\nPress any key to begin calling global destructors ...\n");
	fflush(stdout);

#ifndef _WIN32
	getchar();
#else
	_getch();
#endif

	return 0;
}



__int64 LoadWords(GStringList *pDestination);
int CreateInputFile(GString &strMiddleWord)
{
	try
	{
		printf("This sample works with HUGE test files.\n");
		printf("It is VERY VERY slow under a debugger.\n");
		printf("If you dont have enough RAM, or want to speed it up.......\n");
		printf("Delete TheWholeTruth.txt as many folks must do in their reality..... \n");
		printf("It will use Truth.txt which will obtain evidence you can see.\n\n");
		printf("Note: 777 milliseconds = 777,000 microseconds\nNote: The 32 bit build counts cpu clock cycles\n\n");
			
		
		// Create an XML document
		GPerformanceTimer *profCreate = new GPerformanceTimer("Creating Object Instances");

		// Open file and read each word (space,tab,CR separated) into List
		GStringList lstWords; 

		lstWords.EnableCache(0); 
		__int64 nWordCount = LoadWords(&lstWords);

		AllTheWords Ex1(0); 
		int i = 0;
		// For each word create a simple TheWord object 
		GStringIterator it( &lstWords );
		while (it())
		{
			const char *pWord = it++;

			// and add it to the List in the container
			Ex1.GetList()->AddLast( new TheWord(pWord, ++i, i%2) );
			
			// Hang onto the middle word, we'll search for it later
			if (i == (nWordCount / 2) )
			{
				strMiddleWord.Format("%s%d",pWord,i);
			}

		}
		// done creating the document in memory, print execution time
		delete profCreate;


		// we will hang onto this and use it for each test
		if (bSmallTest)
		{
			g_xml = new GString(3744000);
		}
		else
		{
			g_xml = new GString(83118000);
		}


		// Create an XML document on disk
		GPerformanceTimer *timedXMLDocCreate;
		if (bSmallTest)
			timedXMLDocCreate = new GPerformanceTimer("Create 3.7 MB XML Document");
		else				  
			timedXMLDocCreate = new GPerformanceTimer("Create 81 MB XML Document");
		Ex1.ToXML(g_xml);
		delete timedXMLDocCreate; // done

		g_xml->Empty();

		// Measure time to Re-create a new version (often with new data) of a frequemntly accessed XML document
		GPerformanceTimer *timedXMLDocCreate2;
		if (bSmallTest)
			timedXMLDocCreate2 = new GPerformanceTimer("Create 3.7 MB XML Document Faster");
		else				  
			timedXMLDocCreate2 = new GPerformanceTimer("Create 81 MB XML Document Faster");
		Ex1.ToXML(g_xml);
		delete timedXMLDocCreate2; // done



		GPerformanceTimer *timedXMLDocWrite;
		timedXMLDocWrite = new GPerformanceTimer("Save To Disk");
		g_xml->ToFile("DebugInput.xml");
		delete timedXMLDocWrite; // done




		// Free the memory
		GPerformanceTimer *profRelease = new GPerformanceTimer("Releasing memory");
		Ex1.DecRef(true);
		delete profRelease;
		
		return 1;
	}
	catch(GException &rErr)
	{
		printf(rErr.GetDescription());
		rErr.AddErrorDetail("Creating Sample program Input files");
//		throw rErr;	// This is how Exceptions get thrown all the way from the xmlLex to as far as you want to go.
	}
	return 0; // fail, exception has been printed
}

// read words in from a text file (must have a space after the last word)
__int64 LoadWords(GStringList *pDestination)
{
	GString strAllWords;
	GString strBackOneFolder;
	if (bSmallTest)
		goto ITS_TIME_FOR_THIS;

	if ( !strAllWords.FromFile("TheWholeTruth.txt",0) )
	{
		// strBackOneFolder is so it works from the same folder as the input or any folder 1 deeper
		strBackOneFolder = "..\\\\TheWholeTruth.txt";
		if (!strAllWords.FromFile(strBackOneFolder,0))
		{
ITS_TIME_FOR_THIS:
			bSmallTest = 1;
			if ( !strAllWords.FromFile("Truth.txt",0) )
			{
				strBackOneFolder = "..\\\\Truth.txt"; 
				if ( !strAllWords.FromFile(strBackOneFolder,0))
				{
					printf("FAILED to read input file Truth.txt\n");
					return 0;
				}
			}
			
		}
	}
	char *pTemp = (char *)(const char *)strAllWords;
	char *pLastWordStart = pTemp;
	for(int i=0; i<strAllWords.Length(); i++)
	{
		if (pTemp[i] == ' ' || pTemp[i] == '\t' || pTemp[i] == 13 || pTemp[i] == 10)
		{
			pTemp[i] = 0;
			if (strlen(pLastWordStart) ) 
			{
				pDestination->AddLast(pLastWordStart);
			}
			pLastWordStart = &pTemp[i+1];
		}
	}
	return pDestination->Size();
}



#pragma message("You are building Knowledge......")
