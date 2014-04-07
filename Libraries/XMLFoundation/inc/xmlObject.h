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

#ifndef _XML_BASE_OBJECT_SUPPORT__
#define _XML_BASE_OBJECT_SUPPORT__

#include "xmlDefines.h"
#include "CacheManager.h"
#include "GException.h" // because XMLObject throws these in FromXML
#include "MemberDescriptor.h" // because most compilers allow the declaration for [m_EntryRoot] but gcc does not unless it has the entire definition rather than a just a forward declaration which is all that ought to be necessary.
#include "GHash.h" // for m_hshActiveMaps

#include <stdio.h> // for IMPLEMENT_FACTORY macro: sprintf() / atoi()

class StackFrameCheck;
class XMLRelationshipWrapper;
class XMLObject;
class StringAbstraction;
class MemberHandler;
class XMLObjectDataHandler;
class XMLAttributeList;
class GString;
class GQSortArray;
class GArray;

class XMLObject
{
public:
	// Every valid instance, on valid memory will have a value of 777 in m_ValidObjectBeg
	// It is debugging information compiled into the release build, and it has proven helpful
	// more than once.  XMLObject has been designed to be a lightweight base class, this is
	// extra weight that may someday may be compiled out of a release build so never write
	// permanent code that uses this variable, it makes for a helpful breakpoint when the value
	// is NOT equal to 777 - because all other numbers indicate corruption - and we cant have that.
	int m_ValidObjectBeg;
private:
	// Private member string storage
	// Storage is allocated only when necessary to keep the XMLObject lightweight
	// Note: these variable names are misleading, m_strObjectType should be m_pstrObjectType
	// Long ago, these variables were not pointers.  They added construction overhead for the
	// majority of cases where these were never used.
	GString *m_strObjectType;		// derived class name - Runtime type information
	GString *m_strXMLTag;			// default tag for this instance
	GString *m_oid;
	//GList   *m_lstOIDKeyParts; // in 2014 this was moved to static storage.

	// MemberDescriptor is a structure that describes each member
	friend class MemberDescriptor;



	bool m_bCountingMemberMaps;
	int m_nMappedCount;
	void *m_pMemberDescriptorArray;

	// Note: GetMemberMapCount() is implemented in the DECLARE_FACTORY factory macro in this .h file.
	// This method returns a static variable through a virtual?  uh huh.  
	// Think about that.  Why would we ever want to call a virtual just to get the value of a 
	// static (aka global) variable?  The answer: Because it's in a dynamic variable name that 
	// cannot be known or accessed otherwise.  Notice in the implementation (right here in this .h file)
	// that the variable we will be accessing is a concatenation of the variable class name that the application 
	// programmer uses of the object derived from XMLObject + the word "MemberMapCount".  The ## does that.
	// Now since it's virtual, an object derived from XMLObject reserves the right to map a variable
	// number of members depending on some internal customized state.
	//
	// if bIncrement is 0 = return the static count of membermap() calls for this object type. 
	// if bIncrement is 1 = increment the value then return it.
	// if bIncrement is 2 = set the static count of membermaps for this object type to 0.
	virtual int GetMemberMapCount(char bIncrement){return 0;}; 


	GString *m_TimeStamp;
	GString *m_pToXMLStorage;

	// Custom object data handler - see <ObjectDataHandler.h> for full description and example.  
	XMLObjectDataHandler *m_pDataHandler;
	XMLObjectDataHandler *m_pDefaultDataHandler;
	
	// storage of Attribute mapping rules 
	GHash				*m_pMappedAttributeHash;

	// see GetObjectBehaviorFlags() in this header file
	unsigned int m_nBehaviorFlags;

	// see comment for setMemberStateSummary() in this header file.
	// object/member states are defined in xmlDefines.h
	unsigned int m_nMemberStateSummary;

protected:
	// storage for unmapped attributes is allocated only when necessary
	XMLAttributeList	*m_pAttributes;

	// The following members are set by the derived class during construction
	void *m_pDerivedAddress;	    // == the 'this' of derived class
	ObjectFactory m_classFactory;   // Pointer to factory for this object type
	void SetObjectTypeName(const char *);// sets m_strObjectType ex. 'CCustomer'
	void SetDefaultXMLTag(const char *);	// sets m_strXMLTag ex. 'Customer'

	// if this object was mapped by another object or
	// if this object is mapped into a list of another object
	// this list will contain the MemberDescriptor that
	// was used to create 'this' object, or the MemberDescriptor
	// that describes the containment relationship if 'this' was
	// not factory created.
	//
	// If multiple instances are active, this list holds all the maps
	// for each instance type, not of each instance.
	GList m_lstCreationDescriptors;

	// Member-to-Tag mapping tree root for alphabetical indexing by xml tag
	MemberDescriptor *m_EntryRoot;
	// Create member description structure and add to indexed structures
	void RegisterMember(MemberDescriptor *);
	// search based on xml tag for a member - used by the ObjectFactory
	MemberDescriptor *GetEntry( MemberDescriptor* btRoot,const char * szToFind );
	MemberDescriptor *GetEntry( MemberDescriptor* btRoot,void * pToFind );
	long m_nRefCount;

private:
	
	// If MapMember was called to store objects in GList, GHash or types supporting 
	// deferred destruction.  XMLFoundation can auto-cleanup all the objects it 
	// created - now it is done and it's time to cleanup the data structure
	void ExecuteDeferredDestructions();

	// see comment above XMLObject::IsFirstMap() in XMLObject.cpp
	GList m_lstDataStructures;
	int IsFirstMap( void *pList );

	MemberDescriptor *m_MRUObjectDescriptor;

	bool AddEntry( MemberDescriptor **Root,MemberDescriptor *ToAdd );

	// Storage for objects used to describe object relationships in XML
	GList *m_pRelationShipWrappers;// contains XMLRelationshipWrapper's

