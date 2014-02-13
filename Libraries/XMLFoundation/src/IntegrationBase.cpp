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
static char SOURCE_FILE[] = __FILE__;

#include "GProfile.h"
#include "GDirectory.h"
#include "DynamicLibrary.h"
#include "GException.h"
#include "IntegrationBase.h"
#include "DynamicLibrary.h"

#include <string.h> // for: strlen(), memcmp(), strstr()

InterfaceInstance::InterfaceInstance( const InterfaceInstance &cpyFrm )
	:	BaseInterface(cpyFrm.m_strComponentPath, cpyFrm.m_strComponentSettings)
{  
	m_nThreadOperationMode = 3;
	m_bLockLib = 1;
	m_InvokeArgDelimit		= cpyFrm.m_InvokeArgDelimit;
	m_fnConstruct			= cpyFrm.m_fnConstruct;
	m_fnGetComponents		= cpyFrm.m_fnGetComponents;
	m_fnGetInterfaces		= cpyFrm.m_fnGetInterfaces;
	m_fnGetMethods			= cpyFrm.m_fnGetMethods;
	m_fnGetMethodParams		= cpyFrm.m_fnGetMethodParams;
	m_fnSetOption			= cpyFrm.m_fnSetOption;
	m_fnGetOption			= cpyFrm.m_fnGetOption;
	m_fnInvoke				= cpyFrm.m_fnInvoke;
	m_fnDestruct			= cpyFrm.m_fnDestruct;
}

void InterfaceInstance::Construct( )
{
	// Look in the config file for the Component Path.  For example when loading
	// driver "CStdCall" load entry for "CStdCallPath".  
	GString strComponentPath(GetProfile().GetPath((const char *)m_strDriver, "Path", false) );

	if (m_strDriver.CompareNoCase("TransactXML") == 0 || m_strDriver.CompareNoCase("TXML") == 0)
	{
		strComponentPath = GetProfile().GetPath("TXML", "RuleFilePath");
	}


	// Look in the config file for the Extended Settings.  For example when loading
	// driver "CStdCall" load entry for "CStdCallSettings".  
	GString strComponentSettings(GetProfile().GetString((const char *)m_strDriver, "TXML", false) );

	// retain storage in base calss
	m_strComponentPath = (const char *)strComponentPath;
	m_strComponentSettings = (const char *)strComponentSettings;

	const char *pzOps = m_fnDriverOperation();
	if (strlen(pzOps) > 1)
	{
		m_nThreadOperationMode = pzOps[0] - 48;
		m_bLockLib = pzOps[1] - 48;
	}
	m_iHandle = m_fnConstruct(  (const char *)m_strComponentPath, (const char *)m_strComponentSettings );

	m_fnSetOption(m_iHandle,"rsltGStr",&m_strResults);
	m_pActiveResultGString = &m_strResults;
}
	





