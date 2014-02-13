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
#ifndef _GEN_XFER_OBJ
#define _GEN_XFER_OBJ

#include "xmlObject.h"
#include "GList.h"
#include "ObjectDataHandler.h"
#include "GString.h"

class ListAbstraction;
class ObjQueryParameter : public XMLObject, public XMLObjectDataHandler
{
	GList m_members; // contains GString's
	GString m_ObjTag;
	GString m_ObjVal;
public:
	ObjQueryParameter(){ }
	// used by XMLProcedureCall for a memberless object
	void SetObjectValue(const char *pVal, __int64 nLen, int nType);
	const char * GetObjectValue();
	ObjQueryParameter(const char *pzTag);

	~ObjQueryParameter();
	void AddMember(void *pList,const char *pObjectTag,	ListAbstraction *pHandler);
	void AddMember(const char *pzTag, const char *pzValue);
	void AddMember(const char *pzTag, int pzValue);
	void AddMember(ObjQueryParameter &pContainedObject);
	void AddMember(XMLObject &pContainedObject);

	// Since ObjQueryParameter has purely dynamic members, we dont wnat or need the
	// standard DECLARE_FACTORY and IMPLEMENT_FACTORY macros, therefore we must 
	// supply the pure virtual methods that are normally implemented in the MACRO
	virtual void MapXMLTagsToMembers(){} 
	void *GetUntypedThisPtr(){return (void *)this;}
	void AppendObjectValue(GString& xml);
};

#endif //_GEN_XFER_OBJ
