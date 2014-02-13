#include "stdafx.h"

//notice the #include <XMLFoundation.h> in stdafx.h

#include "MFCTypesFromXML.h"
#include "MFCTypesFromXMLDlg.h"
#include "AbstractionsMFC.h"
#include "GPerformanceTimer.h"



////////////////////////////////////////////////////////////////////////////
// Abstraction implementations for native C++ types into MFC data structures
//
CStringAbstraction gC;      // (G)lobal (C)String Abstraction
CStringListAbstraction gCL; // (G)lobal (C)String (L)ist Abstraction
CStringArrayAbstraction gCA;// (G)lobal (C)String (A)rray Abstraction
CDWordArrayAbstraction gDA; // (G)lobal (D)String (A)rray Abstraction
CDoubleArrayAbstraction gAA;
//
////////////////////////////////////////////////////////////////////////////
// Abstraction implementations for XMLObject types into MFC data structures
// You may create these inside the class containing the data structure
// or they may be at the global level  - here or on stdafx.cpp
CPtrArrayAbstraction<CTinyXMLObject>		gTinyPtrArr;
CListAbstraction<CTinyXMLObject>			gTinyCPtrList;
CMapStringToPtrAbstraction<CTinyXMLObject>	gTinyMap;
CArrayAbstraction<CTinyXMLObject *>			gTinyCArr;
GHashAbstraction							gGHA; // (G)lobal (G)(H)ash Abstraction

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

IMPLEMENT_FACTORY(CTinyXMLObject, TinyObject);
IMPLEMENT_FACTORY(CMFCTypesFromXMLDlg, MFCTypesSample);

CTinyXMLObject::CTinyXMLObject()
{
	ModifyObjectBehavior(NO_MEMBER_MAP_PRECOUNT);
}
void CTinyXMLObject::MapXMLTagsToMembers()
{

	// This is non-typical: "Color" as an Element or as an Attribute maps to m_str
	// To support this, we need the NO_MEMBER_MAP_PRECOUNT object behavior
	MapAttribute(&m_str,"Color", &gC);
	MapMember   (&m_str,"Color", &gC);

	MapObjectID("Color",1,"Color",0); // NOTE: this is required to use Keyed Data Structures (GHash and CMapStringToPtr)
}


/////////////////////////////////////////////////////////////////////////////
// CMFCTypesFromXMLDlg dialog
char pzXML1[] = 
"<MFCTypesSample>\r\n"
	"\t<CString>2 of Everything - using Element OID</CString>\t\r\n"
	"\t<CStringList>\r\n"
		 "\t\t<CString>one</CString>\r\n"
		 "\t\t<CString>two</CString>\r\n"
	"\t</CStringList>\r\n"
	"\t<CStringArray>\r\n"
		 "\t\t<CString>first</CString>\r\n"
		 "\t\t<CString>second</CString>\r\n"
	"\t</CStringArray>\r\n"
	"\t<CDWordArray>\r\n"
		 "\t\t<Int>1</Int>\r\n"
		 "\t\t<Int>2</Int>\r\n"
	"\t</CDWordArray>\r\n"
	"\t<CPtrList>\r\n"
		 "\t\t<TinyObject>\r\n"
			"\t\t\t<Color>Red</Color>\r\n"
		 "\t\t</TinyObject>\r\n"
		 "\t\t<TinyObject>\r\n"
			"\t\t\t<Color>Blue</Color>\r\n"
		 "\t\t</TinyObject>\r\n"
	"\t</CPtrList>\r\n"
	"\t<GList>\r\n"
		 "\t\t<TinyObject>\r\n"
			"\t\t\t<Color>White</Color>\r\n"
		 "\t\t</TinyObject>\r\n"
		 "\t\t<TinyObject>\r\n"
			"\t\t\t<Color>Green</Color>\r\n"
		 "\t\t</TinyObject>\r\n"
	"\t</GList>\r\n"
	"\t<GHash>\r\n"
		 "\t\t<TinyObject>\r\n"
			"\t\t\t<Color>Purple</Color>\r\n"
		 "\t\t</TinyObject>\r\n"
		 "\t\t<TinyObject>\r\n"
			"\t\t\t<Color>Rainbow</Color>\r\n"
		 "\t\t</TinyObject>\r\n"
	"\t</GHash>\r\n"
	"\t<CMapStringToPtr>\r\n"
		 "\t\t<TinyObject>\r\n"
			"\t\t\t<Color>Gold</Color>\r\n"
		 "\t\t</TinyObject>\r\n"
		 "\t\t<TinyObject>\r\n"
			"\t\t\t<Color>Silver</Color>\r\n"
		 "\t\t</TinyObject>\r\n"
	"\t</CMapStringToPtr>\r\n"
	"\t<CArray>\r\n"
		 "\t\t<TinyObject>\r\n"
			"\t\t\t<Color>Orange</Color>\r\n"
		 "\t\t</TinyObject>\r\n"
		 "\t\t<TinyObject>\r\n"
			"\t\t\t<Color>Yellow</Color>\r\n"
		 "\t\t</TinyObject>\r\n"
	"\t</CArray>\r\n"
	"\t<CPtrArray>\r\n"
		 "\t\t<TinyObject>\r\n"
			"\t\t\t<Color>Black</Color>\r\n"
		 "\t\t</TinyObject>\r\n"
		 "\t\t<TinyObject>\r\n"
			"\t\t\t<Color>Pink</Color>\r\n"
		 "\t\t</TinyObject>\r\n"
	"\t</CPtrArray>\r\n"
	"\t<CArrayDouble>\r\n"
		 "\t\t<Double>777000777000777000.7</Double>\r\n"
		 "\t\t<Double>11100011100011000.1</Double>\r\n"
	"\t</CArrayDouble>\r\n"
