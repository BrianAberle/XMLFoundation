#include <XMLFoundation.h>
#include <string.h> // for strcpy()

//////////////////////////////////////////////////////////////////
// Define a simple object
//////////////////////////////////////////////////////////////////
class MyCustomObject : public XMLObject
{
public:							// make public here for example simplicity - this is not required
	GString m_strString;		// A String Member - Untranslated
	GString m_strString2;		// A String Member - Translated
	GString m_strString3;		// A String Member - Translated
	GString m_strString4;		// A String Member - Translated
	GString m_strString5;		// A String Member - Translated
	GString m_strString6;		// A String Member - Translated
	GString m_strString7;		// A String Member - Translated
	GString m_strString8;		// A String Member - Translated
	GString m_strString9;		// A String Member - Translated
	GString m_strString10;		// A String Member - Translated
	GString m_strString11;		// A String Member - Translated
	GString m_strString12;		// A String Member - Translated
	char m_szNative[10];		// a fixed 10 byte buffer
	char m_szNative2[10];		// a fixed 10 byte buffer

	GString m_strColor;			// An attribute , not an element - they translate the same
	int m_nInteger;				// An Integer Member - Untranslated
	int m_nInteger2;			// An Integer Member - Translated
	int m_nInteger3;			// An Integer Member - Translated
	int m_nInteger4;			// An Integer Member - Translated
	int m_nInteger5;			// An Integer Member - Translated
	int m_nInteger6;			// An Integer Member - Translated
	int m_nInteger7;			// An Integer Member - Translated
	int m_nInteger8;			// An Integer Member - Translated

	GStringList m_strList;      // A String List
	GArray		m_strArray;     // An Integer Array



	// This comment was copied from XMLObject.h explaining the 3rd, 4th, and 5th arguments to MapMember()
	///////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Using Translation Maps (new in November 2013)
	//////////////////////////////////////////////////////////////////-/////////////////////////////////////////
	// pzTranslationMapIn translates values - for example this is how you might use it on a String:
	// if pzTranslationMapIn = "red*=Red,Mary Jane=Green,Grass=Green,XML_EMPTY_VALUE=Blue,XML_DEFAULT=Unknown
	//                          ---------------------------------------------------------
	// if <pTag> in the source xml starts with red - like "redness" or "reddish" it assigns member pValue = "Red"
	// if <pTag> in the source xml is "Mary Jane" or "Grass" - it assigns member pValue = "Green"
	// if <pTag></pTag> is empty in source xml - it assigns member pValue = "Blue"
	// <Tag></Tag> will assign a mapped m_member = "", unless XML_EMPTY_VALUE in pzTranslationMapIn overrides default behavior
	//    which will set m_strString2 == "Blue" due to XML_EMPTY_VALUE=Blue in pzTranslationMapIn.
	//
	// pzTranslationMapOut translates values in the XML to differ from what the member variable stores - for example:
	// if pzTranslationMapOut = "1=One,2=Two,777=Root,any*=anyTHING"
	// if pValue = "1" it will be seen as "One" in the XML.
	// if pValue = "2" it will be seen as "Two" in the XML.
	// if pValue = "777" it will be seen as "Root" in the XML.
	// if pValue starts with "any" (like "anyone" or "Anytime" (all wildcard searches are case insensitive)) it comes out as "anyTHING" in the XML.
	//
	// if XML_DEFAULT is specified and there is no known translation for the value, this value will be used.
	// if XML_DEFAULT is not specified and there is no known translation for the value, the untranslated value will be used
	// if XML_DEFAULT= is followed by a comma or null like this: "A=1,B=2,XML_DEFAULT=,C=3" or like this "A=1,B=2,C=3,XML_DEFAULT="
	//    then the value with no known translation will NOT be used, and the value will be "" aka empty.
	//
	//
	// .....and this is how you might use it on an Integer:
	// pzTranslationMapIn  = "1=10,2=20,3=777,one=10,two=20,three=777,XML_EMPTY_VALUE=-777,XML_DEFAULT=0"
	// pzTranslationMapOut = "10=ten,20=twenty,777=infinity,XML_DEFAULT=0"
	//
	//	
	// and the last argument, nTranslationFlags can be any combinations of these values:
	// XLATE_CASE_INSENSITIVE_IN		// When assigning variables from XML
	// XLATE_CASE_INSENSITIVE_OUT		// When writing XML translations
	// XLATE_NO_IN_WILDCARDS			// you never NEED to set this, as it is calculated and cached.

