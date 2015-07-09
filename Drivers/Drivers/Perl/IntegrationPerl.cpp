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
#include "IntegrationPerl.h"

// Gerneral utilities
#include "xmlException.h"
#include "GDirectory.h"
#include "FrameworkAuditLog.h"

// Includes from the Perl distribution CORE directory
// You must also link with Perl.lib, or libPerl.so from the same CORE directory
#include <EXTERN.h> 
#include <perl.h>



PerlInterface::PerlInterface(const char *pzComponentPath,const char *pzComponentSettings)
	:	BaseInterface(pzComponentPath, pzComponentSettings)
{
	my_perl = perl_alloc();
	perl_construct( my_perl );
}

PerlInterface::~PerlInterface()
{
	perl_destruct(my_perl);
	perl_free(my_perl);
	my_perl = 0;
}

// returns list of files from the [Perl] PATH= value in txml.txt
GStringList* PerlInterface::GetComponents()
{
	m_pObjectList.RemoveAll();
	GDirectory dir( m_strComponentPath );
	GStringIterator it(&dir);
	while (it())
		m_pObjectList.AddLast( it++ );
	return &m_pObjectList;
}

GStringList *PerlInterface::GetInterfaces(const char *pzPerlScriptFile)
{
	// Perl doesn't have "Interfaces", but it does have "Packages"
	// A future version will have more native support for packages,
	// but for now, each .pl file is a "Component" and the file name
	// without the extension is the one and only "Interface" to that component.
	m_pInterfaceList.RemoveAll();
	GString GStr;
	GStr = pzPerlScriptFile;

	// assumes pzPerlScriptFile is a *.pl file
	if ( (GStr.Right(3)).CompareNoCase(".pl") == 0)
	{
		// strip the ".pl" and add it as the only interface
		m_pInterfaceList.AddLast((const char *)(GStr.Left(GStr.Length() - 3)));
	}
	else
	{
		// if the pzPerlScriptFile is a some kind of perl object other than *.pl
		m_pInterfaceList.AddLast("Unsupported Object Type!");
	}
	return &m_pInterfaceList;
}


// calls the "ExposedMethods" routine in pzPerlScriptFile to determine results
GStringList *PerlInterface::GetMethods(const char *pzPerlScriptFile, const char *Script)
{
	m_pMethodList.RemoveAll();
	InitTypeInfo(pzPerlScriptFile);
	GListIterator it( m_iC.GetMethods() );
	while (it())
	{
		iMethod *p = (iMethod *)it++;
		m_pMethodList.AddLast( p->GetName() );
	}
	
	return &m_pMethodList;
}

// calls the "ExposedMethods" routine in pzPerlScriptFile to determine results
GStringList *PerlInterface::GetMethodParams(const char *pzPerlScriptFile, const char *Script, const char *pzMethodName)
{
	m_pParamNameList.RemoveAll();
	m_pParamTypeList.RemoveAll();
	InitTypeInfo(pzPerlScriptFile);

	GString strFindMethod(pzMethodName);
	GListIterator it( m_iC.GetMethods() );
	while (it())
	{
		iMethod *p = (iMethod *)it++;
		if (strFindMethod.CompareNoCase( p->GetName() ) == 0)
		{
			GListIterator itParams( p->GetParams() );
			while (itParams())
			{
				iParam *iP = (iParam *)itParams++;
				m_pParamNameList.AddLast(iP->GetName());
				m_pParamTypeList.AddLast(iP->GetType());
			}
		}
	}
	return &m_pParamNameList;
}

// returns extended detail for the last call to GetMethodParams()
GStringList* PerlInterface::GetMethodParamTypes()
{
	return &m_pParamTypeList;
}

// returns extended detail for the last call to GetMethodParams()
const char * PerlInterface::GetMethodReturnType()
{
	m_strReturnType = "String";
	return m_strReturnType;
}

// calls the "ExposedMethods" routine in pzPerlScriptFile to determine results
int PerlInterface::IsAvailable(const char *pzPerlScriptFile, const char *Script, const char *pzMethodName)
{
	InitTypeInfo(pzPerlScriptFile);
	return m_iC.IsMethodAvailable(pzMethodName);
}


