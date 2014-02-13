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
#include "GList.h"
#include "GString.h" // for strnicmp(), and stricmp()

#include <string.h> // for: strlen(), strcpy(), memset(), memcmp()
#include <stdio.h>  // for: sscanf(), sprintf()
#include <ctype.h> // for: tolower(), toupper()

//	#define strcasecmp	stricmp



// creates a text node that owns the memory
// allocated to hold the text
CXMLNode::CXMLNode(const char *szText) :
	m_bOwnsMemory(0),
	m_node(0),
	m_bDisableOutputEscaping(0)
{
		setValue((szText) ? szText : "");
}

// creates a text node that doesn't own the
// memory allocated to hold the text
CXMLNode::CXMLNode(const char *szText, __int64 nLength) :
	m_bOwnsMemory(0),
	m_node(0),
	m_bDisableOutputEscaping(0)
{
	setValue(szText, nLength);
}

// creates an element node
CXMLNode::CXMLNode(CXMLElement *element) :
	m_bOwnsMemory(0),
	m_node(0),
	m_bDisableOutputEscaping(0)
{
	setValue(element);
}

CXMLNode::~CXMLNode()
{
	if (m_bOwnsMemory)
		delete (char *)m_node;
}


void CXMLNode::setValue(const char *value)
{
	if (m_bOwnsMemory)
		delete (char *)m_node;
	m_type = textN;
	if (value)
	{
		m_len = strlen(value);
		m_node = new char [m_len + 1];
		strcpy((char *)m_node, value);
	}
	else
	{
		m_len = 0;
		m_node = new char [m_len + 1];
		memset(m_node,0,1);
	}
	m_bOwnsMemory = true;
}


//*********************************************************
// Description:
//		creates a new attribute and adds the newly created 
//		attribute to the attribute list.
// 
//*********************************************************
CXMLAttribute *CXMLElement::addAttribute(const char *tag, const char *value,
										 int bAppend /* = 1 (true) */,int bMakeValid /* = 1*/)
{
	char *pzNewValid = 0;
	if (!m_pAttributelist)
	{
		m_pAttributelist = new GList;
	}
	else if (bMakeValid)
	{
		__int64 nAddingTagLen = strlen(tag);
		int nThisCount = 1;
		int nLargest = 1;
		int bIsRepeat = 0;

		// if true, then convert "hello" to "hello1" if "hello" already exists.
		// likewise convert "hello" to "hello7" if "hello6" already exists.
		if (bMakeValid)
		{
			GListIterator itAttrs(m_pAttributelist);
			// run through the existing attributes
			while (itAttrs())
			{
				CXMLAttribute *pA = (CXMLAttribute *)itAttrs++;
				// if the pA's Length is >= than ours we need to check if it's the same name
				if (pA->m_nTagLen >= nAddingTagLen)
				{
					if ( memcmp(tag,pA->m_tag,nAddingTagLen) == 0 )
					{
						bIsRepeat = 1;
						// if the sizes are ==, they are the same - skip the sscanf
						nThisCount = 0;
						if (pA->m_nTagLen > nAddingTagLen)
						{
							// convert the ascii following the matching name to an integer
							sscanf (&pA->m_tag[nAddingTagLen], "%d",  &nThisCount);
						}
					}
				}
				// keep a running total of the largest number
				if (nThisCount > nLargest)
					nLargest = nThisCount;
			}
			// create the new "incremented attribute" if necessary
			if (bIsRepeat)
			{
				pzNewValid = new char[nAddingTagLen + 20];
				sprintf(pzNewValid,"%s%d",tag,(int)nLargest+1);
			}
			// use our new name as the attribute name in the XML document
			if (pzNewValid)
				tag = pzNewValid;
		}
	}
	CXMLAttribute *p = new CXMLAttribute(tag,value);
	if (bAppend)
		m_pAttributelist->AddLast(p);
	else
		m_pAttributelist->AddHead(p);
	
	if (pzNewValid)
		delete[] pzNewValid;

	return p;
}

CXMLAttribute *CXMLElement::addAttribute(const char *tag, __int64 nTagLen,
										const char *value, __int64 nValueLen)
{
	if (!m_pAttributelist)
	{
		m_pAttributelist = new GList;
	}
	
	CXMLAttribute *p = new CXMLAttribute(tag,nTagLen,value,nValueLen);
	m_pAttributelist->AddLast(p);
	return p;
}

