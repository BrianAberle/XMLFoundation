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
#ifndef __OBJ_FACT_H
#define __OBJ_FACT_H

#include "xmlDefines.h"
#include "xmlLex.h"
#include "GList.h"
#include "GString.h"

class MemberDescriptor;
class XMLObject;
class xmlException;
class XMLAttributeList;

class XMLObjectFactory
{
	xml::token m_tokenLookAhead;
	xml::token m_token;
	
	const char *m_pzExceptionThrower;
	XMLObject *m_pProtocolObject;
	xmlException *m_pException;
	MemberDescriptor *m_pExceptionMap;

	// member variables to reduce stack data in recursive process
	char m_pzLastFoundOID[512];
	char m_pzLastUpdateTime[32];// 256 bit object timestamp
	int m_bApplyQueryResults;

	xml::lex *m_pLex;
	GList m_paramtererEntities;
	GList m_generalEntities;
	
	// current recusrion level during factorization
	int m_nXMLRecurseLevel;
	
	// recursion level where objects for the m_pResultSetDescriptor are found
	int m_nQueryResultsLevel;

	MemberDescriptor *m_pResultSetDescriptor;
	
private:
	MemberDescriptor *HandleExceptions(const char *pzXml, XMLObject *pObjCurrent);
	void TraceOutBadMemberMapOrVersionMismatch(const char *pzXml, XMLObject *pObjCurrent);
	XMLObject *GetCoorespondingSubObject(MemberDescriptor *pMap,xml::token *pObjectToken,unsigned int *);
	void SetObjectIDAndUpdateTime();
	void GetFirstTokenPastDTD();
	void SetObjectOrElementAttributeValue(xml::token **attNameTok, MemberDescriptor *pMap,XMLObject *pCurObj,unsigned int nObjBehaviorFlags);
	void SetRootObjectAttributeValue(xml::token **attNameTok, MemberDescriptor *pMap,XMLObject *pObjCurrent, unsigned int nObjBehaviorFlags);

	void receiveIntoObject(	XMLObject *pObjCurrent, MemberDescriptor *pMap, unsigned int nFlags, unsigned int nBehaviorFlags);

	// Obtain pointer to sub-object, Factory create if necessary
	XMLObject* CreateObject(const char *pzTag, MemberDescriptor *pMap, unsigned int *nFlags);

	void SetMapFromParent(	XMLObject **pObjParent,
							const char *pzTag, 
							MemberDescriptor **pMap);

	XMLAttributeList *LoadObjectAttributes();
	void FactoryInitComplete(XMLObject *pO, unsigned int nFlags, unsigned int nObjBehaviorFlags);
public:
	XMLObjectFactory(const char *pXMLBuf, const char *pzExceptionThrower = 0);
	~XMLObjectFactory();

	static GString m_strOIDID;

	void setResultDescriptor(MemberDescriptor *pMD){m_pResultSetDescriptor = pMD;}

	// pRootObject is the object that contains the root mapping structure to data 
	// storage locations.  pSecondaryMapHandler is an optional secondary handler
	// that will be called if pRootObject does not have the registered handler
	void extractObjects( XMLObject *pRootObject,
						 XMLObject *pSecondaryMapHandler = 0);
};



#endif //__OBJ_FACT_H
