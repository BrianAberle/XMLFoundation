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

// symbols truncated in the browser database unless MSVC has corrected this
#pragma warning (disable:4786) 
#include "GlobalInclude.h"
static char SOURCE_FILE[] = __FILE__;

#include "xmlObjectFactory.h"
#include "xmlObject.h"
#include "CacheManager.h"
#include "FrameworkAuditLog.h"
#include "FactoryManager.h"
#include "MemberDescriptor.h"
#include "MemberHandler.h"
#include "GException.h"
#include "xmlException.h"
#include "ObjectDataHandler.h"
#include <stdio.h> // for sprintf()

#include <string.h> // for: memcpy(), strlen()
#ifdef _WIN32
	#define strcasecmp	_stricmp
#else
	#include <strings.h>
#endif



#define SHORTHAND_NULL_OBJECT -1

GString XMLObjectFactory::m_strOIDID("oid");

XMLObjectFactory::XMLObjectFactory(const char *pXMLBuf, const char *pzExceptionThrower/* = 0*/) 
{
	m_pzExceptionThrower = pzExceptionThrower;
	m_pProtocolObject = 0;
	m_nXMLRecurseLevel = 0;
	m_nQueryResultsLevel = -1; // init to 'unknown'
	m_bApplyQueryResults = 0;
	m_pException = 0;
	m_pResultSetDescriptor = 0;
	m_pLex = new xml::lex(	&m_paramtererEntities, 
							&m_generalEntities, 
							(char *)pXMLBuf);
}
XMLObjectFactory::~XMLObjectFactory()
{
	if  (m_pException)
	{
		delete m_pException;
		delete m_pExceptionMap;
	}
	
	// The last release gets called so the source XML does not get truncated. 
	if (m_tokenLookAhead.m_type != xml::_unknown)
		m_pLex->releaseLastToken(&m_tokenLookAhead);
	m_pLex->releaseLastToken(&m_token);

	delete m_pLex;
}


