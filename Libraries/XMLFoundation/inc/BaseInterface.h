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

#ifndef _BASE__IFACE_H__
#define _BASE__IFACE_H__


#include "GString.h"
#include "GStringList.h"

typedef void(*CBfn)(const char *, void *);
struct CBDataStruct
{
	int nState;			    // currently unused - may track transactional state
	int nReserved;			// currently unused - the size of this struct cannot change or it will break lots of code so maybe this will be needed someday
	void *pstrDest;		    // a GString * - may be used directly if you link with the same foundation the invoking process is built with.
	CBfn fn;				// 2nd level handler function on the calling process, 1st level is in the driver.
	char *piBuf;			// set by the invoking process, that the called method may indicate processing instructions on the results, assume 1024 bytes
	char *wkBuf;			// set by the invoking process, a region of memory as a working buffer for the called method, size is set by invoker, nWKBufferSize bytes(normally 64000).
	char *wkBuf2;			// set by the invoking process, a region of memory as a working buffer for the called method, size is set by invoker, nWKBufferSize bytes(normally 64000).
	void *pCBArg;		    // a void * user defined by the SetDriverOption("callbackArg",??);
	void *pDriverInterface; // pointer to instance of driver interface (for example, a CStdCallInterface)
	int nWKBufferSize;		// size of memory allocated in "wkBuf"

	union UArg
	{
		__int64 Int64;
		__int64 *PInt64;
		int Int;
		int *PInt;
		const char *CChar;
		char *Char;
		void *Void;
	};
	UArg arg[7];
};


class BaseInterface
{
protected:

	GStringList m_pObjectList;
	GStringList m_pInterfaceList;
	GStringList m_pMethodList;
	
	// m_pParamTypeList[0] is the type for the parameter at m_pParamNameList[0]
	GStringList m_pParamNameList;
	GStringList m_pParamTypeList;
	GString m_strReturnType;

	// Set at Object Construction
	GString m_strComponentPath;
	GString m_strComponentSettings;

public:
	// used to build a combined results set of GetMethodParams(), & GetMethodParamTypes()
	GString m_strMethodParamsSuperSet;
public:
	BaseInterface(const char *pzComponentPath,const char *pzComponentSettings)
	{
		m_strComponentPath = pzComponentPath;
		m_strComponentSettings = pzComponentSettings;
	}
	virtual ~BaseInterface(){}
	virtual GStringList* GetComponents() = 0;
	virtual GStringList* GetInterfaces(const char *pzObject) = 0;
	virtual GStringList* GetMethods(const char *pzObject, const char *pzInterface) = 0;
	virtual GStringList* GetMethodParams(const char *pzObject, const char *pzInterface, const char *pzMethodName) = 0;

	// call the method arguments in a string list iterator
	virtual const char * Invoke(const char *pzObject, const char *pzInterface, const char *pzMethodName, GStringIterator &varArgs) = 0;

	virtual int IsAvailable(const char *pzObject, const char *pzInterface, const char *pzMethodName) = 0;
	virtual int SetOption(const char *pzOption, void *pzValue) = 0;
	virtual void *GetOption(const char *pzOption) = 0;

	// return detailed information about the last method described by GetMethodParams()
	virtual GStringList* GetMethodParamTypes() = 0;
	virtual const char * GetMethodReturnType() = 0;

	const char *GetComponentPath(){ return m_strComponentPath; };
	const char *GetComponentSettings(){ return m_strComponentSettings; };
};

#endif //_BASE__IFACE_H__
