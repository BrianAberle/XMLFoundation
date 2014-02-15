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

#include "xmlObject.h"
#include "FrameworkAuditLog.h"
#include "xmlDefines.h"
#include "StackFrameCheck.h"
#include "MemberDescriptor.h"
#include "CacheManager.h"
#include "MemberHandler.h"
#include "ObjectDataHandler.h"
#include "AttributeList.h"
#include "xmlObjectFactory.h"
#include "GException.h"
#include "RelationshipWrapper.h"
#include "AbstractionsGeneric.h"
#include <stdio.h> // for sprintf() / sscanf()

#ifdef _IOS
	#include <stdlib.h> // for malloc()
#else
	#include <malloc.h> // for malloc()
#endif



#ifdef _WIN32
	#define strcasecmp	_stricmp
	#define LONG_ONE 1i64
#else
	#include <strings.h> // for strcasecmp()
	#define LONG_ONE 1LL
#endif

#include <string.h> // for: strlen()

#ifndef ASSERT
	#define ASSERT(f)    ((void)0)
#endif

// Global, stateless, thread-safe,  abstraction handlers:
GenericStringAbstract gGenericStrHandler;
GStringListAbstraction gGStringListHandler;
GArrayAbstraction gGArrayHandler;
GenericListAbstraction gGListHandler;

GHashAbstraction gGHashHandler;
GQSortAbstraction gGQSortHandler;
GBTreeAbstraction gGBTreeHandler;


class DefaultDataHandler : public XMLObjectDataHandler
{
public:
	DefaultDataHandler(){}
	virtual ~DefaultDataHandler(){}

	GString m_strObjectValue;
	GString m_strCData;

	GString *GetCData()
	{
		return &m_strCData;
	}
	GString *GetOData()
	{
		return &m_strObjectValue;
	}
	void SetObjectValue(const char *strSource, __int64 nLength, int nType)
	{											
		// nType 1 is Parsed Object Data, 
		// nType 2 is <![CDATA
		// nType 3 is custom use
		if (nType == 1)
		{
			if (m_strObjectValue.IsEmpty())
			{
				// use direct memory passthrough when possible
				m_strObjectValue.Attach(strSource,nLength,nLength+1,0);
			}
			else
			{
				m_strObjectValue.write(strSource, nLength); 
			}
		}
		else if (nType == 2)
		{
			// attach directly to the actual memory in the source XML stream
			m_strCData.Attach(strSource,nLength,nLength+1,0);
		}
		else
		{
			m_strObjectValue = strSource;
		}
	}
	void AppendObjectValue(GString& xml)
	{
		if (m_strCData.Length())
		{
			xml << "\n\t<![CDATA[" << m_strCData << "]]>";
		}
		// removes trailing tabs, spaces, carriage returns, new lines - junk that was set as the 'object Value'
		// if the entire value is whitespace, m_strObjectValue becomes empty.
		m_strObjectValue.TrimRightWS();
		if (m_strObjectValue.Length())
			xml.AppendEscapeXMLReserved(m_strObjectValue); // adds the value while escaping 
	}
	
	
	const char * GetObjectValue(GString &str)
	{ 
		m_strObjectValue.SetLength(m_strObjectValue.GetLength());
		str << m_strObjectValue;
		return str.Buf();
	}
	GString *GetObjectValue()
	{
		m_strObjectValue.SetLength(m_strObjectValue.GetLength());
		return &m_strObjectValue;
	}
	void AssignObjectValue(const char *strValue)
	{
		m_strObjectValue = strValue;
	}
};


const char *XMLObject::GetObjectTag() 
{ 
	// factory created objects always know the actual tag with which 
	// they were created from the source XML. This overrides the tag 
	// specified in in the object macros with the actual data used to 
	// create 'this' object instance.  Most objects are mapped with 
	// a TYPE::GetStaticTag(), but objects can be mapped not only to 
	// who they think they are, but to who the controlling call to 
	// MapMember() says they are.  Objects created in this manner know 
	// the xml tag that caused their creation.
	if (m_strXMLTag)
		return (const char *)*m_strXMLTag; 

	// Calling GetVirtualTag() returns the "outer most derived" xmltag
	// used by objects derived from any object when CXMLObject is in the base
	// of the object that is being derived from.  IMPLEMENT_FACTORY()
	// anywhere down the line of inheritance overrides any of the base class
	// definitions preceding it, allowing the object to be passed as an
	// abstract base class while behaving as it's outer most virtual self.
	// This allows for the functionality of an XML object implementation to 
	// be inherited, modified or extended in some way, then mapped to what 
	// ever xml tag specified in the derived and overriding IMPLEMENT_FACTORY()
	return GetVirtualTag();
}

// same as above but for the RTTI Object class name
const char *XMLObject::GetObjectType() 
{
	if (m_strObjectType)
		return (const char *)*m_strObjectType; 
	// user created objects only have RTTI info if RegisterObject() was called
	return GetVirtualType();
}

void XMLObject::SetObjectTypeName( const char *pzNewValue ) //*
{
	if (m_strObjectType)
		delete m_strObjectType;
	m_strObjectType = new GString;
	(*m_strObjectType) = pzNewValue;
}


void XMLObject::SetDefaultXMLTag( const char *pzNewValue ) //*
{
	if (m_strXMLTag)
		delete m_strXMLTag;
	m_strXMLTag = new GString;
	(*m_strXMLTag) = pzNewValue;
}

const char *XMLObject::getOID()
{ 
	// The OID cannot change, so if we already know it, return it.
	if (m_oid && m_oid->Length()) 
		return (const char *)*m_oid; 

	// The OID key parts are defined by the virtual MapXMLTagsToMembers()
	// so call it if we have not already done so, to build the lstOIDKeyParts
	if ( !(m_nBehaviorFlags & HAS_EXECUTED_MEMBER_MAP) )
		LoadMemberMappings();

	// Not all objects have an OID specified.
	GList   *lstOIDKeyParts = GetOIDKeyPartList(0);
	if (lstOIDKeyParts)
	{
		m_oid = new GString;
		GListIterator Iter(lstOIDKeyParts);
		while ( Iter() )																
		{																				
			const char *pzTag = (const char *)Iter++;
			union CAST_THIS_TYPE_SAFE_COMPILERS
			{
				void *       mbrVoid;
				unsigned int mbrInt;
				unsigned __int64 mbrInt64;
			}Member;  
			Member.mbrVoid = Iter++;
			GString strKeyPart;

// considering byteordering independence, the void pointer is cast according, 32 or 64 bit
#if defined(_LINUX64) || defined(_WIN64)  || defined(_IOS)
			if (Member.mbrInt64 == 1)
#else
			if (Member.mbrInt == 1)
#endif
			{
				// use the specified element value as the key part - this is GetMemberByTag() inline
				MemberDescriptor *pMap = GetEntry( pzTag );
				if (pMap)
				{
					pMap->GetMemberValue(strKeyPart);
				}
			}
			else
			{
				// use the specified attribute value as the key part
				FindAttribute( pzTag, strKeyPart );
			}
			*m_oid += (const char *)strKeyPart;
		}
		if (m_oid->Length())
			return (const char *)*m_oid;
	}
	return 0;
}

const char *XMLObject::getTimeStamp()
{ 
	if (m_TimeStamp) 
		return (const char *)*m_TimeStamp; 
	else 
		return 0;
}



void XMLObject::SetMemberSerialize(const char *pTag,bool bSerialize)
{
 	MemberDescriptor *pMap = GetEntry( pTag );
	if ( pMap )
	{
		pMap->SetSerializable( bSerialize );
	}
	else
	{
		GString strError;
		strError << "Cannot set member: " << pTag << " of Object: " << GetObjectType() << "\n";
		strError << "Either you have not called MapMember(), or the member does not exist";
		TRACE_ERROR((const char *)strError);
	}
}

void XMLObject::MapMemberBit(unsigned __int64 *pValue,const char *pTag,int nBit1to64,const char *pzCommaSeparated0Values, const char *pzCommaSeparated1Values)
{
	if(m_bCountingMemberMaps)
	{
		GetMemberMapCount(1);
		return;
	}
	if(nBit1to64 > 0 && nBit1to64 < 65 )
	{
		if (pValue)
		{
			MemberDescriptor *pNewMD = 0;
			if ( !(m_nBehaviorFlags & NO_MEMBER_MAP_PRECOUNT) )
			{
				pNewMD = new( ((char *)m_pMemberDescriptorArray) + (sizeof(MemberDescriptor) * m_nMappedCount++)  ) MemberDescriptor(this,pTag,pValue,LONG_ONE<<(nBit1to64-1),pzCommaSeparated0Values,pzCommaSeparated1Values);
			}
			else
			{
				pNewMD = ::new MemberDescriptor(this,pTag,pValue,LONG_ONE<<(nBit1to64-1),pzCommaSeparated0Values,pzCommaSeparated1Values);
			}
			RegisterMember(pNewMD);


			// a note about the 4th argument above this line . . -------------------
			//	unsigned __int64 nBitMask = 1 << (nBit1toN - 1); 
			//  where 1 is a full 64 bit 1 or 1111111111111111111111111111111111111111111111111111111111111111 in binary, aka 1i64 or 1LL.
			//  the excessive high order bits of LONG_ONE are safely lost in the conversion to an 8, 16, or 32 bit map, and critically preserved for a 64 bit map.
			// Therefore:
			//		0x01 == 1   == (1 << 0) == nBit1
			//		0x02 == 2   == (1 << 1) == nBit2
			//		0x04 == 4   == (1 << 2) == nBit3
			//		0x08 == 8   == (1 << 3) == nBit4
			//		0x10 == 16  == (1 << 4) == nBit5
			//		0x20 == 32  == (1 << 5) == nBit6
			//		0x40 == 64  == (1 << 6) == nBit7
			//		0x80 == 128 == (1 << 7) == nBit8
			// and the pattern continues into the 2nd byte like this...
			//		0x100== 256 == (1 << 8) == nBit9
		}
		else
		{
			char szTemp[64];
			sprintf(szTemp, "Cannot map %s to Null member.", pTag);
			TRACE_ERROR(szTemp);
		}
	}
	else
	{
		char szTemp[64];
		sprintf(szTemp, " MapMemberBit(%s) - bit index nBit1to64 out of range:[%d].", pTag, (int)nBit1to64);
		TRACE_ERROR(szTemp);
	}
}
void XMLObject::MapMemberBit(unsigned int *pValue,const char *pTag,int nBit1to32,const char *pzCommaSeparated0Values, const char *pzCommaSeparated1Values)
{
	if(m_bCountingMemberMaps)
	{
		GetMemberMapCount(1);
		return;
	}
	if(nBit1to32 > 0 && nBit1to32 < 33 )
	{
		if (pValue)
		{
			MemberDescriptor *pNewMD = 0;
			if ( !(m_nBehaviorFlags & NO_MEMBER_MAP_PRECOUNT) )
			{
				pNewMD = new( ((char *)m_pMemberDescriptorArray) + (sizeof(MemberDescriptor) * m_nMappedCount++)  ) MemberDescriptor(this,pTag,pValue,LONG_ONE<<(nBit1to32-1),pzCommaSeparated0Values,pzCommaSeparated1Values);
			}
			else
			{
				pNewMD = ::new MemberDescriptor(this,pTag,pValue,LONG_ONE<<(nBit1to32-1),pzCommaSeparated0Values,pzCommaSeparated1Values);
			}
			RegisterMember(pNewMD);
		}
		else
		{
			char szTemp[64];
			sprintf(szTemp, "Cannot map %s to Null member.", pTag);
			TRACE_ERROR(szTemp);
		}
	}
	else
	{
		char szTemp[64];
		sprintf(szTemp, " MapMemberBit(%s) - bit index nBit1to32 out of range:[%d].", pTag, (int)nBit1to32);
		TRACE_ERROR(szTemp);
	}
}
void XMLObject::MapMemberBit(unsigned short *pValue,const char *pTag,int nBit1to16,const char *pzCommaSeparated0Values, const char *pzCommaSeparated1Values)
{
	if(m_bCountingMemberMaps)
	{
		GetMemberMapCount(1);
		return;
	}
	if(nBit1to16 > 0 && nBit1to16 < 17 )
	{
		if (pValue)
		{
			MemberDescriptor *pNewMD = 0;
			if ( !(m_nBehaviorFlags & NO_MEMBER_MAP_PRECOUNT) )
			{
				pNewMD = new( ((char *)m_pMemberDescriptorArray) + (sizeof(MemberDescriptor) * m_nMappedCount++)  ) MemberDescriptor(this,pTag,pValue,LONG_ONE<<(nBit1to16-1),pzCommaSeparated0Values,pzCommaSeparated1Values);
			}
			else
			{
				pNewMD = ::new MemberDescriptor(this,pTag,pValue,LONG_ONE<<(nBit1to16-1),pzCommaSeparated0Values,pzCommaSeparated1Values);
			}
			RegisterMember(pNewMD);
		}
		else
		{
			char szTemp[64];
			sprintf(szTemp, "Cannot map %s to Null member.", pTag);
			TRACE_ERROR(szTemp);
		}
	}
	else
	{
		char szTemp[64];
		sprintf(szTemp, " MapMemberBit(%s) - bit index nBit1to16 out of range:[%d].", pTag, (int)nBit1to16);
		TRACE_ERROR(szTemp);
	}
}



