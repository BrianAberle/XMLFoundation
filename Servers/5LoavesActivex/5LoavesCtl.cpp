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
// 5LoavesCtl.cpp : Implementation of the C5LoavesCtrl ActiveX Control class.

#include "stdafx.h"
#include "5LoavesActivex.h"
#include "5LoavesCtl.h"
#include "5LoavesPpg.h"



//#include "..\Core\MsgEnglish.cpp"
#include "GProfile.h"
#include "GException.h"
GString g_strLastError;
int isRunning=0;


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_DYNCREATE(C5LoavesCtrl, COleControl)


/////////////////////////////////////////////////////////////////////////////
// Message map

BEGIN_MESSAGE_MAP(C5LoavesCtrl, COleControl)
	//{{AFX_MSG_MAP(C5LoavesCtrl)
	// NOTE - ClassWizard will add and remove message map entries
	//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG_MAP
	ON_OLEVERB(AFX_IDS_VERB_EDIT, OnEdit)
	ON_OLEVERB(AFX_IDS_VERB_PROPERTIES, OnProperties)
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// Dispatch map

BEGIN_DISPATCH_MAP(C5LoavesCtrl, COleControl)
	//{{AFX_DISPATCH_MAP(C5LoavesCtrl)
	DISP_FUNCTION(C5LoavesCtrl, "StartServer", StartServer, VT_I4, VTS_NONE)
	DISP_FUNCTION(C5LoavesCtrl, "StopServer", StopServer, VT_I4, VTS_NONE)
	DISP_FUNCTION(C5LoavesCtrl, "SetConfigFile", SetConfigFile, VT_I4, VTS_BSTR)
	DISP_FUNCTION(C5LoavesCtrl, "SetConfigEntry", SetConfigEntry, VT_I4, VTS_BSTR VTS_BSTR VTS_BSTR)
	DISP_FUNCTION(C5LoavesCtrl, "GetLast5LoavesError", GetLast5LoavesError, VT_BSTR, VTS_NONE)
	//}}AFX_DISPATCH_MAP
END_DISPATCH_MAP()


/////////////////////////////////////////////////////////////////////////////
// Event map

BEGIN_EVENT_MAP(C5LoavesCtrl, COleControl)
	//{{AFX_EVENT_MAP(C5LoavesCtrl)
	// NOTE - ClassWizard will add and remove event map entries
	//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_EVENT_MAP
END_EVENT_MAP()


/////////////////////////////////////////////////////////////////////////////
// Property pages

// TODO: Add more property pages as needed.  Remember to increase the count!
BEGIN_PROPPAGEIDS(C5LoavesCtrl, 1)
	PROPPAGEID(C5LoavesPropPage::guid)
END_PROPPAGEIDS(C5LoavesCtrl)


/////////////////////////////////////////////////////////////////////////////
// Initialize class factory and guid

IMPLEMENT_OLECREATE_EX(C5LoavesCtrl, "5LOAVES.5LoavesCtrl.1",
	0x3f917a51, 0x66b, 0x4ad7, 0x99, 0xd2, 0x95, 0x34, 0x9d, 0xa3, 0xc1, 0x76)


/////////////////////////////////////////////////////////////////////////////
// Type library ID and version

IMPLEMENT_OLETYPELIB(C5LoavesCtrl, _tlid, _wVerMajor, _wVerMinor)


/////////////////////////////////////////////////////////////////////////////
// Interface IDs

const IID BASED_CODE IID_D5Loaves =
		{ 0xc947f177, 0xcaa4, 0x4a05, { 0xab, 0xcc, 0x94, 0xb0, 0xb4, 0xce, 0x3e, 0x3a } };
const IID BASED_CODE IID_D5LoavesEvents =
		{ 0xc55cb491, 0x48f6, 0x4161, { 0xad, 0xde, 0xc0, 0xce, 0x8b, 0x30, 0xb9, 0xe3 } };


/////////////////////////////////////////////////////////////////////////////
// Control type information

