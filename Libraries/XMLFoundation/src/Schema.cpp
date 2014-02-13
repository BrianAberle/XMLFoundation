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

#include "Schema.h"
#include "ProcedureCall.h"
#include "GException.h"
#include "xmlElementTree.h" // included for the CXMLTree in static method ThrowException()
#include "xmlElement.h"		// included for the CXMLTree in static method ThrowException()

#include <string.h> // for: strstr()
#include <stdlib.h> // for: atoi()

// Graft/Extension Exception Flags copied from /Server/xmlHierarchy.h
#define EX_EMPTYORNULL					0x01
#define EX_RETURN_EQUAL					0x02
#define EX_RETURN_CONTAINS				0x04
#define EX_RETURN_STARTSWITH			0x08
#define EX_XMLEXCEPTION					0x10


// SQL data type codes  copied from from /Server/xmldbc.h 
#define	SQL_UNKNOWN_TYPE	0
#define DB_CHAR            1
#define DB_VARCHAR        12
#define DB_NUMERIC         2
#define DB_INTEGER         4
#define DB_BIGINT        (-5)
#define DB_BIT           (-7)
#define DB_TINYINT       (-6)
#define DB_SMALLINT        5
#define DB_FLOAT           6
#define DB_DECIMAL         3
#define DB_REAL            7
#define DB_DOUBLE          8
#define DB_DATE            9
#define DB_TIME           10
#define DB_TIMESTAMP      11
#define DB_TYPE_DATE      91
#define DB_TYPE_TIME      92
#define DB_TYPE_TIMESTAMP 93
#define DB_DEFAULT        99
#define DB_LONGVARCHAR   (-1)
#define DB_BINARY        (-2)
#define DB_VARBINARY     (-3)
#define DB_LONGVARBINARY (-4)


const char *GetDatatype(int nType)
{
	const char *szRet = 0;

	switch (nType)
	{
	case DB_CHAR :
		szRet = "Char";
		break;
	case DB_VARCHAR :
		szRet = "VarChar";
		break;
	case DB_NUMERIC :
		szRet = "Numeric";
		break;
	case DB_INTEGER :
		szRet = "Integer";
		break;
	case DB_BIGINT :
		szRet = "Big Integer";
		break;
	case DB_BIT :
		szRet = "Bit";
		break;
	case DB_TINYINT :
		szRet = "Tiny Integer";
		break;
	case DB_SMALLINT :
		szRet = "Small Integer";
		break;
	case DB_FLOAT :
		szRet = "Float";
		break;
	case DB_DECIMAL :
		szRet = "Decimal";
		break;
	case DB_REAL :
		szRet = "Real";
		break;
	case DB_DOUBLE :
		szRet = "Double";
		break;
	case DB_TYPE_DATE :
	case DB_DATE :
		szRet = "Date";
		break;
	case DB_TYPE_TIME :
	case DB_TIME :
		szRet = "Time";
		break;
	case DB_TYPE_TIMESTAMP :
	case DB_TIMESTAMP :
		szRet = "Time Stamp";
		break;
	case DB_LONGVARCHAR :
		szRet = "Long VarChar";
		break;
	case DB_BINARY :
		szRet = "Binary";
		break;
	case DB_VARBINARY :
		szRet = "VarBinary";
		break;
	case DB_LONGVARBINARY :
		szRet = "Long VarBinary";
		break;
	case DB_DEFAULT :
	default :
		szRet = "Default";
		break;
	}

	return szRet;
}

///////////////////////////////////////////////////////////////////////////////
// SQLColumn
IMPLEMENT_FACTORY(SQLColumn, column);

void SQLColumn::MapXMLTagsToMembers()
{
	MapMember(&m_strName, "name");
	MapMember(&m_nType, "type");
	MapMember(&m_nLength, "length");
	MapMember(&m_nPrecision, "precision");
	MapMember(&m_nScale, "scale");
	MapMember(&m_nIndex, "index");
}

const char *SQLColumn::GetName()
{
	return (const char *)m_strName;
}

const char *SQLColumn::GetDatatypeDescription()
{
	return ::GetDatatype(m_nType);
}

int SQLColumn::GetDatatype()
{
	return m_nType;
}

int SQLColumn::GetLength()
{
	return m_nLength;
}

int SQLColumn::GetPrecision()
{
	return m_nPrecision;
}

int SQLColumn::GetScale()
{
	return m_nScale;
}

SQLColumn::SQLColumn() :
	m_nType(0),
	m_nLength(0),
	m_nPrecision(0),
	m_nScale(0)
{
}

SQLColumn::~SQLColumn()
{
}

///////////////////////////////////////////////////////////////////////////////
// SQLParameter 
IMPLEMENT_FACTORY(SQLParameter, parameter);

void SQLParameter::MapXMLTagsToMembers()
{
	MapMember(&m_strName, "name");
	MapMember(&m_nType, "type");
	MapMember(&m_nLength, "length");
	MapMember(&m_nDirection, "direction");
}

const char *SQLParameter::GetName()
{
	return (const char *)m_strName;
}

const char *SQLParameter::GetDatatypeDescription()
{
	return ::GetDatatype(m_nType);
}

int SQLParameter::GetDatatype()
{
	return m_nType;
}

