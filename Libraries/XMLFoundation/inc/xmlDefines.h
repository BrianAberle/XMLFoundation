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

#ifndef __APP_DEFS_H__
#define __APP_DEFS_H__

#define  MAXPATHNAME		256

// readable typedef's for function pointers
class XMLObject;
typedef XMLObject* (*ObjectFactory)();



// On Windows, a Critical Section is faster than a Mutex but 
// Critical Sections dont exist in Unix or PThreads so this 
// mapping macro was inspired to achieve the best performance
// WindowsPhone does not have a CRITICAL_SECTION
#ifdef _WIN32
	#ifdef __WINPHONE
		#define XML_MUTEX				gthread_mutex_t
		#define XML_INIT_MUTEX(m)		gthread_mutex_init(m,0);
		#define XML_DESTROY_MUTEX(m)	gthread_mutex_destroy(m);
		#define XML_LOCK_MUTEX(m)		gthread_mutex_lock(m);
		#define XML_UNLOCK_MUTEX(m)		gthread_mutex_unlock(m);
	#else
		#define XML_MUTEX				CRITICAL_SECTION
		#define XML_INIT_MUTEX(m)		InitializeCriticalSection(m);
		#define XML_DESTROY_MUTEX(m)	DeleteCriticalSection(m);
		#define XML_LOCK_MUTEX(m)		EnterCriticalSection(m);
		#define XML_UNLOCK_MUTEX(m)		LeaveCriticalSection(m);
	#endif
#else
	#define XML_MUTEX				pthread_mutex_t
	#define XML_INIT_MUTEX(m)		pthread_mutex_init(m,0);
	#define XML_DESTROY_MUTEX(m)	pthread_mutex_destroy(m);
	#define XML_LOCK_MUTEX(m)		pthread_mutex_lock(m);
	#define XML_UNLOCK_MUTEX(m)		pthread_mutex_unlock(m);
#endif


//	data state flags describe the state of object members

// The Object memory state does not sync with the original value set by the Object Factory
#define DATA_DIRTY		0x01 
// The member has been set by either the Object or the Object Factory, the value is valid.
#define DATA_NOT_NULL	0x02
// The member has been assigned a value from the Object Factory, implies DATA_NOT_NULL
#define DATA_CACHED		0x04
// The member has never been assigned a value, it should be considered uninitialized
#define DATA_NULL		0x08 
// The member should be included in the xml serialization stream
#define DATA_SERIALIZE	0x10 


// object behavior flags for XMLObject::ModifyObjectBehavior()
#define MULTI_TYPE_LIST_SUPPORT			0x01 // Set if you own a list that stores > 1 object type
#define OBJECT_UPDATE_NOTIFY			0x02 // sends "ObjectUpdate" to XMLObject::ObjectMessage()
#define MEMBER_UPDATE_NOTIFY			0x04 // sends "MemberUpdate" to XMLObject::ObjectMessage()
#define EARLY_CONTAINER_NOTIFY			0x08 // advanced alternate(non-xml) source that wants ref counting support
#define PREVENT_AUTO_CACHE				0x10 // prevents this object type from being cached and updated by XML
#define HAS_EXECUTED_MEMBER_MAP			0x20 // for internal use only
#define PREVENT_AUTO_DESTRUCTION		0x40 // prevent auto destruction on refcount of 0
#define IS_TEMP_OBJECT					0x80 // for internal use only - 'this' is a temporary/partial copy of a cached object
#define SUBOBJECT_UPDATE_NOTIFY			0x100// when a contained sub object has been updated
#define PREVENT_ATTRIB_AUTOMAP			0x200// prevent auto-map of attributes, if the xml contains an unmapped attribute it will be dropped.
#define NO_MEMBER_MAP_PRECOUNT			0x400// do not consolidate internal memory allocations for new MemberDescriptor's
#define USE_STATE_CACHE					0x800// when an object is referenced by oid as the first attribute - restore the full object state if it is cached.
// If you DO NOT USE XMLObject:: setObjectDataHandler(),GetObjectDataStorage() GetObjectDataValue(), SetObjectDataValue()
// and support for those is not needed, adding this flag will improve xml rendering performance on huge datasets.
#define NO_OBJECT_DATA					0x1000 // NO_OBJECT_DATA = do not accept Object Data assigned by the XMLtokenizer into 'this', if you do not use GetObjectDataValue() - this is FASTER


// These are translation flags used in the nTranslationFlags argument to MapMember()
#define XLATE_CASE_INSENSITIVE_IN	0x01
#define XLATE_CASE_INSENSITIVE_OUT  0x02
#define XLATE_NO_IN_WILDCARDS		0x04  


// ObjectMessage types
#define MSG_XML_ASSIGN				1   // xml factory detected an XML update of a member that is dirty
#define MSG_NON_NUMERIC				2   // non numeric data is mapped to an integer data type
#define MSG_EMPTY_STRING			3	// empty strings may specifically mean something different than a non existing element in the XML - this allows you to distinguish the difference
#define MSG_MEMBER_UPDATE			4	
#define MSG_SUBOBJECT_UPDATE		5
#define MSG_XML_UPDATE				6
#define MSG_DATA_TRUNCATE			7	// XML data does not fit into the storage space it is mapped to
#define MSG_UNMAPPED_ATTRIBUTE		8	// sent when PREVENT_ATTRIB_AUTOMAP is set
#define MSG_UNEXPECTED_VALUE		9	// value is not in the set of expected values



