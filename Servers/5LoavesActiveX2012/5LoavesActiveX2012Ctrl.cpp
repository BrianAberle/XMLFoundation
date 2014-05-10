// 5LoavesActiveX2012Ctrl.cpp : Implementation of the CMy5LoavesActiveX2012Ctrl ActiveX Control class.

#include "stdafx.h"
#include "5LoavesActiveX2012.h"
#include "5LoavesActiveX2012Ctrl.h"
#include "5LoavesActiveX2012PropPage.h"
#include "afxdialogex.h"

#include "GProfile.h"
#include "GException.h"
#include "afxconv.h"
GString g_strLastError;
int isRunning=0;

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

IMPLEMENT_DYNCREATE(CMy5LoavesActiveX2012Ctrl, COleControl)

// Message map

BEGIN_MESSAGE_MAP(CMy5LoavesActiveX2012Ctrl, COleControl)
	ON_OLEVERB(AFX_IDS_VERB_PROPERTIES, OnProperties)
END_MESSAGE_MAP()

// Dispatch map

BEGIN_DISPATCH_MAP(CMy5LoavesActiveX2012Ctrl, COleControl)
	//{{AFX_DISPATCH_MAP(C5LoavesCtrl)
	DISP_FUNCTION(CMy5LoavesActiveX2012Ctrl, "StartServer", StartServer, VT_I4, VTS_NONE)
	DISP_FUNCTION(CMy5LoavesActiveX2012Ctrl, "StopServer", StopServer, VT_I4, VTS_NONE)
	DISP_FUNCTION(CMy5LoavesActiveX2012Ctrl, "SetConfigFile", SetConfigFile, VT_I4, VTS_BSTR)
	DISP_FUNCTION(CMy5LoavesActiveX2012Ctrl, "SetConfigEntry", SetConfigEntry, VT_I4, VTS_BSTR VTS_BSTR VTS_BSTR)
	DISP_FUNCTION(CMy5LoavesActiveX2012Ctrl, "GetLast5LoavesError", GetLast5LoavesError, VT_BSTR, VTS_NONE)
	//}}AFX_DISPATCH_MAP
END_DISPATCH_MAP()

// Event map

BEGIN_EVENT_MAP(CMy5LoavesActiveX2012Ctrl, COleControl)
END_EVENT_MAP()

// Property pages

// TODO: Add more property pages as needed.  Remember to increase the count!
BEGIN_PROPPAGEIDS(CMy5LoavesActiveX2012Ctrl, 1)
	PROPPAGEID(CMy5LoavesActiveX2012PropPage::guid)
END_PROPPAGEIDS(CMy5LoavesActiveX2012Ctrl)

// Initialize class factory and guid

IMPLEMENT_OLECREATE_EX(CMy5LoavesActiveX2012Ctrl, "MY5LOAVESACTIVEX.My5LoavesActiveXCtrl.1",
	0x76f8be7a, 0xa345, 0x46e7, 0x89, 0x4a, 0x7f, 0x5c, 0xf9, 0xfd, 0xd9, 0x47)

// Type library ID and version

IMPLEMENT_OLETYPELIB(CMy5LoavesActiveX2012Ctrl, _tlid, _wVerMajor, _wVerMinor)

// Interface IDs

const IID IID_DMy5LoavesActiveX2012 = { 0x7734809C, 0x20CA, 0x40B9, { 0xBC, 0xB0, 0xC4, 0xAF, 0x23, 0xBA, 0xE9, 0x5B } };
const IID IID_DMy5LoavesActiveX2012Events = { 0xC53F7465, 0x3DD2, 0x450C, { 0xAE, 0x6C, 0xB0, 0x61, 0x54, 0x6D, 0xF9, 0x25 } };

// Control type information

static const DWORD _dwMy5LoavesActiveX2012OleMisc =
	OLEMISC_INVISIBLEATRUNTIME |
	OLEMISC_ACTIVATEWHENVISIBLE |
	OLEMISC_SETCLIENTSITEFIRST |
	OLEMISC_INSIDEOUT |
	OLEMISC_CANTLINKINSIDE |
	OLEMISC_RECOMPOSEONRESIZE;

IMPLEMENT_OLECTLTYPE(CMy5LoavesActiveX2012Ctrl, IDS_MY5LOAVESACTIVEX2012, _dwMy5LoavesActiveX2012OleMisc)

// CMy5LoavesActiveX2012Ctrl::CMy5LoavesActiveX2012CtrlFactory::UpdateRegistry -
// Adds or removes system registry entries for CMy5LoavesActiveX2012Ctrl

BOOL CMy5LoavesActiveX2012Ctrl::CMy5LoavesActiveX2012CtrlFactory::UpdateRegistry(BOOL bRegister)
{
	// TODO: Verify that your control follows apartment-model threading rules.
	// Refer to MFC TechNote 64 for more information.
	// If your control does not conform to the apartment-model rules, then
	// you must modify the code below, changing the 6th parameter from
	// afxRegApartmentThreading to 0.

	if (bRegister)
		return AfxOleRegisterControlClass(
			AfxGetInstanceHandle(),
			m_clsid,
			m_lpszProgID,
			IDS_MY5LOAVESACTIVEX2012,
			IDB_MY5LOAVESACTIVEX2012,
			afxRegApartmentThreading,
			_dwMy5LoavesActiveX2012OleMisc,
			_tlid,
			_wVerMajor,
			_wVerMinor);
	else
		return AfxOleUnregisterClass(m_clsid, m_lpszProgID);
}


