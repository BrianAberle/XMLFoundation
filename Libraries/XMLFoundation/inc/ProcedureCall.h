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
#ifndef _PROC_AT_CLIENT_H_
#define _PROC_AT_CLIENT_H_

#include "MemberDescriptor.h"
#include "XMLProcedureDescriptor.h"

class ObjQueryParameter;
class CXMLDataSource;

typedef void (*lpCallback)(const char *log);

class TScriptCall : public XMLObject
{
	static short m_nProjectPort;
	GStringList m_lstParameters;
	GString m_strScriptSource;
	GString m_strProject;
	GString m_strScriptName;
	CXMLDataSource *m_DataSource;
	GString m_strResults;

	void *m_pUserData; 
public:
	DECLARE_FACTORY(TScriptCall, TScriptCall)
	virtual void MapXMLTagsToMembers();

	TScriptCall(CXMLDataSource *pDS);
	TScriptCall();

	void AddParameter(const char *name, __int64 value);
	void AddParameter(const char *name, const char * value);
	const char *Execute(); 
	const char *GetFile(const char *pzFile);
	static void SetProjectPort(short n) {m_nProjectPort = n;}
};

class XMLProcedureCall : public XMLObject
{
	CXMLDataSource *m_DataSource;
	static int m_RequestID;
	
	const char *MakeValid(const char *pzName, GString &strDestination);
	
	GString m_strRequest;

	void *m_pUserData; 
	bool m_bForceTransaction;
	GString m_strSchema;
	bool m_bRunObjectFactory;
	const char *m_pzReportTemplateName;

	GList m_lstProcedures;				// ObjQueryParameter's
	GList m_lstObjectParams;			// XMLObject's
	GList m_lstObjectParamsAutoDestroy; // XMLObject's
	GList m_lstTransactionalSet;		// ObjQueryParameter's

	ObjQueryParameter *m_pCurrentParamBlock;
	ObjQueryParameter *m_pCurrentProcedure;

	MemberDescriptor *HandleUnmappedMember( const char *pzTag );
protected:

	ObjQueryParameter *m_TagValueParams;
	void Init();

public:

	GList m_lstReturnValues;  // contains CReturnValue's
	GList m_lstReturnParams;  // contains CProcReturn's
	// Get the procedure return value, the pzProcName identifies
	// which procedure in the case of transactional operations
	const char *GetReturnValue(const char *pzProcName);
	
	// For backward compatibility, It's better to use the ProcName
	// to get the return value, since the index value may refer to 
	// the wrong procedure if a transactional procedure is later modified. 
	const char *GetReturnValue(int index);
	void IncludeReturnValues();

	void SetSchema(const char *szSchema);

	// May be called after execute to see the request that was issued
	// to the server.
	const char *GetXMLRequest(){return m_strRequest;}


	DECLARE_FACTORY(XMLProcedureCall, TransactXML)

	XMLProcedureCall(CXMLDataSource *pDS, const char *pzProcName);
	XMLProcedureCall(CXMLDataSource *pDS);
	XMLProcedureCall();

	// By default, ProcedureCalls do not expect a result set of objects.
	// If a result set is expected, ObjQuery will provide information about it.
	virtual long GetRowCount() { return (long)-1;}
	// Procedure calls don't have result sets, the Query will supply this
	virtual const char *getResultObjectTag(){ return "";}

	virtual ~XMLProcedureCall();
	// The procedure name
	GString m_strProcedureName;

	// Only used when specifically enabled in the server extension
	bool hasMoreUnfetchedResults()
	{
		if (FindAttribute("cursorHandle"))
			return true;
		return false;
	}

	void ForceTransaction (); 
	void SetProcedureName (const char * pzName); 
	void SetDataSource(CXMLDataSource *pSource) {m_DataSource = pSource;};
	virtual void MapXMLTagsToMembers();
	void AddDirectSQL(const char * sql, const char *pzDriver,const char *pzServer, const char *pzSchema, const char *pzUser, const char *pzPassword);
	void AddDirectSQL(const char * sql);