XMLObject* XMLObjectFactory::CreateObject(const char *pzTag,MemberDescriptor *pMap, unsigned int *nFlags)
{
	int bDidCreate = 0;
	
	// Default to null if CreateObject() is given a Tag that does
	// not map to an object.  This does not indicate an error.
	// The caller of CreateObject() must determine if null is error. 
	XMLObject* pObject = 0;

	if ( pMap->DataType == MemberDescriptor::ManagedByDerivedClass )
	{
		// let the derived class figure out where the object comes from
		pObject = pMap->m_DataAbstractor.pMemberHandler->
						GetObject(pzTag,m_pzLastFoundOID,m_pzLastUpdateTime,&bDidCreate);
	}
	else if( pMap->IsSubObject() )
	{
		// Containment object instance, the owning object has already 
		// instanciated the object, we only need a pointer to it.
		// for example a 'Customer' that has a m_Order, that Order
		// was allocated along with the customer unlike ObjectPtr<Order> m_pOrder.
		if ( pMap->DataType == MemberDescriptor::XMLObj ) 
		{
			// do not Factory create, this maps to an existing sub-object 
			pObject = pMap->m_Member.pObject;

			if(m_pzLastFoundOID[0])
				pObject->Init( m_pzLastFoundOID, m_pzLastUpdateTime);

			// Add this static map to the 'active system maps'.
			// For all dynamic objects (in list, tree, hash, pointers, etc.)
			// The map will be added in MemberDescriptor::AddContainedObject() so that
			// only a single Map is stored for the N objects that it may create,
			g_ActiveObjectMap.AddActiveObjectMap(pMap);
		}

		// If this object is one of the following types, then we 
		// must obtain an instance of the destination object
		// a. from the factory - for non-cached objects
		// b. from the cache - for cached objects with OID's
		else if ( pMap->DataType == MemberDescriptor::XMLObjList ||
				  pMap->DataType == MemberDescriptor::XMLObjPtr  ||
				  pMap->DataType == MemberDescriptor::Root		 ||
				  pMap->DataType == MemberDescriptor::KeyedDataStructure)
		{
			if (pMap->m_pfnFactoryCreate && pMap->DataType != MemberDescriptor::Root)
			{
				// If we were explicitly given a factory, use it directly. 
				// This object will not be cached.
				pObject = (*(pMap->m_pfnFactoryCreate))();
				pObject->Init( m_pzLastFoundOID, m_pzLastUpdateTime);
				bDidCreate = 1; // flag indicating a new object was just constructed
			}
			else 
				 
			{	 
				// Request it from the Object Cache, create it if it does not already 
				// exist - and place it into the cache if the ObjectID is known
				pObject = cacheManager.getObject(pzTag,m_pzLastFoundOID,m_pzLastUpdateTime,&bDidCreate);
				if ( pObject->GetObjectBehaviorFlags() & USE_STATE_CACHE )
				{
					GString *pState = cacheManager.getState(pzTag,m_pzLastFoundOID);
					if (pState)
					{
						// load the object from the state found in the state cache.
						pObject->SetObjectDescriptor(pMap);
						pObject->FromXML(pState->StrVal());
					}
				}
			}
			
			// If we created the object put it where the owning object has requested
			if ( pObject )
			{
				if ( pMap->DataType == MemberDescriptor::Root )
				{
					// This is a 'Root' object that does not map to the query.
					// For example, we queried for "Orders' and got back a 'Customer'
					// that MAY contain an 'Order' as a child. (we don't know yet)
					// We will now be expecting the actual objects that the query
					// has asked for, to be contained somewhere within this object.
					m_bApplyQueryResults = 1;
				}
				// since this usage is rare, for performance we only hit this code
				// when we know to expect sibling result objects.
				if (m_bApplyQueryResults ) 
				{
					if ( m_pResultSetDescriptor )
					{
						if ( ( // All objects in the query result set are found 
							   // at the same recursion level in the xml stream
							   (m_nXMLRecurseLevel == m_nQueryResultsLevel) ||
							   (m_nQueryResultsLevel == -1)
							 ) &&
							 // And must match the expected tag of the m_pResultSetDescriptor
							 (strcasecmp( (const char *)m_pResultSetDescriptor->strTagName, pzTag ) == 0 )
						   )
						{
							m_nQueryResultsLevel = m_nXMLRecurseLevel;
							m_pResultSetDescriptor->m_DataAbstractor.pListHandler->append( m_pResultSetDescriptor->m_Member.pXMLObjList,pObject );
						}
					}
				}
			}
		}
	}
	
	// Tie together the creating MemberDescriptor with the instance.
	// This enables an object to know it's creator.  Bringing together
	// the Tree of MemberDescriptors with the actual runtime instance
	// of the object.  This framework allows implicit object model
	// navigation - For example, if an instance of an object wanted 
	// to know if it resided within a list in a 'Customer' or as a 
	// pointer within a 'Order', it could navigate to it's runtime
	// owning instance by looking at the ObjectDescriptor in the 
	// MemberDescriptor that now resides within the object.
	if (pObject)
	{
		if (pObject->SetObjectDescriptor(pMap))
		{
			*nFlags |= FACTORY_FLAG_FIRST_REFERENCE;
		}
		else
		{
			// 12/28/2009 clear this flag to prevent IncRef() in FactoryInitComplete()
			// It may have been inadvertantly set in receiveIntoObject(). Search for 
			// "nDeepFlags = nDataFlags" to see where our parent object passed its flags.
			*nFlags &= ~FACTORY_FLAG_FIRST_REFERENCE; 
		}
	}


	if (bDidCreate == 1)
		*nFlags |= FACTORY_FLAG_OBJ_FROM_FACTORY;


	return pObject;
}



