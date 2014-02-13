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

#include "IntegrationPython.h"

// Gerneral utilities
#include "xmlException.h"
#include "GDirectory.h"
#include "FrameworkAuditLog.h"
#include <Python.h>


static PyObject *MakePathObject(char *path)
{
	PyObject *w;
	
	int n = 1;
	PyObject *v = PyList_New(1);
	if (v == NULL)
		throw GException("PythonIntegration", 1);

	w = PyBytes_FromStringAndSize(path, strlen(path));
//	w = PyString_FromStringAndSize(path, strlen(path));
	if (w == NULL) {
		Py_DECREF(v);
		throw GException("PythonIntegration", 2, path, strlen(path));
	}
	PyList_SetItem(v, 0, w);

	return v;
}




PythonInterface::PythonInterface(const char *pzComponentPath,const char *pzComponentSettings)
	:	BaseInterface(pzComponentPath, pzComponentSettings)
{
/* Pythonrun.c Line 77 Says:
   Global initializations.  Can be undone by Py_Finalize().  Don't
   call this twice without an intervening Py_Finalize() call.  When
   initializations fail, a fatal error is issued and the function does
   not return.  On return, the first thread and interpreter state have
   been created.

   Locking: you must hold the interpreter lock while calling this.
   (If the lock has not yet been initialized, that's equivalent to
   having the lock, but you cannot use multiple threads.)
*/
	//don't load site.py
	Py_NoSiteFlag = 1;

	Py_SetProgramName("TXML"); // build in Release

	// It appears we can only have one interpreter instance, but I'm 
	// not sure.  DriverOperation() controls the instance management
	// for this PythonInterface object.
	Py_Initialize();


	// Set the path that contains python scripts
	PyObject *vvv = MakePathObject((char *)pzComponentPath);
	if (PySys_SetObject("path", vvv) != 0)
		throw GException("PythonIntegration", 3, (char *)pzComponentPath );
}

PythonInterface::~PythonInterface()
{
	// Appears to reset Global Interpreter variables
	Py_Finalize();
}

// returns list of files from the [Python] PATH= value in txml.txt
GStringList* PythonInterface::GetComponents()
{
	m_pObjectList.RemoveAll();
	GDirectory dir( m_strComponentPath );
	GStringIterator it(&dir);
	while (it())
	{
		char *pzComponent = (char*)it++; // cast off the const
		
		// lop off the extension
		char *pDot = strpbrk(pzComponent,".");
		if (pDot)
		{
			// make sure it's a .py
			if((pDot[1] == 'p' || pDot[1] == 'P') && 
			   (pDot[2] == 'y' || pDot[2] == 'Y') &&
			   (!pDot[3]) )
			{
				*pDot = 0;
				m_pObjectList.AddLast( pzComponent );
			}
			// otherwise ignore it
		}
		
		// un-write to const memory
		if(*pDot == 0)
		{
			*pDot = '.';
		}
	}
	return &m_pObjectList;
}

GStringList *PythonInterface::GetInterfaces(const char *pzPythonScriptFile)
{
	InitTypeInfo(pzPythonScriptFile);
	return m_iC.GetScopes();
}


// calls the "ExposedMethods" routine in pzPythonScriptFile to determine results
GStringList *PythonInterface::GetMethods(const char *pzPythonScriptFile, const char *pzClassScope)
{
	m_pMethodList.RemoveAll();
	InitTypeInfo(pzPythonScriptFile);
	
	GString strClass = pzClassScope;
	GListIterator it( m_iC.GetMethods() );
	while (it()) // walk through all the methods
	{
		iMethod *p = (iMethod *)it++;
		// look for methods within the specified scope
		if ( strClass.CompareNoCase( p->GetScope() ) == 0 )
			m_pMethodList.AddLast( p->GetName() );
	}
	
	return &m_pMethodList;
}