// copies the tag/value data, so the source need not 
// be scoped beyond the current stack frame
CXMLElement *CXMLElement::addChild(const char *tag, 
								   const char *value)
{
	return addChild(new CXMLElement(tag,value,this));
}

// this data will only be pointed to by the CXMLElement so it 
// must be valid for the duration of this element.
CXMLElement *CXMLElement::addChild(const char *tag, __int64 nTagLen, 
								   const char *value, __int64 nValueLen)
{
	return addChild(new CXMLElement(tag, nTagLen, value, nValueLen, this));
}

// this data will only be pointed to by the CXMLElement so it 
// must be valid for the duration of this element.
CXMLElement *CXMLElement::addChild(const char *tag, __int64 nTagLen)
{
	return addChild(new CXMLElement(tag, nTagLen, this));
}


//*********************************************************
// Description:
//		add an existing CXMLElement as an element
// 
//*********************************************************
CXMLElement *CXMLElement::addChild(CXMLElement *pElement, CXMLElement *ia /* = 0 */)
{
	if (!m_pChildrenlist)
	{
		m_pChildrenlist = new GList;
	}

	if (!ia)
	{
		m_pChildrenlist->AddLast(pElement);
		m_TextChildList.AddLast(new CXMLNode(pElement));
	}
	else
	{
		bool bFound = false;
		// insert after ia
		m_TextChildList.First();
		while (m_TextChildList.Current() != NULL)
		{
			CXMLNode *p = (CXMLNode *)m_TextChildList.Current();
			if ((p->getType() != CXMLNode::textN) &&
				(ia == p->getValue()))
			{
				bFound = true;
				m_TextChildList.AddAfterCurrent(new CXMLNode(pElement));
				break;
			}
			m_TextChildList.Next();
		}

		if (!bFound)
			m_TextChildList.AddLast(new CXMLNode(pElement));

		bFound = false;
		m_pChildrenlist->First();
		while (m_pChildrenlist->Current() != NULL)
		{
			if (pElement == m_pChildrenlist->Current())
			{
				bFound = true;
				m_pChildrenlist->AddAfterCurrent(pElement);
				break;
			}
			m_pChildrenlist->Next();
		}
		if (!bFound)
			m_pChildrenlist->AddLast(pElement);
	}
	
	return pElement;
}

//*********************************************************
// Description:
//		constructs an element w/a tag value pair, an empty 
//		attribute list, and an empty child list.
// 
//*********************************************************
CXMLElement::CXMLElement(const char *tag, 
						 const char *value,
						 CXMLElement *parent) :
	m_bOwnsTag(true)
{
	m_parent = parent;
	m_nTagLen = strlen(tag);

	m_tag = new char [m_nTagLen + 1];
	memcpy(m_tag, tag, m_nTagLen);
	m_tag[m_nTagLen] = 0;

	m_pAttributelist = 0;
	m_pChildrenlist = 0;
	m_itemData[0] = 0;
	m_itemData[1] = 0;
	m_itemData[2] = 0;
	m_itemData[3] = 0;
	m_itemData[4] = 0;

	m_value = new CXMLNode(value);
	m_TextChildList.AddLast(m_value);
}

CXMLElement::CXMLElement(const char *tag, __int64 nTagLen, 
						 const char *value, __int64 nValueLen,
						 CXMLElement *parent)  :
	m_bOwnsTag(false)
{
	m_parent = parent;

	m_tag = (char *)tag;
	m_nTagLen = nTagLen;

	m_pAttributelist = 0;
	m_pChildrenlist = 0;
	m_itemData[0] = 0;
	m_itemData[1] = 0;
	m_itemData[2] = 0;
	m_itemData[3] = 0;
	m_itemData[4] = 0;

	m_value = new CXMLNode(value, nValueLen);
	m_TextChildList.AddLast(m_value);
}


