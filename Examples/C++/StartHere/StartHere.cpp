//---------------------------------------------------------------------------------------
// Beginning with XMLFoundation....
//
// This example is for Linux, Unix, and Windows.
//
// Anyone who honestly calls themselves a C++ programmer, will have the ability to apply 
// XMLFoundation functionality to their own application if you follow these directions:
//
// 1. read this source file top to bottom before you begin with step 2.
// 2. set a break point in main() - step through it one line at a time and watch the output.
//
//----------------------------------------------------------------------------------------
 



// If you want to put this example in an MFC app or COM Object
// replace this line:
//		int main(int argc, char* argv[])
// with
//		void MyExample()
//
// and replace every occurrence of "printf" with "AfxMessageBox"
// then this code will compile just fine in your application.


// ------------ Makefile or Project settings --------------------
// make sure you have "/XMLFoundation/Libraries/XMLFoundation/inc" in the compile include path
// link to  "/XMLFoundation/Libraries/Debug/XmlFoundation.lib" (libXMLFoundation.a in Linux)
// In Windows verify the default C Runtime Libraries is "Multithreaded DLL" or "Debug Multithreaded DLL"
// Because that is the default build setting for XMLFoundation - just as it is for MFC apps.
#include <XMLFoundation.h>



//////////////////////////////////////////////////////////////////
// First define 2 classes derived from XMLObject and map the XML they contain
//////////////////////////////////////////////////////////////////
class MyCustomObject : public XMLObject
{
public:							// All public example simplicity - not required
	GString m_strString;		// A String Member
	GString m_strColor;			// An attribute , not an element
	int m_nInteger;				// An Integer Member

	virtual void MapXMLTagsToMembers()
	{
		//		 Member variable		XML Element		
		MapMember(	&m_nInteger,		"Integer");
		MapMember(	&m_strString,		"String");
		MapAttribute(&m_strColor,		"Color");
	}
	
	// 'this' type, followed by the XML Element name, normally DECLARE_FACTORY() is in an .h file
	DECLARE_FACTORY(MyCustomObject, Thing) 

	MyCustomObject(){} // keep one constructor with no arguments
	~MyCustomObject(){};
};

// IMPLEMENT_FACTORY() must exist in a .CPP file - not an .h file - one for every DECLARE_FACTORY()
IMPLEMENT_FACTORY(MyCustomObject, Thing)
//////////////////////////////////////////////////////////////////
// define the second object....  same as the first with different Element names  (String is String2)
// and the tag to match this object is "Thing2"  ("Thing" maps to MyCustomObject)
// this object type also has no color attribute
//////////////////////////////////////////////////////////////////
class AnotherCustomObject : public XMLObject
{
public:	
	GString m_strString;		// A String Member
	int m_nInteger;				// An Integer Member
	
	// MapXMLTagsToMembers() is most often not inlined like it was in MyCustomObject
	virtual void MapXMLTagsToMembers();

	DECLARE_FACTORY(AnotherCustomObject, Thing2)
	AnotherCustomObject(){}
	~AnotherCustomObject(){};
};
IMPLEMENT_FACTORY(AnotherCustomObject, Thing2)
void AnotherCustomObject::MapXMLTagsToMembers()
{
	MapMember(&m_strString,		"String2");
	MapMember(&m_nInteger,		"Integer2");
}
//////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////


//
// This will be the owning object that contains data as well as other objects
class MyCustomObjectContainer : public XMLObject
{
public:
	// two simple types
	GString m_strWord;
	int m_nNumber;
	
	// This list will hold instances of type MyCustomObject
	GList m_lstMyObjects;
	// Also holds MyCustomObject instances - mapped differently to the XML
	GList m_lstMyObjectsWrapped;

	
	// This is 'containment' of another XMLObject.
	AnotherCustomObject m_O;

	// Same as above, but we will map this one differently to the XML
	AnotherCustomObject m_O2;

	// A MyCustomObjectContainer has 0 or 1 AnotherCustomObject's 
	// If AnotherCustomObject exists in the XML the object will 
	// be allocated, otherwise it will not be. 
	ObjectPtr <AnotherCustomObject> m_ptrExample;