	void WriteObjectEndTag(  GString& xml, int nTabs, const char *TagOverride = 0, 	unsigned int nSerializeFlags = 0, __int64 nEndOfBeginTag = 0);
	int WriteObjectBeginTag( GString& xml, int nTabs, const char *TagOverride = 0,	unsigned int nSerializeFlags = 0 );

	// debugging support private methods
	void FormatMemberToDump(MemberDescriptor *pMD, GString &strDest, int nTabs, StackFrameCheck *pStack, const char *pzPlace);
	void DumpMember(        MemberDescriptor* btRoot, GString &strDest, int nTabs, StackFrameCheck *pStack);

protected:
	// List of nodes in a tree (alternate index to the same data) for 
	// sequential access and preservation of order in which MapMember() 
	// was called for full control of member location in the serialized XML.
	GList m_lstMembers; // contains MemberDescriptor;s

	// Have object "subscribe to data" not contained in member variables
	// see comment in ObjectDataHandler.h for full example.
	// You may want NO_MEMBER_MAP_PRECOUNT if you are using this.
	void setObjectDataHandler( XMLObjectDataHandler * p );

	// Forces a call to MapXMLTagsToMembers() if it has not already been done.
	void LoadMemberMappings();

	virtual void MapXMLTagsToMembers(){};
	virtual const char *GetVirtualTag(){return 0;};
	virtual GList *GetOIDKeyPartList(bool bNew){return 0;}

	// internal helper functions to the ToXML* methods
	void GenerateSubsetXML(MemberDescriptor* btRoot, GString& xml, int nTabs, StackFrameCheck *pStack, int nSerializeFlags, const char *pzSubsetOfObjectByTagName);
	void GenerateOrderedXML(MemberDescriptor* btRoot,GString& xml, int nTabs, StackFrameCheck *pStack, int nSerializeFlags);
	void GenerateMappedXML(MemberDescriptor* btRoot, GString& xml, int nTabs, StackFrameCheck *pStack, int nSerializeFlags);

	// Provides advanced mapping control to the object derived from XMLObject.  
	// Beware not to over-use a feature like this to solve situations that are 
	// more appropriately solved with calls to SetMemberSerialize() or by adjusting flags in the serialization. 
	// Think of it as a 'goto' in 'C++', it can be overused.
	// After a call to UnMapMembers(), the next operation on this object that
	// uses the object data description will re-execute MapXMLTagsToMembers().
	//   Setting [bRealloc] to 1/true will cause the MemberDescriptors to be reallocated new memory.
	//   Setting [bRealloc] to 1/true will never need to be done unless the number of MapMember() calls in MapXMLTagsToMembers() is dynamic.
	//   for example if you have code like:  if (nVar) MapMember(...)  in MapXMLTagsToMembers()
	void UnMapMembers( MemberDescriptor* btRoot = 0, bool bRealloc = 0 );

	// Called from Release() to recursively release children
	void ReferenceChangeChildren(StackFrameCheck* pStack, int nChange);
	void ApplyToChildren(MemberDescriptor *pMD, StackFrameCheck* pStack, int nChange);

public:

	XMLObject();

	// assign all the members possible in this object from the source object
	int CopyState(XMLObject *pCopyFrom);
	
	// special behavior - uses "object behavior flags" defined in xmlDefines.h
	inline unsigned int GetObjectBehaviorFlags(){return m_nBehaviorFlags;}
	void ModifyObjectBehavior(unsigned int nAdd, unsigned int nRemove = 0)
	{
		m_nBehaviorFlags = (m_nBehaviorFlags & ~nRemove) | nAdd;
	}

	// state attachment/detatchment from StateCache
	void ReStoreState(__int64 oid);
	void ReStoreState(const char * oid);
	void StoreState();

	// called after the object members have been initialized from the XML during 
	// a call to FromXML(), it is called EACH time the object is updated.
	virtual void Construct() {};
	// called by the XMLObjectFactory when OID's are pre-specified in the attribute
	// list of the source XML data stream. Faster -  no temp objects are used.
	void Init(const char* m_oid, const char* pzTimeStamp );
	// called by the XMLObjectFactory on the behavior flag EARLY_CONTAINER_NOTIFY
	void EarlyNotify();

    // this supports data that is neither  
	// a child element nor an attribute.  For Example:
	//<Strangeness>"
    //    "Where "
	//    "<String>String Value</String>"
    //    "Does "
	//   "<Integer>777</Integer>"
    //   "This valid XML go?"
	//"</Strangeness>";
	//
	// If 'this' object mapped "String" and "Integer" then  GetObjectDataValue() will be:
	// "Where Does This valid XML go?"
	GString *GetObjectDataStorage();
	void GetObjectDataValue(GString &xml);
	void SetObjectDataValue(const char *);

	GString *GetCDataStorage();


	// Access to attributes of the object, if nUpdate=1 the attribute list will
	// be searched and update if there is an existing attribute called [pzName]
	// if not found or if nUpdate=0 the new attribute will be added.
	void AddAttribute( const char * pzName, int nValue, int nUpdate=0 );
	void AddAttribute( const char * pzName, const char * pzValue, int nUpdate=0 );

	// returns 0 if the attribute is NOT found.  
	int FindAttribute( const char *pzAttName, GString &strDest);
	
	// Use the FindAttribute() that requires a GString in the 2nd arg for threading safety
	// returns null if the attribute is not found
	const char *FindAttribute( const char *pzAttName );

	void RemoveAllAttributes();

	// set attributes of members, rarely used - should your member be an object?
	void AddMemberAttribute( const char *pzMemberTag, const char *pzName, int nValue );
	void AddMemberAttribute( void *pMemberAddress,    const char *pzName, int nValue );
	void AddMemberAttribute( const char *pzMemberTag, const char *pzName, const char * pzValue );
	void AddMemberAttribute( void *pMemberAddress,    const char *pzName, const char * pzValue );

