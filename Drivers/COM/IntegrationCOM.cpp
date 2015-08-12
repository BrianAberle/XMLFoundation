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
#include "IntegrationCOM.h"

#include "COMDEF.H"
#include <ole2.h>
#include <tchar.h>
#include <stdio.h>
#include "GDirectory.h"
#include "ExceptionHandler.h"
#include "GException.h"

// NOTE: this project does NOT link the XMLFoundation - but it directly compiles several
// XMLFoundation utilities. It simplifies driver builds that can get complex when linking 
// things like the Perl runtime or PHP interpreter due to threading models.  Since the base
// driver template does not link to anything it makes it easier to link to anything during
// driver development.

COMInterface::COMInterface( const char *pzComponentPath,const char *pzComponentSettings )
	:	BaseInterface(pzComponentPath, pzComponentSettings)
{
	CoInitialize( 0 );
};

COMInterface::~COMInterface()
{
	CoUninitialize();
};

GStringList * COMInterface::GetComponents()
{
	m_pObjectList.RemoveAll();
	GDirectory dir( (const char *)m_strComponentPath );
	GStringIterator it(&dir);
	while (it())
		m_pObjectList.AddLast( it++ );
	return &m_pObjectList;
}


GStringList *COMInterface::GetInterfaces(const char *pzObject)
{
	m_pInterfaceList.RemoveAll();
	LPTYPELIB pITypeLib;

	char pzObjectAndPath[512];
	sprintf(pzObjectAndPath,"%s\\%s",(const char *)m_strComponentPath,pzObject);
    WCHAR pzwObject[512];
    MultiByteToWideChar(CP_ACP, 0, pzObjectAndPath, -1, pzwObject, 512);

	HRESULT hr = LoadTypeLib( pzwObject, &pITypeLib );
	if ( S_OK != hr )
	{
		throw GException("COMIntegration", 1, pzObject);
	}

	// Enum statically bound Typelibs
	UINT tiCount = pITypeLib->GetTypeInfoCount();
	for ( UINT i = 0; i < tiCount; i++ )
	{
		LPTYPEINFO pITypeInfo;
		HRESULT hr = pITypeLib->GetTypeInfo( i, &pITypeInfo );
		if ( S_OK == hr )
		{
			// class interface name
			BSTR bstrInterface;
			hr = pITypeInfo->GetDocumentation(MEMBERID_NIL, &bstrInterface, 0, 0, 0);
			if ( S_OK != hr )
			{
				return 0;
			}


			char buf [256];
			WideCharToMultiByte(CP_ACP, 0, 
						bstrInterface,
						-1,
						buf,
						256,
						NULL, NULL);
			SysFreeString( bstrInterface );

			m_pInterfaceList.AddLast(buf);
							
			pITypeInfo->Release();
		}
	}
	pITypeLib->Release();
	return &m_pInterfaceList;
}