	virtual void MapXMLTagsToMembers()
	{
		MapMember(&m_strWord,		"Word");
		MapMember(&m_nNumber,		"Number");

		// All of the <Thing>'s under <Owner> goes into this list
		MapMember(&m_lstMyObjects,			 MyCustomObject::GetStaticTag());
		// All of the <Thing>'s under <ContainerName> goes into this list, search this file for "ContainerName" to see the xml it maps to
		MapMember(&m_lstMyObjectsWrapped,	 MyCustomObject::GetStaticTag(),		"ContainerName");
		
		// This shows 3 ways to map an object... 
		//
		// map it directly, search this file for "Straight Containment" to see the XML this maps to
		MapMember(&m_O);
		// map it under a 'container tag name' or 'wrapper tag', search for "WrapperTag"
		MapMember(&m_ptrExample,	AnotherCustomObject::GetStaticTag(), "WrapperTag");
		// redefine the outer object tag, sometimes an "Address1" and and "Address2" map to an Address Object
		// here a "Thing3" maps to AnotherCustomObject, normally mapped to "Thing2"
		MapMember(&m_O2,			"Thing3");
	}
public:
	DECLARE_FACTORY(MyCustomObjectContainer, Owner)
	MyCustomObjectContainer(){}
	~MyCustomObjectContainer(){};
};
IMPLEMENT_FACTORY(MyCustomObjectContainer, Owner)



//
// This is the XML we'll process.  Look at it close.  
//
// the first half is an <Owner> with an int(<Number>), String(<Word>), 
//					    <Thing> objects, and <Thing>'s in a <ContainerName> list.
//
// the second half is 3 <Thing2>'s, one inside a <WrapperTag>, and one that goes by the name <Thing3>
//
char pzXML[] = 
"<!DOCTYPE Owner>"
"<Owner>"
	"<Word>Owners Word</Word>"
	"<Number>777</Number>"
	"<Thing Color='Red'>"
		"<String>One</String>"
		"<Integer>1</Integer>"
	"</Thing>"
	"<Thing Color='Blue'>"
		"<String>Two</String>"
		"<Integer>2</Integer>"
	"</Thing>"
	"<ContainerName>"
		"<Thing Color='Green'>"
			"<String>Three</String>"
			"<Integer>3</Integer>"
		"</Thing>"
		"<Thing Color='Purple'>"
			"<String>Four</String>"
			"<Integer>4</Integer>"
			"<String2>Find This Error</String2>"
		"</Thing>"
	"</ContainerName>"
 
	/*Start our <Thing2>'s*/
	"<Thing2>"
		"<String2>Straight Containment</String2>"
		"<Integer2>5</Integer2>"
	"</Thing2>"
	"<WrapperTag>"
		"<Thing2>"
			"<String2>Point at this</String2>"
			"<Integer2>5</Integer2>"
		"</Thing2>"
	"</WrapperTag>"
	"<Thing3>"
		"<String2>Map to Thing3</String2>"
		"<Integer2>6</Integer2>"
	"</Thing3>"

	/*For debugging,  above*/
	"<NotMapped>There is nowhere to put this data!</NotMapped>"
"</Owner>";

/////////////////////////////////////////////////////////////////
// we will use this XML to UPDATE objects
char pzXML2[] = 
"<!DOCTYPE Owner>"
"<Owner>"
	"<Word>Updated</Word>"
	"<Number>0707070</Number>"
	"<Thing>"
		"<String>Another NEW list Entry!</String>"
		"<Integer>21</Integer>"
	"</Thing>"
	"<ContainerName>"
		"<Thing>"
			"<String>Another In this list too</String>"
			"<Integer>21</Integer>"
		"</Thing>"
	"</ContainerName>"
	"<Thing2>"
		"<String2>UPDATED This Contained Sub-Object</String2>"
	"</Thing2>"
"</Owner>";
//////////////////////////////////////////////////////////////////