static const DWORD BASED_CODE _dw5LoavesOleMisc =
	OLEMISC_INVISIBLEATRUNTIME |
	OLEMISC_SETCLIENTSITEFIRST |
	OLEMISC_INSIDEOUT |
	OLEMISC_CANTLINKINSIDE |
	OLEMISC_RECOMPOSEONRESIZE;

IMPLEMENT_OLECTLTYPE(C5LoavesCtrl, IDS_5LOAVES, _dw5LoavesOleMisc)


/////////////////////////////////////////////////////////////////////////////
// C5LoavesCtrl::C5LoavesCtrlFactory::UpdateRegistry -
// Adds or removes system registry entries for C5LoavesCtrl

BOOL C5LoavesCtrl::C5LoavesCtrlFactory::UpdateRegistry(BOOL bRegister)
{
	// TODO: Verify that your control follows apartment-model threading rules.
	// Refer to MFC TechNote 64 for more information.
	// If your control does not conform to the apartment-model rules, then
	// you must modify the code below, changing the 6th parameter from
	// afxRegInsertable | afxRegApartmentThreading to afxRegInsertable.

	if (bRegister)
		return AfxOleRegisterControlClass(
			AfxGetInstanceHandle(),
			m_clsid,
			m_lpszProgID,
			IDS_5LOAVES,
			IDB_5LOAVES,
			afxRegInsertable | afxRegApartmentThreading,
			_dw5LoavesOleMisc,
			_tlid,
			_wVerMajor,
			_wVerMinor);
	else
		return AfxOleUnregisterClass(m_clsid, m_lpszProgID);
}


/////////////////////////////////////////////////////////////////////////////
// C5LoavesCtrl::C5LoavesCtrl - Constructor

C5LoavesCtrl::C5LoavesCtrl()
{
	InitializeIIDs(&IID_D5Loaves, &IID_D5LoavesEvents);
//	SetErrorDescriptions(pzBoundErrorDescriptions);
}


/////////////////////////////////////////////////////////////////////////////
// C5LoavesCtrl::~C5LoavesCtrl - Destructor

C5LoavesCtrl::~C5LoavesCtrl()
{
}


/////////////////////////////////////////////////////////////////////////////
// C5LoavesCtrl::OnDraw - Drawing function

void C5LoavesCtrl::OnDraw(
			CDC* pdc, const CRect& rcBounds, const CRect& rcInvalid)
{
	pdc->FillRect(rcBounds, CBrush::FromHandle((HBRUSH)GetStockObject(WHITE_BRUSH)));
	pdc->Ellipse(rcBounds);
}


/////////////////////////////////////////////////////////////////////////////
// C5LoavesCtrl::DoPropExchange - Persistence support

void C5LoavesCtrl::DoPropExchange(CPropExchange* pPX)
{
	ExchangeVersion(pPX, MAKELONG(_wVerMinor, _wVerMajor));
	COleControl::DoPropExchange(pPX);

	// TODO: Call PX_ functions for each persistent custom property.

}


/////////////////////////////////////////////////////////////////////////////
// C5LoavesCtrl::OnResetState - Reset control to default state

void C5LoavesCtrl::OnResetState()
{
	COleControl::OnResetState();  // Resets defaults found in DoPropExchange

	// TODO: Reset any other control state here.
}


/////////////////////////////////////////////////////////////////////////////
// C5LoavesCtrl message handlers

int server_start(const char *);
long C5LoavesCtrl::StartServer() 
{
	if (!isRunning)
	{
		try
		{
			char pzName[512];
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
long C5LoavesCtrl::StopServer() 
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

long C5LoavesCtrl::SetConfigFile(LPCTSTR StartConfigFile) 
{
	try
	{
		SetProfile(new GProfile(StartConfigFile,""));
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

long C5LoavesCtrl::SetConfigEntry(LPCTSTR Section, LPCTSTR Entry, LPCTSTR Value) 
{
	try
	{
		GetProfile().SetConfig(Section, Entry, Value);
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

BSTR C5LoavesCtrl::GetLast5LoavesError() 
{
	CString str(g_strLastError);
	return str.AllocSysString();
}