void XMLObject::MapMemberBit(unsigned char *pValue,const char *pTag,int nBit1to8,const char *pzCommaSeparated0Values, const char *pzCommaSeparated1Values)
{
	if(m_bCountingMemberMaps)
	{
		GetMemberMapCount(1);
		return;
	}
	if(nBit1to8 > 0 && nBit1to8 < 9 )
	{
		if (pValue)
		{
			MemberDescriptor *pNewMD = 0;
			if ( !(m_nBehaviorFlags & NO_MEMBER_MAP_PRECOUNT) )
			{
				pNewMD = new( ((char *)m_pMemberDescriptorArray) + (sizeof(MemberDescriptor) * m_nMappedCount++)  ) MemberDescriptor(this,pTag,pValue,LONG_ONE<<(nBit1to8-1),pzCommaSeparated0Values,pzCommaSeparated1Values);
			}
			else
			{
				pNewMD = ::new MemberDescriptor(this,pTag,pValue,LONG_ONE<<(nBit1to8-1),pzCommaSeparated0Values,pzCommaSeparated1Values);
			}
			RegisterMember(pNewMD);
		}
		else
		{
			char szTemp[64];
			sprintf(szTemp, "Cannot map %s to Null member.", pTag);
			TRACE_ERROR(szTemp);
		}
	}
	else
	{
		char szTemp[64];
		sprintf(szTemp, " MapMemberBit(%s) - bit index nBit1to8 out of range:[%d].", pTag, (int)nBit1to8);
		TRACE_ERROR(szTemp);
	}
}

void XMLObject::MapMember(const char *pTag, MemberHandler *pHandler)
{
	if(m_bCountingMemberMaps)
	{
		GetMemberMapCount(1);
		return;
	}
	// This is a slower way to map members, only use it when 
	// you need special processing, or for unsupported datatypes.

	MemberDescriptor *pNewMD = 0;
	if ( !(m_nBehaviorFlags & NO_MEMBER_MAP_PRECOUNT) )
	{
		pNewMD = new( ((char *)m_pMemberDescriptorArray) + (sizeof(MemberDescriptor) * m_nMappedCount++)  ) MemberDescriptor(this, pTag, pHandler);
	}
	else
	{
		pNewMD = ::new MemberDescriptor(this, pTag, pHandler);
	}
	RegisterMember(pNewMD);
}



// Map a collection of Strings
/*****************************************************/
//	usage:
//  MapMember(&m_lstStringSample, "item", &gGStringListHandler ,"CollectionName");
//  
//	maps:
//	(...)
//	<CollectionName>
//		<item>Sample String 1</item>
//		<item>Sample String 2</item>
//		<item>Sample String 3</item>
//	</CollectionName>
//	(...)
void XMLObject::MapMember(GStringList *pStringCollection,	const char *pzElementName,const char *pNestedInTag, const char *pzTranslationMapIn, const char *pzTranslationMapOut,int nTranslationFlags)
{
	if(m_bCountingMemberMaps)
	{
		GetMemberMapCount(1);
		return;
	}
	MapMember(pStringCollection,pzElementName,&gGStringListHandler,pNestedInTag,pzTranslationMapIn,pzTranslationMapOut,nTranslationFlags);
}

void XMLObject::MapMember(void *pStringCollection,	const char *pzElementName,
				StringCollectionAbstraction *pHandler,
				const char *pNestedInTag, const char *pzTranslationMapIn, const char *pzTranslationMapOut,int nTranslationFlags)
{
	if(m_bCountingMemberMaps)
	{
		GetMemberMapCount(1);
		return;
	}

	if ( pNestedInTag )
	{
		XMLRelationshipWrapper *pRelation = new XMLRelationshipWrapper( pNestedInTag );
		if (!m_pRelationShipWrappers)
		{
			m_pRelationShipWrappers = new GList();
		}
		
		m_pRelationShipWrappers->AddLast( pRelation );
		pRelation->AddReference( pStringCollection, pzElementName, pHandler,pzTranslationMapIn, pzTranslationMapOut,nTranslationFlags );

		MemberDescriptor *pNewMD = 0;
		if ( !(m_nBehaviorFlags & NO_MEMBER_MAP_PRECOUNT) )
		{
			pNewMD = new( ((char *)m_pMemberDescriptorArray) + (sizeof(MemberDescriptor) * m_nMappedCount++)  ) MemberDescriptor(this, pNestedInTag,pRelation);
		}
		else
		{
			pNewMD = ::new MemberDescriptor(this, pNestedInTag,pRelation);
		}
		RegisterMember(pNewMD);
	}
	else
	{
		MemberDescriptor *pNewMD = 0;
		if ( !(m_nBehaviorFlags & NO_MEMBER_MAP_PRECOUNT) )
		{
			pNewMD = new( ((char *)m_pMemberDescriptorArray) + (sizeof(MemberDescriptor) * m_nMappedCount++)  ) MemberDescriptor(this,pzElementName,pStringCollection,pHandler,pzTranslationMapIn,pzTranslationMapOut,nTranslationFlags);
		}
		else
		{
			pNewMD = ::new MemberDescriptor(this,pzElementName,pStringCollection,pHandler,pzTranslationMapIn,pzTranslationMapOut,nTranslationFlags);
		}
		RegisterMember(pNewMD);
	}
}


void XMLObject::MapMember(GBTree *pDataStructure,const char *pzObjectName,	const char *pNestedInTag)
{
	if(m_bCountingMemberMaps)
	{
		GetMemberMapCount(1);
		return;
	}
	MapMember(pDataStructure,&gGBTreeHandler,pzObjectName, pNestedInTag);
}
void XMLObject::MapMember(GHash *pDataStructure,const char *pzObjectName,	const char *pNestedInTag)
{
	if(m_bCountingMemberMaps)
	{
		GetMemberMapCount(1);
		return;
	}
	MapMember(pDataStructure,&gGHashHandler,pzObjectName, pNestedInTag);
}
void XMLObject::MapMember(GQSortArray *pDataStructure,const char *pzObjectName,	const char *pNestedInTag)
{
	if(m_bCountingMemberMaps)
	{
		GetMemberMapCount(1);
		return;
	}
	MapMember(pDataStructure,&gGQSortHandler,pzObjectName, pNestedInTag);
}
void XMLObject::MapMember(void *pDataStructure,
					KeyedDataStructureAbstraction *pHandler,
					const char *pzObjectName, 
					const char *pNestedInTag/* = 0*/)
{
	if(m_bCountingMemberMaps)
	{
		GetMemberMapCount(1);
		return;
	}
	if ( pNestedInTag )
	{
		XMLRelationshipWrapper *pRelation = new XMLRelationshipWrapper( pNestedInTag );
		if (!m_pRelationShipWrappers)
		{
			m_pRelationShipWrappers = new GList();
		}
		
		m_pRelationShipWrappers->AddLast( pRelation );
		pRelation->AddReference( pDataStructure, pHandler, pzObjectName );

		MemberDescriptor *pNewMD = 0;
		if ( !(m_nBehaviorFlags & NO_MEMBER_MAP_PRECOUNT) )
		{
			pNewMD = new( ((char *)m_pMemberDescriptorArray) + (sizeof(MemberDescriptor) * m_nMappedCount++)  ) MemberDescriptor(this, pNestedInTag,pRelation);
		}
		else
		{
			pNewMD = ::new MemberDescriptor(this, pNestedInTag,pRelation);
		}
		RegisterMember(pNewMD);
	}
	else
	{
		MemberDescriptor *pNewMD = 0;
		if ( !(m_nBehaviorFlags & NO_MEMBER_MAP_PRECOUNT) )
		{
			pNewMD = new( ((char *)m_pMemberDescriptorArray) + (sizeof(MemberDescriptor) * m_nMappedCount++)  ) MemberDescriptor(this,pzObjectName,pDataStructure,pHandler);
		}
		else
		{
			pNewMD = ::new MemberDescriptor(this,pzObjectName,pDataStructure,pHandler);
		}
		RegisterMember(pNewMD);
	}
}




// Map a dynamically growing Integer array
/*****************************************************/
//	usage:
//  MapMember(&m_dnyaIntArray, "item", &gGArrayHandler ,"CollectionName");
//  
//	maps:
//	(...)
//	<CollectionName>
//		<item>100</item>
//		<item>200</item>
//		<item>300</item>
//	</CollectionName>
//	(...)
void XMLObject::MapMember(GArray *pIntegerArray,const char *pzElementName,const char *pNestedInTag, const char *pzTranslationMapIn, const char *pzTranslationMapOut,int nTranslationFlags)
{
	if(m_bCountingMemberMaps)
	{
		GetMemberMapCount(1);
		return;
	}
	MapMember(pIntegerArray,pzElementName,&gGArrayHandler,pNestedInTag,pzTranslationMapIn,pzTranslationMapOut,nTranslationFlags);
}
void XMLObject::MapMember(void *pIntegerArray,	const char *pzElementName,IntegerArrayAbstraction *pHandler,
							const char *pNestedInTag,const char *pzTranslationMapIn, const char *pzTranslationMapOut,int nTranslationFlags)
{
	if(m_bCountingMemberMaps)
	{
		GetMemberMapCount(1);
		return;
	}
	if ( pNestedInTag )
	{
		XMLRelationshipWrapper *pRelation = new XMLRelationshipWrapper( pNestedInTag );
		if (!m_pRelationShipWrappers)
		{
			m_pRelationShipWrappers = new GList();
		}
		
		m_pRelationShipWrappers->AddLast( pRelation );
		pRelation->AddReference( pIntegerArray, pzElementName, pHandler,pzTranslationMapIn,pzTranslationMapOut,nTranslationFlags );

		MemberDescriptor *pNewMD = 0;
		if ( !(m_nBehaviorFlags & NO_MEMBER_MAP_PRECOUNT) )
		{
			pNewMD = new( ((char *)m_pMemberDescriptorArray) + (sizeof(MemberDescriptor) * m_nMappedCount++)  ) MemberDescriptor(this, pNestedInTag,pRelation);
		}
		else
		{
			pNewMD = ::new MemberDescriptor(this, pNestedInTag,pRelation);
		}
		RegisterMember(pNewMD);
	}
	else
	{
		MemberDescriptor *pNewMD = 0;
		if ( !(m_nBehaviorFlags & NO_MEMBER_MAP_PRECOUNT) )
		{
			pNewMD = new( ((char *)m_pMemberDescriptorArray) + (sizeof(MemberDescriptor) * m_nMappedCount++)  ) MemberDescriptor(this,pzElementName,pIntegerArray,pHandler,pzTranslationMapIn,pzTranslationMapOut,nTranslationFlags);
		}
		else
		{
			pNewMD = ::new MemberDescriptor(this,pzElementName,pIntegerArray,pHandler,pzTranslationMapIn,pzTranslationMapOut,nTranslationFlags);
		}
		RegisterMember(pNewMD);
	}
}

void XMLObject::MapMember(GString *pValue,const char *pTag, const char *pzTranslationMapIn, const char *pzTranslationMapOut,int nTranslationFlags)
{
	if(m_bCountingMemberMaps)
	{
		GetMemberMapCount(1);
		return;
	}
	XMLObject::MapMember(pValue,pTag,&gGenericStrHandler,pzTranslationMapIn,pzTranslationMapOut,nTranslationFlags);
}
void XMLObject::MapMember(void *pValue,const char *pTag,StringAbstraction *pHandler,const char *pzTranslationMapIn,const char *pzTranslationMapOut,int nTranslationFlags)
{
	if(m_bCountingMemberMaps)
	{
		GetMemberMapCount(1);
		return;
	}
	if (pValue)
	{
		MemberDescriptor *pNewMD = 0;
		if ( !(m_nBehaviorFlags & NO_MEMBER_MAP_PRECOUNT) )
		{
			pNewMD = new( ((char *)m_pMemberDescriptorArray) + (sizeof(MemberDescriptor) * m_nMappedCount++)  ) MemberDescriptor(this,pTag,pValue,pHandler,pzTranslationMapIn,pzTranslationMapOut,nTranslationFlags);
		}
		else
		{
			pNewMD = ::new MemberDescriptor(this,pTag,pValue,pHandler,pzTranslationMapIn,pzTranslationMapOut,nTranslationFlags);
		}
		RegisterMember(pNewMD);
	}
	else
	{
		char szTemp[64];
		sprintf(szTemp, "Cannot map %s to Null member.", pTag);
		TRACE_ERROR(szTemp);
	}
};



// Map a single sub-object using a specific tag, possibly not the
// tag supplied by the object in it's DECLARE_FACTROY()
void XMLObject::MapMember(XMLObject *pObj,	const char *pDefaultTagOverride/* = 0*/, 
											const char *pNestedInTag/* = 0*/ )
{
	if(m_bCountingMemberMaps)
	{
		GetMemberMapCount(1);
		return;
	}
	if (!pDefaultTagOverride && pObj)
	{
		pDefaultTagOverride = pObj->GetObjectTag();
	}

	if ( pNestedInTag )
	{
		XMLRelationshipWrapper *pRelation = new XMLRelationshipWrapper( pNestedInTag );
		if (!m_pRelationShipWrappers)
		{
			m_pRelationShipWrappers = new GList();
		}
		m_pRelationShipWrappers->AddLast( pRelation );
		pRelation->AddReference( pDefaultTagOverride, pObj );

		MemberDescriptor *pNewMD = 0;
		if ( !(m_nBehaviorFlags & NO_MEMBER_MAP_PRECOUNT) )
		{
			pNewMD = new( ((char *)m_pMemberDescriptorArray) + (sizeof(MemberDescriptor) * m_nMappedCount++)  ) MemberDescriptor(this, pNestedInTag,pRelation);
		}
		else
		{
			pNewMD = ::new MemberDescriptor(this, pNestedInTag,pRelation);
		}
		RegisterMember(pNewMD);
	}
	else if (pObj)
	{
		MemberDescriptor *pNewMD = 0;
		if ( !(m_nBehaviorFlags & NO_MEMBER_MAP_PRECOUNT) )
		{
			pNewMD = new( ((char *)m_pMemberDescriptorArray) + (sizeof(MemberDescriptor) * m_nMappedCount++)  ) MemberDescriptor(this, pDefaultTagOverride,pObj);
		}
		else
		{
			pNewMD = ::new MemberDescriptor(this, pDefaultTagOverride,pObj);
		}
		RegisterMember(pNewMD);
	}
	else
	{
		char szTemp[64];
		sprintf(szTemp, "Cannot map %s to Null member.", pDefaultTagOverride );
		TRACE_ERROR(szTemp);
	}
};

