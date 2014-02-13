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

#ifndef _MEMBER_MAP_ENTRY_H
#define _MEMBER_MAP_ENTRY_H

#ifndef _WIN32
	#ifndef __int64
		#define	__int64 long long
	#endif
#endif


#include "AttributeList.h"
#include "xmlDefines.h"
#include "ListAbstraction.h"
#include "MemberHandler.h"

class StackFrameCheck;
class StringAbstraction;
class XMLAttributeList;
class TranslationLists;

class MemberDescriptor
{
	// a bit flag field containing NULL, DIRTY, CACHE states
	int m_memberStateFlags;

	// user defined data structure
	void *m_pUserData;

public: 
	// pointer to object that contains the member described by this structure
	XMLObject *m_ParentObject;
	
	// almost always true.  See XMLObject::IsFirstMap() in XMLObject.cpp
	int m_bFirstMap;

	void *GetItemData();
	void  SetItemData(void *);

	~MemberDescriptor();
	void Init( XMLObject *pParent );
	
	// return the type of the member - "string" "char","int" or what ever MapMember() was describing
	const char *GetTypeName(GString &strDestination);

	// GetMemberValue() returns the string representation of the member value for all simple datatypes.
	// For complex types - objects and collections like (list<void>, list<object>, object&, and UserCollections )
	//	  NULL is returned, since there is no simple value.
	const char *GetMemberValue(GString &strDestination);
	
	// if the mapped type is a data structure that contains a reference to pO return 1
	int ObjectExists( XMLObject *pO );

	 // See "Translation Maps Usage" in xmlObject.h
	//  translate to the value we will write in the XML.
	const char *TranslateMemberValueToXMLValue(const char *src);
	const char *TranslateMemberValueToXMLValue(__int64 nIntegers, GString *strDest);
	// translate to the value we will store in the member variable.
	const char *TranslateXMLValueToMemberValue(const char *src);

	// Construct a Root element used to store tag-to-objectFactory mappings for the Factory Manager
	MemberDescriptor(XMLObject *pParent, const char *pzTag, ObjectFactory pFactory, const char * pzClassType);

	// This constructs the MemberDescriptor to refer to an Entity.  It causes the 
	// Entity to be serialized out by name, so that the Object can restore by the
	// value that the tokenizer will put in it's place.
	MemberDescriptor(XMLObject *pParent, const char *pzEntityName);

	// bits
	MemberDescriptor(XMLObject *pParent, const char *pzTag, unsigned char *pVal, unsigned __int64 nBitMask, const char *pzCommaSeparated1Values, const char *pzCommaSeparated0Values);
	MemberDescriptor(XMLObject *pParent, const char *pzTag, unsigned short *pVal, unsigned __int64 nBitMask, const char *pzCommaSeparated1Values, const char *pzCommaSeparated0Values);
	MemberDescriptor(XMLObject *pParent, const char *pzTag, unsigned int *pVal, unsigned __int64 nBitMask, const char *pzCommaSeparated1Values, const char *pzCommaSeparated0Values);
	MemberDescriptor(XMLObject *pParent, const char *pzTag, unsigned __int64*pVal, unsigned __int64 nBitMask, const char *pzCommaSeparated1Values, const char *pzCommaSeparated0Values);

	#ifndef _NO_XMLF_BOOL 
	MemberDescriptor(XMLObject *pParent, const char *pzTag, bool *pVal, int nBoolReadability);
	#endif
	MemberDescriptor(XMLObject *pParent, const char *pzTag, char *pVal, char ForNameManglingOnly);
	MemberDescriptor(XMLObject *pParent, const char *pzTag, short *pVal, const char *pzTranslationMapIn = 0, const char *pzTranslationMapOut = 0,int nTranslationFlags = 0);
	MemberDescriptor(XMLObject *pParent, const char *pzTag, int *pVal, const char *pzTranslationMapIn = 0, const char *pzTranslationMapOut = 0,int nTranslationFlags = 0);
	MemberDescriptor(XMLObject *pParent, const char *pzTag, long *pVal, const char *pzTranslationMapIn = 0, const char *pzTranslationMapOut = 0,int nTranslationFlags = 0);
	MemberDescriptor(XMLObject *pParent, const char *pzTag, __int64 *pVal, const char *pzTranslationMapIn = 0, const char *pzTranslationMapOut = 0,int nTranslationFlags = 0);
	MemberDescriptor(XMLObject *pParent, const char *pzTag, double *pVal, const char *pzTranslationMapIn = 0, const char *pzTranslationMapOut = 0,int nTranslationFlags = 0);	
	// Construct a string XMLEntry
	MemberDescriptor(XMLObject *pParent, const char *pzTag, char *pVal, int nMaxSize, const char *pzTranslationMapIn, const char *pzTranslationMapOut,int nTranslationFlags = 0);
	MemberDescriptor(XMLObject *pParent, const char *pzTag, void *pVal, StringAbstraction *pHandler,const char *pzTranslationMapIn=0,const char *pzTranslationMapOut=0,int nTranslationFlags = 0);
	// Construct a sub-Object pointer XMLEntry
	MemberDescriptor(XMLObject *pParent, const char *pzTag, XMLObject **pObj, ObjectFactory pFactory );
	// Construct a sub-Object XMLEntry
	MemberDescriptor(XMLObject *pParent, const char *pzTag, XMLObject *pObj);
	// Construct an Object Collection XMLEntry
	MemberDescriptor(XMLObject *pParent, const char *pzTag, void *pList, ListAbstraction *pHandler, ObjectFactory pFactory);
	// Construct a member that will be controlled by supplied handler object
	MemberDescriptor(XMLObject *pParent, const char *pzTag, MemberHandler *pHandler);
	// Construct a string collection mapping entry
	MemberDescriptor(XMLObject *pParent, const char *pzTag, void *pStringCollection, StringCollectionAbstraction *pHandler, const char *pzTranslationMapIn = 0, const char *pzTranslationMapOut = 0,int nTranslationFlags = 0);
	// Construct an integer array mapping entry
	MemberDescriptor(XMLObject *pParent, const char *pzTag, void *pIntegerArray, IntegerArrayAbstraction *pHandler, const char *pzTranslationMapIn = 0, const char *pzTranslationMapOut = 0,int nTranslationFlags = 0);
	// Construct a member structure that contains object/key pairs (binary tree, hash etc)
	MemberDescriptor(XMLObject *pParent, const char *pzTag, void *pKeyedDSA,KeyedDataStructureAbstraction *pHandler);