	// Reach out to the farthest derivation and put that 'this' pointer
	// into m_pDerivedAddress, so that through a template accessor or
	// static cast, specifying the derived TYPE we may downcast 'that',
	// (that is cast from derived toward the base), either base in the 
	// case of multiple inheritance.  COM/CORBA use MI.
	virtual void RegisterObject(){}
	virtual const char *GetVirtualType(){return 0;};

	// This resolves implicit upcasting to the object that is multiply
	// derived from CORBA::Object and XMLObject. When called from the
	// derived class the value returned will equal the 'this' pointer
	// of the outermost derived class, that can be then safely downcast
	// to any one of it's multiply inherited base classes.
	void *GetUntypedThisPtr() {return m_pDerivedAddress;}
	void *GetInterfaceObject() {if (!m_pDerivedAddress) RegisterObject(); return m_pDerivedAddress;}
	virtual ~XMLObject();

	// Returns true if any child member in this object or any sub object has a state of dirty.
	int CalculateObjectState( MemberDescriptor* btRoot = 0 );
	
	// access to runtime type and bound XML tag.
	const char *GetObjectType();// type as 1st param to IMPLEMENT_FACTORY					
	const char *GetObjectTag();	// xml tag as 2nd param to IMPLEMENT_FACTORY
	const char *GetXMLTag(){return GetObjectTag();}// less direct, always equal

	// If the runtime instance of 'this' is contained in an object that
	// used MapMember() to define the type relationship, then calling
	// GetParentObject() will return a pointer to the instance of the owner.
	XMLObject *GetParentObject();
	// if the refcount on 'this' is > 1 and the 'owning' objects are not the same
	// for example this 'Item' is on several 'Invoice' objects all referring to 
	// the same 'this'(Item), this method will allow you to call a 'Changed'
	// method on all active parent objects that may need to make screen updates.
	GList *GetParentObjects(GList *lstResults); // returns lstResults

	// Loads m_lstOIDKeyParts:
	// Map a set of unchanging data on this object that uniquely identifies it
	// between all objects of this type.  This will be called from your 
	// MapXMLTagsToMembers() only once.  It is optional if you want automatic
	// cache management. ObjectID's (oid's) may consist of 1 to 5 key parts 
	// that can be elements, attributes, or a combination of both.  
	// nSource=1 for Elements, or 0 is the preceeding tag is an Attribute
	void MapObjectID(const char *pTag1,   int nSource1, 
					 const char *pTag2=0, int nSource2=0,
					 const char *pTag3=0, int nSource3=0,
					 const char *pTag4=0, int nSource4=0,
					 const char *pTag5=0, int nSource5=0);
	
	// map an unknown, let the derived class handle this member variable
	// see: MemberHandler.h for a complete description of this functionality
	void MapEntity(const char *pzEntityName);
	void MapMember(const char *pTag, MemberHandler *pHandler);

	// Map bits within a 8,16,32, or 64 bit space.
	// nBit1toN is a 1 based index value of the bit within pValue
	// pzCommaSeparated0Values might be "No,OFF,0,false" if you want those case insensitive values (False==FALSE==false) to set the bit to 0
	// since "No" is first in the list, that will be used to indicate the bit is 0 when creating the xml.
	// pzCommaSeparated1Values might be "Yes,On,1,True" if you want those case insensitive (YES==Yes==yes) values to set the bit to 1
	// since "Yes" is first in the list, that will be used to indicate the bit is 1 when creating the xml.
	// The lists are parsed, and cached, during the first call to MapXMLTagsToMembers()
	void MapMemberBit(unsigned char *	pValue,const char *pTag,int nBit1to8,const char *pzCommaSeparated0Values, const char *pzCommaSeparated1Values);
	void MapMemberBit(unsigned short *	pValue,const char *pTag,int nBit1to16,const char *pzCommaSeparated0Values, const char *pzCommaSeparated1Values);
	void MapMemberBit(unsigned int *	pValue,const char *pTag,int nBit1to32,const char *pzCommaSeparated0Values, const char *pzCommaSeparated1Values);
	void MapMemberBit(unsigned __int64 *pValue,const char *pTag,int nBit1to64,const char *pzCommaSeparated0Values, const char *pzCommaSeparated1Values);


	///////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Using Translation Maps (new in November 2013)
	//////////////////////////////////////////////////////////////////-/////////////////////////////////////////
	// pzTranslationMapIn translates values - for example this is how you might use it on a String:
	// if pzTranslationMapIn = "red*=Red,Mary Jane=Green,Grass=Green,XML_EMPTY_VALUE=Blue,XML_DEFAULT=Unknown
	//                          ---------------------------------------------------------
	// if <pTag> in the source xml starts with red - like "redness" or "reddish" it assigns member pValue = "Red"
	// if <pTag> in the source xml is "Mary Jane" or "Grass" - it assigns member pValue = "Green"
	// if <pTag></pTag> is empty in source xml - it assigns member pValue = "Blue"
	// <Tag></Tag> will assign a mapped m_member = "", unless XML_EMPTY_VALUE in pzTranslationMapIn overrides default behavior
	//    which will set m_strString2 == "Blue" due to XML_EMPTY_VALUE=Blue in pzTranslationMapIn.
	//
	// pzTranslationMapOut translates values in the XML to differ from what the member variable stores - for example:
	// if pzTranslationMapOut = "1=One,2=Two,777=Root,any*=anyTHING"
	// if pValue = "1" it will be seen as "One" in the XML.
	// if pValue = "2" it will be seen as "Two" in the XML.
	// if pValue = "777" it will be seen as "Root" in the XML.
	// if pValue starts with "any" (like "anyone" or "Anytime" (all wildward searches are case insensitive)) it comes out as "anyTHING" in the XML.
	//
	// if XML_DEFAULT is specified and there is no known translation for the value, this value will be used.
	// if XML_DEFAULT is not specified and there is no known translation for the value, the untranslated value will be used
	// if XML_DEFAULT= is followed by a comma or null like this: "A=1,B=2,XML_DEFAULT=,C=3" or like this "A=1,B=2,C=3,XML_DEFAULT="
	//    then the value with no known translation will NOT be used, and the value will be "" aka empty.
	//
	//
	// .....and this is how you might use it on an Integer:
	// pzTranslationMapIn  = "1=10,2=20,3=777,one=10,two=20,three=777,XML_EMPTY_VALUE=-777,XML_DEFAULT=0"
	// pzTranslationMapOut = "10=ten,20=twenty,777=infinity,XML_DEFAULT=0"
	//
	//	
	// and the last argument, nTranslationFlags can be any combinations of these values:
	// XLATE_CASE_INSENSITIVE_IN		// When assigning variables from XML
	// XLATE_CASE_INSENSITIVE_OUT		// When writing XML translations
	// XLATE_NO_IN_WILDCARDS			// you never NEED to set this, as it is calculated and cached.