"</MFCTypesSample>";


char pzXML2[] = 
"<MFCTypesSample>\r\n"
	"\t<CString>CStringArray &amp; GHash</CString>\t\r\n"
	"\t<CStringArray>\r\n"
		 "\t\t<CString>A</CString>\r\n"
		 "\t\t<CString>B</CString>\r\n"
		 "\t\t<CString>C</CString>\r\n"
		 "\t\t<CString>D</CString>\r\n"
	"\t</CStringArray>\r\n"
	"\t<GHash>\r\n"
		 "\t\t<TinyObject>\r\n"
			"\t\t\t<Color>Purple</Color>\r\n"
		 "\t\t</TinyObject>\r\n"
		 "\t\t<TinyObject>\r\n"
			"\t\t\t<Color>Rainbow</Color>\r\n"
		 "\t\t</TinyObject>\r\n"
	"\t</GHash>\r\n"
"</MFCTypesSample>";


char pzXML3[] = 
"<MFCTypesSample>\r\n"
	"\t<CString>2 of Everything - using Attribute OID</CString>\t\r\n"
	"\t<CStringList>\r\n"
		 "\t\t<CString>one</CString>\r\n"
		 "\t\t<CString>two</CString>\r\n"
	"\t</CStringList>\r\n"
	"\t<CStringArray>\r\n"
		 "\t\t<CString>first</CString>\r\n"
		 "\t\t<CString>second</CString>\r\n"
	"\t</CStringArray>\r\n"
	"\t<CDWordArray>\r\n"
		 "\t\t<Int>1</Int>\r\n"
		 "\t\t<Int>2</Int>\r\n"
	"\t</CDWordArray>\r\n"
	"\t<CPtrList>\r\n"
		 "\t\t<TinyObject Color='Red'/>\r\n"
		 "\t\t<TinyObject Color='Blue'/>\r\n"
	"\t</CPtrList>\r\n"
	"\t<GList>\r\n"
		 "\t\t<TinyObject Color='White'/>\r\n"
		 "\t\t<TinyObject Color='Green'/>\r\n"
	"\t</GList>\r\n"
	"\t<GHash>\r\n"
		 "\t\t<TinyObject Color='Purple'/>\r\n"
		 "\t\t<TinyObject Color='Rainbow'/>\r\n"
	"\t</GHash>\r\n"
	"\t<CMapStringToPtr>\r\n"
		 "\t\t<TinyObject Color='Gold'/>\r\n"
		 "\t\t<TinyObject Color='Silver'/>\r\n"
	"\t</CMapStringToPtr>\r\n"
	"\t<CArray>\r\n"
		 "\t\t<TinyObject Color='Orange'/>\r\n"
		 "\t\t<TinyObject Color='Yellow'/>\r\n"
	"\t</CArray>\r\n"
	"\t<CPtrArray>\r\n"
		 "\t\t<TinyObject Color='Black'/>\r\n"
		 "\t\t<TinyObject Color='Pink'/>\r\n"
	"\t</CPtrArray>\r\n"
	"\t<CArrayDouble>\r\n"
		 "\t\t<Double>7.77000e+077</Double>\r\n"
		 "\t\t<Double>11100011100011000.1</Double>\r\n"
	"\t</CArrayDouble>\r\n"
