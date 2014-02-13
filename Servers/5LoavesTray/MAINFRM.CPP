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
#include "mainfrm.h"
#include <afxpriv.h>

// Message ID used for tray notifications
#define WM_MY_TRAY_NOTIFICATION WM_USER+0
const UINT WM_TASKBARCREATED = 
    ::RegisterWindowMessage(_T("TaskbarCreated"));



IMPLEMENT_DYNAMIC(CMainFrame, CFrameWnd)

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWnd)
	//{{AFX_MSG_MAP(CMainFrame)
	ON_MESSAGE(WM_MY_TRAY_NOTIFICATION, OnTrayNotification)
	ON_WM_CREATE()
	ON_WM_CLOSE()
	ON_COMMAND(ID_APP_OPEN, OnAppOpen)
	ON_COMMAND(ID_APP_SUSPEND, OnAppSuspend)
	ON_WM_SIZE()
	ON_WM_SETFOCUS()
	ON_WM_GETMINMAXINFO()
	ON_WM_ACTIVATE()
	ON_REGISTERED_MESSAGE(WM_TASKBARCREATED, OnTaskBarCreated)
	ON_WM_ERASEBKGND()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

static UINT BASED_CODE indicators[] = {
	ID_SEPARATOR,				// status line indicator
};

CMainFrame::CMainFrame() : m_trayIcon(IDR_TRAYICON)
{
	m_bShutdown = FALSE;
	m_bWantBackgroundPaint = 1;
}

CMainFrame::~CMainFrame()
{
}

// Called by Winodws after the Explorer shell has been restarted.
LRESULT CMainFrame::OnTaskBarCreated(WPARAM wp, LPARAM lp)
{
	const char *lpTip = "5Loaves";

	
	HICON hicon=NULL;
	AfxLoadString(IDR_MAINFRAME, m_trayIcon.m_nid.szTip, sizeof(m_trayIcon.m_nid.szTip));
	hicon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);


	UINT msg;
	m_trayIcon.m_nid.uFlags = 0;

	// Set the icon
	if (hicon) {
		// Add or replace icon in system tray
		msg = m_trayIcon.m_nid.hIcon ? NIM_MODIFY : NIM_ADD;
		m_trayIcon.m_nid.hIcon = hicon;
		m_trayIcon.m_nid.uFlags |= NIF_ICON;
	} else { // remove icon from tray
		if (m_trayIcon.m_nid.hIcon==NULL)
			return TRUE;		// already deleted
		msg = NIM_DELETE;
	}

	// Use the tip, if any
	if (lpTip)
		strncpy(m_trayIcon.m_nid.szTip, lpTip, sizeof(m_trayIcon.m_nid.szTip));
	if (m_trayIcon.m_nid.szTip[0])
		m_trayIcon.m_nid.uFlags |= NIF_TIP;

	// Use callback if any
	if (m_trayIcon.m_nid.uCallbackMessage && m_trayIcon.m_nid.hWnd)
		m_trayIcon.m_nid.uFlags |= NIF_MESSAGE;

	// Do it
    return Shell_NotifyIcon(NIM_ADD, &m_trayIcon.m_nid);
}



int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CFrameWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	// Set up tray icon
	m_trayIcon.SetNotificationWnd(this, WM_MY_TRAY_NOTIFICATION);
	m_trayIcon.SetIcon(IDR_MAINFRAME);

	return 0;
}

void CMainFrame::OnActivate( UINT nState, CWnd*, BOOL b )
{
}
//////////////////
// Close window. Unless we are shutting down, just hide it.
//
void CMainFrame::OnClose() 
{
	if (m_bShutdown)
	{
		((C5LoavesTrayApp *)AfxGetApp())->StopServer();
		CFrameWnd::OnClose();
	}
	else
		ShowWindow(SW_HIDE);
}

//////////////////
// Handle notification from tray icon: display a message.
//
LRESULT CMainFrame::OnTrayNotification(WPARAM uID, LPARAM lEvent)
{
	// let tray icon do default stuff
	return m_trayIcon.OnTrayNotification(uID, lEvent);
}

////////////////////////////////////////////////////////////////
// Command handlers below.
//


void CMainFrame::OnAppOpen() 
{
	try
	{
		ShowWindow(SW_NORMAL);	
		SetForegroundWindow();
	}
	catch(...)
	{
	}
}

void CMainFrame::OnAppSuspend() 
{
	m_bShutdown = TRUE;		// really exit
	SendMessage(WM_CLOSE);	
}


void CMainFrame::OnSize(UINT nType, int cx, int cy) 
{
	CFrameWnd::OnSize(nType, cx, cy);
}


void CMainFrame::OnSetFocus(CWnd* pOldWnd) 
{
	Invalidate();
	m_bWantBackgroundPaint = 1;
	CFrameWnd::OnSetFocus(pOldWnd);
}

void CMainFrame::OnGetMinMaxInfo(MINMAXINFO FAR* lpMMI) 
{
	Invalidate();
	CFrameWnd::OnGetMinMaxInfo(lpMMI);
}

BOOL CMainFrame::OnEraseBkgnd(CDC* pDC) 
{
	if(m_bWantBackgroundPaint)
	{
		m_bWantBackgroundPaint = 0;
		return CFrameWnd::OnEraseBkgnd(pDC);
	}
	return 0;
}
