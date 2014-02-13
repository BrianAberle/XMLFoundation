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
#include "xmlElementTree.h"


//*********************************************************
// Description:
//			constructs an empty XML tree
//			and initializes the XML parser.
// 
//*********************************************************

CXMLTree::CXMLTree() :
	xml::lex (&m_paramtererEntities, 
			  &m_generalEntities,
			  0)
{
	m_root = 0;
}

//*********************************************************
// Description:
//		frees all elements, attributes of elements
//		and child elements in the tree.
// 
//*********************************************************

CXMLTree::~CXMLTree()
{
	delete m_root;
}

//*********************************************************
// Description:
// 
//*********************************************************

CXMLElement *CXMLTree::addChild(CXMLElement *parent, 
								xml::token &tok,
								const char *TokTag,
								__int64 TokTagLength)
{
	// get the value of the element, 
	// or next element tag or attribute
	nextToken(&tok);

	switch (tok.m_type)
	{
		// this is a new element followed by another element
		case xml::_startTag :
			parent = parent->addChild(TokTag, TokTagLength);
			parent = parent->addChild(tok.get(), tok.length());
			break;

		// this is a new element w/possible children
		case xml::_pcdata :
		case xml::_cdata :
			parent = parent->addChild(TokTag, TokTagLength, tok.get(), tok.length());
			break;

		// this is an empty element
		case xml::_emptyEndTag :
		case xml::_endTag :
			parent->addChild(TokTag, TokTagLength);
			break;

		// this is a new element with attributes
		case xml::_attributeName :
			parent = parent->addChild(TokTag, TokTagLength);
			parent = addAttribute(parent, tok, tok.get(), tok.length());
			break;
	}
	return parent;
}


//*********************************************************
// Description:
// 
//*********************************************************



CXMLElement *CXMLTree::addAttribute(CXMLElement *parent, 
									xml::token &tok,
									const char *TokTag,
									__int64 TokTagLength)
{
	nextToken(&tok);

	if (tok.m_type == xml::_pcdata)
	{
		parent->addAttribute(TokTag, TokTagLength, 
							 tok.get(), tok.length());
	}
	else
	{
		// error
	}

	return parent;
}

//*********************************************************
// Description:
//		parses the XML building the tree of elements
// 
//*********************************************************
void CXMLTree::parseXML(char *xml, bool bEatWhite /*= 0*/)
{
	setStream(xml);
	CXMLElement *current = 0;
	xml::token tok;
	do
	{
		nextToken(&tok);
		switch (tok.m_type)
		{
		case xml::_startTag :
			if (current != 0)	
			{
				current = addChild(current, tok, tok.get(), tok.length());
			}
			else if (m_root == 0)
			{
				m_root = new CXMLElement(tok.get(), tok.length());
				current = m_root;
			}
			break;
		case xml::_emptyEndTag :
		case xml::_endTag :
			if (current != 0)
				current = current->getParent();
			if (current == 0)
				return;
			break;
		case xml::_attributeName :
			if (current != 0)
				current = addAttribute(current, tok, tok.get(), tok.length());
			break;
		case xml::_pcdata :
		case xml::_cdata :
			if (current != 0)
			{
				char *pData = tok.get();
				__int64 nDataLen = tok.length();
				if (bEatWhite)
				{
					int i = 0;
					for(;i < nDataLen; i++)
					{
						if (
								(pData[i] != 10) &&
								(pData[i] != 9) &&
								(pData[i] != 8)
						   )
							break;
					}
					if (i == nDataLen)// if everything was whitespace
					{
						nDataLen = 0;
					}
				}

				if (current->getValueLen() && nDataLen)
					current->appendText(pData,nDataLen);
				else
					current->setValue(pData,nDataLen);
			}
			break;
		case xml::_version :
			break;
		case xml::_encoding :
			break;
		case xml::_standalone :
			break;
		case xml::_instruction :
			break;
		case xml::_comment :
			break;
		case xml::_entityName :
			break;
		case xml::_systemLiteral :
			break;
		case xml::_publicLiteral :
			break;
		case xml::_entityValue :
			break;
		}
	}
	while (tok.m_type != xml::_unknown);
}


// Prune out any repeating nodes 
void CXMLTree::PruneTree(CXMLElement *root/* = 0*/)
{
	/*	so that.....
	// Two customers each with different amount of orders
		
		<DocT>
			<Cust>
				<Order>
					....
				</Order>
				<Order>
					....
				</Order>
			<Cust>
			<Cust>
				<Order>
					....
				</Order>
				<Order>
					....
				</Order>
				<Order>
					....
				</Order>
			<Cust>
		</DocT>

	//	becomes...
		A template for customers that may contain orders
		<DocT>
			<Cust>
				<Order>
					....
				</Order>
			<Cust>
		</DocT>
	*/
	
	if (!root)
		root = m_root;

	
	if (root->getChildren())
	{
		// walk deeper every chance we get
		GListIterator itRecurseChildren(root->getChildren());
		while (itRecurseChildren())
		{
			PruneTree((CXMLElement *)itRecurseChildren++);
		}

		// on the way out make the first sibling, the only sibling.
REMOVE_NEXT:
		GListIterator itChildren(root->getChildren());
		CXMLElement *pFirstChild = 0;
		CXMLElement *pNextChild = 0;
		while (itChildren())
		{
			if (!pFirstChild)
				pFirstChild = (CXMLElement *)itChildren++;
			else
				pNextChild = (CXMLElement *)itChildren++;
			if (pNextChild)
			{
				GString strFirst(pFirstChild->m_tag, pFirstChild->m_nTagLen);
				GString strNext(pNextChild->m_tag, pNextChild->m_nTagLen);
				if ( strFirst.CompareNoCase(strNext) == 0 )
				{
					CXMLElement *parent = pFirstChild->getParent();
					if (parent)
					{
						parent->removeChild(pNextChild,true);
						goto REMOVE_NEXT;
					}
				}
			}
		}
	}
}