	///////////////////////////////////////////////////////////////////////////////////////////////////////////


	// Called by the Object Factory when assigning Attributes values to 'this'
	int SetMappedAttribute(const char *pTag,const char *pzValue, __int64 nValueLen);
	// Map an integer
	void MapAttribute(int *pValue,const char *pTag, const char *pzTranslationMapIn = 0, const char *pzTranslationMapOut = 0,int nTranslationFlags = 0);
	// Map a string, see StringAbstraction.h for interface and samples
	void MapAttribute(GString *pstrValue,const char *pTag, const char *pzTranslationMapIn = 0, const char *pzTranslationMapOut = 0,int nTranslationFlags = 0);
	void MapAttribute(GString0 *pstrValue,const char *pTag, const char *pzTranslationMapIn = 0, const char *pzTranslationMapOut = 0,int nTranslationFlags = 0);
	void MapAttribute(GString32 *pstrValue,const char *pTag, const char *pzTranslationMapIn = 0, const char *pzTranslationMapOut = 0,int nTranslationFlags = 0);
	void MapAttribute(void *pValue,const char *pTag,StringAbstraction *pHandler, const char *pzTranslationMapIn = 0, const char *pzTranslationMapOut = 0,int nTranslationFlags = 0);

	// Map a bool, char, short, int ,long int, or very long int
	// nBoolReadability =   0=Yes/No       1=True/False       2=On/Off		3=1/0
#ifndef _NO_XMLF_BOOL 
	void MapMember(bool *pValue,const char *pTag, int nBoolReadability = 1);	// bool (re)added in 2013
#endif// some old compilers #define bool as char or short, creating 2 identical method signatures.  To build XMLFoundation with those older compilers, simply #define _NO_XMLF_BOOL in xmlDefines.h
	void MapMember(char *pValue,const char *pTag);	
	void MapMember(short *pValue,const char *pTag, const char *pzTranslationMapIn = 0, const char *pzTranslationMapOut = 0,int nTranslationFlags = 0);
	void MapMember(int *pValue,const char *pTag, const char *pzTranslationMapIn = 0, const char *pzTranslationMapOut = 0,int nTranslationFlags = 0);
	void MapMember(long *pValue,const char *pTag, const char *pzTranslationMapIn = 0, const char *pzTranslationMapOut = 0,int nTranslationFlags = 0);
	void MapMember(__int64 *pValue, const char *pTag, const char *pzTranslationMapIn = 0, const char *pzTranslationMapOut = 0,int nTranslationFlags = 0);
	void MapMember(double *pValue, const char *pTag);
	
	// map to a char[n] array - a char string but not in a string class like GString or CString
	void MapMember(char *pValue,const char *pTag,int nMaxLen, const char *pzTranslationMapIn = 0, const char *pzTranslationMapOut = 0,int nTranslationFlags = 0);

	// Map a string, see StringAbstraction.h for interface and samples
	void MapMember(GString *pValue,const char *pTag, const char *pzTranslationMapIn = 0, const char *pzTranslationMapOut = 0,int nTranslationFlags = 0);
	void MapMember(GString0 *pValue,const char *pTag, const char *pzTranslationMapIn = 0, const char *pzTranslationMapOut = 0,int nTranslationFlags = 0);
	void MapMember(GString32 *pValue,const char *pTag, const char *pzTranslationMapIn = 0, const char *pzTranslationMapOut = 0,int nTranslationFlags = 0);
	void MapMember(void *pValue,const char *pTag,StringAbstraction *pHandler, const char *pzTranslationMapIn = 0, const char *pzTranslationMapOut = 0,int nTranslationFlags = 0);

	// Map an object into a hash table, binary tree, or QSort array
	// Note: You must Map an OID to use these.
	void MapMember(GHash *pDataStructure,const char *pzObjectName,const char *pNestedInTag = 0);
	void MapMember(GBTree *pDataStructure,const char *pzObjectName,const char *pNestedInTag = 0);
	void MapMember(GQSortArray *pDataStructure,const char *pzObjectName,const char *pNestedInTag = 0);
	void MapMember(void *pDataStructure,KeyedDataStructureAbstraction *pHandler,const char *pzObjectName,const char *pNestedInTag = 0);

	// Map a collection of Strings
	void MapMember(GStringList *pStringCollection,	const char *pzElementName,const char *pNestedInTag = 0, const char *pzTranslationMapIn = 0, const char *pzTranslationMapOut = 0,int nTranslationFlags = 0);
	void MapMember(void *pStringCollection,	const char *pzElementName,StringCollectionAbstraction *pHandler,const char *pNestedInTag = 0, const char *pzTranslationMapIn = 0, const char *pzTranslationMapOut = 0,int nTranslationFlags = 0);

	// Map a dynamically growing Integer array
	void MapMember(GArray *pIntegerArray,	const char *pzElementName,const char *pNestedInTag = 0, const char *pzTranslationMapIn = 0, const char *pzTranslationMapOut = 0,int nTranslationFlags = 0);
	void MapMember (void *pIntegerArray,	const char *pzElementName,IntegerArrayAbstraction *pHandler,const char *pNestedInTag = 0, const char *pzTranslationMapIn = 0, const char *pzTranslationMapOut = 0,int nTranslationFlags = 0);