// CMy5LoavesActiveX2012Ctrl::CMy5LoavesActiveX2012Ctrl - Constructor

CMy5LoavesActiveX2012Ctrl::CMy5LoavesActiveX2012Ctrl()
{
	InitializeIIDs(&IID_DMy5LoavesActiveX2012, &IID_DMy5LoavesActiveX2012Events);
	// TODO: Initialize your control's instance data here.
}

// CMy5LoavesActiveX2012Ctrl::~CMy5LoavesActiveX2012Ctrl - Destructor

CMy5LoavesActiveX2012Ctrl::~CMy5LoavesActiveX2012Ctrl()
{
	// TODO: Cleanup your control's instance data here.
}

// CMy5LoavesActiveX2012Ctrl::OnDraw - Drawing function

void CMy5LoavesActiveX2012Ctrl::OnDraw(
			CDC* pdc, const CRect& rcBounds, const CRect& rcInvalid)
{
	if (!pdc)
		return;

	// TODO: Replace the following code with your own drawing code.
	pdc->FillRect(rcBounds, CBrush::FromHandle((HBRUSH)GetStockObject(WHITE_BRUSH)));
	pdc->Ellipse(rcBounds);
}

// CMy5LoavesActiveX2012Ctrl::DoPropExchange - Persistence support

void CMy5LoavesActiveX2012Ctrl::DoPropExchange(CPropExchange* pPX)
{
	ExchangeVersion(pPX, MAKELONG(_wVerMinor, _wVerMajor));
	COleControl::DoPropExchange(pPX);

	// TODO: Call PX_ functions for each persistent custom property.
}


// CMy5LoavesActiveX2012Ctrl::OnResetState - Reset control to default state

void CMy5LoavesActiveX2012Ctrl::OnResetState()
{
	COleControl::OnResetState();  // Resets defaults found in DoPropExchange

	// TODO: Reset any other control state here.
}


// CMy5LoavesActiveX2012Ctrl message handlers
/////////////////////////////////////////////////////////////////////////////
// C5LoavesCtrl message handlers

int server_start(const char *);
long CMy5LoavesActiveX2012Ctrl::StartServer() 
{
	if (!isRunning)
	{
		try
		{
			TCHAR pzName[512];
			memset(pzName,0,512);
			GetModuleFileName(NULL,pzName,512);

			GString strStartupMessage;
			strStartupMessage << "5Loaves Windows ActiveX [" << pzName << "]";


			int nRet = server_start(strStartupMessage);
			if (nRet)
				isRunning = 1;
			return 1;
		}
		catch ( GException &e)
		{
			if (e.GetError() == 2)		
			{
				g_strLastError = "Startup Password required";
			}
			else
			{
				g_strLastError = e.GetDescription();
			}
			return 0;
		}
		catch ( ... )
		{
			g_strLastError = "Unknown startup error";
			return 0;
		}
	}
	return 1;
}

void server_stop();
long CMy5LoavesActiveX2012Ctrl::StopServer() 
{
	try
	{
		server_stop();
	}
	catch ( GException &e)
	{
		g_strLastError = e.GetDescription();
		return 0;
	}
	catch(...)
	{
		g_strLastError = "Unknown shutdown error";
		return 0;
	}
	isRunning = 0;
	return 1;
}

long CMy5LoavesActiveX2012Ctrl::SetConfigFile(LPCTSTR StartConfigFile) 
{
	try
	{
		int nLen = lstrlen(StartConfigFile);
		char *lpsz = new char[nLen+1];
	    AfxW2AHelper(lpsz, StartConfigFile, nLen);

		SetProfile(new GProfile(lpsz,0));

		delete lpsz;
	}
	catch ( GException &e )
	{
		g_strLastError = e.GetDescription();
		return 0;
	}
	catch(...)
	{
		g_strLastError = "Unknown error";
		return 0;
	}
	return 1;
}

long CMy5LoavesActiveX2012Ctrl::SetConfigEntry(LPCTSTR Section, LPCTSTR Entry, LPCTSTR Value) 
{
	try
	{
		char szSection[256];
		char szEntry[256];
		char szValue[256];
	    AfxW2AHelper(szSection, Section, lstrlen(Section));
	    AfxW2AHelper(szEntry, Entry, lstrlen(Entry));
	    AfxW2AHelper(szValue, Value, lstrlen(Value));




		GetProfile().SetConfig(szSection, szEntry, szValue);
	}
	catch ( GException &e )
	{
		g_strLastError = e.GetDescription();
		return 0;
	}
	catch(...)
	{
		g_strLastError = "Unknown shutdown error";
		return 0;
	}
	return 1;
}

BSTR CMy5LoavesActiveX2012Ctrl::GetLast5LoavesError() 
{
	CString str(g_strLastError.Buf());
	return str.AllocSysString();
}
