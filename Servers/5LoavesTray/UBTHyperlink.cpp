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

#include "stdafx.h"
#include "5LoavesTray.h"
#include "UBTHyperlink.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CUBTHyperlink

CUBTHyperlink::CUBTHyperlink()
{
	m_UnderLine.CreateFont(8, 0, 0, 0, FW_NORMAL, 0, 1, 0,
		DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
		CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
		DEFAULT_PITCH|FF_SWISS,  "MS Sans Serif");
}

CUBTHyperlink::~CUBTHyperlink()
{
}


BEGIN_MESSAGE_MAP(CUBTHyperlink, CStatic)
	//{{AFX_MSG_MAP(CUBTHyperlink)
	ON_CONTROL_REFLECT(BN_CLICKED, OnClicked)
	ON_WM_CTLCOLOR_REFLECT()
	ON_WM_SETCURSOR()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CUBTHyperlink message handlers

void CUBTHyperlink::OnClicked() 
{
	CString strURL;
	GetWindowText(strURL);
	CString strFinalURL;
	strFinalURL = "http://";
	strFinalURL += strURL;
	::ShellExecute(m_hWnd, "open", strFinalURL, NULL, NULL, SW_SHOWNORMAL );
}

void CUBTHyperlink::PreSubclassWindow() 
{
	ModifyStyle(NULL, SS_NOTIFY);
	SetFont(&m_UnderLine);	
	CStatic::PreSubclassWindow();
}

HBRUSH CUBTHyperlink::CtlColor(CDC* pDC, UINT nCtlColor) 
{
	static COLORREF darkBlue = RGB(0x00, 0x00, 0x80);
	pDC->SetTextColor(darkBlue);
	pDC->SetBkMode(TRANSPARENT);
	return (HBRUSH)GetStockObject(NULL_BRUSH);
}

BOOL CUBTHyperlink::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message) 
{
	::SetCursor(AfxGetApp()->LoadStandardCursor(IDC_HELP));
	return TRUE;	
}
