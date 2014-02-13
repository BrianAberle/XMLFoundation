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

#include "xmlObject.h"
#include "GString.h"

class XMLParameter : public XMLObject
{
public:
	int m_nDirection;
	int m_nLength;
	int m_nTypeCode;
	GString m_strTypeName;
	GString m_strParamName;
	GString m_strUserParamValue;
public:
	XMLParameter(){};

public:
	DECLARE_FACTORY(XMLParameter, Parameter)
	virtual void MapXMLTagsToMembers();
};

class XMLProcedureDescriptor : public XMLObject
{
public:
	GList m_lstXMLParameter; // contains XMLParameter's
	XMLProcedureDescriptor();
	~XMLProcedureDescriptor();
public:
	DECLARE_FACTORY(XMLProcedureDescriptor, ReportParameters)
	virtual void MapXMLTagsToMembers();
};