// Either constructs the requested object or throws an exception. 
// pzDriver is the "name" returned by the Driver that you are loading.
InterfaceInstance::InterfaceInstance( const char *pzDriver)
	:	BaseInterface("", "")
{ 
	m_InvokeArgDelimit = "&~&";	
	// language driver operation modes:
	// --------------------------------
	// 1 Single Threaded- 1 instance. TXML Server Blocking. 
	//					  No two methods will ever be simultaneous executed.  
	//					  TXML Server threads wait in line for the single instance of this object.
	// 2 Multi Threaded	- 1 instance. All server threads into a single instance. 
	//					  Handle your own thread safety issues.
	// 3 Multi Object	- Multi instance - Object pool. (DEFAULT)
	//					  Interface Objects are accessed by any thread as necessary,
    //					  A single instance will only be used by a single thread at any moment.
	//					  this relieves the driver implementation of thread blocking routines.
	// 4 As necessary	- Multi instance. Created and destroyed with each use. No caching.
	//
	m_strDriver = pzDriver;
	void *pfn = 0;
	
	// Get location of System Language Drivers
	GString strDriverPath = GetProfile().GetPath("TXML", "Drivers");

	// loop through every file in the Drivers directory
	GDirectory dir( strDriverPath );
	GStringIterator it(&dir);
	while (it())
	{
		GString strFile( it++ ); // next file in Drivers Directory
		
		if ( !(strFile.CompareSubNoCase(".dll",(int)strFile.Length() - 4) == 0 || strFile.CompareSubNoCase(".so",(int)strFile.Length() - 3) == 0) )
			continue; // only attempt to load .DLL and .SO files

		GString strPathedDynamicLib = strDriverPath;
		strPathedDynamicLib += strFile;
		GString strDriver( pzDriver );
		m_dllHandle = _OPENLIB( strPathedDynamicLib );
		if (m_dllHandle)
		{
			try // No Destructor cleanup when we throw from the Constructor
			{
				// if the "GetDriverName" entry point is found
				void *pfn = (void *)_PROCADDRESS(m_dllHandle, "GetDriverName");
				if (pfn)
				{
					// determine if this the driver we are searching for
					fnGetDriverName f = (fnGetDriverName)pfn;
					if ( strDriver.CompareNoCase( f() ) == 0 )
					{
						GString strErrorMessage;

						// load the required entry points from the driver.
						// Throws exception if the assignment cannot be made
						m_fnDriverOperation = (fnDriverOperation)_PROCADDRESS(m_dllHandle, "DriverOperation");
						if (!m_fnDriverOperation )
							throw GException("CInterface", 0, "DriverOperation", pzDriver);

						m_fnConstruct = (fnConstruct)_PROCADDRESS(m_dllHandle, "Construct");
						if (!m_fnConstruct )
							throw GException("CInterface", 0, "Construct", pzDriver);

						m_fnGetComponents = (fnGetComponents)_PROCADDRESS(m_dllHandle, "GetComponents");
						if (!m_fnGetComponents )
							throw GException("CInterface", 0, "GetComponents", pzDriver);

						m_fnGetInterfaces = (fnGetInterfaces)_PROCADDRESS(m_dllHandle, "GetInterfaces");
						if (!m_fnGetInterfaces )
							throw GException("CInterface", 0, "GetInterfaces", pzDriver);

						m_fnGetMethods = (fnGetMethods)_PROCADDRESS(m_dllHandle, "GetMethods");
						if (!m_fnGetMethods )
							throw GException("CInterface", 0, "GetMethods", pzDriver);

						m_fnGetMethodParams = (fnGetMethodParams)_PROCADDRESS(m_dllHandle, "GetMethodParams");
						if (!m_fnGetMethodParams )
							throw GException("CInterface", 0, "GetMethodParams", pzDriver);

						m_fnIsAvailable = (fnIsAvailable)_PROCADDRESS(m_dllHandle, "IsAvailable");
						if (!m_fnIsAvailable )
							throw GException("CInterface", 0, "IsAvailable", pzDriver);

						m_fnSetOption = (fnSetOption)_PROCADDRESS(m_dllHandle, "SetOption");
						if (!m_fnSetOption )
							throw GException("CInterface", 0, "SetOption", pzDriver);

						m_fnGetOption = (fnGetOption)_PROCADDRESS(m_dllHandle, "GetOption");
						if (!m_fnGetOption )
							throw GException("CInterface", 0, "GetOption", pzDriver);

						m_fnInvoke = (fnInvoke)_PROCADDRESS(m_dllHandle, "Invoke");
						if (!m_fnInvoke )
							throw GException("CInterface", 0, "Invoke", pzDriver);

						m_fnDestruct = (fnDestruct)_PROCADDRESS(m_dllHandle, "Destruct");
						if (!m_fnDestruct )
							throw GException("CInterface", 0, "Destruct", pzDriver);

						// Invoke the Driver Initialization routine
						Construct();

						// Driver is loaded an initialized, "this" is constructed.
						return;
					}
				}
			}
			catch(...)
			{
				if (!m_bLockLib)
					_CLOSELIB(m_dllHandle);
				throw; // rethrow whatever we caught
			}
			if (!m_bLockLib)
				_CLOSELIB(m_dllHandle);
		}
	}


	// if we made it here, the requested driver was NOT loaded.  
	// We don't exactly know why. But we can state the facts.
	GString strErrorMessage;
	strErrorMessage.Format("Failed to load driver [%s]\n",pzDriver);
	if (!m_dllHandle)
	{
		strErrorMessage += "No drivers could be loaded from [";
		strErrorMessage += strDriverPath;
		strErrorMessage += "]\n";
	}
	else if (!pfn)
	{
		if (!m_bLockLib)
			_CLOSELIB(m_dllHandle);
		strErrorMessage += "No Language Drivers exposing the [GetDriverName] entry point were found.";
	}
	else
	{
		if (!m_bLockLib)
			_CLOSELIB(m_dllHandle);
		strErrorMessage += "Error while handshaking with Driver.";
	}
	throw GException("CInterface", 1, (const char *)strErrorMessage);
}

InterfaceInstance::~InterfaceInstance()
{ 
	m_fnDestruct(m_iHandle); 
	if (!m_bLockLib)
		_CLOSELIB(m_dllHandle);
};

GStringList* InterfaceInstance::GetComponents()
{
	m_pObjectList.RemoveAll();
	GString strList (m_fnGetComponents(m_iHandle));
	m_pObjectList.DeSerialize("&&",strList);
	return &m_pObjectList;
};

