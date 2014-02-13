// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#if !defined(AFX_STDAFX_H__0878E114_7829_4386_9253_104020219D1E__INCLUDED_)
#define AFX_STDAFX_H__0878E114_7829_4386_9253_104020219D1E__INCLUDED_


#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define VC_EXTRALEAN		// Exclude rarely-used stuff from Windows headers

#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions
#include <afxdtctl.h>		// MFC support for Internet Explorer 4 Common Controls
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>			// MFC support for Windows Common Controls
#endif // _AFX_NO_AFXCMN_SUPPORT

// make sure you have "/XMLFoundation/Libraries/XMLFoundation/inc" in the compile include path
// link to  "/XMLFoundation/Libraries/Debug/XmlFoundation.lib" (libXMLFoundation.a in Linux)
// In Windows verify the default C Runtime Libraries is "Multithreaded DLL" or "Debug Mulithreaded DLL"
// Because that is the default build setting for XMLFoundation - just as it is for MFC apps.

// NOTE:This <XMLFoundation> #include must be AFTER the AFX stuff or it will error with this:
// fatal error C1189: #error:WINDOWS.H already included.  MFC apps must not #include <windows.h>
#include <XMLFoundation.h>
#include "GPerformanceTimer.h" // only used for getTimeMS() definition


//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__0878E114_7829_4386_9253_104020219D1E__INCLUDED_)