//	Parse XML elements into an XMLObject
//	necessary to hold the information represented in the XML
void XMLObjectFactory::receiveIntoObject( XMLObject *pObjCurrent/* = 0*/, 
										  MemberDescriptor *pMap/* = 0*/,
										  unsigned int nDataFlags,
										  unsigned int nParentBehaviorFlags)
{
	m_nXMLRecurseLevel++;
	xml::token *tok;
	if (m_tokenLookAhead.m_type != xml::_unknown)
	{
		tok = &m_tokenLookAhead;
	}
	else
	{
		tok = &m_token;
		// temporarily assign to any type that does not terminate the while.  It will 
		// be re-assigned at the first nextToken() immediately inside the while().
		tok->m_type = xml::_startTag; 
	}


	unsigned int nObjBehaviorFlags = (pObjCurrent) ? pObjCurrent->GetObjectBehaviorFlags() : nParentBehaviorFlags;

	// Now begin iterating through the 'members' of pObjCurrent.
	int bAssigned = 0;
	// parse this entire object, till it's end tag 
	while (tok->m_type != xml::_endTag && tok->m_type != xml::_emptyEndTag)
	{
		if (m_tokenLookAhead.m_type == xml::_unknown)
		{
			m_pLex->nextToken(&m_token);	
			tok = &m_token;
			// bail upon invalid XML
			if (tok->m_type == xml::_unknown)
				return;
		}
		
		
		// if this is an element value - (Character Data)
		if( tok->m_type == xml::_cdata) 
		{
			XMLObjectDataHandler* o = pObjCurrent->GetObjectDataHandler();
			if(o)
			{
				o->SetObjectValue(tok->get(),tok->length(),2);
			}
		}
		
		// members: <member></member> need to be assigned
		if ( tok->m_type == xml::_endTag && pMap && !bAssigned)
		{
			pMap->Set( "", 0, nDataFlags );
			if ( nObjBehaviorFlags & MEMBER_UPDATE_NOTIFY )
			{
				// FYI: this virtual call is made only if the object behavior flag is set
				// that is because at this level, for every XML element, needlessly pushing
				// 5 arguments on the stack to needlessly call a stubbed out handler is a performance error.
				pMap->m_ParentObject->ObjectMessage(MSG_MEMBER_UPDATE, pMap->strTagName, "" , 0, 0);
			}
		}
		// if this is  (Parsed Character Data)
		else if ( tok->m_type == xml::_pcdata )
		{
			// Set the member variable value if it's mapped && the UpdateTime in the
			// XML stream is newer than the UpdateTime in an existing object.
			if (pMap)
			{
				pMap->Set( tok->get(), tok->length(), nDataFlags );
				if ( nObjBehaviorFlags & MEMBER_UPDATE_NOTIFY )
				{
					pMap->m_ParentObject->ObjectMessage(MSG_MEMBER_UPDATE, pMap->strTagName, tok->get(), tok->length(), 0);
				}
			}
			else // this 'else' was added on 3/10/2008
			{
				if ( (nObjBehaviorFlags & NO_OBJECT_DATA) == 0) // this 'if' was added 2/6/2014
				{
					if (pObjCurrent && pObjCurrent->GetObjectDataHandler())
					{
						// note: if pObjCurrent->GetObjectType() does not use 
						pObjCurrent->GetObjectDataHandler()->SetObjectValue(tok->get(), tok->length(), 1);
					}
				}
			}
			bAssigned = 1;
		}
		else if ( tok->m_type == xml::_attributeName )
		{
			//   set the attributes on the root object (has no owning map)
			if ( (pObjCurrent && !pMap) || (pObjCurrent && pMap && pMap->IsSubObject()))
			{
				SetRootObjectAttributeValue(&tok,pMap,pObjCurrent,nObjBehaviorFlags);
			}
			else
			{
				SetObjectOrElementAttributeValue(&tok,pMap,pObjCurrent,nObjBehaviorFlags);
			}
		}
		else if ( tok->m_type == xml::_emptyEndTag )
		{
			// short hand xml notation for an empty element
			if (pMap)
			{
				pMap->Set( "", 0, nDataFlags );
				if ( nObjBehaviorFlags & MEMBER_UPDATE_NOTIFY )
				{
					pMap->m_ParentObject->ObjectMessage(MSG_MEMBER_UPDATE, pMap->strTagName, "" , 0, 0);
				}
			}
			if (m_tokenLookAhead.m_type != xml::_unknown)
				m_pLex->releaseLastToken(&m_tokenLookAhead);
		}
		else if ( tok->m_type == xml::_startTag )
		{
			// Recurse into and get the value for the element or entire sub-object
			MemberDescriptor *pMap = (pObjCurrent) ? pObjCurrent->GetEntry( tok->get() ) : 0;
			if ( !pMap )
			{
				if (m_pProtocolObject)
				{
					pMap = m_pProtocolObject->GetEntry( tok->get() );
				}
				if (!pMap)
				{
					pMap = HandleExceptions( tok->get(),pObjCurrent);
				}
				if ( !pMap && pObjCurrent )
				{
					pMap = pObjCurrent->HandleUnmappedMember( tok->get() );
				}
				if (!pMap)
				{
					TraceOutBadMemberMapOrVersionMismatch(tok->get(),pObjCurrent);
				}
			}


			
			// if this tag is an object, get a pointer to the object, 
			// otherwise get null if this is the member of an object
			XMLObject *pSubObject;


//			unsigned int nDeepFlags = 0;			// this was the original code (for years...)
			unsigned int nDeepFlags = nDataFlags;	// 3/08/2008 Default to the value of the previous stack frame.
			
			if (nDataFlags & FACTORY_FLAG_FIRST_REFERENCE) 
				nDeepFlags |= FACTORY_FLAG_UPDATE_CLEAN_MEMBERS;

			if (pMap && pMap->IsSubObject())
			{
				pSubObject = GetCoorespondingSubObject(pMap,tok,&nDeepFlags);
			}
			else
			{
				pSubObject = 0;
				if (m_tokenLookAhead.m_type != xml::_unknown)
				{
					m_pLex->releaseLastToken(&m_tokenLookAhead);
				}
			}
			// recursivly load the next XML tag into this object
			if (pSubObject != (XMLObject *)SHORTHAND_NULL_OBJECT)
			{
// If you blow the stack or crash in any way during this process this may help you figure out where you went 
//  wrong by un-commenting the following two lines, and setting a breakpoint on the second
//	if (pSubObject && pSubObject->m_ValidObjectBeg != 777)	
//  {int i; i++;}

 				receiveIntoObject(pSubObject, pMap, nDeepFlags, nObjBehaviorFlags);
				if ( pSubObject && nObjBehaviorFlags & SUBOBJECT_UPDATE_NOTIFY )
				{
					// send 'this' a notification that a sub object has been updated
					pObjCurrent->ObjectMessage(MSG_SUBOBJECT_UPDATE, pMap->strTagName, 0, 0, pSubObject);
				}
			}

			// since we share the same token across all stack frames, we need to re-init
			// after returning from a recursive call, to be sure processing continues.
			tok->m_type = xml::_startTag;
		}
		else
		{ 
			// nothing to do for endtags, and attribute tags
		} 

		if (m_tokenLookAhead.m_type != xml::_unknown)
		{
			m_pLex->releaseLastToken(&m_tokenLookAhead);
		}

	} // (while != End Tag) iterate through this objects members
	
	// object notification of initialization or update by Object Factory
	if (pObjCurrent)
		FactoryInitComplete( pObjCurrent, nDataFlags,  nObjBehaviorFlags); 
	m_nXMLRecurseLevel--;
}


