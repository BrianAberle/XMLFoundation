#include <XMLFoundation.h>
#include <GPerformanceTimer.h>

#ifdef _WIN32
	#include <conio.h> // for getch()
#endif


// This example was developed for the IETF discussions.
//
// Topic: Is this Concept/Design/Implementation a protocol?
//
// Answer: Yes, it can be considered a protocol if you add some small constraints to "Standard XML".
//         These additions, ALTHOUGH THEY ARE OPTIONAL, do define a protocol.  The reason that this 
//		   is done will be shown here in this example.
// 
// This specialized "XML Object Parser" we might call it knows a special key word that no other 
// parser on earth knows, it is "oid"  It was designed to be brief, "oid" Stands for Object ID.
// the attribute "oid" if to be recognized as valid as an Object Index is POSITIONAL and CASE SENSITIVE.
// Putting "oid" as the 3rd attribute has no effect, this violates some thought on positional significance 
// of attribute order because this special optimization demands that "oid" be #1 in position as the attribute.
// This is the protocol.  Yep.  Cant get around that.
//
// This is why: Without this protocol - XML updates are SLOW.  This is how the world works WITHOUT "oid":
// The tokenizer is getting fragments of data that it cannot possibly identify as an object until a
// "Temporary Object" is built to contain all of the data.  This "DOM like" Temporary object might only be a  
// small subset of the actual object - a complex object may have many member variables and  the xml used to 
// update it contains only the key or key parts plus the changes - a delta rather than an overwrite.  
// Since the key to the object instance exists within the data it becomes 'necessary' to create the 
// temporary object because in that temporary object is the information needed to index the actual object.
//
// In the world with an "oid" (that must appear in just the right place(1st) and case(lower)) we do not 
// need any temporary object or memory to hold the data because the tokenizer peeks ahead one token to look 
// for "oid". If it exists, we obtain the one and only object instance and parse directly into it.  This
// eliminates a terrible replication liability, because that temporary space is more 'up to date' with 
// the most recent data for a brief moment.  In a distributed system, transactionally you must account
// for the state in time of that 'shadow object' likely a DOM tree as an intermediary an undesirable
// state of the replication process in a distributed data structure that uses XML for data markup.
//
// This is a major performance improvement utilizing much less memory and CPU.
// This is a protocol.  Speed is the reason. 
// Why was ETag added to HTTP in 1.1?  
// HTTP 1.0 didnt cache - I remember when ETag was added to HTTP 1.1 (ServerCore.cpp uses it)
// Caching was a needed feature in HTTP, no less than the "oid" in XML.
// so should we call this XML++, or XML 1.2, or should we ignore it because its politically inconvenient?
//
// There is already an example which shows the performance improvement gained by the "oid"
// The 'IndexObjects' example times an "Update" vs "Update Faster" and you can see the difference in that
// example also. However, that example was not designed as a case to emphasize exactly how major of a 
// performance difference it is.  The larger the temporary object, the more magnified the time and memory in 
// dealing with it - that example uses many very small objects - and even with those very small objects
// you can see that the oid update is faster - look how much faster oid updates are with 
// a larger more complex object .....


// notice that this object has no key, no index, no oid
class MyAddressObject : public XMLObject
{
	GString m_strLine1;
	GString m_strLine2;
	GString m_strCity;
	GString m_strState;
	GString m_strZip;
	virtual void MapXMLTagsToMembers()
	{
		MapMember(	&m_strLine1,"Line1");
		MapMember(	&m_strLine2,"Line2");
		MapMember(	&m_strCity,	"City");
		MapMember(	&m_strState,"State");
		MapMember(	&m_strZip,	"Zip");
	}

	DECLARE_FACTORY(MyAddressObject, Address) 

	MyAddressObject(){} 
	~MyAddressObject(){};
};
IMPLEMENT_FACTORY(MyAddressObject, Address)



class MyCustomObject : public XMLObject
{
public:							// make public here for example simplicity - this is not required
	GString m_strString;		// A String Member
	GString m_strColor;			// An attribute , not an element
	int m_nInteger;				// An Integer Member
	char m_szNative[10];		// a fixed 10 byte buffer
	bool m_bBool;				// bool
	GStringList m_strList;      // A String List

