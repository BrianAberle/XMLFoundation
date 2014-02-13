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

#ifndef __XML_ELEMENT_H_
#define __XML_ELEMENT_H_

#ifndef _WIN32
	#include "xmlDefines.h"
#endif

#include "GList.h"
#include "GString.h"

class CXMLAttribute;

class CXMLElement;

// class used to store text or element node pointer
class CXMLNode
{
public:

	//typedef enum xmlNodeType { textN, elementN };
	enum xmlNodeType { textN, elementN };

protected:

	// pointer to a text node or an XMLElement node
	void *m_node;
	__int64  m_len;
	bool  m_bOwnsMemory;
	bool  m_bDisableOutputEscaping;
	xmlNodeType m_type;

public:

	xmlNodeType getType() { return m_type; }

	void setValue(const char *value);
	inline void setValue(const char *value, __int64 nValueLen);
	inline const void *getValue() const { return m_node; }
	inline __int64 getValueLen() const { return m_len; }
	inline void setValue(CXMLElement *element);

	inline void disableOutputEscaping();
	inline bool getDisableOutputEscaping();

	// creates a text node that owns the memory
	// allocated to hold the text
	CXMLNode(const char *szText);

	// creates a text node that doesn't own the
	// memory allocated to hold the text
	CXMLNode(const char *szText, __int64 nLength);

	// creates an element node
	CXMLNode(CXMLElement *element);

	// destroys the text node if it owns the memory
	~CXMLNode();
};

inline void CXMLNode::disableOutputEscaping()
{
	m_bDisableOutputEscaping = true;
}

inline bool CXMLNode::getDisableOutputEscaping()
{
	return m_bDisableOutputEscaping;
}

inline void CXMLNode::setValue(const char *value, __int64 nValueLen)
{
	if (m_bOwnsMemory)
		delete (char *)m_node;
	m_type = textN;
	m_bOwnsMemory = false;
	m_node = (char *)value;
	m_len = nValueLen;
}

inline void CXMLNode::setValue(CXMLElement *value)
{
	if (m_bOwnsMemory)
		delete (char *)m_node;
	m_type = elementN;
	m_node = (CXMLElement *)value;
	m_len = sizeof(CXMLElement *);
}

class CXMLElement
{

protected:

	CXMLElement *m_parent;

	// list of attributes for the element
	GList *m_pAttributelist;

	// list of child elements
	GList *m_pChildrenlist;

	// list of text nodes and child elements in document order
	GList m_TextChildList;

	void *m_itemData[5];

	GString m_strOID;

// Warning: the following information is public only for preformance
// reasons.  Several compilers ignore inlining requests forcing a new
// stack frame for each access.  Due to potential large CPU cycles 
// being used on a per element level - the data has been made public
// so that we guarantee another stack frame will not be created to access
// this data. The application developer is warned not to change this data.
public:
	// element tag name
	char *m_tag;
	__int64 m_nTagLen;

	bool m_bOwnsTag;

	// element value (first text node in the m_TextChildList)
	CXMLNode *m_value;

public:
//	void escapeValueAppend(const char *src, GString& str_xml, int nLength = -1) const;

	inline void setOID(const char *szOID);
	inline const char * getOID() const;

	inline void setItemData(void *itemData, int nIndex = 0);
	inline void *getItemData(int nIndex = 0) const;

	// Warning: our implementation of the XML parser does not copy data.
	// The tokenizer parses a source input stream. Pointers into the stream
	// are handed up to the Parser that creates objects like this CXMLElement.
	// Since the source input never gets copied the tokenizer is very fast
	// and it uses 1/2 the memory of 'normal' parsers.  Using these members
	// will sprinkle null terminations in the actual source buffer of XML that
	// was passed into the XML Parser breaking the C++ rule of const.  Do not 
	// use these methods if you want to preserve the source XML buffer that 
	// you parsed.
	inline const char *getpzTag() const;
	inline const char *getpzValue() const;

	// returns the name of the element, may not be null terminated!
	inline const char *getTag() const;
	inline __int64 getTagLen() const;

	// returns the value of the element, may not be null terminated!
	inline const char *getValue() const;
	inline __int64 getValueLen() const;

	// sets the value of the element
	// this sets the first text node in the
	// m_TextChildList
	inline void setValue(const char *value);
	inline void setValue(const char *value, __int64 nValueLen);

	// appends a text value to the element
	// this preserves the order of the text nodes
	// and children nodes of the document.
	inline CXMLNode *appendText(const char *value);
	inline CXMLNode *appendText(const char *value, __int64 nValueLen);

	// returns a pointer to this element's parent or 
	// NULL if this element doesn't have a parent.
	inline CXMLElement *getParent() const;