"</MFCTypesSample>";



void CMFCTypesFromXMLDlg::MapXMLTagsToMembers()
{

	//	Member Variable				 XML Tag						Abstract		Outer XML Tag
	MapMember(&m_strCStr,			"CString",						&gC);
	MapMember(&m_lstCStrs,			"CString",						&gCL,			"CStringList");
	MapMember(&m_arrCStrs,			"CString",						&gCA,			"CStringArray");
	MapMember(&m_arrDWords,			"Int",							&gDA,			"CDWordArray");

	MapMember(&m_arrDbl,			"Double",						&gAA,			"CArrayDouble");

	MapMember(&m_lstMyObjects,		CTinyXMLObject::GetStaticTag(), &gTinyCPtrList,	"CPtrList");
	MapMember(&m_arrMyObjects,		CTinyXMLObject::GetStaticTag(), &gTinyCArr,		"CArray");
	MapMember(&m_arr2MyObjs,		CTinyXMLObject::GetStaticTag(), &gTinyPtrArr,	"CPtrArray");
	MapMember(&m_glstMyObjects,		CTinyXMLObject::GetStaticTag(),					"GList");

	MapMember(&m_htblTinyObjs,		&gGHA  /*<-->*/,CTinyXMLObject::GetStaticTag(),	"GHash"); 
	MapMember(&m_mapStrToPtr,		&gTinyMap,		CTinyXMLObject::GetStaticTag(),	"CMapStringToPtr"); 

}


CMFCTypesFromXMLDlg::CMFCTypesFromXMLDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CMFCTypesFromXMLDlg::IDD, pParent), m_strCStr(' ',1000000)
	