	MyAddressObject m_Address;				 // MyCustomObject contains 1	 Address in m_pAddress
	ObjectPtr <MyAddressObject> m_pAddress2; // MyCustomObject contains 0..1 Address in  m_pAddress2
	GList m_lstAddresses;					 // MyCustomObject contains 0..n Address's in  m_lstAddresses

	
	int m_nID;  // typical approach, sticking the KEY down in the data


	int m_nA;
	int m_nB;
	int m_nC;
	int m_nD;
	int m_nE;
	int m_nF;
	int m_nG;


	virtual void MapXMLTagsToMembers()
	{
		
		MapObjectID("ID", 1);
		MapMember(	&m_nID,	"ID");

		// map objects using various containment techniques
		MapMember(&m_Address);		
		MapMember(&m_pAddress2,		MyAddressObject::GetStaticTag(),	"WrapperTag");
		MapMember(&m_lstAddresses,	MyAddressObject::GetStaticTag(),	"ListOfAddresses");

		//			Member variable		XML Element						Wrapper

		MapMember( &m_strList,		"StringList",						"Wrapper");
		MapMember(	&m_nInteger,	"Number");
		MapMember(	&m_strString,	"String");
		MapMember(	&m_bBool,		"Bool");
		MapMember( m_szNative,		"FixedBuffer", sizeof(m_szNative) );

		MapMember(		&m_nA,		"OneLetterA"	);
		MapMember(		&m_nB,		"TwoLetterB"	);
		MapMember(		&m_nC,		"ThreeLetterC"	);
		MapMember(		&m_nD,		"FourLetterD"	);
		MapMember(		&m_nE,		"FivePointE"	);
		MapMember(		&m_nF,		"SixPointF"		);
		MapMember(		&m_nG,		"GGGGGGG"		);

		MapAttribute(&m_strColor,	"Color");
	}
	
	// 'this' type, followed by the XML Element name, normally DECLARE_FACTORY() is in an .h file
	DECLARE_FACTORY(MyCustomObject, Thing) 

	MyCustomObject(){} // keep one constructor with no arguments
	~MyCustomObject(){};
};
// IMPLEMENT_FACTORY() must exist in a .CPP file - not an .h file - one for every DECLARE_FACTORY()
IMPLEMENT_FACTORY(MyCustomObject, Thing)


// this class will contain them all
class MyCustomContainer : public XMLObject
{
public:
	GHash m_hashCache;

	void MapXMLTagsToMembers()
	{
		MapMember(&m_hashCache,  MyCustomObject::GetStaticTag());
	}
	DECLARE_FACTORY(MyCustomContainer, AllMyObjects)

};
IMPLEMENT_FACTORY(MyCustomContainer, AllMyObjects)

/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////

//
// This is the XML we'll process.
//



// Object Index 1
char pzXML1[] = 
"<AllMyObjects>"
"<Thing oid='1' Color='White'>"
	"<ID>1</ID>"						// it is not necessary to duplicate the key in the data
	"<String>Hello World!</String>"  
	"<Number>1984</Number>"
	"<Bool>True</Bool>"
	"<FixedBuffer>native</FixedBuffer>"
	"<OneLetterA>1</OneLetterA>"
	"<TwoLetterB>2</TwoLetterB>"
	"<ThreeLetterC>3</ThreeLetterC>"
	"<FourLetterD>4</FourLetterD>"
	"<FivePointE>5</FivePointE>"
	"<SixPointF>6</SixPointF>"
	"<GGGGGGG>7</GGGGGGG>"
	"<Address>"
		"<Line1>Address 1 is here</Line1>"
		"<Line2>second line of address 1</Line2>"
		"<City>Cartersville</City>"
		"<State>Georgia</State>"
		"<Zip>30121</Zip>"
	"</Address>"
	"<WrapperTag>"
		"<Address>"
			"<Line1>Address 2 is here</Line1>"
			"<Line2>line 222</Line2>"
			"<City>Antioch</City>"
			"<State>California</State>"
			"<Zip>94509</Zip>"
		"</Address>"
	"</WrapperTag>"
	"<ListOfAddresses>"
		"<Address>"
			"<Line1>Address 3 is here</Line1>"
			"<Line2>line 2 of 333</Line2>"
			"<City>Northglenn</City>"
			"<State>Colorado</State>"
			"<Zip>80233</Zip>"
		"</Address>"
		"<Address>"
			"<Line1>Address 4 is here</Line1>"
			"<Line2>line 2 of 444</Line2>"
			"<City>Atlanta</City>"
			"<State>GA</State>"
			"<Zip>30333</Zip>"
		"</Address>"
	"</ListOfAddresses>"
	"<Wrapper>"
	     "<StringList>one</StringList>"
	     "<StringList>two</StringList>"
	     "<StringList>3.33333333333333</StringList>"
	     "<StringList>444</StringList>"
	     "<StringList>5 Point Part</StringList>"
	     "<StringList>6 Point Part</StringList>"
	     "<StringList>777 G Star</StringList>"
	     "<StringList>eight</StringList>"
	     "<StringList>nine</StringList>"
	     "<StringList>ten</StringList>"
	"</Wrapper>"
