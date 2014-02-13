// --------------------------------------------------------------------------
//						United Business Technologies
//			  Copyright (c) 2000 - 2010  All Rights Reserved.
//
// Source in this file is released to the public under the following license:
// --------------------------------------------------------------------------
// This toolkit may be used free of charge for any purpose including corporate
// and academic use.  For profit, and Non-Profit uses are permitted.
//
// This source code and any work derived from this source code must retain 
// this copyright at the top of each source file.
// --------------------------------------------------------------------------



// This allows an object to have a value that is not in a member variable.
// The concept is not allowed in C++, but is valid XML syntax.  For example,
// <SomeObject>										// maps to object
//		This is the object value					// maps to ObjectDataHandler
//		<ObjectMember>member value</ObjectMember>	// maps to object member
// <//SomeObject>
// if an object is interested in "subscribing" to this data that does not 
// have a cooresponding member variable the following methods can be overriden.

// A real world usage example :
// <PhoneNumber>
// 	   (415)123-4567x123
// </PhoneNumber>
// where the object model defines PhoneNumber as an object, not a string.
//
// class PhoneNumber : public XMLObject, public XMLObjectDataHandler
// {
// 	string areacode;
// 	string prefix;
// 	string suffix;
// 	string extension;
//  MapXMLTagsToMembers(){/*nothing to map*/}
//	PhoneNumber()
//	{
//		Register the special data handler as 'this' object, You may use
//		any object derived from XMLObjectDataHandler and eliminate the
//		multiple inheritance used in this example.
//		setObjectDataHandler(this);
//	}
//	void SetObjectValue(const char *strSource)
//	{
		// parse the data "\n\t(415)123-4567x123\n\0"
		// and place the values in the cooresponding
		// member variables.
//	}

//	You must escape "&'<> with &quot; &amp; &apos; &lt; and &gt;
//  you can use GString::AppendEscapeXMLReserved() to achieve this
//	void AppendObjectValue(GString& xml)
//	{
//		xml << "(" << xml.AppendEscapeXMLReserved(areacode) << ")"
//			<< xml.AppendEscapeXMLReserved(prefix) << "-"
//			<< xml.AppendEscapeXMLReserved(suffix) << "x"
//			<< xml.AppendEscapeXMLReserved(extension);
//	}
// };
//
//
///////////////////////////////////////////////////////////////////////
// 
// You may also create a 'handler' and not use Multiple Inheritance like this:
//
// class MyPhoneNum :  public XMLObjectDataHandler
// {
// 	string areacode;
// 	string prefix;
// 	string suffix;
// 	string extension;
//	void SetObjectValue(const char *strSource)
//	{
		// parse the data "\n\t(415)123-4567x123\n\0"
		// and place the values in the cooresponding
		// member variables.
//	}

//	You must escape "&'<> with &quot; &amp; &apos; &lt; and &gt;
//  you can use GString::AppendEscapeXMLReserved() to achieve this
//	void AppendObjectValue(GString& xml)
//	{
//		xml << "(";  xml.AppendEscapeXMLReserved(areacode);  xml << ")";
//		xml.AppendEscapeXMLReserved(prefix); xml << "-";
//		xml.AppendEscapeXMLReserved(suffix); xml << "x";
//		xml.AppendEscapeXMLReserved(extension);
//	}
// }
//
//
// using this approach, then create an instance as a member variable:
//
// MyPhoneNum  m_Mine;
// 
// then set the handler - you may also set the same handler in multiple places this way
//
//	setObjectDataHandler(&m_Mine);



#ifndef _XML_OBJECT_HANDLER_H__
#define _XML_OBJECT_HANDLER_H__

class XMLObjectDataHandler
{
public:
	XMLObjectDataHandler(){ };
	virtual ~XMLObjectDataHandler(){ };

public:
	// called by the framework to assign the value to your object
	// nLength is the length of strSource unless it is -1 to indicate strSource is a null terminated string
	// nType 1 is Parsed Object Data, 
	// nType 2 is <![CDATA
	// nType 3 is custom use
	virtual void SetObjectValue(const char *strSource, __int64 nLength, int nType) = 0;
	// called by the framework during serialization for you 
	// to add the object value to the XML stream
	virtual void AppendObjectValue(GString& xml) = 0;
	// called by XMLObject::GetObjectDataValue() to obtain the raw(unescaped) object value

	// return storage for CDATA
	virtual GString *GetCData() {return 0;}
	// return storage for Object Data
	virtual GString *GetOData() {return 0;}
	

	// these two methods do NOT need to be implemented.  They are called by 
	// XMLObject::GetObjectDataValue() and XMLObject::SetObjectDataValue() 
	// which are NOT called by the framework - only by the user when accessing data
	// inside the "DefaultDataHandler" class implemented in XMLObject.h
	virtual const char * GetObjectValue(GString &str){ return 0; }
	virtual void AssignObjectValue(const char *strSource){}
};

#endif //_XML_OBJECT_HANDLER_H__

