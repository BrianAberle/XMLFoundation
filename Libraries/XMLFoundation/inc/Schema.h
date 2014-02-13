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

#ifndef __SCHEMA_H__
#define __SCHEMA_H__

/*

	Schema.h

	Contains classes that describe database schema's

  	SQLColumn describes a column of a result set returned
	by a stored procedure or the column	of a table.

	SQLResultColumns describes the columns that are returns
	when a SQL select statement or stored procedure is
	executed.

	SQLParameter describes an input/output parameter of
	a stored procedure

	SQLProcedure describes a stored procedures parameter list,
	result set columns and the text used to create the procedure.
	This object will contain a list of SQLColumn objects and
	a list of SQLParameter objects.

	SQLTable describes a table in tems of its column's.  This
	object will contain a list of SQLColumn objects.

	SQLSchema describes the tables and procedures of a database
	schema.  This object will contain the name of the schema,
	a list of SQLTable objects and a list of SQLProcedure objects.

	SQLSchemas describes the available schemas available.
	This object will contain a list of SQLSchema objects.

*/

#include "xmlObject.h"
#include "GString.h"
#include "ObjectPointer.h"

///////////////////////////////////////////////////////////////////////////////
// SQLColumn
class SQLColumn : public XMLObject
{
protected:
	void MapXMLTagsToMembers();

	GString m_strName;		// Name of the column
	int		m_nType;		// DB_TYPE of the column
	int		m_nLength;		// Length of the column
	int		m_nPrecision;	// Precision of the column
	int		m_nScale;		// Scale of the column
	int		m_nIndex;		// index of this SQLColumn within a list
public:
	
	DECLARE_FACTORY(SQLColumn, column)

	const char *GetName();
	const char *GetDatatypeDescription();
	int GetDatatype();
	int GetLength();
	int GetPrecision();
	int GetScale();
	int GetIndex() {return m_nIndex;};
	void SetIndex(int n) {m_nIndex = n;};


	SQLColumn();
	~SQLColumn();
};

///////////////////////////////////////////////////////////////////////////////
// SQLParameter 
class SQLParameter : public XMLObject
{
protected:
	void MapXMLTagsToMembers();

	GString m_strName;		// Name of the parameter
	int		m_nType;		// DB_TYPE of the parameter
	int		m_nLength;		// Length of the parameter
	int		m_nDirection;	// Direction of the parameter (input/output/in out)

public:
	
	GString m_strValue;		// Storage for application data

	DECLARE_FACTORY(SQLParameter, parameter)

	const char *GetName();
	const char *GetDatatypeDescription();
	int GetDatatype();
	int GetLength();
	int GetDirection();

	SQLParameter();
	~SQLParameter();
};

///////////////////////////////////////////////////////////////////////////////
// SQLTable 
class CXMLDataSource;
class SQLTable : public XMLObject
{
	bool m_bHasFetched;
	void Fetch();

	CXMLDataSource *m_pBroker;
	GString m_strDBDriver;
	GString m_strSchema;
	GString m_strServer;
	GString m_strDBUser;
	GString m_strDBPass;
	GString m_strTable;

protected:
	void MapXMLTagsToMembers();

	GString m_strName;			// Name of the table
	GList m_lstColumns;	// Column list
	SQLTable(){}
public:

	DECLARE_FACTORY(SQLTable, table)

	void SetTable(const char *pzTable);
	const char   *GetName();
	GList *GetColumnList();

	SQLTable( CXMLDataSource *pBroker,
				  const char *pzTable,
				  
				  // optional to override the [default] in txml.txt on the broker.
				  const char *pzDBDriver = 0,

				  // optional to change the default logon stored in txml.txt
				  // for the driver specified in the previous param.  
				  const char *pzDBServer = 0,
				  const char *pzDBSchema = 0,
				  const char *pzDBUser = 0,
				  const char *pzDBPass = 0);

	~SQLTable();
};

