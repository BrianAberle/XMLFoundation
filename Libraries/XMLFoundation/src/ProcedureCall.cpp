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

#include "ProcedureCall.h"
#include "xmlObject.h"
#include "xmlObjectFactory.h" 
#include "ObjQueryParameter.h"
#include "FactoryManager.h"
#include "FrameworkAuditLog.h"
#include "xmlDataSource.h"
#include "RelationshipWrapper.h"
#include "GException.h"
#include "GProfile.h"

#include <string.h> // for: strpbrk(), strlen(), strstr(), memcpy()
#ifdef _WIN32
	#define strcasecmp	_stricmp
#else 
	//  __sun	and _LINUX	
	#include <strings.h>// for :strcasecmp()
#endif


IMPLEMENT_FACTORY(XMLProcedureCall,TransactXML)
IMPLEMENT_FACTORY(CProcReturn,Identity)
IMPLEMENT_FACTORY(CReturnValue,Proc)

void CProcReturn::MapXMLTagsToMembers()
{
	MapMember(&m_strProcName, "Source");
	MapMember(&m_strParamName, "ParamName");
	MapMember(&m_strValue, "ParamValue");
}


int XMLProcedureCall::m_RequestID = 0;
lpCallback XMLProcedureCall::m_lpfnSend = 0;
lpCallback XMLProcedureCall::m_lpfnRecv = 0;
short TScriptCall::m_nProjectPort = 0;

XMLProcedureCall::XMLProcedureCall(CXMLDataSource *pDS) :
m_strRequest(4096)
{
	Init();
	SetDataSource(pDS);
}


XMLProcedureCall::XMLProcedureCall(CXMLDataSource *pDS, const char *pzProcName) :
	m_strRequest(4096)
{
	Init();
	SetProcedureName(pzProcName);
	SetDataSource(pDS);
}

void XMLProcedureCall::Init() 
{
	SetObjectTypeName("XMLProcedureCall");
	SetDefaultXMLTag("TransactXML");
	ModifyObjectBehavior(NO_MEMBER_MAP_PRECOUNT);
	m_pCurrentParamBlock = 0;
	m_pCurrentProcedure = 0;
	m_TagValueParams = 0;
	m_DataSource = 0;
	m_bForceTransaction = 0;
	m_bRunObjectFactory = 1;
	m_pzReportTemplateName = 0;
	m_pzXMLExtractFromReport = 0;
	m_strXml = 0;
	m_pUserData = 0;
}

void XMLProcedureCall::ForceTransaction()
{
	m_bForceTransaction = 1;
}

XMLProcedureCall::XMLProcedureCall() 
{
	Init();
};


const char *XMLProcedureCall::GetReturnValue(const char *pzProcName)
{
	GListIterator Iter2(&m_lstReturnValues);
	while (Iter2())
	{
		CReturnValue *pRet = (CReturnValue *)Iter2++;
		if( (strcasecmp(pRet->FindAttribute("sp"), pzProcName) == 0) )
		{
			return pRet->FindAttribute("ret");
		}
	}
	
	GListIterator Iter(&m_lstReturnParams);
	while (Iter())
	{
		CProcReturn *pRet = (CProcReturn *)Iter++;
		if( (strcasecmp((const char *)pRet->m_strProcName, pzProcName) == 0) || 
			(strcasecmp((const char *)pRet->m_strParamName, pzProcName) == 0) )
		{
			return (const char *)pRet->m_strValue;
		}
	}

	return 0;
}

void XMLProcedureCall::IncludeReturnValues()
{
	if (m_pCurrentProcedure)
		m_pCurrentProcedure->AddAttribute("IncludeReturnValues","yes");
}


const char *XMLProcedureCall::GetReturnValue(int index)
{
	GListIterator Iter(&m_lstReturnParams);
	int nListIndex = 0;
	while (Iter())
	{
		CProcReturn *pRet = (CProcReturn *)Iter++;
		if (index == nListIndex)
		{
			return (const char *)pRet->m_strValue;
		}
		nListIndex++;
	}
	return 0;
}