void XMLObjectFactory::FactoryInitComplete(XMLObject *pO, unsigned int nDataFlags, unsigned int nObjBehaviorFlags)
{
	// pMap could be null if pO is the root object on which FromXML() was called.
	// In that case the User created the object so no pMap will exist.
	MemberDescriptor *pMap = pO->GetObjectDescriptor();
	const char *pzOID = pO->getOID();
	XMLObject *pCachedInstance = 0;
	if (pzOID)
		pCachedInstance = cacheManager.findObject(pzOID, pO->GetVirtualType() );

	int nIsTempObject = (pCachedInstance && pCachedInstance != pO) ? 1 : 0;
	// nIsTempObject is only true when using late bound OID's through XMLObject::MapOID()
	// If the OID was not optimized in the XML as the first attribute of the object
	// an instance of the object was created prior to having the information of the OID.
	// Now, construction is complete - and we can guarantee that we have obtained all
	// data necessary to make the OID, so we go to the cache and see if an object
	// already exists for that OID.  If the object created already existed in the cache
	// the cached object is the one the application already has a reference to, so
	// the pO(the object just created) becomes a 'temp' object used to 'update' the 
	// cached object as the CopyState() is executed.  The temp object is then destroyed.

	if (nIsTempObject)
	{
		// pO is a temp object with a late bound OID. Copy into the 'real' cached object.
		// If this object has been newly referenced by another, then increment.
		if ( pCachedInstance->CopyState(pO) )
		{
			// If the Object Instance was the first of it's type as reported by the parent 
			// object that called MapMember() for this object, AND it already existed in 
			// the cache this is a new reference so IncRef() it.
			pCachedInstance->IncRef();
		}
		else
		{
			// The action was an update, then there is no new reference
		}
	}
	else
	{
		if ( pzOID && pCachedInstance != pO )
		{
			cacheManager.enterObject(pzOID, pO->GetVirtualType(), pO );
		}

		if (pCachedInstance && ( nDataFlags & FACTORY_FLAG_FIRST_REFERENCE ) && pCachedInstance->GetObjectDescriptorCount() > 1)
		{
			pCachedInstance->IncRef(0,1);
		}
	}

	
	// get a pointer to the object we are about to put into it's destination structure
	XMLObject *pObject = (pCachedInstance) ? pCachedInstance : pO;

	// object update notification
	if ( nObjBehaviorFlags & OBJECT_UPDATE_NOTIFY )
	{
		// pO will == 'this' unless it's a temp object containing delta data.
		pObject->ObjectMessage(MSG_XML_UPDATE, pzOID, 0, nDataFlags, pO);
	}



	// add the new object to the list/structure if it's not already there.
	// if it is already there, then the structure already points to the object
	// that we have just 'refreshed' or updated
	if ( pMap &&
		(pMap->DataType == MemberDescriptor::XMLObjList || 
		 pMap->DataType == MemberDescriptor::KeyedDataStructure ||
		 pMap->DataType == MemberDescriptor::XMLObjPtr ))
	{
		// if it did not come from the cache, add it to list/tree/hash/objPointer etc..
		if (!pCachedInstance)
		{
			pMap->AddContainedObject(pObject);
		}
		else
		{
			// otherwise the cached object state has already been updated.
			// if the owner (list/ObjPointer/tree/hash etc) already has a reference
			// to it then we're already done. Only add the object to the container 
			// if it does not already have a reference.

			if (pzOID &&  pMap->DataType == MemberDescriptor::KeyedDataStructure)
			{
				if (!pMap->GetKeyedContainedObject(pzOID))
				{
					pMap->AddContainedObject(pObject);
				}
			}
			else
			{
				xmlObjectIterator iter = 0;
				XMLObject *pObj = pMap->GetFirstContainedObject(&iter);
				while (pObj)
				{
					if (pObj == pObject)
					{
						pMap->ReleaseObjectIterator(iter);
						break;
					}
					pObj = pMap->GetNextContainedObject(iter);
				}
				if (pObj != pObject)
				{
					// If this object (from the cache) did not already exist in this 
					// structure now we have a new reference to it
					pMap->AddContainedObject(pObject);
				}
			}
		}
	}

	//  if we copied this's state to the real object in the cache and , 'pO' is a temp object. disappear.
	if (nIsTempObject)
	{
		pMap->SetAsTempObject(pO);
		pO->DecRef(); // destroy it, or pool it if this is a heavily used type
	}

	if ( nObjBehaviorFlags & EARLY_CONTAINER_NOTIFY )
	{
		// this calls MapXMLTagsToMembers() and builds structures that
		// are normally built only 'on-demand' for better performance.
		pObject->EarlyNotify();
	}

	// object notification that a FromXML() update is complete.	
	pObject->Construct();
}



