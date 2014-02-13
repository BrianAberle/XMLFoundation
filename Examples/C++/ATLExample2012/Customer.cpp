// Customer.cpp : Implementation of CCustomer

#include "stdafx.h"
#include "Customer.h"
#include <comdef.h>

IMPLEMENT_ATL_FACTORY(CCustomer, Customer)

// CCustomer

void CCustomer::MapXMLTagsToMembers()
{
	MapObjectID("ID",1);
	MapMember(&m_nInteger,  "ID");
	MapMember(&m_strString, "Name");
	MapMember(&m_lstCMyATLObj,		CCustomer::GetStaticTag());
}

long CCustomer::GetCustomerID()
{
	return m_nInteger;
}


// Direct COM data accessors
STDMETHODIMP CCustomer::get_TheCount(long *pVal)
{
	*pVal = m_nInteger;
	return S_OK;
}
STDMETHODIMP CCustomer::put_TheCount(long newVal)
{
	m_nInteger = newVal;
	return S_OK;
}
STDMETHODIMP CCustomer::SetValues(long lValue, BSTR strValue)
{
	_bstr_t b(strValue);
	m_nInteger = lValue;
	m_strString = (const char *)b;
	return S_OK;
}
STDMETHODIMP CCustomer::get_TheString(BSTR *pVal)
{
	CComBSTR temp( (const char *)m_strString );
	*pVal = temp.Copy();
	return S_OK;
}
STDMETHODIMP CCustomer::put_TheString(BSTR newVal)
{
	_bstr_t b(newVal);
	m_strString = (const char *)b;
	return S_OK;
}


// COM over XML data accessors
STDMETHODIMP CCustomer::put_XMLState(BSTR newVal)
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
STDMETHODIMP CCustomer::get_XMLState(BSTR *pVal)
{
	CComBSTR temp( ToXML() );
	*pVal = temp.Copy();
	return S_OK;
}



// various object methods
STDMETHODIMP CCustomer::get_Dump(BSTR *pVal)
{
	CComBSTR temp( Dump() );
	*pVal = temp.Copy();
	return S_OK;
}

STDMETHODIMP CCustomer::GetMyInterface(long CustomerID, IUnknown **pIOResult)
{
	// walk the list of children Objects
	GListIterator it(&m_lstCMyATLObj);
	while(it())
	{
		XMLObject *pO = (XMLObject *)it++;
		CComObject<CCustomer>*pIO = (CComObject<CCustomer>*)pO->GetInterfaceObject();
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

STDMETHODIMP CCustomer::RemoveSubObjects()
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