{
	ModifyObjectBehavior(NO_MEMBER_MAP_PRECOUNT);

	//{{AFX_DATA_INIT(CMFCTypesFromXMLDlg)
	m_strCStr= _T("");;
	m_strCPtrListCount = _T("");
	m_strGListObjCount = _T("");
	m_strArrCount = _T("");
	m_strArr2Count = _T("");
	m_strHashCount = _T("");
	m_strMap2Ptr = _T("");
	m_strCompleteMessage = _T("");
	m_strCompleteMessage2 = _T("");
	m_strCompleteMessage3 = _T("");
	m_strXML2 = _T("");
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CMFCTypesFromXMLDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CMFCTypesFromXMLDlg)
	DDX_Control(pDX, IDC_BTN_XML7, m_btn7);
	DDX_Control(pDX, IDC_BTN_XML6, m_btn6);
	DDX_Control(pDX, IDC_BTN_XML5, m_btn5);
	DDX_Control(pDX, IDC_BTN_XML4, m_btn4);
	DDX_Control(pDX, IDC_BTN_XML3, m_btn3);
	DDX_Control(pDX, IDC_BTN_XML2, m_btn2);
	DDX_Control(pDX, IDC_BTN_XML1, m_btn1);
	DDX_Control(pDX, IDC_LIST3, m_lstboxStrArr);
	DDX_Control(pDX, IDC_LIST2, m_lstBoxArray);
	DDX_Control(pDX, IDC_LIST1, m_lstBox);
	DDX_Text(pDX, IDC_RICHEDIT_XML, m_strXML);
	DDX_Text(pDX, IDC_EDT_CSTR, m_strCStr);
	DDX_Text(pDX, IDC_CPTR_OBJS, m_strCPtrListCount);
	DDX_Text(pDX, IDC_GLST_OBJS, m_strGListObjCount);
	DDX_Text(pDX, IDC_ARR_OBJS, m_strArrCount);
	DDX_Text(pDX, IDC_ARR_OBJS2, m_strArr2Count);
	DDX_Text(pDX, IDC_GHSH_OBJS, m_strHashCount);
	DDX_Text(pDX, IDC_MAP2PTR_OBJS, m_strMap2Ptr);
	DDX_Text(pDX, IDC_COMPLETE_MSG, m_strCompleteMessage);
	DDX_Text(pDX, IDC_COMPLETE_MSG2, m_strCompleteMessage2);
	DDX_Text(pDX, IDC_COMPLETE_MSG3, m_strCompleteMessage3);
	DDX_Text(pDX, IDC_RICHEDIT_XML2, m_strXML2);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CMFCTypesFromXMLDlg, CDialog)
	//{{AFX_MSG_MAP(CMFCTypesFromXMLDlg)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_TOXML, OnToxml)
	ON_BN_CLICKED(IDC_BTN_FIRST_CPTR, OnBtnFirstCptr)
	ON_BN_CLICKED(IDC_BTN_LAST_GLIST, OnBtnLastGlist)
	ON_BN_CLICKED(IDC_BTN_VIEW_CARRAY, OnBtnViewCarray)
	ON_BN_CLICKED(IDC_BTN_VIEW_ARR2, OnBtnViewArr2)
	ON_BN_CLICKED(IDC_BTN_HASH_SEARCH, OnBtnHashSearch)
	ON_BN_CLICKED(IDC_BTN_SEARCH_MAP, OnBtnSearchMap)
	ON_BN_CLICKED(IDC_BTN_XML1, OnBtnXml1)
	ON_BN_CLICKED(IDC_BTN_XML2, OnBtnXml2)
	ON_BN_CLICKED(IDC_BTN_XML3, OnBtnXml3)
	ON_BN_CLICKED(IDC_BTN_XML4, OnBtnXml4)
	ON_BN_CLICKED(IDC_BTN_XML5, OnBtnXml5)
	ON_BN_CLICKED(IDC_BTN_XML6, OnBtnXml6)
	ON_BN_CLICKED(IDC_BTN_XML7, OnBtnXml7)
	ON_WM_CTLCOLOR()
	ON_BN_CLICKED(IDC_BTN_CUSTOM_XML, OnBtnCustomXml)
	ON_BN_CLICKED(IDC_BTN_DEL, OnBtnDel)
	ON_BN_CLICKED(IDC_BTN_DEL2, OnBtnDel2)
	ON_BN_CLICKED(IDC_BTN_DEL3, OnBtnDel3)
	ON_WM_CLOSE()
	ON_BN_CLICKED(IDC_BTN_DEL4, OnBtnDel4)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMFCTypesFromXMLDlg message handlers

BOOL CMFCTypesFromXMLDlg::OnInitDialog()
{
	CDialog::OnInitDialog();


	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon
	
	// TODO: Add extra initialization here
	
	return TRUE;  // return TRUE  unless you set the focus to a control
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CMFCTypesFromXMLDlg::OnPaint() 
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
HCURSOR CMFCTypesFromXMLDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}

void CMFCTypesFromXMLDlg::OnFromXML(const char *pzTestName) 
{
	UpdateData(TRUE);

	// clean the GUI from the last run
	m_lstCStrs.RemoveAll();
	m_arrCStrs.RemoveAll();
	m_arrDWords.RemoveAll();
	m_strCStr = "";

	// Delete Objects in structures held from the last FromXML()
	// This is optional depending on desired behavior.  The data structures may be configured to 
	// eternally append new objects/data, as the (m_lstCStrs/m_arrCStrs/m_arrDWords) would
	// and another may consider the XML an "update" to the objects already instanciated and have 
	// an ObjectID defined as would happen in (m_lstMyObjects/m_arrMyObjects/m_arr2MyObjs/m_strMap2Ptr)
	// Here we start each test with empty data structures.
	m_strCompleteMessage= "Clearing Structures and freeing memory from previous test";
	m_strCompleteMessage2= "";
	m_strCompleteMessage3= "";
	UpdateData(FALSE);
	DeleteObjects(0); 



	BeginWaitCursor();
	m_strCompleteMessage= "Parsing XML / Creating Objs / Adding Obj to DataStructures";
	UpdateData(FALSE);
	GString strError;
	long lBegin = getTimeMS();


	if ( !FromXMLX(m_strXML,&strError) )
	{
		AfxMessageBox(strError);
		return;
	}
	long lElapsed = getTimeMS() - lBegin;
	m_strCompleteMessage.Format("Test[%s] Total Factorization Time:%d milliseconds",pzTestName, lElapsed);
	EndWaitCursor();
	
	// walk the CStringList
	m_lstBox.ResetContent();
	POSITION pos;
	for( pos = m_lstCStrs.GetHeadPosition(); pos != NULL; )
	{
		m_lstBox.AddString( m_lstCStrs.GetNext( pos ) );
	}

	// walk the CStringArray
	m_lstboxStrArr.ResetContent();
	__int64 nItems = m_arrCStrs.GetSize();
	int i;
	for(i=0; i<nItems; i++)
	{
		m_lstboxStrArr.AddString( m_arrCStrs[i] );
	}

	// walk the DWordArray
	m_lstBoxArray.ResetContent();
	nItems = m_arrDWords.GetSize();
	for(i=0; i<nItems; i++)
	{
		CString s;
		s.Format("%d",m_arrDWords[i]);
		m_lstBoxArray.AddString( s );
	}

	m_strCPtrListCount.Format("%d",(int)m_lstMyObjects.GetCount());
	m_strArrCount.Format("%d",(int)m_arrMyObjects.GetSize());
	m_strArr2Count.Format("%d",(int)m_arr2MyObjs.GetSize());
	
	m_strHashCount.Format("%d",(int)m_htblTinyObjs.GetCount());
	m_strGListObjCount.Format("%d",(int)m_glstMyObjects.GetCount());
	m_strMap2Ptr.Format("%d",(int)m_mapStrToPtr.GetCount());

	UpdateData(FALSE);
}