// Maps a member object pointer to an XML Tag.  This is ideal to map 
// object relationships that are not containment.  For example an object
// that contains a pointer to an object that it does not "own".
void XMLObject::MapMember(XMLObject **pObj, 
							 const char *pzTag, 
							 const char *pNestedInTag/* = 0 */, 
							 ObjectFactory pFactory/* = 0*/)
{
	if(m_bCountingMemberMaps)
	{
		GetMemberMapCount(1);
		return;
	}
	if ( pNestedInTag )
	{
		XMLRelationshipWrapper *pRelation = new XMLRelationshipWrapper( pNestedInTag );
		if (!m_pRelationShipWrappers)
		{
			m_pRelationShipWrappers = new GList();
		}
		m_pRelationShipWrappers->AddLast( pRelation );
		pRelation->AddReference( pzTag, pObj, pFactory );

		MemberDescriptor *pNewMD = 0;
		if ( !(m_nBehaviorFlags & NO_MEMBER_MAP_PRECOUNT) )
		{
			pNewMD = new( ((char *)m_pMemberDescriptorArray) + (sizeof(MemberDescriptor) * m_nMappedCount++)  ) MemberDescriptor(this, pNestedInTag,pRelation);
		}
		else
		{
			pNewMD = ::new MemberDescriptor(this, pNestedInTag,pRelation);
		}
		RegisterMember(pNewMD);
	}
	else
	{
		MemberDescriptor *pNewMD = 0;
		if ( !(m_nBehaviorFlags & NO_MEMBER_MAP_PRECOUNT) )
		{
			pNewMD = new( ((char *)m_pMemberDescriptorArray) + (sizeof(MemberDescriptor) * m_nMappedCount++)  ) MemberDescriptor(this, pzTag,pObj,pFactory);
		}
		else
		{
			pNewMD = ::new MemberDescriptor(this, pzTag,pObj,pFactory);
		}
		RegisterMember(pNewMD);
	}
}


void XMLObject::MapMember(GList *pList,const char *pObjectTag,const char *pNestedInTag, ObjectFactory pFactory )
{
	MapMember(pList,pObjectTag,&gGListHandler,pNestedInTag,pFactory);
}

void XMLObject::MapMember(void *pList,const char *pObjectTag,ListAbstraction *pHandler,
								const char *pNestedInTag/* = 0*/, ObjectFactory pFactory/* = 0*/ )
{
	if(m_bCountingMemberMaps)
	{
		GetMemberMapCount(1);
		return;
	}
	int bFirstMap = IsFirstMap(pList);
	if (pList)
	{
		if ( pNestedInTag )
		{
			XMLRelationshipWrapper *pRelation = new XMLRelationshipWrapper( pNestedInTag );
			if (!m_pRelationShipWrappers)
			{
				m_pRelationShipWrappers = new GList();
			}
			m_pRelationShipWrappers->AddLast(pRelation);
			pRelation->AddReference(pObjectTag,pList,pHandler,pFactory,bFirstMap);

			MemberDescriptor *pNewMD = 0;
			if ( !(m_nBehaviorFlags & NO_MEMBER_MAP_PRECOUNT) )
			{
				pNewMD = new( ((char *)m_pMemberDescriptorArray) + (sizeof(MemberDescriptor) * m_nMappedCount++)  ) MemberDescriptor(this,pNestedInTag,pRelation);
			}
			else
			{
				pNewMD = ::new MemberDescriptor(this,pNestedInTag,pRelation);
			}
			RegisterMember(pNewMD);
		}
		else
		{
			MemberDescriptor *pNewMD = 0;
			if ( !(m_nBehaviorFlags & NO_MEMBER_MAP_PRECOUNT) )
			{
				pNewMD = new( ((char *)m_pMemberDescriptorArray) + (sizeof(MemberDescriptor) * m_nMappedCount++)  ) MemberDescriptor(this,pObjectTag,pList,pHandler,pFactory);
			}
			else
			{
				pNewMD = ::new MemberDescriptor(this,pObjectTag,pList,pHandler,pFactory);
			}
			pNewMD->m_bFirstMap = bFirstMap;
			RegisterMember(pNewMD);
		}
	}
	else
	{
		char szTemp[64];
		sprintf(szTemp, "Cannot map Null list to tag: %s", pObjectTag);
		TRACE_ERROR(szTemp);
	}
}

void XMLObject::MapMember(__int64 *pValue,const char *pTag, const char *pzTranslationMapIn, const char *pzTranslationMapOut,int nTranslationFlags)
{
	if(m_bCountingMemberMaps)
	{
		GetMemberMapCount(1);
		return;
	}
	if (pValue)
	{
		MemberDescriptor *pNewMD = 0;
		if ( !(m_nBehaviorFlags & NO_MEMBER_MAP_PRECOUNT) )
		{
			pNewMD = new( ((char *)m_pMemberDescriptorArray) + (sizeof(MemberDescriptor) * m_nMappedCount++)  ) MemberDescriptor(this, pTag,pValue,pzTranslationMapIn,pzTranslationMapOut,nTranslationFlags);
		}
		else
		{
			pNewMD = ::new MemberDescriptor(this, pTag,pValue,pzTranslationMapIn,pzTranslationMapOut,nTranslationFlags);
		}
		RegisterMember(pNewMD);
	}
	else
	{
		char szTemp[64];
		sprintf(szTemp, "Cannot map %s to Null member.", pTag);
		TRACE_ERROR(szTemp);
	}
}

void XMLObject::MapMember(long *pValue,const char *pTag, const char *pzTranslationMapIn, const char *pzTranslationMapOut,int nTranslationFlags)
{
	if(m_bCountingMemberMaps)
	{
		GetMemberMapCount(1);
		return;
	}
	if (pValue)
	{
		MemberDescriptor *pNewMD = 0;
		if ( !(m_nBehaviorFlags & NO_MEMBER_MAP_PRECOUNT) )
		{
			pNewMD = new( ((char *)m_pMemberDescriptorArray) + (sizeof(MemberDescriptor) * m_nMappedCount++)  ) MemberDescriptor(this, pTag,pValue,pzTranslationMapIn,pzTranslationMapOut,nTranslationFlags);
		}
		else
		{
			pNewMD = ::new MemberDescriptor(this, pTag,pValue,pzTranslationMapIn,pzTranslationMapOut,nTranslationFlags);
		}
		RegisterMember(pNewMD);
	}
	else
	{
		char szTemp[64];
		sprintf(szTemp, "Cannot map %s to Null member.", pTag);
		TRACE_ERROR(szTemp);
	}
}

void XMLObject::MapMember(double *pValue,const char *pTag)
{
	if(m_bCountingMemberMaps)
	{
		GetMemberMapCount(1);
		return;
	}
	if (pValue)
	{
		MemberDescriptor *pNewMD = 0;
		if ( !(m_nBehaviorFlags & NO_MEMBER_MAP_PRECOUNT) )
		{
			pNewMD = new( ((char *)m_pMemberDescriptorArray) + (sizeof(MemberDescriptor) * m_nMappedCount++)  ) MemberDescriptor(this, pTag,pValue);
		}
		else
		{
			pNewMD = ::new MemberDescriptor(this, pTag,pValue);
		}
		RegisterMember(pNewMD);
	}
	else
	{
		char szTemp[64];
		sprintf(szTemp, "Cannot map %s to Null member.", pTag);
		TRACE_ERROR(szTemp);
	}
}


void XMLObject::MapMember(int *pValue,const char *pTag, const char *pzTranslationMapIn, const char *pzTranslationMapOut,int nTranslationFlags)
{
	if(m_bCountingMemberMaps)
	{
		GetMemberMapCount(1);
		return;
	}
	if (pValue)
	{
		MemberDescriptor *pNewMD = 0;
		if ( !(m_nBehaviorFlags & NO_MEMBER_MAP_PRECOUNT) )
		{
			pNewMD = new( ((char *)m_pMemberDescriptorArray) + (sizeof(MemberDescriptor) * m_nMappedCount++)  ) MemberDescriptor(this, pTag,pValue,pzTranslationMapIn,pzTranslationMapOut,nTranslationFlags);
		}
		else
		{
			pNewMD = ::new MemberDescriptor(this, pTag,pValue,pzTranslationMapIn,pzTranslationMapOut,nTranslationFlags);
		}
		RegisterMember(pNewMD);
	}
	else
	{
		char szTemp[64];
		sprintf(szTemp, "Cannot map %s to Null member.", pTag);
		TRACE_ERROR(szTemp);
	}
}

void XMLObject::MapMember(short *pValue,const char *pTag, const char *pzTranslationMapIn, const char *pzTranslationMapOut,int nTranslationFlags)
{
	if(m_bCountingMemberMaps)
	{
		GetMemberMapCount(1);
		return;
	}
	if (pValue)
	{
		MemberDescriptor *pNewMD = 0;
		if ( !(m_nBehaviorFlags & NO_MEMBER_MAP_PRECOUNT) )
		{
			pNewMD = new( ((char *)m_pMemberDescriptorArray) + (sizeof(MemberDescriptor) * m_nMappedCount++)  ) MemberDescriptor(this, pTag,pValue,pzTranslationMapIn,pzTranslationMapOut,nTranslationFlags);
		}
		else
		{
			pNewMD = ::new MemberDescriptor(this, pTag,pValue,pzTranslationMapIn,pzTranslationMapOut,nTranslationFlags);
		}
		RegisterMember(pNewMD);
	}
	else
	{
		char szTemp[64];
		sprintf(szTemp, "Cannot map %s to Null member.", pTag);
		TRACE_ERROR(szTemp);
	}
}


#ifndef _NO_XMLF_BOOL
void XMLObject::MapMember(bool *pValue,const char *pTag, int nBoolReadability)
{
	if(m_bCountingMemberMaps)
	{
		GetMemberMapCount(1);
		return;
	}
	if (pValue)
	{
		MemberDescriptor *pNewMD = 0;
		if ( !(m_nBehaviorFlags & NO_MEMBER_MAP_PRECOUNT) )
		{
			pNewMD = new( ((char *)m_pMemberDescriptorArray) + (sizeof(MemberDescriptor) * m_nMappedCount++)  ) MemberDescriptor(this, pTag, pValue, nBoolReadability);
		}
		else
		{
			pNewMD = ::new MemberDescriptor(this, pTag, pValue, nBoolReadability);
		}
		RegisterMember(pNewMD);
	}
	else
	{
		char szTemp[64];
		sprintf(szTemp, "Cannot map %s to Null member.", pTag);
		TRACE_ERROR(szTemp);
	}
}
#endif // _NO_XMLF_BOOL


void XMLObject::MapMember(char *pValue,const char *pTag, int nMaxLen, const char *pzTranslationMapIn, const char *pzTranslationMapOut,int nTranslationFlags)
{
	if(m_bCountingMemberMaps)
	{
		GetMemberMapCount(1);
		return;
	}
	if (pValue)
	{
		MemberDescriptor *pNewMD = 0;
		if ( !(m_nBehaviorFlags & NO_MEMBER_MAP_PRECOUNT) )
		{
			pNewMD = new( ((char *)m_pMemberDescriptorArray) + (sizeof(MemberDescriptor) * m_nMappedCount++)  ) MemberDescriptor(this, pTag, pValue, nMaxLen, pzTranslationMapIn, pzTranslationMapOut,nTranslationFlags);
		}
		else
		{
			pNewMD = ::new MemberDescriptor(this, pTag, pValue, nMaxLen, pzTranslationMapIn, pzTranslationMapOut,nTranslationFlags);
		}
		RegisterMember(pNewMD);
	}
	else
	{
		char szTemp[64];
		sprintf(szTemp, "Cannot map %s to Null member.", pTag);
		TRACE_ERROR(szTemp);
	}
}

void XMLObject::MapMember(char *pValue,const char *pTag)
{
	if(m_bCountingMemberMaps)
	{
		GetMemberMapCount(1);
		return;
	}
	if (pValue)
	{
		// note: the 'x' only keeps the method signature name mangling from being ambigous

		MemberDescriptor *pNewMD = 0;
		if ( !(m_nBehaviorFlags & NO_MEMBER_MAP_PRECOUNT) )
		{
			pNewMD = new( ((char *)m_pMemberDescriptorArray) + (sizeof(MemberDescriptor) * m_nMappedCount++)  ) MemberDescriptor(this, pTag, pValue, 'x');
		}
		else
		{
			pNewMD = ::new MemberDescriptor(this, pTag, pValue, 'x');
		}
		RegisterMember(pNewMD);
	}
	else
	{
		char szTemp[64];
		sprintf(szTemp, "Cannot map %s to Null member.", pTag);
		TRACE_ERROR(szTemp);
	}
}


int XMLObject::SetMappedAttribute(const char *pTag,const char *pzValue, __int64 nValueLen)
{
	if(!m_EntryRoot)
	{
		LoadMemberMappings();
	};

	if (m_pMappedAttributeHash)
	{
		GString strUpper(pTag);
		strUpper.MakeUpper();
		MemberDescriptor *pMD = (MemberDescriptor *)m_pMappedAttributeHash->Lookup(strUpper);
		if (pMD)
		{
			pMD->Set(pzValue, nValueLen, 1, 1);
			return 1;
		}
	}
	return 0;
}

void XMLObject::MapAttribute(GString *pstrValue,const char *pTag, const char *pzTranslationMapIn, const char *pzTranslationMapOut,int nTranslationFlags)
{
	if(m_bCountingMemberMaps)
	{
		GetMemberMapCount(1);
		return;
	}
	MapAttribute(pstrValue,pTag,&gGenericStrHandler,pzTranslationMapIn,pzTranslationMapOut,nTranslationFlags);
}

