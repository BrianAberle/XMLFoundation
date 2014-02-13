// MFCTypesFromXMLDlg.h : header file
//

#if !defined(AFX_MFCTYPESFROMXMLDLG_H__DCE515AD_2185_400D_8AB2_0E9F2A42FA7C__INCLUDED_)
#define AFX_MFCTYPESFROMXMLDLG_H__DCE515AD_2185_400D_8AB2_0E9F2A42FA7C__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

/////////////////////////////////////////////////////////////////////////////
// CMFCTypesFromXMLDlg dialog

class CTinyXMLObject : public XMLObject
{
	DECLARE_FACTORY(CTinyXMLObject, TinyObject);
	CTinyXMLObject();
public:
	CString m_str;
	virtual void MapXMLTagsToMembers();
};

//typedef CArray<CTinyXMLObject*, CTinyXMLObject*> CArrayTinyObjects;
#include <afxtempl.h>
class CMFCTypesFromXMLDlg : public CDialog, public XMLObject
{
// Construction
	DECLARE_FACTORY(CMFCTypesFromXMLDlg, MFCTypesSample);
public:
	void OnFromXML(const char *pzTestName);

	virtual void MapXMLTagsToMembers();
	CMFCTypesFromXMLDlg(CWnd* pParent = NULL);	// standard constructor
	~CMFCTypesFromXMLDlg();

// Dialog Data
	//{{AFX_DATA(CMFCTypesFromXMLDlg)
	enum { IDD = IDD_MFCTYPESFROMXML_DIALOG };
	CButton	m_btn7;
	CButton	m_btn6;
	CButton	m_btn5;
	CButton	m_btn4;
	CButton	m_btn3;
	CButton	m_btn2;
	CButton	m_btn1;
	CListBox	m_lstboxStrArr;
	CListBox	m_lstBoxArray;
	CListBox	m_lstBox;
	CString	m_strXML;
	CString	m_strCStr;
	CString	m_strCPtrListCount;
	CString	m_strGListObjCount;
	CString	m_strArrCount;
	CString	m_strArr2Count;
	CString	m_strHashCount;
	CString	m_strMap2Ptr;
	CString	m_strCompleteMessage;
	CString	m_strCompleteMessage2;
	CString	m_strCompleteMessage3;
	CString	m_strXML2;
	//}}AFX_DATA
	
	// MFC containers for C++ types
	CStringList m_lstCStrs;
	CDWordArray m_arrDWords;
	CStringArray m_arrCStrs;
	CArray<double,double> m_arrDbl;
	
	// MFC containers for XMLObjects
	CPtrList	m_lstMyObjects;
	CPtrArray	m_arr2MyObjs;
	CArray<CTinyXMLObject*, CTinyXMLObject*> m_arrMyObjects;
	CMapStringToPtr m_mapStrToPtr;
	
	// NON-MFC container for XMLObjects
	GList m_glstMyObjects;
	GHash m_htblTinyObjs;  // *** (Indexed) ***


	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMFCTypesFromXMLDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	HICON m_hIcon;
	void DeleteObjects(int bMFCOnly);
	void makeXMLInput(CString &strDest, const char *pzType, int nObjCount);
	CString m_strHashToSearchFor;
	CString m_strMapToSearchFor;
	int Load50kObjects(const char *pzType);

	// Generated message map functions
	//{{AFX_MSG(CMFCTypesFromXMLDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnToxml();
	afx_msg void OnBtnFirstCptr();
	afx_msg void OnBtnLastGlist();
	afx_msg void OnBtnViewCarray();
	afx_msg void OnBtnViewArr2();
	afx_msg void OnBtnHashSearch();
	afx_msg void OnBtnSearchMap();
	afx_msg void OnBtnXml1();
	afx_msg void OnBtnXml2();
	afx_msg void OnBtnXml3();
	afx_msg void OnBtnXml4();
	afx_msg void OnBtnXml5();
	afx_msg void OnBtnXml6();
	afx_msg void OnBtnXml7();
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	afx_msg void OnBtnCustomXml();
	afx_msg void OnBtnDel();
	afx_msg void OnBtnDel2();
	afx_msg void OnBtnDel3();
	afx_msg void OnBtnDel4();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MFCTYPESFROMXMLDLG_H__DCE515AD_2185_400D_8AB2_0E9F2A42FA7C__INCLUDED_)
