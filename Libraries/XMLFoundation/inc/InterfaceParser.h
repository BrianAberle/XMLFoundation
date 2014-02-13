// The following are utility classes (iParam,iMethod, && iComponent)
// used for component technologies that do not support native
// interface query capabilities (like Perl and C shared libs)
#ifndef _INTERFACE_PARSER_
#define _INTERFACE_PARSER_

#include "GString.h"
#include "GStringList.h"


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
	GString m_strScope;
	GString m_strName;
	GString m_strRetType;
	GList m_lstParameters;// iParams's
public:
	iMethod(const char *pzMethodName,const char *pzMethodRetType, const char *pzClassOrScope)
		{m_strName = pzMethodName;m_strRetType = pzMethodRetType;if(pzClassOrScope) m_strScope = pzClassOrScope;};
	iParam *AddParam(const char *pzName, const char * pzType)
		{	iParam *pRet = new iParam(pzName,pzType); 
			m_lstParameters.AddLast(pRet);
			return pRet;
		}
	const char *GetScope() {return m_strScope;}
	const char *GetName() {return m_strName;}
	const char *GetReturnType() {return m_strRetType;}
	GList *GetParams() {return &m_lstParameters;}
};

class iComponent 
{
	GList m_lstMethods;// iMethods's
	GStringList m_lstScopes;// GStrings
public:
	void UnCache();
	int IsMethodAvailable(const char *pzMethod);
	iMethod *AddMethod(const char *pzMethodName, const char *pzMethodRetType, const char *pzClassOrScope);
	iComponent(){}
	void LoadTypeLib(const char *pzDynamicLib);
	GStringList *GetScopes();
	GList *GetMethods(){ return &m_lstMethods; }
	iParam *GetMethodParam(const char *pzMethod, int nIndex);
	int GetRequiredParamCount(const char *pzMethod);
};

#endif //_INTERFACE_PARSER_
