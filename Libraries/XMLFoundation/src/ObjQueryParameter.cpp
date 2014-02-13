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

#include "ObjQueryParameter.h"
#include "xmlDefines.h"
#include "xmlObject.h"
#include <stdio.h>  // for: sprintf()


ObjQueryParameter::ObjQueryParameter(const char *pzTag)
{	
	m_ObjTag = pzTag;
	// an ObjQueryParameter is always dirty and must be serialized
	setMemberStateSummary(DATA_DIRTY);

	// and we need keep the mapped members dynamic
	ModifyObjectBehavior(NO_MEMBER_MAP_PRECOUNT);

	SetDefaultXMLTag( pzTag );
	SetObjectTypeName( pzTag );
	m_pDerivedAddress = this;
	setObjectDataHandler(this);
}



ObjQueryParameter::~ObjQueryParameter()
{
	GListIterator Iter(&m_members);
	while (Iter())
	{
		GString *pStr = (GString *)Iter++;
		delete pStr;
	}
}

void ObjQueryParameter::SetObjectValue(const char *pVal, __int64 nLen, int nType)
{
	m_ObjVal = pVal;
}

const char * ObjQueryParameter::GetObjectValue()
{
	return (const char *)m_ObjVal;
}

void ObjQueryParameter::AppendObjectValue(GString& xml)
{
	if (m_ObjVal.Length())
	{
		xml.AppendEscapeXMLReserved((const char *)m_ObjVal);
	}
}


void ObjQueryParameter::AddMember(	void *pList,const char *pObjectTag,
												ListAbstraction *pHandler)
{
	MapMember(pList,pObjectTag,pHandler);
}


void ObjQueryParameter::AddMember(XMLObject &pContainedObject)
{
	MapMember(&pContainedObject, pContainedObject.GetObjectTag());
}

void ObjQueryParameter::AddMember(ObjQueryParameter &pContainedObject)
{
	MapMember(&pContainedObject, pContainedObject.GetObjectTag());
}

void ObjQueryParameter::AddMember(const char *pzTag, int pzValue)
{
	char tempConversionBuffer[15];
	sprintf(tempConversionBuffer,"%d",(int)pzValue);
	AddMember(pzTag, tempConversionBuffer );
}


void ObjQueryParameter::AddMember(const char *pzTag, const char *pzValue)
{
	GString *pStr = new GString;
	*pStr << pzValue;
	MapMember(pStr,	pzTag);
	m_members.AddLast(pStr);
}