void XMLObject::MapAttribute(void *pValue,const char *pTag,StringAbstraction *pHandler, const char *pzTranslationMapIn, const char *pzTranslationMapOut,int nTranslationFlags)
{
	if (!m_pMappedAttributeHash)
	{		
		// Create the hash with a small prime number
		m_pMappedAttributeHash = new GHash(7);
	}

	if (pValue)
	{
		MemberDescriptor *pNewMD = 0;
		if ( !(m_nBehaviorFlags & NO_MEMBER_MAP_PRECOUNT) )
		{
			pNewMD = new( ((char *)m_pMemberDescriptorArray) + (sizeof(MemberDescriptor) * m_nMappedCount++)  ) MemberDescriptor(this,pTag,pValue,pHandler,pzTranslationMapIn,pzTranslationMapOut,nTranslationFlags);
		}
		else
		{
			pNewMD = ::new MemberDescriptor(this,pTag,pValue,pHandler,pzTranslationMapIn,pzTranslationMapOut,nTranslationFlags);
		}

		GString strUpper(pTag);
		strUpper.MakeUpper();
		m_pMappedAttributeHash->Insert(strUpper, pNewMD);
	}
	else
	{
		char szTemp[64];
		sprintf(szTemp, "Cannot map %s to Null member.", pTag);
		TRACE_ERROR(szTemp);
	}
};

void XMLObject::MapAttribute(int *pValue,const char *pTag, const char *pzTranslationMapIn, const char *pzTranslationMapOut,int nTranslationFlags)
{
	if (!m_pMappedAttributeHash)
	{		
		// Create the hash with a small prime number
		m_pMappedAttributeHash = new GHash(7);
	}
	if (pValue)
	{
		MemberDescriptor *pNewMD = 0;
		if ( !(m_nBehaviorFlags & NO_MEMBER_MAP_PRECOUNT) )
		{
			pNewMD = new( ((char *)m_pMemberDescriptorArray) + (sizeof(MemberDescriptor) * m_nMappedCount++)  ) MemberDescriptor(this, pTag,pValue,pzTranslationMapIn,pzTranslationMapOut,nTranslationFlags);
		}
		else
		{
			pNewMD = ::new MemberDescriptor(this, pTag,pValue,pzTranslationMapIn,pzTranslationMapOut,nTranslationFlags);
		}

		GString strUpper(pTag);
		strUpper.MakeUpper();
		m_pMappedAttributeHash->Insert(strUpper, pNewMD);
	}
	else
	{
		char szTemp[64];
		sprintf(szTemp, "Cannot map %s to Null member.", pTag);
		TRACE_ERROR(szTemp);
	}
}


void XMLObject::AddMemberAttribute( const char *pzMemberTag, const char * pzName, int nValue )
{
	MemberDescriptor *pDesc = GetEntry( pzMemberTag );
	if (pDesc)
	{
		pDesc->AddAttribute( pzName, nValue );
	}
	else
	{
		TRACE_ERROR("Member not found - cannot add attributes");
	}
}


void XMLObject::AddMemberAttribute( void * pMemberAddress,  const char * pzName, int nValue )
{
	MemberDescriptor *pDesc = GetEntry( pMemberAddress );
	if (pDesc)
	{
		pDesc->AddAttribute( pzName, nValue );
	}
	else
	{
		TRACE_ERROR("Member not found - cannot add attributes");
	}
}

void XMLObject::AddMemberAttribute( const char *pzMemberTag, const char * pzName, const char * pzValue )
{
	MemberDescriptor *pDesc = GetEntry( pzMemberTag );
	if (pDesc)
	{
		pDesc->AddAttribute( pzName, pzValue );
	}
	else
	{
		TRACE_ERROR("Member not found - cannot add attributes");
	}
}

void XMLObject::AddMemberAttribute( void * pMemberAddress,   const char * pzName, const char * pzValue )
{
	MemberDescriptor *pDesc = GetEntry( pMemberAddress );
	if (pDesc)
	{
		pDesc->AddAttribute( pzName, pzValue );
	}
	else
	{
		TRACE_ERROR("Member not found - cannot add attributes");
	}
}





void XMLObject::AddAttribute( const char * pzName, int nValue, int nUpdate/*=0*/ )
{
	GString strTemp;
	strTemp.Format("%d", (int)nValue);
	AddAttribute( pzName, (const char *)strTemp, nUpdate );
}



void XMLObject::AddAttribute( const char * pzName, const char * pzValue, int nUpdate/*=0*/ )
{
	if (pzName && pzValue)
	{
		// assign the value to the object member previously mapped with MapAttribute()
		if (!SetMappedAttribute(pzName,pzValue, strlen(pzValue))) // this line added 8/14/2003
		{
			// if it was not mapped, create a new storage list if necessary
			if (!m_pAttributes)
			{
				m_pAttributes = new XMLAttributeList;
			}
			// then append the value to the list
			m_pAttributes->AddAttribute( pzName, pzValue, nUpdate );
		}
	}
}

int XMLObject::FindAttribute( const char *pzAttName, GString &strDest )
{
	int pRet = 0;
	if (m_pAttributes)
	{
		// check in the "Dynamic" or "Unmapped" attributes first
		const char *pAttribValue = m_pAttributes->FindAttribute( pzAttName );
		if (pAttribValue)
		{
			strDest = pAttribValue;
			pRet = 1;
		}
	}
	if (!pRet)  // then look in m_pMappedAttributeHash if we didnt find it in m_pAttributes
	{
		GString strUpper(pzAttName);
		strUpper.MakeUpper();
		MemberDescriptor *pMD = (MemberDescriptor *)m_pMappedAttributeHash->Lookup(strUpper);
		if (pMD)
		{
			pMD->GetMemberValue( strDest );
			pRet = 1;
		}
	}
	return pRet;
}

const char *XMLObject::FindAttribute( const char *pzAttName )
{
	static GString strDest;
	if ( FindAttribute( pzAttName, strDest ) )
		return strDest;

	return 0; // so you can distinguish between "Empty"(aka "") and "Never Assigned"(aka null)
}


//
// CalculateObjectState() updates an objects MemberStateSummary.
// This method checks the state of all members in an object.  If any
// member is dirty then the object's MemberStateSummary becomes dirty.
// If no dirty members are found we do the same for cached members by
// setting the object's MemberStateSummary to cached.  DIRTY overrides
// CACHED.  If a member is dirty, the object summary is set to dirty.
// The fact that it is cached is irrelevant if the member is dirty.
int XMLObject::CalculateObjectState( MemberDescriptor* btRoot )
{
	if (!btRoot)
		btRoot = m_EntryRoot;

	if( btRoot && btRoot->Left )
	{
		int nRet = CalculateObjectState( btRoot->Left );
		if (nRet)
			return nRet;
	}

	// for each member that maps to an object
	if ( btRoot && btRoot->IsSubObject() )
	{
		// and for each object it represents
		xmlObjectIterator iter = 0;
		XMLObject *pObj = btRoot->GetFirstContainedObject(&iter);
		while (pObj)
		{
			if ( pObj->getMemberStateSummary( DATA_DIRTY ) )
			{
				// stop recursion, carry dirty state up to the parent object
				btRoot->ReleaseObjectIterator(iter);  //12/23/2009
				return DATA_DIRTY;
			}
			if ( pObj->getMemberStateSummary( DATA_CACHED ) )
			{
				// stop recursion, carry cached state up to the parent object
				btRoot->ReleaseObjectIterator(iter);  // 12/23/2009
				return DATA_CACHED;
			}
			
			int nBits = pObj->CalculateObjectState();
			if ( nBits )
			{
				// this object carries the state summary of one of it's members
				pObj->setMemberStateSummary( nBits );
				btRoot->ReleaseObjectIterator(iter);  // 12/23/2009
				return nBits;
			}
			pObj = btRoot->GetNextContainedObject(iter);
		}
	}

	if( btRoot && btRoot->Right )
	{
		// carry the state up to owning object
		int nRet = CalculateObjectState( btRoot->Right );
		if (nRet)
			return nRet;
	}
	return 0;
}

void XMLObject::FromXMLFile(const char *pzFileName, XMLObject *pSecondaryHandler /*= 0*/,const char *pzErrorSubSystem /*= 0*/)
{
	GString strXMLInput;
	strXMLInput.FromFile(pzFileName);
	FromXML((const char *)strXMLInput, pSecondaryHandler, pzErrorSubSystem);
}

void XMLObject::FromXML(const char *pzXML, 
						XMLObject *pSecondaryHandler /*= 0*/,
						const char *pzErrorSubSystem /*= 0*/)
{
	XMLObjectFactory factory ( (const char *)pzXML , pzErrorSubSystem);
	factory.extractObjects(this,pSecondaryHandler);
}


int XMLObject::FromXMLX(const char *pzXML, GString *pErrorDescriptionDestination/*=0*/, XMLObject *pSecondaryHandler /*= 0*/)
{

	try
	{
		XMLObjectFactory factory ( (const char *)pzXML , 0 );
		factory.extractObjects(this,pSecondaryHandler);
		return 1;
	}
	catch(GException &e)
	{
		if (pErrorDescriptionDestination)
			*pErrorDescriptionDestination  << e.GetDescription();
	}
	catch(...)
	{
		ASSERT(FALSE);
	}
	
	// if we got here, it failed and we caught an exception
	return 0;
}

int XMLObject::FromXMLFileX(const char *pzFileName, GString *pErrorDescriptionDestination/*= 0*/, XMLObject *pSecondaryHandler/* = 0*/)
{
	try
	{
		FromXMLFile(pzFileName, pSecondaryHandler, 0);
		return 1;
	}
	catch(GException &e)
	{
		if (pErrorDescriptionDestination)
			*pErrorDescriptionDestination  << e.GetDescription() << " (" << e.GetError() << ")";;
	}
	catch(...)
	{
		ASSERT(FALSE);
	}
	
	// if we got here, it failed and we caught an exception
	return 0;
}


bool XMLObject::ToXMLFile(__int64 nPreAllocateBytes, const char *pzPathAndFileName,bool bThrowOnFail/* = 1*/,  int nAppend/* = 0*/,unsigned int nSerializeFlags /*= FULL_SERIALIZE*/)
{
	GString strDest(nPreAllocateBytes);

	int nRet = 0; // default to failure
	try
	{
		ToXML(&strDest,0, 0, nSerializeFlags );
		if (nAppend)
			nRet = strDest.ToFileAppend(pzPathAndFileName,bThrowOnFail);
		else
			nRet = strDest.ToFile(pzPathAndFileName,bThrowOnFail);
	}
	catch(GException &e)
	{
		if (bThrowOnFail)
		{
			// Failed to serialize object [%s]
			e.AddErrorDetail("XML Object", 1, GetObjectType());
			throw e;
		}
	}
	return (nRet) ? true : false;
}

bool XMLObject::ToXMLFile(const char *pzPathAndFileName, int nTabs, const char *TagOverride, int nSerializeFlags, int nEstimatedBytes )
{
	GString strDest(nEstimatedBytes);
	ToXML(&strDest,nTabs, TagOverride, nSerializeFlags);
	int nRet = strDest.ToFile(pzPathAndFileName,0);
	return (nRet) ? true : false;
}


const char *XMLObject::ToXML(__int64 nPreAllocSize /*=4096*/, unsigned int nSerializeFlags /*= FULL_SERIALIZE*/)
{
	if (!m_pToXMLStorage)
		m_pToXMLStorage = new GString(nPreAllocSize);
	m_pToXMLStorage->Empty();
	ToXML(m_pToXMLStorage, 0, 0, nSerializeFlags);
	return *m_pToXMLStorage;
}

void XMLObject::ReStoreState(long oid)
{
	GString strOid;
	strOid.Format("%ld",(int)oid);
	ReStoreState((const char *)strOid);
}
void XMLObject::ReStoreState(int oid)
{
	GString strOid;
	strOid.Format("%d",(int)oid);
	ReStoreState((const char *)strOid);
}
void XMLObject::ReStoreState(const char * oid)
{
	GString *pState = cacheManager.findState(oid,GetObjectType());
	if (pState)
	{
		// since this is 'our' private XML it cannot fail to parse so an exception will 'never' be 
		// caught here,  but since FromXML can throw exceptions, we'll make sure about never
		try
		{
			FromXML(pState->StrVal());
		}
		catch(...)
		{
		}
	}
}

void XMLObject::StoreState()
{
	if (!SerializeObject())
		return;

	GString StrXML;
	StackFrameCheck Stack( this, 0 );
	if ( !(m_nBehaviorFlags & HAS_EXECUTED_MEMBER_MAP) )
		LoadMemberMappings();
	
	WriteObjectBeginTag( StrXML, 0, 0, 0 );
	// hang on to the length - at the end of the ...<StartTag>
	// because if that length does not grow prior to WriteObjectEndTag() below - it can be "short hand terminated" in the XML
	// For example, <brian/> would otherwise be <brian></brian>
	__int64 nEndOfBeginTag = StrXML.GetLength();

	// If the object has a value that is NOT contained in a member
	// variable, add it to the XML stream here.
	if ( m_pDataHandler )
	{
		m_pDataHandler->AppendObjectValue( StrXML );
	}
																				
	// recursively stream out member data
	int nSerializeFlags = USE_OBJECT_MARKERS | FULL_SERIALIZE;
	GenerateMappedXML( m_EntryRoot, StrXML, 0, &Stack, nSerializeFlags);

	WriteObjectEndTag( StrXML, 0, 0, 0, nEndOfBeginTag );
	cacheManager.enterState(getOID(),GetObjectType(),StrXML);
}

//<?xml version="1.0" encoding="UTF-8"?>
//<!DOCTYPE device SYSTEM "hpzscwn7.dtd">