void XMLObjectFactory::GetFirstTokenPastDTD()
{
	bool bSearchingForToken = true;

	while ( bSearchingForToken )
	{
		m_pLex->nextToken(&m_token); 
		if (m_token.m_type == xml::_unknown)	
			break;
		// the tokenizer is correctly positioned in the XML stream once we get 
		// this start tag so return to begin Factory creation process.
		bSearchingForToken = (m_token.m_type == xml::_startTag) ? false : true;
	}

}

// Copies Object attributes into a list, extracting the ObjectID and 
// UpdateTime attributes to m_pzLastFoundOID & m_pzLastUpdateTime.
XMLAttributeList *XMLObjectFactory::LoadObjectAttributes()
{
	// initialize to empty
	m_pzLastFoundOID[0] = 0;
	m_pzLastUpdateTime[0] = 0;

	// Heap storage of attributes for this object.  If the attributes are 
	// mapped, they will be moved from this list to the correct object
	// member, otherwise this list will be 'attached' to the object
	// and released by the object when the object is destroyed.
	XMLAttributeList *pAttributeList = 0;

	// We must save the last token we read but did not use and 
	// allow the factory to correctly handle this token.
	m_pLex->releaseLastToken(&m_tokenLookAhead);
	m_pLex->releaseLastToken(&m_token);
	m_pLex->nextToken(&m_tokenLookAhead);

	while ( m_tokenLookAhead.m_type == xml::_attributeName )
	{
		if (!pAttributeList)
		{
			pAttributeList = new XMLAttributeList;
		}
		// If we just found the ObjectID, hang on to it so we don't have to 
		// walk the attribute list when we need it.
		if ( (strcasecmp(m_tokenLookAhead.get(), (const char *)m_strOIDID) == 0) )
		{
			// now get the attribute value (m_tokenLookAhead.m_type = xml::_pcdata)
			m_pLex->nextToken(&m_tokenLookAhead);

			__int64 nDataLen = m_tokenLookAhead.length();
			memcpy(m_pzLastFoundOID,m_tokenLookAhead.get(),nDataLen+1);
			m_pzLastFoundOID[nDataLen] = 0;
			pAttributeList->AddAttribute( m_strOIDID, m_pzLastFoundOID );

		}
		else if ( (strcasecmp(m_tokenLookAhead.get(), "UpdateTime") == 0) )
		{
			// now get the UpdateTime attribute value
			m_pLex->nextToken(&m_tokenLookAhead);

			__int64 nDataLen = m_tokenLookAhead.length();
			memcpy(m_pzLastUpdateTime,m_tokenLookAhead.get(),nDataLen+1);
			m_pzLastUpdateTime[nDataLen] = 0;
			pAttributeList->AddAttribute( "UpdateTime", m_pzLastUpdateTime );
		}
		else // All other attributes go directly into the attribute storage list
		{	 // that will be owned by the Object.
			
			// store starting location of the attribute name
			char *pAttName = m_tokenLookAhead.get();
			__int64 nDataLen = m_tokenLookAhead.length();

			// get the next token (attribute value)
			m_pLex->nextToken(&m_tokenLookAhead);
			
			// temporarily null term the attribute name (fast - no temp buffer is used)
			char chHold = pAttName[nDataLen];
			pAttName[nDataLen] = 0;

			pAttributeList->AddAttribute( pAttName, m_tokenLookAhead.get() );

			// unterminate the attribute name in the tokenizer input buffer
			pAttName[nDataLen] = chHold;

		}
		m_pLex->nextToken(&m_tokenLookAhead);
	}
	return pAttributeList;
}

