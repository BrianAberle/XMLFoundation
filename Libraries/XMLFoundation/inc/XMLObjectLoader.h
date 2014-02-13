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



// The XMLObjLoader is a type of ObjQuery.
// It is an ObjQuery that uses a stream of XML directly
// from a flat file source rather than an RPC/Socket Call,
// this bypasses all the features in an XMLProcedure call
// for quick and simple Object access to persisted state.

#include "ObjQuery.h"

template <class TYPE> class XMLObjLoader : public ObjQuery<TYPE>
{
public:
	~XMLObjLoader( ) {}

	XMLObjLoader( ) : ObjQuery<TYPE>( "PersistedObject", 32767, false )
	{
	}

	ObjResultSet<TYPE> &LoadObjectFromMemory( const char *pxBufferOfXML ) 
	{
		// instanciate a XMLObjectFactory using the supplied XML
		XMLObjectFactory factory ( pxBufferOfXML );

		// 1. parses the XML
		// 2. creates any required objects on the heap, references what's 
		//    cached with proper oid's
		// 3. places pointers to CORBA objects in m_ObjectList
		factory.extractObjects(this);
		return MakeNewResultSet();
	}

	ObjResultSet<TYPE> &LoadObjectFromDisk( const char *pzFileContainingObjectState )
	{

		// load the object state into memory (in an RWCString)
		RWCString strXml;
		fstream fs( pzFileContainingObjectState,  ios::in);
 		if (fs.good())
		{
			strXml.readFile(fs);
			return LoadObjectFromMemory( strXml.data() );
		}
		cout << "Object state file could not be opened" << endl;
		// Generate an empty result set
		return MakeNewResultSet(); 
	}

	static void StoreObjectToDisk( XMLObject *pObject, const char *pzFileToStoreObjectState )
	{
		RWCString strXMLStream;	

		StoreObjectToMemory( pObject, strXMLStream );

		// write it to disk
		fstream fs(pzFileToStoreObjectState,  ios::out);
		fs << strXMLStream;
	
	}

	static void StoreObjectToMemory( XMLObject *pObject, RWCString &strXMLStreamDestinationBuffer )
	{
		// header
		strXMLStreamDestinationBuffer = "<?xml version=\"1.0\" standAlone='yes'?>\n";
		
		// modify to allow optional inclusion of non-dirty members
		pObject->ToXML( strXMLStreamDestinationBuffer, true, 0, 0, false );
	}

};