//*********************************************************
// Description:
//		constructs an empty element w/only a tag, an empty 
//		attribute list, and an empty child list.
// 
//*********************************************************
CXMLElement::CXMLElement(const char *tag, __int64 nTagLen, 
						 CXMLElement *parent) :
	m_bOwnsTag(false)
{
	static const char *szEmpty = "";

	m_parent = parent;

	m_tag = (char *)tag;
	m_nTagLen = nTagLen;

	m_pAttributelist = 0;
	m_pChildrenlist = 0;
	m_itemData[0] = 0;
	m_itemData[1] = 0;
	m_itemData[2] = 0;
	m_itemData[3] = 0;
	m_itemData[4] = 0;
	m_value = new CXMLNode(szEmpty, 0);
	m_TextChildList.AddLast(m_value);
}

//*********************************************************
// Description:
//		frees all attributes and children
// 
//*********************************************************
CXMLElement::~CXMLElement()
{
	if (m_bOwnsTag)
		delete m_tag; 

	//for each item in the child list:
	if (m_pChildrenlist)
	{
		GListIterator itChildren(m_pChildrenlist);
		while (itChildren())
		{
			delete (CXMLElement *)itChildren++;
		}
		delete m_pChildrenlist;
		m_pChildrenlist = 0;
	}

	// for each item in the m_attributelist
	if (m_pAttributelist)
	{
		GListIterator itAttrs(m_pAttributelist);
		while (itAttrs())
		{
			delete (CXMLAttribute *)itAttrs++;
		}
		delete m_pAttributelist;
		m_pAttributelist = 0;
	}

	// for each item in the m_TextChildList
	GListIterator it(&m_TextChildList);
	while (it())
	{
		delete (CXMLNode *)it++;
	}

}

//	inserts valid XML into the destination string
void CXMLElement::createXML(GString& str_xml, 
							int nTabs/* = 0*/, 
							bool bAddDocType, /* = false */
							bool bBeautify, /* = true */
							bool bShortHand) const
{
	m_tag[m_nTagLen] = 0;

	if ((bAddDocType) && (nTabs == 0))
	{

#ifdef __EVAL_COPY__

		str_xml << "<!-- \n"
				   "********** Evaluation Version of TransactXML Server. **********\n\n"
				   "United Business Technologies, Inc. grants use of this product \n"
				   "in Academic or non-commercial, non-profit, non-business environments\n"
				   "for evaluation purposes only.\n\n"
				   "Copyright (c) 1998, 1999, 2000, 2001 United Business Technologies, Inc.\n"
				   "All Rights Reserved. \n\n"
				   "www.UnitedBusinessTechnologies.com\n"
				   " -->\n";
#endif

	}

	if (bAddDocType && *m_tag)
	{
		str_xml	<< "<!DOCTYPE ";
		str_xml << m_tag;
		str_xml << ">\n";
	}

	if (bBeautify)
	{
		while(nTabs)
		{
			str_xml.write( g_TABS,(nTabs > MAX_TAB_WRITE) ? MAX_TAB_WRITE : nTabs  );
			if (nTabs > MAX_TAB_WRITE)
			{
				nTabs -= MAX_TAB_WRITE;
			}
			else
			{
				break;
			}
		}
	}

	// insert the element's m_tag
	str_xml << '<';
	str_xml << m_tag;

	// insert the element's attributes
	if (m_pAttributelist)
	{
		GListIterator itAttrs(m_pAttributelist);
		while (itAttrs())
		{
			((CXMLAttribute *)itAttrs++)->createXML(str_xml);
		}
	}

	// if the element is empty, terminate the start m_tag
	if (!getValueLen() && m_TextChildList.Size() <= 1)
	{
		if (bShortHand)
		{
			str_xml << "/>";
		}
		else
		{
			str_xml << '>';
			str_xml << "</";
			str_xml << m_tag;
			str_xml << '>';
		}
		if (bBeautify)
		{
			str_xml << "\n";
		}
	}
	// otherwise, insert the element value (escaping all mark-up)
	// and then insert all children.
	else
	{
		// terminate the start tag
		str_xml << '>';

		// Iterate through the m_TextChildList and 
		// output all text and element nodes
		GListIterator it(&m_TextChildList);
		bool bNewLine = true;
		while (it())
		{
			CXMLNode *p = (CXMLNode *)it++;
			if (p->getType() == CXMLNode::textN)
			{
				if (p->getDisableOutputEscaping())
					str_xml.write((const char *)p->getValue(), p->getValueLen());
				else
				{
					// note: need to pass , nSerializeFlags to AppendEscapeXMLReserved()
					str_xml.AppendEscapeXMLReserved((const char *)p->getValue(), p->getValueLen());
				}
			}
			else
			{
				if (bNewLine && bBeautify)
					str_xml << "\n";
				((CXMLElement *)p->getValue())->createXML(str_xml, nTabs + 1, false, bBeautify, bShortHand);
				bNewLine = false;
			}
		}

		if ( m_pChildrenlist && bBeautify )
		{
			while(nTabs)
			{
				str_xml.write( g_TABS,(nTabs > MAX_TAB_WRITE) ? MAX_TAB_WRITE : nTabs  );
				if (nTabs > MAX_TAB_WRITE)
				{
					nTabs -= MAX_TAB_WRITE;
				}
				else
				{
					break;
				}
			}

		}

		// insert the element's end tag
		str_xml << "</";
		str_xml << m_tag;
		str_xml << '>';
		if (bBeautify)
			str_xml << "\n";
	}
}