int SQLParameter::GetLength()
{
	return m_nLength;
}

int SQLParameter::GetDirection()
{
	return m_nDirection;
}

SQLParameter::SQLParameter()
{
}

SQLParameter::~SQLParameter()
{
}


///////////////////////////////////////////////////////////////////////////////
// SQLTable 
IMPLEMENT_FACTORY(SQLTable, table);

void SQLTable::MapXMLTagsToMembers()
{
	MapMember(&m_strName, "name");
	MapMember(&m_lstColumns, SQLColumn::GetStaticTag());
}

void SQLTable::SetTable(const char *pzTable)
{
	m_strTable = pzTable;
	m_bHasFetched = false;
}

void SQLTable::Fetch()
{
	if (!m_bHasFetched)
	{
		XMLProcedureCall proc(m_pBroker, "DescribeTable");
		proc.AddAttribute( "DescribeTable", "yes" );
		
		if (m_strDBDriver.Length())
			proc.AddAttribute( "Driver", m_strDBDriver );
		if (m_strSchema.Length())
			proc.AddAttribute( "Schema", m_strSchema );
		if (m_strServer.Length())
			proc.AddAttribute("Server", m_strServer);
		if (m_strDBUser.Length())
			proc.AddAttribute("DBUser", m_strDBUser);
		if (m_strDBPass.Length())
			proc.AddAttribute("DBPassword", m_strDBPass);
		if (m_strTable.Length())
			proc.AddParameter("Table", m_strTable );
		proc.Execute(this);
		m_bHasFetched = true;
	}
}

SQLTable::SQLTable(
				  CXMLDataSource *pBroker,
				  const char *pzTable,
					  
				  // optional to override the [default] in txml.txt on the broker.
				  const char *pzDBDriver/* = 0*/,
	
				  // optional to change the default logon stored in txml.txt
				  // for the driver specified in the previous param.  
				  const char *pzDBServer/* = 0*/,
				  const char *pzDBSchema/* = 0*/,
				  const char *pzDBUser/* = 0*/,
				  const char *pzDBPass/* = 0*/ )
{
	m_bHasFetched = false;
	m_pBroker = pBroker;
	if (pzDBDriver)
		m_strDBDriver = pzDBDriver;
	if (pzDBServer)
		m_strServer = pzDBServer;
	if (pzDBSchema)
		m_strSchema = pzDBSchema;
	if (pzDBUser)
		m_strDBUser = pzDBUser;
	if (pzDBPass)
		m_strDBPass = pzDBPass;
	m_strTable = pzTable;
}

const char *SQLTable::GetName()
{
	Fetch();
	return (const char *)m_strName;
}

GList *SQLTable::GetColumnList()
{
	Fetch();
	return &m_lstColumns;
}


SQLTable::~SQLTable()
{
}


///////////////////////////////////////////////////////////////////////////////
// SQLProcedure 
IMPLEMENT_FACTORY(SQLProcedure, procedure);

void SQLProcedure::MapXMLTagsToMembers()
{
	MapMember(&nColumnCount, "ColumnCount");
	MapMember(&m_strName, "name");
	MapMember(&m_strText, "text");
	MapMember(&m_lstParameters, SQLParameter::GetStaticTag());
	MapMember(&m_lstColumns, SQLColumn::GetStaticTag());
}


void SQLProcedure::Fetch()
{
	if (!m_bHasFetched)
	{
		XMLProcedureCall proc;

		proc.SetDataSource( m_pBroker );
		proc.AddProcedure("describe-results", m_strQuery );
		proc.AddAttribute("DescribeProcedure", "yes");
		
		// If specific params have been specified use them.  
		// Otherwise the statement will be bound to spaces for 
		// intermediate execution to determine the result set.
		GStringIterator pIt(&m_strParamList);
		while(pIt())
		{
			const char *pzName  = pIt++; 
			const char *pzValue = pIt++; 
			proc.AddParameter(pzName, pzValue);
		}

		if (m_strDBDriver.Length())
			proc.AddAttribute("Driver", m_strDBDriver );

		if (m_strDBServer.Length())
			proc.AddAttribute("Server", m_strDBServer );
		
		if (m_strSchema.Length())
			proc.AddAttribute("Schema", m_strSchema );

		if (m_strDBUser.Length())
			proc.AddAttribute("DBUser", m_strDBUser );

		if (m_strDBPass.Length())
			proc.AddAttribute("DBPassword", m_strDBPass );

		proc.Execute(this, true);

		m_bHasFetched = true;
	}
}

void SQLProcedure::ReSetQuery(const char *pzNextQuery)
{
	m_strQuery = pzNextQuery;
	
	m_lstColumns.RemoveAll();
	m_lstParameters.RemoveAll();
	m_strName = "";
	m_strText = "";

	m_bHasFetched = false;
}

int SQLProcedure::GetColumnCount()
{
	Fetch();
	return nColumnCount;
}

const char *SQLProcedure::GetName()
{
	Fetch();
	return (const char *)m_strName;
}

const char *SQLProcedure::GetText()
{
	Fetch();
	return (const char *)m_strText;
}