///////////////////////////////////////////////////////////////////////////////
// SQLProcedure 
class CXMLDataSource;
class SQLProcedure : public XMLObject
{
	bool m_bHasFetched;
	void Fetch();
	GStringList m_strParamList;

	CXMLDataSource *m_pBroker;
	GString m_strQuery;
	
	GString m_strDBDriver;
	GString m_strDBServer;
	GString m_strSchema;
	GString m_strDBUser;
	GString m_strDBPass;
protected:
	void MapXMLTagsToMembers();

	int nColumnCount;
	GString m_strName;				// Name of the table
	GList m_lstParameters;	// Parameter list
	GList m_lstColumns;		// Column list
	GString m_strText;				// Procedure text
	SQLProcedure(){}

public:
	DECLARE_FACTORY(SQLProcedure, procedure)
	
	SQLProcedure( CXMLDataSource *pBroker,
				  const char *pzQuery,
				  
				  // optional to override the [default] in txml.txt on the server.
				  const char *pzDBDriver = 0,

				  // optional to change the default logon stored in txml.txt
				  // for the driver specified in the previous param.  
				  const char *pzDBServer = 0,
				  const char *pzDBSchema = 0,
				  const char *pzDBUser = 0,
				  const char *pzDBPass = 0);
	
	
	// in most cases AddParameter() does not need to be used.  SQL or SP's
	// can be described without passing execution arguments.  If the command
	// is an insert or an SP with an insert that requires valid key constraint
	// Parameters then you should supply them here.  This provides the driver
	// with enough information to successfully execute pzQuery to determine
	// it's result set then rollback to reverse Database modifications.
	void AddParameter(const char *pzName, const char *pzValue);

	
	// return detail of the current query
	const char   *GetName();
	const char   *GetText();
	GList *GetColumnList();
	GList *GetParameterList();
	
	// -1 if unknown, otherwise the number of columns
	int GetColumnCount();

	CXMLDataSource *GetDataSource(){return m_pBroker;};

	// Sets the current query that the Get*'s describe
	void ReSetQuery(const char *pzNextQuery);
	
	~SQLProcedure();
};

///////////////////////////////////////////////////////////////////////////////
// SQLSchema
class SQLSchema : public XMLObject
{
	bool m_bHasFetched;

	CXMLDataSource *m_pBroker;

	GString m_strDriver;
	GString m_strServer;
	GString m_strSchema;
	GString m_strUser;
	GString m_strPass;

	void Fetch();

protected:
	void MapXMLTagsToMembers();

	GString m_strName;				// Name of the schema
	GList m_lstTables;		// Table list
	GList m_lstProcedures;	// Procedure list
	SQLSchema(){};

public:
	DECLARE_FACTORY(SQLSchema, schema)

	const char   *GetName();
	GList *GetTableList();
	GList *GetProcedureList();

	SQLSchema( CXMLDataSource *pBroker,
				  
				  // optional to override the [default] in txml.txt on the broker.
				  const char *pzDBDriver = 0,

				  // optional to change the default logon stored in txml.txt
				  // for the driver specified in the previous param.  
				  const char *pzDBServer = 0,
				  const char *pzDBSchema = 0,
				  const char *pzDBUser = 0,
				  const char *pzDBPass = 0);
	~SQLSchema();
};

///////////////////////////////////////////////////////////////////////////////
// SQLSchemas
class CXMLDataSource;
class SQLSchemas : public XMLObject
{
	CXMLDataSource *m_broker;

protected:
	void MapXMLTagsToMembers();

	GList m_lstSchemas;	// Schema list

public:
	DECLARE_FACTORY(SQLSchemas, schemas)

	// Old-style interface, do not use
	SQLSchemas();
	GList *GetSchemaList();

	// New style interface
	SQLSchemas( CXMLDataSource *pBroker ){m_broker = pBroker;}
	GList *GetSchemaList(const char *pzDBServer,
								const char *pzDBUser,
								const char *pzDBPass,
								const char *pzDataSource);

