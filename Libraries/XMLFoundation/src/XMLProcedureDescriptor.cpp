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

#include "XMLProcedureDescriptor.h"
#include "xmlObject.h"


IMPLEMENT_FACTORY(XMLParameter, Parameter)
IMPLEMENT_FACTORY(XMLProcedureDescriptor, ReportParameters)

void XMLParameter::MapXMLTagsToMembers()
{
	MapMember(&m_strTypeName, "TypeName");
	MapMember(&m_strParamName,"Name");
	MapMember(&m_nTypeCode,"Type");
	MapMember(&m_nLength,"Length");
	MapMember(&m_nDirection,"Direction");
}


void XMLProcedureDescriptor::MapXMLTagsToMembers()
{
	MapMember(&m_lstXMLParameter,  XMLParameter::GetStaticTag() );
}

XMLProcedureDescriptor::~XMLProcedureDescriptor()
{
}

XMLProcedureDescriptor::XMLProcedureDescriptor()
{
}