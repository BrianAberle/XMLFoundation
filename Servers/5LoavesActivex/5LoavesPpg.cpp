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
// 5LoavesPpg.cpp : Implementation of the C5LoavesPropPage property page class.

#include "stdafx.h"
#include "5LoavesActivex.h"
#include "5LoavesPpg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_DYNCREATE(C5LoavesPropPage, COlePropertyPage)


/////////////////////////////////////////////////////////////////////////////
// Message map

BEGIN_MESSAGE_MAP(C5LoavesPropPage, COlePropertyPage)
	//{{AFX_MSG_MAP(C5LoavesPropPage)
	// NOTE - ClassWizard will add and remove message map entries
	//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// Initialize class factory and guid

IMPLEMENT_OLECREATE_EX(C5LoavesPropPage, "5LOAVES.5LoavesPropPage.1",
	0x540541ec, 0xcba1, 0x47f7, 0x96, 0xaf, 0x8d, 0x4d, 0xe4, 0xe, 0x47, 0xa1)


/////////////////////////////////////////////////////////////////////////////
// C5LoavesPropPage::C5LoavesPropPageFactory::UpdateRegistry -
// Adds or removes system registry entries for C5LoavesPropPage

BOOL C5LoavesPropPage::C5LoavesPropPageFactory::UpdateRegistry(BOOL bRegister)
{
	if (bRegister)
		return AfxOleRegisterPropertyPageClass(AfxGetInstanceHandle(),
			m_clsid, IDS_5LOAVES_PPG);
	else
		return AfxOleUnregisterClass(m_clsid, NULL);
}


/////////////////////////////////////////////////////////////////////////////
// C5LoavesPropPage::C5LoavesPropPage - Constructor

C5LoavesPropPage::C5LoavesPropPage() :
	COlePropertyPage(IDD, IDS_5LOAVES_PPG_CAPTION)
{
	//{{AFX_DATA_INIT(C5LoavesPropPage)
	// NOTE: ClassWizard will add member initialization here
	//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA_INIT
}


/////////////////////////////////////////////////////////////////////////////
// C5LoavesPropPage::DoDataExchange - Moves data between page and properties

void C5LoavesPropPage::DoDataExchange(CDataExchange* pDX)
{
	//{{AFX_DATA_MAP(C5LoavesPropPage)
	// NOTE: ClassWizard will add DDP, DDX, and DDV calls here
	//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA_MAP
	DDP_PostProcessing(pDX);
}


/////////////////////////////////////////////////////////////////////////////
// C5LoavesPropPage message handlers