GStringList *COMInterface::GetMethods(const char *pzObject, const char *pzInterface)
{
	m_pMethodList.RemoveAll();
	LPTYPELIB pITypeLib;

	char pzObjectAndPath[512];
	sprintf(pzObjectAndPath,"%s\\%s",(const char *)m_strComponentPath,pzObject);
    WCHAR pzwObject[512];
    MultiByteToWideChar(CP_ACP, 0, pzObjectAndPath, -1,
                      pzwObject, 512);

	HRESULT hr = LoadTypeLib( pzwObject, &pITypeLib );
	if ( S_OK != hr )
	{
		throw GException("COMIntegration", 1, pzObject);
	}

	// Enum statically bound Typelibs
	UINT tiCount = pITypeLib->GetTypeInfoCount();
	for ( UINT i = 0; i < tiCount; i++ )
	{
		LPTYPEINFO pITypeInfo;
		
		HRESULT hr = pITypeLib->GetTypeInfo( i, &pITypeInfo );

		if ( S_OK == hr )
		{
			// class interface name
			BSTR bstrInterface;
			hr = pITypeInfo->GetDocumentation(MEMBERID_NIL, &bstrInterface, 0, 0, 0);
			if ( S_OK != hr )
			{
				return 0;
			}

			TYPEATTR * pTypeAttr;
			hr = pITypeInfo->GetTypeAttr( &pTypeAttr );
			if ( S_OK != hr )
			{
				SysFreeString( bstrInterface );
				return 0;
			}


			char buf [256];
			WideCharToMultiByte(CP_ACP, 0, 
						bstrInterface,
						-1,
						buf,
						256,
						NULL, NULL);
			SysFreeString( bstrInterface );

			GString strCurrentIntreface;
			strCurrentIntreface = buf;
			if ( strCurrentIntreface.CompareNoCase( pzInterface ) == 0)
			{
				LPTYPEINFO pITypeInfo;
				HRESULT hr = pITypeLib->GetTypeInfo( i, &pITypeInfo );

				if ( pTypeAttr->cFuncs )
				{
					for ( unsigned i = 0; i < pTypeAttr->cFuncs; i++ )
					{
						FUNCDESC * pFuncDesc;
						pITypeInfo->GetFuncDesc( i, &pFuncDesc );


						SAFEARRAY *psaNames = NULL;
						BSTR myStringArray[256];
						unsigned int nOutCount = 0;

						// Max supported parameters = 256
						pITypeInfo->GetNames(
							pFuncDesc->memid, 
							(unsigned short **)myStringArray, 
							256, 
							&nOutCount);


						for (unsigned int i = 0; i < nOutCount; i++) 
						{
							
							char buf [512];
							WideCharToMultiByte(CP_ACP, 0, 
										myStringArray[i],
										-1,
										buf,
										512,
										NULL, NULL);
							if (!pFuncDesc->lprgelemdescParam)
							{
								continue;
							}
							if (i == 0)
							{
								m_pMethodList.AddLast(buf);
							}
						}
						for (i = 0; i < nOutCount; i++) 
						{
							SysFreeString(myStringArray[i]);
						}

						pITypeInfo->ReleaseFuncDesc( pFuncDesc );						
					}
				}
			}
			SysFreeString( bstrInterface );
			pITypeInfo->Release();
		}
	}
	pITypeLib->Release();
	return &m_pMethodList;
}