	~SQLSchemas();
};

///////////////////////////////////////////////////////////////////////////////
// XMLRule
class XMLRule : public XMLObject
{
protected:
	void MapXMLTagsToMembers();

	GString m_strName;				// Name of the rule

public:
	DECLARE_FACTORY(XMLRule, File)

	const char   *GetName();

	XMLRule();
	~XMLRule();
};

///////////////////////////////////////////////////////////////////////////////
// XMLRules
class XMLRules : public XMLObject
{
protected:
	void MapXMLTagsToMembers();

	GList m_lstRules;	// Rule list

public:
	DECLARE_FACTORY(XMLRules, rules)

	GList *GetRuleList();

	XMLRules();
	~XMLRules();
};

///////////////////////////////////////////////////////////////////////////////
// XMLDriver
class XMLDriver : public XMLObject
{
protected:
	void MapXMLTagsToMembers();

	GString m_strName;				// Name of the driver

public:
	DECLARE_FACTORY(XMLDriver, driver)

	const char   *GetName();

	XMLDriver();
	~XMLDriver();
};

class XMLDriverCommandTemplate : public XMLObject
{
protected:
	void MapXMLTagsToMembers();

	GString m_strName;
	GString m_strCommandTemplate;

public:
	DECLARE_FACTORY(XMLDriverCommandTemplate, CommandTemplate)
	const char   *GetName() {return m_strName;}
	const char   *GetTemplate() {return m_strCommandTemplate;}

	XMLDriverCommandTemplate(){};
	~XMLDriverCommandTemplate(){};
};


///////////////////////////////////////////////////////////////////////////////
// XMLDrivers
class CXMLDataSource;
class XMLDrivers : public XMLObject
{
	CXMLDataSource *m_broker;
	int m_bHasLoadedDBMSDetail;
	void LoadDBMSDriverDetail();

protected:
	void MapXMLTagsToMembers();
	GList m_lstCommandTemplates;	// XMLDriverCommandTemplate's
	GString *m_pDriverRTFHELP;	// allocated in 'ctor for buffer prealloc

	GList m_lstDrivers;	// XMLDriver's
	GStringList m_lstObjectFiles;

	XMLDrivers();
public:
	DECLARE_FACTORY(XMLDrivers, drivers)
	
	XMLDrivers( CXMLDataSource *broker );

	// DataSource Drivers
	GList *GetDataSourceDrivers();

	// Language Drivers
	GStringList *GetLanguageDrivers();


	const char *GetDriverRTFDoc(const char *pzDriverName, bool bShared);
	void SetDriverRTFDoc(const char *pzDriverName, const char *pzRTF);
	GList *GetDriverCommandTemplates(const char *pzDriverName);
	GList *GetCmdTemplateList(){return &m_lstCommandTemplates;}


	//	For backward compilation compatibility only,
	const char * GetDefaultDriver(){return "";}
	const char * GetDefaultUser(){return "";}
	const char * GetDefaultPassword(){return "";}
	const char * GetDefaultServer(){return "";}

	~XMLDrivers();
};

class CXMLDataSource;
class ComponentInterface : public XMLObject 
{
	CXMLDataSource *m_broker;
	GString m_strModelTypeName;
	GStringList m_ArgumentList;
	
	// Invoke(const char *) defaults
	GString m_strComponent; 
	GString m_strInterface;

protected:
	ComponentInterface( ){};
	void MapXMLTagsToMembers();

	GStringList m_lstObjectFiles;
	GStringList m_lstMethods;
	GStringList m_lstInterfaces;

	// m_pParamTypeList[0] is the type for parameter m_pParamNameList[0]
	GStringList m_pParamNameList;
	GStringList m_pParamTypeList;
	GString m_strReturnType;
	GString m_strInvokeResults;

	int m_nExceptionFlags;
	GString m_strExceptionMatch;

public:
	DECLARE_FACTORY(ComponentInterface, ComponentInterface)
	

