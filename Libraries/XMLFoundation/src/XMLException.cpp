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

#include "xmlException.h"
#include "xmlObject.h"

IMPLEMENT_FACTORY(xmlException, Exception)

void xmlException::MapXMLTagsToMembers()
{
	MapMember((XMLObject *)this, "Description");
	MapMember((int *)&_error, "ErrorNumber");
	MapMember((int *)&_subSystem, "SubSystem");
}

const char *xmlException::GetDescription()
{
	return (const char *)*((GString *)this);
}

