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


// Fast, portable, non-validating, XML Lexical Analyzer 
#include "GlobalInclude.h"
static char SOURCE_FILE[] = __FILE__;

#include "xmlLex.h"
#include <string.h> // for: strlen(), memcpy(), memcmp(), strcmp(), memset()
#include <stdlib.h> // for: atoi()
#include <ctype.h>  // for: isdigit()


typedef struct
{
	const char * tag;
	const char * value;
} EntityPair;

EntityPair SysEntities [] =
{
	{ "lt",   "<"  },
	{ "gt",   ">"  },
	{ "amp",  "&"  },
	{ "apos", "'"  },
	{ "quot", "\"" },
};

int hexToBase10(const char *szBase16)
{
	int nBase10 = 0;
	int nDigit;
	int nLen = (int)strlen(szBase16);

	for (int i = 0; i < nLen; i++)
	{   
		if (isdigit(szBase16[i]))
			nDigit = szBase16[i] - '0';
		else if ((szBase16[i] >= 'A') && (szBase16[i] <= 'F'))
			nDigit = szBase16[i] - 'A' + 10;
		else if ((szBase16[i] >= 'a') && (szBase16[i] <= 'f'))
			nDigit = szBase16[i] - 'a' + 10;
		else if ((szBase16[i] == 'x') || (szBase16[i] == 'X'))
			continue;
		else
			break;

		nBase10 = nBase10 *	16 + nDigit;
	}
	
	return nBase10;
}

char convertCharacterRef(const char * szXML)
{
	int nChar = atoi(szXML);
	if ((*szXML == 'x') || (*szXML == 'X'))
		nChar = hexToBase10(szXML);

	return (char)nChar;
}

// lex parsing MACROS
#define isWhitespace(ch) ( ch == 0x20 || ch == 0x09 || ch == 0x0D || ch == 0x0A)
#define isLetter(ch) ((ch >= 0x41 && ch <= 0x5A) || (ch >= 0x61 && ch <= 0x7A))
#define isDigit(ch) (ch >= 0x30 && ch <= 0x39)
#define isNameChar(ch)	(ch >= 0x41 && ch <= 0x5A) || (ch >= 0x61 && ch <= 0x7A) ||	(ch >= 0x30 && ch <= 0x39) ||	ch == '.' ||	ch == '-' ||	ch == '_' ||	ch == ':'
#define qappend(token,ptr,copy){	if ((copy == false) && (token->m_heap == false))	{		if (token->m_gp == 0)			token->m_gp = (char *)ptr;			token->m_len++;		}	else		{		token->append(ptr,copy);	}}
#define isNewLine() { if ((m_streamStack.m_nNext == 1) && (m_s->m_xml[m_s->m_offset] == 0x0D)) { m_line++; m_byte = 0; } }
#define offsetLine() { if (m_streamStack.m_nNext == 1) m_byte++; }

/*

State tables w/states:
	Document table

		Prolog table
			XML declaration table
				['<?xml']
					Version table
						[white space]
						[tag]
						Equal table
							[white space (zero or one)]
							['=']
							[white space (zero or one)]
						[quote]
						[value]
						[quote]
					Encoding table (zero or one)
						[white space (zero or one)]
						[tag]
						Equal table
							[white space (zero or one)]
							['=']
							[white space (zero or one)]
						[quote]
						[value]
						[quote]
					Standalone table (zero or one)
						[white space  (zero or one)]
						[tag]
						Equal table
							[white space (zero or one)]
							['=']
							[white space (zero or one)]
						[quote]
						[value]
						[quote]
				['?>']
			Misc.
				[white space] (zero or one)
				Processing instruction table
					['<?']
					[instruction]
					['?>']
				Comment table
					['<!--']
					[comment]
					['-->']
				(repeat while '<?' or '<!--')
			Document type table (DTD)
				['<!DOCTYPE']
					[name]
					[external id (zero or one)]
					['[' (zero or one)]
						Markup table
							Element declaration table
								['<!ELEMENT']
								[element declaration]
								['>'] <-- goto repeat
							Attribute list declaration table
								['<!ATTLIST']
								[attribute list declaration]
								['>'] <-- goto repeat
							Entity declaration table
								['<!ENTITY']
								[white space]
								['%' (zero or one)] <-- if true [white space]
								[entity name]
								[white space]
								[quote]
								[entity value]
								[quote]
								[white space (zero or one)]
								['>'] <-- goto repeat
							Notation declaration table
								['<!NOTATION']
								[notation declaration]
								['>'] <-- goto repeat
							Processing instruction table
								['<?']
								[instruction]
								['?>'] <-- goto repeat
							Comment table
								['<!--']
								[comment]
								['-->'] <-- goto repeat
							[Entity reference '%' (zero or one)]
					(repeat until ']')
				Misc.
					[white space] (zero or one)
					Processing instruction table
						['<?']
						[instruction]
						['?>']
					Comment table
						['<!--']
						[comment]
						['-->']
					(repeat while '<?' or '<!--')

		Element table (content)
			[parsed character data]
			[character data]
			Processing instruction table
			Comment table
			Start tag table
				['<']
				[name]
				attribute table
					[white space]
					[name]
					Equal table
						[white space (zero or one)]
						['=']
						[white space (zero or one)]
					[quote]
					[value]
					[quote]
				(repeat until '/' or '>')
				['/>'] <-- skip element end table
				['>']  <-- do element end table?
			Element end table
				[white space (zero or one]
				['</']
				[name]
				[white space (zero or one)]
				['>']
			[white space (zero or one)]
			(repeat while tag stack not empty)
		
		Misc.
			[white space] (zero or one)
			Processing instruction table
				['<?']
				[instruction]
				['?>']
			Comment table
				['<!--']
				[comment]
				['-->']
			(repeat while '<?' or '<!--')

*/

