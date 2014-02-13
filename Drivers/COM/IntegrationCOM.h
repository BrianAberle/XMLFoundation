#ifndef _INTEGRATION_COM_H__
#define _INTEGRATION_COM_H__

// See Usage Sample in "IntegrationBase.h"

#include "IntegrationBase.h"
class COMInterface : public BaseInterface
{
	GString m_strInvokeReturnValue;
public:
	COMInterface(const char *pzComponentPath,const char *pzComponentSettings);
	~COMInterface();
	GStringList* GetComponents();
	GStringList* GetInterfaces(const char *pzObject);
	GStringList* GetMethods(const char *pzObject, const char *pzInterface);
	GStringList* GetMethodParams(const char *pzObject, const char *pzInterface, const char *pzMethodName);
	int IsAvailable(const char *pzObject, const char *pzInterface, const char *pzMethodName);
	const char * Invoke(const char *pzObject, const char *pzInterface, const char *pzMethodName, GStringIterator &varArgs);
	
	// return detailed information about the last method described by GetMethodParams()
	GStringList* GetMethodParamTypes();
	const char * GetMethodReturnType();

	int SetOption(const char *pzOption, void *pzValue){return 0;}
	void *GetOption(const char *pzOption){return 0;}

};

#endif