"</Thing>"
"</AllMyObjects>";
//////////////////////////////////////////////////////////////////////////////
char pzXML1UpdateSlow[] = 
"<AllMyObjects>"
"<Thing Color='White'>"
	"<ID>1</ID>"						// it is not necessary to duplicate the key in the data
	"<String>Slow Hello World!</String>"  
	"<Number>1984</Number>"
	"<Bool>True</Bool>"
	"<FixedBuffer>native</FixedBuffer>"
	"<OneLetterA>1</OneLetterA>"
	"<TwoLetterB>2</TwoLetterB>"
	"<ThreeLetterC>3</ThreeLetterC>"
	"<FourLetterD>4</FourLetterD>"
	"<FivePointE>5</FivePointE>"
	"<SixPointF>6</SixPointF>"
	"<GGGGGGG>7</GGGGGGG>"
	"<Address>"
		"<Line1>Address 1 is here</Line1>"
		"<Line2>second line of address 1</Line2>"
		"<City>Cartersville</City>"
		"<State>Georgia</State>"
		"<Zip>30121</Zip>"
	"</Address>"
	"<WrapperTag>"
		"<Address>"
			"<Line1>Address 2 is here</Line1>"
			"<Line2>line 222</Line2>"
			"<City>Antioch</City>"
			"<State>California</State>"
			"<Zip>94509</Zip>"
		"</Address>"
	"</WrapperTag>"
	"<ListOfAddresses>"
		"<Address>"
			"<Line1>Address 3 is here</Line1>"
			"<Line2>line 2 of 333</Line2>"
			"<City>Northglenn</City>"
			"<State>Colorado</State>"
			"<Zip>80233</Zip>"
		"</Address>"
		"<Address>"
			"<Line1>Address 4 is here</Line1>"
			"<Line2>line 2 of 444</Line2>"
			"<City>Atlanta</City>"
			"<State>GA</State>"
			"<Zip>30333</Zip>"
		"</Address>"
	"</ListOfAddresses>"
	"<Wrapper>"
	     "<StringList>one</StringList>"
	     "<StringList>two</StringList>"
	     "<StringList>3.33333333333333</StringList>"
	     "<StringList>444</StringList>"
	     "<StringList>5 Point Part</StringList>"
	     "<StringList>6 Point Part</StringList>"
	     "<StringList>777 G Star</StringList>"
	     "<StringList>eight</StringList>"
	     "<StringList>nine</StringList>"
	     "<StringList>ten</StringList>"
	"</Wrapper>"
