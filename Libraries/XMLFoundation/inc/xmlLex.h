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
#ifndef __XML_NAMESPACE__
#define __XML_NAMESPACE__


#include "GList.h"
#include "GStack.h"
#include "GString.h"
#include "GException.h"

#include <string.h> // for memset()

class xml
{
public:

	class lex;
	class token;
	typedef void (*lpCallback)(xml::lex *lex, xml::token *tok);

//	typedef enum tokenType
	enum tokenType
	{
		_unknown = 0,
		_version = 1,
		_encoding,
		_standalone,
		_instruction,
		_comment,
		_systemLiteral,
		_publicLiteral,
		_entityName,
		_entityValue,
		_startTag,
		_emptyEndTag,
		_endTag,
		_attributeName,
		_pcdata,
		_cdata
	};

//	typedef enum primaryState
	enum primaryState
	{
		_prolog,
			_xml_decl,
			_xml_pre_dtd,
				_xml_dtd,
		_misc,
		_content,
		_finished
	};

//	typedef enum secondaryState
	enum secondaryState
	{
		_xml_decl_version,
		_xml_decl_encoding,
		_xml_decl_standalone,
		_xml_pi,
		_xml_comment,
		_xml_dtd_name,
		_xml_pre_markup,
		_xml_markup,
		_xml_post_markup,
		_xml_system_literal,
		_xml_public_literal,
		_xml_entity,
		_xml_entity_end,
		_xml_element_start,
		_xml_element_start_end,
		_xml_element_end_tag,
		_xml_element_end_end,
		_xml_element_attribute,
		_xml_element_attribute_value,
		_xml_element_content
	};

	#define MAX_ERROR_BYTES 30

	typedef struct tagStream
	{
		long m_offset;	// current byte offset into xml stream
		char *m_xml;	// xml stream
		bool m_delete;
	} Stream;

	class token
	{

	protected:
		
		GString *m_os;
		bool  m_heap;
		char *m_gp;
		char m_chPreTerminatedByte;
		__int64  m_len;

		// allow the lex class direct access for performance
		friend class lex;

	public:

		tokenType m_type;
		token();
		~token();

		inline char *release()
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


		inline void append(const char *s, bool copy = false);
		inline void append(char ch, bool copy = false);

		void toHeap()
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

		inline char *get()
		{
			if (m_heap != false)
			{
				if (m_os == 0)
				{
					m_os = new GString();
				}

				m_gp = (char *)(const char *)*m_os;
			}

			return (m_gp != 0) ? m_gp : (char *)"";
		}
		inline __int64 length()
		{ 
			return m_len; 
		};
		inline bool isEmpty()
		{
			char *s = get();
			if (s != 0)
				return (s[0] == 0);
			else
				return false;	
		}
	};

	class entity
	{
		char *m_tag;
		char *m_value;

	public:
		
		entity();
		~entity();

		void setTag(token *t);
		const char *getTag();
		void setValue(token *t);
		const char *getValue();
	};

	class lex
	{

	protected:

		GString m_strError;
		GString m_strTempError;

		lpCallback m_lpfnExternalDTD;

		bool handleReserved(const char *word, bool extract = true);
		void handleEntityReference();
		bool handleWhitespace();

		bool getVersion(token *tok);
		bool getEncoding(token *tok);
		bool getStandalone(token *tok);
		bool getInstruction(token *tok);
		bool getComment(token *tok);
		bool getName(token *tok);
		inline bool getCharacterData(token *tok);
		bool getCData(token *tok);
		bool getAttribute(token *tok);
		bool getEntity(token *tok);
		bool getLiteral(token *tok);

		long m_line;	// current line 
		long m_byte;	// current byte on line

		bool m_standalone;
		bool m_postmarkup;

		bool m_parameterEntity;

		char m_quote;

		char *m_documentType;

		// used to verify start-end tag match
		GStack m_tagStack;

		// list of parameter entities %
		GList *m_parameterEntities;
		// list of general entities &
		GList *m_generalEntities;

		// used to manage entity expansion
		GStack m_streamStack;
		GList  m_streams;

		Stream *m_s;

		void getEqual();
		void getQuote();
		void getContent(token *tok);
		void getDTD(token *tok);
		void getMisc(token *tok);
		void getXMLDecl(token *tok);
		void getToken(token *tok);

		primaryState m_state;
		primaryState m_nextState;
		secondaryState m_sstate;

		GList m_heapedTokens;

		virtual void loadExternal(lpCallback lpFn, token *tok);

	public :

		bool isParameterEntity();
		
		// xml data stream
		lex(GList *parameterEntities, GList *generalEntities,	char *stream);
		// clean's up heap
		virtual ~lex();

		// sets a reference of stream to m_s
		void setStream(char *stream);
		// gets the current line 0x0D
		long getLine() { return m_line; }
		GList *getPEs() { return m_parameterEntities; }
		GList *getGEs() { return m_generalEntities;   }

		// sets the state to parse external DTD
		void setStateDTD(const lex &l);
		// callback function used to load and parse 
		// the external DTD
		void setCallbackDTD(lpCallback lpFn);

		// returns the next token in the XML
		void nextToken(token *tok);

		// Every token must be released, normally nextToken()
		// is called with the previously issued token to be released.
		void releaseLastToken(token *tok)
		{
			char *pHeapedToken = tok->release();
			if (pHeapedToken)
				m_heapedTokens.AddLast(pHeapedToken);
		}
	};
};

#endif //__XML_NAMESPACE__