GStringList *PythonInterface::GetMethodParams(const char *pzPythonScriptFile, const char *pzClassScope, const char *pzMethodName)
{
	m_pParamNameList.RemoveAll();
	m_pParamTypeList.RemoveAll();
	InitTypeInfo(pzPythonScriptFile);

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
GStringList* PythonInterface::GetMethodParamTypes()
{
	return &m_pParamTypeList;
}

// returns extended detail for the last call to GetMethodParams()
const char * PythonInterface::GetMethodReturnType()
{
	m_strReturnType = "String";
	return m_strReturnType;
}

// calls the "ExposedMethods" routine in pzPythonScriptFile to determine results
int PythonInterface::IsAvailable(const char *pzPythonScriptFile, const char *pzClassScope, const char *pzMethodName)
{
	InitTypeInfo(pzPythonScriptFile);
	return m_iC.IsMethodAvailable(pzMethodName);
}



PyObject *PythonInterface::GetPythonArgumentObject(const char *pzPythonScriptFile, const char *pzMethodName, GStringIterator &varArgs)
{
	int nPassedParams = 0;
	int nRequiredParamCount = 0;
	if (  strcmp(pzMethodName,"ExposedMethods") != 0  )
		nRequiredParamCount = m_iC.GetRequiredParamCount(pzMethodName);


	// Support these native Python datatypes
	GString strS("string");
	GString strI("integer");
	GString strF("float");

	PyObject *v = PyTuple_New(nRequiredParamCount);

	// For each required param 
	for(int i=0; i<nRequiredParamCount; i++)
	{
		if (! (varArgs()) )
		{
			// required %d params, but supplied %d nRequiredParamCount,nPassedParams
			throw GException("PythonIntegration", 14,pzPythonScriptFile,pzMethodName,nRequiredParamCount,nPassedParams);
		}
		const char *pzArgValue = varArgs++;
		iParam *pI = m_iC.GetMethodParam(pzMethodName, nPassedParams);
		// String
		if(  strS.CompareNoCase(pI->GetType()) == 0  )
		{
//			PyObject *x = PyString_FromStringAndSize(pzArgValue, strlen(pzArgValue));
			PyObject *x = PyBytes_FromStringAndSize(pzArgValue, strlen(pzArgValue));
			PyTuple_SetItem(v, nPassedParams++, x);
		}
		// Int, Long & Short
		else if(  strI.CompareNoCase(pI->GetType()) == 0  )
		{
			PyObject *x = PyLong_FromLong( atoi(pzArgValue) );
//			PyObject *x = PyInt_FromLong( atoi(pzArgValue) );
			PyTuple_SetItem(v, nPassedParams++, x);
		}
		// Double, Float
		else if(  strF.CompareNoCase(pI->GetType()) == 0  )
		{
			PyObject *x = PyFloat_FromDouble( atof(pzArgValue) );
			PyTuple_SetItem(v, nPassedParams++, x);
		}
		else
		{
			// unsupported datatype
			throw GException("PythonIntegration", 4, pI->GetType() );
		}
	}
	if (nPassedParams != nRequiredParamCount)
	{
		throw GException("PythonIntegration", 5, pzMethodName,pzPythonScriptFile,nRequiredParamCount,nPassedParams );
	}
	return v;
}

const char * PythonInterface::Invoke(const char *pzPythonScriptFile, const char *pzClassScope, const char *pzMethodName, GStringIterator &varArgs)
{
	if (strcmp(pzMethodName,"ExposedMethods") != 0)
		InitTypeInfo(pzPythonScriptFile);

	GString strGlobal = "Global";
	
	PyObject *pmod = PyImport_ImportModule( (char *)pzPythonScriptFile );
	if (pmod == NULL)
	{
		// Failed to Compile or load module[%s] from [%s]
		throw GException("PythonIntegration", 6, m_strComponentPath,pzPythonScriptFile );
	}

	
	// Call a global function
	if ( strGlobal.CompareNoCase("Global") == 0 )
	{
		PyObject *mdict = PyModule_GetDict(pmod);
		PyObject *func = PyDict_GetItemString(mdict, (char *)pzMethodName );
		if (!func) 
		{
			// Method [%s] not found at a global scope in module [%s]
			throw GException("PythonIntegration", 7,pzMethodName,pzPythonScriptFile );
		}
		if ( !PyCallable_Check(func) ) 
		{
			//  Global Method [%s] failed PyCallable_Check() in module [%s]
			throw GException("PythonIntegration", 8,pzMethodName,pzPythonScriptFile );
		}

		PyObject *pargs = GetPythonArgumentObject(pzPythonScriptFile, pzMethodName, varArgs);
		PyObject *rslt = PyObject_CallObject(func, pargs);
		char *pResult;
		if (!PyArg_Parse(rslt, "s", &pResult))
		{
			// %s::%s::%s return type is not string
			throw GException("PythonIntegration", 9 ,pzPythonScriptFile, pzClassScope ,pzMethodName );
		}
		m_strInvokeReturnValue = pResult;
	}
	// Call Method[pzMethodName] on Object [pzClassScope]
	else
	{
		// Find the class
		PyObject *pclass = PyObject_GetAttrString(pmod, (char *)pzClassScope );   /* fetch module.class */
		if (pclass == NULL)
		{
			// can't find class[%s] in module [%s]
			throw GException("PythonIntegration", 10 ,pzClassScope ,pzPythonScriptFile );
		}
		
		// Prepare empty args for the object ctor
		PyObject *pargs = Py_BuildValue("()");
		if (pargs == NULL) 
		{
			// Python internal - Py_BuildValue() failed;
			throw GException("PythonIntegration", 11 ,pzClassScope ,pzPythonScriptFile );
		}

		// Call the 'ctor, make an instance
		PyObject *pinst = PyEval_CallObject(pclass, pargs);         
		if (pinst == NULL)
		printf("Error calling module.klass()");

		PyObject *pmeth  = PyObject_GetAttrString(pinst, (char *)pzMethodName); 
		if (pmeth == NULL)
		{
	      // Can't find method[%s] in class[%s] in module[%s]
			throw GException("PythonIntegration", 12 , pzMethodName ,pzClassScope ,pzPythonScriptFile );
		}

		PyObject *pargs2 = GetPythonArgumentObject(pzPythonScriptFile, pzMethodName, varArgs);

		// Call the method
		PyObject *pres = PyEval_CallObject(pmeth, pargs2);
		if (pres == NULL)
		{
			// error calling method
			throw GException("PythonIntegration", 13 , pzMethodName ,pzClassScope ,pzPythonScriptFile );
		}
		
		// Check the return value
		char *pzReturn;
		if (!PyArg_Parse(pres, "s", &pzReturn))
		{
			// %s::%s::%s return type is not string
			throw GException("PythonIntegration", 9 ,pzPythonScriptFile, pzClassScope ,pzMethodName );
		}
		m_strInvokeReturnValue = pzReturn;
	}

	return m_strInvokeReturnValue;
}


void PythonInterface::InitTypeInfo(const char *pzPythonScriptFile)
{
	// only load the interface description one time
	if (m_strLastLoadedTypeLib.CompareNoCase(pzPythonScriptFile) == 0)
		return;
	m_iC.UnCache();
	m_strLastLoadedTypeLib = pzPythonScriptFile;

	// prepare an empty argument list
	GStringList emptyList;
	GStringIterator varArgs(new GStringList);

	const char *pzRet = Invoke(pzPythonScriptFile, "Global", "ExposedMethods", varArgs);
	
//  pzRet Interface description format:
//  -------------------------------------
//	"Class::Routine&varName&char *!"
//	"Class::OtherRoutine&Var1&char *&Var2&char *!";

	m_iC.LoadTypeLib(pzRet);
}

int PythonInterface::SetOption(const char *pzOption, void *pzValue)
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


void *PythonInterface::GetOption(const char *pzOption)
{

	return 0;
}
