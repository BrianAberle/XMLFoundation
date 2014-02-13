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
// MyATLObj.h : Declaration of the CMyATLObj

#ifndef __MYATLOBJ_H_
#define __MYATLOBJ_H_

#include "xmlObject.h"
#include "GString.h"
#include "GList.h"

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CMyATLObj
class ATL_NO_VTABLE CMyATLObj : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CMyATLObj, &CLSID_MyATLObj>,
	public IDispatchImpl<IMyATLObj, &IID_IMyATLObj, &LIBID_EXATLCOMLib>,
	public XMLObject
{

	// Private Members
	long m_nInteger;
	GString m_strString;
	// list of nested Object/Interface pairs.
	GList m_lstCMyATLObj; 

public:
	CMyATLObj()
	{
		m_nInteger = 777;
	}
	~CMyATLObj();
	long GetCustomerID();

DECLARE_REGISTRY_RESOURCEID(IDR_MYATLOBJ)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CMyATLObj)
	COM_INTERFACE_ENTRY(IMyATLObj)
	COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()


	void MapXMLTagsToMembers();

public:
	STDMETHOD(RemoveSubObjects)();
	STDMETHOD(GetMyInterface)(/*[in]*/ long Key, /*[out, retval]*/ IUnknown **pI);
	DECLARE_FACTORY(CMyATLObj, Container)

	STDMETHOD(get_TheString)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(get_Dump)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_TheString)(/*[in]*/ BSTR newVal);
	STDMETHOD(SetValues)(/*[in]*/ long lValue, /*[in]*/ BSTR strValue);
	STDMETHOD(get_XMLState)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_XMLState)(/*[in]*/ BSTR newVal);
	STDMETHOD(get_TheCount)(/*[out, retval]*/ long *pVal);
	STDMETHOD(put_TheCount)(/*[in]*/ long newVal);
};

#endif //__MYATLOBJ_H_