//<?xml version="1.0" encoding="utf-8"?>
//<!DOCTYPE dotfuscatorMap SYSTEM "http://www.preemptive.com/dotfuscator/dtd/dotfuscatorMap_v1.2.dtd">

bool XMLObject::ToXML(GString *xml, int nTabs, const char *TagOverride, unsigned int nSerializeFlags, StackFrameCheck *pStack/* = 0*/, const char *pzSubsetOfObjectByTagName/* = 0*/)
{
	if (!SerializeObject())														
		return true;															
	StackFrameCheck Stack( this, pStack );										
	if ( !(m_nBehaviorFlags & HAS_EXECUTED_MEMBER_MAP) )
		LoadMemberMappings();													
																				
	// WRITE_DOCTYPE
	if ( (( nSerializeFlags & INCLUDE_DOCTYPE_DECLARATION ) != 0) && Stack.getStackSize() == 1)
	{
		(*xml) << "<!DOCTYPE ";
		if ( TagOverride )
			(*xml) << TagOverride;
		else
			(*xml) << GetObjectTag();
		(*xml) << ">";
		
		// adds a CR || CR/LF depending on the UNIX_LINEFEEDS / WINDOWS_LINEFEEDS
		MemberDescriptor::TabifyXML(*xml,0,nSerializeFlags);
		
	}
																				
	WriteObjectBeginTag( *xml, nTabs, TagOverride, nSerializeFlags );
	// hang on to the length - at the end of the ...<StartTag>
	// because if that length does not grow prior to WriteObjectEndTag() below - it can be "short hand terminated" in the XML
	// For example, <brian/> would otherwise be <brian></brian>
	__int64 nEndOfBeginTag = xml->GetLength();
																				

	// If the object has a value that is NOT contained in a member variable, add it to the XML stream here.
	if ( m_pDataHandler )
	{
		if ( ( nSerializeFlags & EXCLUDE_OBJECT_VALUE ) == 0)
		{
			if ( !(m_nBehaviorFlags & NO_OBJECT_DATA) )
			{
				m_pDataHandler->AppendObjectValue( *xml );
			}
		}
	}


	if (pzSubsetOfObjectByTagName)
	{
		GenerateSubsetXML( m_EntryRoot, *xml, nTabs+1, &Stack, nSerializeFlags, pzSubsetOfObjectByTagName); 
	}
	else
	{
		// recursivly stream out member data 
		if ( ( nSerializeFlags & ORDER_MEMBERS_ALPHABETICALLY ) != 0 )
			GenerateOrderedXML( m_EntryRoot, *xml, nTabs+1, &Stack, nSerializeFlags);
		else																		
			GenerateMappedXML( m_EntryRoot, *xml, nTabs+1, &Stack, nSerializeFlags);	
	}

	WriteObjectEndTag( *xml, nTabs, TagOverride, nSerializeFlags, nEndOfBeginTag );
																				
	// terminate if we're not recursed
	if ( Stack.getStackSize() == 1 )
		(*xml) << "\0";
	return true;
}



void XMLObject::FormatMemberToDump(MemberDescriptor *pMD, GString &strDest, int nTabs, StackFrameCheck *pStack, const char *pzPlace)
{
	pMD->IsDirty();
	pMD->IsNull();
	pMD->IsCached();

	GString strTypeDescription;
	GString strMemberValue;

	pMD->GetTypeName(strTypeDescription);
	const char *bValue = pMD->GetMemberValue(strMemberValue);
	GString strTabs((char)0x09, (short)nTabs);

	// if this member is a UserStringCollection || UserIntegerArray
	if (pMD->DataType == MemberDescriptor::UserStringCollection)
	{
		userStringListIterator iter;
		const char *pzEntry = pMD->m_DataAbstractor.pStrListHandler->getFirst(pMD->m_Member.pUserStringCollection,&iter);
		strDest << "\n" << (const char *)strTabs << "List Entry Name: " <<  pMD->strTagName << "\n" << (const char *)strTabs <<"Entry Values:\n";
		while (pzEntry)
		{
			strDest << (const char *)strTabs <<  pzEntry << "\n";
			pzEntry = pMD->m_DataAbstractor.pStrListHandler->getNext(pMD->m_Member.pUserStringCollection,iter);
		}
	}
	else if (pMD->DataType == MemberDescriptor::UserIntegerArray)
	{
		int nIndex = 0;
		int nIsValidIndex = 0;
		int nValue = pMD->m_DataAbstractor.pIntArrayHandler->getAt(pMD->m_Member.pUserIntegerArray,nIndex,&nIsValidIndex);
		strDest << "\n" << (const char *)strTabs << "Array Entry Name: " <<  pMD->strTagName << "\n" << (const char *)strTabs <<"Entry Values:\n";
		while(nIsValidIndex)
		{
			strDest << (const char *)strTabs <<  nValue << "\n";
			nIndex++;
			nValue = pMD->m_DataAbstractor.pIntArrayHandler->getAt(pMD->m_Member.pUserIntegerArray,nIndex,&nIsValidIndex);
		}
	}
	// if this member is an object or  collection of objects
	else if( !bValue )
	{
		// count of objects that this member represents
		__int64 nObjectCount = pMD->GetObjectContainmentCount();

		strDest << "\n";
		strDest << strTabs;
		strDest << "--------------------------------\n";
		strDest << strTabs;
		strDest << "Type  :";
		strDest << strTypeDescription;
		strDest << "\n";
		strDest << strTabs;
		strDest << "Tag   :";
		strDest << pMD->strTagName;
		strDest << "\n";
		strDest << strTabs;
		
		if ( pMD->DataType == MemberDescriptor::XMLObjList )
		{
			strDest << "\n" << strTabs << "Contains:" << nObjectCount << " items";
		}

		if (! pStack->isNestedStackFrame() )
		{
			xmlObjectIterator iter = 0;
			XMLObject *pObj = pMD->GetFirstContainedObject(&iter);
			while (pObj)
			{
				pObj->Dump( strDest, nTabs, pStack );
				pObj = pMD->GetNextContainedObject(iter);
			}
		}
	}
	// otherwise dump this simple datatype
	else
	{
		GString strState;
		if (pMD->IsDirty())
			strState << "Dirty | ";
		else
			strState << "Clean | ";

		if (pMD->IsNull())
			strState << "Null | ";
		else
			strState << "Valid | ";

		if (pMD->IsCached())
			strState << "Cached";
		else
			strState << "Uncached";

		strDest.FormatAppend(
				"\n%s--------------------------------\n%sType  :%s\n%sTag   :%s\n%sValue :%s\n%sState :(%s)\n%sKind  :%s",
				(const char *)strTabs,
				(const char *)strTabs,
				(const char *)strTypeDescription,
				(const char *)strTabs,
				(const char *)pMD->strTagName,
				(const char *)strTabs,
				(const char *)strMemberValue,
				(const char *)strTabs,
				(const char *)strState,
				(const char *)strTabs,
				pzPlace
				);
	}
}

void XMLObject::DumpMember( MemberDescriptor* btRoot, GString &strDest, int nTabs, StackFrameCheck *pStack)
{
	if( btRoot )
	{
		if( btRoot->Left )
			DumpMember( btRoot->Left, strDest, nTabs, pStack );

		// if a single list is mapped twice to the same object only dump it once
		if (btRoot->m_bFirstMap)
			FormatMemberToDump(btRoot,strDest,nTabs,pStack,"Element");

		if( btRoot->Right )
			DumpMember( btRoot->Right, strDest, nTabs, pStack );
	}
}


const char *XMLObject::Dump()
{
	if (!m_pToXMLStorage)
		m_pToXMLStorage = new GString(20000);
	m_pToXMLStorage->Empty();
	Dump(*m_pToXMLStorage);
	return *m_pToXMLStorage;
}

// Dump to a file
void XMLObject::Dump( const char *pzFileName, int bAppendToFile /* = 0*/ )
{
	GString str( 20000 ); 
	Dump(str);
	if (bAppendToFile)
		str.ToFileAppend(pzFileName,0);
	else
		str.ToFile(pzFileName,0);
}


// Dump to a GString
void XMLObject::Dump(GString &strDest, int nTabs/* = 0*/, StackFrameCheck *pStack/* = 0*/)
{
	StackFrameCheck Stack( this, pStack );
	if(!m_EntryRoot)
	{
		if (pStack)
		{
			return;
		}
		LoadMemberMappings();
	};

	GString strTabs((char)0x09, (short)nTabs);
	
	strDest << "\n";
	if (nTabs)
		strDest << strTabs;
	strDest << "Object Instance name: ";
	strDest << GetObjectType();
	strDest << "\n";
	if (nTabs)
		strDest << strTabs;
	strDest << "{\n";

	if (nTabs)
		strDest << strTabs;
	strDest << char(0x09);
	
	GString strTypeDescription;
	m_EntryRoot->GetTypeName(strTypeDescription);
	if (strTypeDescription != "List<XMLObject *>")
	{
		strDest << "string\tOID = ";
		if (m_oid && m_oid->Length()) 
			strDest << *m_oid;
		strDest << "\n";
		if (nTabs)
			strDest << strTabs;
		strDest << char(0x09);
		strDest << "string\tUpdateTime = ";
		if (m_TimeStamp && m_TimeStamp->Length())
			strDest << *m_TimeStamp;
		strDest << "\n";
		if (nTabs)
			strDest << strTabs;
		strDest << char(0x09);
		strDest << "\t\tReferences = ";
		// print ref count 
		strDest << m_nRefCount;
	}
	else
	{
		strDest << "Object container type";
	}
	if (nTabs)
		strDest << strTabs;
	strDest << char(0x09);


	GString strTabs2((char)0x09, (short)(nTabs+1));
	//////////////////////////////////////////////////////////////
	//	Dump Attributes
	if (m_pMappedAttributeHash) // mapped attributes
	{
		GHashIterator it(m_pMappedAttributeHash);
		while(it())
		{
			MemberDescriptor *pMD = (MemberDescriptor *)it++;
			FormatMemberToDump(pMD,strDest,nTabs+1,pStack, "Mapped Attribute");
		}
	}
	if (m_pAttributes) // un-mapped attributes
	{																					
		GListIterator Iter(&m_pAttributes->m_attrList);
		while ( Iter() )
		{
			XMLAttributeList::XMLAttributeNameValuePair *temp = (XMLAttributeList::XMLAttributeNameValuePair *)Iter++;
			strDest.FormatAppend(
					"\n%s--------------------------------\n%sType  :%s\n%sTag   :%s\n%sValue :%s\n%sState :(%s)\n%sKind  :%s",
					(const char *)strTabs2,
					(const char *)strTabs2,
					(const char *)"string",
					(const char *)strTabs2,
					(const char *)temp->strAttrName,
					(const char *)strTabs2,
					(const char *)temp->strAttrValue,
					(const char *)strTabs2,
					(const char *)"No state tracking",
					(const char *)strTabs2,
					"Un-mapped attribute"
					);


		}
	}
	if (m_pDataHandler)
	{
		GString strTemp;
		m_pDataHandler->AppendObjectValue(strTemp);
		strTemp.TrimLeftWS();
		strTemp.TrimRightWS();
		strDest.FormatAppend(
				"\n%s--------------------------------\n%sType  :%s\n%sTag   :%s\n%sValue :%s\n%sState :(%s)\n%sKind  :%s",
				(const char *)strTabs2,
				(const char *)strTabs2,
				(const char *)"string",
				(const char *)strTabs2,
				(const char *)"--None--",
				(const char *)strTabs2,
				(const char *)strTemp,
				(const char *)strTabs2,
				(const char *)"No state tracking",
				(const char *)strTabs2,
				"Object Value"
				);
	}
	//////////////////////////////////////////////////////////////
	

	// recurse
	DumpMember( m_EntryRoot, strDest, nTabs + 1, &Stack );

	strDest << "\n";
	if (nTabs)
		strDest << strTabs;
	strDest << "}";

	// terminate if we're not recursed
	if (Stack.getStackSize() == 1)
	{
		strDest << '\0';
	}
}


void XMLObject::WriteObjectEndTag(GString& xml, int nTabs, const char *TagOverride, unsigned int nSerializeFlags, __int64 nEndOfBeginTag )
{
	if ( (  nSerializeFlags & USE_OBJECT_MARKERS ) != 0)
	{
		// we short hand terminated in WriteObjectBeginTag()
		return;
	}
	
	if ( (nSerializeFlags & EXCLUDE_SHORT_TERMINATION) != 0  || xml.GetLength() != nEndOfBeginTag )
	{
		if ( m_EntryRoot )
		{
			if ( (  nSerializeFlags & NO_WHITESPACE ) == 0)
			{
				MemberDescriptor::TabifyXML(xml,nTabs);
			}
		}
		xml << "</";
		if ( TagOverride )
		{
			xml << TagOverride;
		}
		else
		{
			xml << GetObjectTag();
		}
		xml << '>';
	}
	else
	{
		xml.Insert(nEndOfBeginTag-1,"/");
	}
}

