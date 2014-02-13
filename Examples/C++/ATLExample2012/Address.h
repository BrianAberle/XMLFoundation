// Address.h : Declaration of the CAddress

#pragma once
#include "ATLExample2012_i.h"
#include "resource.h"       // main symbols
#include <comsvcs.h>
#include "xmlObject.h"
#include "GString.h"
#include "GList.h"

using namespace ATL;



// CAddress

class ATL_NO_VTABLE CAddress :
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CAddress, &CLSID_Address>,
	public IDispatchImpl<IAddress, &IID_IAddress, &LIBID_ATLExample2012Lib, /*wMajor =*/ 1, /*wMinor =*/ 0>,
	public XMLObject
{
public:
	CAddress()
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

DECLARE_REGISTRY_RESOURCEID(IDR_ADDRESS)

DECLARE_NOT_AGGREGATABLE(CAddress)

BEGIN_COM_MAP(CAddress)
	COM_INTERFACE_ENTRY(IAddress)
	COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

	void MapXMLTagsToMembers(){};
public:
	DECLARE_FACTORY(CAddress, Address)


};


OBJECT_ENTRY_AUTO(__uuidof(Address), CAddress)