const char * PerlInterface::Invoke(const char *pzPerlScriptFile, const char *Script, const char *pzMethodName, GStringIterator &varArgs)
{
	if (Script)
		InitTypeInfo(pzPerlScriptFile);

	try
	{
		// path qualify the pzPerlScriptFile
		char pzObjectAndPath[512];
		sprintf(pzObjectAndPath,"%s%s",(const char *)m_strComponentPath,pzPerlScriptFile);


		char *pzPerlFileArg[] = { "", pzObjectAndPath };
		
		// parse the Perl Script
 		perl_parse(my_perl, 0, 2, pzPerlFileArg, (char **)NULL);

		// Perl's stack pointer 
		SV **sp = PL_stack_sp;
		push_scope();
		
		// Begin Stack Marker for routine arguments 
		*PL_markstack_ptr = (sp) - PL_stack_base;

		// Supported Native Perl datatypes
		GString strS("string");
		GString strI("integer");

		int nRequiredParamCount = m_iC.GetRequiredParamCount(pzMethodName);
		int nPassedParams = 0;
		while (varArgs())
		{
			const char *pzArgValue = varArgs++;
			iParam *pI = m_iC.GetMethodParam(pzMethodName, nPassedParams++);
			if(  strS.CompareNoCase(pI->GetType()) == 0  )
			{
				// create a new "String" param and push it on the stack
				XPUSHs(sv_2mortal(newSVpv((char *)pzArgValue, strlen(pzArgValue))));
			}
			else if(  strI.CompareNoCase(pI->GetType()) == 0  )
			{
				// create a new "Integer" param and push it on the stack
				XPUSHs(sv_2mortal(newSViv(atoi(pzArgValue))));
			}
			else
			{
				throw GException("PerlIntegration",2,pI->GetType(),pzMethodName,pzPerlScriptFile);
			}
		}
		if (nPassedParams != nRequiredParamCount)
		{
			throw GException("PerlIntegration", 1,pzMethodName,pzPerlScriptFile,nRequiredParamCount,nPassedParams);
		}


		// position the stack pointer to be popped by perl
		PL_stack_sp = sp;

		// call the Perl Routine



		int nCount = perl_call_pv((char *)pzMethodName, 0);
		
		// position the sp at the first return value
		sp = PL_stack_sp;
		
		// If there is anything to pop off the return stack
		if (nCount)
		{
			m_strInvokeReturnValue = POPp;
		}
		else
		{
			throw GException("PerlIntegration", 3,pzMethodName,pzPerlScriptFile);
		}
		
		// cleanup
		PL_stack_sp = sp;
		pop_scope();
	}	
	catch(...)
	{
		throw GException("PerlIntegration", 4,pzMethodName,pzPerlScriptFile);
	}
	return m_strInvokeReturnValue;
}


void PerlInterface::InitTypeInfo(const char *pzPerlScriptFile)
{
	// only load the interface description one time
	if (m_strLastLoadedTypeLib.CompareNoCase(pzPerlScriptFile) == 0)
		return;
	m_iC.UnCache();
	m_strLastLoadedTypeLib = pzPerlScriptFile;

	// prepare an empty argument list
	GStringList emptyList;
	GStringIterator varArgs(new GStringList);

	const char *pzRet = Invoke(pzPerlScriptFile, 0, "ExposedMethods", varArgs);
	
//  Target Interface description format:
//  -------------------------------------
//	"Routine&varName&char *!"
//	"OtherRoutine&Var1&char *&Var2&char *!";
	int nLen = strlen(pzRet);
	char *pzTarget = new char[nLen];
	int nTIndex = 0;
	
	// copy data into pzTarget with markup chars removed
	for(int i = 0; i < nLen; i++)
	{
		// strip tabs and newlines
		if (pzRet[i] != 0x0A && pzRet[i] != 0x09)
		{
			pzTarget[nTIndex++] = pzRet[i];
		}
	}
	pzTarget[nTIndex++] = 0;

	
	m_iC.LoadTypeLib(pzTarget);
	delete pzTarget;
}


int PerlInterface::SetOption(const char *pzOption, void *pzValue)
{
	GString strOption(pzOption);
	if (strOption.CompareNoCase("callback") == 0)
	{
		m_CB = (CBfn)pzValue;
		return 1;
	}
	if (strOption.CompareNoCase("callbackArg") == 0)
	{
		m_CBArg = pzValue;
		return 1;
	}
	if (strOption.CompareNoCase("piBuf") == 0)
	{
		m_PIBuffer = (char *)pzValue;
		return 1;
	}
	if (strOption.CompareNoCase("wkBuf") == 0)
	{
		m_WKBuffer = (unsigned char *)pzValue;
		return 1;
	}
	if (strOption.CompareNoCase("wkBuf2") == 0)
	{
		m_WKBuffer2 = (unsigned char *)pzValue;
		return 1;
	}
	if (strOption.CompareNoCase("wkBufSize") == 0)
	{
		m_nWKBufferSize = (int)pzValue;
	}

	return 0;
}


void *PerlInterface::GetOption(const char *pzOption)
{

	return 0;
}
