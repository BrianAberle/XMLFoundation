#if !defined(AFX_5LOAVESACTIVEX_H__EC711E7A_3EBA_4660_9323_756DB6E2B39B__INCLUDED_)
#define AFX_5LOAVESACTIVEX_H__EC711E7A_3EBA_4660_9323_756DB6E2B39B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

// 5LoavesActivex.h : main header file for 5LOAVESACTIVEX.DLL

#if !defined( __AFXCTL_H__ )
	#error include 'afxctl.h' before including this file
#endif

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// C5LoavesApp : See 5LoavesActivex.cpp for implementation.

class C5LoavesApp : public COleControlModule
{
public:
	BOOL InitInstance();
	int ExitInstance();
};

extern const GUID CDECL _tlid;
extern const WORD _wVerMajor;
extern const WORD _wVerMinor;

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_5LOAVESACTIVEX_H__EC711E7A_3EBA_4660_9323_756DB6E2B39B__INCLUDED)
