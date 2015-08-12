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
// XMLDialogDlg.cpp : implementation file
//

#include "stdafx.h"
#include "XMLDialog.h"
#include "XMLDialogDlg.h"
#include "ObjectModel.h"
#include "GException.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CXMLDialogDlg dialog

IMPLEMENT_FACTORY(CXMLDialogDlg,		Customer)

void CXMLDialogDlg::MapXMLTagsToMembers()
{
	MapMember(&m_strName,			"ContactName",	&gC);
	MapMember(&m_strCity,			"City",			&gC);
	MapMember(&m_strCountry,		"Country",		&gC);
	MapMember(&m_lstOrders,			MyOrder::GetStaticTag());
}



CXMLDialogDlg::CXMLDialogDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CXMLDialogDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CXMLDialogDlg)
	m_strCity = _T("");
	m_strCountry = _T("");
	m_strName = _T("");
	m_strRichEditXML = _T("");
	m_nProductID = 0;
	m_strPrice = _T("");
	m_strDesc = _T("");
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);

	
	ModifyObjectBehavior(SUBOBJECT_UPDATE_NOTIFY);
}

void CXMLDialogDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CXMLDialogDlg)
	DDX_Control(pDX, IDC_LIST_LI, m_ListLineItems);
	DDX_Control(pDX, IDC_LIST_ORDERS, m_List);
	DDX_Text(pDX, IDC_EDT_CITY, m_strCity);
	DDX_Text(pDX, IDC_EDT_COUNTRY, m_strCountry);
	DDX_Text(pDX, IDC_EDT_NAME, m_strName);
	DDX_Text(pDX, IDC_RICHEDIT_XML, m_strRichEditXML);
	DDX_Text(pDX, IDC_EDT_PROD_ID, m_nProductID);
	DDX_Text(pDX, IDC_EDT_PRICE, m_strPrice);
	DDX_Text(pDX, IDC_EDT_DESC, m_strDesc);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CXMLDialogDlg, CDialog)
	//{{AFX_MSG_MAP(CXMLDialogDlg)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BTN_LOADGUI, OnBtnLoadgui)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST_ORDERS, OnItemchangedListOrders)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST_LI, OnItemchangedListLi)
	ON_EN_CHANGE(IDC_EDT_PROD_ID, OnChangeLI)
	ON_BN_CLICKED(IDC_BTN_MAKEXML, OnBtnMakexml)
	ON_EN_CHANGE(IDC_EDT_DESC, OnChangeLI)
	ON_EN_CHANGE(IDC_EDT_PRICE, OnChangeLI)
	ON_BN_CLICKED(IDC_BTN_ADD, OnBtnAdd)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CXMLDialogDlg message handlers


////////////////////////////////////////////////////
// Order list
#define NUM_LIST_COLUMNS	2
static _TCHAR *_gszColumnLabel[NUM_LIST_COLUMNS] =
{
	_T("Ship Date"), _T("Order Date")
};
static int _gnColumnWidth[NUM_LIST_COLUMNS] =
{
	120, 120
};
////////////////////////////////////////////////////
// Line Item list
#define NUM_LIST_COLUMNS_LI		3
static _TCHAR *_gszColumnLabelLI[NUM_LIST_COLUMNS_LI] =
{
	_T("ID"), _T("Price"), _T("Desc")
};
static int _gnColumnWidthLI[NUM_LIST_COLUMNS_LI] =
{
	70, 70, 100
};
////////////////////////////////////////////////////



char pXML2[] = 
"<Customer>\r\n"
	"\t<ContactName>New Dude</ContactName>\r\n"
	"\t<City>Chocolate City</City>\r\n"
	"\t<Country>Hipville</Country>\r\n"
	"\t<Order>\r\n"
		"\t\t<ShippedDate>1997-09-02</ShippedDate>\r\n"
		"\t\t<OrderDate>1997-08-25</OrderDate>\r\n"
		"\t\t<LineItem>\r\n"
			"\t\t\t<UnitPrice>45.6000</UnitPrice>\r\n"
			"\t\t\t<ProductID>28</ProductID>\r\n"
			"\t\t\t<Description/>\r\n"
		"\t\t</LineItem>\r\n"
		"\t\t<LineItem>\r\n"
			"\t\t\t<UnitPrice>18.0000</UnitPrice>\r\n"
			"\t\t\t<ProductID>39</ProductID>\r\n"
			"\t\t\t<Description/>\r\n"
		"\t\t</LineItem>\r\n"
		"\t\t<LineItem>\r\n"
			"\t\t\t<UnitPrice>12.0000</UnitPrice>\r\n"
			"\t\t\t<ProductID>46</ProductID>\r\n"
			"\t\t\t<Description/>\r\n"
		"\t\t</LineItem>\r\n"
	"\t</Order>\r\n"
	"\t<Order>\r\n"
		"\t\t<ShippedDate>Futuristic</ShippedDate>\r\n"
		"\t\t<OrderDate>Tomorrow</OrderDate>\r\n"
		"\t\t<LineItem>\r\n"
			"\t\t\t<UnitPrice>1234567.77</UnitPrice>\r\n"
			"\t\t\t<ProductID>1234567</ProductID>\r\n"
			"\t\t\t<Description/>\r\n"
		"\t\t</LineItem>\r\n"
	"\t</Order>\r\n"