void CMFCTypesFromXMLDlg::OnToxml() 
{
	UpdateData(TRUE);
	
	long lBegin = getTimeMS();
	GString strXML(2000000); // 2 Million byte preallocation
	ToXML(&strXML);
	long lElapsed = getTimeMS() - lBegin;

	// Fast: Push it into the CString
	char *pC = m_strXML2.GetBuffer((int)strXML.Length()+1);
	memcpy((void *)pC,strXML.Buf(),(int)strXML.Length()+1);
	// note:Faster would be to GString::Attach() to a CString::GetBuffer() when CString can be safely preallocated)
	
	m_strXML2.ReleaseBuffer((int)strXML.Length());
	
	GString strAbbrev; strAbbrev << strXML.Length();
	GString strComma; strComma << strXML.Length();
	

	m_strCompleteMessage3.Format("%d ms to create %s bytes(%s) of XML",lElapsed,strComma.CommaNumeric(),strAbbrev.AbbreviateNumeric());
	UpdateData(FALSE);
}

void CMFCTypesFromXMLDlg::DeleteObjects(int bMFCOnly)
{
	BeginWaitCursor();

	//
	// MFC Data Structures
	//
	///////////////////////////////////////////////////////////////////////////////////////////
	// walk the [CPtrList] to delete objects that were added to it by the ObjectFactory
	POSITION pos2;
	for( pos2 = m_lstMyObjects.GetHeadPosition(); pos2 != NULL; )
	{
		CTinyXMLObject *p = (CTinyXMLObject *)m_lstMyObjects.GetNext( pos2 );
		delete p;
		// fyi: if you remove the above line of code, and replace it with the following line.......
		//p->DecRef();
		//
		// Then it has the same effect.  If Using COM/CORBA interfaces, DecRef() is preferrable
		// in the event that the object still has a reference on it elsewhere.
	}
	m_lstMyObjects.RemoveAll(); // <--- IMPORTANT
	///////////////////////////////////////////////////////////////////////////////////////////
	// walk the [CArray] to delete objects that were added to it by the ObjectFactory
	int i;
	for (i=0;i < m_arrMyObjects.GetSize();i++)
	{
		CTinyXMLObject *p = m_arrMyObjects.ElementAt(i);
		delete p; // or p->DecRef();
	}
	m_arrMyObjects.RemoveAll(); // <--- IMPORTANT
	///////////////////////////////////////////////////////////////////////////////////////////
	// walk the [CPtrArray] to delete objects that were added to it by the ObjectFactory
	for (i=0;i < m_arr2MyObjs.GetSize();i++)
	{
		CTinyXMLObject *p = (CTinyXMLObject *)m_arr2MyObjs.ElementAt(i);
		delete p; // or p->DecRef();
	}
	m_arr2MyObjs.RemoveAll(); // <--- IMPORTANT
	///////////////////////////////////////////////////////////////////////////////////////////
	// walk the [CMapStringToPtr] to delete objects that were added to it by the ObjectFactory
	POSITION pos;
	CString key;
	void *pRet = 0;
	for( pos = m_mapStrToPtr.GetStartPosition(); pos != NULL; )
	{
		m_mapStrToPtr.GetNextAssoc( pos, key, pRet );
		delete (CTinyXMLObject *)pRet; // or p->DecRef();
	}
	m_mapStrToPtr.RemoveAll(); // <--- IMPORTANT
	///////////////////////////////////////////////////////////////////////////////////////////

	// As a general rule when mapping MFC data structures to MapMember() you should
	// not only free the objects - but ALSO empty the data structures prior to 
	// destruction of the object derived from XMLObject.  Do not leave garbage pointers in 
	// the data structures at the time ~XMLObject() runs.

	
	//
	// XMLFoundation "G" Data Structures
	//
	///////////////////////////////////////////////////////////////////////////////////////////
	if (!bMFCOnly)
	{
		///////////////////////////////////////////////////////////////////////////////////////////
		// walk the [GHash] to delete objects that were added to it by the ObjectFactory
		GHashIterator it(&m_htblTinyObjs);
		while(it())
		{
			CTinyXMLObject *p = (CTinyXMLObject *)it++;
			p->DecRef();
		}
		m_htblTinyObjs.RemoveAll();

		///////////////////////////////////////////////////////////////////////////////////////////
		// walk the [GList] to delete objects that were added to it by the ObjectFactory
		GListIterator it2(&m_glstMyObjects);
		while(it2())
		{
			CTinyXMLObject *p = (CTinyXMLObject *)it2++;
			p->DecRef();
		}
		m_glstMyObjects.RemoveAll();

	}

	
	EndWaitCursor();
}