	///////////////////////////////////////////////////////////////////////////////////////////////////////////




	virtual void MapXMLTagsToMembers()
	{
	//	                                          This argument only effects how members are assigned								and this effects how the XML is created.
	//			Member variable		XML Element	--------------------------------------------------------							 ------------------------------------------
		MapMember( &m_strString,	"String");
		MapMember( &m_strString2,	"String2", "red*=Red,Mary Jane=Green,grass=Green",												"1=One,2=Two,777=Root,any*=anyVALUE");
		MapMember( &m_strString3,	"String3", "red*=Red,Mary Jane=Green,grass=Green",												"1=One,2=Two,777=Root,any*=anyVALUE");
		MapMember( &m_strString4,	"String4", "red*=Red,Mary Jane=Green,grass=Green",												"1=One,2=Two,777=Root,any*=anyVALUE",XLATE_CASE_INSENSITIVE_IN);
		MapMember( &m_strString5,	"String5", "red*=Red,Mary Jane=Green,grass=Green",												"1=One,2=Two,777=Root,any*=anyVALUE");
		MapMember( &m_strString6,	"String6", "red*=Red,Mary Jane=Green,grass=Green",												"1=One,2=Two,777=Root,any*=anyVALUE");
		MapMember( &m_strString7,	"String7", "red*=Red,Mary Jane=Green,grass=Green",												"XMLFoundation=best,any*=anyVALUE",XLATE_CASE_INSENSITIVE_OUT | XLATE_CASE_INSENSITIVE_IN);
		MapMember( &m_strString8,	"String8", "red*=Red,Mary Jane=Green,grass=Green,XML_EMPTY_VALUE=Blue",							"XMLFoundation=best,any*=anyVALUE");
		MapMember( &m_strString9,	"String9", "red*=Red,Mary Jane=Green,grass=Green,XML_EMPTY_VALUE=Blue",							"1=One,2=Two,777=Root,any*=anyVALUE,XML_DEFAULT=xxxxxx");
		MapMember( &m_strString10,	"String10","red*=Red,Mary Jane=Green,grass=Green,					 ",							"1=One,2=Two,777=Root,any*=anyVALUE,XML_EMPTY_VALUE=zzzzzzz");
		MapMember( &m_strString11,	"String11","red*=Red,Mary Jane=Green,grass=Green,XML_DEFAULT=We Know ",							"1=One,2=Two,777=Root,any*=anyVALUE");
		MapMember( &m_strString12,	"String12","red*=Red,Mary Jane=Green,grass=Green,XML_DEFAULT=We Know,XML_EMPTY_VALUE=Blue",		"1=One,2=Two,777=Root,any*=anyVALUE");

		// this one MapMember handles all string in the string list - unlike the individual per string maps above
		MapMember( &m_strList,		"StringList",	"Wrapper",  "red*=Red,Mary Jane=Green,grass=Green,XML_EMPTY_VALUE=Blue",						"Green=G.G.G.Green,Red=Bloody Red,777=Root,pur*=Purp");

		MapMember( m_szNative, "FixedBuffer",sizeof(m_szNative), "red*=Red,Mary Jane=Green,grass=Green"								"1=One,2=Two,777=Root,any*=anyVALUE");
		MapMember( m_szNative2,"FixedBuffer2",sizeof(m_szNative2),"red*=01234567899999,Mary Jane=Green,grass=Green"								"1=One,2=Two,777=Root,any*=anyVALUE");


		MapMember( &m_nInteger,		"Number");
		MapMember( &m_nInteger2,	"Number2", "1=10,2=20,3=777,one=10,two=20,three=777,XML_EMPTY_VALUE=-777,XML_DEFAULT=1234567",	"10=100,20=200,777=21,XML_DEFAULT=");
		MapMember( &m_nInteger3,	"Number3", "1=10,2=20,3=777,one=10,two=20,three=777,XML_EMPTY_VALUE=-777,XML_DEFAULT=1234567",	"10=100,20=200,777=21");
		MapMember( &m_nInteger4,	"Number4", "1=10,2=20,3=777,one=10,two=20,three=777,XML_EMPTY_VALUE=-777,XML_DEFAULT=1234567",	"10=100,20=200,777=21");
		MapMember( &m_nInteger5,	"Number5", "1=10,2=20,3=777,one=10,two=20,three=777,XML_EMPTY_VALUE=-777,XML_DEFAULT=1234567",	"10*=100,20=200,777=21");
		MapMember( &m_nInteger6,	"Number6", "1=10,2=20,3=777,one=10,two=20,three=777,XML_EMPTY_VALUE=-777,XML_DEFAULT=1234567",	"10=100,20=200,777=21");
		MapMember( &m_nInteger7,	"Number7", "1=10,2=20,3=777,one=10,two=20,three=777,XML_EMPTY_VALUE=-777,XML_DEFAULT=1234567",	"10=100,20=200,777=21");
		MapMember( &m_nInteger8,	"Number8", "1=10,2=20,3=777,one=10,two=20,three=777,					 XML_DEFAULT=1234567",	"10=100,20=200,777=21");


		MapMember( &m_strArray,		"IntArray", "Wrapper2", "1=10,2=20,3=777,one=10,two=20,three=777,XML_EMPTY_VALUE=-777,XML_DEFAULT=7777777",	"10=100,20=200,777=Three Sevens > Numbers");
		MapAttribute(&m_strColor,	"Color");

	}
	