//////////////////////////////////////////////////////////////////////////////
// lex

xml::lex::lex(GList *parameterEntities,
			  GList *generalEntities,
			  char *stream) :
	m_streamStack(20),
	m_tagStack(50),
	m_documentType(0),
	m_parameterEntities(parameterEntities),
	m_generalEntities(generalEntities),
	m_line(1),
	m_byte(1),
	m_quote(0),
	m_standalone(true),
	m_lpfnExternalDTD(0),
	m_postmarkup(true)
{
	m_s = new Stream;
	m_s->m_xml = stream;
	m_s->m_offset = 0;
	m_s->m_delete = false;
	GStackPush(m_streamStack,m_s);
	m_streams.AddLast(m_s);

	m_state = _prolog;
}

void xml::lex::setStream(char *stream)
{
	if (m_s->m_xml == 0)
		m_s->m_xml = stream;
}

void xml::lex::setStateDTD(const lex &l)
{
	m_state = _xml_dtd;
	m_nextState = l.m_nextState;
	m_sstate = _xml_markup;
	m_postmarkup = false;
}

xml::lex::~lex()
{
	delete [] m_documentType;
	GListIterator itStreams(&m_streams);
	while (itStreams())
	{
		Stream *s = (Stream *)itStreams++;
		if (s->m_delete != false)
			delete s->m_xml;
		delete s;
	}

//	GListIterator itHeapedTokens(&m_heapedTokens);
//	while (itHeapedTokens())
//	{
//		delete (char *)itHeapedTokens++;
//	}
}

void xml::lex::setCallbackDTD(lpCallback lpFn)
{
	m_lpfnExternalDTD = lpFn;
}

bool xml::lex::isParameterEntity()
{
	return m_parameterEntity;
}

void xml::lex::getEqual()
{
	static const char* s_equal = "=";

	handleWhitespace();

	if (handleReserved(s_equal) == false)
	{
		throw GException("XML Parser", 2, m_line, m_byte);
	}

	handleWhitespace();
}

void xml::lex::getQuote()
{
	if (m_s != 0)
	{
		char ch = m_s->m_xml[m_s->m_offset];
		m_s->m_offset++;
		offsetLine();

		if ((ch == '"') || (ch == '\''))
		{
			if (m_quote == 0)
				m_quote = ch;
			else if ((m_quote != 0) && (ch == m_quote))
				m_quote = 0;
			else
			{
				throw GException("XML Parser", 6, m_line, m_byte);
			}
		}
		else
		{
			throw GException("XML Parser", 6, m_line, m_byte);
		}
	}
}

void xml::lex::getXMLDecl(xml::token *tok)
{
	static const char* s_version = "version";
	static const char* s_encoding = "encoding";
	static const char* s_standalone = "standalone";
	static const char* s_endXMLDecl = "?>";

xmlDecl:;

	switch (m_sstate)
	{
		case _xml_decl_version :
			if (handleWhitespace() == false)
			{
				throw GException("XML Parser", 20, m_line, m_byte);
			}
			if (handleReserved(s_version) == false)
			{
				throw GException("XML Parser", 13, m_line, m_byte);
			}
			getEqual();
			getQuote();
			tok->m_type = xml::_version;
			if (getVersion(tok) == false)
			{
				throw GException("XML Parser", 13, m_line, m_byte);
			}
			getQuote();
			m_sstate = xml::_xml_decl_encoding;
			break;
		case _xml_decl_encoding :
			handleWhitespace();
			if (handleReserved(s_encoding) != false)
			{
				getEqual();
				getQuote();
				tok->m_type = xml::_encoding;
				if (getEncoding(tok) == false)
				{
					throw GException("XML Parser", 14, m_line, m_byte);
				}
				getQuote();
			}
			else
			{
				m_sstate = xml::_xml_decl_standalone;
				goto xmlDecl;
			}
			m_sstate = xml::_xml_decl_standalone;
			break;
		case _xml_decl_standalone :
			handleWhitespace();
			if (handleReserved(s_standalone) != false)
			{
				getEqual();
				getQuote();
				tok->m_type = xml::_standalone;
				if (getStandalone(tok) == false)
				{
					throw GException("XML Parser", 15, m_line, m_byte);
				}
				getQuote();
			}
			handleWhitespace();
			if (handleReserved(s_endXMLDecl) == false)
			{
				throw GException("XML Parser", 3, m_line, m_byte);
			}
			m_state = xml::_misc;
	}
}

void xml::lex::getMisc(xml::token *tok)
{
	static const char* s_startPI = "<?";
	static const char* s_endPI = "?>";
	static const char* s_startComment = "<!--";
	static const char* s_endComment = "-->";

	handleWhitespace();

	if (handleReserved(s_startPI) != false)
	{
		tok->m_type = xml::_instruction;
		getInstruction(tok);
		if (handleReserved(s_endPI) == false)
		{
			throw GException("XML Parser", 4, m_line, m_byte);
		}
	}
	else if (handleReserved(s_startComment) != false)
	{
		tok->m_type = xml::_comment;
		getComment(tok);
		if (handleReserved(s_endComment) == false)
		{
			throw GException("XML Parser", 5, m_line, m_byte);
		}
	}
	else
	{
		m_state = m_nextState;
	}
}