"</Thing>"
"</AllMyObjects>";
//////////////////////////////////////////////////////////////////////////////
char pzXML1UpdateFast[] = 
"<AllMyObjects>"
"<Thing oid='1' Color='White'>"
	"<ID>1</ID>"						// it is not necessary to duplicate the key in the data
	"<String>Fast Hello World!</String>"  
	"<Number>1984</Number>"
	"<Bool>True</Bool>"
	"<FixedBuffer>native</FixedBuffer>"
	"<OneLetterA>1</OneLetterA>"
	"<TwoLetterB>2</TwoLetterB>"
	"<ThreeLetterC>3</ThreeLetterC>"
	"<FourLetterD>4</FourLetterD>"
	"<FivePointE>5</FivePointE>"
	"<SixPointF>6</SixPointF>"
	"<GGGGGGG>7</GGGGGGG>"
	"<Address>"
		"<Line1>Address 1 is here</Line1>"
		"<Line2>second line of address 1</Line2>"
		"<City>Cartersville</City>"
		"<State>Georgia</State>"
		"<Zip>30121</Zip>"
	"</Address>"
	"<WrapperTag>"
		"<Address>"
			"<Line1>Address 2 is here</Line1>"
			"<Line2>line 222</Line2>"
			"<City>Antioch</City>"
			"<State>California</State>"
			"<Zip>94509</Zip>"
		"</Address>"
	"</WrapperTag>"
	"<ListOfAddresses>"
		"<Address>"
			"<Line1>Address 3 is here</Line1>"
			"<Line2>line 2 of 333</Line2>"
			"<City>Northglenn</City>"
			"<State>Colorado</State>"
			"<Zip>80233</Zip>"
		"</Address>"
		"<Address>"
			"<Line1>Address 4 is here</Line1>"
			"<Line2>line 2 of 444</Line2>"
			"<City>Atlanta</City>"
			"<State>GA</State>"
			"<Zip>30333</Zip>"
		"</Address>"
	"</ListOfAddresses>"
	"<Wrapper>"
	     "<StringList>one</StringList>"
	     "<StringList>two</StringList>"
	     "<StringList>3.33333333333333</StringList>"
	     "<StringList>444</StringList>"
	     "<StringList>5 Point Part</StringList>"
	     "<StringList>6 Point Part</StringList>"
	     "<StringList>777 G Star</StringList>"
	     "<StringList>eight</StringList>"
	     "<StringList>nine</StringList>"
	     "<StringList>ten</StringList>"
	"</Wrapper>"
"</Thing>"
"</AllMyObjects>";
//////////////////////////////////////////////////////////////////////////////

// Object Index 2 AND 3
char pzXML2[] = 
"<AllMyObjects>"
"<Thing oid='2' Color='Black'>"
	"<ID>2</ID>"						// it is not necessary to duplicate the key in the data
	"<String>2222222222</String>"  
	"<Number>1984v2</Number>"
	"<Bool>True</Bool>"
	"<FixedBuffer>2native</FixedBuffer>"
	"<OneLetterA>21</OneLetterA>"
	"<TwoLetterB>22</TwoLetterB>"
	"<ThreeLetterC>23</ThreeLetterC>"
	"<FourLetterD>24</FourLetterD>"
	"<FivePointE>25</FivePointE>"
	"<SixPointF>26</SixPointF>"
	"<GGGGGGG>27</GGGGGGG>"
	"<Address>"
		"<Line1>222 Address 1 is here</Line1>"
		"<Line2>second line of address 1</Line2>"
		"<City>Cartersville</City>"
		"<State>Georgia</State>"
		"<Zip>30121</Zip>"
	"</Address>"
	"<WrapperTag>"
		"<Address>"
			"<Line1>222 Address 2 is here</Line1>"
			"<Line2>line 222</Line2>"
			"<City>Antioch</City>"
			"<State>California</State>"
			"<Zip>94509</Zip>"
		"</Address>"
	"</WrapperTag>"
	"<ListOfAddresses>"
		"<Address>"
			"<Line1>222 Address 3 is here</Line1>"
			"<Line2>line 2 of 333</Line2>"
			"<City>Northglenn</City>"
			"<State>Colorado</State>"
			"<Zip>80233</Zip>"
		"</Address>"
		"<Address>"
			"<Line1>222 Address 4 is here</Line1>"
			"<Line2>line 2 of 444</Line2>"
			"<City>Atlanta</City>"
			"<State>GA</State>"
			"<Zip>30333</Zip>"
		"</Address>"
	"</ListOfAddresses>"
	"<Wrapper>"
	     "<StringList>2one</StringList>"
	     "<StringList>2two</StringList>"
	     "<StringList>3.33333333333333</StringList>"
	     "<StringList>444</StringList>"
	     "<StringList>5 Point Part</StringList>"
	     "<StringList>6 Point Part</StringList>"
	     "<StringList>777 G Star</StringList>"
	     "<StringList>2eight</StringList>"
	     "<StringList>2nine</StringList>"
	     "<StringList>2ten</StringList>"
	"</Wrapper>"