	// tree nodes alphebetically sorted by Tag
	MemberDescriptor *Left;    
	MemberDescriptor *Right;

	// This is the Key for searching the tree, supplied by the derived class.
	// This is the tag that the XML Parser uses to obtain storage for element data
	// when it encounters a new tag in the XML.
	GString strTagName;

	// Always NULL unless pointing to member sub-object of type Collection.
	ObjectFactory m_pfnFactoryCreate; 

	// Object's attributes are stored in the object, not in this descriptor
	// so if IsSubObject() is true, m_pAttributes will always be null. Most 
	// member variables have no attributes, so only when a member with 
	// attributes is encountered is m_pAttributes allocated. 
	XMLAttributeList *m_pAttributes;

	// Access to attributes of the member.
	void AddAttribute( const char * pzName, const char * pzValue );
	void AddAttribute( const char * pzName, int nValue );
	const char *FindAttribute( const char *pzAttName );
	
	void SetAsTempObject(XMLObject *pO);


	// This member may represent 0-n objects. These methods provide 
	// information about, and access to the Contained Objects.
	bool IsSubObject()
	{ 
		if ( DataType == ManagedByDerivedClass )
		{
			return m_DataAbstractor.pMemberHandler->IsMemberObject((const char *)strTagName);
		}
		return (DataType == XMLObjList || DataType == XMLObj || DataType == Root || DataType == XMLObjPtr || DataType == KeyedDataStructure);
	}
	bool IsSimpleType()
	{
		if (DataType == ManagedByDerivedClass || DataType == UserString || DataType == Bit
			|| DataType == Str || DataType == Bool || DataType == Int || DataType == Long
			|| DataType == Byte || DataType == Short || DataType == Int64 || DataType == Double)
			return true;
		return false;
	}

	
	// If this member contains 1 or more objects, the following set
	// of methods abstract a common interface no matter what the 
	// specific type of object container is.
	int AddContainedObject( XMLObject *pO );
	__int64 GetObjectContainmentCount();
	void RemoveLastContainedObject( xmlObjectIterator iter, GString &strtag, XMLObject *pAddressOfInvalidMemory );
	XMLObject *GetNextContainedObject( xmlObjectIterator iter );
	XMLObject *GetFirstContainedObject( xmlObjectIterator *iter );
	XMLObject *GetKeyedContainedObject( const char *key );

	// Only release an iterator if you have not walked the entire list.
	// Calling GetNextContainedObject() after the last item in a list 
	// will return null and delete the iterator.
	void ReleaseObjectIterator( xmlObjectIterator iter );

	// if this abstracted type supports deferred destruction (GList, GHash) - then cleanup
	void DeferredDestruction();


	// When the XML Parser sets the data, it is setting the initial value
	// so it is not considered dirty, however when an application developer
	// sets the state of a member variable, it is now dirty.
	void Set(const char * pzData, __int64 nDataLen, bool bSetDirty, bool bUpdateCleanMembers);
	// same, uses "member set flags" in xmlDefines.h
	void Set(const char * pzData, __int64 nDataLen, unsigned int nSetFlags);