XMLProcedureCall::~XMLProcedureCall()
{
	if (m_pzXMLExtractFromReport)
	{
		delete m_pzXMLExtractFromReport;
	}
	if (m_strXml)
	{
		m_DataSource->release(m_strXml,0,m_pUserData);
	}

	if (m_TagValueParams)
	{
		delete m_TagValueParams;
	}

	GListIterator IterProc(&m_lstReturnParams); // Params list
	while (IterProc())
	{
		delete (CProcReturn *)IterProc++;
	}
	GListIterator IterValue(&m_lstReturnValues); // Values list
	while (IterValue())
	{
		delete (CReturnValue *)IterValue++;
	}

	GListIterator Iter2(&m_lstObjectParamsAutoDestroy);
	while (Iter2())
	{	
		ObjQueryParameter *pO = (ObjQueryParameter *)Iter2++;
		delete pO;
	}
}

// The pzProcedureCallName can be:
//	a. a rule script name(with the .xml extension) ex. "MyRuleScript"
//	b. a stored procedure name ex. "sp_InsertOrder"
//  c. free hand SQL. ex "INSERT INTO ...(?,?) )"
void XMLProcedureCall::SetProcedureName (const char *pzProcedureCallName) 
{
	int nInSQL = 0;
	if (strpbrk(pzProcedureCallName," "))
	{
		m_strProcedureName = "Transact";
		nInSQL = 1;
	}
	else
	{
		m_strProcedureName = pzProcedureCallName;
	}
	
	ObjQueryParameter *pProc = new ObjQueryParameter(m_strProcedureName);

	if (nInSQL)
	{
		pProc->AddAttribute("sql",pzProcedureCallName);
	}

	// pBlock will become a member variable in the [pProc] object.
	// This gives it a MemberDescriptor identical to a static MapMember() call - here it's dynamic.
	ObjQueryParameter *pBlock = new ObjQueryParameter("Parameters");
	
	
	pProc->AddMember(*pBlock);  // pBlock is mow a member - DecRef() will kill it like any dynamic child XMLObject

	m_lstProcedures.AddLast(pProc);
	//m_lstObjectParamsAutoDestroy.AddLast(pBlock);      This was bad.   Fixed 12/21/2013 by commenting out this line
	m_lstObjectParamsAutoDestroy.AddLast(pProc);	// however this is absolutely necessary
	m_pCurrentParamBlock = pBlock;
	m_pCurrentProcedure = pProc;
}

void XMLProcedureCall::MapXMLTagsToMembers()
{
	MapMember(&m_lstProcedures,"UserDefined"); 

	MapMember(&m_lstReturnParams,  CProcReturn::GetStaticTag(), 0, CProcReturn::GetStaticFactory() );
	
	MapMember(&m_lstReturnValues, CReturnValue::GetStaticTag(), "Retvals", CReturnValue::GetStaticFactory());
	// Return Values can be serialized "into" this object but not out.
	SetMemberSerialize("Retvals", false);
}


// start a new parameter block in the current procedure.
// ie: reexecute the current procedure with different paramaters
// ideal for fast bulk insert.
void XMLProcedureCall::StartNewParameterBlock()
{
	ObjQueryParameter *pParam = new ObjQueryParameter("Parameters");
	m_pCurrentParamBlock = pParam;

	// add a new block to the current procedure
	m_pCurrentProcedure->AddMember(*pParam);
}

void XMLProcedureCall::StoreOutParam(const char *recallVarName)
{
	ObjQueryParameter *pParam = new ObjQueryParameter(recallVarName);
	pParam->AddAttribute("out","true");
	AddParameter( pParam );
}

// Use a previously stored variable as the paramater input value.
void XMLProcedureCall::RecallVariable(const char *name)
{
	ObjQueryParameter *pParam = new ObjQueryParameter(name);
	pParam->AddAttribute("source","Recall");
	AddParameter( pParam );
}

//	<VariableStore type="varchar" source=column column=CustomerID name=userVarCustID/>
void XMLProcedureCall::StoreDataValue(const char *userVarName, const char *column)
{
	ObjQueryParameter *pParam = new ObjQueryParameter("VariableStore");
	pParam->AddAttribute("source","column");
	pParam->AddAttribute("column",column);
	pParam->AddAttribute("name",userVarName);
	AddParameter( pParam );
}