	GStringList* GetComponents(const char *pzModel);
	GStringList* GetInterfaces(const char *pzModel, const char *pzComponent);
	GStringList* GetMethods(const char *pzModel, const char *pzComponent, const char *pzInterface);
	GStringList* GetMethodParams(const char *pzModel, const char *pzObject, const char *pzInterface, const char *pzMethod);

	// call repeatedly to batch up arguments prior to Invoke()
	void AddArgument(const char *pzName, const char *pzValue);
	void AddArgument(const char *pzName, int nValue);
	
	// use SetScope() and full c'tor together with the short Invoke()
	void SetScope ( const char *pzComponent, const char *pzInterface );
	const char *Invoke (const char *pzMethodIn);
	// or provide all data on each call to Invoke()
	const char * Invoke(const char *pzModel, const char *pzObject, const char *pzInterface, const char *pzMethod);
	
	// Scans an input string for criteria that matches an exception case.  
	static void ThrowException(int nExceptionFlags ,const char *pzExceptionMatch, const char *pzData, const char *pzExecSignature );
	
	GStringList* GetMethodParamTypes();
	const char * GetMethodReturnType();

	ComponentInterface( CXMLDataSource *broker,	// required
			const char *pzComponentModelType = 0,// required for use of short Invoke() 
			int nExceptionFlags = 0,			// optional to cause Invoke() to throw
			const char *pzExceptionMatch = 0 );	// optioanl to cause Invoke() to throw
	
	~ComponentInterface();
};

#define FORMAT_USER_DEFINED		0
#define FORMAT_COMMA_SEPERATED	1
#define FORMAT_HTML				2


// These structures are used by DesignerXML to pass all the information
// necessary to construct a single node of the Hierarchy.  One 
// overloaded construction method of CHierarchy takes an ItemDescriptionData * 

//typedef enum InputType
enum InputType
{
	ITNotSet = -1,
	ITParameter = 0,// parameter of the original query
	ITPeerNode,		// the value of a peer node
	ITConstant,		// a constant string value
	ITDBMSColumn	// 0 based index into the DBMS result columns
};


class PageParam : public XMLObject
{
	DECLARE_FACTORY(PageParam, XMLGraftParam)
	virtual void MapXMLTagsToMembers();
	GString		strBatchedParamValue;	// used at runtime to store param value after resolved
public:
	void SetBatchValue(const char *p){ strBatchedParamValue = p; }
	const char *GetBatchValue(){ return strBatchedParamValue; }
	GString		strParameterType;
	GString		strParameterName;
	InputType	nIT;	// type of parameter input
	GString		strIT;	// parameter name, peer node tag, constant value

	PageParam() : nIT((InputType)0)
	{
	};
	PageParam( PageParam *cpyFrom )
	{
		strBatchedParamValue = cpyFrom->strBatchedParamValue;
		strParameterType = cpyFrom->strParameterType;
		strParameterName = cpyFrom->strParameterName;
		nIT = cpyFrom->nIT;
		strIT = cpyFrom->strIT;
	};
};

// interface to Cache and Disk exposed 
// only through development servers.
class DevServerAdmin : public XMLObject
{
	CXMLDataSource *m_broker;

	GStringList m_lstFiles;
	GStringList m_lstCache;

	GString		m_strFile;
	
	GString		m_strCacheXML;

public:
	DECLARE_FACTORY(DevServerAdmin, Admin)
	DevServerAdmin(){}
	DevServerAdmin(CXMLDataSource *p){m_broker = p;}
	void MapXMLTagsToMembers();
	
	// Disk Interface
	GStringList*FileList(const char *pzPath);
	const char *FileView(const char *pzPathAndFile);
	void		FilePut(const char *pzPathAndFile, const char *pzData, int nDataLen);

	// XMLObject Cache
	GStringList*ViewCacheObjects();	
	const char *ViewCacheObjectXML(const char *OIDClass);
	void		FreeObjectCache();
	void		FreeOneObject(const char *OIDClass);

