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

#ifndef _ATTRIB_BASE
#define _ATTRIB_BASE

#include "xmlDefines.h"
#include "GString.h"
#include "GList.h"

class GListIterator;
class XMLAttributeList
{
public:
	class XMLAttributeNameValuePair
	{
	public:
		GString strAttrName;
		GString strAttrValue;
		XMLAttributeNameValuePair(const char * pzName, const char * pzValue);
		~XMLAttributeNameValuePair(){}
	};

	// CXMLStringAttribute used to be a class declared inside AttributeBase
	// When the constructor for CXMLStringAttribute was moved to a separate cpp file
	// The compiler complained that it does not understand the class name CXMLStringAttribute
	GList m_attrList; // contains XMLAttributeNameValuePair's
	~XMLAttributeList();
	void AddAttribute(const char * pzName, const char * pzValue, int nUpdate=0);
	const char *FindAttribute(const char *pzAttName);
	void UpdateAttributes(XMLAttributeList *p);
	void RemoveAll();
};

#endif //_ATTRIB_BASE
