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
#ifndef _JavaVMIntegration_H_
#define _JavaVMIntegration_H_

#include "IntegrationBase.h"
#include "GException.h"


// See Usage Sample in "IntegrationBase.h"

struct JavaVM_;
class JavaInterface : public BaseInterface
{
	GString m_strInvokeReturnValue;
	// One VM for all threads.  Static to keep and store inter-transactional state.
	static JavaVM_ *m_jvm;

public:

	JavaInterface(const char *pzComponentPath, const char *pzComponentSettings);
	~JavaInterface();
	GStringList* GetComponents(); // returns class files 
	GStringList* GetInterfaces(const char *pzObject);
	GStringList* GetMethods(const char *pzObject, const char *pzInterface);
	GStringList* GetMethodParams(const char *pzObject, const char *pzInterface, const char *pzMethodName);
	GStringList* GetMethodParamTypes();
	const char * GetMethodReturnType();
	int IsAvailable(const char *pzObject, const char *pzInterface, const char *pzMethodName);
	const char * Invoke(const char *pzObject, const char *pzInterface, const char *pzMethodName, GStringIterator &varArgs);

	int SetOption(const char *pzOption, void *pzValue){return 0;}
	void *GetOption(const char *pzOption){return 0;}

};



#endif