void xml::lex::loadExternal(lpCallback lpFn, token *tok)
{
	if (!lpFn)
		throw GException("XML Parser", 26, m_line, m_byte);

	lpFn(this, tok);
}

void xml::lex::getDTD(xml::token *tok)
{
	static const char* s_system = "SYSTEM";
	static const char* s_public = "PUBLIC";
	static const char* s_markupDeclStart = "[";
	static const char* s_markupDeclEnd = "]";
	static const char* s_entityDecl = "<!ENTITY";
	static const char* s_entityEnd = ">";
	static const char* s_parsedEntityRef = "%";

dtdTop:;

	switch (m_sstate)
	{
		case _xml_dtd_name :
			if (handleWhitespace() == false)
			{
				throw GException("XML Parser", 20, m_line, m_byte);
			}
			if (getName(tok) == false)
			{
				throw GException("XML Parser", 0, m_line, m_byte);
			}

			m_documentType = new char[tok->length() + 1];
			memcpy(m_documentType, tok->get(), tok->length());
			m_documentType[tok->length()] = 0;
			tok->release();

			handleWhitespace();

			// check for external identifiers
			if (handleReserved(s_system) != false)
				m_sstate = _xml_system_literal;
			else if (handleReserved(s_public) != false)
				m_sstate = _xml_public_literal;
			else
				m_sstate = _xml_pre_markup;

			goto dtdTop;
		case _xml_system_literal :
			if (handleWhitespace() == false)
			{
				throw GException("XML Parser", 20, m_line, m_byte);
			}
			getQuote();
			tok->m_type = _systemLiteral;
			getLiteral(tok);
			getQuote();

			// load the external DTD using the
			// DTD callback function
			if (m_state == xml::_xml_dtd)
				loadExternal(m_lpfnExternalDTD, tok);

			m_sstate = _xml_pre_markup;
			break;
		case _xml_public_literal :
			if (handleWhitespace() == false)
			{
				throw GException("XML Parser", 20, m_line, m_byte);
			}
			getQuote();
			tok->m_type = _publicLiteral;
			getLiteral(tok);
			getQuote();
			m_sstate = _xml_system_literal;
			break;
		case _xml_pre_markup :
			handleWhitespace();
			if (handleReserved(s_markupDeclStart) != false)
			{
				m_sstate = _xml_markup;
				goto dtdTop;
			}
			m_sstate = _xml_post_markup;
			break;
		case _xml_markup :
			handleWhitespace();

			if (handleReserved(s_markupDeclEnd) != false)
			{
				m_sstate = _xml_post_markup;
				goto dtdTop;
			}

			getMisc(tok);

			// look for elements, attribute lists, entities, and notation
			if (tok->m_type == xml::_unknown)
			{
				if (handleReserved(s_entityDecl) != false)
				{
					m_parameterEntity = false;
					if (handleWhitespace() == false)
					{
						throw GException("XML Parser", 20, m_line, m_byte);
					}
					// declaration of a parameter entity
					if (handleReserved(s_parsedEntityRef) != false)
					{
						m_parameterEntity = true;
						if (handleWhitespace() == false)
						{
							throw GException("XML Parser", 20, m_line, m_byte);
						}
					}

					tok->m_type = _entityName;
					if (getName(tok) == false)
					{
						throw GException("XML Parser", 11, m_line, m_byte);
					}

					xml::entity *p = new xml::entity();
					p->setTag(tok);
					if (m_parameterEntity == false)
					{
						if (m_generalEntities != 0)
							m_generalEntities->AddLast(p);
					}
					else
					{
						if (m_parameterEntities != 0)
							m_parameterEntities->AddLast(p);
					}

					m_sstate = _xml_entity;
				}
				else if (handleReserved(s_parsedEntityRef) != false)
				{
					handleEntityReference();
					goto dtdTop;
				}
			}

			break;

		case _xml_entity :
			if (handleWhitespace() == false)
			{
				throw GException("XML Parser", 20, m_line, m_byte);
			}

			getQuote();
			tok->m_type = xml::_entityValue;
			if (getEntity(tok) != false)
			{
				xml::entity *p = 0;
				if (m_parameterEntity == false)
				{
					if (m_generalEntities != 0)
						p = (xml::entity *)m_generalEntities->Last();
				}
				else
				{
					if (m_parameterEntities != 0)
						p = (xml::entity *)m_parameterEntities->Last();
				}
				p->setValue(tok);
			}
			else
			{
				throw GException("XML Parser", 9, m_line, m_byte);
			}
			getQuote();

			m_sstate = _xml_entity_end;
			break;

		case _xml_entity_end :
			handleWhitespace();

			if (handleReserved(s_entityEnd) == false)
			{
				throw GException("XML Parser", 7, m_line, m_byte);
			}

			m_sstate = _xml_markup;
			goto dtdTop;

		case _xml_post_markup :
			handleWhitespace();
			if ((handleReserved(s_entityEnd) == false) &&
				(m_postmarkup != false))
			{
				throw GException("XML Parser", 1, m_line, m_byte);
			}

			m_state = _misc;
			m_nextState = _content;
			m_sstate = _xml_element_start;
	}

	if ((m_state == _xml_dtd) && (tok->m_type == xml::_unknown))
	{
		m_sstate = _xml_post_markup;
		goto dtdTop;
	}
}