ObjQueryParameter *XMLProcedureCall::NestProcedure(const char * ProcName, ObjQueryParameter *pParent/*=0*/)
{
	ObjQueryParameter *pProcInTransaction = new ObjQueryParameter(ProcName);
	ObjQueryParameter *pParamBlock = new ObjQueryParameter("Parameters");
	pProcInTransaction->AddMember(*pParamBlock);

	if (pParent)
		pParent->MapMember(pProcInTransaction, ProcName);
	else
		this->MapMember(pProcInTransaction,ProcName);

	m_lstObjectParamsAutoDestroy.AddLast(pProcInTransaction);
	m_pCurrentParamBlock = pParamBlock;
	m_pCurrentProcedure = pProcInTransaction;
	return pProcInTransaction;
}

void XMLProcedureCall::AddDirectSQL(const char * sql)
{
	AddProcedure("DirectSQL");

	// causes CXMLRequest::m_strValue to contain the SQL in the Transaction
	m_pCurrentProcedure->SetObjectValue(sql,-1,3);

	// Force the SQL to go through the tranasction handler
	// of the XML data server since the direct sql will not
	// have a file that describes the XML result set.

	//	ForceTransaction();
	
	// when ForceTransaction() is there, a square html view falls into
	// the TransactSQL() handler in the server, it should hit the
	// GetHTMLMarkup() in the server.  I can either reorder the attribute 
	// presidence or remove the "Transaction=yes" attribute here..... 

}

void XMLProcedureCall::AddDirectSQL(const char * sql, const char *pzDriver,const char *pzServer, const char *pzSchema, const char *pzUser, const char *pzPassword)
{
	AddDirectSQL(sql);

	// These do not need to be supplied, if they are not the 
	// default values from txml.txt (on the server) will be used
	if (pzDriver)
		AddAttribute("Driver",  pzDriver);
	if (pzSchema)
		AddAttribute("Schema",  pzSchema);
	if (pzServer)
		AddAttribute("Server",  pzServer);
	if (pzUser)
		AddAttribute("DBUser",	pzUser);
	if (pzPassword)
		AddAttribute("DBPassword",pzPassword);

}

void XMLProcedureCall::AddProcedure(const char * ProcName, const char *ProcValue)
{
	AddProcedure(ProcName);
	m_pCurrentProcedure->SetObjectValue(ProcValue,-1,3);
}

void XMLProcedureCall::AddProcedure(const char * ProcName)
{
	if (m_strProcedureName.Length() == 0)
	{
		SetProcedureName(ProcName);
	}
	else
	{
		ObjQueryParameter *pProcInTransaction = new ObjQueryParameter(ProcName);
		ObjQueryParameter *pParamBlock = new ObjQueryParameter("Parameters");
		pProcInTransaction->AddMember(*pParamBlock);
		m_pCurrentParamBlock = pParamBlock;
		m_pCurrentProcedure = pProcInTransaction;
		m_lstProcedures.AddLast(pProcInTransaction);
		m_lstObjectParamsAutoDestroy.AddLast(pProcInTransaction);
	}
}

void XMLProcedureCall::AddParameter( const char * pzTag, __int64 value )
{
	GString strTemp;
	strTemp << value;
	AddParameter(pzTag, (const char *)strTemp);
}

#define isLetter(ch) ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z'))
#define isNameChar(ch) ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z')) || (ch >= '0' && ch <= '9') || (ch == '.') || (ch == '-') || (ch == '_') || (ch == ':')
const char *XMLProcedureCall::MakeValid(const char *pzName, GString &strDestination)
{
	if ( !isLetter(pzName[0]) )
	{
		// prefix invalid XML tag			
		strDestination = "Param_";
	}
	
	// Replace any other invalid chars with '_'
	__int64 nLen = (pzName) ? strlen(pzName) : 0;
	for(int i=0; i < nLen; i++)
	{
		if ( isNameChar(pzName[i]) )
			strDestination << pzName[i];
		else
			strDestination << '_';
	}

	// If the entire name is invalid - example "12345"
	// (since an XML Element cannot start with a number XML 1.0)
	// replace the invalid name with a valid one.
	if (strDestination.IsEmpty())
		strDestination = "Parameter";

	return (const char *)strDestination;
}