CMFCTypesFromXMLDlg::~CMFCTypesFromXMLDlg()
{

	// passing argument [1] deletes objects ONLY from MFC data structures.  Not the GList/GHash
	// You are free to delete manually from the G classes if you prefer.
	DeleteObjects(1); 


	// The question might arise - why didn't we need to clean up the objects in the GList/GHash
	//
	//
	// The base class created these objects, so in many/most cases it is actually
	// more correct from an object-oriented aspect that the base class clean up what
	// the base class created.  In my own applications I choose not to use [CArray] or [CPtrList]
	// to store XMLObject derived objects becase they do not support the auto-cleanup of 
	// objects like GList does.  
	
	// IMHO, GList is better for storing factory created objects in most cases.
	//
	// Check out CListAbstraction::create() in AbstractionsMFC.h to see why the CPtrList 
	// cannot clean up with the neatness that GList can.  Check out how the 
	// GenericListAbstraction class is able to solve the problem using a custom list.
	//
	// This is because C++ creates bottom up and destroys top down but in this situation
	// we want our derived(top) class to contain the storage for our base(bottom) class.
	// As we destroy top down, the list goes out of scope before the base class can
	// iterate it.  GList has the ability to "defer destruction" until after the ~dtor is
	// called by C++ scoping rules.

}

void CMFCTypesFromXMLDlg::OnBtnFirstCptr() 
{
	POSITION pos;
	pos = m_lstMyObjects.GetHeadPosition();
	if (pos)
	{
		CTinyXMLObject *p = (CTinyXMLObject *)m_lstMyObjects.GetNext( pos );
		if (p)
			AfxMessageBox(p->ToXML());
	}
}

void CMFCTypesFromXMLDlg::OnBtnLastGlist() 
{
	CTinyXMLObject *p = (CTinyXMLObject *)m_glstMyObjects.Last();
	if (p)
		AfxMessageBox(p->ToXML());

}

void CMFCTypesFromXMLDlg::OnBtnViewCarray() 
{
	GString GS;
	for (int i=0;i < m_arrMyObjects.GetSize();i++)
	{
		CTinyXMLObject *p = m_arrMyObjects.ElementAt(i);
		GS << p->ToXML() << "\r\n";
	}
	AfxMessageBox(GS);
}



void CMFCTypesFromXMLDlg::OnBtnViewArr2() 
{
	GString GS;
	for (int i=0;i < m_arr2MyObjs.GetSize();i++)
	{
		CTinyXMLObject *p = (CTinyXMLObject *)m_arr2MyObjs.ElementAt(i);
		GS << p->ToXML() << "\r\n";
	}
	AfxMessageBox(GS);
		
}


