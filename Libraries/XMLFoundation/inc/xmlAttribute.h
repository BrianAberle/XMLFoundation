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

#ifndef __XML_ATTRIBUTE_H_
#define __XML_ATTRIBUTE_H_

#include "xmlDefines.h" // for __int64 definition

class GString;
class CXMLAttribute
{
	bool m_bOwnsMemory;
	bool m_bDisableOutputEscaping;

protected:

// Warning: the following information is public only for preformance
// reasons.  Some old compilers ignored inlining requests forcing a new
// stack frame for each access.  Due to potential large CPU cycles 
// being used on a per attribute level - the data has been made public
// and the application developer is warned not to change this data.
public:
	//	attribute tag name
	char *m_tag;
	__int64   m_nTagLen;

	//	attribute value
	char *m_value;
	__int64 m_nValueLen;

public:

	inline void disableOutputEscaping();

	//	returns the name of the attribute
	inline const char *getTag() const;
	inline __int64 getTagLen() const;

	//	returns the value of the attribute
	inline const char *getValue() const;
	inline __int64 getValueLen() const;

	//	sets the value of the attribute
	void setValue(const char *value);
	inline void setValue(const char *value, __int64 nValueLen);

	//	constructs an attribute w/a tag value pair
	CXMLAttribute(const char *Tag, __int64 nTagLen);
	CXMLAttribute(const char *Tag);
	CXMLAttribute(const char *Tag, __int64 nTagLen,
				  const char *Value, __int64 nValueLen);
	CXMLAttribute(const char *Tag, const char *Value);
	~CXMLAttribute();


	// define a copy constructor
	CXMLAttribute(const CXMLAttribute& attribute); 

	// inserts valid XML into the destination string
	void createXML(GString& str_xml) const;

	// define an assignment operator
	inline void operator=(const CXMLAttribute& attribute); 

	// define an equality test operator
	inline int operator==(const CXMLAttribute& attribute) const; 

	// define a less-than operator
	inline int operator<(const CXMLAttribute& attribute) const; 
};

inline void CXMLAttribute::disableOutputEscaping()
{
	m_bDisableOutputEscaping = true;
}

inline const char *CXMLAttribute::getTag() const
{
	return m_tag;
}

inline __int64 CXMLAttribute::getTagLen() const
{
	return m_nTagLen;
}

inline const char *CXMLAttribute::getValue() const
{
	return m_value;
}

inline __int64 CXMLAttribute::getValueLen() const
{
	return m_nValueLen;
}

inline void CXMLAttribute::setValue(const char *value, __int64 nValueLen)
{
	if (!m_bOwnsMemory)
	{
		m_value = (char *)value;
		m_nValueLen = nValueLen;
	}
	else
	{
		// "Design time Error!  The Attribute and it's value must use the same memory management." <<  endl;
		throw 1;
	}
}

#endif // __XML_ATTRIBUTE_H_