void xml::lex::getContent(xml::token *tok)
{
	static const char* s_emptyElement = "/>";
	static const char* s_endElement = "</";
	static const char* s_elementEnd = ">";
	static const char* s_cdataStart = "<![CDATA[";
	int nEndlessLoopDetection = 0;
	if (m_s != 0)
	{

contentTop:;
		if (nEndlessLoopDetection++ > 1000)
		{
			// When parsing a XML with an invalid element <@CustomerID>
			// 1000 is well beyond any normal behavior of stream stacks.
			throw GException("XML Parser", 27, m_line, m_byte);
		}
		switch (m_sstate)
		{
			case xml::_xml_element_start :
				if (!m_s)
					throw GException("XML Parser", 28, m_line, m_byte);

				if ((m_s->m_xml[m_s->m_offset] == '<') && 
					(isNameChar(m_s->m_xml[m_s->m_offset + 1])))
				{
					m_s->m_offset++;
					offsetLine();
					tok->m_type = _startTag;
					if (getName(tok) != false)
					{
						GStackPush(m_tagStack,tok->get());

						m_sstate = _xml_element_attribute;

						if ((m_tagStack.m_nNext == 1) &&
							(m_documentType != 0))
						{
							if (memcmp(m_documentType, tok->get(), tok->length()) != 0)
							{
								throw GException("XML Parser", 16, m_line, m_byte);
							}
						}
					}
					else
					{
						throw GException("XML Parser", 11, m_line, m_byte);
					}
				}
				else
				{
					// make sure that the next tag isn't a >
					if (m_s->m_xml[m_s->m_offset + 1] == *s_elementEnd)
						throw GException("XML Parser", 11, m_line, m_byte);
		
					m_sstate = _xml_element_content;
					goto contentTop;
				}

				break;

			case xml::_xml_element_attribute :
				if (handleWhitespace() != false)
				{
					if (getName(tok) != false)
					{
						tok->m_type = _attributeName;
						m_sstate = _xml_element_attribute_value;
					}
					else
					{
						m_sstate = _xml_element_start_end;
						goto contentTop;
					}
				}
				else
				{
					m_sstate = _xml_element_start_end;
					goto contentTop;
				}
				break;

			case xml::_xml_element_start_end :
				if (handleReserved(s_emptyElement) != false)
				{
					// return an empty end tag token
					tok->m_type  = _emptyEndTag;
					m_sstate = _xml_element_start;
				}
				else if (handleReserved(s_elementEnd) != false)
				{
					m_sstate = _xml_element_content;
					goto contentTop;
				}
				else
				{
					throw GException("XML Parser", 8, m_line, m_byte);
				}

				break;

			case xml::_xml_element_attribute_value :
				getEqual();
				getQuote();
				tok->m_type = _pcdata;
				getAttribute(tok);
				getQuote();
				m_sstate = _xml_element_attribute;

				break;

			case xml::_xml_element_content :
				if (getCharacterData(tok) != false)
					tok->m_type = _pcdata;
				else if (handleReserved(s_cdataStart) != false)
				{
					getCData(tok);
					tok->m_type = _cdata;
				}
				else
				{
					getMisc(tok);

					if (tok->m_type == _unknown)
					{
						m_sstate = _xml_element_end_tag;
						goto contentTop;
					}
				}
				break;

			case xml::_xml_element_end_tag :
				handleWhitespace();
				if (handleReserved(s_endElement) != false)
				{
					tok->m_type = _endTag;
					getName(tok);

					m_sstate = _xml_element_end_end;
				}
				else if (m_tagStack.m_nNext)
				{
					m_sstate = _xml_element_start;
					goto contentTop;
				}
				break;

			case xml::_xml_element_end_end :
				handleWhitespace();
				if (handleReserved(s_elementEnd) == false)
				{
					throw GException("XML Parser", 8, m_line, m_byte);
				}

				m_sstate = _xml_element_start;
				goto contentTop;

				break;
		}
	}

	if ((tok->m_type == _endTag) || 
		(tok->m_type == _emptyEndTag))
	{
		// throw an exception if the end tag
		// doesn't match the last start tag
		GStackPopType(m_tagStack,tag,char *)
		if ( (tok->isEmpty() == false) && (tag != 0) )
		{
			if (memcmp(tag, tok->get(), tok->length()) != 0)
			{
				GString strStartTag;
				while ((tag) && (*tag != '>') && (!isWhitespace(*tag)))
				{
					strStartTag += *tag;
					tag++;
				}
				GString strEndTag(tok->get(), tok->length());
				throw GException("XML Parser", 12, (const char *)strEndTag, (const char *)strStartTag,m_line, m_byte);
			}
		}

		if (m_tagStack.m_nNext == 0)
		{
			m_state = xml::_finished;
			m_nextState = xml::_finished;

			if ((tok->m_type == _endTag) && (handleReserved(s_elementEnd) == false))
			{
				throw GException("XML Parser", 8, m_line, m_byte);
			}
		}
	}
}

