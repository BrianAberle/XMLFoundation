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

#ifndef _ROOTABLE_FACTORY_OBJECTS

#include "xmlDefines.h"
#include "GArray.h"
#include "GString.h"

class MemberDescriptor;


class XMLObject;
typedef XMLObject* (*ObjectFactory)();

typedef void* (*IFaceFactory)(void *);

#define _ROOTABLE_FACTORY_OBJECTS



class XMLObjectFactoryManager
{
	static int m_UIDCounter;

	struct structIFaceCreate
	{
		IFaceFactory  Iff;				// Factory Fn for the Interface class
		ObjectFactory Of;				// Factory Fn for the Business Object
		GString	  strBusObjClassName;
		GString	  strIFaceObjClassName;
	};

	// array of structures containing pointers to fn's that create new business objects
	// there will be one entry per IMPLEMENT_FACTORY in your application
	GArray m_factoryList; // contains MemberDescriptor's

	// array of structures containing pointers to fn's that create new interface objects
	GArray	m_IFacefactoryList; // contains structIFaceCreate's


	// Here's C++ at it's finest.  The private constructor forbids construction
	// privilege at compile time.  The only exception to this rule is that members
	// of type XMLObjectFactoryManager may call private methods.  The static accessor
	// getFactoryManager() is a member method.  In this method a static instance
	// of XMLObjectFactoryManager is defined.  This alone is powerful, guaranteeing we will
	// never have > 1 instance of a FactoryManager.  (If it was public, an application
	// programmer could instantiate them).  
	
	// Now it's other purpose, even more important, is that this design pattern 
	// eliminates global race conditions. The XMLObjectFactoryManager is basically 
	// a global list of Fn*'s.  Each new object registers itself with a macro
	// that wrote another global object.  Obviously the list must be constructed 
	// before the first entry is added.  If they are both "global" there is no 
	// way to guarantee construction order.  By using the static member in a 
	// static method we guaranteed the construction order.
	
	// Additionally, XMLObjectFactoryManager once used GList to contain these items
	// however GList uses a custom memory management via a global object - the race conditions 
	// apply to the destructor as well.  In this case, only in one build the destructor of this
	// global class needed to iterate a GList - after all of the global GList::Nodes had been
	// released at application exit - while this global object contained a List that was no longer
	// connected by GList::Nodes which still contain a valid pointer that needs to be released
	// in the destructor of this object - now in the GArray we always have access to those valid pointers.
	XMLObjectFactoryManager();
public:
	static XMLObjectFactoryManager &getFactoryManager();

	// called by macro REG_FACTORY to register this object and it's factory
	void Add(	ObjectFactory, 
				const char * szTag, 
				const char * szClassName);


	// called by macro IMP_BOL_RELATION to register this interface
	void Add(	IFaceFactory Iff, 
				ObjectFactory Of, 
				const char *iFaceClassName, 
				const char *BusObjClassName);

	structIFaceCreate *FindIFaceInfo( const char *pzBOLClassName );

	// Check all the objects registered with REG_FACTORY, return an instance of a match
	MemberDescriptor *GetRootableMapEntry(const char * pzTag);
	MemberDescriptor *GetRootableMapEntry(ObjectFactory pFn);
	const char *GetTag(ObjectFactory pFn);
	ObjectFactory GetFactory(const char * pzTag);
	const char *GetSDKClassName(const char * pzTag);

	~XMLObjectFactoryManager();
};




class RegisterInterface
{
public:
	RegisterInterface(	IFaceFactory,
						ObjectFactory, 
						const char * szIFaceClassName, 
						const char * szBusObjClassName);
};


#endif //_ROOTABLE_FACTORY_OBJECTS