GStringList *COMInterface::GetMethodParams(const char *pzObject, const char *pzInterface, const char *pzMethodName)
{
	m_pParamNameList.RemoveAll();
	m_pParamTypeList.RemoveAll();

	LPTYPELIB pITypeLib;

	char pzObjectAndPath[512];
	sprintf(pzObjectAndPath,"%s\\%s",(const char *)m_strComponentPath,pzObject);
    WCHAR pzwObject[512];
    MultiByteToWideChar(CP_ACP, 0, pzObjectAndPath, -1,
                      pzwObject, 512);

	HRESULT hr = LoadTypeLib( pzwObject, &pITypeLib );
	if ( S_OK != hr )
	{
		throw GException("COMIntegration", 1, pzObject);
	}

	// Enum statically bound Typelibs
	UINT tiCount = pITypeLib->GetTypeInfoCount();
	for ( UINT i = 0; i < tiCount; i++ )
	{
		LPTYPEINFO pITypeInfo;

		HRESULT hr = pITypeLib->GetTypeInfo( i, &pITypeInfo );

		if ( S_OK == hr )
		{
			
			// TYPEATTR contains object details (methods and attributes)
			TYPEATTR * pTypeAttr;
			hr = pITypeInfo->GetTypeAttr( &pTypeAttr );
			if ( S_OK != hr )
			{
				return 0;
			}
			
			// Get the Interface name into a GString in 17 lines of code.
			BSTR bstrInterface;
			hr = pITypeInfo->GetDocumentation(MEMBERID_NIL, &bstrInterface, 0, 0, 0);
			if ( S_OK != hr )
			{
				return 0;
			}
			char pzCurrentInterface [256]; // temporary holder 
			WideCharToMultiByte(CP_ACP, 0, 
						bstrInterface,
						-1,
						pzCurrentInterface,
						256,
						NULL, NULL);
			SysFreeString( bstrInterface );
			GString strCurrentIntreface(pzCurrentInterface); // assign the value into a GString


			// if we found the requested interface in the typelib
			if ( strCurrentIntreface.CompareNoCase( pzInterface ) == 0) 
			{
				LPTYPEINFO pITypeInfo;
				HRESULT hr = pITypeLib->GetTypeInfo( i, &pITypeInfo );

				// for each exposed method
				for ( unsigned i = 0; i < pTypeAttr->cFuncs; i++ )
				{
					// attributes of method[i] in this interface
					FUNCDESC * pFuncDesc;
					pITypeInfo->GetFuncDesc( i, &pFuncDesc );


					// GetNames() gets the names of the method and its parameters and loads them at 
					// the location specified by the input SAFEARRAY pointer.
					SAFEARRAY *psaNames = NULL;
					BSTR myStringArray[256];	// limits support to methods < 255 parameters
					unsigned int nOutCount = 0;
					pITypeInfo->GetNames(
						pFuncDesc->memid, 
						(unsigned short **)myStringArray, 
						256, 
						&nOutCount);

					// for each parameter of this method
					for (unsigned int i = 0; i < nOutCount; i++) 
					{
						
						char pzMethodOrParamName [256];
						WideCharToMultiByte(CP_ACP, 0, 
									myStringArray[i],
									-1,
									pzMethodOrParamName,
									256,
									NULL, NULL);
						if (!pFuncDesc->lprgelemdescParam) 
						{
							break; // AddRef() and Release() end up here
						}
						
						// the first position of the array is the method name if it's not what we're looking for, move on to the next method.
						GString strMethod;
						strMethod = pzMethodName;
						if ( i == 0 && strMethod.CompareNoCase( pzMethodOrParamName ) != 0 )
							break;

						char pzDataType [256];
						
						// The elemdescFunc field indicates the return type of the method. 
						// The lprgelemdescParam element in a FUNCDESC points at an array of ELEMDESC structures. Each ELEMDESC represents one of the method's parameters.
						int nTypeID = (i == 0) ? pFuncDesc->elemdescFunc.tdesc.vt : pFuncDesc->lprgelemdescParam[i-1].tdesc.vt;
						
						switch (nTypeID)
						{
						case VT_I2:
						case VT_I4:
								sprintf(pzDataType,"Integer");
								break;
						case VT_R4:
						case VT_R8:
								sprintf(pzDataType,"Double");
								break;
						case VT_CY:
								sprintf(pzDataType,"Currency");
								break;
						case VT_DATE:
								sprintf(pzDataType,"DateTime");
								break;
						case VT_BSTR:
								sprintf(pzDataType,"String");
								break;
						case VT_DISPATCH:
								sprintf(pzDataType,"Dispatch");
								break;
						case VT_ERROR:
								sprintf(pzDataType,"Error");
								break;
						case VT_BOOL:
								sprintf(pzDataType,"Boolean");
								break;
						case VT_VARIANT:
								sprintf(pzDataType,"Variant");
								break;
						case VT_UNKNOWN:
								sprintf(pzDataType,"Unknown");
								break;
						case VT_DECIMAL:
								sprintf(pzDataType,"Decimal");
								break;
						case VT_I1:
						case VT_UI1:
								sprintf(pzDataType,"Byte");
								break;
						case VT_UI2:
						case VT_INT:
						case VT_UINT:
						case VT_UI4:
								sprintf(pzDataType,"Integer");
								break;
						case VT_I8:
						case VT_UI8:
								sprintf(pzDataType,"Integer64");
								break;
						case VT_VOID:
								sprintf(pzDataType,"Void");
								break;
						case VT_PTR:
								sprintf(pzDataType,"char *");
								break;
						case VT_SAFEARRAY:
								sprintf(pzDataType,"Array");
								break;
						case VT_CARRAY:
								sprintf(pzDataType,"CArray");
								break;
						case VT_USERDEFINED:
								sprintf(pzDataType,"UserDef");
								break;
						case VT_LPSTR:
								sprintf(pzDataType,"String");
								break;
						case VT_LPWSTR:
								sprintf(pzDataType,"WString");
								break;
						case VT_HRESULT:
								sprintf(pzDataType,"HResult");
								break;
							default:
								sprintf(pzDataType,"Unknown Type");
								break;
						}
						if (i > 0) // this is a parameter
						{
							m_pParamNameList.AddLast(pzMethodOrParamName);
							m_pParamTypeList.AddLast(pzDataType);
						}
						else // This is the return type
						{
							m_strReturnType = pzDataType;
						}
                    }
					for (i = 0; i < nOutCount; i++) 
					{
						SysFreeString(myStringArray[i]);
					}

					pITypeInfo->ReleaseFuncDesc( pFuncDesc );
				}
			}
			SysFreeString( bstrInterface );
			pITypeInfo->Release();
		}
	}
	pITypeLib->Release();
	return &m_pParamNameList;
}

