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
#include "XMLFoundation.h"


void dump(CXMLElementx *node, int *indent)
{
	printf("\n");
	for (int i = 0 ; i < *indent; i++)
		printf("  ");


	// write out this Element
	GString strTag(node->getTag(), node->getTagLen());
	printf((const char *)strTag);

	GString str(node->getValue(),node->getValueLen());
	str.TrimLeftWS();
	if (str.Length())
	{
		printf("=%s",(const char *)str); 
	}
	

	// and it's attributes, if there are any
	if (node->getAttributes())
	{
		printf(" { ");
		GListIterator itAttrs(node->getAttributes());
		while (itAttrs())
		{
			CXMLAttribute *pAttribute = (CXMLAttribute *)itAttrs++;
			GString strAttTag(pAttribute->m_tag, pAttribute->m_nTagLen);
			printf((const char *)strAttTag);
			printf("=");
			GString strAttVal(pAttribute->m_value, pAttribute->m_nValueLen);
			printf((const char *)strAttVal);
			printf(" ");
		}
		printf ("}");
	}


	// and do the same for the children of this node
	(*indent)++;
	if (node->getChildren())
	{
		GListIterator itChildren(node->getChildren());
		while (itChildren())
		{
			dump((CXMLElementx *)itChildren++, indent);
		}
	}
	(*indent)--;
}

// optional, implement this for external DTD
void __cdecl CallbackDTD(xml::lex *lex, xml::token *tok)
{
	static GString strError;

	GString strDTD;
	strDTD.write(tok->get(), tok->length());
	strDTD << '\0';

	GString strExt;
	strExt.FromFile(strDTD);

	xml::lex lexDTD(lex->getPEs(), 
					lex->getGEs(), 
					(char *)(const char *)strExt);
	lexDTD.setStateDTD(*lex);
	do
	{
		lexDTD.nextToken(tok);
	}
	while (tok->m_type != xml::_unknown);
}

int main(int argc, char* argv[])
{
	try
	{
		CXMLTree tree;
		tree.setCallbackDTD(CallbackDTD);

		GString strXML;
		strXML.FromFile("LexDemo1.xml");

//		long startTime = getTimeMS();
		tree.parseXML((char *)(const char *)strXML);
//		cout << "Time in milliseconds to parse XML into the tree: " << getTimeMS() - startTime;

		int indent = 0;
		CXMLElementx *root = tree.getRoot();
		if (root)
			dump(root, &indent);
	}
	catch( GException &rErr)
	{
		printf(rErr.GetDescription());
	}
	return 0;
}

