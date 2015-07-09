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
#include "FilePoster.h"
#include "FilePosterDlg.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif



#define __NO_OPENSSL // simplify the example
#include "..\..\..\Servers\Core\ServerCore.cpp"


/////////////////////////////////////////////////////////////////////////////
// CFilePosterDlg dialog

CFilePosterDlg::CFilePosterDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CFilePosterDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CFilePosterDlg)
	m_bUseProxyFirewall = FALSE;
	m_strConnectName = _T("UBTsAccountForYou");
	m_strLocalFile = _T("Press (...) to select a file to send");
	m_nProxyPort = 81;
	m_strProxyServer = _T("192.168.1.11");
	m_strTitle = _T("");
	m_strResults = _T("");
	m_strSwitchBoardPath = _T("/PublicPath/");
	m_nSwitchBoardPort = 80;
	m_strSwitchBoardServer = _T("www.SwitchBoardServer.com");
	m_strUserName = _T("Me");
	m_strBasicAuth = _T("user:password");
	m_bUseBasicAuth = FALSE;
	//}}AFX_DATA_INIT
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CFilePosterDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CFilePosterDlg)
	DDX_Check(pDX, IDC_CHK_PROXY, m_bUseProxyFirewall);
	DDX_Text(pDX, IDC_EDT_CONNECT_NAME, m_strConnectName);
	DDX_Text(pDX, IDC_EDT_LOCAL_FILE, m_strLocalFile);
	DDX_Text(pDX, IDC_EDT_PROXY_PORT, m_nProxyPort);
	DDX_Text(pDX, IDC_EDT_PROXY_SRV, m_strProxyServer);
	DDX_Text(pDX, IDC_EDT_REMOTE_FILE, m_strTitle);
	DDX_Text(pDX, IDC_EDT_RESULTS, m_strResults);
	DDX_Text(pDX, IDC_EDT_SB_PATH, m_strSwitchBoardPath);
	DDX_Text(pDX, IDC_EDT_SB_PORT, m_nSwitchBoardPort);
	DDX_Text(pDX, IDC_EDT_SB_SERVER, m_strSwitchBoardServer);
	DDX_Text(pDX, IDC_EDT_USER_NAME, m_strUserName);
	DDX_Text(pDX, IDC_EDT_STR_AUTH, m_strBasicAuth);
	DDX_Check(pDX, IDC_CHK_AUTH, m_bUseBasicAuth);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CFilePosterDlg, CDialog)
	//{{AFX_MSG_MAP(CFilePosterDlg)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BTN_SEND, OnBtnSend)
	ON_WM_TIMER()
	ON_BN_CLICKED(IDC_BTN_DIR_PICK, OnBtnDirPick)
	ON_BN_CLICKED(IDC_BTN_PROXY_HELP, OnBtnProxyHelp)
	ON_BN_CLICKED(IDC_BTN_MEET_HELP, OnBtnMeetHelp)
	ON_BN_CLICKED(IDC_BTN_ID_HELP, OnBtnIdHelp)
	ON_BN_CLICKED(IDC_BTN_LOC_HELP, OnBtnLocHelp)
	ON_BN_CLICKED(IDC_BUTTON1, OnButton1)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CFilePosterDlg message handlers

BOOL CFilePosterDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	
	return TRUE;  // return TRUE  unless you set the focus to a control
}

//  If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CFilePosterDlg::OnPaint() 
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, (WPARAM) dc.GetSafeHdc(), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

HCURSOR CFilePosterDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}

MessageSendArgs *pArg = 0;
void CFilePosterDlg::OnBtnSend() 
{

	GetDlgItem(IDC_BTN_SEND)->EnableWindow(FALSE);
	pArg = new MessageSendArgs;
	UpdateData(TRUE);
	if (m_bUseProxyFirewall)
	{
		pArg->strServer = m_strProxyServer;
		pArg->nPort = m_nProxyPort;
		pArg->strSwitchBoardServer = m_strSwitchBoardServer;
	}
	else
	{
		pArg->strServer = m_strSwitchBoardServer;
		pArg->nPort = m_nSwitchBoardPort;
	}
	
	if (m_bUseBasicAuth)
		pArg->strBasicHTTPAuth = m_strBasicAuth;

	pArg->strPublicPathName = m_strSwitchBoardPath;
	pArg->strConnectName = m_strConnectName;
	pArg->strUserName = m_strUserName;
	pArg->strFileToSend = m_strLocalFile;
	pArg->strRemoteTitle = m_strTitle;


	// hang the GUI and thump up a messagebox
//	MessageSend((void *)pArg);
//	AfxMessageBox(pArg->strErrorDescription);
//	delete pArg;
//	GetDlgItem(IDC_BTN_SEND)->EnableWindow(TRUE);

	// or specify async and do your own message
	// pump checking it's progress in an OnTimer()
	// call back or at some specific event in your
	// application.
	pArg->bExecuteAsync = 1;
	SetTimer( 777,1000, 0 );
	MessageSend(pArg);
}

int nConnectWait = 0;
void CFilePosterDlg::OnTimer(UINT_PTR nIDEvent) 
{
	if (nIDEvent == 777 && pArg)
	{
		if (pArg->nResult > 2)
		{
			MessageBeep(MB_ICONEXCLAMATION);
			m_strResults = (const char *)pArg->strErrorDescription;
			nConnectWait = 0;
			KillTimer(777);
			delete pArg;
			GetDlgItem(IDC_BTN_SEND)->EnableWindow(TRUE);
		}
		else if (pArg->nResult == 2)
		{
			// do your completion event
			MessageBeep(0);
			m_strResults = (const char *)pArg->strResponse;
			nConnectWait = 0;
			KillTimer(777);
			delete pArg;
			GetDlgItem(IDC_BTN_SEND)->EnableWindow(TRUE);
		}
		else if (pArg->nTotalDataSent && pArg->nFileBytesToSend) 
		{
			float nPct = ((float)pArg->nTotalDataSent / (float)pArg->nFileBytesToSend) * 100;
			int nnPct = nPct;
			m_strResults.Format("%%%d Sent",nnPct);
		}
		else
		{
			nConnectWait++;
			m_strResults.Format("%d Waiting to send",nConnectWait);
		}
		UpdateData(FALSE);
	}

	CDialog::OnTimer(nIDEvent);
}

