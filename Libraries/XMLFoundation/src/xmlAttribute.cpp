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
#include "GlobalInclude.h"
static char SOURCE_FILE[] = __FILE__;

#include "xmlAttribute.h"
#include "xmlElement.h"
#include <string.h> // for: strlen(), memcpy(), strcpy()

CXMLAttribute::CXMLAttribute(const char *tag) :
	m_bDisableOutputEscaping(false)
{
	m_bOwnsMemory = true;
	m_value = 0;
	m_nTagLen = strlen(tag);
	m_tag = new char [m_nTagLen + 1];
	memcpy(m_tag, tag, m_nTagLen);
	m_tag[m_nTagLen] = 0;
}

CXMLAttribute::CXMLAttribute(const char *tag, __int64 nTagLen ) :
	m_bDisableOutputEscaping(false)
{
	m_bOwnsMemory = 0;
	m_value = 0;
	m_tag = (char *)tag;
	m_nTagLen = nTagLen;
}

CXMLAttribute::CXMLAttribute(const char *tag, const char *Value) :
	m_bDisableOutputEscaping(false)
{
	m_bOwnsMemory = true;

	m_nTagLen = strlen(tag);
	m_tag = new char [m_nTagLen + 1];
	memcpy(m_tag, tag, m_nTagLen);
	m_tag[m_nTagLen] = 0;

	m_nValueLen = strlen(Value);
	m_value = new char [m_nValueLen + 1];
	memcpy(m_value, Value, m_nValueLen);
	m_value[m_nValueLen] = 0;

}

CXMLAttribute::CXMLAttribute(const char *tag, __int64 nTagLen,
							 const char *Value, __int64 nValueLen) :
	m_bDisableOutputEscaping(false)
{
	m_bOwnsMemory = false;

	m_tag = (char *)tag;
	m_nTagLen = nTagLen;

	m_value = (char *)Value;
	m_nValueLen = nValueLen;
}

CXMLAttribute::~CXMLAttribute()
{
	if (m_bOwnsMemory)
	{
		delete m_tag;
		delete m_value;
	}
}

void CXMLAttribute::setValue(const char *value)
{
	if (m_bOwnsMemory)
	{
		if (m_value)
		{
			delete m_value;
		}
	}

	m_nValueLen = strlen(value);
	m_value = new char [m_nValueLen + 1];
	strcpy(m_value, value);
}


//*********************************************************
// Description:
//		inserts valid XML into the destination string
// 
//*********************************************************
void CXMLAttribute::createXML(GString& str_xml) const
{
	m_tag[m_nTagLen] = 0;
	if ((m_value) && (m_nValueLen > 0))
		m_value[m_nValueLen] = 0;

	str_xml << ' ';
	// insert the atribute tag
	str_xml << m_tag;
	str_xml << "=\"";

	// insert the atribute value
	if (m_bDisableOutputEscaping)
		str_xml << m_value;
	else // note: need to pass , nSerializeFlags to AppendEscapeXMLReserved()
		str_xml.AppendEscapeXMLReserved(m_value);
	str_xml << '"';
}
