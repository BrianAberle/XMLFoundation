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
#if !defined(AFX_PASSWORDHELPER_H__012B86DC_B981_11D3_A6EA_00A0CC542CEF__INCLUDED_)
#define AFX_PASSWORDHELPER_H__012B86DC_B981_11D3_A6EA_00A0CC542CEF__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class PasswordHelper  
{
public:
	static void Hash(const char* pszSalt, const char* pszPass, unsigned char digest[16]); 

};

#endif // !defined(AFX_PASSWORDHELPER_H__012B86DC_B981_11D3_A6EA_00A0CC542CEF__INCLUDED_)