	// 'this' type, followed by the XML Element name, normally DECLARE_FACTORY() is in an .h file
	DECLARE_FACTORY(MyCustomObject, Thing) 

	MyCustomObject(){m_nInteger6 = 21; m_strString9="MyNullValue";} // keep one constructor with no arguments
	~MyCustomObject(){};
};
// IMPLEMENT_FACTORY() must exist in a .CPP file - not an .h file - one for every DECLARE_FACTORY()
IMPLEMENT_FACTORY(MyCustomObject, Thing)




//
// This is the XML we'll process.
//
char pzXML[] = 
"<Thing Color='Black and White'>"
	
	"<FixedBuffer>Red White and Blue</FixedBuffer>"		// sets m_szNative == "Red" - the value from the XML will not fit in the member variable
	"<FixedBuffer2>Red White and Blue</FixedBuffer2>"   // sets m_szNative == "012345678" - notice the value is truncated from the translation value

	"<Number>1984</Number>" // untranslated

	"<Number2>10777</Number2>"			// sets m_nInteger2 == 1234567 due to XML_DEFAULT=1234567" - compare to Number8
	"<Number3>2</Number3>"				// sets m_nInteger3 == 20
	"<Number4>three</Number4>"			// sets m_nInteger4 == 777
	"<Number5>garbage</Number5>"		// sets m_nInteger5 == 1234567 due to XML_DEFAULT=1234567"

	// empty vs null
//	"<Number6></Number6>"				// leaves m_nInteger6 == 21 due to its constructor initialized default == 21
	"<Number7></Number7>"				// sets m_nInteger7 == -777 due to XML_EMPTY_VALUE=-777
	"<Number8>10777</Number8>"			// sets m_nInteger8 == 10777 since XML_DEFAULT= is not present

	"<String>Untranslated</String>"     // untranslated
	"<String2>redness</String2>"		// sets m_strString2 == "Red" from the red* wildcard match
	"<String3>reddish</String3>"		// sets m_strString3 == "Red" from the red* wildcard match
	"<String4>Mary jane</String4>"		// sets m_strString4 == "Green"		(with XLATE_CASE_INSENSITIVE_IN in the last argument to MapMember of m_strString4 )
	"<String5>Mary jane</String5>"		// sets m_strString5 == "Mary jane" (without XLATE_CASE_INSENSITIVE_IN in the last argument to MapMember of m_strString2 )
	"<String6>Mary Jane</String6>"		// sets m_strString6 == "Green"     - notice the uppercase 'J'
	"<String7>grAss</String7>"			// sets m_strString7 == "Green"

	// empty vs null
	"<String8></String8>"				// sets m_strString8 == "Blue" due to XML_EMPTY_VALUE=Blue