GList *SQLProcedure::GetColumnList()
{
	Fetch();
	int i = 0;
	GListIterator it(&m_lstColumns);
	while (it())
	{
		SQLColumn *p = (SQLColumn *)it++;
		p->SetIndex(i++);
	}

	return &m_lstColumns;
}

GList *SQLProcedure::GetParameterList()
{
	Fetch();
	return &m_lstParameters;
}

void SQLProcedure::AddParameter(const char *pzName, const char *pzValue)
{
	m_bHasFetched = false;
	m_strParamList.AddLast(pzName);
	m_strParamList.AddLast(pzValue);
}


SQLProcedure::SQLProcedure( 
				  CXMLDataSource *pBroker,
				  const char *pzQuery,
					  
				  // optional to override the [default] in txml.txt on the broker.
				  const char *pzDBDriver/* = 0*/,
	
				  // optional to change the default logon stored in txml.txt
				  // for the driver specified in the previous param.  
				  const char *pzDBServer/* = 0*/,
				  const char *pzDBSchema/* = 0*/,
				  const char *pzDBUser/* = 0*/,
				  const char *pzDBPass/* = 0*/ )
{
	nColumnCount = -1;
	m_pBroker = pBroker;
	m_strQuery = pzQuery;
	if (pzDBDriver)
		m_strDBDriver = pzDBDriver;

	m_bHasFetched = false;

	if (pzDBDriver)
		m_strDBDriver = pzDBDriver;
	if (pzDBServer)
		m_strDBServer = pzDBServer;
	if (pzDBSchema)
		m_strSchema = pzDBSchema; 
	if (pzDBUser)
		m_strDBUser = pzDBUser;
	if (pzDBPass)	
		m_strDBPass = pzDBPass;
}

SQLProcedure::~SQLProcedure()
{
}


///////////////////////////////////////////////////////////////////////////////
// SQLSchema
IMPLEMENT_FACTORY(SQLSchema, schema);

void SQLSchema::MapXMLTagsToMembers()
{
	MapMember(&m_strName, "name");
	MapMember(&m_lstTables, SQLTable::GetStaticTag());
	MapMember(&m_lstProcedures, SQLProcedure::GetStaticTag());
}

const char  *SQLSchema::GetName()
{
	Fetch();
	return (const char *)m_strName;
}

void SQLSchema::Fetch()
{
	if (!m_bHasFetched)
	{
		XMLProcedureCall proc;
		proc.SetDataSource( m_pBroker );
		proc.AddAttribute( "DescribeTables", "yes" );
		proc.AddAttribute( "DescribeProcedures", "yes" );

		if (m_strSchema.Length())
			proc.SetSchema(m_strSchema);
    
		if (m_strDriver.Length())
			proc.AddAttribute("Driver", m_strDriver);
		
		if (m_strServer.Length())
			proc.AddAttribute("Server", m_strServer);

		if (m_strUser.Length())
			proc.AddAttribute("DBUser", m_strUser);

		if (m_strPass.Length())
			proc.AddAttribute("DBPassword", m_strPass);

		proc.Execute(this);
		m_bHasFetched = true;
	}
}

GList *SQLSchema::GetTableList()
{
	Fetch();
	return &m_lstTables;
}

GList *SQLSchema::GetProcedureList()
{
	Fetch();
	return &m_lstProcedures;
}

SQLSchema::SQLSchema( CXMLDataSource *pBroker,
				  
				  // optional to override the [default] in txml.txt on the broker.
				  const char *pzDBDriver/* = 0*/,

				  // optional to change the default logon stored in txml.txt
				  // for the driver specified in the previous param.  
				  const char *pzDBServer/* = 0*/,
				  const char *pzDBSchema/* = 0*/,
				  const char *pzDBUser/* = 0*/,
				  const char *pzDBPass/* = 0*/)
{
	m_bHasFetched = false;

	m_pBroker = pBroker;

	if (pzDBDriver)
		m_strDriver = pzDBDriver;
	if(pzDBServer)
		m_strServer = pzDBServer;
	if(pzDBSchema)
		m_strSchema = pzDBSchema;
	if(pzDBUser)
		m_strUser = pzDBUser;
	if (pzDBPass)
		m_strPass = pzDBPass;

}

SQLSchema::~SQLSchema()
{
}


///////////////////////////////////////////////////////////////////////////////
// SQLSchemas
IMPLEMENT_FACTORY(SQLSchemas, schemas);

void SQLSchemas::MapXMLTagsToMembers()
{
	MapMember(&m_lstSchemas, SQLSchema::GetStaticTag());
}



GList *SQLSchemas::GetSchemaList(const char *pzDBServer,
										const char *pzDBUser,
										const char *pzDBPass,
										const char *pzDataSource)
{
	XMLProcedureCall proc;
	proc.SetDataSource( m_broker );

	proc.AddAttribute("Server", pzDBServer );
	proc.AddAttribute("DBUser", pzDBUser );
	proc.AddAttribute("DBPassword", pzDBPass );
	proc.AddAttribute("Driver", pzDataSource );

	proc.AddAttribute( "GetSchemas", "yes" );
	proc.Execute(this);

	return &m_lstSchemas;
}

GList *SQLSchemas::GetSchemaList()
{
	return &m_lstSchemas;
}

SQLSchemas::SQLSchemas()
{
}