void CMFCTypesFromXMLDlg::OnBtnHashSearch() 
{
// To search, we need to OID.  
// It is strangely defined as the Attribute 'Color' + the Element 'Color'.  
// That perhaps is a poor example for keying your data, it is unusual feature documentation.
// so to search for it we need to duplicate the value of color back to back

// color"2890-777" becomes key 2890-7772890-777
	CTinyXMLObject *p;
	p = (CTinyXMLObject *)m_htblTinyObjs.Lookup("2890-7772890-777");
	if (p)
		AfxMessageBox(p->ToXML());

}
void CMFCTypesFromXMLDlg::OnBtnSearchMap() 
{
	void *p = 0;
	m_mapStrToPtr.Lookup("2890-7772890-777",p);
	if (p)
		AfxMessageBox(((CTinyXMLObject *)p)->ToXML());
}


void CMFCTypesFromXMLDlg::OnBtnXml1() 
{
	m_strXML = pzXML2;
	m_strHashToSearchFor = "Purple";


	UpdateData(FALSE);
	GetDlgItem(IDC_RICHEDIT_XML)->Invalidate();
	OnFromXML("#1");
	OnToxml();
}

void CMFCTypesFromXMLDlg::OnBtnXml2() 
{
	m_strXML = pzXML3;

	m_strMapToSearchFor = "Gold";
	m_strHashToSearchFor = "Purple";

	UpdateData(FALSE);
	GetDlgItem(IDC_RICHEDIT_XML)->Invalidate();
	OnFromXML("#2");
	OnToxml();
}

void CMFCTypesFromXMLDlg::OnBtnXml3() 
{
	m_strXML = pzXML1;
	m_strMapToSearchFor = "Gold";
	m_strHashToSearchFor = "Purple";

	UpdateData(FALSE);
	GetDlgItem(IDC_RICHEDIT_XML)->Invalidate();
	OnFromXML("#3");
	OnToxml();
}

int CMFCTypesFromXMLDlg::Load50kObjects(const char *pzType) 
{
	//
	// Load it in a GString
	GString GXML(1840000);
	GXML << "<MFCTypesSample>\r\n\t<CString>50,000 Objects in " << pzType << "</CString>\r\n\t<" << pzType << ">\r\n";
	if (!GXML.FromFileAppend("50kObjects.txt",0))
	{
		AfxMessageBox("50kObjects.txt test input file not found.");
		return 0;
	}
	GXML << "\r\n\t</" << pzType << ">\r\n" << "</MFCTypesSample>";

	// Push it into the CString
	char *pC = m_strXML.GetBuffer((int)GXML.Length()+1);
	memcpy((void *)pC,GXML.Buf(),(int)GXML.Length()+1);
	m_strXML.ReleaseBuffer((int)GXML.Length());
	return 1;
}


void CMFCTypesFromXMLDlg::OnBtnXml4() 
{
	if (!Load50kObjects("CPtrList"))
		return;
	UpdateData(FALSE);
	GetDlgItem(IDC_RICHEDIT_XML)->Invalidate();
	OnFromXML("CPtrList");
	OnToxml();
}

void CMFCTypesFromXMLDlg::OnBtnXml5() 
{
	if (!Load50kObjects("GList"))
		return;
	UpdateData(FALSE);
	GetDlgItem(IDC_RICHEDIT_XML)->Invalidate();
	OnFromXML("GList");
	OnToxml();
}

void CMFCTypesFromXMLDlg::OnBtnXml6() 
{
	if (!Load50kObjects("CMapStringToPtr"))
		return;

	UpdateData(FALSE);
	GetDlgItem(IDC_RICHEDIT_XML)->Invalidate();
	OnFromXML("CMapStringToPtr");

	long lBegin = getTimeMS();
	void *p = 0;
	GString strFoundObejcts("Indexed Objects Found:\r\n\r\n");

	m_mapStrToPtr.Lookup("2890-7772890-777",p); 
	if (p) strFoundObejcts << ((CTinyXMLObject *)p)->ToXML() << "\r\n";

	m_mapStrToPtr.Lookup("21777-615621777-6156",p); 
	if (p) 	strFoundObejcts << ((CTinyXMLObject *)p)->ToXML() << "\r\n";

	long lElapsed = getTimeMS() - lBegin;
	
	m_strCompleteMessage2.Format("%d milliseconds to search CMapCStringTpPtr 1000 times",lElapsed);
	UpdateData(FALSE);
	
	OnToxml();

	UpdateData(FALSE);
	AfxMessageBox(strFoundObejcts);
}

