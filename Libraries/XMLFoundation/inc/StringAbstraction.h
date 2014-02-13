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
#ifndef XML_STRING_ABSTRACTION
#define XML_STRING_ABSTRACTION

// see AbstractionsMFC.h for a sample implementation using MFC's CString
// see AbstractionRougeWave.h for a sample implementation using RougeWave's RWCString
// see AbstractionGeneric.h for a sample implementation using iostream's

typedef void * userString;

class StringAbstraction
{
public:
	virtual int isEmpty(userString pString) = 0;
	virtual __int64 length(userString pString) = 0;
	virtual void assign(userString pString, const char *pzValue) = 0;
	virtual void append(userString pString, const char *pzValue) = 0;
	virtual const char *data(userString pString) = 0;
};

#endif //XML_LIST_ABSTRACTION