void xml::lex::getToken(xml::token *tok)
{
	static const char* s_decl = "<?xml";
	static const char* s_doctype = "<!DOCTYPE";

tokenTop:;

	switch (m_state)
	{
		case xml::_prolog :
			if (handleReserved(s_decl) != false)
			{
				m_state = xml::_xml_decl;
				m_sstate = xml::_xml_decl_version;
			}
			else
			{
				m_state = xml::_misc;
			}
			m_nextState = xml::_xml_pre_dtd;
			goto tokenTop;
		case xml::_xml_decl :
			getXMLDecl(tok);
			if ((tok->m_type == _unknown) && (m_state != xml::_xml_decl))
				goto tokenTop;
			break;
		case xml::_misc :
			getMisc(tok);
			if (m_state != xml::_misc)
				goto tokenTop;
			break;
		case xml::_xml_pre_dtd :
			if (handleReserved(s_doctype) != false)
			{
				// parse the DTD
				m_state = xml::_xml_dtd;
				m_nextState = xml::_xml_dtd;
				m_sstate = xml::_xml_dtd_name;
			}
			else
			{
				m_state = xml::_content;
				m_sstate = _xml_element_start;
				m_nextState = m_state;
			}
			goto tokenTop;
		case xml::_xml_dtd :
			getDTD(tok);
			if (m_state != xml::_xml_dtd)
				goto tokenTop;
			break;
		case xml::_content :
			getContent(tok);
			break;
	}
}

bool xml::lex::getAttribute(xml::token *tok)
{
	if (m_s != 0)
	{
		while (m_s->m_xml[m_s->m_offset] != m_quote)
		{
			isNewLine();
			if (m_s->m_xml[m_s->m_offset] != '&')
			{
				qappend(tok,&m_s->m_xml[m_s->m_offset],false);
				m_s->m_offset++;
				offsetLine();
			}
			// expand character references, entities
			else
			{
				m_s->m_offset++;
				offsetLine();
				// expand the charcter reference
				if (m_s->m_xml[m_s->m_offset] == '#')
				{
					m_s->m_offset++;
					offsetLine();

					GString os;
					while (m_s->m_xml[m_s->m_offset] != ';')
					{
						isNewLine();
						os << m_s->m_xml[m_s->m_offset];
						m_s->m_offset++;
						offsetLine();
					}
					m_s->m_offset++;
					offsetLine();

					tok->append(convertCharacterRef((const char *)os), true);
				}
				else
				{
					bool found = false;

					// build the entity tag
					GString os;
					while (m_s->m_xml[m_s->m_offset] != ';')
					{
						isNewLine();
						os << m_s->m_xml[m_s->m_offset];
						m_s->m_offset++;
						offsetLine();
					}
					m_s->m_offset++;
					offsetLine();
					char *paramEntity = (char *)(const char *)os;

					// check the entity against the system entities
					for (int i = 0; i < sizeof(SysEntities) / sizeof(EntityPair); i++)
					{
						if (strcmp(paramEntity, SysEntities[i].tag) == 0)
						{
							qappend(tok, SysEntities[i].value, true);
							found = true;
						}
					}

					if ((m_generalEntities != 0) && (found == false))
					{
						GListIterator it(m_generalEntities);

						// find the entity and append it to the value.
						entity *pEntity;
						while ((found == false) && (it()))
						{
							pEntity = (entity *)it++;
							if (strcmp(paramEntity, pEntity->getTag()) == 0)
							{
								qappend(tok,pEntity->getValue(), true);
								found = true;
							}
						}

					}

					// throw an exception if the entity was not found
					if (found == false)
					{
						m_strTempError = paramEntity;
						throw GException("XML Parser", 21, (const char *)m_strTempError, m_line, m_byte);
					}
				}
			}
		}

		return (m_s->m_xml[m_s->m_offset] == m_quote);
	}

	return false;
}

bool xml::lex::getEntity(xml::token *tok)
{
	if (m_s != 0)
	{
		while (m_s->m_xml[m_s->m_offset] != m_quote)
		{
			isNewLine();
			if ((m_s->m_xml[m_s->m_offset] != '&') && 
				(m_s->m_xml[m_s->m_offset] != '%'))
			{
				qappend(tok,&m_s->m_xml[m_s->m_offset], false);
				m_s->m_offset++;
				offsetLine();
			}
			// expand all character references
			// and parameter entities
			else 
			{
				switch (m_s->m_xml[m_s->m_offset])
				{
				case '&' :
					// expand the charcter reference
					if (m_s->m_xml[m_s->m_offset + 1] == '#')
					{
						m_s->m_offset += 2;
						GString os;

						while (m_s->m_xml[m_s->m_offset] != ';')
						{
							isNewLine();
							os << m_s->m_xml[m_s->m_offset];
							m_s->m_offset++;
							offsetLine();
						}
						m_s->m_offset++;
						offsetLine();

						tok->append(convertCharacterRef(os), true);
					}
					else
					{
						qappend(tok,&m_s->m_xml[m_s->m_offset], false);
						m_s->m_offset++;
						offsetLine();
					}
					break;
				case '%' :
					{
						m_s->m_offset++;
						offsetLine();
						bool found = false;

						// append the parameter entity
						GString os;

						while (m_s->m_xml[m_s->m_offset] != ';')
						{
							isNewLine();
							os << m_s->m_xml[m_s->m_offset];
							m_s->m_offset++;
							offsetLine();
						}
						m_s->m_offset++;
						offsetLine();

						char *paramEntity = (char *)(const char *)os;
						// find the parameter entity
						// and append it to the value.
						if (m_parameterEntities != 0)
						{
							GListIterator it(m_parameterEntities);
							entity *pEntity;
							while ((found == false) && (it()))
							{
								pEntity = (entity *)it++;
								if (strcmp(paramEntity, pEntity->getTag()) == 0)
								{
									qappend(tok,pEntity->getValue(), true);
									found = true;
								}
							}
						}

						// throw an exception if the entity was not found
						if (found == false)
						{
							m_strTempError = paramEntity;
							throw GException("XML Parser", 21, (const char *)m_strTempError, m_line, m_byte);
						}
					}
					break;
				}
			}

			if (m_s->m_xml[m_s->m_offset] == 0)
			{
				GStackPop(m_streamStack);
				GStackTopType(m_streamStack,m_s,Stream *);

			}
		}

		if (m_s->m_xml[m_s->m_offset] == 0)
		{
			GStackPop(m_streamStack);
			GStackTopType(m_streamStack,m_s,Stream *);
		}
	}

	return (tok->isEmpty() == false);
}