"</Thing>"
"<Thing oid='3' Color='Purple'>"
	"<ID>3</ID>"						// it is not necessary to duplicate the key in the data
	"<String>Hello World 3.3333333</String>"  
	"<Number>1984v3</Number>"
	"<Bool>True</Bool>"
	"<FixedBuffer>Three</FixedBuffer>"
	"<OneLetterA>3</OneLetterA>"
	"<TwoLetterB>3</TwoLetterB>"
	"<ThreeLetterC>3</ThreeLetterC>"
	"<FourLetterD>3</FourLetterD>"
	"<FivePointE>3</FivePointE>"
	"<SixPointF>3</SixPointF>"
	"<GGGGGGG>3</GGGGGGG>"
	"<Address>"
		"<Line1>333 Address 1 is here</Line1>"
		"<Line2>second line of address 1</Line2>"
		"<City>Cartersville</City>"
		"<State>Georgia</State>"
		"<Zip>30121</Zip>"
	"</Address>"
	"<WrapperTag>"
		"<Address>"
			"<Line1>333 Address 2 is here</Line1>"
			"<Line2>line 222</Line2>"
			"<City>Antioch</City>"
			"<State>California</State>"
			"<Zip>94509</Zip>"
		"</Address>"
	"</WrapperTag>"
	"<ListOfAddresses>"
		"<Address>"
			"<Line1>333 Address 3 is here</Line1>"
			"<Line2>line 2 of 333</Line2>"
			"<City>Northglenn</City>"
			"<State>Colorado</State>"
			"<Zip>80233</Zip>"
		"</Address>"
		"<Address>"
			"<Line1>333 Address 4 is here</Line1>"
			"<Line2>line 2 of 444</Line2>"
			"<City>Atlanta</City>"
			"<State>GA</State>"
			"<Zip>30333</Zip>"
		"</Address>"
	"</ListOfAddresses>"
	"<Wrapper>"
	     "<StringList>3one</StringList>"
	     "<StringList>3two</StringList>"
	     "<StringList>33.33333333333333</StringList>"
	     "<StringList>3444</StringList>"
	     "<StringList>5 Point Part</StringList>"
	     "<StringList>6 Point Part</StringList>"
	     "<StringList>777 G Star</StringList>"
	     "<StringList>3eight</StringList>"
	     "<StringList>3nine</StringList>"
	     "<StringList>3ten</StringList>"
	"</Wrapper>"
"</Thing>"
"</AllMyObjects>";




int main(int argc, char* argv[])
{
	GString strDebug;
	MyCustomContainer Obs;
	
	///////////////////////////////////////////////////
	// add object 1 to the cache
	///////////////////////////////////////////////////
	Obs.FromXMLX(pzXML1);

//	strDebug << Obs.m_hashCache.GetCount() << " Object in cache.\n" << Obs.ToXML();
//	printf(strDebug);
//  output: 1 Object in cache followed by XML.


	///////////////////////////////////////////////////
	// add objects 2 and 3 to the cache
	///////////////////////////////////////////////////
	Obs.FromXMLX(pzXML2);

//	strDebug << Obs.m_hashCache.GetCount() << " Objects in cache.\n" << Obs.ToXML();
//	printf(strDebug);
//  output: 3 Objects in cache followed by 3 Times as much XML.


// now here is the proof:

	// update object ID 1
	GPerformanceTimer *pGTime = new GPerformanceTimer("Update Slow",0);
	Obs.FromXMLX(pzXML1UpdateSlow);
	delete pGTime;

//	strDebug << Obs.m_hashCache.GetCount() << " Objects in cache.\n" << Obs.ToXML();
//	printf(strDebug);
//	 output: 3 Objects in cache followed by XML.
// NOTE we do not have 4 objects, we have only 3 still
// NOTE we did not add a new object we updated an object


	GPerformanceTimer *pG2Time = new GPerformanceTimer("Update Fast",0);
	Obs.FromXMLX(pzXML1UpdateFast);
	delete pG2Time;

//	strDebug << Obs.m_hashCache.GetCount() << " Objects in cache.\n" << Obs.ToXML();
//	printf(strDebug);
//	 output: 3 Objects in cache followed by XML.


//The output contains the proof.


	printf("\n\n\n\t\tPRESS A KEY!");
#ifndef _WIN32
	getchar();
#else
	_getch();
#endif

	return 0;
}


