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
// MyATLObj.cpp : Implementation of CMyATLObj
#include "stdafx.h"
#include "ExATLCOM.h"
#include "MyATLObj.h"
#include <comdef.h>


IMPLEMENT_ATL_FACTORY(CMyATLObj, Container)

CMyATLObj::~CMyATLObj()
{
	m_nInteger = 0;
}

long CMyATLObj::GetCustomerID()
{
	return m_nInteger;
}


void CMyATLObj::MapXMLTagsToMembers()
{
	MapObjectID("CustomerID",1);
	MapMember(&m_nInteger,  "CustomerID");
	MapMember(&m_strString, "CustomerName");
	MapMember(&m_lstCMyATLObj,		CMyATLObj::GetStaticTag());
}


/////////////////////////////////////////////////////////////////////////////
// CMyATLObj



// Direct COM data accessors
STDMETHODIMP CMyATLObj::get_TheCount(long *pVal)
{
	*pVal = m_nInteger;
	return S_OK;
}
STDMETHODIMP CMyATLObj::put_TheCount(long newVal)
{
	m_nInteger = newVal;
	return S_OK;
}
STDMETHODIMP CMyATLObj::SetValues(long lValue, BSTR strValue)
{
	_bstr_t b(strValue);
	m_nInteger = lValue;
	m_strString = (const char *)b;
	return S_OK;
}
STDMETHODIMP CMyATLObj::get_TheString(BSTR *pVal)
{
	CComBSTR temp( (const char *)m_strString );
	*pVal = temp.Copy();
	return S_OK;
}
STDMETHODIMP CMyATLObj::put_TheString(BSTR newVal)
{
	_bstr_t b(newVal);
	m_strString = (const char *)b;
	return S_OK;
}


// COM over XML data accessors
STDMETHODIMP CMyATLObj::put_XMLState(BSTR newVal)
{
	_bstr_t b(newVal);
	try
	{
		FromXML((const char *)b);
	}catch(...)
	{
		return S_FALSE;
	}
	return S_OK;
}
STDMETHODIMP CMyATLObj::get_XMLState(BSTR *pVal)
{
	CComBSTR temp( ToXML() );
	*pVal = temp.Copy();
	return S_OK;
}



// various object methods
STDMETHODIMP CMyATLObj::get_Dump(BSTR *pVal)
{
	CComBSTR temp( Dump() );
	*pVal = temp.Copy();
	return S_OK;
}
STDMETHODIMP CMyATLObj::GetMyInterface(long CustomerID, IUnknown **pIOResult)
{
	// walk the list of children Objects
	GListIterator it(&m_lstCMyATLObj);
	while(it())
	{
		XMLObject *pO = (XMLObject *)it++;
		CComObject<CMyATLObj>*pIO = (CComObject<CMyATLObj>*)pO->GetInterfaceObject();
		if (pIO->GetCustomerID() == CustomerID)
		{
			*pIOResult = pIO;
			(*pIOResult)->AddRef();
			(*pIOResult)->AddRef();
			break;
		}
	}
	return S_OK;
}
STDMETHODIMP CMyATLObj::RemoveSubObjects()
{
	GListIterator it(&m_lstCMyATLObj);
	while(it())
	{
		XMLObject *pO = (XMLObject *)it++;
		pO->DecRef();
	}
	m_lstCMyATLObj.RemoveAll();
	return S_OK;
}
