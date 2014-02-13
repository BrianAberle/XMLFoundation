// --------------------------------------------------------------------------
//						United Business Technologies
//			  Copyright (c) 2000 - 2010 All Rights Reserved.
//
// Source in this file is released to the public under the following license:
// --------------------------------------------------------------------------
// This toolkit may be used free of charge for any purpose including corporate
// and academic use.  For profit, and Non-Profit uses are permitted.
//
// This source code and any work derived from this source code must retain 
// this copyright at the top of each source file.
// --------------------------------------------------------------------------

#ifndef __XML_ELEMENT_TREE_H_
#define __XML_ELEMENT_TREE_H_

#include "GList.h"
#include "xmlLex.h"

class CXMLElement;
class CXMLTree : public xml::lex
{
	GList m_paramtererEntities;
	GList m_generalEntities;

protected:
	CXMLElement *addChild(CXMLElement *parent, 
						  xml::token &tok,
						  const char *TokTag,
						  __int64 TokTagLength);
	CXMLElement *addAttribute(CXMLElement *parent, 
							  xml::token &tok,
							  const char *TokTag,
							  __int64 TokTagLength);
	CXMLElement *m_root;

public:

	void PruneTree(CXMLElement *root = 0);
	virtual void parseXML(char *xml, bool bEatWhite = 0);

	inline CXMLElement *getRoot(void);

	inline void detach(void);
	inline void attach(CXMLElement *);

	/*
		constructs an empty XML tree
		and initializes the XML parser
	*/
	CXMLTree();
	
	/*
		frees all elements, attributes of elements
		and child elements in the tree.
	*/
	virtual ~CXMLTree();
};

inline void CXMLTree::attach(CXMLElement *pNode)
{
	if (m_root != 0)
		throw 1;
	m_root = pNode;
}

inline CXMLElement *CXMLTree::getRoot(void)
{
	return m_root;
}

inline void CXMLTree::detach(void)
{
	m_root = 0;
}

#endif // __XML_ELEMENT_TREE_H_