int XMLObject::WriteObjectBeginTag(GString& xml, int nTabs, const char *TagOverride, unsigned int nSerializeFlags )
{
	if (  nTabs && ((nSerializeFlags & NO_WHITESPACE) == 0)   )
		MemberDescriptor::TabifyXML(xml,nTabs);

	xml << '<';
	if ( TagOverride )
		xml << TagOverride;
	else
		xml << GetObjectTag();

	if ( (  nSerializeFlags & USE_OBJECT_MARKERS ) != 0)
	{
		xml << " oid=\"" << getOID() << "\"/>";
		StoreState();
		return 0;
	}
	else 
	{																					
		if ( (  nSerializeFlags & EXCLUDE_UNMAPPED_ATTRIBUTES ) == 0)
		{
			if (m_pAttributes) // unmapped attributes
			{
				GListIterator Iter(&m_pAttributes->m_attrList);
				while ( Iter() )
				{
					XMLAttributeList::XMLAttributeNameValuePair *temp =
						(XMLAttributeList::XMLAttributeNameValuePair *)Iter++;

					xml << " " << temp->strAttrName << "=\"";

					unsigned int nFlags = nSerializeFlags;
					nFlags |= INTERNAL_ATTRIBUTE_ESCAPE;
					xml.AppendEscapeXMLReserved(temp->strAttrValue,temp->strAttrValue.Length(),nFlags);
					xml << "\"";	
//					temp->strAttrValue << "\"";	
				}
			}
		}

		if ( (  nSerializeFlags & EXCLUDE_MAPPED_ATTRIBUTES ) == 0)
		{
			if (m_pMappedAttributeHash) // mapped attributes
			{
				GHashIterator it(m_pMappedAttributeHash);
				while(it())
				{
					MemberDescriptor *pMD = (MemberDescriptor *)it++;
					GString strMemberValue;
					pMD->GetMemberValue(strMemberValue);
					xml << " " << pMD->strTagName << "=\"";
					
					unsigned int nFlags = nSerializeFlags;
					nFlags |= INTERNAL_ATTRIBUTE_ESCAPE;
					xml.AppendEscapeXMLReserved(strMemberValue,strMemberValue.Length(),nFlags);
					xml << "\"";	
				}
			}
		}

	}
	xml << '>';
	return nTabs;
}


// have the derived class define itself
void XMLObject::LoadMemberMappings()
{
	// only build the member mapping tree once
	if ( !(m_nBehaviorFlags & HAS_EXECUTED_MEMBER_MAP) )
	{
		m_nBehaviorFlags |= HAS_EXECUTED_MEMBER_MAP;

		// first call MapXMLTagsToMembers() just to get a count of the members that are mapped
		if ( !(m_nBehaviorFlags & NO_MEMBER_MAP_PRECOUNT) )
		{
			// GetMemberMapCount() is explained in XMLObject.h - see the note
			int nArraySize = GetMemberMapCount(0);

			// if we have already counted the MapMember's in another object 
			// instance of this same type there is no need to do it again 
			if ( nArraySize == 0 )
			{
				// that means this 'extra' call to MapXMLTagsToMembers() is extremely infrequent.
				m_bCountingMemberMaps = 1;
				MapXMLTagsToMembers();
				m_bCountingMemberMaps = 0;
				
				nArraySize = GetMemberMapCount(0);
			}
			
			// now rather than calling new() once for each MapMember called, we will only call malloc() 1 time.
			// The single allocation will hold an array of 1 MemberDescriptor for each call to MapMember()
			m_pMemberDescriptorArray = malloc(sizeof(MemberDescriptor) * nArraySize);
		}
		// now call MapXMLTagsToMembers() again and now we will explicitly call the 'ctor 
		// [nArraySize] times on the memory at [m_pMemberDescriptorArray]
		MapXMLTagsToMembers();
	}
}


void XMLObject::UnMapMembers( MemberDescriptor* btRoot/* = 0*/, bool bRealloc/* =0 */)
{
	//	Delete and/or ~destruct all nodes in the MemberMapping tree for this object.
	//  If we are managing our own memory call the destructor on that memory space - we do not free() it.
	//	this does not free any associated memory owned by the members themselves.
	GListIterator Iter(&m_lstMembers);
	while (Iter())
	{
		// if the object specifies behavior NO_MEMBER_MAP_PRECOUNT the behavior can be more dynamic
		if ( (m_nBehaviorFlags & NO_MEMBER_MAP_PRECOUNT) )
		{
			delete (MemberDescriptor *)Iter++;
		}
		else
		{
			// explicit destructor call
			((MemberDescriptor *)Iter++)->MemberDescriptor::~MemberDescriptor();
		}
	}
	m_lstMembers.RemoveAll();


	m_EntryRoot = 0;
	m_nBehaviorFlags &= ~HAS_EXECUTED_MEMBER_MAP; // clear the flag
	m_nMappedCount = 0;

	if (bRealloc)
	{
		free(m_pMemberDescriptorArray);
		m_pMemberDescriptorArray = 0;
		GetMemberMapCount(2);
	}
}

void XMLObject::setObjectDataHandler( XMLObjectDataHandler * p )
{
	m_pDataHandler = p;
}


MemberDescriptor *XMLObject::HandleUnmappedMember( const char *pzTag )
{
	// The Object factory encountered a tag that was not mapped
	// and this virtual method has not been implemented in the derived class.
	return 0;
}


MemberDescriptor* XMLObject::GetEntry( const char * szTagToFind )
{ 
	// build the member mapping tree, if it has not already been done
	if ( !(m_nBehaviorFlags & HAS_EXECUTED_MEMBER_MAP) )  // this if() was added 11/11/2013 because it removed a stack call from the tokenizer to member variable assignment - it's faster.
		LoadMemberMappings();

	// search the map entry tree for the structure defining a 
	// member variable that maps to the supplied XML tag
	MemberDescriptor *pMap = GetEntry( m_EntryRoot , szTagToFind );
	
	return pMap;
};

MemberDescriptor* XMLObject::GetEntry( void* pAddrOfMemberToFind  )
{ 
	// build the member mapping tree, if it has not already been done
	LoadMemberMappings();

	// search the map entry tree for the structure defining a 
	// member variable that maps to the supplied memory address
	MemberDescriptor *pMap = GetEntry( m_EntryRoot , pAddrOfMemberToFind );
	
	return pMap;
};


void XMLObject::SetMember( void *pMemberToSet, int pzNewValue, 
										   MemberDescriptor* btRoot )
{
	char charValue[20];
	sprintf(charValue,"%d",(int)pzNewValue);
	SetMember(pMemberToSet,charValue,btRoot);
}

void XMLObject::SetMember( void *pMemberToSet, const char *pzNewValue, MemberDescriptor* btRoot )
{
	if(!btRoot)
	{
		if (!m_EntryRoot)
		{
			// if the mapping tree is not constructed, do so now
			LoadMemberMappings();
		}
		btRoot = m_EntryRoot;
	}
	if( btRoot->Left )
		SetMember( pMemberToSet, pzNewValue , btRoot->Left );

	if (btRoot->m_Member.pBool == pMemberToSet)
	{
		btRoot->Set(pzNewValue, -1, true, true);
	}

	if( btRoot->Right )
		SetMember( pMemberToSet, pzNewValue , btRoot->Right );
}



const char *XMLObject::GetMemberByTag( const char *pzTagName, GString &strDest )
{
	MemberDescriptor *pMap = GetEntry( pzTagName );
	if (pMap)
	{
		pMap->GetMemberValue(strDest);
	}
	else
	{
		char szTemp[128];
		sprintf(szTemp, "GetMemberByTag() failed. The tag <%s> "
			"has not been mapped to object <%s>", pzTagName, GetObjectType());
		TRACE_ERROR(szTemp);
	}
	return strDest;
}

int XMLObject::GetMemberByTagInt( const char *pzTagName )
{
	int nRetValue = -1;
	MemberDescriptor *pMap = GetEntry( pzTagName );
	if (pMap)
	{
		GString strDest;
		pMap->GetMemberValue(strDest);
	}
	else
	{
		char szTemp[128];
		sprintf(szTemp, "GetMemberByTagInt() failed. The tag <%s> "
			"has not been mapped to object <%s>", pzTagName, GetObjectType());
		TRACE_ERROR(szTemp);
	}
	return nRetValue;
}

void XMLObject::SetMemberByTag( const char *pzTagName, GString &strNewValue, bool bSetDirty/* = true*/)
{
	MemberDescriptor *pMap = GetEntry( pzTagName );
	if (pMap)
	{
		pMap->Set(strNewValue.Buf(), strNewValue.Length(), bSetDirty, true);
	}
	else
	{
		char szTemp[128];
		sprintf(szTemp, "SetMemberByTag() failed. The tag <%s> "
			"has not been mapped to object <%s>", pzTagName, GetObjectType());
		TRACE_ERROR(szTemp);
	}
}

void XMLObject::SetMemberByTag( const char *pzTagName,	const char *pzNewValue, bool bSetDirty/* = true*/)
{
	MemberDescriptor *pMap = GetEntry( pzTagName );
	if (pMap)
	{
		pMap->Set(pzNewValue, strlen(pzNewValue), bSetDirty, true);
	}
	else
	{
		char szTemp[128];
		sprintf(szTemp, "SetMemberByTag() failed. The tag <%s> "
			"has not been mapped to object <%s>", pzTagName, GetObjectType());
		TRACE_ERROR(szTemp);
	}
}

const char *XMLObject::GetMemberTag(void *pAddressOfMemberToCheck)
{
	MemberDescriptor *pMap = GetEntry( pAddressOfMemberToCheck );
	if (pMap)
	{
		return pMap->strTagName;
	}
	return 0;
}

void XMLObject::setObjectDirty(int bAllMembers/* = 0*/)
{
	if (bAllMembers)
	{
		LoadMemberMappings();
		GListIterator Iter(&m_lstMembers);
		while (Iter())
		{
			MemberDescriptor *pMap = (MemberDescriptor *)Iter++;
			pMap->SetDirty();
		}
	}
	
	m_nMemberStateSummary |= DATA_DIRTY;
	
	// in some cases we may need to iterate parent objects of all instances here
	XMLObject *pO = GetParentObject();
	if (pO)
		pO->setObjectDirty();
}


bool XMLObject::setMemberDirty(void *pAddressOfMemberToCheck, int bDirty/*=1*/)
{
	MemberDescriptor *pMap = GetEntry( pAddressOfMemberToCheck );
	if (pMap)
	{
		pMap->SetDirty(bDirty);
		m_nMemberStateSummary = 0; // added 3/9/2008
	}
	else
	{
		char szTemp[128];
		sprintf(szTemp, "Dirty state lookup failed. "
			"The member has not been mapped to object <%s> ", GetObjectType());
		TRACE_ERROR(szTemp);
	}
	return 0;
}

bool XMLObject::setMemberDirty(char *pzTagNameOfMemberToCheck, int bDirty/*=1*/)
{
	MemberDescriptor *pMap = GetEntry( pzTagNameOfMemberToCheck );
	if (pMap)
	{
		pMap->SetDirty( bDirty );
		m_nMemberStateSummary = 0; // added 3/9/2008
	}
	else
	{
		char szTemp[128];
		sprintf(szTemp, "Dirty state lookup failed. The tag <%s> "
			"has not been mapped to object <%s> ", 
			pzTagNameOfMemberToCheck, GetObjectType());
		TRACE_ERROR(szTemp);
	}
	return 0;
}

// returns true if the memory state does not sync with the original value set by the Object Factory
bool XMLObject::isMemberDirty(void *pAddressOfMemberToCheck)
{
	MemberDescriptor *pMap = GetEntry( pAddressOfMemberToCheck );
	if (pMap)
	{
		return pMap->IsDirty();
	}
	else
	{
		char szTemp[128];
		sprintf(szTemp, "Dirty state lookup failed. The member "
			"has not been mapped to object <%s> ", GetObjectType());
		TRACE_ERROR(szTemp);
	}
	return 0;
}

// returns true if the memory state does not sync with the original value set by the Object Factory
bool XMLObject::isMemberDirty(char *pzTagNameOfMemberToCheck)
{
	MemberDescriptor *pMap = GetEntry( pzTagNameOfMemberToCheck );
	if (pMap)
	{
		return pMap->IsDirty();
	}
	else
	{
		char szTemp[128];
		sprintf(szTemp, "Dirty state lookup failed. The tag <%s> "
			"has not been mapped to object <%s> ", 
			pzTagNameOfMemberToCheck, GetObjectType());
		TRACE_ERROR(szTemp);
	}
	return 0;
}



// returns true if the member has never been assigned a value, it is uninitialized
bool XMLObject::isMemberNull(void *pAddressOfMemberToCheck)
{
	MemberDescriptor *pMap = GetEntry( pAddressOfMemberToCheck );
	if (pMap)
	{
		return pMap->IsNull();
	}
	else
	{
		char szTemp[128];
		sprintf(szTemp, "Null state lookup failed. The member "
			"has not been mapped to object <%s> ", GetObjectType());
		TRACE_ERROR(szTemp);
	}
	return 0;
}

// returns true if the member has never been assigned a value, it is uninitialized
bool XMLObject::isMemberNull(char *pzTagNameOfMemberToCheck)
{
	MemberDescriptor *pMap = GetEntry( pzTagNameOfMemberToCheck );
	if (pMap)
	{
		return pMap->IsNull();
	}
	else
	{
		char szTemp[128];
		sprintf(szTemp, "Null state lookup failed. The tag <%s> "
			"has not been mapped to object <%s> ", 
			pzTagNameOfMemberToCheck, GetObjectType());
		TRACE_ERROR(szTemp);
	}
	return 0;
}

// returns true if the member has been assigned a value from the Object Factory
bool XMLObject::isMemberCached(void *pAddressOfMemberToCheck)
{
	MemberDescriptor *pMap = GetEntry( pAddressOfMemberToCheck );
	if (pMap)
	{
		return pMap->IsCached();
	}
	else
	{
		char szTemp[128];
		sprintf(szTemp, "Cached state lookup failed. The member "
			"has not been mapped to object <%s> ", GetObjectType());
		TRACE_ERROR(szTemp);
	}
	return 0;
}

// returns true if the member has been assigned a value from the Object Factory
bool XMLObject::isMemberCached(char *pzTagNameOfMemberToCheck)
{
	MemberDescriptor *pMap = GetEntry( pzTagNameOfMemberToCheck );
	if (pMap)
	{
		return pMap->IsCached();
	}
	else
	{
		char szTemp[128];
		sprintf(szTemp, "Cached state lookup failed. The tag <%s> "
			"has not been mapped to object <%s> ", 
			pzTagNameOfMemberToCheck, GetObjectType());
		TRACE_ERROR(szTemp);
	}
	return 0;
}




