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
#if !defined(AFX_FILEPOSTERDLG_H__25663427_632F_4E05_8EFA_4C853077904A__INCLUDED_)
#define AFX_FILEPOSTERDLG_H__25663427_632F_4E05_8EFA_4C853077904A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

/////////////////////////////////////////////////////////////////////////////
// CFilePosterDlg dialog

class CFilePosterDlg : public CDialog
{
// Construction
public:
	CFilePosterDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	//{{AFX_DATA(CFilePosterDlg)
	enum { IDD = IDD_FILEPOSTER_DIALOG };
	BOOL	m_bUseProxyFirewall;
	CString	m_strConnectName;
	CString	m_strLocalFile;
	int		m_nProxyPort;
	CString	m_strProxyServer;
	CString	m_strTitle;
	CString	m_strResults;
	CString	m_strSwitchBoardPath;
	int		m_nSwitchBoardPort;
	CString	m_strSwitchBoardServer;
	CString	m_strUserName;
	CString	m_strBasicAuth;
	BOOL	m_bUseBasicAuth;
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CFilePosterDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	//{{AFX_MSG(CFilePosterDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnBtnSend();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnBtnDirPick();
	afx_msg void OnBtnProxyHelp();
	afx_msg void OnBtnMeetHelp();
	afx_msg void OnBtnIdHelp();
	afx_msg void OnBtnLocHelp();
	afx_msg void OnButton1();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_FILEPOSTERDLG_H__25663427_632F_4E05_8EFA_4C853077904A__INCLUDED_)
