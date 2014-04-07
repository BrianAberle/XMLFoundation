#include <XMLFoundation.h>
#include "RelationshipWrapper.h"

//
// Suppose you have an application that currently outputs a data format.  
// This could be ANY data format, the data format for a complex word processing application
// might be RTF, and a data format for a transactional application might be ACH or EDI.
// The data format for GProfile was INI until March 2014 when the same (simple) object model
// was modified to include XML as a native input/output format. The "data format"  (which ever)
// is generated from Application layer objects which must me modeled in memory often referred to 
// as the object model, which is an instance of the data model.

// "Containment Design"  vs "Inherited Design"

//
// This is the object model: [name,value] pairs that exist in a list, each list has a name.
// This is INI, it is also a sub-data structure concept used in EDI, RTF, and many formats
// ***********************************************************************************
class INIEntry : public XMLObject
{
public:
	GString m_strName;
	GString m_strValue;
	virtual void MapXMLTagsToMembers()
	{
		MapAttribute(&m_strName,"name");
		MapAttribute(&m_strValue,"value");
	}
	DECLARE_FACTORY(INIEntry, setting)
};

class INISection : public XMLObject
{
public:
	GString		m_strName;
	GList		m_lstNVP;
	virtual void MapXMLTagsToMembers()
	{
		MapAttribute(&m_strName,"name");
		MapMember(&m_lstNVP, GProfileEntry::GetStaticTag());
	}
	DECLARE_FACTORY(INISection, section)
};
IMPLEMENT_FACTORY(INISection, section);
IMPLEMENT_FACTORY(INIEntry, setting);
// ***********************************************************************************


// Now suppose that [MyExistingObject] represents some class that you have already developed
// and you want to integrate a native XML format in addition to any pre-existing formats - 
// this is the path to systems integrations.  A, B, and C are 3 designs that acconplish this.
// choosing one over the other is largely a matter of preference.  I selected design "C"
// over "B" in GProfile ONLY because even when privately inherited from XMLObject my C++
// development IDE lists all the members/methods of XMLObject in GProfile, so for example
// when I type "GetProfile." the IDE shows a list of methods in GProfile and ONLY because
// I don't want to see XMLObject members/methods in my GProfile, I switched from "B" to "C"



///////////////   MyObject.h   ///////////////////////////////
class MyExistingObjectA  // this object will 'use' XMLObject there will be --NO-- changes to it's header file
{
public:							
	GString m_strString;
	GList m_lstSections;
	const char *WriteDataFormat(GString *pstrDest);
	void ReadDataFormat(const char *pzData);
};
///////////////   MyObject.cpp   ///////////////////////////////
// #include "RelationshipWrapper.h"
void MyExistingObjectA::ReadDataFormat(const char *pzData)
{
	// This creates temporary XMLObject, that essentially calls MapMember() on non-member variables
	// the call to AddReference() mapped the list in 'this' object
	XMLRelationshipWrapper objectContainer("configuration");
	objectContainer.AddReference(GProfileSection::GetStaticTag(), &m_lstSections, &gGListHandler,0,1);
	objectContainer.FromXMLX(pzData);
	// so now [objectContainer] is destroyed and [m_lstSections] was populated by FromXMLX()
}
const char *MyExistingObjectA::WriteDataFormat(GString *pstrDest)
{
	XMLRelationshipWrapper objectContainer("configuration");
	objectContainer.AddReference(GProfileSection::GetStaticTag(), &m_lstSections, &gGListHandler,0,1);
	objectContainer.ToXML(pstrDest);
	return *pstrDest;
}
////////////////////////////////////////////////////////////
///////////////   MyObject.h   ///////////////////////////////
class MyExistingObjectB : private XMLObject   // this object now 'is' an XMLObject 
{
public:
	GString m_strString;
	GList m_lstSections;
	const char *WriteDataFormat(GString *pstrDest);
	void ReadDataFormat(const char *pzData);
	MyExistingObjectB(){}
	~MyExistingObjectB(){};
	
	// Note: this needed to be added to MyExistingObject to use direct inheritance
	virtual void MapXMLTagsToMembers()
	{
		MapMember(&m_lstSections, GProfileSection::GetStaticTag());
	}
	DECLARE_FACTORY(MyExistingObjectB, MyObject) 
};
IMPLEMENT_FACTORY(MyExistingObjectB, MyObject)
///////////////   MyObject.cpp   ///////////////////////////////
void MyExistingObjectB::ReadDataFormat(const char *pzData)
{
	FromXMLX(pzData);  // it cant get much easier than this.
}
const char *MyExistingObjectB::WriteDataFormat(GString *pstrDest)
{
	ToXML(pstrDest);  // it cant get much easier than this.
	return *pstrDest;
}
////////////////////////////////////////////////////////////
class MyExistingObjectC
{
public:							
	XMLRelationshipWrapper m_objectContainer; // added to our existing .h file
	GString m_strString;
	GList m_lstSections;
	const char *WriteDataFormat(GString *pstrDest);
	void ReadDataFormat(const char *pzData);
	MyExistingObjectC() : m_objectContainer("configuration") 
	{
		m_objectContainer.AddReference(GProfileSection::GetStaticTag(), &m_lstSections, &gGListHandler,0,1);
	}
	~MyExistingObjectC(){};
};
////////////////////////////////////////////////////////////
void MyExistingObjectC::ReadDataFormat(const char *pzData)
{
	m_objectContainer.FromXMLX(pzData);  // it cant get much easier than this.
}
const char *MyExistingObjectC::WriteDataFormat(GString *pstrDest)
{
	m_objectContainer.ToXML(pstrDest);  // it cant get much easier than this.
	return *pstrDest;
}