void XMLObjectFactory::SetObjectIDAndUpdateTime()
{
	// An Object may have a special key attribute called oid that
	// when supplied as the first attribute and when set to a value
	// that is unique to all objects of this type, provides special 
	// cache speed improvements and auto-relationship binding.

	m_pLex->releaseLastToken(&m_tokenLookAhead);
	m_pLex->releaseLastToken(&m_token);
	m_pLex->nextToken(&m_tokenLookAhead);
	
	// it will be init'd before we use it.
	__int64 nDataLen; 
	
	m_pzLastFoundOID[0] = 0;
	m_pzLastUpdateTime[0] = 0;
	if ( m_tokenLookAhead.m_type == xml::_attributeName )
	{
		if ( (strcasecmp(m_tokenLookAhead.get(), (const char *)m_strOIDID) == 0) )
		{
			// re-use the m_tokenLookAhead
			m_pLex->nextToken(&m_tokenLookAhead);
			if (m_tokenLookAhead.m_type == xml::_pcdata)
			{
				nDataLen = m_tokenLookAhead.length();
				memcpy(m_pzLastFoundOID,m_tokenLookAhead.get(),nDataLen+1);
				m_pzLastFoundOID[nDataLen] = 0;
			}
			else
			{
				TRACE_ERROR("Parsing error: invalid value found after AttributeStartTag");
			}
		}
	}
}



MemberDescriptor *XMLObjectFactory::HandleExceptions(const char *pzXml, XMLObject *pObjCurrent)
{
	MemberDescriptor *retVal = 0;
	if ( strcasecmp( "Exception", pzXml ) == 0)
	{
		m_pException = new xmlException;
		m_pExceptionMap = ::new MemberDescriptor(pObjCurrent,pzXml,m_pException);
		retVal = m_pExceptionMap;
	}
	return retVal;
}

void XMLObjectFactory::TraceOutBadMemberMapOrVersionMismatch(const char *pzXml, XMLObject *pObjCurrent)
{
	// there is no map for this XML Tag, assume newer version XML
	if (pObjCurrent)
	{
		char szTemp[128];
		sprintf(szTemp, "*** Tag <%s> does not map to Object <%s> *************",
			pzXml, pObjCurrent->GetObjectType());
		TRACE_WARNING(szTemp)
	}
	else
	{
		char szTemp[128];
		sprintf(szTemp, "Tag <%s> unmappable.", pzXml);
		TRACE_WARNING(szTemp)
	}
}



