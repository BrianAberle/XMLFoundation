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
#ifndef _CStdCallIntegration_H_
#define _CStdCallIntegration_H_

#ifdef _WIN32
	#define MSFT_EXPORT __declspec(dllexport)
#else
	#define MSFT_EXPORT
#endif


#include "IntegrationBase.h"

class iParam
{
	GString m_strName;
	GString m_strType;
public:
	iParam(const char *pzName, const char *pzType){m_strName = pzName; m_strType = pzType;}
	const char *GetName(){return m_strName;}
	const char *GetType(){return m_strType;}
};

class iMethod
{
	GString m_strName;
	GString m_strRetType;
	GList m_lstParameters;// iParams's
public:
	iMethod(const char *pzMethodName,const char *pzMethodRetType)
		{m_strName = pzMethodName;m_strRetType = pzMethodRetType;};
	iParam *AddParam(const char *pzName, const char * pzType)
		{	iParam *pRet = new iParam(pzName,pzType); 
			m_lstParameters.AddLast(pRet);
			return pRet;
		}
	const char *GetName() {return m_strName;}
	const char *GetReturnType() {return m_strRetType;}
	GList *GetParams() {return &m_lstParameters;}
};


class iComponent
{
	GList m_lstMethods;// iMethods's
public:
	void UnCache();
	int IsMethodAvailable(const char *pzMethod);
	iMethod *AddMethod(const char *pzMethodName, const char *pzMethodRetType);
	iComponent(){}
	void LoadTypeLib(const char *pzDynamicLib);
	GList *GetMethods(){ return &m_lstMethods; }
	int GetRequiredParamCount(const char *pzMethod);
};

class CStdCallInterface : public BaseInterface
{
	iComponent m_iC;
	void InitTypeInfo(const char *pzDynamicLib);
	GString m_strLastLoadedTypeLib;

	const char *GetSig(const char *pzObject,const char *pzInterface,const char *pzMethodName);
	void InnerInvoke(void *pfn, const char *pzMethodName, const char *pzModuleName, 
					GStringIterator &varArgs, int nRequiredParamCount);
	
	GString m_strPathQualifiedDynamicLib;
	const char *FullPathDynamicLib(const char *pzDynamicLib);

public:
	GString m_Delimit;
	GString m_DataMap;
	char *m_PIBuffer;
	unsigned char *m_WKBuffer;
	unsigned char *m_WKBuffer2;
	int m_nWKBufferSize;
	void *m_CBArg;
	GString *m_pRsltStr; // destination GString Object
	CBfn m_CB;
	int m_bUnload;
	void *m_dllHandle;

public:
	CStdCallInterface(const char *pzComponentPath, const char *pzComponentSettings);
	~CStdCallInterface();
	GStringList* GetComponents(); // returns class files 
	GStringList* GetInterfaces(const char *pzObject);
	GStringList* GetMethods(const char *pzObject, const char *pzInterface);
	GStringList* GetMethodParams(const char *pzObject, const char *pzInterface, const char *pzMethodName);
	const char * Invoke(const char *pzObject, const char *pzInterface, const char *pzMethodName, GStringIterator &varArgs);
	int IsAvailable(const char *pzObject, const char *pzInterface, const char *pzMethodName);
	int SetOption(const char *pzOption, void *pzValue);
	void *GetOption(const char *pzOption);

	// return detailed information about the last method described by GetMethodParams()
	GStringList* GetMethodParamTypes();
	const char * GetMethodReturnType();
};



#endif