void CFilePosterDlg::OnBtnDirPick() 
{
	CFileDialog dlg(TRUE); // open dialog
	if (dlg.DoModal() == IDOK)
	{
		UpdateData(TRUE); // get values fron GUI
		m_strLocalFile = dlg.GetPathName();
		m_strTitle = dlg.GetFileName();
		UpdateData(FALSE); // put them to GUI
	}
}

void CFilePosterDlg::OnBtnProxyHelp() 
{
	GString strMsg;	
	strMsg  << "Only set the 'use proxy' check-box if your browser requires a proxy\n"
			<< "to reach the internet.  Here is how to check:\n\n"
			<< "If you use IE: Select 'Tools' then 'Internet Options...'\n"
			<< "Select the 'Connections' tab then press 'LAN settings'\n"
			<< "Copy the values from the 'proxy server' section.\n\n\n"
			<< "If you use Navigator:\n"
			<< "Edit>Preferences>Advanced>Proxies>Manual Proxy Configuration>View\n\n\n\n"
			<< "Only Set the 'Use Basic HTTP Auth' if your network uses\n"
			<< "Basic HTTP Authentication causing your browser to prompt you\n"
			<< "for a user/password before connecting to the internet.\n";
	AfxMessageBox(strMsg);
}

void CFilePosterDlg::OnBtnMeetHelp() 
{
	GString strMsg;	
	strMsg  << "This is the location where you will 'meet' the recipient.\n"
			<< "The recipient must give you a location to meet, it may be\n"
			<< "a public SwitchBoardServer like www.SwitchBoardServer.com\n"
			<< "or a server assigned by the receiver.  The port is almost\n"
			<< "always 80 and will be defined by the SwitchBoard operator.\n"
			<< "'SwitchBoard Path' is also assigned by the SwitchBoard operator\n";
	AfxMessageBox(strMsg);
}

void CFilePosterDlg::OnBtnIdHelp() 
{
	GString strMsg;	
	strMsg  << "Connect-Name: Identifies who you want to send to\n\n"
			<< "Your Name: Identifies yourself, you can put any value\n"
			<< "you want in 'Your Name' unless the receiver only allows\n"
			<< "certified connections from the SwitchBoardServer.\n";
	AfxMessageBox(strMsg);
}

void CFilePosterDlg::OnBtnLocHelp() 
{
	GString strMsg;	
	strMsg  << "Local File: Is a full path and file name to send\n"
			<< "press the '...' button to select a file from your machine\n\n"
			<< "Title: is the name of the file on the remote machine.\n"
			<< "Most receivers will only allow a file name here, but if they\n"
			<< "do allow then you can put in the entire destination path\n"
			<< "like 'c:\\dir\\file.doc' or '\\\\Server\\Dept\\Project\\file.doc'\n";
	AfxMessageBox(strMsg);
}

bool UnpackNotes(const char *pzDestFile)
{
	HINSTANCE hInst = AfxGetResourceHandle();
	HRSRC hRsrc = ::FindResource(hInst, (LPCSTR)MAKEINTRESOURCE(IDR_README1), "README");
	if (hRsrc == NULL)
	{
		return false;
	}
	HGLOBAL hGlobal = ::LoadResource(hInst, hRsrc);
	if (hGlobal == NULL)
	{
		return false;
	}
	LPCSTR lpsz = (LPCSTR)::LockResource(hGlobal);
	if (lpsz == NULL)
	{
		return false;
	}

	DWORD dwWritten = SizeofResource(hInst, hRsrc);
	if (dwWritten == 0)
	{
		return false;
	}

	HANDLE Handle = CreateFile(pzDestFile, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_HIDDEN, 0);
	if (INVALID_HANDLE_VALUE == Handle)
	{
		return false;
	}

	if (!WriteFile(Handle, lpsz, dwWritten, &dwWritten, NULL))
	{
		CloseHandle(Handle);
		return false;
	}

	CloseHandle(Handle);

	return true;
}


void CFilePosterDlg::OnButton1() 
{
	char pzBuf[512];
	GetTempPath(512,pzBuf);
	GString strTempFile ( pzBuf );
	if ( strTempFile.GetAt(strTempFile.GetLength() - 1) != '\\')
		strTempFile << "\\";
	strTempFile << "FilePoster.Notes.txt";

	UnpackNotes(strTempFile);

	GString strCommand("notepad.exe ");
	strCommand << strTempFile;

	PROCESS_INFORMATION pInfo;
	STARTUPINFO         sInfo;

	sInfo.cb              = sizeof(STARTUPINFO);
	sInfo.lpReserved      = NULL;
	sInfo.lpReserved2     = NULL;
	sInfo.cbReserved2     = 0;
	sInfo.lpDesktop       = NULL;
	sInfo.lpTitle         = NULL;
	sInfo.dwFlags         = 0;
	sInfo.dwX             = 0;
	sInfo.dwY             = 0;
	sInfo.dwFillAttribute = 0;
	sInfo.wShowWindow     = SW_SHOW;
	CreateProcess(NULL, (char *)(const char *)strCommand, NULL, NULL, FALSE, 0,NULL, NULL, &sInfo, &pInfo);
	
}
