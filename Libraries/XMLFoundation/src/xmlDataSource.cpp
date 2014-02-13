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
#include "GlobalInclude.h"
static char SOURCE_FILE[] = __FILE__;

#include "xmlDataSource.h"
#include "xmlObject.h"

const char *CXMLDataSource::GetAuthenticationString()
{
	m_strAuthentication.Format("&user=%s&pass=%s",(const char *)m_strUser, (const char *)m_strPwd);
	return (const char *)m_strAuthentication;
}

void *CXMLDataSource::lockConnection()
{
	return 0;
}

void CXMLDataSource::unlockConnection(void *)
{
}

CXMLDataSource::~CXMLDataSource()
{
}

CXMLDataSource::CXMLDataSource(const char **ppQueryAttributes, int nDataStreamFlags, int nConnectionPoolSize)
{
	m_nDataStreamFlags = nDataStreamFlags;
	int i;
	for (i = 0; i < nConnectionPoolSize; i++)
	{

		createPooledConnection();
	}

	i = 0;
	while (ppQueryAttributes && ppQueryAttributes[i][0])
	{
		m_lstAttributes.AddLast((void *)ppQueryAttributes[i]);
		m_lstAttributes.AddLast((void *)ppQueryAttributes[i+1]);
		i += 2;
	}

	m_strAddress += "127.0.0.1";
	m_nPort = 80;

}

void CXMLDataSource::SetUser(const char *szUser)
{
	m_strUser.Empty();
	m_strUser += szUser;
}

void CXMLDataSource::SetPwd(const char *szPwd)
{
	m_strPwd.Empty();
	m_strPwd += szPwd;
}

void CXMLDataSource::SetConnectionInfo(const char *szAddress, short nPort)
{
	m_strAddress.Empty();
	m_strAddress += szAddress;
	m_nPort = nPort;
	IPAddressChange();
}

void CXMLDataSource::AddAttributes(XMLObject *pObj)
{
	GListIterator Iter(&m_lstAttributes);
	while (Iter())
	{
		const char *pTag = (const char * )Iter++;
		const char *pValue = (const char *)Iter++;
		pObj->AddAttribute(pTag,pValue);
	}
}