	// Map a collection of Objects in a List
	void MapMember(void *pList,const char *pObjectTag,ListAbstraction *pHandler,const char *pNestedInTag = 0,ObjectFactory pFactory = 0 );
	void MapMember(GList *pList,const char *pObjectTag,const char *pNestedInTag = 0,ObjectFactory pFactory = 0 );

	// Map a sub-object using a tag other than defined in the DECLARE_Factory
	void MapMember(XMLObject *pObj,	const char *pDefaultTagOverride = 0,const char *pzWrapper = 0 );
	// Map a sub-object pointer to an object residing in the ObjectCache
	void MapMember(XMLObject **pObj,const char *pzTag,const char *pNestedInTag= 0,ObjectFactory pFactory=0);


	// Allows the default state of 'true' for member serialization to be changed.
	// This is how backward compatibility is achieved when Element tag
	// names change.  For example, if m_nVersion is mapped to an Element
	// named "VersionNumber" but you want all future protocols refer to this 
	// Element as to "ProtocolVersion".  This is achieved by the following code:
	// (...)
	//MapMember(&m_nVersion,"VersionNumber");
	//MapMember(&m_nVersion,"ProtocolVersion");
	//SetMemberSerialize("VersionNumber", false );
	// (...)
	// This allows either "VersionNumber" or "ProtocolVersion" to set the value of 
	// m_nVersion, but always refers to it as "ProtocolVersion" while serializing XML.
	void SetMemberSerialize(const char *pTag, bool bSerialize);
	
	// override this to conditionally include an object/subobject in ToXML() results.
	// see RelationshipWrapper.h for implementation example.
	virtual bool SerializeObject() {return true;};


	// given the address of a member, return it's XML Tag defined in MapXMLTagsToMembers()
	const char *GetMemberTag(void *pAddressOfMemberToSet);

	// call this to mark an object for serialization, for example if a 
	// list of inventory objects is loaded from XML, and several new inventory
	// objects are created by the user and added to the list, calling
	// setObjectDirty() on each new inventory object allows you to serialize
	// the list and write out only the 'new' objects possibly for a DBMS insert.
	// If the bAllMembers flag is '1', then every member in this object is also
	// set as dirty.
	void setObjectDirty(int bAllMembers = 0);

	// When objects are populated from the XML stream, they have a state of
	// Not dirty and Not null.  When the members are assigned by the derived 
	// class through SetMember() or SetMemberByTag() the state becomes dirty.
	// When using the SetMember() or SetMemberByTag() there is no need to 
	// setMemberDirty(), but if the derived class does direct assignments
	// to members that should be serialized, setMemberDirty() should be called.
	// if you ever want to serialize or track the state change only.
	// Setting bDirty to 0 will clear the dirty flag for a member variable.
	bool setMemberDirty(void *pAddressOfMemberToSet, int bDirty = 1);
	bool setMemberDirty(char *pzTagNameOfMemberToSet, int bDirty = 1);

	// true if the memory state does not sync with the 
	// original value set by the Object Factory
	bool isMemberDirty(void *pAddressOfMemberToCheck);
	bool isMemberDirty(char *pzTagNameOfMemberToCheck);

	// true if the member has never been assigned a value, it is uninitialized
	bool isMemberNull(void *pAddressOfMemberToCheck);
	bool isMemberNull(char *pzTagNameOfMemberToCheck);

	// true if the member has been assigned a value from the Object Factory
	bool isMemberCached(void *pAddressOfMemberToCheck);
	bool isMemberCached(char *pzTagNameOfMemberToCheck);
	
	
	// FromXML() Loads this object (Serialize In) and all it's child objects from an xml source.
	// FromXML() Throws an XMLException if anything goes wrong.  
	// Generally you supply only the 1st argumnent a pointer to the source xml or the filename of it.
	// Generally you allow the other args to default to 0
	// If a pSecondaryHandler is supplied it will be queried for storage handlers for any items that are not mapped to this object or one of it's child objects.  XMLProcedureCall is an example pSecondaryHandler.
	// The optional pzErrorSubSystem will prefix the Exception text if an XMLException is serialized in as XML and re-thrown, as XMLProcedureCall does, and uses this to determine where in a distributed system it was thrown from.
	// NOTE: Throws GException
	void FromXML(	 const char *pzXML,			XMLObject *pSecondaryHandler=0,const char *pzErrorSubSystem=0);
	// (if you get a MSFT compile error here(2059 and 2238) move the #include <XMLFoundation.h> to your stdafx.h)
	void FromXMLFile(const char *pzFileName,	XMLObject *pSecondaryHandler=0,const char *pzErrorSubSystem=0);
	
	// Same as FromXML() and FromXMLFile(), but it does NOT throw an exception.
	// this is true of any XMLObject method name that is followed by an X
	// returns 1 on success, 0 for failure
	// if you don't want error details, leave [pErrorDescriptionDestination] NULL
	// if [pErrorDescriptionDestination] points to a GString, that string will contain the error string
	int FromXMLX(const char *pzXML, GString *pErrorDescriptionDestination = 0, XMLObject *pSecondaryHandler = 0);
	int FromXMLFileX(const char *pzFileName, GString *pErrorDescriptionDestination = 0, XMLObject *pSecondaryHandler = 0);
	// if FromXMLFile() returns 1 on success and 0 for failure
	// load [pzFileName] into [xmlData], populates 'this' from the source XML that will be in [xmlData].
	bool FromXMLFileX(GString& xmlData, const char *pzFileName, GString *pErrorDescriptionDestination = 0);


