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

#include "AttributeList.h"

#ifdef _WIN32
	#include <string.h> // for stricmp()
	#define strcasecmp	_stricmp
#else
	//_LINUX and __sun
	#include <strings.h>// for :strcasecmp()
#endif

XMLAttributeList::XMLAttributeNameValuePair::XMLAttributeNameValuePair(const char * pzName, const char * pzValue)
{ 
	strAttrName	= pzName; 
	strAttrValue = pzValue;
};

void XMLAttributeList::UpdateAttributes(XMLAttributeList *p)
{
	GListIterator Iter(&p->m_attrList);
	while (Iter())
	{
		XMLAttributeNameValuePair *temp = (XMLAttributeNameValuePair *)Iter++;
		AddAttribute((const char *)temp->strAttrName, (const char *)temp->strAttrValue, 1);
	}
}

void XMLAttributeList::RemoveAll()
{
	GListIterator Iter(&m_attrList);
	while (Iter())
	{
		delete (XMLAttributeNameValuePair *)Iter++;
	}
	m_attrList.RemoveAll();
}


void XMLAttributeList::AddAttribute(const char * pzName, const char * pzValue, int nUpdate/*=0*/)
{ 
	if (nUpdate)
	{
		GListIterator Iter(&m_attrList);
		while (Iter())
		{
			XMLAttributeNameValuePair *temp = (XMLAttributeNameValuePair *)Iter++;
			if ( strcasecmp((const char *)temp->strAttrName, pzName ) == 0)
			{
				temp->strAttrValue = pzValue;
				return;
			}
		}
	}

	// add it if we could not, or should not, update it
	m_attrList.AddLast(new XMLAttributeNameValuePair(pzName, pzValue));
};

XMLAttributeList::~XMLAttributeList()
{ 
	GListIterator Iter(&m_attrList);
	while (Iter())
	{
		delete (XMLAttributeNameValuePair *)Iter++;
	}
};

// return a pointer to the value of the first attribute in the attribute
// list matching the supplied name.
const char *XMLAttributeList::FindAttribute( const char *pzAttName )
{
	GListIterator Iter(&m_attrList);
	while (Iter())
	{
		XMLAttributeNameValuePair *temp = (XMLAttributeNameValuePair *)Iter++;
		if ( strcasecmp((const char *)temp->strAttrName, pzAttName ) == 0)
		{
			return (const char *)temp->strAttrValue;
		}
	}
	return 0;
}

