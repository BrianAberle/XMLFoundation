// LineItem.h : Declaration of the CLineItem

#pragma once
#include "ATLExample2012_i.h"
#include "resource.h"       // main symbols
#include <comsvcs.h>
#include "xmlObject.h"
#include "GString.h"
#include "GList.h"

using namespace ATL;



// CLineItem

class ATL_NO_VTABLE CLineItem :
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CLineItem, &CLSID_LineItem>,
	public IDispatchImpl<ILineItem, &IID_ILineItem, &LIBID_ATLExample2012Lib, /*wMajor =*/ 1, /*wMinor =*/ 0>,
	public XMLObject
{
public:
	CLineItem()
	{
	}

	DECLARE_PROTECT_FINAL_CONSTRUCT()

	HRESULT FinalConstruct()
	{
		return S_OK;
	}

	void FinalRelease()
	{
	}

DECLARE_REGISTRY_RESOURCEID(IDR_LINEITEM)

DECLARE_NOT_AGGREGATABLE(CLineItem)

BEGIN_COM_MAP(CLineItem)
	COM_INTERFACE_ENTRY(ILineItem)
	COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

	void MapXMLTagsToMembers(){};



// ILineItem
public:
	DECLARE_FACTORY(CLineItem, LineItem)
};

OBJECT_ENTRY_AUTO(__uuidof(LineItem), CLineItem)
