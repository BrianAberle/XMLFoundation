#ifndef _TXMLIntegration_H_
#define _TXMLIntegration_H_

#include "BaseInterface.h"
#include "InterfaceParser.h"
#include <Python.h>

// See Usage Sample in "IntegrationBase.h"
class PythonInterface : public BaseInterface
{
	iComponent m_iC;
	void InitTypeInfo(const char *pzDynamicLib);
	GString m_strLastLoadedTypeLib;
	GString m_strInvokeReturnValue;
	PyObject *GetPythonArgumentObject(const char *pzPythonScriptFile, const char *pzMethodName, GStringIterator &varArgs);

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
	PythonInterface(const char *pzComponentPath,const char *pzComponentSettings);
	~PythonInterface();
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