	bool IsDirty() 
	{
		if ( DataType == ManagedByDerivedClass )
		{
			return m_DataAbstractor.pMemberHandler->IsMemberDirty((const char *)strTagName);
		}
		return ( ( m_memberStateFlags & DATA_DIRTY ) != 0);
	}
	bool IsNull()
	{
		if ( DataType == ManagedByDerivedClass )
		{
			m_DataAbstractor.pMemberHandler->IsNull( (const char *)strTagName );
		}
		return ( ( m_memberStateFlags & DATA_NULL ) != 0);
	}
	bool IsCached()
	{
		if ( DataType == ManagedByDerivedClass )
		{
			return m_DataAbstractor.pMemberHandler->IsCached( (const char *)strTagName );
		}
		return ( ( m_memberStateFlags & DATA_CACHED ) != 0);
	}
	bool IsSerializable(){ 	 return ( ( m_memberStateFlags & DATA_SERIALIZE ) != 0);    }
	void SetSerializable( bool bSerialize )
	{
		if ( bSerialize )
		{
			m_memberStateFlags |= DATA_SERIALIZE;
		}
		else
		{
			m_memberStateFlags &= ~DATA_SERIALIZE;
		}
	}
	void SetDirty( int bDirty = 1 );

	// Native, and user defined datatype support
	enum DataTypes 
	{ 
		XMLObj, 
		XMLObjPtr,
		UserString,
		UserStringCollection,
		UserIntegerArray,
		XMLObjList,
		Bit,
		Byte,
		Short,
		Int,
		Long,
		Int64, 
		Double,
		Str, 
		Root, 
		Bool, 
		ManagedByDerivedClass,
		KeyedDataStructure,
		EntityReference
	} DataType;


	// These are mutually exclusive and store the location of 
	// XMLObject derived member data.  
	// If the derived type is a collection, pObjectList contains 
	//     storage for objects created from the m_pfnFactoryCreate.
	// pObject points to a member sub-Object in the derived class.
	union DERIVED_DATA_STORAGE
	{
		const char *pClassName;
		char *pNativeStr;
		bool *pBool;
		XMLObject *pObject;
		XMLObject **ppObject;
		void *pUserString;
		void *pXMLObjList;
		void *pUserStringCollection;
		void *pUserIntegerArray;
		char *pByte;
		short *pShort;
		int *pInt;
		long *pLong;
		void *pKeyedDSA;
		__int64 *pInt64;
		double *pDouble;
		unsigned char *p8Bits;
		unsigned short *p16Bits;
		unsigned int *p32Bits;
		unsigned __int64 *pBits;
	}m_Member;  

	// abstration handlers are used make atomic operations on data in any data structure.  
	// sizeof(m_DataAbstractor) is 8.
	union DERIVED_DATA_MANAGEMENT_ABSTRACTION
	{
		ListAbstraction *pListHandler;
		StringAbstraction *pStringHandler;
		MemberHandler *pMemberHandler;
		StringCollectionAbstraction *pStrListHandler;
		IntegerArrayAbstraction *pIntArrayHandler;
		KeyedDataStructureAbstraction *pKeyDSA;
		// ------------------------------------------
		// The following variables use this already allocated and otherwise unused union space rather than creating new variable space.

		__int64 nBufferMaxSize;		// when mapping a char *, store the sizeof() it here
		int nBoolReadability;		// when mapping a bool, store (an enumeration in a union) here:
									// 0=Yes/No       1=True/False       2=On/Off		3=1/0
		unsigned __int64 nBitMask;	// when mapping a MapMemberBit()

	}m_DataAbstractor;


	// The previous union holds member data handling information.
	// This union holds additional data - currently translation maps - but this union has the same concept as m_DataAbstractor
	// it is simply an additional 8 bytes of storage allocated for each MemberDescriptor - each DataType may use this for any purpose
	union MORE_MEMBER_DATA
	{
		TranslationLists *pTranslationLists;
	}m_MoreData;

	
	// this overload for new() intentionally does nothing - as it assumes the memory has already been allocated.
	void* operator new(size_t /* ignored */, void* where) throw() { return where; }
	
	// so there is nothing to delete() - and adding an empty delete() is not wise as it could possibly be invoked unintentionally
	// so ... lets just ignore the compiler warning because this "memory managed" design is intentional.
	//	void operator delete(void*, size_t){};
#ifdef _WIN32
	#pragma warning (disable:4291)
#endif

	// XML serialization support for the member that this entry describes
	void MemberToXML(GString& xml, int nTabs, StackFrameCheck *pStack,int nSerializeFlags);
	static inline void TabifyXML(GString& xml, int nTabs, int nFlags = 0 )
	{
		if (xml.Length())
		{
			if ( (  nFlags & WINDOWS_LINEFEEDS ) == 0)
			{
				xml << "\r\n";
			}
			else if ( (  nFlags & UNIX_LINEFEEDS ) == 0)
			{
				xml << "\n";
			}
			else
			{
				#ifdef _WIN32
					xml << "\r\n";
				#else
					xml << "\n";
				#endif
			}
		}

		while(nTabs)
		{
			xml.write( g_TABS,(nTabs > MAX_TAB_WRITE) ? MAX_TAB_WRITE : nTabs  );
			if (nTabs > MAX_TAB_WRITE)
			{
				nTabs -= MAX_TAB_WRITE;
			}
			else
			{
				break;
			}
		}
	}
};

#endif //_MEMBER_MAP_ENTRY_H
