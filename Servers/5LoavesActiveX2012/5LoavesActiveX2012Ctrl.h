#pragma once

// 5LoavesActiveX2012Ctrl.h : Declaration of the CMy5LoavesActiveX2012Ctrl ActiveX Control class.


// CMy5LoavesActiveX2012Ctrl : See 5LoavesActiveX2012Ctrl.cpp for implementation.

class CMy5LoavesActiveX2012Ctrl : public COleControl
{
	DECLARE_DYNCREATE(CMy5LoavesActiveX2012Ctrl)

// Constructor
public:
	CMy5LoavesActiveX2012Ctrl();

// Overrides
public:
	virtual void OnDraw(CDC* pdc, const CRect& rcBounds, const CRect& rcInvalid);
	virtual void DoPropExchange(CPropExchange* pPX);
	virtual void OnResetState();

// Implementation
protected:
	~CMy5LoavesActiveX2012Ctrl();

	DECLARE_OLECREATE_EX(CMy5LoavesActiveX2012Ctrl)    // Class factory and guid
	DECLARE_OLETYPELIB(CMy5LoavesActiveX2012Ctrl)      // GetTypeInfo
	DECLARE_PROPPAGEIDS(CMy5LoavesActiveX2012Ctrl)     // Property page IDs
	DECLARE_OLECTLTYPE(CMy5LoavesActiveX2012Ctrl)		// Type name and misc status

// Message maps
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

