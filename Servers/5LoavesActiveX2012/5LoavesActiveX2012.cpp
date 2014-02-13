// 5LoavesActiveX2012.cpp : Implementation of CMy5LoavesActiveX2012App and DLL registration.

#include "stdafx.h"
#include "5LoavesActiveX2012.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


CMy5LoavesActiveX2012App theApp;

const GUID CDECL _tlid = { 0x93B52DBE, 0x79D2, 0x4E96, { 0x95, 0x8B, 0x98, 0x10, 0x1E, 0x6E, 0xCD, 0x26 } };
const WORD _wVerMajor = 1;
const WORD _wVerMinor = 0;



// CMy5LoavesActiveX2012App::InitInstance - DLL initialization

BOOL CMy5LoavesActiveX2012App::InitInstance()
{
	BOOL bInit = COleControlModule::InitInstance();

	if (bInit)
	{
		// TODO: Add your own module initialization code here.
	}

	return bInit;
}



// CMy5LoavesActiveX2012App::ExitInstance - DLL termination

int CMy5LoavesActiveX2012App::ExitInstance()
{
	// TODO: Add your own module termination code here.

	return COleControlModule::ExitInstance();
}



// DllRegisterServer - Adds entries to the system registry

STDAPI DllRegisterServer(void)
{
	AFX_MANAGE_STATE(_afxModuleAddrThis);

	if (!AfxOleRegisterTypeLib(AfxGetInstanceHandle(), _tlid))
		return ResultFromScode(SELFREG_E_TYPELIB);

	if (!COleObjectFactoryEx::UpdateRegistryAll(TRUE))
		return ResultFromScode(SELFREG_E_CLASS);

	return NOERROR;
}



// DllUnregisterServer - Removes entries from the system registry

STDAPI DllUnregisterServer(void)
{
	AFX_MANAGE_STATE(_afxModuleAddrThis);

	if (!AfxOleUnregisterTypeLib(_tlid, _wVerMajor, _wVerMinor))
		return ResultFromScode(SELFREG_E_TYPELIB);

	if (!COleObjectFactoryEx::UpdateRegistryAll(FALSE))
		return ResultFromScode(SELFREG_E_CLASS);

	return NOERROR;
}
