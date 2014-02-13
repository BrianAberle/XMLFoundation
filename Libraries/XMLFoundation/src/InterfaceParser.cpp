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
#include "GlobalInclude.h"
#include "InterfaceParser.h"
#include "GString.h"
#include "GStringList.h"
#include "GException.h"

#include <string.h> // for: strstr(), 



void iComponent::LoadTypeLib(const char *pzRawTypeInfo)
{
// Data comes in the following format.
//		"SayHello&&varName&&char *&&&&"
//		"OtherFunction&&Var1&&char *&&Var2&&char *&&&&";

	char *pzData = (char *)pzRawTypeInfo;

	// lists of '&&' seperated items are terminated with '&&&&'
	char *pBeg = pzData;
	char *pEnd = strstr(pzData,"!");
	char chSave = 0;
	if (pEnd)
	{
		chSave = *pEnd;
		*pEnd = 0;
	}

	// Iterate through the lists, parsing into objects
	while (pBeg && *pBeg)
	{
		// Iterate through the items in this list.
		GStringList list("&",pBeg);
		GStringIterator it(&list);
		if(it())
		{
			const char *pzMethod = it++;

			
			char *pzScopeOrClass = 0;
			char *pzScopeColons = strstr((char *)pzMethod,"::");
			if(pzScopeColons)
			{
				*pzScopeColons = 0;
				pzScopeOrClass = (char *)pzMethod;
				pzMethod = pzScopeColons + 2;
			}
			iMethod *iM = AddMethod(pzMethod,"String",pzScopeOrClass);
			if(pzScopeColons)
			{
				*pzScopeColons = ':';
			}
			

			while (it())
			{
				const char *pzVarName = it++;
				const char *pzVarType = 0;
				if(it())
				{
					pzVarType = it++;
				}
				iM->AddParam(pzVarName,pzVarType);
			}
		}

		if(chSave)	*pEnd = chSave;
		
		// move ahead to the next list
		pBeg = pEnd + 4;
		if ( pBeg && *pBeg ) 
			pEnd = strstr(pBeg,"!");
		if (pEnd)
		{
			chSave = *pEnd;
			*pEnd = 0;
		}
		else
		{
			chSave = 0;
		}
	}
}




void iComponent::UnCache()
{
	GListIterator it( &m_lstMethods );
	while (it())
	{
		iMethod *p = (iMethod *)it++;
		delete p;
	}
	m_lstMethods.RemoveAll();
}

GStringList *iComponent::GetScopes()
{
	m_lstScopes.RemoveAll();
	GString strLastScope;

	GListIterator it( GetMethods() );
	while (it())
	{
		iMethod *p = (iMethod *)it++;
		if (strLastScope.CompareNoCase( p->GetScope() ) != 0)
		{
			strLastScope = p->GetScope();
			m_lstScopes.AddLast(p->GetScope());
		}
	}
	return &m_lstScopes;
}


int iComponent::GetRequiredParamCount(const char *pzMethod)
{
	
	GString strFindMethod(pzMethod);
	if (strFindMethod.CompareNoCase( "ExposedMethods" ) == 0)
		return 0;

	GListIterator it( GetMethods() );
	while (it())
	{
		iMethod *p = (iMethod *)it++;
		if (strFindMethod.CompareNoCase( p->GetName() ) == 0)
		{
			return p->GetParams()->Size();
		}
	}
	
	throw GException("CStdCallIntegration", 2,pzMethod);

}

iMethod *iComponent::AddMethod(const char *pzMethodName, const char *pzMethodRetType, const char *pzClassOrScope)
{
	iMethod *pRet = new iMethod(pzMethodName, pzMethodRetType, pzClassOrScope);
	m_lstMethods.AddLast(pRet);
	return pRet;
}

iParam *iComponent::GetMethodParam(const char *pzMethod, int nIndex)
{ 
	int nCount = 0;
	GString strFindMethod(pzMethod);
	GListIterator it( GetMethods() );
	while (it())
	{
		iMethod *p = (iMethod *)it++;
		if (strFindMethod.CompareNoCase( p->GetName() ) == 0)
		{

			GListIterator it2( p->GetParams() );
			while (it2())
			{
				iParam *pP = (iParam *)it2++;
				if (nCount == nIndex)
					return pP;
				nCount++;
			}
		}
	}
	return 0;
}

int iComponent::IsMethodAvailable(const char *pzMethod)
{
	GString strFindMethod(pzMethod);
	GListIterator it( GetMethods() );
	while (it())
	{
		iMethod *p = (iMethod *)it++;
		if (strFindMethod.CompareNoCase( p->GetName() ) == 0)
		{
			return 1;
		}
	}
	return 0;;
}