SQLSchemas::~SQLSchemas()
{
}


///////////////////////////////////////////////////////////////////////////////
// XMLRule
IMPLEMENT_FACTORY(XMLRule, File);

void XMLRule::MapXMLTagsToMembers()
{
	MapMember(&m_strName, "name");
}

const char  *XMLRule::GetName()
{
	return (const char *)m_strName;
}

XMLRule::XMLRule()
{
}

XMLRule::~XMLRule()
{
}


///////////////////////////////////////////////////////////////////////////////
// XMLRules
IMPLEMENT_FACTORY(XMLRules, rules);

void XMLRules::MapXMLTagsToMembers()
{
	MapMember(&m_lstRules, XMLRule::GetStaticTag());
}

GList *XMLRules::GetRuleList()
{
	return &m_lstRules;
}

XMLRules::XMLRules()
{
}

XMLRules::~XMLRules()
{
}

























///////////////////////////////////////////////////////////////////////////////
// XMLDriver
IMPLEMENT_FACTORY(XMLDriver, driver);

void XMLDriver::MapXMLTagsToMembers()
{
	MapMember(&m_strName, "name");
}

const char  *XMLDriver::GetName()
{
	return (const char *)m_strName;
}

XMLDriver::XMLDriver()
{
}

XMLDriver::~XMLDriver()
{
}


///////////////////////////////////////////////////////////////////////////////
// XMLDrivers
IMPLEMENT_FACTORY(XMLDriverCommandTemplate, CommandTemplate)

void XMLDriverCommandTemplate::MapXMLTagsToMembers()
{
	MapMember(&m_strName, "Command");
	MapMember(&m_strCommandTemplate, "Template");
}


IMPLEMENT_FACTORY(XMLDrivers, drivers);


XMLDrivers::XMLDrivers( CXMLDataSource *broker )
{
	m_broker = broker;
	m_bHasLoadedDBMSDetail = 0;

	// 100k buffer prealloc for help file incase images get embedded
	m_pDriverRTFHELP = new GString(100000);
}


void XMLDrivers::LoadDBMSDriverDetail()
{
	if (!m_bHasLoadedDBMSDetail)
	{
		XMLProcedureCall proc(m_broker,"Interface-XMLDrivers");
		proc.SetDataSource(m_broker);
		proc.AddAttribute( "GetDrivers", "yes" );
		proc.Execute(this);
		m_bHasLoadedDBMSDetail = 1;
	}
}

GStringList *XMLDrivers::GetLanguageDrivers()
{
	m_lstObjectFiles.RemoveAll();

	XMLProcedureCall proc(m_broker,"Interface-XMLDrivers");
	proc.AddAttribute( "GetLanguageDrivers", "yes" );
	proc.Execute(this);
	return &m_lstObjectFiles;
	
}


GList *XMLDrivers::GetDataSourceDrivers()
{
	LoadDBMSDriverDetail();
	return &m_lstDrivers;
}

void XMLDrivers::MapXMLTagsToMembers()
{
	MapMember(&m_lstObjectFiles, "Component","ComponentList");
	MapMember(&m_lstDrivers, XMLDriver::GetStaticTag() );
	MapMember(m_pDriverRTFHELP, "RtfHelp");
	MapMember(&m_lstCommandTemplates, XMLDriverCommandTemplate::GetStaticTag());
}


XMLDrivers::XMLDrivers()
{
	// 100k buffer prealloc for big help files
	m_pDriverRTFHELP = new GString(100000);
}

XMLDrivers::~XMLDrivers()
{
	delete m_pDriverRTFHELP;
}

const char *XMLDrivers::GetDriverRTFDoc(const char *pzDriverName, bool bShared )
{
	*m_pDriverRTFHELP = "";

	XMLProcedureCall proc(m_broker,"Interface-XMLDrivers");
	proc.AddAttribute( "GetDriverDoc", "yes" );
	proc.AddParameter( "Driver", pzDriverName );
	if (bShared)
		proc.AddParameter( "Mode", "Shared");
	else
		proc.AddParameter( "Mode", "ReadOnly");

	proc.Execute(this);

	return *m_pDriverRTFHELP;
}


void XMLDrivers::SetDriverRTFDoc(const char *pzDriverName, const char *pzRTF)
{
	XMLProcedureCall proc(m_broker,"Interface-XMLDrivers");
	proc.AddAttribute( "SetDriverDoc", "yes" );
	proc.AddParameter( "Driver", pzDriverName );
	proc.AddParameter( "Doc", pzRTF );
	proc.Execute(this);
}


GList *XMLDrivers::GetDriverCommandTemplates(const char *pzDriverName)
{
	XMLProcedureCall proc(m_broker,"Interface-XMLDrivers");
	proc.AddAttribute( "GetDriverCommandTemplates", "yes" );
	proc.AddParameter( "Driver", pzDriverName );
	proc.Execute(this);

	return &m_lstCommandTemplates;
}




///////////////////////////////////////////////////////////////////////////////
// ComponentInterface
IMPLEMENT_FACTORY(ComponentInterface, ComponentInterface)