void XMLProcedureCall::AddProcAttribute(const char * name, int value)
{
	if (m_pCurrentProcedure)
	{
		m_pCurrentProcedure->AddAttribute(name,value);
	}
}
void XMLProcedureCall::AddProcAttribute(const char * name, const char * value)
{
	if (m_pCurrentProcedure)
	{
		m_pCurrentProcedure->AddAttribute(name,value);
	}
}


void XMLProcedureCall::AddParameter(const char *pzName, const char *pzValue)
{
	// Make the param name a valid XML element name. Change the
	// user supplied name if necessary since we bind by order not
	// by name.  The name is a comment for readability.  This is faster
	// at the server and lowest common denominator in object technologies.
	GString strTemp;
	const char *pzTag = MakeValid(pzName,strTemp);

	ObjQueryParameter *pParam = new ObjQueryParameter(pzTag);
	if (pzValue)
		pParam->SetObjectValue( pzValue, -1, 3 );
	else
		pParam->SetObjectValue( "", -1, 3 ); 
	AddParameter( pParam );
}


void XMLProcedureCall::AddParameter(XMLObject *pObject)
{
	if (m_pCurrentParamBlock)
	{
		// it's a parameter of a procedure within this transaction
		m_pCurrentParamBlock->AddMember(*pObject);
	}
	else
	{
		throw GException("XMLProcedureCall", 0);
	}
}

void XMLProcedureCall::SetAttribute( const char * pzName, int nValue )
{
	GString strTemp;
	strTemp.Format("%ld", (int)nValue);
	AddAttribute( pzName, (const char *)strTemp );
}

void XMLProcedureCall::SetAttribute( const char * pzName, const char * pzValue )
{
	if (m_pCurrentProcedure)
	{
		// it's a parameter of a procedure within this transaction
		m_pCurrentProcedure->AddAttribute(pzName, pzValue);
	}
	else
	{
		throw GException("XMLProcedureCall", 1);
	}
}

// If this gets called, the result XML has a root object 
// that does not map to the query. For example, we queried 
// for "Orders' and got back a 'Customer' that MAY contain 
// an 'Order' as a child. (we don't know yet)
// We'll tell the XMLObjectFactory to create the 'unknown'
// object with the member descriptor stored in the 
// FactoryManager. (rather than the result set list factory)
// When the object that we are really looking for is encountered
// we get the results into the query by the current setting
// passed to setResultDescriptor().
MemberDescriptor *XMLProcedureCall::HandleUnmappedMember( const char *pzTag )
{
	return XMLObjectFactoryManager::getFactoryManager().GetRootableMapEntry(pzTag);
}

const char *XMLProcedureCall::GetHtmlSqlView(const char *pzDestFile/* = 0*/)
{
	m_bRunObjectFactory = 0;

	AddAttribute("BrowseRowAndColumn",  "yes");
	const char *pRet =  Execute();
	if (pzDestFile)
	{
		GString str(pRet);
		try
		{
			str.ToFile(pzDestFile);
		}
		catch(GException &e)
		{
			e.AddErrorDetail("XMLProcedureCall", 2, pzDestFile);
			throw e;
		}
	}
	return pRet;
}


void XMLProcedureCall::Describe(XMLObject *pDestinatonObject)
{
	AddAttribute( "DescribeProcedure", "yes" );
	Execute(pDestinatonObject);
}

void XMLProcedureCall::ResultColumns(XMLObject *pDestinatonObject)
{
	AddAttribute("DescribeResults", "yes");
	Execute(pDestinatonObject);
}



