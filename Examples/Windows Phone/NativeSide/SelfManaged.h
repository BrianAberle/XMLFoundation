#pragma once
using namespace InterfaceMyType;
#include "XMLFoundation.h"
#include <string.h> // only for strcpy() in this example

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
//	unsigned __int64 m_bits;	// 64 bits = You can comment out the 8 bit line above and replace it with this one.

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

namespace NativeSide
{
	public ref class SelfManaged sealed
    {
		int m_isRunning;
    public:
        SelfManaged(IMyType^ candidate);
		Platform::String^ SelfManaged::GetServerLogData();
		Platform::String^ NativeStartHere0();
		Platform::String^ StartServerCore(Platform::String^ str);
		Platform::String^ Test();
	private:
		IMyType^ m_MyManagedObject;
    };
}