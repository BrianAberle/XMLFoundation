// --------------------------------------------------------------------------
//						United Business Technologies
//			  Copyright (c) 2000 - 2012  All Rights Reserved.
//
// Source in this file is released to the public under the following license:
// --------------------------------------------------------------------------
// This toolkit may be used free of charge for any purpose including corporate
// and academic use.  For profit, and Non-Profit uses are permitted.
//
// This source code and any work derived from this source code must retain 
// this copyright at the top of each source file.
// --------------------------------------------------------------------------
#pragma warning (disable:4786)
#include <list>
#include <string>
#include <iostream>
#include "xmlObject.h"

#include "AbstractionsSTD.h"
using namespace std ;




class MyLocation : public XMLObject
{
public:
	string m_strDescription;
	virtual void MapXMLTagsToMembers();
public:
	DECLARE_FACTORY(MyLocation, Location);
	
	MyLocation(){ };
	~MyLocation(){ };
};

// three global STL abstraction instances used by MapMember()
STDStringAbstract stdS;
STDListAbstraction<MyLocation *> stdL;// note typed lists must have one instance of a STDListAbstraction for each type mapped.
STDStringListAbstraction stdSL;




IMPLEMENT_FACTORY(MyLocation,		Location)
void MyLocation::MapXMLTagsToMembers()
{
	MapMember(&m_strDescription,	"Label",	&stdS);
}
// -----------------------------------------------------------------------------------
class MyCustomer : public XMLObject
{
public: // normally members would be private, but for this example public simplifies cout'ing them to debug
	string m_strContactName;
	string m_strCity;
	string m_strCountry;
	list<MyLocation *> m_lstPlaces;
	list<string> m_lstColors; 

	virtual void MapXMLTagsToMembers();
public:
	// Object --> XML Element map
	DECLARE_FACTORY(MyCustomer, Customer);
	
	MyCustomer(){ };
	~MyCustomer(){ };
};

IMPLEMENT_FACTORY(MyCustomer,		Customer)

void MyCustomer::MapXMLTagsToMembers()
{
	MapMember(&m_strContactName,	"Name",							&stdS);								// Abstraction naming
	MapMember(&m_strCity,			"City",							&stdS);								//--------------
	MapMember(&m_strCountry,		"Country",						&stdS);								// S =  String
	MapMember(&m_lstPlaces,			MyLocation::GetStaticTag(),		&stdL,    "STLObjectList" );        // L  = List
	MapMember(&m_lstColors,			"Color",						&stdSL,   "ColorList" );            // SL = StringList
}


char pCustomerUpdate[] = 
"<Customer oid='1'>"
	"<Name>Old School Super User</Name>"
	"<City>Antioch</City>"
	"<Country>California</Country>"
	"<STLObjectList>"
		"<Location>"
			"<Label>one</Label>"
		"</Location>"
		"<Location>"
			"<Label>two</Label>"
		"</Location>"
	"</STLObjectList>"
	"<ColorList>"
		"<Color>Red</Color>"
		"<Color>White</Color>"
		"<Color>Blue</Color>"
	"</ColorList>"
"</Customer>";

int main(int argc, char* argv[])
{
    MyCustomer c;
	c.FromXMLX(pCustomerUpdate);
	cout << c.ToXML() << "\n\n";
	
	return 0;
}