const char *XMLProcedureCall::GetXML()
{
	if (m_pzReportTemplateName)
	{
		if (m_strXml)
		{
			// Comming soon to the server...
			// If the m_strXml contains the HTML view generated by TXML
			// XSLT it will have the source XML embedded at the head of
			// the output in an HTML comment, so that the rendered view
			// always can reference it's data with which it was rendered.
			const char *p = strstr(m_strXml,"<0-EOXML>");
			if (p)
			{
				__int64 nSize = p-m_strXml-5;
				m_pzXMLExtractFromReport = new char [nSize + 1];
				memcpy(m_pzXMLExtractFromReport,&m_strXml[4],nSize);
				m_pzXMLExtractFromReport[nSize] = 0;
				return m_pzXMLExtractFromReport;
			}
			else
			{
				return 0;
			}
		}
	}
	else
	{
		// if the user has called Execute() already, return what we have.
		if (m_strXml)
			return m_strXml;

		// otherwise go get it the XML without creating any objects.
		m_bRunObjectFactory = 0;
		return Execute();
	}
	return 0;
}

const char *XMLProcedureCall::GetCustomMarkup()
{
	AddAttribute( "GetCustomMarkup", "yes" );
	m_bRunObjectFactory = 0;
	return Execute();
}

const char *XMLProcedureCall::GetSquareResults()
{
	AddAttribute( "GetCommaSeperated", "yes" );
	m_bRunObjectFactory = 0;
	return Execute();
}

const char *XMLProcedureCall::MergeXSL(const char *szXSLTemplate)
{
	AddAttribute( "MergeXSL", "yes" );
	AddAttribute( "Template", szXSLTemplate );
	m_bRunObjectFactory = 0;
	return Execute();
}



void XMLProcedureCall::SetSchema(const char *szSchema)
{
	m_strSchema = szSchema;
}