	// ToXML
	//----------------------------------------------------------------------------------------------
	// Recurse this object structure to a linear buffer represented in XML
	// into a GString.  Any existing value in the GString will be replaced.
	// [xml] = Destination
	// [nTabs] = normally 0-the level of tab indentation of the lowest branch
	// [TagOverride] = null or the tag name of the root branch,
	// [nSerializeFlags] = any pipe separated combination of the XML serialization flags found in xmlDefines.h
	// [pStack] = always null for public access or passed to you in the virtual - this is an internal state check to prevent infinate recursion(NOT allowed) due to circular object references(Are Allowed) example Customer has list of Customers and the outer instance is referenced in the list - that otherwise would serialize for infinity and that is not allowed as performace is critical to success.
	// [pzSubsetOfObjectByTagName] = pipe separated list of members to serialize for example,"name|phone|id" otherwise null to serialize all members. Consider SetMemberSerialize() rather than [pzSubsetOfObjectByTagName]
	// [nPreAllocSize] = Pre-estimated result size for [MUCH] better performance on large result sets.
	void ToXML(GString& xml, int nSerializeFlags = FULL_SERIALIZE);
	const char * ToXML(__int64 nPreAllocSize = 4096, unsigned int nSerializeFlags = FULL_SERIALIZE);
	virtual bool ToXML(GString* xml, int nTabs = 0, const char *TagOverride = 0, unsigned int nSerializeFlags = FULL_SERIALIZE, StackFrameCheck *pStack = 0, const char *pzSubsetOfObjectByTagName = 0);

	//             The VIRTUAL ToXML()  ==  Custom Object Serialization
	// All other forms of ToXML ultimately call this one, and it is the only virtual ToXML().
	// You can override object behavior for WRITING XML by overloading this method.
	// Look at this file - right click in your debugger here -> ".\Examples\C++\BigData\BigData.cpp"
	// Look at "Case 1", See how ToXML() can be overloaded for custom object serialization.
	// ObjectDataHandler.h contains an example of an alternate Serialization design to choose from.


	// File is created or overwritten unless nAppend = 1
	// returns 1 for success, 0 for fail.  If bThrowOnFail is 1, an exception will be thrown on failure
	bool ToXMLFile(__int64 nPreAllocateBytes, const char *pzPathAndFileName, bool bThrowOnFail = 1, int nAppend = 0, unsigned int nSerializeFlags = FULL_SERIALIZE);
	bool ToXMLFile(const char *pzPathAndFileName, int nTabs = 0, const char *TagOverride = 0, int nSerializeFlags = FULL_SERIALIZE, int nEstimatedBytes = 8192);
	bool ToXMLFile(GString& xmlDestination, const char *pzFileName, int nSerializeFlags = FULL_SERIALIZE);
	//----------------------------------------------------------------------------------------------


	// Called by the XML Parser to obtain storage for a tag value  Search by tag name
	virtual MemberDescriptor *GetEntry( const char * szTagToFind );
	// or by memory address.  For example:   GetEntry( &m_strFirstName );
	// to keep xml tag names central to one location-->MapXMLTagsToMembers()
	MemberDescriptor *GetEntry( void * pMemberToFind );


	// Contains summary state info, for example if ANY singe member is dirty
	// then the DATA_DIRTY bit will be on.  If ANY singe member is cached
	// then the DATA_CACHED bit will be on.  If you are interested in the 
	// state of a specific member, use the isMemberDirty() method.
	void setMemberStateSummary(int nBitFlag) {	m_nMemberStateSummary |= nBitFlag;  }
	bool getMemberStateSummary(int nBitFlag) { return ( ( m_nMemberStateSummary & nBitFlag ) != 0);  }


	// Set object member variables and maintain dirty state flag values
	void SetMember( void *pMemberToSet, const char *pzNewValue,  MemberDescriptor* btRoot = 0 );
	void SetMember( void *pMemberToSet, int pzNewValue, 	     MemberDescriptor* btRoot = 0 );

	// Generic Event Handler for custom object behavior. Abstract to reduce virtual method table
	// -----------------------------------------------------------------------------------------
	// nCase = "XMLAssign", (member tag, XML value, value len, null)
	//			when the FromXML() contains a value for a member set dirty by setMemberDirty()
	//			see MemberDescriptor::Set() in MemberDescriptor.cpp for details.
	// nCase = "NonNumeric" (member tag, XML value, value len, null)
	//			 when non-numeric XML data is mapped to an numeric only type.
	// nCase = "EmptyString"(member tag, XML value, value len, null)
	//			 when an empty("") value is assigned to a string, empty often differs from 'unknown' or 'unassigned'
	// nCase = "ObjectUpdate" when OBJECT_UPDATE_NOTIFY is a set behavior flag. (oid, null, flags, pObjSrc)
	// nCase = "XMLUpdate"    when OBJECT_UPDATE_NOTIFY is a set behavior flag. (oid, null, flags, pObjSrc)
	// nCase = "SubObjectUpdate" when SUBOBJECT_UPDATE_NOTIFY is a set behavior flag. (xml tag, null, flags, pObjSrc)
	// nCase = "MemberUpdate" when MEMBER_UPDATE_NOTIFY is a set behavior flag. (tag, value, valuelen, null)
	//
	// To turn on "ObjectUpdate" messages use this code in your constructor: ModifyObjectBehavior(OBJECT_UPDATE_NOTIFY);
	// You should return 0(NULL) for all message definitions as of 2012 - in the future new messages may require special return values
	virtual void *ObjectMessage( int nCase, const char *pzArg1, const char *pzArg2, __int64 nArg3 = 0, void *pArg4 = 0 ){return 0;};


	// When a tag is encountered that does not have a MapMember() entry
	// associated with it, this handler is called for the developer
	// to supply an "on-the-fly" MemberDescriptor during the Factory process.
	// This is useful for dynamic objects. 
	virtual MemberDescriptor *HandleUnmappedMember( const char *pzTag );
	//	For example,
	// MemberDescriptor *YourObject::HandleUnmappedMember( const char *pzTag )
	// {