MemberDescriptor* XMLObject::GetEntry( MemberDescriptor* btRoot, const char * szToFind )
{	
	if(btRoot)
	{
		int nTest = strcasecmp((const char *)btRoot->strTagName,szToFind);
		if( nTest > 0 )
			return GetEntry( btRoot->Left, szToFind);
		else if( nTest < 0 )
			return GetEntry( btRoot->Right, szToFind);
		else
		{
			return btRoot;
		}
	}
	return 0; // Tag not found
}


MemberDescriptor* XMLObject::GetEntry( MemberDescriptor* btRoot, void * pToFind )
{	
	if(!btRoot)
	{
		if (!m_EntryRoot)
		{
			// if the mapping tree is not constructed, do so now
			LoadMemberMappings();
		}
		btRoot = m_EntryRoot;
	}
	if( btRoot->Left )
	{
		MemberDescriptor *p = GetEntry( btRoot->Left, pToFind );
		if (p)
			return p;
	}


	if (btRoot->m_Member.pBool == pToFind)
	{
		return btRoot;
	}

	if( btRoot->Right )
	{
		MemberDescriptor *p = GetEntry( btRoot->Right, pToFind );
		if (p)
			return p;
	}
	
	return 0; // not found
}



void XMLObject::RegisterMember(MemberDescriptor *pNewMember)
{
	// added 11/05/2009:  Replace &"'></ with an _
	pNewMember->strTagName.ReplaceChars("&<>/\"' ",'_');

	// Member-to-Tag mapping tree root for fast indexing by xml tag
	AddEntry( &m_EntryRoot, pNewMember );

	// List of nodes in tree (alternate index to the same data) for 
	// sequential access and preservation of order that MapMember() was called
	m_lstMembers.AddLast(pNewMember); // contains MemberDescriptor's
}

bool XMLObject::AddEntry( MemberDescriptor** Root, MemberDescriptor *ToAdd )
{
	if( *Root == 0 )    
	{
		*Root = ToAdd;
		return true;
	}
	int nTreePlacementTest = strcasecmp((const char *)(*Root)->strTagName,(const char *)ToAdd->strTagName);

	if( nTreePlacementTest > 0 )
		return AddEntry( &((*Root)->Left), ToAdd );
	else 
		return AddEntry( &((*Root)->Right), ToAdd );
	return false;
}





void XMLObject::GenerateMappedXML( MemberDescriptor* btRoot, 
							GString& xml, 
							int nTabs, 
							StackFrameCheck *pStack, 
							int nSerializeFlags)
{
	GListIterator Iter(&m_lstMembers);
	while (Iter())
	{
		MemberDescriptor *pMemberMap = (MemberDescriptor *)Iter++;

		if  ( (  nSerializeFlags & DIRTY_AND_CACHED_MEMBERS_ONLY ) != 0)
		{
			if (pMemberMap->IsDirty() || pMemberMap->IsCached())
			{
				pMemberMap->MemberToXML(xml,nTabs,pStack,nSerializeFlags);
			}
		}
		else
		{
			pMemberMap->MemberToXML(xml,nTabs,pStack,nSerializeFlags);
		}
	}
}

void XMLObject::GenerateSubsetXML( MemberDescriptor* btRoot, 
							GString& xml, 
							int nTabs, 
							StackFrameCheck *pStack, 
							int nSerializeFlags,
							const char *pzSubsetOfObjectByTagName)
{
	if (pzSubsetOfObjectByTagName)
	{
		GStringList lst("|",pzSubsetOfObjectByTagName);
		GStringIterator Iter(&lst);
		while (Iter())
		{
			MemberDescriptor *pMemberMap = GetEntry(Iter++);

			if  ( (  nSerializeFlags & DIRTY_AND_CACHED_MEMBERS_ONLY ) != 0)
			{
				if (pMemberMap->IsDirty() || pMemberMap->IsCached())
				{
					pMemberMap->MemberToXML(xml,nTabs,pStack,nSerializeFlags);
				}
			}
			else
			{
				pMemberMap->MemberToXML(xml,nTabs,pStack,nSerializeFlags);
			}
		}
	}
}

void XMLObject::GenerateOrderedXML( MemberDescriptor* btRoot, 
							GString& xml, 
							int nTabs, 
							StackFrameCheck *pStack, 
							int nSerializeFlags)
{
	if(btRoot)
	{
		if( btRoot->Left )
			GenerateOrderedXML( btRoot->Left, xml, nTabs, pStack, nSerializeFlags );

		btRoot->MemberToXML(xml,nTabs,pStack,nSerializeFlags);

		if( btRoot->Right )
			GenerateOrderedXML( btRoot->Right, xml, nTabs, pStack, nSerializeFlags );
	}
}

void XMLObject::ExecuteDeferredDestructions()
{
	GListIterator Iter(&m_lstMembers);
	while (Iter())
	{
		MemberDescriptor *pMD = (MemberDescriptor *)Iter++;
		pMD->DeferredDestruction();
	}
}

XMLObject::~XMLObject()
{
	// if this active instance is cached, make sure we don't 
	// keep any references to it after it's deleted.
	if ( m_oid && m_strObjectType && !(m_nBehaviorFlags & IS_TEMP_OBJECT) )
	{
		cacheManager.releaseObject(GetObjectType(),getOID());
	}

	// Normally Objects Auto-DecRef() upon destruction, but you CAN turn that off for some strangely special object.
	if ( !(m_nBehaviorFlags & PREVENT_AUTO_DESTRUCTION) )
	{
		StackFrameCheck Stack(this, 0);
		if(!m_EntryRoot)
			LoadMemberMappings();
		
		// If your app crashes under this line of code, check that this base class is pointing to valid data structures 
 		// in the derived class.  Almost certainly - the only cause for a crash here is that the automatic reference
		// counting is traversing an object that YOU DELETED.  If you delete them, you need to get the invalid
		// references out of any lists, arrays, trees YOU mapped them to or put them in - or just DON'T DELETE IT 
		// and it will NOT introduce a memory leak.  XMLObject reference counting works.
		ReferenceChangeChildren(&Stack, -1);
	}

	// a final destroy() on objects whose ~dtors have already been called because they reside in the class derived from 'this'
	ExecuteDeferredDestructions();


	// if the derived object ever called MapAttribute() on 'this'
	if (m_pMappedAttributeHash)
	{
		GHashIterator it(m_pMappedAttributeHash);
		while(it())
		{
			if ( (m_nBehaviorFlags & NO_MEMBER_MAP_PRECOUNT) )
			{
				delete (MemberDescriptor *)it++;
			}
			else
			{
				// explicit destructor call
				((MemberDescriptor *)it++)->MemberDescriptor::~MemberDescriptor();
			}
		}
		delete m_pMappedAttributeHash;
	}



	if (m_pRelationShipWrappers)
	{
		GListIterator Iter(m_pRelationShipWrappers);
		while (Iter())
		{
			XMLRelationshipWrapper *pRW = (XMLRelationshipWrapper *)Iter++;

			delete pRW;
		}
//		if (m_pRelationShipWrappers) 
		delete m_pRelationShipWrappers;
	}

	// deletes any MemberDescriptor's allocated when MapMember() was called 
	UnMapMembers();

	// In order to make XMLObject as light weight as possible and eliminate any
	// unnecessary construction overhead, member variables (lists, strings etc)
	// are only allocated when used, and likewise only destroyed when used.
	if (m_oid)
		delete m_oid; m_oid = 0;
	if (m_TimeStamp)
		delete m_TimeStamp; m_TimeStamp = 0;
	if (m_pToXMLStorage)
		delete m_pToXMLStorage; m_pToXMLStorage = 0;

	if (m_pMemberDescriptorArray)
		free(m_pMemberDescriptorArray); m_pMemberDescriptorArray = 0;
	

	if (m_pAttributes)
		delete m_pAttributes; m_pAttributes = 0;
	if (m_strXMLTag)
		delete m_strXMLTag; m_strXMLTag = 0;
//	if (m_strObjectType)
//		delete m_strObjectType; m_strObjectType = 0;
	if (m_pDefaultDataHandler)
		delete m_pDefaultDataHandler;

}


XMLObjectDataHandler *XMLObject::GetObjectDataHandler() 
{
	if (m_pDataHandler) 
		return m_pDataHandler;

	if (!m_pDefaultDataHandler)
	{
		m_pDefaultDataHandler = new DefaultDataHandler;
		m_pDataHandler = m_pDefaultDataHandler;
	}

	return m_pDataHandler;
}

GString *XMLObject::GetCDataStorage()
{
	XMLObjectDataHandler *p = GetObjectDataHandler();
	GString *pG =  p->GetCData();
	pG->SetLength(pG->Length());
	return pG;
}

GString *XMLObject::GetObjectDataStorage()
{
	XMLObjectDataHandler *p = GetObjectDataHandler();
	GString *pG =  p->GetOData();
	pG->SetLength(pG->Length());
	return pG;
}

void XMLObject::GetObjectDataValue(GString &xml)
{
	XMLObjectDataHandler *p = GetObjectDataHandler();
	p->GetObjectValue(xml);
}

void XMLObject::SetObjectDataValue(const char *pValue)
{
	XMLObjectDataHandler *p = GetObjectDataHandler();
	p->AssignObjectValue(pValue);
}


long XMLObject::IncRef(StackFrameCheck *pStack /*=0*/, int nDeep/* = 1*/)
{
	StackFrameCheck Stack(this, pStack);

	if (nDeep)
	{
		if(!m_EntryRoot)
		{
			LoadMemberMappings();
		}
		ReferenceChangeChildren(&Stack, 1);
	}
	m_nRefCount++;
	return m_nRefCount;
}

long XMLObject::DecRef(int nUserOwnsObject/* = false*/, StackFrameCheck *pStack /*=0*/)
{
	if (m_nRefCount < 1)
	{
		return m_nRefCount;
	}

	StackFrameCheck Stack(this, pStack);
	
	if(!m_EntryRoot)
	{
		// so we know what our children are
		LoadMemberMappings();
	}
	
	
	// call DecRef() on each child that is an object
	ReferenceChangeChildren(&Stack, -1); // recursion

	// NEXT - decrement our own reference count
	//
	long nRefs = --m_nRefCount;

	if (nRefs == 0)
	{
		// If this object is in the cache, remove it. 
		if ( m_oid && !(m_nBehaviorFlags & IS_TEMP_OBJECT) && !nUserOwnsObject)
		{
			cacheManager.releaseObject(GetObjectType(), (const char *)*m_oid );
		}
	}
	
	if (0 == nRefs && !nUserOwnsObject)
	{
		m_nBehaviorFlags |= PREVENT_AUTO_DESTRUCTION;
		delete this;
		return 0;
	}
		
	return nRefs;
}


XMLObject::XMLObject() :
	m_ValidObjectBeg(777),
	m_pMappedAttributeHash(0),
	m_MRUObjectDescriptor(0),
	m_nRefCount(1),
	m_pAttributes(0),
	m_pDataHandler(0),
	m_nMemberStateSummary(0),
	m_EntryRoot(0),
	m_pRelationShipWrappers(0),
	m_oid(0),
	m_TimeStamp(0),
	m_strXMLTag(0),
	m_strObjectType(0),
	m_pToXMLStorage(0),
	m_bCountingMemberMaps(0),
	m_nMappedCount(0),
	m_pMemberDescriptorArray(0),
	m_nBehaviorFlags(0),
	m_pDefaultDataHandler(0)
{
}


void XMLObject::Init(const char* oid, const char* pzUpdateTime)
{
	if (m_oid && oid)
		delete m_oid;

	if (m_TimeStamp && pzUpdateTime)
		delete m_TimeStamp;

	if (oid)
		m_oid = new GString;

	if (pzUpdateTime)
		m_TimeStamp = new GString;
	
	if (oid)
		(*m_oid) = oid;

	if (pzUpdateTime)
		(*m_TimeStamp) = pzUpdateTime;
}


void XMLObject::MapEntity(const char *pzEntityName)
{
	if (pzEntityName)
	{
		MemberDescriptor *pNewMD = 0;
		if ( !(m_nBehaviorFlags & NO_MEMBER_MAP_PRECOUNT) )
		{
			pNewMD = new( ((char *)m_pMemberDescriptorArray) + (sizeof(MemberDescriptor) * m_nMappedCount++)  ) MemberDescriptor(this, pzEntityName);
		}
		else
		{
			pNewMD = ::new MemberDescriptor(this, pzEntityName);
		}
		RegisterMember(pNewMD);
	}
	else
	{
		TRACE_ERROR("Cannot map to Null entity name.");
	}
}