void ComponentInterface::MapXMLTagsToMembers()
{
	
	MapMember(&m_strInvokeResults, "InvokeResults");
	MapMember(&m_strReturnType, "ReturnType");
	MapMember(&m_lstObjectFiles, "Component" ,"ComponentList");
	MapMember(&m_lstInterfaces, "Interface" ,"InterfaceList");
	MapMember(&m_lstMethods, "Method" ,"MethodList");
	MapMember(&m_pParamNameList, "Parameter" ,"ParamNameList");
	MapMember(&m_pParamTypeList, "DataType" ,"ParamTypeList");
}

const char * ComponentInterface::GetMethodReturnType()
{
	return m_strReturnType;
}


// pzModel supported interfaces are: "COM" "Java" "CStdCall"
GStringList *ComponentInterface::GetComponents( const char *pzModel )
{
	m_lstObjectFiles.RemoveAll();
	XMLProcedureCall proc(m_broker,"GetObjects");
	proc.AddAttribute( "GetObjects", "yes" );
	proc.AddAttribute( "Model",		pzModel );
	proc.Execute(this);
	return &m_lstObjectFiles;
}

GStringList *ComponentInterface::GetInterfaces( const char *pzModel, const char *pzComponent )
{
	m_lstInterfaces.RemoveAll();
	XMLProcedureCall proc(m_broker,"GetInterfaces");
	proc.AddAttribute( "GetInterfaces", "yes" );
	proc.AddAttribute( "Model",		pzModel );
	proc.AddParameter( "Component", pzComponent );
	proc.Execute(this);

	return &m_lstInterfaces;
}

GStringList *ComponentInterface::GetMethods( const char *pzModel, const char *pzComponent, const char *pzInterface )
{
	m_lstMethods.RemoveAll();

	XMLProcedureCall proc(m_broker,"GetMethods");
	proc.SetDataSource(m_broker);
	proc.AddAttribute( "GetMethods", "yes" );
	proc.AddAttribute( "Model",		pzModel );
	proc.AddParameter( "Component", pzComponent );
	proc.AddParameter( "Interface", pzInterface );
	proc.Execute(this);
	return &m_lstMethods;
}

void ComponentInterface::AddArgument(const char *pzName, int nValue)
{
	GString strConvert;
	strConvert.Format("%d",(int)nValue);
	AddArgument(pzName, (const char *)strConvert);
}

void ComponentInterface::AddArgument(const char *pzName, const char *pzValue)
{
	m_ArgumentList.AddLast(pzName);
	m_ArgumentList.AddLast(pzValue);
}

// optional call if COMPONENT::INTERFACE is not supplied to "Invoke(const char *)"
void ComponentInterface::SetScope ( const char *pzComponent, const char *pzInterface )
{
	m_strComponent = pzComponent; 
	m_strInterface = pzInterface;
}


// call AddArgument() repetedly for each argument before Invok()ing it.
// 1) Fully qualified -->"COMPONENT::INTERFACE::METHOD"
//			Example Invoke("MyJava.class::MyJava::DoIt")
//			full qualification is not necessary if SetScope() has been called.

// 2) partially qualified: "INTERFACE::METHOD"
//			Example Invoke("COMInterface::DoIt")
//			Overrides the default interface provided to SetScope().

// 3) partially qualified: "METHOD"
//			Example Invoke("DoIt")
//			Uses the default component & interface provided to SetScope().
const char *ComponentInterface::Invoke (const char *pzMethodIn)
{
	GString strMethodCopy = pzMethodIn;
	char *pzMethodParse = (char *)(const char *)strMethodCopy;

	char *pzComponent = 0;
	char *pzInterface = 0;
	char *pzMethod = 0;

	char *pz1 = strstr(pzMethodParse,"::");
	if (pz1)
	{
		pz1[0] = 0; 
		char *pz2 = strstr(pz1+2,"::");
		if (pz2)
		{
			pz2[0] = 0; 
			pzComponent = pzMethodParse;
			pzInterface = pz1+2;
			pzMethod = pz2+2;
		}
		else
		{
			pzInterface = pzMethodParse;
			pzMethod = pz1+2;
		}
	}
	GString strLastInvokedSignature = m_strModelTypeName;
	strLastInvokedSignature += "::";


	// allow override of component, and interface
	XMLProcedureCall proc(m_broker,"InvokeMethod");
	proc.AddAttribute( "InvokeMethod", "yes" );
	proc.AddAttribute( "Model",		m_strModelTypeName );
	if (pzComponent)
	{
		proc.AddParameter( "Component", pzComponent );
		strLastInvokedSignature += pzComponent;
	}
	else
	{
		proc.AddParameter( "Component", m_strComponent );
		strLastInvokedSignature += pzComponent;
	}
	strLastInvokedSignature += "::";

	if (pzInterface)
	{
		proc.AddParameter( "Interface",  pzInterface  );
		strLastInvokedSignature += pzInterface;
	}
	else
	{
		proc.AddParameter( "Interface",  m_strInterface  );
		strLastInvokedSignature += m_strInterface;
	}
	strLastInvokedSignature += "::";
	
	if (pzMethod)
	{
		proc.AddParameter( "Method",	 pzMethod  );
		strLastInvokedSignature += pzMethod;
	}
	else
	{
		proc.AddParameter( "Method",	 pzMethodIn  );
		strLastInvokedSignature += pzMethodIn;
	}
	strLastInvokedSignature += "(";

	
	// now add user params and Execute()
	GStringIterator it( &m_ArgumentList );
	int bAddComma = 0;
	while (it())
	{
		if (bAddComma)
			strLastInvokedSignature += ",";
		const char *pzName = it++;
		const char *pzValue = it++;
		proc.AddParameter( pzName, pzValue );
		strLastInvokedSignature += pzValue;
		bAddComma = 1;
	}
	strLastInvokedSignature += ")";
	try
	{
		proc.Execute(this);
	}
	catch( GException &ex)
	{
		throw GException("ComponentInterface", 2, ex.GetDescription(), (const char *)strLastInvokedSignature);
	}
	ThrowException(m_nExceptionFlags ,(const char *)m_strExceptionMatch, (const char *)m_strInvokeResults, (const char *)strLastInvokedSignature );

	return m_strInvokeResults;
}