int main(int argc, char* argv[])
{

#ifdef _DEBUG
	// This is an optional debugging tool.  It helps a lot.  Look at it. See why "NotMapped" is in the log file.
	// If any tags in the XML do not match up with the object mapping the warning will be in this debug file.
	// you need to open it with WordPad or VC++ in Windows - don't use notepad because it uses UNIX style line feeds.
	SetLogFile("MyAppDebugLog.txt");
#endif


	MyCustomObjectContainer O;
	try
	{
		// Parse the xml
		// Instantiate 5 new objects
		// Assign 20 member variables
		// Puts each object in the correct list or container
		O.FromXML(pzXML); // Bam!							
		// It is finished.
		

		//////////////////////////////////////////////////////////////////////////
		// now, print out what just happened.
		//
		// look at the main Object, "O" we call it, 
		GString strDebug;
		strDebug << "Yo! Check out O:" << O.m_strWord << "[" << O.m_nNumber << "]\n";
		printf(strDebug);
		strDebug.Empty();
		strDebug<< "Look! m_lstMyObjects has [" << O.m_lstMyObjects.Size() << "] Objects.\n";
		printf(strDebug);
		strDebug.Empty();
		strDebug << "AND m_lstMyObjectsWrapped has [" << O.m_lstMyObjectsWrapped.Size() << "]\n";
		printf(strDebug);

		// look at the first of the two object in list [O.m_lstMyObjects]
		MyCustomObject *pObj = (MyCustomObject *)O.m_lstMyObjects.First();
		strDebug.Empty();
		strDebug << "A factory created object:" << pObj->m_strString << "[" << pObj->m_nInteger << "][" << pObj->m_strColor << "]\n";
		printf(strDebug);

		// look at the first of the two object in list [O.m_lstMyObjectsWrapped] -----> AND CHANGE IT
		pObj = (MyCustomObject *)O.m_lstMyObjectsWrapped.First();
		strDebug.Empty();
		strDebug << "Another factory created object:" << pObj->m_strString << "[" << pObj->m_nInteger << "][" << pObj->m_strColor << "]\n";
		printf(strDebug);
		pObj->m_strString = "A NEW VALUE!!!!";  // <---- Change the value here.


		// look at the object [m_O] contained in the main object, O.
		strDebug.Empty();
		strDebug << "Contained object m_O is:[" << O.m_O.m_strString << "][" << O.m_O.m_nInteger << "]\n";
		printf(strDebug);
		printf("and m_O's XML is:\n");
		printf(O.m_O.ToXML());  // Generate some XML just for fun.
		
		// look at the object [m_O2] contained in the main object
		strDebug.Empty();
		strDebug << "\nContained object m_O2[" << O.m_O2.m_strString << "][" << O.m_O2.m_nInteger << "]\n";
		printf(strDebug);

		// pointers allow you to distinguish between an "unassigned" object and a "null" object.
		AnotherCustomObject *pO = O.m_ptrExample;
		if (pO)
		{
			strDebug.Empty();
			strDebug << "Object Pointer pO[" << pO->m_strString << "][" << pO->m_nInteger << "]\n";
			printf(strDebug);
		}


		//////////////////////////////////////////////////////////////////////////
		// now, make changes we will see when we generate XML
		O.m_strWord = "Words of Art, Words of War.";
		
		// create a new object and put it in the [m_lstMyObjects] list
		MyCustomObject *pObjNew = new MyCustomObject;
		pObjNew->m_strString = "This IS something NEW.";
		pObjNew->m_nInteger = 7;
		pObjNew->m_strColor = "Perl White";
		pObjNew->AddAttribute("TotallyNew","Poetic Code");
		O.m_lstMyObjects.AddLast(pObjNew);

		// Make the new XML
		O.ToXMLFile("MyXMLOutput.txt");
		printf(O.ToXML());

		// now load [MyXMLOutput.txt] in an editor and compare it to what we started with
		//                            bound in the global [pzXML] string.
		// notice "A NEW VALUE!!!!" is a changed value in an existing object
		// notice "Words of Art, Words of War." in the root object.  go figure.
		// notice the new object we added : "This IS something NEW." 


		// look close at pzXML2 defined above.
		// we will change the value of 'this' and the objects it contains through an "XML Update"
		// get ready for what is about to happen.
		// we will create 1 new object, put it in the list
		// and update 7 member variables based on pzXML2
		O.FromXML(pzXML2); // Bam.
		// It is finished.

		// Lets see what happened

		// re-generate XML with the changes we just made and write it in the named output file.
		O.ToXMLFile("MyXMLOutput2.txt");
		// load [MyXMLOutput2.txt] in an editor and compare it to [MyXMLOutput.txt] to see that
		// FromXML(pzMLX2) did all that the comments above it boasted.
		//
		// to see more advanced features in "XML Updates" see the example program titled "ObjectCache"
		// 
		// to see more advanced data structure mapping see the example program titled "IndexObjects"

		// and check out a full object dump, it's like having a runtime debugger.
		O.Dump("MyObjectDump.txt");

		printf("\nRead the Source code then Compare the output files......\n");


	}
	catch( GException &rErr )
	{
		// you probably have invalid XML if you get here...
		printf( rErr.GetDescription() );
	}
	return 0;
}