"</Customer>\r\n";



BOOL CXMLDialogDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon
	
	LV_COLUMN lvc;
	lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
	int i;
	for(i = 0; i<NUM_LIST_COLUMNS; i++) 
	{
		lvc.iSubItem = i;
		lvc.pszText = _gszColumnLabel[i];
		lvc.cx = _gnColumnWidth[i];
		lvc.fmt = LVCFMT_LEFT;
		m_List.InsertColumn(i,&lvc);
	}

	for(i = 0; i<NUM_LIST_COLUMNS_LI; i++) 
	{
		lvc.iSubItem = i;
		lvc.pszText = _gszColumnLabelLI[i];
		lvc.cx = _gnColumnWidthLI[i];
		lvc.fmt = LVCFMT_LEFT;
		m_ListLineItems.InsertColumn(i,&lvc);
	}

	m_strRichEditXML = pXML2;
	UpdateData(FALSE);
	
	return TRUE;  // return TRUE  unless you set the focus to a control
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.
void CXMLDialogDlg::OnPaint() 
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

// The system calls this to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CXMLDialogDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}


void CXMLDialogDlg::ParseRawXML()
{
	try
	{
		FromXML(m_strRichEditXML);
	}
	catch (GException &ex)
	{
		// the exception only contains a general parsing error, we can optionally add specific info about the tast that failed
		ex.AddErrorDetail("Failed to parse raw XML in CXMLDialogDlg");
		throw ex;
	}
}

void CXMLDialogDlg::OnBtnLoadgui() 
{
	m_List.DeleteAllItems();

	// this gets the freehand XML into m_strRichEditXML
	UpdateData(TRUE); 

	try
	{
		ParseRawXML();

		// note:UpdateData() does not push the list of 'Orders'[m_lstOrders] into the ListCtrl.  The simplest 
		// way is to iterate [m_lstOrders] that contains the 'Orders' after the call to FromXML() is complete.
		// 
		// This shows you the (more complex?), SAXish like, way that will add 'Order' objects to the GUI as they are 
		// added to [m_lstOrders] by the Object Factory during the call to FromXML().  This requires adding 
		// this code to this's constructor:    ModifyObjectBehavior(SUBOBJECT_UPDATE_NOTIFY);
		// This causes the XMLFoundation to call ObjectMessage(), as each "Order" gets it's data from the XML
		// This Actually adds data to the GUI DURING THE PARSING PROCESS, as opposed to the 'simplest' way that
		// will add the data into the CListCtrl AFTER the parsing process.

	}
	catch(GException &ex)
	{
#ifdef _UNICODE
		AfxMessageBox(ex.GetDescriptionUnicode());
		AfxMessageBox(ex.GetStackAsStringUnicode());
		AfxMessageBox(ex.ToXMLUnicode());
//		GString g(ex.ToXMLUnicode());
//		g.ToFile("c:\\XMLFoundation\\XXX64.txt");
#else
		AfxMessageBox(ex.GetDescription());
		AfxMessageBox(ex.GetStackAsString());
		AfxMessageBox(ex.ToXML());
//		GString g(ex.ToXML());
//		g.ToFile("c:\\XMLFoundation\\ XXX.txt");
#endif
	}
	UpdateData(FALSE);
}


void *CXMLDialogDlg::ObjectMessage( int nCase, const char *pzArg1, const char *pzArg2, __int64 nArg3, void *pArg4 )
{
	if(nCase == MSG_SUBOBJECT_UPDATE)	// there are various messages make sure we have the right one
	{
		XMLObject *p = (XMLObject *)pArg4; 
		GString strObjType(p->GetObjectType());
		
		// we only map one sub object type from Customer so this is only done for the sake of
		// providing an example of how to distinguish between multiple subobject types if there were any
		if(strObjType.Compare("MyOrder") == 0) 
		{
			// Now that we know for sure what we are dealing with we can cast it up
			MyOrder *pO = (MyOrder *)p;

//			int nItemIndex = m_List.InsertItem(LVIF_IMAGE|LVIF_TEXT|LVIF_STATE|LVIF_PARAM,
	//							  0, pO->m_strOrderDate, LVIS_FOCUSED|LVIS_SELECTED, LVIS_FOCUSED|LVIS_SELECTED, 0, (long)pO);

			int nItemIndex = m_List.InsertItem(LVIF_IMAGE | LVIF_TEXT | LVIF_STATE | LVIF_PARAM,
				0, pO->m_strOrderDate, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED, 0, (LPARAM)pO);

			m_List.SetItemText(nItemIndex, 1, pO->m_strShippedDate);

			// uncomment this to see that it works
			//AfxMessageBox(pO->ToXML());		// call base class method
			//AfxMessageBox(pO->m_strOrderDate);// directly access derived class data

		}
	}
	return 0;
}