	// Component Cache
	void		FreeRule(const char *pzRule);
	void		FreeScript(const char *pzScript);
	void		FreeXSL(const char *pzXSL);
};



class ExternalCall : public XMLObject
{
public:
	DECLARE_FACTORY(ExternalCall, ExternalCall)
	void MapXMLTagsToMembers();
	
	int m_bIsRemote; // 1 if the Server is NOT the same machine we are on.

	// input parameters for Grafting or Object Extensions Invoke()
	GList m_lstInputParameters;	// contains PageParam's
	// *Graft or Extension Location
	GString m_strServer;
	int m_nPort;
	GString m_strStaticUser;	// login credentials must exist if nUseRunTimeUserPass = 0
	GString m_strStaticPass;
	int m_nUseRunTimeUserPass; //1 - uses the credentials of the active Document
	// *Invoke Methodalogy
	GString m_strComponent;
	GString m_strInterface;
	GString m_strMethod;
	GString m_strComponentType; //COM, CStdCall, Java, TXML, Perl, Python etc
	int m_nExceptionFlags; // like EX_EMPTYORNULL | EX_XMLEXCEPTION
	GString m_strExceptionMatch;
	const char *GetMethodSignature(GString &strSignature, bool bUseValues, GList *pValueList = 0);
	///******************* Graft and Component Extension Data *******************

};

class MarkupColProperties : public XMLObject
{
public:
	DECLARE_FACTORY(MarkupColProperties, MarkupColProperties)
	virtual void MapXMLTagsToMembers();
	int nColumn;
	GString strColumnName;
	GString strColumnNameOverride;
	int bIsIncluded;
	int nFixedWidth;

	ObjectPtr <ExternalCall> m_ptrExternalCall;
	MarkupColProperties()
	{
		bIsIncluded = 1;
		nFixedWidth = -1;
	}

};

class NonXMLResultsMarkup : public XMLObject
{
	MarkupColProperties *FindColumnMarkup(int nColIndex,const char *pzQueryColumnName, int bCreate = 0)
	{
		GListIterator it(&lstMarkupColProperties);
		while (it())
		{
			MarkupColProperties *p = (MarkupColProperties *)it++;
			if (p->nColumn == nColIndex && ( p->strColumnName.CompareNoCase(pzQueryColumnName) == 0) )
			{
				return p;
			}
		}
		if (bCreate)
		{
			MarkupColProperties *p = new MarkupColProperties();
			p->nColumn = nColIndex;
			p->strColumnName = pzQueryColumnName;
			lstMarkupColProperties.AddLast(p);
			return p;
		}
		return 0;
	}
public:
	DECLARE_FACTORY(NonXMLResultsMarkup, NonXMLResultsMarkup)
	virtual void MapXMLTagsToMembers();
	// Structure of external call information or NULL
	ExternalCall *GetExternalCall(int nColIndex,const char *pzQueryColumnName, int bCreate = 0)
	{
		MarkupColProperties *pMCP = FindColumnMarkup(nColIndex,pzQueryColumnName,bCreate);
		if (pMCP)
		{
			if (pMCP->m_ptrExternalCall.isValid())
				return pMCP->m_ptrExternalCall;
			else if (bCreate)
			{
				ExternalCall *p = new ExternalCall();
				pMCP->m_ptrExternalCall.assign(p);;
				return p;
			}
		}
		return 0;
	}
	void SetExternalCall(int nColIndex, const char *pzQueryColumnName, ExternalCall *pExt)
	{
		if (pExt)
		{
			MarkupColProperties *pMCP = FindColumnMarkup(nColIndex,pzQueryColumnName,1);
			if (pMCP)
			{
				pMCP->m_ptrExternalCall.assign(pExt);
			}
		}
	}

