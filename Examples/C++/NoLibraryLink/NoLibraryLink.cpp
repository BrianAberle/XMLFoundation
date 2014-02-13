// Unlike all the other project examples, this one does not link to the compiled library.  
// It includes the source code from the XMLFoundation into this project.
// This is how this project differs than the others in configuration:

// added all the files under XMlFoundation/src
// EXCPEPT
//	WinCERuntimeC.cpp 
//	src/Utils/EnglishErrors.cpp 
//	WinINetHTTPDataSource.cpp (unless you plan to use it and link wininet also)

// added ../../../libraries/XMLFoundation/inc/Win32 to the project include paths
// link to wsock32.lib
// turned off precompiled headers because VC++ will otherwise want #include "stdafx.h" in every source file from XMLFoundation source that you put into your project.


//
//
// The remainder of this file is identical to the StartHere0 Example
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
// In Windows verify the default C Runtime Libraries is "Multithreaded DLL" or "Debug Mulithreaded DLL"
// Because that is the default build setting for XMLFoundation - just as it is for MFC apps.
#include <XMLFoundation.h>




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
	GStringList m_strList;      // A String List

	virtual void MapXMLTagsToMembers()
	{
	//			Member variable		XML Element		
		MapMember( &m_strList,		"StringList", "Wrapper");
		MapMember(	&m_nInteger,	"Number");
		MapMember(	&m_strString,	"String");
		MapMember( m_szNative,		"FixedBuffer", sizeof(m_szNative) );
		MapAttribute(&m_strColor,	"Color");
	}
	
	// 'this' type, followed by the XML Element name, normally DECLARE_FACTORY() is in an .h file
	DECLARE_FACTORY(MyCustomObject, Thing) 

	MyCustomObject(){} // keep one constructor with no arguments
	~MyCustomObject(){};
};
// IMPLEMENT_FACTORY() must exist in a .CPP file - not an .h file - one for every DECLARE_FACTORY()
IMPLEMENT_FACTORY(MyCustomObject, Thing)




//
// This is the XML we'll process.
//
char pzXML[] = 
"<Thing Color='Red'>"
	"<String>Owners Word</String>"
	"<Number>777</Number>"
	"<FixedBuffer>native</FixedBuffer>"
	"<Wrapper>"
	     "<StringList>one</StringList>"
	     "<StringList>two</StringList>"
	"</Wrapper>"
"</Thing>";



int main(int argc, char* argv[])
{
	MyCustomObject O;
	O.FromXMLX(pzXML);

	// look at the Object "O".
	GString strDebug;
	strDebug << "Yo! Check out O:" << O.m_strString << "[" << O.m_nInteger << "]:" << O.m_szNative << "\n\n\n";
	printf(strDebug);

	// set some data 
	O.m_strString = "Root was here";
	
	// add any encoding tags or doctype you need - if you need them other wise skip the next two lines
	GString strXMLStreamDestinationBuffer = "<?xml version=\"1.0\" standAlone='yes'?>\n";
	strXMLStreamDestinationBuffer << "<!DOCTYPE totallyCustom SYSTEM \"http://www.IBM.com/example.dtd\">";

	O.ToXML( &strXMLStreamDestinationBuffer);

	printf(strXMLStreamDestinationBuffer);
/*
	<?xml version="1.0" standAlone='yes'?>
	<!DOCTYPE totallyCustom SYSTEM "http://www.IBM.com/example.dtd">
	<Thing Color="Red">
			<Wrapper>
					<StringList>one</StringList>
					<StringList>two</StringList>
			</Wrapper>
			<Number>777</Number>
			<String>Root was here</String>
			<FixedBuffer>native</FixedBuffer>
	</Thing>
*/	



	return 0;
}