	// 	// This is the equivalent of a dynamic MapMember().  During the Factory
	// 	// creation process, we found a tag with no static map, and here we 
	// 	// will map it to a GString.
	// 	GString *pValue = new GString();
	// 	static GenericStringAbstract GSA;
    //
	//	MemberDescriptor *pMD = new MemberDescriptor(this, pzTag, pValue, &GSA);
	//	
	//	// place pMD in the base class (where the MemberDescriptor is normally created)
	//	// so it gets cleaned up when the object goes out of scope.
	//	RegisterMember(pMD);
	//
	//	// hang onto pValue since it was dynamically created by this class, it 
	//	// will get cleaned up by this class also.
	//	m_listDynamicStringMembers.AddLast(pValue);
	//	return pMD;
	// }



	//	Set the value of a member variable using it's XML Tag name as a unique identifier
	void SetMemberByTag( const char *pzTagName, const char *pzNewValue, bool bSetDirty = true);
	void SetMemberByTag( const char *pzTagName, GString &strNewValue, bool bSetDirty = true);
	const char *GetMemberByTag( const char *pzTagName, GString &strDest );
	int GetMemberByTagInt( const char *pzTagName );

	// returns NULL or a string to uniquely identify this object among all instances of this type.
	const char *getOID();
	// returns NULL or a string that contains an object time stamp.
	const char *getTimeStamp();

	
	// the object descriptor that created, or references 'this'	
	int IsObjectDescriptor(MemberDescriptor *p);
	int GetObjectDescriptorCount();
	int SetObjectDescriptor(MemberDescriptor *p);
	MemberDescriptor *GetObjectDescriptor(); // returns Most Recently Used Descriptor

	// see comment above [m_pDataHandler]
	XMLObjectDataHandler *GetObjectDataHandler();


	// assign a pre-built attribute list to an object that has no preexisting attributes
	void SetAttributeList(XMLAttributeList *p);

	// Dumps Entire Object state with all object associations to standard out
	const char *Dump();
	// Dumps Entire Object state with all object associations to a file
	// tip: use a good editor to view that allows you to press Ctrl-} 
	//      when your cursor is positioned on a brace'{' to find pairs of {}
	void Dump( const char *pzFileName, int bAppendToFile = 0 );
	// Append an Object state dump onto the supplied GString;
	void Dump(GString &strDest, int nTabs = 0, StackFrameCheck *pStack = 0);

	// Reference counting.
	// always use a NULL pStack for public access, 
	// almost always nDeep=1 to Inc() references of containment, unless custom refcounting.
	long IncRef(StackFrameCheck *pStack =0, int nDeep=1);
	
	// Decrements the reference count for this object.
	// If the count drops to zero, the object deletes itself.
	// nSubObjectsOnly=1 to DecRef() all contained objects but not 'this' object
	// nSubObjectsOnly=1 when 'this' is on the stack and should not be deleted on a refcount of 0
	long DecRef(int nSubObjectsOnly = 0, StackFrameCheck *pStack =0);
	// To DecRef() all contained objects, but not 'this'
	void DecRefContained(){ DecRef(1); m_nBehaviorFlags |=PREVENT_AUTO_DESTRUCTION;}
	long GetRefCount(){ return m_nRefCount; }

};

// runtime maps of active objects for extended object navigation.
class MapOfActiveObjects
{
	GHash m_hshActiveMaps;
public:
	MapOfActiveObjects(){}
	~MapOfActiveObjects(){}
	void AddActiveObjectMap(MemberDescriptor *pNew);
	void RemoveActiveObjectMap(MemberDescriptor *pOld);
	int IsActiveObjectMap(MemberDescriptor *pMap);
};
extern MapOfActiveObjects g_ActiveObjectMap;


#define GET_FACTORY(class_name) (class_name::FactoryCreate)

// Macro to create a typed instance of a derivative
// object, and return via common base class type.
// DECLARE_FACTORY reserves xml_tag for future use

#define DECLARE_FACTORY(class_name, xml_tag)	public:	virtual int GetMemberMapCount(char bIncrement);  virtual GList *GetOIDKeyPartList(bool bNew);  static const char *GetStaticTag();	static const char *GetStaticType();	static class_name *Attach(const char *pzOid);	static class_name *Attach(int nOid);	const char *GetVirtualType();	const char *GetVirtualTag();	static ObjectFactory GetStaticFactory(); static XMLObject* FactoryCreate();	void RegisterObject();
#define DECLARE_XML(class_name)                 public: virtual int GetMemberMapCount(char bIncrement);  virtual GList *GetOIDKeyPartList(bool bNew);  static const char *GetStaticTag();	static const char *GetStaticType();	static class_name *Attach(const char *pzOid);	static class_name *Attach(int nOid);	const char *GetVirtualType();	const char *GetVirtualTag();	static ObjectFactory GetStaticFactory(); static XMLObject* FactoryCreate();	void RegisterObject();

class RegisterBusinessObject
{
public:
	RegisterBusinessObject(	ObjectFactory, const char * szXMLTagIdentifier, const char * szBusObjClassName);
};


// MS VS2005 is 1400
// VC6 is 1200
// vs2012 needs this and VC6 cant have it defined
#	if defined(_MSC_VER) && _MSC_VER >= 1400
#define XSPRINTF sprintf_s
#else
#define XSPRINTF sprintf
#endif

class GlobalKeyPartLists
{
public:
	GList m_lst;
	GlobalKeyPartLists(){}
	~GlobalKeyPartLists()
	{
		GListIterator it(&m_lst);
		while (it())
		{
			delete (GList *)it++;
		}
	}
};

extern GlobalKeyPartLists g_KeyPartsListCleanup; // in xmlObject.cpp


