// Customer.h : Declaration of the CCustomer

#pragma once
#include "ATLExample2012_i.h"
#include "resource.h"       // main symbols
#include <comsvcs.h>
#include "xmlObject.h"
#include "GString.h"
#include "GList.h"

using namespace ATL;



// CCustomer

class ATL_NO_VTABLE CCustomer :
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CCustomer, &CLSID_Customer>,
	public IDispatchImpl<ICustomer, &IID_ICustomer, &LIBID_ATLExample2012Lib, /*wMajor =*/ 1, /*wMinor =*/ 0>,
	public XMLObject
{
	long m_nInteger;
	GString m_strString;
	GList m_lstCMyATLObj;  // list of objects of type Customer

public:
	CCustomer()
	{
	}
	long GetCustomerID();

	DECLARE_PROTECT_FINAL_CONSTRUCT()

	HRESULT FinalConstruct()
	{
		return S_OK;
	}

	void FinalRelease()
	{
	}

DECLARE_REGISTRY_RESOURCEID(IDR_CUSTOMER)

DECLARE_NOT_AGGREGATABLE(CCustomer)

BEGIN_COM_MAP(CCustomer)
	COM_INTERFACE_ENTRY(ICustomer)
	COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

	void MapXMLTagsToMembers();

	STDMETHOD(RemoveSubObjects)();
	STDMETHOD(GetMyInterface)(/*[in]*/ long Key, /*[out, retval]*/ IUnknown **pI);


// ICustomer
public:
	DECLARE_FACTORY(CCustomer, Customer)

	STDMETHOD(get_TheString)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(get_Dump)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_TheString)(/*[in]*/ BSTR newVal);
	STDMETHOD(SetValues)(/*[in]*/ long lValue, /*[in]*/ BSTR strValue);
	STDMETHOD(get_XMLState)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_XMLState)(/*[in]*/ BSTR newVal);
	STDMETHOD(get_TheCount)(/*[out, retval]*/ long *pVal);
	STDMETHOD(put_TheCount)(/*[in]*/ long newVal);
};

OBJECT_ENTRY_AUTO(__uuidof(Customer), CCustomer)