	// Should this Column be included in the results
	int IncludeColumn(int nColIndex,const char *pzQueryColumnName)
	{
		MarkupColProperties *pMCP = FindColumnMarkup(nColIndex,pzQueryColumnName);
		if (pMCP)
		{
			return pMCP->bIsIncluded;
		}
		return 1;
	}
	void SetIncludeColumn(int nColIndex,const char *pzQueryColumnName, int bIncluded)
	{
		MarkupColProperties *pMCP = FindColumnMarkup(nColIndex,pzQueryColumnName,1);
		pMCP->bIsIncluded = bIncluded;
	}

	// -1 is "Natural Width" 0 or positive is the fixed width
	int ColumnFixedWidth(int nColIndex,const char *pzQueryColumnName)
	{
		MarkupColProperties *pMCP = FindColumnMarkup(nColIndex,pzQueryColumnName);
		if (pMCP)
		{
			return pMCP->nFixedWidth;
		}
		return -1;
	}
	void SetColumnFixedWidth(int nColIndex,const char *pzQueryColumnName, int nWidth)
	{
		MarkupColProperties *pMCP = FindColumnMarkup(nColIndex,pzQueryColumnName,1);
		pMCP->nFixedWidth = nWidth;
	}

	// The output heading 
	const char *GetColumnHeader(int nColIndex,const char *pzQueryColumnName)
	{
		MarkupColProperties *pMCP = FindColumnMarkup(nColIndex,pzQueryColumnName);
		if (pMCP)
			return pMCP->strColumnNameOverride;
		return pzQueryColumnName;
	}
	void SetColumnHeader(int nColIndex,const char *pzQueryColumnName, const char *pzNewHeader)
	{
		MarkupColProperties *pMCP = FindColumnMarkup(nColIndex,pzQueryColumnName,1);
		pMCP->strColumnNameOverride = pzNewHeader;
	}
	

	GList lstMarkupColProperties; // MarkupColProperties's
	GString strHeader;
	
	GString strPreColumn;
	GString strPostColumn;
	GString strColumnSeperator;
	
	GString strEndOfColumns;

	GString strBeginDataRow;
	GString strPreData;
	GString strPostData;
	GString strDataSeperator;
	GString strEndDataRow;

	GString strFooter;
	GString strGlobalEscapes;
	GString strViewExt;

	~NonXMLResultsMarkup()
	{
		GListIterator it(&lstMarkupColProperties);
		while (it())
			delete (MarkupColProperties *)it++;
	}

	NonXMLResultsMarkup(){}
	NonXMLResultsMarkup(int nType)
	{
		switch (nType)
		{
		case FORMAT_HTML:
			strHeader = "<HTML><HEAD><TITLE>Results</TITLE></HEAD><BODY BGCOLOR=\"#FFFFFF\"><A NAME=\"topanchor\"><TABLE BORDER=1><TR>";
			strPreColumn = "<TH ALIGN=CENTER>";
			strPostColumn = "</TH>";
			strColumnSeperator = "";
			strEndOfColumns = "</TR>";
			strBeginDataRow = "<TR>";
			strPreData = "<TD ALIGN=CENTER>";
			strPostData = "</TD>";
			strDataSeperator = "";
			strEndDataRow = "</TR>";
			strFooter = "</TABLE></CENTER></BODY></HTML>";
			strGlobalEscapes= "&=&amp;&&'=&#39;&&<=&lt;&&>=&gt;&&\"=&quot;"; 
			strViewExt = "html";
			break;
		case FORMAT_COMMA_SEPERATED:
			strHeader = "";
			strPreColumn = "[";
			strPostColumn = "]";
			strColumnSeperator = ",";
			strEndOfColumns = "\n";
			strBeginDataRow = "";
			strPreData = "";
			strPostData = "";
			strDataSeperator = ",";
			strEndDataRow = "\n";
			strFooter = "";
			strGlobalEscapes= "\"=\"\""; // " = "" Quote is replace with QuoteQuote
			strViewExt = "txt";
			break;
		case FORMAT_USER_DEFINED:
			// Set the member variables yourself.
			break;
		}
	}
};


#endif // __SCHEMA_H__