// call AddArgument() repetedly for each argument before Invok()ing it.
const char *ComponentInterface::Invoke(const char *pzModel, const char *pzComponent, const char *pzInterface, const char *pzMethod)
{
	GString strLastInvokedSignature;
	strLastInvokedSignature.Format("%s::%s::%s(",pzComponent,pzInterface,pzMethod);

	XMLProcedureCall proc(m_broker,"InvokeMethod");
	proc.AddAttribute( "InvokeMethod", "yes" );
	proc.AddAttribute( "Model",		pzModel );
	proc.AddParameter( "Component", pzComponent );
	proc.AddParameter( "Interface", pzInterface );
	proc.AddParameter( "Method",	pzMethod );
	GStringIterator it( &m_ArgumentList );
	int bComma = 0;
	while (it())
	{
		if (bComma)
			strLastInvokedSignature += ",";
		const char *pzName = it++;
		const char *pzValue = it++;
		proc.AddParameter( pzName, pzValue );
		strLastInvokedSignature += pzValue;
	}
	strLastInvokedSignature += ")";

	proc.Execute(this);
	ThrowException(m_nExceptionFlags ,(const char *)m_strExceptionMatch, (const char *)m_strInvokeResults, (const char *)strLastInvokedSignature );
	return m_strInvokeResults;
}

GStringList* ComponentInterface::GetMethodParams(const char *pzModel, const char *pzComponent, const char *pzInterface, const char *pzMethod)
{
	m_pParamNameList.RemoveAll();

	XMLProcedureCall proc(m_broker,"GetMethodParams");
	proc.AddAttribute( "GetMethodParams", "yes" );
	proc.AddAttribute( "Model",		pzModel );
	proc.AddParameter( "Component", pzComponent );
	proc.AddParameter( "Interface", pzInterface );
	proc.AddParameter( "Method",	pzMethod );
	proc.Execute(this);
	
	return &m_pParamNameList;
}

GStringList* ComponentInterface::GetMethodParamTypes()
{
	return &m_pParamTypeList;
}

/*static*/
void ComponentInterface::ThrowException(int nExceptionFlags ,const char *pzExceptionMatch, const char *pzData, const char *pzExecSignature )
{
	if (!nExceptionFlags)
		return;

	// set defaults
	int bThrowOnNullOrEmpty = 0;
	int nComparisonType = 0;	//0=NoCompare
	int bLookForXMLException = 0;

	// extract bit flag settings
	if ( ( nExceptionFlags & EX_EMPTYORNULL ) != 0)
		bThrowOnNullOrEmpty = 1;
	if ( ( nExceptionFlags & EX_RETURN_EQUAL ) != 0)
		nComparisonType = EX_RETURN_EQUAL;
	if ( ( nExceptionFlags & EX_RETURN_CONTAINS ) != 0)
		nComparisonType = EX_RETURN_CONTAINS;
	if ( ( nExceptionFlags & EX_RETURN_STARTSWITH ) != 0)
		nComparisonType = EX_RETURN_STARTSWITH;
	if ( ( nExceptionFlags & EX_XMLEXCEPTION ) != 0)
		bLookForXMLException = 1;
	
	if (bThrowOnNullOrEmpty)
	{
		if (!pzExceptionMatch || !pzExceptionMatch[0])
			throw GException("CustomException", 1, pzExecSignature);
	}
	if (nComparisonType == EX_RETURN_EQUAL)
	{
		GString strEXMatch(pzExceptionMatch);
		if ( strEXMatch.CompareNoCase(pzData) == 0 )
			throw GException("CustomException", 3, pzExecSignature, pzData);
	}
	if (nComparisonType == EX_RETURN_STARTSWITH)
	{
		GString strEXMatch( pzExceptionMatch );
		if ( (int)strlen(pzData) >= strEXMatch.Length() )
		{
			GString strMatch(pzData,strEXMatch.Length());
			if ( strEXMatch.CompareNoCase((const char *)strMatch) == 0 )
				throw GException("CustomException", 4, pzExecSignature, (const char *)strMatch);
		}
	}
	if (nComparisonType == EX_RETURN_CONTAINS)
	{
		if ( strstr(pzData,pzExceptionMatch) )
		{
			throw GException("CustomException", 5, pzExecSignature, pzExceptionMatch);
		}
	}
	if ( bLookForXMLException )
	{
		int bIsXMLException = 0;
		try
		{
			CXMLTree xmlGraftTree;
			xmlGraftTree.parseXML( (char *)pzData );
			CXMLElement *pRoot = xmlGraftTree.getRoot();
			
			GString strRootTag(pRoot->getTag(), pRoot->getTagLen());
			if (strRootTag.CompareNoCase("Exception") == 0)
			{
				int nError = 0;
				CXMLElement *pE = pRoot->findChild("ErrorNumber");
				if (pE)
				{
					GString strE(pE->getTag(), pE->getTagLen());
					nError = atoi((const char *)strE);
				}

				int nSubSys = 0;
				CXMLElement *pS = pRoot->findChild("SubSystem");
				if (pS)
				{
					GString strS(pS->getTag(), pS->getTagLen());
					nSubSys = atoi((const char *)strS);
				}

				GString strDescription("User Defined Exception with no Description");
				CXMLElement *pD = pRoot->findChild("Description");
				if (pD)
				{
					GString strD(pD->getTag(), pD->getTagLen());
					strDescription = (const char *)strD;
				}

				GStringList Stack;
				CXMLElement *pC = pRoot->findChild("CallStack");
				if (pC)
				{
					int i = 0;
					while (1)
					{
						CXMLElement *pF = pC->findChild("Frame",++i);
						if (!pF)
							break;
						GString strF(pF->getTag(), pF->getTagLen());
						Stack.AddLast(strF);
					}
				}
				Stack.AddLast(pzExecSignature);

				bIsXMLException = 1;
				throw GException(nError, nSubSys, (const char *)strDescription, &Stack);
			}
		}
		catch( GException &)
		{
			if (bIsXMLException)
			{
				// propigate the user exception 
				throw;
			}
			else
			{
			// ignore this exception.  
			// If we can't parse the pzData then we know this is not an XMLException.  
			}
		}
	}
}