const char *XMLProcedureCall::Execute( XMLObject *pDestinatonObject /*= 0*/, 
							    bool bDestinationObjectTagPresent /* = 1*/)
{
	if (!m_DataSource)
	{
		throw GException("XMLProcedureCall", 8);
	}

	if (!m_strSchema.IsEmpty())
		AddAttribute( "Schema", (const char *)m_strSchema );

	m_DataSource->AddAttributes(this);
	if ( m_lstProcedures.Size() > 1  ||		// Multiple DB operations or joins(within the same transaction) 
		 m_bForceTransaction 		 ||		// Force Transactional
		 strstr((const char *)m_strProcedureName,"::") )	// executing CustomDLL.procedure

	{
		AddAttribute( "Transaction", "yes" );
	}

	if ( GetRowCount() != (long)-1 )
	{
		AddAttribute( "maxObjects", GetRowCount() );
	}

	if (m_pzReportTemplateName)
	{
		AddAttribute( "MergeXML", "yes" );
		AddAttribute( "Template", m_pzReportTemplateName );
	}
	
	ToXML(&m_strRequest);

	if (m_lpfnSend)
	{
		m_lpfnSend((const char *)m_strRequest);
	}

	const char *pDebugFile = GetProfile().GetString("Debug", "DebugTraceXML", false);
	if (pDebugFile && pDebugFile[0])
	{
		// trace out the xml being sent to the server
		GString strTrace;
		strTrace.Format("\n\n\n\nSent To [%s]-----------\n%s",m_DataSource->GetServerAddress(),(const char *)m_strRequest);
		strTrace.ToFileAppend(pDebugFile);
	}
	
	int nRetryCount = 0;
RESEND:
	try
	{
		
		m_strXml = m_DataSource->send(	(const char *)m_strProcedureName,
										(const char *)m_strRequest,
										(int)m_strRequest.Length(),/* Length w/o Null terminator */
										0, 
										&m_pUserData,"TransactXML=");

		if (m_lpfnRecv)
			m_lpfnRecv(m_strXml);

	}
	catch(GException &e)
	{
		// "General error parsing XML stream" means the data was corrupted in transit.
		if (e.GetError() == 7)
		{
			// Resend the request.
			if (nRetryCount++ < 3)
			{
				TRACE_WARNING("Attempting resend" );
				goto RESEND;
			}
		}
		
		// "the handle is invalid".  We need to 'reboot' the datasource. (WININET)
		if ((e.GetError()  == 6) && 
			(e.GetSystem() == 0)) 
		{
			// Resend the request.
			if (nRetryCount++ < 3)
			{
				TRACE_WARNING("Attempting resend" );
				m_DataSource->IPAddressChange();
				goto RESEND;
			}
		}

		// This helps distinguish Client errors from Server errors
		// unless the error was a client side connect error.
		throw GException("XMLProcedureCall", 4, m_DataSource->GetServerAddress(), e.GetDescription());
	}

	if (pDebugFile && pDebugFile[0])
	{
		// trace out the xml returned from the server
		GString strTrace("\n\n\n\nReceived:\n----------\n");
		const char *pXML = GetXML();
		if (pXML)
			strTrace += pXML;
		strTrace.ToFileAppend(pDebugFile);
	}

	// map to tags that we expect to recieve in the query results
	LoadMemberMappings();

	if ( m_bRunObjectFactory && m_strXml && m_strXml[0] )
	{
		const char *pFactoryXML = m_strXml;
		if (m_pzReportTemplateName)
		{
			pFactoryXML = GetXML();
		}
		
		try
		{
			if (pDestinatonObject)
			{
		
				// The tag "XMLQueryResults" can be anything.  The outer most
				// tag is never verified, this object will contain the Root
				// starting point for factorization (the pDestinatonObject object)
				if (bDestinationObjectTagPresent)
				{
					XMLRelationshipWrapper objectContainer("XMLQueryResults");

					objectContainer.ModifyObjectBehavior(PREVENT_AUTO_DESTRUCTION);


					const char *tag = pDestinatonObject->GetObjectTag();
					objectContainer.AddReference(	tag, pDestinatonObject );


					// When we are paring into an object and the object tag is
					// specified in the XML.  For example, when placing:
					// <Results>
					//		<Customer>
					//			<Widgit>
					//				...
					//			</Widgit>
					//		</Customer>
					// </Results>

					// This will allow us to put Widgit's in a 'Customer' or any 
					// type of object that the pDestinatonObject describes.
					objectContainer.FromXML(pFactoryXML,this);
				}
				else
				{
					// When we are paring into an object but that object is not
					// specified in the XML.  For example, when placing:
					// <Results>
					//		<Widgit>
					//			...
					//		</Widgit>
					// </Results>

					// This will allow us to put Widgit's in a 'Customer' or any 
					// type of object that the pDestinatonObject describes.
					pDestinatonObject->FromXML(pFactoryXML,this);
				}
			}
			else
			{
				XMLObjectFactory factory ( pFactoryXML,m_DataSource->GetServerAddress() );
				// Only Queries have result descriptors
				if ( getResultObjectTag() )
				{
					factory.setResultDescriptor( GetEntry( getResultObjectTag() ) );
				}
				factory.extractObjects(this);
			}
		}
		catch (GException &)
		{
			throw;
		}

// in a debug build, this code is better off compiled out so that the debugger will break closer to the problem.
// in a release build, this may help to 'crash softer'.
#ifndef _DEBUG
		// if we should catch an unhandled exception here
		catch ( ... )
		{
			TRACE_ERROR("Fatal Error while factory creating objects" );
			TRACE_ERROR("1. Check your String/List Handlers" );
			TRACE_ERROR("2. Did you delete a cached object and not remove it from the cache?");
	
			throw GException("XMLProcedureCall", 6);
		}
#endif

	}
	else
	{
		if (m_bRunObjectFactory)
		{
			// we should never get nothing, it may indicate 
			// communication failure depending on the type of DataSource
			TRACE_WARNING("Nothing received from DataBroker" );
			throw GException("XMLProcedureCall", 7);
		}
	}
	return m_strXml;
}

// Old-Style password management
//#include "Base64.h"
//#include "PasswordHelper.h"
void XMLProcedureCall::SetUser(const char *szUser, const char *szPwd)
{
/*
	AddAttribute( "U.s.e.r.", szUser );

	unsigned char digest[16];
	const char *pszSalt = szUser;
	const char *pszPass = szPwd;
	PasswordHelper::Hash(pszSalt, pszPass, digest);

	BUFFER b;
	BufferInit(&b);
	uuencode((unsigned char *)digest, sizeof(digest), &b);
	GString strPwd((char *)b.pBuf, b.cLen);
	AddAttribute( "P.a.s.s.w.o.r.d.", (const char *)strPwd );
	BufferTerminate(&b);
*/
}


