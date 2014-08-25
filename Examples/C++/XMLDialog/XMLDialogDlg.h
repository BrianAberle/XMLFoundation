// XMLDialogDlg.h : header file
//

#if !defined(AFX_XMLDIALOGDLG_H__CE51C17C_CF23_428D_8DF0_FEDB71D77628__INCLUDED_)
#define AFX_XMLDIALOGDLG_H__CE51C17C_CF23_428D_8DF0_FEDB71D77628__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "xmlObject.h"
#include "GList.h"

/////////////////////////////////////////////////////////////////////////////
// CXMLDialogDlg dialog

class CXMLDialogDlg : public CDialog, public XMLObject
{
	void LoadLineItems() ;
	void ParseRawXML();
// Construction
public:
	CXMLDialogDlg(CWnd* pParent = NULL);	// standard constructor
	virtual void *ObjectMessage( int nCase, const char *pzArg1, const char *pzArg2, __int64 nArg3 = 0, void *pArg4 = 0 );

	GList m_lstOrders;
	virtual void MapXMLTagsToMembers();
	DECLARE_FACTORY(CXMLDialogDlg, Customer);

// Dialog Data
	//{{AFX_DATA(CXMLDialogDlg)
	enum { IDD = IDD_XMLDIALOG_DIALOG };
	CListCtrl	m_ListLineItems;
	CListCtrl	m_List;
	CString	m_strCity;
	CString	m_strCountry;
	CString	m_strName;
	CString	m_strRichEditXML;
	int		m_nProductID;
	CString	m_strPrice;
	CString	m_strDesc;
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CXMLDialogDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	//{{AFX_MSG(CXMLDialogDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnBtnLoadgui();
	afx_msg void OnItemchangedListOrders(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnItemchangedListLi(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnChangeLI();
	afx_msg void OnBtnMakexml();
	afx_msg void OnBtnAdd();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_XMLDIALOGDLG_H__CE51C17C_CF23_428D_8DF0_FEDB71D77628__INCLUDED_)