// Only valid after GetMethodParams() has been called.
GStringList* COMInterface::GetMethodParamTypes()
{
	return &m_pParamTypeList;
}

const char *COMInterface::GetMethodReturnType()
{
	return m_strReturnType;
}


int COMInterface::IsAvailable(const char *pzObject, const char *pzInterface, const char *pzMethodName)
{
	GStringList *pList = GetMethods(pzObject, pzInterface);
	GStringIterator it( pList );
	GString strMatch(pzMethodName);
	while (it())
	{
		if (strMatch.Compare(it++) == 0)
		{
			return 1;
		}
	}
	return 0;
}




// Performance (relative to as fast as possible) is very poor - This will be improved later
// by caching the TypeLib information, and pooling the implementation objects.  
const char * COMInterface::Invoke(const char *pzObject, const char *pzInterface, const char *pzMethodName, GStringIterator &varArgs)
{
	LPTYPELIB pITypeLib;

	char pzObjectAndPath[512];
	sprintf(pzObjectAndPath,"%s\\%s",(const char *)m_strComponentPath,pzObject);
    WCHAR pzwObject[512];
    MultiByteToWideChar(CP_ACP, 0, pzObjectAndPath, -1, pzwObject, 512);

	HRESULT hr = LoadTypeLib( pzwObject, &pITypeLib );
	if ( S_OK != hr )
	{
		throw GException("COMIntegration", 1, pzObject);
	}

	// Enum statically bound Typelibs
	UINT tiCount = pITypeLib->GetTypeInfoCount();
	for ( UINT iTypeLibIndex = 0; iTypeLibIndex < tiCount; iTypeLibIndex++ )
	{
		LPTYPEINFO pITypeInfo;
		HRESULT hr = pITypeLib->GetTypeInfo( iTypeLibIndex, &pITypeInfo );

		if ( S_OK == hr )
		{
			
			// TYPEATTR contains object details (methods and attributes)
			TYPEATTR * pTypeAttr;
			hr = pITypeInfo->GetTypeAttr( &pTypeAttr );
			if ( S_OK != hr )
			{
				throw GException("COMIntegration", 2);
			}
			
			// Get the Interface name into a GString.
			BSTR bstrInterface;
			hr = pITypeInfo->GetDocumentation(MEMBERID_NIL, &bstrInterface, 0, 0, 0);
			if ( S_OK != hr )
			{
				throw GException("COMIntegration", 3);
			}
			char pzCurrentInterface [256]; // temporary holder 
			WideCharToMultiByte(CP_ACP, 0, bstrInterface, -1, pzCurrentInterface, 256, NULL, NULL);
			SysFreeString( bstrInterface );
			GString strCurrentIntreface(pzCurrentInterface);


			// if we found the requested interface in the typelib
			if ( strCurrentIntreface.CompareNoCase( pzInterface ) == 0) 
			{
				LPTYPEINFO pITypeInfo;
				HRESULT hr = pITypeLib->GetTypeInfo( iTypeLibIndex, &pITypeInfo );

				// for each exposed method
				for ( unsigned int iMethodIndex = 0; iMethodIndex < pTypeAttr->cFuncs; iMethodIndex++ )
				{
					bool bPerformInvoke = true;

					// attributes of method[i] in this interface
					FUNCDESC * pFuncDesc;
					hr = pITypeInfo->GetFuncDesc( iMethodIndex, &pFuncDesc );


					// GetNames() gets the names of the method and its parameters and loads them into myStringArray 
					SAFEARRAY *psaNames = NULL;
					BSTR myStringArray[256];	// limits support to methods < 255 parameters
					unsigned int nOutCount = 0;
					hr = pITypeInfo->GetNames( pFuncDesc->memid, (unsigned short **)myStringArray, 256, &nOutCount);

					int nNumberOfArguments = nOutCount - 1; // subtract 1 because the first position is the method name

					// Gets type information for the method and it's parameters
					DISPID *MethodDispIDs = new DISPID[nOutCount];	
					hr = pITypeInfo->GetIDsOfNames(&myStringArray[0], nOutCount, MethodDispIDs);

					// storage for the parameters bound for execution by ::Invoke()
					DISPPARAMS dispparamsNoArgs = {NULL, NULL, 0, 0};
					DISPPARAMS dpArguments;
					VARIANTARG *pBoundParams = 0;
					dpArguments.cArgs = nNumberOfArguments;
					dpArguments.cNamedArgs= nNumberOfArguments;
					if (nNumberOfArguments > 0)
					{
						pBoundParams = new VARIANTARG[nNumberOfArguments];
						for(int nInit = 0; nInit < nNumberOfArguments; nInit++)
						{
							VariantInit(&pBoundParams[nInit]);
						}
						dpArguments.rgdispidNamedArgs= &(MethodDispIDs[1]);
						dpArguments.rgvarg=pBoundParams;
					}


					// for each parameter of this method (and the method name in the 0th position)
					int nBoundParamsIndex = -1;
					for (unsigned int iParamIndex = 0; iParamIndex < nOutCount; iParamIndex++) 
					{
						
						// get the method name into pzMethodOrParamName
						char pzMethodOrParamName [256];
						WideCharToMultiByte(CP_ACP, 0, myStringArray[iParamIndex],	-1,pzMethodOrParamName, 256, NULL, NULL);
						
						
						if (!pFuncDesc->lprgelemdescParam) 
						{
							bPerformInvoke = false;
							break; // AddRef() and Release() end up here
						}
						
						// the first position of the array is the method name
						GString strMethod;
						strMethod = pzMethodName;
						if ( iParamIndex == 0 && strMethod.CompareNoCase( pzMethodOrParamName ) != 0 )
						{
							bPerformInvoke = false;
							break; // this is not the method we're looking for, try the next one
						}

						// the first index is the return type of the function - Don't prepare a parameter for this one.
						if (iParamIndex == 0)
							continue;
						

						// The elemdescFunc field indicates the return type of the method. 
						// The lprgelemdescParam element in a FUNCDESC points at an array of ELEMDESC structures. Each ELEMDESC represents one of the method's parameters.
						int nTypeID = pFuncDesc->lprgelemdescParam[iParamIndex-1].tdesc.vt;
						
						// set the type of the argument in the variant paramater bind storage
						nBoundParamsIndex++;
						pBoundParams[nBoundParamsIndex].vt = nTypeID;

						// now set the value in this switch statement
						GString strValue = varArgs++;
						GString strUnsupportedDataType;
						switch (nTypeID)
						{
						case VT_BOOL: // Boolean
								/* 0 == FALSE, -1 == TRUE */
								if (strValue.CompareNoCase("yes") == 0 || 
									strValue.CompareNoCase("true") == 0	|| 
									strValue.CompareNoCase("1") == 0)
									pBoundParams[nBoundParamsIndex].boolVal = -1;
								else
									pBoundParams[nBoundParamsIndex].boolVal = 0;
								break;
						case VT_I1:
						case VT_UI1:// Byte
								pBoundParams[nBoundParamsIndex].bVal = strValue[0];
								break;
						case VT_I2:  
						case VT_UI2:// Short Integer
								pBoundParams[nBoundParamsIndex].iVal=atoi( (char *)(const char *)strValue );
								break;
						case VT_I4:
						case VT_INT:		
						case VT_UINT:
						case VT_UI4:// Integer 
								pBoundParams[nBoundParamsIndex].lVal=atol( (char *)(const char *)strValue );
								break;
						case VT_BSTR:// BSTR
								WCHAR pzwValue[512];
								MultiByteToWideChar(CP_ACP, 0, (char *)(const char *)strValue, -1, pzwValue, 512);
								pBoundParams[nBoundParamsIndex].bstrVal= SysAllocString( pzwValue );
								break;
						case VT_PTR: // inproc char *
								pBoundParams[nBoundParamsIndex].pcVal = (char *)(const char *)strValue;
								break;
						case VT_DATE:
								// strValue should be in the form mm/dd/yyyy
								WCHAR pzwDate[512];
								MultiByteToWideChar(CP_ACP, 0, (char *)(const char *)strValue, -1, pzwDate, 512);
								VarDateFromStr(  pzwDate,  ::GetUserDefaultLCID(), 0,  &pBoundParams[nBoundParamsIndex].date  );
								break;
						case VT_I8:
						case VT_UI8:
								strUnsupportedDataType = "Integer64";
								break;
						case VT_R4:
						case VT_R8: 
								strUnsupportedDataType = "Double";
								break;
						case VT_CY:
								strUnsupportedDataType = "Currency";
								break;
						case VT_DISPATCH:
								strUnsupportedDataType = "Dispatch";
								break;
						case VT_ERROR:
								strUnsupportedDataType = "Error";
								break;
						case VT_VARIANT:
								strUnsupportedDataType = "Variant";
								break;
						case VT_UNKNOWN:
								strUnsupportedDataType = "Unknown";
								break;
						case VT_DECIMAL:
								strUnsupportedDataType = "Decimal";
								break;
						case VT_VOID:
								strUnsupportedDataType = "Void";
								break;
						case VT_SAFEARRAY:
								strUnsupportedDataType = "Array";
								break;
						case VT_CARRAY:
								strUnsupportedDataType = "CArray";
								break;
						case VT_USERDEFINED:
								strUnsupportedDataType = "UserDef";
								break;
						case VT_LPSTR:
								strUnsupportedDataType = "String";
								break;
						case VT_LPWSTR:
								strUnsupportedDataType = "WString";
								break;
						case VT_HRESULT:
								strUnsupportedDataType = "HResult";
								break;
							default:
								strUnsupportedDataType << "Unknown COM Type " << nTypeID << '\0';
								break;
						}
						
						if (!strUnsupportedDataType.IsEmpty())
						{
							throw GException("COMIntegration", 4,(const char *)strValue,(const char *)strUnsupportedDataType);
						}
                    }
					
					// cleanup some of the stuff we're done with.
					pITypeInfo->ReleaseFuncDesc( pFuncDesc );
					for (unsigned int nReleaseIndex = 0; nReleaseIndex < nOutCount; nReleaseIndex++) 
					{
						SysFreeString(myStringArray[nReleaseIndex]);
					}

					// Only Perform the Invoke() if this is the correct method and we've bound the input params
					if (bPerformInvoke)
					{
						// Invoke output storage
						VARIANT vReturnValue;
						VariantInit(&vReturnValue);
						EXCEPINFO ei;
						unsigned int nArgError = 0;

						
						void *pInstance = 0;
	//													??????WHY?????
    //
	//             pITypeInfo->CreateInstance() returns "Wrong module kind for the operation" -2147317571 (800288BD)

	//					I want to Invoke() directly on the ITypeLib rather than going through IDispatch. like this:
    // 					hr = pITypeInfo->CreateInstance(NULL, guid, &pInstance);
	//					hr = pITypeInfo->Invoke(
	//						pInstance,
	//						dispid,  // I got this one from pITypeInfo->GetIDsOfNames("MyMethod")
	//						DISPATCH_METHOD,
	//						&dpArguments
	//						&vReturnValue, 
	//						&ei, &nArgError);

	//					I can only hope that pITypeInfo->CreateInstance() will create an instance of the 
	//					object that this typelib is bound to and not go to the registry seeking the location 
    //					of the binary to load.  If I cannot stop the registry lookup, I would not consider
	//					COM reasonably secure, due to the amount of management required to admin Registry
	//					Access.  I'm not exactly sure what is goin on under the cover here, and I like knowing.
    //					


						
						// ----- I'm sure this duct tape won't hold long ------
						// AppWizard Com objects by default have a coclass(always 1 prior to iTypeLibIndex).
						// VB does not, unless it's a VB OCX(then the coclass is always after iTypeLibIndex).
						// There must be a correct way from the current 'TypeInfo' to obtain the coclass guid 
						// required by CreateInstance(), until I find it check (Prior, Current, then Next)
						int nSearchCount = (iTypeLibIndex) ? 3 : 2;
						int nSearchStart = (iTypeLibIndex) ? iTypeLibIndex - 1 : iTypeLibIndex;
						int nIterationCount = 0;
						for(int nSearch = nSearchStart - 1; nIterationCount < nSearchCount; nIterationCount++)
						{
							nSearch++;
							LPTYPEINFO pITypeInfoTemp;
							hr = pITypeLib->GetTypeInfo( nSearch, &pITypeInfoTemp );
							if (S_OK != hr)
								continue;
							TYPEATTR * pTypeAttrTemp;
							hr = pITypeInfoTemp->GetTypeAttr( &pTypeAttrTemp );
							if (S_OK != hr)
								continue;
							
							hr = CoCreateInstance(pTypeAttrTemp->guid, NULL, CLSCTX_ALL, IID_IDispatch, &pInstance);
							if (S_OK != hr)
								continue;
							else
								break;
					
						}
						if (S_OK != hr)
						{
							throw GException("COMIntegration", 5);
						}


						// Finally Invoke the method on our new instance

						// Invoke() fails with DISP_E_BADVARTYPE for anything it can't marshall
						// like a "const char *" or VT_PTR.  If we didn't have to pass through
						// IDispatch->Invoke() but used the pITypeInfo->Invoke() we could use VT_PTR
						// to represent "const char *" and MFC programmers won't have to deal with BSTR's
						hr = ((IDispatch *)pInstance)->Invoke(MethodDispIDs[0], 
															IID_NULL,
															LOCALE_USER_DEFAULT,
															DISPATCH_METHOD,
															&dpArguments,
															&vReturnValue,&ei, &nArgError);

						// return an exception if Invoke() failed
						GString strInvokeError;
						if (S_OK != hr)
						{
							switch(hr)
							{
							case DISP_E_BADPARAMCOUNT: strInvokeError << "COM: The number of elements provided to DISPPARAMS is different from the number of arguments accepted by the method or property.";  break;
							case DISP_E_BADVARTYPE: strInvokeError << "COM: One of the arguments is not [oleautomation] compatible. ";  break;
							case DISP_E_EXCEPTION: strInvokeError << "COM: The application needs to raise an exception. In this case, the structure passed in pExcepInfo should be filled in.";  break;
							case DISP_E_MEMBERNOTFOUND: strInvokeError << "COM: The requested member does not exist, or the call to Invoke tried to set the value of a read-only property.";  break;
							case DISP_E_NONAMEDARGS: strInvokeError << "COM: This implementation of IDispatch does not support named arguments.";  break;
							case DISP_E_OVERFLOW: strInvokeError << "COM: One of the arguments in rgvarg could not be coerced to the specified type.";  break;
							case DISP_E_PARAMNOTFOUND: strInvokeError << "COM: One of the parameter DISPIDs does not correspond to a parameter on the method. In this case, puArgErr should be set to the first argument that contains the error.";  break;
							case DISP_E_TYPEMISMATCH: strInvokeError << "COM: One or more of the arguments could not be coerced. The index within rgvarg of the first parameter with the incorrect type is returned in the puArgErr parameter.";  break;
							case DISP_E_UNKNOWNINTERFACE: strInvokeError << "COM: The interface identifier passed in riid is not IID_NULL.";  break;
							case DISP_E_UNKNOWNLCID: strInvokeError << "COM: The member being invoked interprets string arguments according to the LCID, and the LCID is not recognized. If the LCID is not needed to interpret arguments, this error should not be returned.";  break;
							case DISP_E_PARAMNOTOPTIONAL: strInvokeError << "COM: A required parameter was omitted.";  break;
							default : strInvokeError.Format("COM: Unknown COM Error[%d] on Invoke()",(int)hr);  break;
							}
							throw GException("COMIntegration", 6, (const char *)strInvokeError);

						}
						delete MethodDispIDs;

						// convert the Invoke() return value.  Each case is responsible to either
						// place the return value into m_strInvokeReturnValue or place the type 
						// of types we can't deal with into strUnsupportedDataType.
						GString strUnsupportedDataType;
						switch(vReturnValue.vt)
						{
						case VT_BOOL: // Boolean
								/* 0 == FALSE, -1 == TRUE */
								if (vReturnValue.boolVal == -1)
									m_strInvokeReturnValue << "True";
								else
									m_strInvokeReturnValue << "False";
								break;
						case VT_I1:
						case VT_UI1:// Byte
								m_strInvokeReturnValue << vReturnValue.bVal;
								break;
						case VT_I2: 
						case VT_UI2: // Short Int
								m_strInvokeReturnValue << vReturnValue.iVal;
						case VT_I4:
						case VT_INT:		
						case VT_UINT:
						case VT_UI4:// Integer 
								m_strInvokeReturnValue << vReturnValue.lVal;
								break;
						case VT_BSTR:// BSTR
								char buf [256];
								WideCharToMultiByte(CP_ACP, 0, 
											vReturnValue.bstrVal,
											-1,
											buf,
											256,
											NULL, NULL);
								// SysFreeString( vReturnValue.bstrVal );  Is this necessary?
								m_strInvokeReturnValue = buf;
								break;
						case 0:
						case VT_VOID: // No Return Value
								m_strInvokeReturnValue = "";
								break;
						case VT_PTR: // char * 
								m_strInvokeReturnValue = (const char *)vReturnValue.pcVal;
								break;
						case VT_DATE: //Date
								BSTR bstrOut; 
  								VarBstrFromDate( vReturnValue.date, ::GetUserDefaultLCID(), 0,  &bstrOut  );

								char bufTemp [256];
								WideCharToMultiByte(CP_ACP, 0, 
											bstrOut,
											-1,
											bufTemp,
											256,
											NULL, NULL);
								// SysFreeString( vReturnValue.bstrVal );  Is this necessary?
								m_strInvokeReturnValue << bufTemp;
								break;
						case VT_I8:
						case VT_UI8:
								strUnsupportedDataType = "Integer64";
								break;
						case VT_R4:
						case VT_R8: 
								strUnsupportedDataType = "Double";
								break;
						case VT_CY:
								strUnsupportedDataType = "Currency";
								break;
						case VT_DISPATCH:
								strUnsupportedDataType = "Dispatch";
								break;
						case VT_ERROR:
								strUnsupportedDataType = "Error";
								break;
						case VT_VARIANT:
								strUnsupportedDataType = "Variant";
								break;
						case VT_UNKNOWN:
								strUnsupportedDataType = "Unknown";
								break;
						case VT_DECIMAL:
								strUnsupportedDataType = "Decimal";
								break;
						case VT_SAFEARRAY:
								strUnsupportedDataType = "Array";
								break;
						case VT_CARRAY:
								strUnsupportedDataType = "CArray";
								break;
						case VT_USERDEFINED:
								strUnsupportedDataType = "UserDef";
								break;
						case VT_LPSTR:
								strUnsupportedDataType = "String";
								break;
						case VT_LPWSTR:
								strUnsupportedDataType = "WString";
								break;
						case VT_HRESULT:
								strUnsupportedDataType = "HResult";
								break;
							default:
								strUnsupportedDataType << "Unknown Type " << vReturnValue.vt << '\0';
								break;
						}
						if ( !strUnsupportedDataType.IsEmpty() )
						{
							throw GException("COMIntegration", 7, (const char *)strUnsupportedDataType,vReturnValue.vt);

						}

						break;	// we got our return value, don't look at the next method.
					}
    			}
			}
			SysFreeString( bstrInterface );
			pITypeInfo->Release();
		}
	}
	pITypeLib->Release();

	return m_strInvokeReturnValue;
}


