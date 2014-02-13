#if !defined(AFX_5LOAVESCTL_H__EEFE5E05_5510_4E6F_9D42_A16876A6E3EB__INCLUDED_)
#define AFX_5LOAVESCTL_H__EEFE5E05_5510_4E6F_9D42_A16876A6E3EB__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

// 5LoavesCtl.h : Declaration of the C5LoavesCtrl ActiveX Control class.

/////////////////////////////////////////////////////////////////////////////
// C5LoavesCtrl : See 5LoavesCtl.cpp for implementation.

class C5LoavesCtrl : public COleControl
{
	DECLARE_DYNCREATE(C5LoavesCtrl)

// Constructor
public:
	C5LoavesCtrl();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(C5LoavesCtrl)
	public:
	virtual void OnDraw(CDC* pdc, const CRect& rcBounds, const CRect& rcInvalid);
	virtual void DoPropExchange(CPropExchange* pPX);
	virtual void OnResetState();
	//}}AFX_VIRTUAL

// Implementation
protected:
	~C5LoavesCtrl();

	DECLARE_OLECREATE_EX(C5LoavesCtrl)    // Class factory and guid
	DECLARE_OLETYPELIB(C5LoavesCtrl)      // GetTypeInfo
	DECLARE_PROPPAGEIDS(C5LoavesCtrl)     // Property page IDs
	DECLARE_OLECTLTYPE(C5LoavesCtrl)		// Type name and misc status

// Message maps
	//{{AFX_MSG(C5LoavesCtrl)
		// NOTE - ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

// Dispatch maps
	//{{AFX_DISPATCH(C5LoavesCtrl)
	afx_msg long StartServer();
	afx_msg long StopServer();
	afx_msg long SetConfigFile(LPCTSTR StartConfigFile);
	afx_msg long SetConfigEntry(LPCTSTR Section, LPCTSTR Entry, LPCTSTR Value);
	afx_msg BSTR GetLast5LoavesError();
	//}}AFX_DISPATCH
	DECLARE_DISPATCH_MAP()

// Event maps
	//{{AFX_EVENT(C5LoavesCtrl)
	//}}AFX_EVENT
	DECLARE_EVENT_MAP()

// Dispatch and event IDs
public:
	enum {
	//{{AFX_DISP_ID(C5LoavesCtrl)
	dispidStartServer = 1L,
	dispidStopServer = 2L,
	dispidSetConfigFile = 3L,
	dispidSetConfigEntry = 4L,
	dispidGetLast5LoavesError = 5L,
	//}}AFX_DISP_ID
	};
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_5LOAVESCTL_H__EEFE5E05_5510_4E6F_9D42_A16876A6E3EB__INCLUDED)
