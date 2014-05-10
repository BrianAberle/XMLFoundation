//
// This will become a collection of examples concerning large data sets - currently it has 1 case.
//

// Case 1: This example creates 1 million (or 1/2 million) objects - as if from stored data to test the 
//         time that it takes to convert the memory objects to XML.
//


#include <XMLFoundation.h>

#ifdef _WIN32
	#include <conio.h> // for getch()
#endif

#include "GPerformanceTimer.h"



// MAKE_OTHER_OBJECTS defines the size of this test.  You may need to run this example in release or reduce this number.
// 1,000,000 Objects - runs a good 64 bit test on a computer with 6GB of RAM creating 346 MB of XML in 1 ToXML() call.
#if defined(_MSC_VER) && _MSC_VER <= 1200 
	#define MAKE_OTHER_OBJECTS			500000
#else
	#define MAKE_OTHER_OBJECTS			1000000
#endif




bool g_bMyOverrideToXML = 0;
class MyOtherCustomObject : public XMLObject
{
protected:
	DECLARE_FACTORY(MyOtherCustomObject, MyOtherCustomObject);

	int m_nA;
	int m_nB;
	int m_nC;
	int m_nD;
	int m_nE;
	int m_nF;
	int m_nG;
public:
	// --------------------------------------------------------------------------------------------
	// NOTE (part 1)   This supplies ALL the information required to READ and WRITE XML
	void MapXMLTagsToMembers()
	{	
		MapMember(		&m_nA,			"TheAAATown"				);
		MapMember(		&m_nB,			"BigBigB"					);
		MapMember(		&m_nC,			"CCDoYouC"					);
		MapMember(		&m_nD,			"IAmMileHighFromDDDenver"	);
		MapMember(		&m_nE,			"sEEEiCanFly"				);
		MapMember(		&m_nF,			"ICanFFFlyLikeAFlyLikeA"	);
		MapMember(		&m_nG,			"LikeAGGGuru"				);
	}
	
	// NOTE (part 2) - However, this virtual overload of ToXML() is a little faster in release, and a lot faster in a debugger, 
	//				   and also a maintenance liability - these are tradeoffs
	//				   The greatness in overloading ToXML() is the infinate flexibility with ultimate performance for custom object behavior.
	//				   Since ToXML() is in the recursive call chain, you can easily serialize member objects derived from XMLObject
	//				   also - Since ToXML() is in the recursive call chain, you can override 1 Object type that is way down in a call chain for example:
	//				   "Customer" conatains a list of "Orders" that contain list of "LineItems" that overload ToXML().  Customer.ToXML() includes your 
	//				   custom "LineItems" overloaded ToXML results in the subset of the grander XML document which is a "Customer" object document.
	// --------------------------------------------------------------------------------------------
	virtual bool ToXML(GString* xml, int nTabs = 0, const char *TagOverride = 0, unsigned int nSerializeFlags = FULL_SERIALIZE, StackFrameCheck *pStack = 0, const char *pzSubsetOfObjectByTagName = 0)
	{

		if (!g_bMyOverrideToXML)
			return XMLObject::ToXML(xml, nTabs,TagOverride,nSerializeFlags,pStack,pzSubsetOfObjectByTagName);

		// note that this code is only executed when (g_bMyOverrideToXML), 
		// this is does the same thing as the XMLObject base class handler.  Here we do it by hand.
		(*xml) << "\r\n\t\t<MyOtherCustomObject>\r\n\t\t\t";
		(*xml) << "<TheAAATown>"				<< m_nA <<  "</TheAAATown>\r\n\t\t\t";
		(*xml) << "<BigBigB>"					<< m_nB <<  "</BigBigB>\r\n\t\t\t";
		(*xml) << "<CCDoYouC>"					<< m_nC <<  "</CCDoYouC>\r\n\t\t\t";
		(*xml) << "<IAmMileHighFromDDDenver>"	<< m_nD <<  "</IAmMileHighFromDDDenver>\r\n\t\t\t";
		(*xml) << "<sEEEiCanFly>"				<< m_nE <<  "</sEEEiCanFly>\r\n\t\t\t";
		(*xml) << "<ICanFFFlyLikeAFlyLikeA>"	<< m_nF <<  "</ICanFFFlyLikeAFlyLikeA>\r\n\t\t\t";
		(*xml) << "<LikeAGGGuru>"				<< m_nG <<  "</LikeAGGGuru>\r\n\t\t";

		(*xml) << "</MyOtherCustomObject>";
		
		return 1;
	}

