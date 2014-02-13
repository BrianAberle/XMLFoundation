// StartupArgs.cpp : implementation file
//

#include "stdafx.h"
#include "5LoavesTray.h"
#include "StartupArgs.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CStartupArgs dialog


CStartupArgs::CStartupArgs(CWnd* pParent /*=NULL*/)
	: CDialog(CStartupArgs::IDD, pParent)
{
	//{{AFX_DATA_INIT(CStartupArgs)
	m_strPass = _T("");
	m_strPort = _T("10888");
	//}}AFX_DATA_INIT
}


void CStartupArgs::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CStartupArgs)
	DDX_Control(pDX, IDC_WWW, m_www);
	DDX_Text(pDX, IDC_EDT_PASS, m_strPass);
	DDX_Text(pDX, IDC_EDT_PORT, m_strPort);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CStartupArgs, CDialog)
	//{{AFX_MSG_MAP(CStartupArgs)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CStartupArgs message handlers
