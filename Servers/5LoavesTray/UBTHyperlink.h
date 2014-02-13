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

#if !defined(AFX_UBTHYPERLINK_H__463D1D5C_0475_4485_831B_239CAF7662F5__INCLUDED_)
#define AFX_UBTHYPERLINK_H__463D1D5C_0475_4485_831B_239CAF7662F5__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// UBTHyperlink.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CUBTHyperlink window

class CUBTHyperlink : public CStatic
{
	CFont m_UnderLine;
public:
	CUBTHyperlink();

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CUBTHyperlink)
	protected:
	virtual void PreSubclassWindow();
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CUBTHyperlink();

	// Generated message map functions
protected:
	//{{AFX_MSG(CUBTHyperlink)
	afx_msg void OnClicked();
	afx_msg HBRUSH CtlColor(CDC* pDC, UINT nCtlColor);
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_UBTHYPERLINK_H__463D1D5C_0475_4485_831B_239CAF7662F5__INCLUDED_)