//////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////
IMPLEMENT_FACTORY(TScriptCall, TScriptCall)

void TScriptCall::MapXMLTagsToMembers(){};

TScriptCall::TScriptCall(CXMLDataSource *pDS)
{
	m_DataSource = pDS;
}

TScriptCall::TScriptCall()
{
}


void TScriptCall::AddParameter(const char * name, __int64 value)
{
	// name is ignored
	GString strASCII;
	strASCII << value;
	m_lstParameters.AddLast((const char *)strASCII);
}
void TScriptCall::AddParameter(const char * name, const char * value)
{
	// name is ignored
	m_lstParameters.AddLast(value);
}

const char *TScriptCall::GetFile(const char *pzFile)
{
	int nRetryCount = 0;
RESEND:
	try
	{
		m_strResults = m_DataSource->send(	"TScript",
										pzFile,
										(int)strlen(pzFile),/* -1 = Don't send the Null terminator */
										0,
										&m_pUserData,"TScriptFile=");

		if (XMLProcedureCall::m_lpfnRecv)
			XMLProcedureCall::m_lpfnRecv(m_strResults);

	}
	catch(GException &e)
	{
		// "General error parsing XML stream" means the data was corrupted in transit.
		if (e.GetError() == 7)
		{
			// Resend the request.
			if (nRetryCount++ < 3)
			{
				TRACE_WARNING("Attempting resend" );
				goto RESEND;
			}
		}
		
		// "the handle is invalid".  We need to 'reboot' the datasource. (WININET)
		if ((e.GetError()  == 6) && 
			(e.GetSystem() == 0)) 
		{
			// Resend the request.
			if (nRetryCount++ < 3)
			{
				TRACE_WARNING("Attempting resend" );
				m_DataSource->IPAddressChange();
				goto RESEND;
			}
		}

		// This helps distinguish Client errors from Server errors
		// unless the error was a client side connect error.
		throw GException("XMLProcedureCall", 4, m_DataSource->GetServerAddress(), e.GetDescription());
	}
	return m_strResults;
}

const char *TScriptCall::Execute()
{
	GString strCommand;
	int bAddedFirstParam = 0;

	if (m_nProjectPort)
	{
		strCommand.Format("%d",(int)m_nProjectPort);
	}
	else
	{
		strCommand = m_strProject;
	}
	strCommand += ":CallScript(";
	
	GStringIterator it( &m_lstParameters );
	while (it())
	{
		if (bAddedFirstParam)
			strCommand += ',';
		GString str(it++);
		strCommand += '"';
		strCommand += str.EscapeWithHex("'\"%");
		strCommand += '"';
		bAddedFirstParam = 1;
	}
	strCommand += ")";

	if (XMLProcedureCall::m_lpfnSend)
	{
		XMLProcedureCall::m_lpfnSend( strCommand );
	}

	int nRetryCount = 0;
RESEND:
	try
	{
		m_strResults = m_DataSource->send(	"TScript",
										strCommand,
										(int)strCommand.Length(),/* -1 = Don't send the Null terminator */
										0,
										&m_pUserData,"TScriptServ=");

		if (XMLProcedureCall::m_lpfnRecv)
			XMLProcedureCall::m_lpfnRecv(m_strResults);

	}
	catch(GException &e)
	{
		// "General error parsing XML stream" means the data was corrupted in transit.
		if (e.GetError() == 7)
		{
			// Resend the request.
			if (nRetryCount++ < 3)
			{
				TRACE_WARNING("Attempting resend" );
				goto RESEND;
			}
		}
		
		// "the handle is invalid".  We need to 'reboot' the datasource. (WININET)
		if ((e.GetError()  == 6) && 
			(e.GetSystem() == 0)) 
		{
			// Resend the request.
			if (nRetryCount++ < 3)
			{
				TRACE_WARNING("Attempting resend" );
				m_DataSource->IPAddressChange();
				goto RESEND;
			}
		}

		// This helps distinguish Client errors from Server errors
		// unless the error was a client side connect error.
		throw GException("XMLProcedureCall", 4, m_DataSource->GetServerAddress(), e.GetDescription());
	}
	return m_strResults;
}
