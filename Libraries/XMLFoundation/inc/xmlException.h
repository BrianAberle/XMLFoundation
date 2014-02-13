// --------------------------------------------------------------------------
//						United Business Technologies
//			  Copyright (c) 2000 - 2010 All Rights Reserved.
//
// Source in this file is released to the public under the following license:
// --------------------------------------------------------------------------
// This toolkit may be used free of charge for any purpose including corporate
// and academic use.  For profit, and Non-Profit uses are permitted.
//
// This source code and any work derived from this source code must retain 
// this copyright at the top of each source file.
// --------------------------------------------------------------------------
#ifndef _XML_EXCEPT_H__
#define _XML_EXCEPT_H__

#include "xmlObject.h"
#include "GException.h"

// Takes a simple object, GException, and maps XML tags to it.
class xmlException : public GException, public XMLObject
{

public:
	
	xmlException()  {};
	~xmlException() {};
	
	DECLARE_FACTORY(xmlException, Exception)
	virtual void MapXMLTagsToMembers();
	virtual const char *GetDescription();
};

#endif //_XML_EXCEPT_H__