#define FACTORY_FLAG_OBJ_FROM_FACTORY			0x01 // else it's from the cache
#define FACTORY_FLAG_FIRST_REFERENCE			0x02 // first ref of map
#define FACTORY_FLAG_UPDATE_CLEAN_MEMBERS		0x04 
#define FACTORY_FLAG_SET_DIRTY					0x08


///////////////////////////////////////////////////////////////////////
//	serialization flags for ToXML()
//////////////////////////////////////////////////////////////////////
// Otherwise XML tags appear in the order they were mapped
#define ORDER_MEMBERS_ALPHABETICALLY	0x01  
// deeply recurse without including dirty members
#define RECURSE_OBJECTS_DEEP			0x02  
// includes any member with a state of DATA_CACHED
#define INCLUDE_ALL_CACHED_MEMBERS		0x04  
// normally empty strings serialize as <string/> rather than <string><string>
#define EXCLUDE_SHORT_TERMINATION		0x08
// Do not serialize any data from MapAttribute() members
#define EXCLUDE_MAPPED_ATTRIBUTES		0x10  
// Do not serialize attributes that came in via XML but were unmapped with MapAttribute()
#define EXCLUDE_UNMAPPED_ATTRIBUTES		0x20  
// Adds the DOCTYPE to the beginning of the XML
#define INCLUDE_DOCTYPE_DECLARATION		0x40  
// includes any member regardless of it's state
#define FULL_SERIALIZE					0x80  
// include OID's only
#define USE_OBJECT_MARKERS				0x100 
// more compact, faster, less human readable, The XML will have no Tabs, Carriage returns or Linefeeds
#define NO_WHITESPACE					0x200 
// do not include empty strings in XML - mapped to GString or CString
#define NO_EMPTY_STRINGS				0x400 

// do not include <!CDATA> -OR- the ObjectDataHandler data.
// If you DO NOT USE XMLObject:: setObjectDataHandler(),GetObjectDataStorage() GetObjectDataValue(), SetObjectDataValue()
// and support for those is not needed, adding this flag will improve xml rendering performance on huge datasets.
#define EXCLUDE_OBJECT_VALUE			0x800 

// do not include empty strings in XML - mapped to GStringList or CStringList
#define NO_EMPTY_STRINGS_IN_STRINGLIST  0x1000

// Windows = "\r\n" = OD OA = ascii(13) + ascii(10)
// UNIX = "\n" = OA = ascii(10)
// if not specified - the default will match the operating system generating the XML
#define WINDOWS_LINEFEEDS				0x2000 
#define UNIX_LINEFEEDS					0x4000 

// Do not serialize any member that has not been explicitly assigned a value by the ObjectFactory 
// or by the application developer via a call to XMLObject::SetMember() or XMLObject::setMemberDirty()
#define DIRTY_AND_CACHED_MEMBERS_ONLY	0x8000  

// used in GString::AppendEscapeXMLReserved()
// if CONTROL_CHAR_ESCAPE is NOT enabled, serialization assumes codepage 1252 - Latin ASCII
#define CONTROL_CHAR_ESCAPE				0x10000
#define NO_0_31_CHAR_ESCAPE				0x20000
#define INTERNAL_ATTRIBUTE_ESCAPE		0x40000




//
// Platform independant data types, when datasize matters
//
#ifdef _WIN32
	typedef unsigned char       BYTE;  // (always  8 bits)
	typedef unsigned short      WORD;  // (always 16 bits)
	typedef unsigned long       DWORD; // (always 32 bits)
	#define _CRT_SECURE_NO_WARNINGS
	#define _SCL_SECURE_NO_WARNINGS 
#else
	typedef unsigned char       BYTE;  // (always  8 bits)
	typedef unsigned short      WORD;  // (always 16 bits)
// DB2's <sqlcli.h> defines DWORD as sqlint32
//	typedef unsigned long       DWORD; // (always 32 bits)
#endif


// MSVC & gnu define bool, CSet++ does not
#ifdef _AIX
#ifndef true
	// bool should be 8 bits, but this conflicts with RogueWave
	#define	__int64 long long
	typedef int bool; 
	#define true 1
	#define false 0
#endif
#endif

#ifdef _IOS
	#include <inttypes.h>
	#define	__int64 long long
#endif

// support for the GNU compiler
#if defined _LINUX || defined __sun || defined _HPUX
	#define	__cdecl
	#define	__int64 long long
#endif


#ifdef _WIN32
	#define	XMLF_LONG_ZERO	0i64
	#define	XMLF_LONG_ONE	1i64
#else
	#define	XMLF_LONG_ZERO	0LL
	#define	XMLF_LONG_ONE	1LL
#endif



#ifndef NOMINMAX

#ifndef ___max
#define ___max(a,b)            (((a) > (b)) ? (a) : (b))
#endif

#ifndef ___min
#define ___min(a,b)            (((a) < (b)) ? (a) : (b))
#endif

#endif  /* NOMINMAX */



#endif //__APP_DEFS_H__
