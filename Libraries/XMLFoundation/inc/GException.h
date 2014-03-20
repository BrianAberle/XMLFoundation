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

#ifndef __GEXCEPTION_H__
#define __GEXCEPTION_H__

#include "GString.h"
#include "GStringList.h"
//#include "GProfile.h"

class GException : public GString
{

protected:

	// 4-byte sub-system, 4-byte error code
	long		_cause;
	long		_error;
	short		_subSystem;

	GString		_ret;  // used as an internal temp buffer
	GStringList	_stk;  // OS/Core supplied callstack if available
	
	// user added error detail.  For example if 'this' = "Could not open file"
	// _ErrorDetail may contain "Loading user settings", and "Server Initialization"
	// to pinpoint where in a system "Could not open file" happened.
	GStringList	_ErrorDetail;  

	GException();

public:

	// copy constructor
	GException(const GException&);
	GException(const char* pzSystem, int error, ...);
	GException(int error, const char *pzErrorText);
	GException(int nError, int nSubSystem, const char *pzText, GStringList *pStack);

	// add location or detail to 'this' in a catch, prior to a rethrow 
	void AddErrorDetail(const char *pzDetail){_ErrorDetail.AddLast(pzDetail);}
	void AddErrorDetail(const char* szSystem, int error, ...);
	void AddErrorDetail(int error, const char *pzErrorText);

	virtual ~GException();
	virtual const char *GetDescription();
	virtual const GStringList *GetStack();
	virtual const char *GetStackAsString();
	long  GetCause();
	long  GetError()  { return _error; }
	void  SetError(long error) { _error = error; }
	short GetSystem() { return _subSystem; }
	const char *ToXML(const char *pzExceptionParent = "TransactResultSet");
};

void SetErrorDescriptions(const char *pzErrData);
class GlobalErrorLanguage
{
public:
	GlobalErrorLanguage();
	~GlobalErrorLanguage();
};


// Not available on Windows CE or Windows Phone
#ifndef WINCE
 #ifndef __WINPHONE
  #if  defined(_DEBUG)&&  defined(_WIN32) // This is only for Windows desktop

    #include <windows.h>
    
	#pragma pack( push, before_imagehlp, 8 )
    #include <imagehlp.h>
    #pragma pack( pop, before_imagehlp )


	class GCallStack
	{
	protected:
		GString _strStk;
		GStringList _stk;
	public:
		GCallStack(const GCallStack &);
		GCallStack(HANDLE hThread, CONTEXT& c);
		~GCallStack();

		const char *GetStackAsString();
		const GStringList *GetStack();
	};
  #endif
 #endif // __WINPHONE
#endif // WINCE



#endif //  __GEXCEPTION_H__