ComponentInterface::ComponentInterface( CXMLDataSource *broker, const char *pzComponentType /* = 0*/, int nExceptionFlags/* = 0*/, const char *pzExceptionMatch/* = 0*/ )
{
	m_nExceptionFlags = nExceptionFlags;
	m_strExceptionMatch = pzExceptionMatch;
	m_broker = broker;
	if (pzComponentType)
		m_strModelTypeName = pzComponentType;
}

ComponentInterface::~ComponentInterface()
{
}

void PageParam::MapXMLTagsToMembers()
{
	MapMember(&strParameterName, "Name");
	MapMember(&strParameterType, "ParamType");
	MapMember(&strIT, "Source");
	MapMember((int *)&nIT, "Type");
}


const char *ExternalCall::GetMethodSignature(GString &strSignature, bool bUseValues, GList *pValueList)
{
	strSignature.Format("%s::%s::%s::%s::%s(",
						(const char *)m_strServer,(const char *)m_strComponentType,
						(const char *)m_strComponent,(const char *)m_strInterface,
						(const char *)m_strMethod);

	// add the arguments for the Method/Graft we are invoking
	int bAddComma = 0;
	GList *pParamValueStorage = (pValueList) ? pValueList : &m_lstInputParameters;
	
	GListIterator it(pParamValueStorage);
	while (it())
	{
		if (bAddComma)
			strSignature += ",";
		PageParam *p = (PageParam *)it++;
				
		if (bUseValues)
			strSignature += p->GetBatchValue();
		else
		{
			strSignature += p->strParameterType;
			strSignature += " ";
			strSignature += p->strParameterName;
		}
		
		
		
		bAddComma = 1;
		strSignature += p->GetBatchValue();
	}
	strSignature += ")";
	return strSignature;
}


IMPLEMENT_FACTORY(NonXMLResultsMarkup, NonXMLResultsMarkup)
IMPLEMENT_FACTORY(MarkupColProperties, MarkupColProperties)
IMPLEMENT_FACTORY(PageParam, XMLGraftParam)
IMPLEMENT_FACTORY(ExternalCall, ExternalCall)
IMPLEMENT_FACTORY(DevServerAdmin, Admin)


void ExternalCall::MapXMLTagsToMembers()
{
	MapMember(&m_lstInputParameters, PageParam::GetStaticTag(), "ParamList");
	MapMember(&m_strServer, "ExternalComponentServer");
	MapMember((int *)&m_nPort,"ExternalComponentPort");
	MapMember(&m_strStaticUser, "ExternalComponentUser");
	MapMember(&m_strStaticPass, "ExternalComponentPass");
	MapMember((int *)&m_nUseRunTimeUserPass,"UseRunTimeUserPass");
	MapMember(&m_strComponent, "Component");
	MapMember(&m_strInterface, "Interface");
	MapMember(&m_strMethod, "Method");
	MapMember(&m_strComponentType,"ComponentType");
	MapMember(&m_strExceptionMatch, "ExceptionMatch");
	MapMember((int *)&m_nExceptionFlags,"ExceptionFlags");
	MapMember((int *)&m_bIsRemote,"IsRemote");
}