	// unlinks itself from its current parent and
	// reasignes itself to a different parent.
	void setParent(CXMLElement *np, CXMLElement *ia = 0);

	inline GList *getChildren();
	inline GList *getAttributes();
	inline GList *getAllNodes();

	//	creates a new attribute and adds the newly created 
	//	attribute to the attribute list.
	CXMLAttribute *addAttribute(const char *tag, __int64 nTagLen,
								const char *value, __int64 nValueLen);
	CXMLAttribute *addAttribute(const char *tag, const char *value, int bAppend = 1,int bMakeValid = 1);


	CXMLAttribute *findAttribute(const char *tag) const;

	void removeAllChildren();

	//	creates a new element and adds the newly created child 
	//	to the children list.
	CXMLElement *addChild(const char *tag, 
						  const char *value);
	CXMLElement *addChild(const char *tag, 
						  __int64 nTagLen);
	CXMLElement *addChild(const char *tag, __int64 nTagLen, 
						  const char *value, __int64 nValueLen);
	// add an existing CXMLElement as an element
	CXMLElement *addChild(CXMLElement *pElement, CXMLElement *ia = 0);

	// find the 1 based nth Occurrence of a child element
	CXMLElement *findChild(const char *tag, int nOccurrence = 1) const;

	// remove a child node.
	void removeChild(CXMLElement *pElement, bool bDestroy = true);

	//	constructs an element w/a tag value pair, an empty 
	//	attribute list, and an empty child list.
	CXMLElement(const char *tag, 
				const char *value,
				CXMLElement *parent = 0);
	CXMLElement(const char *tag, __int64 nTagLen, 
				const char *value, __int64 nValueLen,
				CXMLElement *parent = 0);
	CXMLElement(const char *tag, __int64 nTagLen, 
			    CXMLElement *parent = 0);

	//	frees all attributes and children
	~CXMLElement();


	// inserts valid XML into the destination string
	void createXML(GString& str_xml, 
				   int nTabs = 0, 
				   bool bAddDocType = false, 
				   bool bBeautify = true,
				   bool bShortHand = true) const;

	// Define an equality test operator
	inline int operator==(const CXMLElement& element) const;

	// Define a less-than operator
	inline int operator<(const CXMLElement& element) const;

//	void toFile(const char *dst, long nGrowBy = 5000) const;
};

inline void CXMLElement::setOID(const char *szOID)
{
	m_strOID = szOID;
}

inline const char * CXMLElement::getOID() const
{
	return (const char *)m_strOID;
}

inline const char * CXMLElement::getpzTag() const
{
	m_tag[m_nTagLen] = 0;
	return m_tag;
}

inline const char * CXMLElement::getpzValue() const
{
	char *pV = (char *)(const char *)m_value->getValue();
	pV[m_value->getValueLen()] = 0;
	return pV;
}


// Warning: Data may not be null terminated.  Also use getTagLen()
inline const char *CXMLElement::getTag() const
{
	return m_tag;
}
inline __int64 CXMLElement::getTagLen() const
{
	return m_nTagLen;
}

inline void CXMLElement::setValue(const char *value)
{
	m_value->setValue(value);
}

inline void CXMLElement::setValue(const char *value, __int64 nValueLen)
{
	m_value->setValue(value, nValueLen);
}

inline CXMLNode *CXMLElement::appendText(const char *value)
{
	m_TextChildList.AddLast(new CXMLNode(value));
	return (CXMLNode *)m_TextChildList.Last();
}

inline CXMLNode *CXMLElement::appendText(const char *value, __int64 nValueLen)
{
	m_TextChildList.AddLast(new CXMLNode(value, nValueLen));
	return (CXMLNode *)m_TextChildList.Last();
}

// Warning: Data may not be null terminated.  Also use getValueLen()
inline const char *CXMLElement::getValue() const
{
	return (const char *)m_value->getValue();
}
inline __int64 CXMLElement::getValueLen() const
{
	return m_value->getValueLen();
}

inline CXMLElement *CXMLElement::getParent() const
{
	return m_parent;
}

inline GList * CXMLElement::getChildren()
{
	return m_pChildrenlist;
}

inline GList * CXMLElement::getAllNodes()
{
	return &m_TextChildList;
}

inline GList * CXMLElement::getAttributes()
{
	return m_pAttributelist;
}

inline void CXMLElement::setItemData(void *itemData, int nIndex/* = 0*/)
{
	if (nIndex > 4)
		return;
	m_itemData[nIndex] = itemData;
}

inline void *CXMLElement::getItemData(int nIndex /* = 0*/) const
{
	if (nIndex > 4)
		return 0;
	return m_itemData[nIndex];
}

#endif // __XML_ELEMENT_H_
