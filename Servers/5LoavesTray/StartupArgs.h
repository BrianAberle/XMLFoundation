#if !defined(AFX_STARTUPARGS_H__26202845_AF81_4669_8AAC_C33546E70441__INCLUDED_)
#define AFX_STARTUPARGS_H__26202845_AF81_4669_8AAC_C33546E70441__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// StartupArgs.h : header file
//
#include "UBTHyperlink.h"
/////////////////////////////////////////////////////////////////////////////
// CStartupArgs dialog

class CStartupArgs : public CDialog
{
// Construction
public:
	CStartupArgs(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CStartupArgs)
	enum { IDD = IDD_DLG_STARTUP_PASS };
	CUBTHyperlink	m_www;
	CString	m_strPass;
	CString	m_strPort;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CStartupArgs)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CStartupArgs)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STARTUPARGS_H__26202845_AF81_4669_8AAC_C33546E70441__INCLUDED_)
