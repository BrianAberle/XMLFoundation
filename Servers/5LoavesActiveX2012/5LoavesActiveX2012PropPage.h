#pragma once

// 5LoavesActiveX2012PropPage.h : Declaration of the CMy5LoavesActiveX2012PropPage property page class.


// CMy5LoavesActiveX2012PropPage : See 5LoavesActiveX2012PropPage.cpp for implementation.

class CMy5LoavesActiveX2012PropPage : public COlePropertyPage
{
	DECLARE_DYNCREATE(CMy5LoavesActiveX2012PropPage)
	DECLARE_OLECREATE_EX(CMy5LoavesActiveX2012PropPage)

// Constructor
public:
	CMy5LoavesActiveX2012PropPage();

// Dialog Data
	enum { IDD = IDD_PROPPAGE_MY5LOAVESACTIVEX2012 };

// Implementation
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Message maps
protected:
	DECLARE_MESSAGE_MAP()
};