#define IMPLEMENT_FACTORY_NO_REG(class_name, xml_tag)					\
int class_name::GetMemberMapCount(char bIncrement)						\
{																		\
	static int class_name##MemberMapCount = 0;							\
	if (bIncrement==2){ class_name##MemberMapCount = 0; return 0;}		\
	return (bIncrement==1) ? ++class_name##MemberMapCount : class_name##MemberMapCount;	\
}																		\
const char * class_name::GetVirtualTag()								\
{																		\
	return class_name::GetStaticTag();									\
}																		\
GList *class_name::GetOIDKeyPartList(bool bNew)							\
{																		\
	static GList *class_name##OIDList = 0;								\
	if (bNew && class_name##OIDList == 0)								\
	{																	\
		class_name##OIDList = new GList();								\
		g_KeyPartsListCleanup.m_lst.AddLast(class_name##OIDList);		\
	}																	\
	return class_name##OIDList;											\
}																		\
class_name * class_name::Attach(int nOid)								\
{																		\
	char pzOid[20];														\
	XSPRINTF(pzOid,"%d",nOid);											\
	XMLObject *p = cacheManager.getObject(#xml_tag,pzOid,"",0);			\
	return (class_name *)p->GetUntypedThisPtr();						\
}																		\
class_name *class_name::Attach(const char *pzOid)						\
{																		\
	XMLObject *p = cacheManager.getObject(#xml_tag,pzOid,"",0);			\
	return (class_name *)p->GetUntypedThisPtr();						\
}																		\
const char * class_name::GetStaticTag()									\
{																		\
	static const char *pXMLTag = #xml_tag;								\
	return pXMLTag;														\
}																		\
ObjectFactory class_name::GetStaticFactory()							\
{																		\
	return class_name::FactoryCreate;									\
}																		\
const char * class_name::GetStaticType()								\
{																		\
	static const char *pClassName = #class_name;						\
	return pClassName;													\
}																		\
const char * class_name::GetVirtualType()								\
{																		\
	return class_name::GetStaticType();									\
}																		\
void class_name::RegisterObject()										\
{																		\
	SetObjectTypeName(#class_name);										\
	SetDefaultXMLTag(#xml_tag);											\
	m_pDerivedAddress = this;											\
	m_classFactory = class_name::FactoryCreate;							\
}																		\
XMLObject* class_name::FactoryCreate()									\
{																		\
	class_name *pTemp = new class_name();								\
	pTemp->RegisterObject();											\
	return (XMLObject*)pTemp;											\
}																		\

// ***********		IMPLEMENT_FACTORY	**************
#define IMPLEMENT_FACTORY(class_name, xml_tag)							\
IMPLEMENT_FACTORY_NO_REG(class_name, xml_tag)							\
RegisterBusinessObject class_name##Unique(								\
					class_name::FactoryCreate,							\
					#xml_tag,											\
					#class_name);										\

/*
#define IMPLEMENT_XML(class_name, xml_tag)	
IMPLEMENT_FACTORY_NO_REG(class_name, xml_tag)	
RegisterBusinessObject class_name##Unique(
				class_name::FactoryCreate, #xml_tag,#class_name);
*/



#define CORBA_COM_BASE(class_name, xml_tag)								\
int class_name::GetMemberMapCount(char bIncrement)						\
{																		\
	static int class_name##MemberMapCount = 0;							\
	if (bIncrement==2){ class_name##MemberMapCount = 0; return 0;}		\
	return (bIncrement==1) ? ++class_name##MemberMapCount : class_name##MemberMapCount;	\
}																		\
GList *class_name::GetOIDKeyPartList(bool bNew)							\
{																		\
	static GList *class_name##OIDList = 0;								\
	if (bNew && class_name##OIDList == 0)								\
	{																	\
		class_name##OIDList = new GList();								\
		g_KeyPartsListCleanup.m_lst.AddLast(class_name##OIDList);		\
	}																	\
	return class_name##OIDList;											\
}																		\
const char * class_name::GetVirtualTag()								\
{																		\
	return class_name::GetStaticTag();									\
}																		\
const char * class_name::GetVirtualType()								\
{																		\
	return class_name::GetStaticType();									\
}																		\
																		\
const char * class_name::GetStaticTag(){								\
	static const char *pXMLTag = #xml_tag;								\
	return pXMLTag;														\
}																		\
ObjectFactory class_name::GetStaticFactory()							\
						{return class_name::FactoryCreate;}				\
const char * class_name::GetStaticType(){								\
	static const char *pClassName = #class_name;						\
	return pClassName;													\
}																		\
void class_name::RegisterObject()										\
{																		\
	SetObjectTypeName(#class_name);										\
	SetDefaultXMLTag(#xml_tag);											\
	m_pDerivedAddress = this;											\
	m_classFactory = class_name::FactoryCreate;							\
}																		\



// Bridge to Microsoft's COM ATL (Active Template Library)
#define IMPLEMENT_ATL_FACTORY(class_name, xml_tag)						\
CORBA_COM_BASE(class_name, xml_tag)										\
																		\
XMLObject* class_name::FactoryCreate()									\
{																		\
	CComObject<class_name>* pTemp = NULL;								\
	CComObject<class_name>::CreateInstance(&pTemp);						\
	pTemp->RegisterObject();											\
	return (XMLObject*)pTemp;											\
}																		\
																		\
RegisterBusinessObject class_name##Unique(								\
				class_name::FactoryCreate,								\
				#xml_tag,												\
				#class_name);											\


// Bridge to OMG's ORB (Object Request Broker)
#define IMPLEMENT_ORB_FACTORY(class_name, xml_tag)						\
CORBA_COM_BASE(class_name, xml_tag)										\
																		\
XMLObject* class_name::FactoryCreate()									\
{																		\
	class_name *pImpl = new class_name();								\
	pImpl->RegisterObject();											\
	return (XMLObject *)pImpl;											\
}																		\
																		\
RegisterBusinessObject class_name##Unique(								\
				class_name::FactoryCreate,								\
				#xml_tag,												\
				#class_name);											\


#endif //_XML_BASE_OBJECT_SUPPORT__
