//---------------------------------------------------------------------------------------
// Beginning with XMLFoundation....
//
// This example is for Linux, Unix, and Windows.
//
// This is the smallest simplest working example
//----------------------------------------------------------------------------------------
 


// ------------ Makefile or Project settings --------------------
// make sure you have "/XMLFoundation/Libraries/XMLFoundation/inc" in the compile include path
// link to  "/XMLFoundation/Libraries/Debug/XmlFoundation.lib" (libXMLFoundation.a in Linux)
// In Windows verify the default C Runtime Libraries is "Multithreaded DLL" or "Debug Multithreaded DLL"
// Because that is the default build setting for XMLFoundation - just as it is for MFC apps.
#include <XMLFoundation.h>

#include <string.h> // for strcpy()



//////////////////////////////////////////////////////////////////
// Define a simple object
//////////////////////////////////////////////////////////////////
class MyCustomObject : public XMLObject
{
public:							// make public here for example simplicity - this is not required
	GString m_strString;		// A String Member
	GString m_strColor;			// An attribute , not an element
	int m_nInteger;				// An Integer Member
	char m_szNative[10];		// a fixed 10 byte buffer
	bool m_bBool;				// bool
	GStringList m_strList;      // A String List

	unsigned char m_bits;		// 8 bits  
//	unsigned __int64 m_bits;	// 64 bits = You can comment out the 8 bit line above and replace it with this one.  No other code change is necessary.

	virtual void MapXMLTagsToMembers()
	{
	//			Member variable		XML Element		
		MapMember( &m_strList,		"StringList", "Wrapper");
		MapMember(	&m_nInteger,	"Number");
		MapMember(	&m_strString,	"String");
		MapMember(	&m_bBool,		"Bool");
		MapMember( m_szNative,		"FixedBuffer", sizeof(m_szNative) );

		MapAttribute(&m_strColor,	"Color");

		MapMemberBit( &m_bits,		"Seven77thBit", 7, "False,No,Off,0",  "True,Yes,On,1");
		MapMemberBit( &m_bits,		"OnItsSide8",   8, "Black",			  "White");
		MapMemberBit( &m_bits,		"Fifty5",		55, "no",			  "yes"); // if 55 is out of range - it will not map - see error log.  Use the 64 bit m_bits, and it maps.
	}
	
	// 'this' type, followed by the XML Element name, normally DECLARE_FACTORY() is in an .h file
	DECLARE_FACTORY(MyCustomObject, Thing) 

	MyCustomObject(){m_bits=0;} // keep one constructor with no arguments
	~MyCustomObject(){};
};
// IMPLEMENT_FACTORY() must exist in a .CPP file - not an .h file - one for every DECLARE_FACTORY()
IMPLEMENT_FACTORY(MyCustomObject, Thing)


//
// This is the XML we'll process.
//
char pzXML[] = 
"<Thing Color='Black and White'>"
	"<String>Hello World!</String>"  
	"<Number>1984</Number>"
	"<Bool>True</Bool>"
	"<FixedBuffer>native</FixedBuffer>"
	"<Wrapper>"
	     "<StringList>one</StringList>"
	     "<StringList>two</StringList>"
	"</Wrapper>"
	"<Seven77thBit>on</Seven77thBit>"
	"<OnItsSide8>white</OnItsSide8>"
	"<Fifty5>yes</Fifty5>"
"</Thing>";



#ifdef _WIN32
	#define LONG_ONE 1i64
#else
	#define LONG_ONE 1LL
#endif


#include "GStringCipherZip.h"

int main(int argc, char* argv[])
{
	MyCustomObject O;
	O.FromXMLX(pzXML);

	// look at the Object "O".
	GString strDebug;
	strDebug << "After FromXMLX()=[" << O.m_strString << "][" << O.m_nInteger << "][" << O.m_strColor << "][" << O.m_szNative << "]\n\n\n";


	// print out the value of each individual bit in m_bits
	// Change "<OnItsSide8>" from "white" to "black" in pzXML and watch how the bits change
	for(int i=0;i<8;i++)
		strDebug.FormatAppend("bit%d=%d\n",i+1,((O.m_bits&(LONG_ONE<<i))!=0));  // :)
//		this is the output:
//			bit1=0
//			bit2=0
//			bit3=0
//			bit4=0
//			bit5=0
//			bit6=0
//			bit7=1
//			bit8=1

	// check the value of the 55th and 60th bits - (only if sizeof(m_bits) is 8 bytes which it is an __int64)
	if ( sizeof(O.m_bits) == 8 )
	{
		strDebug.FormatAppend("bit%d=%d\n",54+1,((O.m_bits&(LONG_ONE<<54))!=0)); // check the 55th bit
		strDebug.FormatAppend("bit%d=%d\n",59+1,((O.m_bits&(LONG_ONE<<59))!=0)); // check the 60th bit
	}
	

	// set some data 
	O.m_strString = "Root was here";
	// or any of this kind of data (notice how it automatically becomes escaped XML proper in the output)
	O.m_strString.Empty();
	for (int ch=32; ch < 256; ch++)
	{
		O.m_strString << (char)ch;
	}
	// attributes escape a little differently
	O.m_strColor = O.m_strString;


	O.m_nInteger = 21;

	strcpy(O.m_szNative,"so native");
	O.m_bBool = !O.m_bBool;

	// turn the 8th bit off - changes the value to "Black" in the XML - remove the following line and it stays "White" in the output XML
	O.m_bits &= ~(1<<7);

	// add any encoding tags or doctype you need - if you need them other wise skip the next two lines
//	GString strXMLStreamDestinationBuffer = "<?xml version=\"1.0\" standAlone='yes'?>\n";
	GString strXMLStreamDestinationBuffer = "<?xml version=\"1.0\" encoding='windows-1252'?>\r\n";
	strXMLStreamDestinationBuffer << "<!DOCTYPE totallyCustom SYSTEM \"http://www.IBM.com/example.dtd\">\r\n";
	O.ToXML( &strXMLStreamDestinationBuffer);
/*  
	// this is the xml in strXMLStreamDestinationBuffer 
	<?xml version="1.0" standAlone='yes'?>
	<!DOCTYPE totallyCustom SYSTEM "http://www.IBM.com/example.dtd">
	<Thing Color="Black and White">
			<Wrapper>
					<StringList>one</StringList>
					<StringList>two</StringList>
			</Wrapper>
			<Number>21</Number>
			<String>Root was here</String>
			<String2>Root</String2>
			<Bool>False</Bool>
			<FixedBuffer>so native</FixedBuffer>
			<Seven77thBit>True</Seven77thBit>
			<OnItsSide8>Black</OnItsSide8>
	</Thing>.
*/	
	strXMLStreamDestinationBuffer.ToFile("EncodingTest.txt");

	strDebug << strXMLStreamDestinationBuffer;
	printf(strDebug);

	return 0;
}


