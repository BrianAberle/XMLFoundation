// XMLDialog.h : main header file for the XMLDIALOG application
//

#if !defined(AFX_XMLDIALOG_H__D2127E6C_6CED_4BC4_8258_4EDC51319709__INCLUDED_)
#define AFX_XMLDIALOG_H__D2127E6C_6CED_4BC4_8258_4EDC51319709__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

/////////////////////////////////////////////////////////////////////////////
// CXMLDialogApp:
// See XMLDialog.cpp for the implementation of this class
//

class CXMLDialogApp : public CWinApp
{
public:
	CXMLDialogApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CXMLDialogApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CXMLDialogApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_XMLDIALOG_H__D2127E6C_6CED_4BC4_8258_4EDC51319709__INCLUDED_)
