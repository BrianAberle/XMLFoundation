// xml.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "xml.h"

#include "glist.h"

#include <fstream.h>

#include <afx.h>
/*
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
*/

#include <rw/cstring.h>

int main(int argc, char* argv[])
{
	RWCString strXml;
	fstream fs("C:\\src\\SAIC\\CDR\\Testing\\TestFiles\\ListHCPLabProceduresDetail.response", ios::in|ios::binary);
//	fstream fs("C:\\src\\SAIC\\CDR\\Testing\\TestFiles\\ListCodesResponse.xml", ios::in|ios::binary);
//	fstream fs("C:\\data.xml", ios::in|ios::binary);
	strXml.readFile(fs);
	fs.close();

	CGenericList paramtererEntities;
	CGenericList generalEntities;

	long length = strlen((const char *)strXml);
	char *xml = new char[length + 1];
	memcpy(xml, (const char *)strXml, length + 1);

	xml::token tok;

DWORD dwT = 0;
for (int i = 0; i < 20; i++)
{
	fs.open("c:\\data.txt", ios::out|ios::binary|ios::trunc);

	DWORD dwStart = GetTickCount();
	{
		xml::lex l(&paramtererEntities, 
				   &generalEntities, 
				   xml);
		try
		{
			do
			{
				l.tokenize(&tok);
/*
				switch (tok.m_type)
				{
				case xml::_version :
					fs << "version = ";
					fs.write(tok.get(), tok.length());
					fs << endl;
					break;
				case xml::_encoding :
					fs  << "encoding = ";
					fs.write(tok.get(), tok.length());
					fs << endl;
					break;
				case xml::_standalone :
					fs  << "stand alone = ";
					fs.write(tok.get(), tok.length());
					fs << endl;
					break;
				case xml::_instruction :
					fs << "instruction {";
					fs.write(tok.get(), tok.length());
					fs << "}"
					   << endl;
					break;
				case xml::_comment :
					fs  << "comment {";
					fs.write(tok.get(), tok.length());
					fs << "}"
						<< endl;
					break;
				case xml::_entityName :
					fs  << "entity name = ";
					fs.write(tok.get(), tok.length());
					fs << endl;
					continue;
				case xml::_systemLiteral :
					fs  << "system literal = ";
					fs.write(tok.get(), tok.length());
					fs << endl;
					break;
				case xml::_publicLiteral :
					fs  << "public literal = ";
					fs.write(tok.get(), tok.length());
					fs << endl;
					break;
				case xml::_entityValue :
					if (l.isParameterEntity())
						fs  << "parameter ";
					fs  << "entity value = ";
					fs.write(tok.get(), tok.length());
					fs << endl;
					continue;
				case xml::_startTag :
					fs  << "start tag = ";
					fs.write(tok.get(), tok.length());
					fs << endl;
					break;
				case xml::_endTag :
					fs  << "end tag = ";
					fs.write(tok.get(), tok.length());
					fs << endl;
					break;
				case xml::_attributeName :
					fs  << "attribute = ";
					fs.write(tok.get(), tok.length());
					fs << endl;
					break;
				case xml::_pcdata :
					fs  << "parsed c data = ";
					fs.write(tok.get(), tok.length());
					fs << endl;
					break;
				case xml::_cdata :
					fs  << "c data = ";
					fs.write(tok.get(), tok.length());
					fs << endl;
					break;
				}
*/
			}
			while (tok.m_type != xml::_unknown);
		}
		catch (const char *szErr)
		{
			cout << szErr << endl;
		}
		catch (xml::error &err)
		{
			fs  << "Error : ";
			if (err.err)
				fs << err.err;
			fs  << endl
				<< "Offset "
				<< err.offset
				<< " < "
				<< err.bytes
				<< " > "
				<< endl
				<< endl;
		}
	}
	DWORD dwEnd = GetTickCount();
	dwT += (dwEnd - dwStart);

	cout << "Start : " 
		 << dwStart
		 << endl
		 << "End : "
		 << dwEnd
		 << endl
		 << "Total : "
		 << dwEnd - dwStart
		 << endl;
}

cout << "Average : " << (dwT / 20) << endl;

	delete [] xml;

	return 0;
}