bool xml::lex::getComment(xml::token *tok)
{
	static const char* s_commentEnd = "-->";
	bool ret = false;
	if (m_s != 0)
	{
		do
		{
			isNewLine();
			qappend(tok,&m_s->m_xml[m_s->m_offset], false);
			m_s->m_offset++;
			offsetLine();
			if (m_s->m_xml[m_s->m_offset] == *s_commentEnd)
				ret = handleReserved(s_commentEnd, false);
		}
		while (ret == false);
	}

	return (tok->isEmpty() == false);
}

bool xml::lex::getInstruction(xml::token *tok)
{
	// an exception will be thrown if
	// eof is reached before finding
	// the PI end.
	if (m_s != 0)
	{
		while ((m_s->m_xml[m_s->m_offset] != '?') && 
			   (m_s->m_xml[m_s->m_offset + 1] != '>'))
		{
			isNewLine();
			qappend(tok,&m_s->m_xml[m_s->m_offset], false);
			m_s->m_offset++;
			offsetLine();
		}
	}

	return (tok->isEmpty() == false);
}

bool xml::lex::getStandalone(xml::token *tok)
{
	static const char * s_no = "no";
	static const char * s_yes = "yes";

	if (handleReserved(s_no))
	{
		m_standalone = false;
		tok->append(s_no, true);
	}
	else if (handleReserved(s_yes))
	{
		m_standalone = true;
		tok->append(s_yes, true);
	}

	return (tok->isEmpty() == false);
}

bool xml::lex::getName(xml::token *tok)
{
	if (m_s != 0)
	{
		if (isLetter(m_s->m_xml[m_s->m_offset]))
		{
			while (isNameChar(m_s->m_xml[m_s->m_offset]))
			{
				qappend(tok,&m_s->m_xml[m_s->m_offset], false);
				m_s->m_offset++;
				offsetLine();
			}
		}
	}

	return (tok->isEmpty() == false);
}

bool xml::lex::getCData(xml::token *tok)
{
	static const char* s_cdataEnd = "]]>";
	bool ret = false;

	if (m_s != 0)
	{
		do
		{
			isNewLine();
			qappend(tok,&m_s->m_xml[m_s->m_offset], false);
			m_s->m_offset++;
			offsetLine();
			if (m_s->m_xml[m_s->m_offset] == *s_cdataEnd)
				ret = handleReserved(s_cdataEnd);
		}
		while (ret == false);
	}

	return (tok->isEmpty() == false);
}

void xml::lex::handleEntityReference()
{
	bool found = false;

	GString os;

	char *paramEntity = 0;

	// find the parameter entity and add it to the parse stream
	if (m_s != 0)
	{
		// append the parameter entity
		while (m_s->m_xml[m_s->m_offset] != ';')
		{
			isNewLine();
			os << m_s->m_xml[m_s->m_offset];
			m_s->m_offset++;
			offsetLine();
		}
		m_s->m_offset++;
		offsetLine();

		paramEntity = (char *)(const char *)os;
		if (m_parameterEntities != 0)
		{
			GListIterator it(m_parameterEntities);
			entity *pEntity;
			while ( (found == false) && (it()) )
			{
				pEntity = (entity *)it++;
				if (strcmp(paramEntity, pEntity->getTag()) == 0)
				{
					// add the entity to the parse stack
					__int64 length = strlen(pEntity->getValue());

					m_s = new Stream;
					m_s->m_xml = new char[length + 1];;
					memcpy(m_s->m_xml, pEntity->getValue(), length + 1);
					m_s->m_offset = 0;
					m_s->m_delete = true;
					GStackPush(m_streamStack,m_s);
					m_streams.AddLast(m_s);
					found = true;
				}
			}
		}
	}

	// throw an exception if the entity was not found
	if (found == false)
	{
		m_strTempError = paramEntity;
		throw GException("XML Parser", 21, (const char *)m_strTempError, m_line, m_byte);
	}
}

