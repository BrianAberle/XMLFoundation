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

#ifndef _TXMLIntegration_H_
#define _TXMLIntegration_H_

#include "BaseInterface.h"
#include "InterfaceParser.h"

// See Usage Sample in "IntegrationBase.h"
class PerlInterface : public BaseInterface
{
	struct interpreter *my_perl;
	GString m_strInvokeReturnValue;
	iComponent m_iC;
	void InitTypeInfo(const char *pzDynamicLib);
	GString m_strLastLoadedTypeLib;

	// values set by ServerCore 
	char *m_PIBuffer;
	unsigned char *m_WKBuffer;
	unsigned char *m_WKBuffer2;
	int m_nWKBufferSize;
	void *m_CBArg;
	GString *m_pRsltStr; // destination GString Object
	CBfn m_CB;
	int m_bUnload;
	

public:
	PerlInterface(const char *pzComponentPath,const char *pzComponentSettings);
	~PerlInterface();
	GStringList* GetComponents(); // returns class files 
	GStringList* GetInterfaces(const char *pzObject);
	GStringList* GetMethods(const char *pzObject, const char *pzInterface);
	GStringList* GetMethodParams(const char *pzObject, const char *pzInterface, const char *pzMethodName);
	int IsAvailable(const char *pzXmlScriptFile, const char *Script, const char *pzMethodName);
	const char * Invoke(const char *pzObject, const char *pzInterface, const char *pzMethodName, GStringIterator &varArgs);

	// return detailed information about the last method described by GetMethodParams()
	GStringList* GetMethodParamTypes();
	const char * GetMethodReturnType();

	int SetOption(const char *pzOption, void *pzValue);
	void *GetOption(const char *pzOption);

};




#endif