void CXMLDialogDlg::LoadLineItems() 
{
	m_ListLineItems.DeleteAllItems();

	LVITEM itm;
	memset(&itm,0,sizeof(LVITEM));  itm.iItem = -1;  itm.mask = LVIF_TEXT;
	itm.iItem = m_List.GetNextItem(itm.iItem, LVNI_SELECTED);
	if (itm.iItem != -1)
	{
		MyOrder *pO = (MyOrder *)m_List.GetItemData(itm.iItem);
		
		GListIterator it(&pO->m_lstLineItems);
		while(it())
		{
			MyOrderLineItem *pLI = (MyOrderLineItem *)it++;


// Note: the "g" code, replaced the "buf" code because it also compiles in a Unicode build			
//			char buf[10];
			GString g;
			g << pLI->m_nProductID;
			int nItemIndex = m_ListLineItems.InsertItem(LVIF_IMAGE|LVIF_TEXT|LVIF_STATE|LVIF_PARAM, 0, 
				// itoa(pLI->m_nProductID,buf,10), 
				g,
				LVIS_FOCUSED|LVIS_SELECTED, LVIS_FOCUSED|LVIS_SELECTED, 0, (LPARAM)pLI);

			m_ListLineItems.SetItemText(nItemIndex, 1, pLI->m_strUnitPrice);
			m_ListLineItems.SetItemText(nItemIndex, 2, pLI->m_strDescription);
		}
		
	}
}




void CXMLDialogDlg::OnItemchangedListOrders(NMHDR* pNMHDR, LRESULT* pResult) 
{
	LoadLineItems();
	*pResult = 0;
}

void CXMLDialogDlg::OnItemchangedListLi(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;

	LVITEM itm;
	memset(&itm,0,sizeof(LVITEM));  itm.iItem = -1;  itm.mask = LVIF_TEXT;
	itm.iItem = m_ListLineItems.GetNextItem(itm.iItem, LVNI_SELECTED);
	if (itm.iItem != -1)
	{
		MyOrderLineItem *pLI = (MyOrderLineItem *)m_ListLineItems.GetItemData(itm.iItem);
		m_nProductID = pLI->m_nProductID;
		m_strPrice = (const char *)pLI->m_strUnitPrice;
		m_strDesc = (const char *)pLI->m_strDescription;
		UpdateData(FALSE);
	}
	

	*pResult = 0;
}

void CXMLDialogDlg::OnChangeLI() 
{
	UpdateData(TRUE)	;
	LVITEM itm;
	memset(&itm,0,sizeof(LVITEM));  itm.iItem = -1;  itm.mask = LVIF_TEXT;
	itm.iItem = m_ListLineItems.GetNextItem(itm.iItem, LVNI_SELECTED);
	if (itm.iItem != -1)
	{
		MyOrderLineItem *pLI = (MyOrderLineItem *)m_ListLineItems.GetItemData(itm.iItem);
		pLI->m_nProductID = m_nProductID;
		pLI->m_strUnitPrice.Empty();
		pLI->m_strDescription.Empty();
		pLI->m_strUnitPrice << m_strPrice;
		pLI->m_strDescription << m_strDesc;
		LoadLineItems();
	}

}

void CXMLDialogDlg::OnBtnMakexml() 
{
	UpdateData(TRUE);			 // pickup the changes from the GUI into the member variables
	m_strRichEditXML = ToXML();  // create the new XML
	UpdateData(FALSE);           // display the new XML in the edit box
}



void CXMLDialogDlg::OnBtnAdd() 
{
	
	// Get the selected Order
	MyOrder *pSelectedOrder = 0;
	LVITEM itm;
	memset(&itm,0,sizeof(LVITEM));  itm.iItem = -1;  itm.mask = LVIF_TEXT;
	itm.iItem = m_List.GetNextItem(itm.iItem, LVNI_SELECTED);
	if (itm.iItem != -1)
	{
		pSelectedOrder = (MyOrder *)m_List.GetItemData(itm.iItem);
	}


	if (pSelectedOrder)
	{

		// Create the new object - assign the data
		MyOrderLineItem *pLI = new MyOrderLineItem();
		pLI->m_nProductID = 777;
		pLI->m_strUnitPrice = "7.77";
		pLI->m_strDescription = "New from the GUI";
		
		// add it to the Order		
		pSelectedOrder->m_lstLineItems.AddLast(pLI);

		// add it to the GUI
//		char buf[10];
		GString g;
		g << pLI->m_nProductID;

		int nItemIndex = m_ListLineItems.InsertItem(LVIF_IMAGE|LVIF_TEXT|LVIF_STATE|LVIF_PARAM, 0, 
		//itoa(pLI->m_nProductID,buf,10), 
		g,LVIS_FOCUSED|LVIS_SELECTED, LVIS_FOCUSED|LVIS_SELECTED, 0, (LPARAM)pLI);
		m_ListLineItems.SetItemText(nItemIndex, 1, pLI->m_strUnitPrice);
		m_ListLineItems.SetItemText(nItemIndex, 2, pLI->m_strDescription);

	}		

	
}