inline bool xml::lex::getCharacterData(xml::token *tok)
{
	if (m_s != 0)
	{
		while ((m_s != 0) &&
			   (m_s->m_xml[m_s->m_offset] != 0) && 
			   (m_s->m_xml[m_s->m_offset] != '<'))
		{
			isNewLine();
			if (m_s->m_xml[m_s->m_offset] != '&')
			{
				qappend(tok,&m_s->m_xml[m_s->m_offset], false);
				m_s->m_offset++;
				offsetLine();
			}
			else
			{
				switch (m_s->m_xml[m_s->m_offset])
				{
				case '&' :
					// expand the charcter reference
					m_s->m_offset++;
					offsetLine();
					if (m_s->m_xml[m_s->m_offset] == '#')
					{
						m_s->m_offset++;
						offsetLine();
						GString os;

						while (m_s->m_xml[m_s->m_offset] != ';')
						{
							isNewLine();
							os << m_s->m_xml[m_s->m_offset];
							m_s->m_offset++;
							offsetLine();
						}
						m_s->m_offset++;
						offsetLine();
						tok->append(convertCharacterRef(os), true);
					}
					else
					{
						bool found = false;

						// append the parameter entity
						GString os;

						while (m_s->m_xml[m_s->m_offset] != ';')
						{
							isNewLine();
							if (m_s->m_xml[m_s->m_offset] == ' ')
								throw GException("XML Parser", 25, m_line, m_byte);

							os << m_s->m_xml[m_s->m_offset];
							m_s->m_offset++;
							offsetLine();
						}
						m_s->m_offset++;
						offsetLine();

						char *paramEntity = (char *)(const char *)os;
						// check the entity against the system entities
						for (int i = 0; i < sizeof(SysEntities) / sizeof(EntityPair); i++)
						{
							if (strcmp(paramEntity, SysEntities[i].tag) == 0)
							{
								qappend(tok, SysEntities[i].value, true);
								found = true;
							}
						}

						// find the parameter entity
						// and append it to the value.
						if ((m_generalEntities != 0) && (found == false))
						{
							GListIterator it(m_generalEntities);
							entity *pEntity;
							while ( (found == false) && (it()) )
							{
								pEntity = (entity *)it++;
								if (strcmp(paramEntity, pEntity->getTag()) == 0)
								{
									// add the entity to the parse stack
									__int64 length = strlen(pEntity->getValue());

									m_s = new Stream;
									m_s->m_xml = new char[length + 1];;
									memcpy(m_s->m_xml, pEntity->getValue(), length + 1);
									m_s->m_offset = 0;
									m_s->m_delete = true;
									GStackPush(m_streamStack,m_s);
									m_streams.AddLast(m_s);

									tok->toHeap();

									found = true;
								}
							}
						}

						// throw an exception if the entity was not found
						if (found == false)
						{
							m_strTempError = paramEntity;
							throw GException("XML Parser", 21, (const char *)m_strTempError, m_line, m_byte);
						}
					}
					break;
				}
			}

			if (m_s->m_xml[m_s->m_offset] == 0)
			{
				GStackPop(m_streamStack);
				GStackTopType(m_streamStack,m_s,Stream *);

			}
		}
		if ((m_s != 0) && (m_s->m_xml[m_s->m_offset] == 0))
		{
			GStackPop(m_streamStack);
			GStackTopType(m_streamStack,m_s,Stream *);

		}
	}

	return (tok->isEmpty() == false);
}

bool xml::lex::getLiteral(xml::token *tok)
{
	if (m_s != 0)
	{
		while (m_s->m_xml[m_s->m_offset] != m_quote)
		{
			isNewLine();
			qappend(tok,&m_s->m_xml[m_s->m_offset], false);
			m_s->m_offset++;
			offsetLine();
		}
	}

	return (tok->isEmpty() == false);
}

bool xml::lex::getEncoding(xml::token *tok)
{
	// first character in the encoding 
	// must be a letter
	if (m_s != 0)
	{
		if (isLetter(m_s->m_xml[m_s->m_offset]))
		{
			while (isNameChar(m_s->m_xml[m_s->m_offset]))
			{
				qappend(tok,&m_s->m_xml[m_s->m_offset], false);
				m_s->m_offset++;
				offsetLine();
			}
		}
	}

	return (tok->isEmpty() == false);
}

bool xml::lex::getVersion(xml::token *tok)
{
	if (m_s != 0)
	{
		while (isNameChar(m_s->m_xml[m_s->m_offset]))
		{
			qappend(tok,&m_s->m_xml[m_s->m_offset], false);
			m_s->m_offset++;
			offsetLine();
		}
	}

	return (tok->isEmpty() == false);
}

bool xml::lex::handleWhitespace()
{
	// peek forward and check if next 
	// character in xml buffer is 
	// a white space character, if it
	// is, then extract it.
	bool ret = false;

	if (m_s != 0)
	{
		while (isWhitespace(m_s->m_xml[m_s->m_offset]))
		{
			ret = true;
			isNewLine();
			m_s->m_offset++;
			offsetLine();
		}
	}

	return ret;
}

bool xml::lex::handleReserved(const char *word, bool extract /* = true */)
{
	bool ret = false;

	if (m_s != 0)
	{
		short offset = 0;
		do
		{
			ret = (*word == m_s->m_xml[m_s->m_offset + offset]);
			offset++;
			word++;
		}
		while ((*word) && (ret != false));
		
		if ((ret != false) && 
			(extract != false))
		{
			m_s->m_offset += offset;
			if (m_streamStack.m_nNext == 1) 
				m_byte += offset;
		}

		while ((m_s != 0) && (m_s->m_xml[m_s->m_offset] == 0))
		{
			GStackPop(m_streamStack);
			GStackTopType(m_streamStack,m_s,Stream *);
		}
	}

	return ret;
}


