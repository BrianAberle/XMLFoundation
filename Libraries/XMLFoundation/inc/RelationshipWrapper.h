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

// used by XMLObject::MapMember() when a wrapper tag is specified.
#ifndef _RELATIONSHIP_WRAP__
#define _RELATIONSHIP_WRAP__

enum ContainedType
{
	ContainedType_ListAbstraction = 1,
	ContainedType_StringCollectionAbstraction = 2,
	ContainedType_IntegerArrayAbstraction = 3,
	ContainedType_XMLObject = 4,
	ContainedType_XMLObject_Ptr = 5,
	ContainedType_KeyedDataStructureAbstraction = 6
};

#include "MemberDescriptor.h"
class XMLRelationshipWrapper : public XMLObject
{
	XMLRelationshipWrapper(){/*never used*/}
	
	// 1=list, 2=string list, 3=int array, 4=object, 5=objPointer, 6=KeyedDataStructure
	ContainedType m_nContainedType; 

	// abstract the abstraction specified by m_nContainedType
	void *m_pData;
	void *m_pAccessor;
public:
	XMLRelationshipWrapper(const char *pzTag)
	{	
		SetDefaultXMLTag( pzTag );
		SetObjectTypeName( pzTag );
		m_pDerivedAddress = this;
		ModifyObjectBehavior(NO_MEMBER_MAP_PRECOUNT);
		setMemberStateSummary(DATA_DIRTY);
	}

	~XMLRelationshipWrapper()
	{
		// added 12/28/2009 because a list wrapped in a RelationShipWrapper was decrementing twice in ~XMLObject
		// once in ReferenceChangeChildren(), then a second time when this owning object was destroyed
		ModifyObjectBehavior(PREVENT_AUTO_DESTRUCTION,0);
	};
	virtual const char *GetVirtualType() 
	{ 
		return "RelationshipWrapper"; 
	}
	
	virtual bool SerializeObject()
	{
		bool bRet = true;
		switch (m_nContainedType)
		{
		case ContainedType_ListAbstraction: // don't write the wrapper if the wrapped list is empty
		bRet = (((ListAbstraction *)m_pAccessor)->itemCount(m_pData) == 0) ? false : true;
		break;
		case ContainedType_StringCollectionAbstraction: // likewise if there are no strings in a string list
		bRet = (((StringCollectionAbstraction *)m_pAccessor)->itemCount(m_pData) == 0) ? false : true;
		break;
		case ContainedType_IntegerArrayAbstraction: 
		bRet = (((IntegerArrayAbstraction *)m_pAccessor)->itemCount(m_pData) == 0) ? false : true;
		break;
		case ContainedType_XMLObject: // always write a sub object unless it specifically says not to
		bRet = ((XMLObject *)m_pData)->SerializeObject();
		break;
		case ContainedType_XMLObject_Ptr: // the same for object pointers if they've been allocated
		if (m_pData && *((XMLObject **)m_pData) && (*((XMLObject **)m_pData))->SerializeObject()  )
			bRet = true;
		else
			bRet = false;
		break;
		case ContainedType_KeyedDataStructureAbstraction:
			bRet = (((KeyedDataStructureAbstraction *)m_pAccessor)->itemCount(m_pData) == 0) ? false : true;
		break;
		}
		
		return bRet;
	};
	
	// XMLRelationshipWrapper is a pseudo-Object used for grouping like members.
	// this is used only by XMLObject to build "wrapper classes"
	// AddReference is overloaded for each "mappable-wrappable" type.
	void AddReference(const char *pzTag, void *pList,ListAbstraction *pHandler,ObjectFactory pFactory, int bFirstMap)
	{
		m_nContainedType = ContainedType_ListAbstraction;
		m_pData = pList;
		m_pAccessor = pHandler;

		MapMember(pList,pzTag,pHandler,0,pFactory);
		MemberDescriptor *pMD = (MemberDescriptor *)m_lstMembers.Last(); 
		pMD->m_bFirstMap = bFirstMap;
	}
	void AddReference(const char *pzTag, XMLObject **ppObj, ObjectFactory pFactory )
	{
		m_nContainedType = ContainedType_XMLObject_Ptr;
		m_pData = ppObj;

		MapMember(ppObj,pzTag,0,pFactory);
	}
	void AddReference(const char *pzTag, XMLObject *pObj )
	{
		m_pData = pObj;
		m_nContainedType = ContainedType_XMLObject;

		MapMember(pObj,pzTag);
	}
	void AddReference(void *pStringCollection, const char *pzElementName, StringCollectionAbstraction *pHandler, const char *pzTranslationMapIn, const char *pzTranslationMapOut,int nTranslationFlags)
	{
		m_nContainedType = ContainedType_StringCollectionAbstraction;
		m_pData = pStringCollection;
		m_pAccessor = pHandler;

		MapMember( pStringCollection,pzElementName, pHandler, 0,pzTranslationMapIn, pzTranslationMapOut,nTranslationFlags);
	}
	void AddReference(void *pIntegerArray, const char *pzElementName, IntegerArrayAbstraction *pHandler, const char *pzTranslationMapIn, const char *pzTranslationMapOut,int nTranslationFlags)
	{
		m_nContainedType = ContainedType_IntegerArrayAbstraction;
		m_pData = pIntegerArray;
		m_pAccessor = pHandler;

		MapMember( pIntegerArray,pzElementName, pHandler, 0, pzTranslationMapIn, pzTranslationMapOut, nTranslationFlags);
	}
	
	void AddReference( void *pDataStructure, KeyedDataStructureAbstraction *pHandler, 
						const char *pObjectName)
	{
		m_nContainedType = ContainedType_KeyedDataStructureAbstraction;
		m_pData = pDataStructure;
		m_pAccessor = pHandler;

		MapMember( pDataStructure, pHandler, pObjectName, 0);
	}
	
	virtual void MapXMLTagsToMembers(){}
};

#endif //_RELATIONSHIP_WRAP__