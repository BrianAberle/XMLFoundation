#pragma once

// 5LoavesActiveX2012.h : main header file for 5LoavesActiveX2012.DLL

#if !defined( __AFXCTL_H__ )
#error "include 'afxctl.h' before including this file"
#endif

#include "resource.h"       // main symbols


// CMy5LoavesActiveX2012App : See 5LoavesActiveX2012.cpp for implementation.

class CMy5LoavesActiveX2012App : public COleControlModule
{
public:
	BOOL InitInstance();
	int ExitInstance();
};

extern const GUID CDECL _tlid;
extern const WORD _wVerMajor;
extern const WORD _wVerMinor;