//
// This is the XML we'll process.
//
char pzXML[] = 
"<configuration>"
	"<section name='Section A'>"
		"<setting name='Path' value='/XMLFoundation/Examples/'/>"
	"</section>"
	"<section name='System'>"
		"<setting name='Pool' value='20'/>"
		"<setting name='ProxyPool' value='0'/>"
	"</section>"
	"<section name='HTTP'>"
		"<setting name='Port' value='8088'/>"
		"<setting name='Enable' value='yes'/>"
		"<setting name='Home' value='/XMLFoundation'/>"
		"<setting name='Index' value='XMLFoundation2014.html'/>"
		"<setting name='EnableServerExtensions' value='yes'/>"
	"</section>"
"</configuration>";

// ******************************************************************
// ******************************************************************
// ******************************************************************
class CMatter : public XMLObject
{
public:
	GString m_strWeight;
	virtual void MapXMLTagsToMembers()
	{
		MapMember(&m_strWeight,	"Weight");
	}
	DECLARE_FACTORY(CMatter, Matter) 
	CMatter(){} 
	~CMatter(){};
};
IMPLEMENT_FACTORY(CMatter, Matter)
//------------------------------------------------
class CLife : public CMatter 
{
public:
	GString m_strDNA;
	virtual void MapXMLTagsToMembers()

	{
		MapMember(	&m_strDNA,	"DNA");
		CMatter::MapXMLTagsToMembers(); // explicit base class call
	}
	
	DECLARE_FACTORY(CLife, Life) 
		CLife(){   }
	~CLife(){};
};
IMPLEMENT_FACTORY(CLife, Life)
//------------------------------------------------
class CHuman : public CLife
{
public:
	GString m_strFingerPrint;
	GString m_strGender;

	virtual void MapXMLTagsToMembers()
	{
		MapMember(&m_strFingerPrint,"FingerPrint");
		MapMember(&m_strGender,"Gender");
		CLife::MapXMLTagsToMembers(); // explicit base class call
	}
	
	DECLARE_FACTORY(CHuman, Human) 

	CHuman(){} 
	~CHuman(){};
};
IMPLEMENT_FACTORY(CHuman, Human)
//------------------------------------------------
char pzXML3[] = 
"<Human>"
	"<Gender>Male</Gender>"
	"<DNA>1101010001010101101011000010101010</DNA>"
	"<FingerPrint>Unique</FingerPrint>"
	"<Weight>777</Weight>"
"</Human>";



void MappingAndInheritance()
{
	CHuman O;

	//"<Human>"
	//	"<Gender>Male</Gender>"
	//	"<DNA>1101010001010101101011000010101010</DNA>"
	//	"<FingerPrint>Unique</FingerPrint>"
	//	"<Weight>777</Weight>"
	//"</Human>";  
	O.FromXMLX(pzXML3);
	
	GString strDebug;
	strDebug << "\n\n\nGender:" << O.m_strGender << "      FingerPrint:" 
			 << O.m_strFingerPrint << "\n" << "DNA:" << O.m_strDNA 
			 << "      Weight:" << O.m_strWeight << "\n\n";
	printf(strDebug);
	//
	// Here you can see that although a CHuman only mapped 2 items it has all 4
	//
	// Gender:Male      FingerPrint:Unique
	// DNA:1101010001010101101011000010101010      Weight:777	
	//////////////////////


	// and likewise it writes out all 4 in XML although it only mapped 2
	printf(O.ToXML());
	//////////////////////
	//<Human>
	//        <FingerPrint>Unique</FingerPrint>
	//		  <Gender>Male</Gender>
	//        <DNA>1101010001010101101011000010101010</DNA>
	//        <Weight>777</Weight>
	//</Human>	
	//////////////////////

	
	
	//Notice that CLife is being created with the same xml that the CHuman was created with.
	CLife life;
	life.FromXML(pzXML3);// Gender and FingerPrint are now unmapped data
	strDebug.Empty();
	strDebug << "\n\nDNA:"	<< life.m_strDNA << "      " 
			 << "Weight:"	<< life.m_strWeight << "\n\n";
	printf(strDebug);
	//////////////////////
	// DNA:1101010001010101101011000010101010      Weight:777
	//////////////////////


	printf(life.ToXML());
	//////////////////////
	//<Life>
	//        <DNA>1101010001010101101011000010101010</DNA>
	//        <Weight>777</Weight>
	//</Life>
	//////////////////////


	// So- for example, you may create an object CPlant that like
	// the CHuman is derived from CLife.  A CPlant would contain
	// the elements of CLife (DNA) and of CMatter (Weight) by inheritance.

	// If each XML message represents a transaction it is wise
	// to map the commonalities of all transactions, or groups of
	// transactions into a base class that allows derivatives
	// to inherit the base elements of the transaction that
	// will only be maintained in one place.

}

// ******************************************************************
// ******************************************************************
// ******************************************************************

int main(int argc, char* argv[])
{
	// take your pick, Design A==B==C

	MyExistingObjectA A;
	MyExistingObjectB B;
	MyExistingObjectC C;
	A.ReadDataFormat(pzXML);
	B.ReadDataFormat(pzXML);
	C.ReadDataFormat(pzXML);

	GString a;
	GString b;
	GString c;
	printf( A.WriteDataFormat(&a) );
	printf( "\r\n ---------- \r\n" );
	printf( B.WriteDataFormat(&b) );
	printf( "\r\n ---------- \r\n" );
	printf( C.WriteDataFormat(&c) );


	// as we saw above, there is a time to contain(or inherit)
	// and as you can see below there is a time to inherit only inherit

	MappingAndInheritance();
	
	return 0;
}