	virtual ~MyOtherCustomObject() {};
	MyOtherCustomObject(){}
	MyOtherCustomObject(int i) 
	{
		m_nA = m_nB = m_nC = m_nD = m_nE = m_nF = m_nG = i;
	}


};


class TheContainer : public XMLObject
{
protected:
	DECLARE_FACTORY(TheContainer, TheContainer);
	TheContainer() {};	

	GList m_lstOtherObjects; // contains instances of MyOtherCustomObject's

public:
	TheContainer(bool bBuildData);
	virtual ~TheContainer() {};
	void MapXMLTagsToMembers()
	{	
		MapMember(&m_lstOtherObjects,	MyOtherCustomObject::GetStaticTag(),	"ListOfAllMyOtherCustomObjects"	);
	}
};
IMPLEMENT_FACTORY(MyOtherCustomObject,	MyOtherCustomObject);


//					 Object		XML Element
IMPLEMENT_FACTORY(TheContainer,	TheContainer);



TheContainer::TheContainer(bool bBuildData)
{
	if (bBuildData)
	{
		for (int i=0; i < MAKE_OTHER_OBJECTS; i++)
		{
			MyOtherCustomObject* pOther = new MyOtherCustomObject(i);
			m_lstOtherObjects.AddLast(pOther);
		}
	}
}


int main(int argc, char* argv[])
{
	/////////////////////////////////////////////////////////////
	// BigData Test CASE 1 - Begin
	/////////////////////////////////////////////////////////////
	TheContainer xx(1);
	

	// pre-allocate the GString to be big enough already
	GString strXML( MAKE_OTHER_OBJECTS * sizeof(MyOtherCustomObject) + 1024 );


	GString strResults;
	GPerformanceTimer PerfTimer("ToXML1");

	strXML = xx.ToXML();
	PerfTimer.PerformanceStop(&strResults);
	strXML.Empty();
	
	// the second time is faster, since the MemberDescriptors are prebuilt - we can update the data values
	// and repeat the rendering of the document with ToXML(), the MemberDescriptors are not rebuilt - so 
	// once the system is running this time would be the 'normal' time to create the document in strXML
	GString strTime2;
	GPerformanceTimer PerfTimer2("ToXML2");
	strXML = xx.ToXML();
	PerfTimer2.PerformanceStop(&strTime2);
	strXML.Empty();

	// the third way is the fastest, but it involves a custom overload of ToXML(), and it's not that much faster
	g_bMyOverrideToXML = 1;
	GString strTime3;
	GPerformanceTimer PerfTimer3("ToXML3");
	strXML = xx.ToXML();
	PerfTimer3.PerformanceStop(&strTime3);



	GString fmtLen;
	fmtLen << strXML.Length();

	strResults << " " << strTime2	<< " " << strTime3 <<  "\n" << fmtLen.CommaNumeric() << " bytes of XML";
	strResults.Prepend("Results: ");

	printf("\n\n");
	printf(strResults); 
	printf("\n");

	strXML.SetLength(512);	// print out the first 512 of the 340+ million bytes in that GSting
	printf(strXML.Buf());

	/////////////////////////////////////////////////////////////
	// BigData Test CASE 1 - End
	/////////////////////////////////////////////////////////////










	printf("\n\n\n\t\tPRESS A KEY!");
#ifndef _WIN32
	getchar();
#else
	_getch();
#endif


	return 0;
}


