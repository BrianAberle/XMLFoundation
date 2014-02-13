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

#ifndef __XML_DATA_SOURCE_HH_
#define __XML_DATA_SOURCE_HH_


#define XML_UUENCODE	0x01 
#define XML_COMPRESS	0x02

#include "GList.h"
#include "GHash.h"
#include "GString.h"

class XMLObject;
class CXMLDataSource
{
	// fast pool implementation, uses memory addresses as unique hash keys
	GHash m_ConnectionPool;
	GList m_ConnectionPoolAvailable;
	GList m_ConnectionPoolInUse;
	GList m_lstAttributes;

	GString m_strUser;
	GString m_strPwd;

	int m_nDataStreamFlags;

	// private internal methods for thread blocking and ConnectionPool access
	void *lockConnection();
	void unlockConnection(void *);
	
	// called for each connection when the CXMLDataSource is destroyed.
	virtual void  deletePooledConnection(void *pConnection) { }
	
	// create connections that will be passed to you when send() is called.
	virtual void *createPooledConnection() { return 0; }

protected:

	virtual void uuEncode(){}	
	virtual void uuDecode(){}	
	virtual void compress(){}	
	virtual void deCompress(){}	

	GString m_strAddress;
	GString m_strAuthentication;

	short   m_nPort;
public:
	// notification that future connections may be to a different server
	// The implementation may need to reset cached connection objects
	virtual void IPAddressChange(){};
	
	const char *GetServerAddress() { return (const char *)m_strAddress; }
	short GetServerPort() { return m_nPort; }

	void SetUser(const char *szUser);
	const char *GetUser() { return (const char *)m_strUser; }
	void SetPwd(const char *szPwd);
	const char *GetPwd()  { return (const char *)m_strPwd; }

	const char *GetAuthenticationString();
	virtual void SetConnectionInfo(const char *szAddress, short nPort);

	void AddAttributes(XMLObject *pObj);

	
	// ppProcAttributes
	// ----------------
	// This structure is optionally supplied to the 'ctor of the
	// data source.  Settings in this structure are optional 
	// behavior overrides that get attached to any transaction
	// executed from this datasource.  They should alternate
	// between Name and value.  Termination is the first null name.
	//
	//const char * ProcAttributes[] = 
	//{
	//	"IncludeReturnValues", "Yes",
	//	""
	//};

	CXMLDataSource(const char **ppProcAttributes = 0, int nDataStreamFlags = 0, int ConnectionPoolSize = 0 );
	virtual ~CXMLDataSource();
	
	// You will need to supply a virtual implementation to this method.
	// Call the base class first to handle "from memory" and "from disk" data 
	// sources so that you derive that functionality into your data source type.
	// Send will be called when a request is to be sent to the server.  If 
	// connection pooling is enabled, then a pointer to the connection
	// will be passed to you as well.  You should send the request to the 
	// server and wait for a response xml message.  Return the response
	// as the return value of this method.  You should directly return
	// the buffer returned from your protocol, for example if RPC allocates
	// a buffer for the return xml there is no need to copy the data to 
	// your own temporary buffer, just return the RPC buffer.  After the
	// xmlObjectFramework has processed the results, release() will be
	// called and you can free the memory there.
	virtual const char *send(const char *pzProcedureName,
							 const char *xml,
							 __int64 nXMLRequestLen,
							 void *PooledConnection,
							 // ppUserData can be the 'object' that contains the data
							 // that is returned by your implementation.  If could be 
							 // an RPC proxy, or equalenent used to get the data.
							 // It's important to keep the DataSource stateless or 
							 // you will need one per ProcedureCall in a multi-threaded
							 // application.  Return that state here so that it will
							 // be handed back to you upon release();
							 // see CSocketHTTPDataSource::send() and ::release()
							 void **ppUserData, 
							 const char *szCommandPrefix = "TransactXML=") = 0;

	// allows you to release any memory you allocated in send(); return 1 if release is made.
	virtual int release(const char *resultFromSend, void *PooledConnection, void *pUserData){return 0;};
};

#endif //__XML_DATA_SOURCE_HH_