void CXMLElement::removeAllChildren()
{
	//for each item in the child list:
	if (m_pChildrenlist)
	{
		GListIterator itChildren(m_pChildrenlist);
		while (itChildren())
		{
			delete (CXMLElement *)itChildren++;
		}
		delete m_pChildrenlist;
		m_pChildrenlist = 0;
	}

	// remove all children from the m_TextChildList
	m_TextChildList.First();
	while (m_TextChildList.Current() != NULL)
	{
		CXMLNode *p = (CXMLNode *)m_TextChildList.Current();
		if (p->getType() == CXMLNode::elementN)
			m_TextChildList.RemoveCurrent();
		else
			m_TextChildList.Next();
	}
}

void CXMLElement::removeChild(CXMLElement *pElement, bool bDestroy /* = true */)
{
	// remove the  child from the m_TextChildList)
	m_TextChildList.First();
	while (m_TextChildList.Current() != NULL)
	{
		CXMLNode *p = (CXMLNode *)m_TextChildList.Current();
		if ((p->getType() != CXMLNode::textN) &&
			(pElement == p->getValue()))
		{
			delete p;
			m_TextChildList.RemoveCurrent();
			break;
		}
		m_TextChildList.Next();
	}

	if (m_pChildrenlist)
	{
		m_pChildrenlist->First();
		while (m_pChildrenlist->Current() != NULL)
		{
			if (pElement == m_pChildrenlist->Current())
			{
				if (bDestroy)
					delete (CXMLElement *)m_pChildrenlist->Current();
				m_pChildrenlist->RemoveCurrent();
				break;
			}
			m_pChildrenlist->Next();
		}
	}
}

void CXMLElement::setParent(CXMLElement *np, CXMLElement *ia /* = 0 */)
{
	CXMLElement *parent = getParent();
	if (parent)
		parent->removeChild(this, false);

	np->addChild(this, ia);
	m_parent = np;
}

CXMLAttribute *CXMLElement::findAttribute(const char *tag) const
{
	if (m_pAttributelist)
	{
		GListIterator itAtts(m_pAttributelist);
		while (itAtts())
		{
			CXMLAttribute *p = (CXMLAttribute *)itAtts++;
			if (p->getTagLen() == (int)strlen(tag))
			{
				if (strnicmp(p->getTag(), tag, p->getTagLen() ) == 0)
				{
					return p;
				}
			}
		}
	}

	return NULL;
}

// find the 1 based nth Occurrence of a child element
CXMLElement *CXMLElement::findChild(const char *tag, int nOccurrence/* = 1*/) const
{
	int nFoundOccurrence = 0;
	if (m_pChildrenlist)
	{
		GListIterator itChildren(m_pChildrenlist);
		while (itChildren())
		{
			CXMLElement *p = (CXMLElement *)itChildren++;
			if (p->getTagLen() == (int)strlen(tag))
			{
				if (strnicmp(p->getTag(), tag, p->getTagLen() ) == 0)
				{
					nFoundOccurrence++;
					if (nOccurrence == nFoundOccurrence)
						return p;
				}
			}
		}
	}
	return 0;
}


//void CXMLElement::toFile(const char *dst, long nGrowBy /* = 5000 */) const
//{
//	GString strBuffer( nGrowBy );
//	createXML(strBuffer);
//
//	fstream fs(dst,  ios::out|ios::trunc);
//	if (fs.good())
//	{
//		fs.write((const char *)strBuffer, strBuffer.Length());
//	}
//}
