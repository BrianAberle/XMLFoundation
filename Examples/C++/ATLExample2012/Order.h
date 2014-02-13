// Order.h : Declaration of the COrder

#pragma once
#include "ATLExample2012_i.h"
#include "resource.h"       // main symbols
#include <comsvcs.h>
#include "xmlObject.h"
#include "GString.h"
#include "GList.h"
using namespace ATL;



// COrder

class ATL_NO_VTABLE COrder :
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<COrder, &CLSID_Order>,
	public IDispatchImpl<IOrder, &IID_IOrder, &LIBID_ATLExample2012Lib, /*wMajor =*/ 1, /*wMinor =*/ 0>,
	public XMLObject
{
public:

	COrder()
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

DECLARE_REGISTRY_RESOURCEID(IDR_ORDER)

DECLARE_NOT_AGGREGATABLE(COrder)

BEGIN_COM_MAP(COrder)
	COM_INTERFACE_ENTRY(IOrder)
	COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

void MapXMLTagsToMembers(){};



// IOrder
public:
	DECLARE_FACTORY(COrder, Order)
};

OBJECT_ENTRY_AUTO(__uuidof(Order), COrder)
