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



// This class contains methods to resolve type information for user 
// defined processing behavior.  When an object derived from XMLObject 
// calls MapMember() it has the opportunity to map the members so that the 
// base class handles all the details of dirty/get/set/streaming and 
// type information  or this can be supplied by the derived class.  When 
// MapMember() is called with a single parameter of only a tag followed
// by a derivation of this class, then the datatype is set to 
// 'ManagedByDerivedClass' and the handler becomes responsible for
// complete behavior implementation.
// ************************************************************************

#ifndef _USER_MEMBER_HANDLER_BASE_
#define _USER_MEMBER_HANDLER_BASE_

// ** Design note **
// If you use "one handler instance per member" then you will never 
// be interested in value of pzTag that is supplied to each member.  
// You can implement this handler to handle all the tags for a specific 
// object in which case your return value may be different depending 
// on which member this method call pertains to.
// ** Design note **


class MemberHandler
{
public:
	// return true if the value is "unknown".  If SetMember() has not 
	// been called and you have not taken special steps to obtain 
	// an initial value return true.  In this case, "null" may not 
	// literally be a zero address, null may be "not = to the Database value".
	virtual bool IsNull(const char *pzTag) = 0;

	// if SetMember() has been called by the framework where the value
	// comes from the database, then this member's value is cached if you can
	// recall it on demand.  If the value is set by a client (not the database)
	// it is not null, but is also not cached.
	virtual bool IsCached(const char *pzTag) = 0;
	
	// return true if tag maps to member that is a contained object
	// derived from XMLObject.  pzTag will always be the first parameter
	// you supplied to MapMember().  
	virtual bool IsMemberObject(const char *pzTag) = 0;

	// return true if the value has been modified since it's first set value.
	// If you are not using conditional object serialization you can always
	// return true to include this member in the XML stream.
	virtual bool IsMemberDirty(const char *pzTag) = 0;

	// returns false if the tag does not map to anything 
	// in the object otherwise the value is set.
	virtual bool SetMember(const char *pzTag, const char *pzVal) = 0;

	// returns 0 if the tag does not map to an object, 
	// otherwise a pointer to the object.
	// you should set *bDidCreate = 1 if you want SetAttributeList() to be called
	// and replace any existing attributes.  *bDidCreate = 0 will not call 
	// SetAttributeList(), rather SetMappedAttribute() will be called for every 
	// attribute in the XML.
	virtual XMLObject *GetObject(const char *pzTag, 
								 const char *pzOID,
								 const char *pzUpdateTime,
								 int *bDidCreate) = 0;

	// custom "Get" behavior.  You should append 
	// the value to the supplied ostream.
	virtual bool GetMember( const char *pzTag,
							GString &strAppendValueDestination) = 0;


	// custom object streaming for you to handle serialization of 
	// complex objects. This member will only be called if you 
	// returned true from IsMemberObject().
	virtual void StreamObject(	const char *pzTag, 
								GString& xml, 
								int nDebugBeautifyTabs, 
								bool bForceOutput) = 0;

	// custom attribute management
	virtual bool SetAttribute(const char *pzAttrib, 
									const char *pzVal) = 0;

};

#endif //_USER_MEMBER_HANDLER_BASE_