	// Add a procedure to this transaction. AddParameter() sets the params 
	// of the current procedure(most recently added to the transaction).
	void AddProcedure(const char * ProcName);
	void AddProcedure(const char * ProcName, const char *ProcValue);

	// Set exec details for the current procedure.  Parameter's are
	// required and positional, ProcAttribute's are optional and non positional
	void AddParameter(const char * name, __int64 value);
	void AddParameter(const char * name, const char * value);
	
	// Note:Caller allocates the new object parameter and must never delete it.
	//		It will be destroyed when XMLProcedureCall goes out of scope.
	void AddParameter(XMLObject *pObject);

	void AddProcAttribute(const char * name, const char * value);
	void AddProcAttribute(const char * name, int value);

	// Add attributes to the transaction.
	void SetAttribute( const char * pzName, int nValue );
	void SetAttribute( const char * pzName, const char * pzValue );

	// Start a new parameter block in the current procedure.
	// ie: reexecute the current procedure with different paramaters (bulk insert)
	void StartNewParameterBlock();

	
	void StoreOutParam(const char *recallVarName);
	void StoreDataValue(const char *userVarName, const char *column);
	void RecallVariable(const char *name);


	ObjQueryParameter *NestProcedure(const char * ProcName, ObjQueryParameter *pParent=0);
	
	const char *m_strXml;
	char *m_pzXMLExtractFromReport;

	// Server build only.
	void SetUser(const char *szUser, const char *szPassword);

	// if the Destination Object is supplied as an outer tag set
	// bDestinationObjectTagPresent = true.  For example, if you are
	// adding Orders into a Customer and your XML looks like this:
	// <Customer oid=1>
	//		<Order>
	//			<OrderMembers>
	// In this case from your Customer Object call proc.Execute(this,true)
	//  
	//  
	// If your XML does not have the destination object tag present set
	// bDestinationObjectTagPresent = false.  For example, when your
	// XML looks like this:
	//	<Order>
	//		<OrderMembers>
	// In this case from your Customer Object call proc.Execute(this,false)
	const char *Execute(XMLObject *pDestinatonObject = 0, 
						bool bDestinationObjectTagPresent  = 1);


	// Pick a "Data View" translation of the results - XML, HTML, CSV, or an XSLT translation
	// --------------------------------------------------------------------------------------

	// May be called inplace of Execute() to obtain the 
	// XML and bypass the Object Factorization process, or called
	// after Execute() to return the XML that objects were created from.
	const char *GetXML();
	// Returns a square result set marked up in HTML
	const char *GetHtmlSqlView(const char *pzDestFile = 0);
	// returns a traditional square result set from the database
	// Results are Comma Seperated Values(CSV) with any data containing a comma
	// wrapped in quotes and "" to escape a " in the data.
	const char *GetSquareResults();
	const char *MergeXSL(const char *szXSLTemplate);
	// execute a custom markup handler written in (java, Perl, Python, .NET, COM, CORBA, or C++) 
	const char *GetCustomMarkup();
	// --------------------------------------------------------------------------------------

	

	// Instanciate the XMLProcedureCall with the name of the procedure
	// want details about, then call Describe() and you will be returned
	// the list of required parameters and and types for the procedure.
	void Describe(XMLObject *pDestinatonObject);
	
	void ResultColumns(XMLObject *pDestinatonObject);

	// register global call backs to log send, recv data
	static lpCallback m_lpfnSend;
	static lpCallback m_lpfnRecv;
	static void SetSendCallBack(lpCallback lpFn){m_lpfnSend = lpFn;}
	static void SetRecvCallBack(lpCallback lpFn){m_lpfnRecv = lpFn;}
};

// storage for 'out' params or data captured in a call to StoreReturnValue()
// generally this will be an identity, rowcount or information about the call.
class CProcReturn : public XMLObject
{
public:
	GString m_strProcName;
	GString m_strParamName;
	GString m_strValue;
	DECLARE_FACTORY(CProcReturn,Identity)
	virtual void MapXMLTagsToMembers();
};


// storage for the return value of a procedure 
// call stored as attributes of this object
class CReturnValue : public XMLObject
{
	DECLARE_FACTORY(CReturnValue,Proc)
};


#endif //_PROC_AT_CLIENT_H_
