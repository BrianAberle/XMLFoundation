// --------------------------------------------------------------------------
//						United Business Technologies
//			  Copyright (c) 2000 - 2014  All Rights Reserved.
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
#ifndef _LIBRARY_IN_1_FILE
static char SOURCE_FILE[] = __FILE__;
#endif


#include "xmlException.h"
#include "xmlObject.h"

IMPLEMENT_FACTORY(xmlException, Exception)

void xmlException::MapXMLTagsToMembers()
{
	MapMember(&_strExceptionReason, "Description");
	MapMember((int *)&_error, "ErrorNumber");
	MapMember((int *)&_subSystem, "SubSystem");
	MapMember(&_stk, "Frame", "CallStack");
	MapMember(&_ErrorDetail, "Detail","UserContext");
}

const char *xmlException::GetDescription()
{
	return (const char *)*((GString *)this);
}

