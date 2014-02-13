#if !defined(AFX_5LOAVESPPG_H__1C6DF405_F0F6_4D83_AB74_021CBEAA8F24__INCLUDED_)
#define AFX_5LOAVESPPG_H__1C6DF405_F0F6_4D83_AB74_021CBEAA8F24__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

// 5LoavesPpg.h : Declaration of the C5LoavesPropPage property page class.

////////////////////////////////////////////////////////////////////////////
// C5LoavesPropPage : See 5LoavesPpg.cpp.cpp for implementation.

class C5LoavesPropPage : public COlePropertyPage
{
	DECLARE_DYNCREATE(C5LoavesPropPage)
	DECLARE_OLECREATE_EX(C5LoavesPropPage)

// Constructor
public:
	C5LoavesPropPage();

// Dialog Data
	//{{AFX_DATA(C5LoavesPropPage)
	enum { IDD = IDD_PROPPAGE_5LOAVES };
		// NOTE - ClassWizard will add data members here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA

// Implementation
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Message maps
protected:
	//{{AFX_MSG(C5LoavesPropPage)
		// NOTE - ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_5LOAVESPPG_H__1C6DF405_F0F6_4D83_AB74_021CBEAA8F24__INCLUDED)