void xml::lex::nextToken(token *tok)
{
	char *pHeapedToken = tok->release();
	if (pHeapedToken)
	{
		// most tokens point to data in the source XML stream
		// those tokens are "valid" until the tokenizer goes 
		// out of scope.  Tokens that had entities expanded in 
		// them were put on the heap in tokenizer allocated memory.
		// The user of a tokenizer does not have to distinguish
		// between the two types of memory management.  All tokens
		// are valid until the tokenizer goes out of scope. So We 
		// save those "rare" heaped tokens here.
		m_heapedTokens.AddLast(pHeapedToken);
	}

	if (m_state != xml::_finished)
	{
		getToken(tok);
		if (tok->m_type == xml::_unknown)
		{
			// throw an exception if eof found
			// and start tags still exist
			if (m_tagStack.m_nNext != 0)
			{
				do
				{
					GStackPopType(m_tagStack,tag,char *);

					while ((tag) && (*tag != '>') && (!isWhitespace(*tag)))
					{
						m_strTempError += *tag;
						tag++;
					}

					if (m_tagStack.m_nNext != 0)
						m_strTempError += ", ";
				}
				while (m_tagStack.m_nNext != 0);
				m_strTempError += '.';

				throw GException("XML Parser", 18, (const char *)m_strTempError, m_line, m_byte);
			}
			else
			{
				if (m_s != 0)
				{
					if (m_s->m_xml[m_s->m_offset] != 0)
					{
						throw GException("XML Parser", 24, m_line, m_byte);
					}
				}
			}
		}
	}

	if (!tok->m_heap && tok->m_gp)
	{
		tok->m_chPreTerminatedByte = tok->m_gp[tok->m_len];

		// ------------------------------------------------------------------------
		tok->m_gp[tok->m_len] = 0;  // <-- if you crash here, you do not have write 
		// permission to the source XML buffer, or the buffer is no longer valid.
		// If you parsed XML on the stack (o.FromXML("<MyXML>..</MyXML>")) this can
		// happen, move it to the heap.
		// ------------------------------------------------------------------------
	}
}

//////////////////////////////////////////////////////////////////////////////
// token

xml::token::token()
{
	memset(this, 0, sizeof(xml::token));
}

xml::token::~token()
{
	if (m_heap != false)
	{
		delete [] m_gp;
		delete m_os;
	}
}

inline void xml::token::append(const char *s, bool copy /*= false*/)
{
	if ((copy == false) && (m_heap == false))
	{
		if (m_gp == 0)
			m_gp = (char *)s;
		m_len++;
	}
	else
	{
		if (m_os == 0)
		{
			m_os = new GString();
		}

		if (m_gp != 0)
		{
			m_os->write(m_gp, m_len);;
			m_gp = 0;
		}

		m_heap = true;
		
		if (copy == false)
		{
			*m_os << *s;
			m_len++;
		}
		else
		{
			*m_os << s;
			m_len += strlen(s);
		}
	}
}

inline void xml::token::append(char ch, bool copy /*= false*/)
{
	if ((copy != false) || (m_heap != false))
	{
		if (m_os == 0)
		{
			m_os = new GString();
		}

		if (m_gp != 0)
		{
			m_os->write(m_gp, m_len);
			m_gp = 0;
		}

		m_heap = true;
		*m_os << ch;
	}

	m_len++;
}
/*
char *xml::token::release()
{
	char *pHeapedMemory;
	if (m_heap != false)
	{
		pHeapedMemory = m_gp;
		delete m_os;
	}
	else
	{
		// un-null terminate
		if (m_chPreTerminatedByte)
		{
			m_gp[m_len] = m_chPreTerminatedByte;
		}
		pHeapedMemory = 0;
	}
	memset(this, 0, sizeof(xml::token));
	return pHeapedMemory;
}

void xml::token::toHeap()
{
	if (m_gp != 0)
	{
		if (m_os == 0)
		{
			m_os = new GString;
		}

		m_os->write(m_gp, m_len);;
		m_gp = 0;
	}

	m_heap = true;
}

inline bool xml::token::isEmpty()
{
	char *s = get();
	if (s != 0)
		return (s[0] == 0);
	else
		return false;	
}
*/
////////////////////////////////////////////////////////////////////
//
xml::entity::entity() :
	m_tag(0),
	m_value(0)
{
}

xml::entity::~entity()
{
	delete [] m_tag;
	delete [] m_value;
}

void xml::entity::setTag(token *tok)
{
	delete m_tag;
	m_tag = new char [tok->length() + 1];
	memcpy(m_tag,tok->get(),tok->length());
	m_tag[tok->length()] = 0;
}

const char *xml::entity::getTag()
{
	return (m_tag != 0) ? m_tag : "";
}

void xml::entity::setValue(token *tok)
{
	delete m_value;
	m_value = new char [tok->length() + 1];
	memcpy(m_value,tok->get(),tok->length());
	m_value[tok->length()] = 0;
}

const char *xml::entity::getValue()
{
	return (m_value != 0) ? m_value : "";
}