GStringList* InterfaceInstance::GetInterfaces(const char *pzObject)
{
	m_pInterfaceList.RemoveAll();
	GString strList(m_fnGetInterfaces(m_iHandle,pzObject));
	m_pInterfaceList.DeSerialize("&&",strList);
	return &m_pInterfaceList;
};

GStringList* InterfaceInstance::GetMethods(const char *pzObject, const char *pzInterface)
{
	m_pMethodList.RemoveAll();
	GString strList(m_fnGetMethods(m_iHandle,pzObject,pzInterface));
	m_pMethodList.DeSerialize("&&",strList);
	return &m_pMethodList;
};

// return detailed information about the last method described by GetMethodParams()
GStringList* InterfaceInstance::GetMethodParamTypes()
{
	return &m_pParamTypeList;
};
const char * InterfaceInstance::GetMethodReturnType()
{
	return m_strReturnType;
};
GStringList* InterfaceInstance::GetMethodParams(const char *pzObject, const char *pzInterface, const char *pzMethodName)
{
	m_pParamNameList.RemoveAll();
	m_pParamTypeList.RemoveAll();

	// get the "NameList!TypeList!ReturnType" from the driver
	const char *pResults = m_fnGetMethodParams(m_iHandle,pzObject,pzInterface,pzMethodName);
	if (!pResults) // if the method was not found return an empty list
	{
		// "2=Method[%s] not exported by[%s]\r\n"
		throw GException("CInterface", 2, pzMethodName, pzObject);
	}

	GString strLists(pResults);
	const char *pzLists = strLists;

	// if there are parameters (because both lists are empty)
	if ( memcmp(pzLists,"!!",8) != 0 )
	{
		// find the 'end' of the first list
		char *pListSeperator = (char *)strstr(pzLists,"!");
		*pListSeperator = 0;
		// load m_pParamNameList from the serialized list data
		m_pParamNameList.DeSerialize("&",pzLists);
		
		// advance to the next list
		pListSeperator += 4;	// 4 is the size of the list terminator "&&&&"
		// find it's end
		char *pListTerminator = (char *)strstr(pListSeperator,"!");
		*pListTerminator = 0;

		// load m_pParamTypeList from the serialized list data
		m_pParamTypeList.DeSerialize("&",pListSeperator);
		
		// advance past the end of the 2nd list.
		pListTerminator += 4;
		// set the return type
		m_strReturnType = pListTerminator;
	}
	else
	{
		// the return type is after the two back-to-back list terminations
		m_strReturnType = pzLists + 8;
	}

	return &m_pParamNameList;
};

int InterfaceInstance::IsAvailable(const char *pzObject, const char *pzInterface, const char *pzMethodName)
{
	return m_fnIsAvailable(m_iHandle,pzObject,pzInterface,pzMethodName);
}

void *InterfaceInstance::GetOption(const char *pzOption)
{
	return m_fnGetOption(m_iHandle,pzOption);
}

int InterfaceInstance::SetOption(const char *pzOption, void *pzValue)
{
	GString strOption(pzOption);
	
	if (strOption.CompareNoCase("rsltGStr") == 0)
	{
		m_pActiveResultGString = (GString *)pzValue;
	}
	if (strOption.CompareNoCase("Delimit") == 0)
	{
		m_InvokeArgDelimit = (const char *)pzValue;
	}
	return m_fnSetOption(m_iHandle,pzOption,pzValue);
}

const char * InterfaceInstance::InvokeEx(const char *pzObject, const char *pzInterface, const char *pzMethodName, const char *pArgData, const char *pzArgSizes, int *pnOutResultSize)
{
	m_fnSetOption(m_iHandle,"DataMap",(void *)pzArgSizes); // pzArgSizes = n|n|n|n

/*	m_strInvokeReturnValue = */m_fnInvoke(m_iHandle,
										pzObject, 
										pzInterface, 
										pzMethodName, 
										pArgData );
	
	if (pnOutResultSize)
		*pnOutResultSize = (int)m_pActiveResultGString->Length();

	return m_pActiveResultGString->StrVal();
}

const char * InterfaceInstance::Invoke(const char *pzObject, const char *pzInterface, const char *pzMethodName, GStringIterator &varArgs)
{
	GString strArgs;
	int nCount = 0;
	while ( varArgs() )
	{
		if (nCount)
			strArgs += m_InvokeArgDelimit;
		nCount++;
		strArgs += (char *)(const char *)varArgs++;
	}
	
	m_fnSetOption(m_iHandle,"DataMap",(void *)""); // data is not mapped, it is delimitted.

	m_fnInvoke(m_iHandle,
				pzObject, 
				pzInterface, 
				pzMethodName, 
				(const char *)strArgs );

	return m_pActiveResultGString->StrVal();
};


