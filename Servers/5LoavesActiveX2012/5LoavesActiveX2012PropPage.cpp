// 5LoavesActiveX2012PropPage.cpp : Implementation of the CMy5LoavesActiveX2012PropPage property page class.

#include "stdafx.h"
#include "5LoavesActiveX2012.h"
#include "5LoavesActiveX2012PropPage.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

IMPLEMENT_DYNCREATE(CMy5LoavesActiveX2012PropPage, COlePropertyPage)

// Message map

BEGIN_MESSAGE_MAP(CMy5LoavesActiveX2012PropPage, COlePropertyPage)
END_MESSAGE_MAP()

// Initialize class factory and guid

IMPLEMENT_OLECREATE_EX(CMy5LoavesActiveX2012PropPage, "MY5LOAVESACTIV.My5LoavesActivPropPage.1",
	0xef1f1f57, 0x18f6, 0x42b0, 0x8c, 0x29, 0xc, 0xfd, 0x8d, 0x87, 0x44, 0x13)

// CMy5LoavesActiveX2012PropPage::CMy5LoavesActiveX2012PropPageFactory::UpdateRegistry -
// Adds or removes system registry entries for CMy5LoavesActiveX2012PropPage

BOOL CMy5LoavesActiveX2012PropPage::CMy5LoavesActiveX2012PropPageFactory::UpdateRegistry(BOOL bRegister)
{
	if (bRegister)
		return AfxOleRegisterPropertyPageClass(AfxGetInstanceHandle(),
			m_clsid, IDS_MY5LOAVESACTIVEX2012_PPG);
	else
		return AfxOleUnregisterClass(m_clsid, NULL);
}

// CMy5LoavesActiveX2012PropPage::CMy5LoavesActiveX2012PropPage - Constructor

CMy5LoavesActiveX2012PropPage::CMy5LoavesActiveX2012PropPage() :
	COlePropertyPage(IDD, IDS_MY5LOAVESACTIVEX2012_PPG_CAPTION)
{
}

// CMy5LoavesActiveX2012PropPage::DoDataExchange - Moves data between page and properties

void CMy5LoavesActiveX2012PropPage::DoDataExchange(CDataExchange* pDX)
{
	DDP_PostProcessing(pDX);
}

// CMy5LoavesActiveX2012PropPage message handlers