void MarkupColProperties::MapXMLTagsToMembers()
{
	MapMember(&strColumnName, "Name");
	MapMember(&strColumnNameOverride, "NameOverride");
	MapMember(&nColumn, "Column");
	MapMember(&m_ptrExternalCall, ExternalCall::GetStaticTag() );
	MapMember(&nFixedWidth, "FixedWidth");
	MapMember(&bIsIncluded, "IncludeColumn");
}

void NonXMLResultsMarkup::MapXMLTagsToMembers()
{
	MapMember(&lstMarkupColProperties, MarkupColProperties::GetStaticTag());
	MapMember(&strHeader, "Header");
	MapMember(&strPreColumn, "PreColumn");
	MapMember(&strPostColumn, "PostColumn");
	MapMember(&strColumnSeperator, "ColumnSeperator");
	MapMember(&strEndOfColumns, "EndOfColumns");
	MapMember(&strBeginDataRow, "BeginDataRow");
	MapMember(&strPreData, "PreData");
	MapMember(&strPostData, "PostData");
	MapMember(&strDataSeperator, "DataSeperator");
	MapMember(&strEndDataRow, "EndDataRow");
	MapMember(&strFooter, "Footer");
	MapMember(&strGlobalEscapes, "GlobalEscapes");
	MapMember(&strViewExt, "ViewerExtension");
}


void DevServerAdmin::MapXMLTagsToMembers()
{
	MapMember(&m_lstFiles, "File" ,"FileList");
	MapMember(&m_strFile, "FileData");
//	MapMember(&m_strCacheHTML, "CacheHTML");
	MapMember(&m_lstCache, "Object" ,"CacheList");
	MapMember(&m_strCacheXML, "CacheXML");
}

// Disk Interface
GStringList *DevServerAdmin::FileList(const char *pzPath)
{
	m_lstFiles.RemoveAll();

	XMLProcedureCall proc(m_broker,"ServerAdmin");
	proc.SetDataSource(m_broker);
	proc.AddAttribute( "ListFiles", "yes" );
	proc.AddParameter( "Path", pzPath );
	proc.Execute(this);
	
	return &m_lstFiles;
}

const char *DevServerAdmin::FileView(const char *pzPathAndFile)
{
	XMLProcedureCall proc(m_broker,"ServerAdmin");
	proc.SetDataSource(m_broker);
	proc.AddAttribute( "GetFile", "yes" );
	proc.AddParameter( "File", pzPathAndFile );
	proc.Execute(this);
	// uudecode
	return m_strFile;
}

void DevServerAdmin::FilePut(const char *pzPathAndFile, const char *pzData, int nDataLen)
{
	// uuencode
	XMLProcedureCall proc(m_broker,"ServerAdmin");
	proc.SetDataSource(m_broker);
	proc.AddAttribute( "FilePut", "yes" );
	proc.AddParameter( "FileName", pzPathAndFile );
	proc.AddParameter( "FileData", pzData );
	proc.Execute(this);
}

// an HTML view of all cached XMLObjects
GStringList *DevServerAdmin::ViewCacheObjects()
{
	m_lstCache.RemoveAll();

	XMLProcedureCall proc(m_broker,"ServerAdmin");
	proc.SetDataSource(m_broker);
	proc.AddAttribute( "ViewCacheObjects", "yes" );
	proc.Execute(this);

	return &m_lstCache;
}

// an XML view of a single Object
const char *DevServerAdmin::ViewCacheObjectXML(const char *pzOIDClass)
{
	XMLProcedureCall proc(m_broker,"ServerAdmin");
	proc.SetDataSource(m_broker);
	proc.AddAttribute( "ViewCacheObjectXML", "yes" );
	proc.AddParameter( "OIDClass", pzOIDClass );
	proc.Execute(this);

	return m_strCacheXML;
}

void DevServerAdmin::FreeObjectCache()
{
	XMLProcedureCall proc(m_broker,"ServerAdmin");
	proc.SetDataSource(m_broker);
	proc.AddAttribute( "FreeObjectCache", "yes" );
	proc.Execute(this);
}

void DevServerAdmin::FreeOneObject(const char *pzOIDClass)
{
	XMLProcedureCall proc(m_broker,"ServerAdmin");
	proc.SetDataSource(m_broker);
	proc.AddAttribute( "FreeOneObject", "yes" );
	proc.AddParameter( "OIDClass", pzOIDClass );
	proc.Execute(this);
}

void DevServerAdmin::FreeRule(const char *pzRule)
{
	XMLProcedureCall proc(m_broker,"ServerAdmin");
	proc.SetDataSource(m_broker);
	proc.AddAttribute( "FreeRule", "yes" );
	proc.AddParameter( "Rule", pzRule );
	proc.Execute(this);
}

void DevServerAdmin::FreeScript(const char *pzScript)
{
	XMLProcedureCall proc(m_broker,"ServerAdmin");
	proc.SetDataSource(m_broker);
	proc.AddAttribute( "FreeScript", "yes" );
	proc.AddParameter( "Script", pzScript );
	proc.Execute(this);
}

void DevServerAdmin::FreeXSL(const char *pzXSL)
{
	XMLProcedureCall proc(m_broker,"ServerAdmin");
	proc.SetDataSource(m_broker);
	proc.AddAttribute( "FreeXSL", "yes" );
	proc.AddParameter( "XSL", pzXSL );
	proc.Execute(this);
}