XMLObject *XMLObjectFactory::GetCoorespondingSubObject(  MemberDescriptor *pMap,
														 xml::token *pObjectToken,
														 unsigned int *nFlags)
{
	// Get the Type Name of the object that we are working with
	char *pzObjectTag = (char *)pObjectToken->get();
	__int64 nObjectTagLen = pObjectToken->length();

	// assigns the value of Attribute 'oid' in m_pzLastFoundOID, if oid is the first attribute.
	SetObjectIDAndUpdateTime();

	// Updates the (m_pzLastFoundOID & m_pzLastUpdateTime) member variables
	// and extracts all attributes for this object into an XMLAttributeList
//	XMLAttributeList *pAL = LoadObjectAttributes();

	// null terminate the object tag
	char chDisplacedByNull = pzObjectTag[nObjectTagLen];
	pzObjectTag[nObjectTagLen] = 0;

	// CreateObject() returns NULL when the member type is not an object
	XMLObject *pSubObject = (pMap) ? CreateObject(pzObjectTag,pMap,nFlags) : 0;

	// un-null terminate
	pzObjectTag[nObjectTagLen] = chDisplacedByNull;

/*
	// if we have a list of attributes for this object, apply them to the object
	if (pSubObject && pAL)
	{
		// update any mapped attributes then free the heaped list
		GListIterator Iter(&pAL->m_attrList);
		while (Iter())
		{
			XMLAttributeList::XMLAttributeNameValuePair *pANVP = 
				(XMLAttributeList::XMLAttributeNameValuePair *)Iter++;
			pSubObject->SetMappedAttribute(pANVP->strAttrName,pANVP->strAttrValue, strlen(pANVP->strAttrValue));
		}
		// If the object was newly created directly attach the heaped attribute list
		if (nFlags & FACTORY_FLAG_OBJ_FROM_FACTORY)
		{
			// The object will now own the list and clean it up upon destruction.
			pSubObject->SetAttributeList(pAL);
		}
		else
		{
			// The object has been pulled from the cache and any new mapped attributes 
			// have been updated.  Now we can destroy the attribute list.
			delete pAL;
		}
	}
*/


	// NULL members are always assigned a value when supplied in the source XML
	// stream.  DIRTY members are never assigned a new value when supplied in 
	// the source XML except by special object behavior.  CLEAN members are sometimes 
	// assigned a new value when  supplied in the source XML, depending on if the data 
	// in the XML stream is newer than the data previously supplied.
	*nFlags |= FACTORY_FLAG_UPDATE_CLEAN_MEMBERS;

	if (pSubObject)
	{
		// CreateObject() may have returned a pointer to a previously created
		// object that might have a different update time.
		if ( m_pzLastUpdateTime[0] && strcasecmp( m_pzLastUpdateTime, 
											pSubObject->getTimeStamp() ) == 0)
		{
			// Since the Row in the DB has not changed, the members in the object
			// need not change unless they are being supplied for the first time.
			// Members in the state of NULL will still be assigned in the case of
			// additional information being added to an existing object.  Members
			// that are DIRTY are never replaced since DIRTY members are newer
			// data that has not yet been committed.  CACHED and CLEAN members
			// are the ones affected by this flag since we know that value is 
			// unchanged we do not reassign the same value each time the database
			// supplies it to us.

			*nFlags &= ~FACTORY_FLAG_UPDATE_CLEAN_MEMBERS; // clear the flag
		}
	}
	
	// return the object that was either created or retrieved from the cache
	return pSubObject;
}