void CMFCTypesFromXMLDlg::OnBtnXml7() 
{
	if (!Load50kObjects("GHash"))
		return;

	UpdateData(FALSE);
	GetDlgItem(IDC_RICHEDIT_XML)->Invalidate();
	OnFromXML("#7");

	long lBegin = getTimeMS();
	void *p = 0;
	GString strFoundObejcts("Indexed Objects Found:\r\n\r\n");
	for (int i=0; i<100; i++)
	{
		CTinyXMLObject *p;
		p = (CTinyXMLObject *)m_htblTinyObjs.Lookup("21777-615621777-6156"); if (p && i ==0) strFoundObejcts << ((CTinyXMLObject *)p)->ToXML() << "\r\n";
		p = (CTinyXMLObject *)m_htblTinyObjs.Lookup("32629-2177732629-21777"); if (p && i ==0) strFoundObejcts << ((CTinyXMLObject *)p)->ToXML() << "\r\n";
		p = (CTinyXMLObject *)m_htblTinyObjs.Lookup("777-31851777-31851"); if (p && i ==0) strFoundObejcts << ((CTinyXMLObject *)p)->ToXML() << "\r\n";
	}
	long lElapsed = getTimeMS() - lBegin;
	m_strCompleteMessage2.Format("%d milliseconds to search GHash 1000 times",lElapsed);
	UpdateData(FALSE);

	OnToxml();
	
	UpdateData(FALSE);
	AfxMessageBox(strFoundObejcts);
}

HBRUSH CMFCTypesFromXMLDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor) 
{
   HBRUSH hbr = CDialog::OnCtlColor(pDC, pWnd, nCtlColor);

   if (pWnd->GetDlgCtrlID() == IDC_COMPLETE_MSG || pWnd->GetDlgCtrlID() == IDC_COMPLETE_MSG2 || pWnd->GetDlgCtrlID() == IDC_COMPLETE_MSG3)
   {
      pDC->SetTextColor(RGB(255, 0, 0));
      pDC->SetBkMode(TRANSPARENT);

   }

   return hbr;
}

void CMFCTypesFromXMLDlg::OnBtnCustomXml() 
{
	UpdateData(TRUE);

	m_strMapToSearchFor = "Gold";
	m_strHashToSearchFor = "Purple";

	OnFromXML("Custom");
	OnToxml();
}

void CMFCTypesFromXMLDlg::OnBtnDel() 
{
	CTinyXMLObject *p = (CTinyXMLObject *)m_lstMyObjects.RemoveHead();
	if (p) p->DecRef(); // same as delete p;

	m_strCPtrListCount.Format("%d",(int)m_lstMyObjects.GetCount());
	UpdateData(FALSE);
}

void CMFCTypesFromXMLDlg::OnBtnDel2() 
{
	// walk the [CArray] to delete objects
	for (int i=0;i < m_arrMyObjects.GetSize();i++)
	{
		CTinyXMLObject *p = m_arrMyObjects.ElementAt(i);
		p->DecRef(); // same as delete p;
	}
	m_arrMyObjects.RemoveAll();

	m_strArrCount.Format("%d",(int)m_arrMyObjects.GetSize());
	UpdateData(FALSE);
}

void CMFCTypesFromXMLDlg::OnBtnDel3() 
{
	CTinyXMLObject *p = (CTinyXMLObject *)m_arr2MyObjs.GetAt(0);
	m_arr2MyObjs.RemoveAt(0);
	if (p) p->DecRef(); // same as delete p;

	m_strArr2Count.Format("%d",(int)m_arr2MyObjs.GetSize());
	UpdateData(FALSE);
}

void CMFCTypesFromXMLDlg::OnBtnDel4() 
{
	CTinyXMLObject *p = (CTinyXMLObject *)m_glstMyObjects.RemoveFirst();
	if (p) p->DecRef(); // same as delete p;


	m_strGListObjCount.Format("%d",(int)m_glstMyObjects.GetCount());
	UpdateData(FALSE);
}
