#include "xmlTree.h"
#include "xmlAttribute.h"
#include "xmlElement.h"
#include "glist.h"
#include <sys/timeb.h>
#include <time.h>

#ifndef _WIN32
	#define _timeb timeb
	#define _ftime ftime
	int __argc;
	char** __argv;
#endif
// For portable timing in milliseconds - "ftime" is not ANSI C 
// but it looks like both MSVC and IBM C-Set support this function.
inline long getTimeMS()
{
	struct _timeb tb;
	_ftime(&tb);
   	return tb.time * 1000 + tb.millitm;
}




void dump(CGenericList *pAttributelist)

{
	if (pAttributelist)
	{
		CListIterator itAttrs(pAttributelist);
		while (!itAttrs)
		{
			CXMLAttribute *pAttribute = (CXMLAttribute *)itAttrs++;
			cout.write(pAttribute->m_tag, pAttribute->m_nTagLen);
			cout << "=";
			cout.write(pAttribute->m_value, pAttribute->m_nValueLen);
			cout << ' ';
		}
	}
}

void dump(CXMLElementx *root,  int *indent)
{
	for (int i = 0 ; i < *indent; i++)
		cout << '\t';

	if (root->m_nTagLen < 0)
		root->m_nTagLen = root->m_nTagLen;

	cout.write(root->m_tag, root->m_nTagLen);
	cout << "=";
	cout.write(root->m_value, root->m_nValueLen);
	cout << " { ";
	dump(root->getAttributes() );
	cout	<< "}" << endl;
	(*indent)++;

	if (root->getChildren())
	{
		CListIterator itChildren(root->getChildren());
		while (!itChildren)
		{
			dump((CXMLElementx *)itChildren++, indent);
		}
	}
	(*indent)--;
}



void main()
{
	fstream fs("ListHCPLabProceduresDetail.response", ios::in);
	fs.seekg(0,ios::end);
	int nSize = fs.tellg();
	fs.seekg(0,ios::beg);
	char *xmlBuf = new char[nSize + 1];
	memset(xmlBuf,0,nSize + 1);
	fs.read(xmlBuf,nSize);
	CXMLTree tree;

	// 940 milliseconds to Parse and build a tree for 
	// a 4.82 Megabyte XML file in a PII450 in Windows NT.
	long startTime = getTimeMS();
	tree.parseXML( xmlBuf );
	cout << "Time in milliseconds to parse XML into the tree: " << getTimeMS() - startTime;

	int nIndent = 0;
//	dump(tree.getRoot(), &nIndent);
	delete xmlBuf;
}