void XMLObject::ApplyToChildren(MemberDescriptor *pMD, StackFrameCheck* pStack, int nChange)
{
//  Uncomment the following two lines to peek inside the XMLFoundation reference counting design @ runtime.
//	GString g2; GString g1;  const char *n = pMD->GetTypeName(g1);  
//	printf("\nApplyToChildren[%d.%d.%d] for %s<%s> w/ %d objs\n",nChange,(int)pStack->getStackSize(),pStack->isNestedStackFrame(),n,pMD->strTagName.Buf(),pMD->GetObjectContainmentCount());


	// if this member represents an object or collection of objects release them.
	if ( !pStack->isNestedStackFrame() )
	{
		// iterate through all objects that this MemberDescriptor represents
		xmlObjectIterator iter = 0;
		XMLObject *pObj = pMD->GetFirstContainedObject(&iter);
		while (pObj)
		{
			// if the count is decrementing
			if (nChange < 0)
			{
				// Prevent auto deletion of direct 'containment' sub objects.
				// Those objects we NOT allocated with malloc/new.
				// The objects that contain them WERE allocated with malloc/new.
				// For example, if you malloc/new space for a struct and within that struct is another struct (a sub struct)
				//   the sub struct is not responsible for freeing/deleting the memory space that it exists in because it exists
				//   in a region of memory that was acquired for the malloc/new of the struct which contain this sub struct.
				int nUserOwnsObj = (pMD->DataType == MemberDescriptor::XMLObj) ? 1 : 0; 
				
				GString strTag(pObj->getOID());
				long lRefCount = pObj->DecRef(nUserOwnsObj, pStack);
				if (lRefCount == 0)
				{
					pMD->RemoveLastContainedObject(iter,strTag,pObj);
				}

/* // removed 12/26/2009 

				if ( (pMD->DataType == MemberDescriptor::XMLObjList) && (lRefCount == 0) )
				{
 					if ( m_nBehaviorFlags & MULTI_TYPE_LIST_SUPPORT )
					{
						// We released an object in a list. Now remove it from the list.
						// Ensures we do not Release() the same object twice when more than
						// one object type is mapped into the SAME list.
						pMD->RemoveLastContainedObject(iter,strTag,pObj);
					}

				}
*/
			}
			// otherwise it's incrementing
			else 
			{
				// Add a reference to this object
				pObj->IncRef(pStack);
			}
			pObj = pMD->GetNextContainedObject(iter);
		}
	}
	else
	{
		printf("\nNested frame exits\n");
	}
}

void XMLObject::ReferenceChangeChildren(StackFrameCheck* pStack, int nChange)
{
	GListIterator Iter(&m_lstMembers);
	while (Iter())
	{
		MemberDescriptor *pMD = (MemberDescriptor *)Iter++;
		if (pMD->m_bFirstMap && !pMD->IsSimpleType() )
		{
			ApplyToChildren(pMD, pStack, nChange);
		}
	}
}

void XMLObject::SetAttributeList(XMLAttributeList *p)
{
	if (m_pAttributes)
	{
		delete m_pAttributes;
	}
	m_pAttributes = p;
}



// IsFirstMap() is designed to support objects that map into data 
// structures.  For example, 'Customers' that map to a list, or 'Orders' 
// that map to a hash table.  In some circumstances, multiple object 
// types may be mapped to the same data structure (Customers and Orders 
// in the same list)  If the following two entries are encountered:
//
//	MapMember(&m_lstMixedObjects,  CFoo::GetStaticTag(), 0 );
//	MapMember(&m_lstMixedObjects,  CBar::GetStaticTag(), 0 );
//
// The list has been mapped twice, once for each type of object that it contains.
// Each MapMember() implementation in XMLObject will call IsFirstMap() the first call
// returns true, and the second returns false.
int XMLObject::IsFirstMap( void *pList )
{
	GListIterator Iter(&m_lstDataStructures);
	while ( Iter() )
	{
		void *p = Iter++;
		if (p == pList)
		{
			return 0;
		}
	}
	m_lstDataStructures.AddLast(pList);
	return 1;
}

void XMLObject::MapObjectID( const char *pTag1, int nSource1, 
							 const char *pTag2, int nSource2,
							 const char *pTag3, int nSource3,
							 const char *pTag4, int nSource4,
							 const char *pTag5, int nSource5)
{
	if(m_bCountingMemberMaps)
	{
		return;
	}
	
	GList   *lstOIDKeyParts = GetOIDKeyPartList(1);
	if (lstOIDKeyParts->Size() == 0)
	{
		lstOIDKeyParts->AddLast((void *)pTag1);
		lstOIDKeyParts->AddLast((void *)nSource1);
		if (pTag2)
		{
			lstOIDKeyParts->AddLast((void *)pTag2);
			lstOIDKeyParts->AddLast((void *)nSource2);
		}
		if (pTag3)
		{
			lstOIDKeyParts->AddLast((void *)pTag3);
			lstOIDKeyParts->AddLast((void *)nSource3);
		}
		if (pTag4)
		{
			lstOIDKeyParts->AddLast((void *)pTag4);
			lstOIDKeyParts->AddLast((void *)nSource4);
		}
		if (pTag5)
		{
			lstOIDKeyParts->AddLast((void *)pTag5);
			lstOIDKeyParts->AddLast((void *)nSource5);
		}
	}
	else
	{
		// 2nd Key definition ignored, the existing definition will be used
	}
}



int XMLObject::CopyState(XMLObject *pCopyFrom)
{
	// move the updated member values into the cached object
	GListIterator Iter( &pCopyFrom->m_lstMembers );
	while (Iter())
	{
		MemberDescriptor *pMemberMap = (MemberDescriptor *)Iter++;
		if ( !pMemberMap->IsNull() )
		{
			if (pMemberMap->IsSimpleType() )
			{
				GString strValue;
				pMemberMap->GetMemberValue(strValue);
				SetMemberByTag(pMemberMap->strTagName,strValue,false);
			}
			else 
			{
				// Get the destination member descriptor for this list, hash, array or whatever data structure it is
				MemberDescriptor *pMoveToMap = GetEntry( pMemberMap->strTagName );

				// pMoveToMap is the "RelationshipWrapper" in the destination object
				// pMemberMap is the "RelationshipWrapper" in the source object - (this is what we are looping through)


				// move object references from the data structure in the temp object into the cached object
				xmlObjectIterator iter = 0;
				XMLObject *pObjSource1 = pMemberMap->GetFirstContainedObject(&iter);
				while (pObjSource1)
				{
					GString strVType ( pObjSource1->GetVirtualType() );
					if (strVType == "RelationshipWrapper")
					{
						// get the relationship wrapper object instances
						// note RelationshipWrapper's can only have 1 MemberDescriptor
						xmlObjectIterator iter1 = 0;
						xmlObjectIterator iter2 = 0;
						XMLObject *pObjSource = pMemberMap->GetFirstContainedObject(&iter1);
						XMLObject *pObjDest = pMoveToMap->GetFirstContainedObject(&iter2);
						pMemberMap->ReleaseObjectIterator(&iter1);
						pMoveToMap->ReleaseObjectIterator(&iter2);


						// get the source MemberDescriptor to the Data structure in the RelationshipWrapper
						MemberDescriptor *pSrcMapDataStruct = 0;
						GListIterator IterSource(&pObjSource->m_lstMembers);
						while (IterSource())
						{
							pSrcMapDataStruct = (MemberDescriptor *)IterSource++;
						}

						// get the destination MemberDescriptor to the Data structure in the RelationshipWrapper
						MemberDescriptor *pDestMapDataStruct = 0;
						GListIterator IterDest(&pObjDest->m_lstMembers);
						while (IterDest())
						{
							pDestMapDataStruct = (MemberDescriptor *)IterDest++;
						}

						// finally, add Objects from pSrcMapDataStruct into pDestMapDataStruct (if they are not already there)
						xmlObjectIterator iterFinal = 0;
						XMLObject *pOFinal = pSrcMapDataStruct->GetFirstContainedObject(&iterFinal);
						while (pOFinal)
						{
							if ( !pDestMapDataStruct->ObjectExists( pOFinal ) )
							{
								pDestMapDataStruct->AddContainedObject( pOFinal );
								pOFinal->IncRef();
							}
							
							pOFinal = pSrcMapDataStruct->GetNextContainedObject(iterFinal);
						}

					}

					pObjSource1 = pMemberMap->GetNextContainedObject(iter);
				}
			}
		}
	}

	// move the updated mapped attribute values into the cached object
	if (pCopyFrom->m_pMappedAttributeHash)
	{
		GHashIterator it(pCopyFrom->m_pMappedAttributeHash);
		while(it())
		{
			MemberDescriptor *pMD = (MemberDescriptor *)it++;
			GString strValue;
			pMD->GetMemberValue(strValue);
			SetMappedAttribute(pMD->strTagName, (const char *)strValue, (int)strValue.Length());
		}
	}

	// move the updated non-mapped attribute values into the cached object
	if (pCopyFrom->m_pAttributes)
	{
		if (!m_pAttributes)
			m_pAttributes = new XMLAttributeList;
		m_pAttributes->UpdateAttributes(pCopyFrom->m_pAttributes);
	}
	
	// if the source object was factory created, keep creator's map. 
	return SetObjectDescriptor(pCopyFrom->GetObjectDescriptor());

}


void XMLObject::RemoveAllAttributes()
{
	if (m_pAttributes)
	{
		m_pAttributes->RemoveAll();
	}
}




MapOfActiveObjects g_ActiveObjectMap;
void MapOfActiveObjects::AddActiveObjectMap(MemberDescriptor *pNew)
{
	GString strKey;
#if defined(_LINUX64) || defined(_WIN64) || defined(_IOS)
	strKey << (unsigned __int64)(void *)pNew; // The memory address is a 64 bit integer - convert to string
#else
	strKey << (unsigned int)(void *)pNew; // The memory address is a 32 bit integer - convert to string
#endif
	if (!m_hshActiveMaps.Lookup(strKey))
		m_hshActiveMaps.Insert(strKey,pNew);
}
int MapOfActiveObjects::IsActiveObjectMap(MemberDescriptor *pMap)
{
	GString strKey;
#if defined(_LINUX64) || defined(_WIN64)  || defined(_IOS)
	strKey << (unsigned __int64)(void *)pMap; // The memory address is a 64 bit integer - convert to string
#else
	strKey << (unsigned int)(void *)pMap; // The memory address is a 32 bit integer - convert to string
#endif

	if (m_hshActiveMaps.Lookup(strKey))
		return 1;
	return 0;
}
void MapOfActiveObjects::RemoveActiveObjectMap(MemberDescriptor *pOld)
{
	GString strKey;
#if defined(_LINUX64) || defined(_WIN64) || defined(_IOS)
	strKey << (unsigned __int64)(void *)pOld; // The memory address is a 64 bit integer - convert to string
#else
	strKey << (unsigned int)(void *)pOld; // The memory address is a 32 bit integer - convert to string
#endif
	m_hshActiveMaps.RemoveKey(strKey);
}




GList *XMLObject::GetParentObjects(GList *lstResults)
{
	GListIterator Iter(&m_lstCreationDescriptors);
	while ( Iter() )																
	{																				
		MemberDescriptor *p = (MemberDescriptor *)Iter++;
		if (g_ActiveObjectMap.IsActiveObjectMap(p))
		{
			if (lstResults)
				lstResults->AddLast(p->m_ParentObject);
		}
		else
			m_lstCreationDescriptors.Remove(p);
	}
	return lstResults;
}

XMLObject *XMLObject::GetParentObject()
{
	GListIterator Iter(&m_lstCreationDescriptors);
	while ( Iter() )																
	{
		MemberDescriptor *p = (MemberDescriptor *)Iter++;
		if (g_ActiveObjectMap.IsActiveObjectMap(p))
			return p->m_ParentObject;
		else
			m_lstCreationDescriptors.Remove(p);
	}
	return 0;
}


void XMLObject::EarlyNotify()
{
	LoadMemberMappings();
	GListIterator Iter(&m_lstMembers);
	while (Iter())
	{
		MemberDescriptor *pMap = (MemberDescriptor *)Iter++;
		if (pMap->DataType == MemberDescriptor::XMLObjList)
		{
			pMap->m_DataAbstractor.pListHandler->create(pMap->m_Member.pXMLObjList);
		}
		else if (pMap->DataType == MemberDescriptor::KeyedDataStructure)
		{
			pMap->m_DataAbstractor.pKeyDSA->create(pMap->m_Member.pKeyedDSA);		
		}
	}
}

MemberDescriptor *XMLObject::GetObjectDescriptor()
{ 
	return m_MRUObjectDescriptor;
}

int XMLObject::IsObjectDescriptor(MemberDescriptor *p)
{ 
	if (m_lstCreationDescriptors.Size() > 0)
	{
		MemberDescriptor *pF =  (MemberDescriptor *)m_lstCreationDescriptors.First(); 
		if (pF == p)
			return 1; 
		else
		{
			GListIterator Iter(&m_lstCreationDescriptors);
			while (Iter())
			{
				MemberDescriptor *pMD = (MemberDescriptor *)Iter++;
				if (pMD == p)
					return 1;
			}
			return 0;
		}
	}
	else
		return 0;
}

int XMLObject::GetObjectDescriptorCount()
{
	return m_lstCreationDescriptors.Size();
} 

int XMLObject::SetObjectDescriptor(MemberDescriptor *p) 
{ 
	m_MRUObjectDescriptor = p;
	GListIterator Iter(&m_lstCreationDescriptors);
	while (Iter())
	{
		MemberDescriptor *pMD = (MemberDescriptor *)Iter++;
		if (p == pMD)
			return 0; // already exists.
	}
	m_lstCreationDescriptors.AddHead(p);
	
	return 1; // first of it's kind
}

void XMLObject::ToXML(GString& xmlData, int nSerializeFlags /*= FULL_SERIALIZE*/)
{
	xmlData.Empty();
	ToXML(&xmlData, 0, NULL, nSerializeFlags);
}

bool XMLObject::ToXMLFile(GString& xmlData, const char *pzFileName, int nSerializeFlags /*= FULL_SERIALIZE*/)
{
	xmlData.Empty();
	ToXML(&xmlData, 0, NULL, nSerializeFlags);
	return xmlData.ToFile(pzFileName, 0);
}

bool XMLObject::FromXMLFileX(GString& xmlData, const char *pzFileName, GString *pErrorDescriptionDestination)
{
	try
	{
		// FromFile() can throw() GExceptions
		if (xmlData.FromFile(pzFileName))
		{
			// FromXML() can throw() GExceptions
			FromXML(xmlData.Buf(), 0, 0);
			return true;
		}
	}
	catch(GException &e) // FromXMLFileX() does not throw
	{
		if (pErrorDescriptionDestination)
			*pErrorDescriptionDestination  << e.GetDescription() << " (" << e.GetError() << ")";;

	}
	catch(...)  // FromXMLFileX() does not throw (anything)
	{
		ASSERT(FALSE);
	}
	return false;
}