//	"<String9></String9>"				// leaves m_strString9 to its constructor initialized default == "MyNullValue"
	"<String10></String10>"				// sets m_strString10 == "" because XML_EMPTY_VALUE= is not present
	"<String11>unknown value</String11>"// sets m_strString11 == "We know " due to XML_DEFAULT=We know
	"<String12></String12>"				// sets m_strString12 == "Blue" because XML_EMPTY_VALUE has precedence over XML_DEFAULT

	"<Wrapper>"								// VALUE IN MEMBER			VALUE IN OUTPUT XML
	     "<StringList>redness</StringList>"	// Red						Bloody Red
	     "<StringList>reddish</StringList>" // Red						Bloody Red
		 "<StringList>Grass</StringList>"	// Grass					Grass
		 "<StringList>grass</StringList>"	// Green					G.G.G.Green
	     "<StringList>purple</StringList>"  // purple					Purp	
	     "<StringList></StringList>"		// Blue						Blue
	"</Wrapper>"
	"<Wrapper2>"							// value in variable		value in output XML
	     "<IntArray>3</IntArray>"			// 777						Three Sevens &#62; Numbers
	     "<IntArray>three</IntArray>"		// 777						Three Sevens &#62; Numbers
		 "<IntArray></IntArray>"			// -777						-777
		 "<IntArray>garbage</IntArray>"		// 7777777					7777777
	"</Wrapper2>"
"</Thing>";




int main(int argc, char* argv[])
{
	MyCustomObject O;
	O.FromXMLX(pzXML);

	// look at the Object "O".  Look at the cool debugging built into any object derived from XMLObject.
	// we cant use ToXML() to quickly print out all the member values - as they will be translated during XML creation
	// Dump() prints out the current member values in memory.
	O.Dump("DebugDumpFile.txt");
	printf (O.Dump());
	
	// look at the output file, and the comments in pzXML[] at the same time.


	// now we will assign some members.
	O.m_strString = "Still untranslated";
	O.m_strString2 = "1";				// this will translate to "One" the XML
	O.m_strString3 = "2";				// this will translate to "Two" the XML
	O.m_strString4 = "777";				// this will translate to "Root" the XML
	O.m_strString5 = "anybody";			// this will translate to "anyVALUE" the XML
	O.m_strString6 = "anything";		// this will translate to "anyVALUE" the XML
	O.m_strString7 = "XMLFoundation";	// this will translate to "best" the XML
	O.m_strString8 = "xmlFoundation";   // this will translate to "xmlFoundation" the XML
	O.m_strString9 = "";  //default		// this will translate to "xxxxxx" the XML
	O.m_strString10 = "";  // empty		// outputs an empty string in the XML: <String10/>
	O.m_strString11 = "Unchanged";
	O.m_strString12 = "";

	O.m_nInteger = 21;					// untranslated
	O.m_nInteger2 = 34096765;			// since XML_DEFAULT=, and there is no translation from 34096765 it come out as nothing in the XML
	O.m_nInteger3 = 10; // translates to 100
	O.m_nInteger4 = 777; // translates to 21
	O.m_nInteger5 = 10777;	// translates to 100 because 10*=100
	O.m_nInteger6 = 10777;	// 10777
	O.m_nInteger7 = 0; 
	O.m_nInteger8 = 0; 

	strcpy(O.m_szNative,"so native");


	O.ToXMLFile("DebugXMLOutput.txt");
	printf( O.ToXML() );


	return 0;
}