//	Called by receiveIntoObject() after an "AttributeStartTag" has been found
//	this will get the next and only possible token (the value of the attribute)
//	and set it to the current member or object through the CMemberMappingEntry
void XMLObjectFactory::SetRootObjectAttributeValue(xml::token **attNameTok,
												   MemberDescriptor *pMap,
													  XMLObject *pObjCurrent,
													unsigned int nObjBehaviorFlags)
{
	__int64 nLen = (*attNameTok)->length();
	
	// strAttributeName will be [nLen] bytes long, we can use less memory by getting the exact amount from the heap
	// or take GStrings 256 byte stack default and briefly use a little more memory much faster than the heap allocation
	GString strAttributeName((*attNameTok)->get(), nLen);

	if (m_tokenLookAhead.m_type != xml::_unknown)
	{
		// attNameTok was a pointer to m_tokenLookAhead, so we 
		// are freeing it here before we ask for the next token.
		m_pLex->releaseLastToken(&m_tokenLookAhead);
		*attNameTok = &m_token;
	}
	m_pLex->nextToken(&m_token);


	if (m_token.m_type == xml::_pcdata)
	{
		// add attributes to this member if it is mapped to the object
		int nMapped = 0;
		if (pMap)
		{
			// Loads the MemberDescriptors for the object if this is the first attribute
			// then copies the token value into mapped member variable in the Object.
			nMapped = pObjCurrent->SetMappedAttribute(strAttributeName, m_token.get(), m_token.length() );
		}
		if ( !nMapped )
		{
			if ( nObjBehaviorFlags & PREVENT_ATTRIB_AUTOMAP )
			{
				// log the fact that data in the XML was lost because it was not mapped
				char szTemp[128];
				sprintf(szTemp, "*** Attribute Tag <%s> does not map to Object <%s> *************",
					strAttributeName.Buf(), pObjCurrent->GetObjectType());
				TRACE_WARNING(szTemp);

				// allow for custom override
				pObjCurrent->ObjectMessage(MSG_UNMAPPED_ATTRIBUTE, strAttributeName, m_token.get(), m_token.length(), 0);
			}
			else
			{
				pObjCurrent->AddAttribute(strAttributeName, m_token.get());
			}
		}
	}
	else
	{
		// debug tracing:
		// expected token (eType,strXml) following attributeStartTag (strAttTagName)
		GString strErr;
		strErr.Format("Unexpected XML <%s> following Token <%s> in object <%s>", m_token.get(), strAttributeName.Buf(), pObjCurrent->GetObjectType());
		TRACE_ERROR((const char *)strErr)
		throw;
	}
}


void XMLObjectFactory::SetObjectOrElementAttributeValue(xml::token **attNameTok, 
														   MemberDescriptor *pMap,
														   XMLObject *pCurObj,
														   unsigned int nObjBehaviorFlags)
{
	__int64 nLen = (*attNameTok)->length();

	// strAttributeName will be [nLen] bytes long, we can use less memory by getting the exact amount from the heap
	// or take GStrings 256 byte stack default and briefly use a little more memory much faster than the heap allocation
	GString strAttributeName((*attNameTok)->get(), nLen);

	if (m_tokenLookAhead.m_type != xml::_unknown)
	{
		// attNameTok was a pointer to m_tokenLookAhead, so we 
		// are freeing it here before we ask for the next token.
		m_pLex->releaseLastToken(&m_tokenLookAhead);
		*attNameTok = &m_token;
	}
	m_pLex->nextToken(&m_token);


	if (m_token.m_type == xml::_pcdata)
	{
		// Set it in the object if it has been mapped
		int nMapped = 0;
		if (pMap)
			nMapped = pMap->m_ParentObject->SetMappedAttribute(strAttributeName, m_token.get(), m_token.length() );
		if ( !nMapped )
		{
			// add attributes to this member if it is mapped to the object
			if ( pMap && pMap->DataType == 
									MemberDescriptor::ManagedByDerivedClass )
			{
				if ( pMap->m_Member.pObject )
				{
					pMap->m_DataAbstractor.pMemberHandler->
										SetAttribute(strAttributeName, m_token.get());
				}
			}
			else if (pMap)
			{
				pMap->AddAttribute(strAttributeName, m_token.get());
				if ( nObjBehaviorFlags & MEMBER_UPDATE_NOTIFY )
				{
					pCurObj->ObjectMessage(MSG_MEMBER_UPDATE, pMap->strTagName, m_token.get(), m_token.length(), 0);
				}
			}
		}
	}
	else
	{
		// debug tracing:
		// expected token (eType,strXml) following attributeStartTag (strAttTagName)
		GString strErr;
		strErr.Format("Unexpected XML <%s> following Token <%s> in object <%s>", m_token.get(), strAttributeName.Buf(), (const char *)pMap->strTagName);
		TRACE_ERROR((const char *)strErr)
		throw;
	}
}

void XMLObjectFactory::extractObjects( XMLObject *pRootObject, 
									   XMLObject *pSecondaryMapHandler /* = 0*/)
{
	// position token stream at the first 'ElementStartTag' and get it's value
	m_pProtocolObject = pSecondaryMapHandler;
	GetFirstTokenPastDTD();
	receiveIntoObject( pRootObject, 0, 0, 0);

	// If we factory created an exception object, throw it.
	if (m_pException)
	{
		if(m_pzExceptionThrower)
		{
			// This distinguishes Client errors from Server errors
			throw GException("XML Object Factory", 1, m_pzExceptionThrower,m_pException->GetDescription());
		}
		else
			throw GException(*m_pException);
	}
}
