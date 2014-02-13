#ifdef ___VISIBROKER___   // Borland/Enterprise Studio
 #include <string.h>
 #include <fstream.h>
 #include "ExCORBA_c.hh"
 USE_STD_NS
#elif  ___ORBIX___		  // Iona/iPortal
 #include <it_cal/iostream.h>
 #include <it_cal/fstream.h>
 #include <omg/orb.hh>
 IT_USING_NAMESPACE_STD
 #include "ExCORBA.hh"
#elif  ___OBJECTBROKER___ // Weblogic/Tuxedo (Enterprise Edition)
#include <Tobj_Bootstrap.h>
#include <tobj_c.h>
 #include <fstream.h>
 #include "ExCORBA_c.h"
#endif

// The server object can be built as stateless or statefull.
// Generally in a stateless implementation, some type of 'handle',
// 'cookie', or 'session key'  is passed along with each method 
// call so that the server has a key to re-associate it's state.

// The IDL defines two sets of method entry points, one requiring
// a session key and one without.  This client application will
// act as stateless if the following line is enabled.  
// Tuxedo (WebLogic Enterprise) servers and MTS (Microsoft Transaction 
// Server) based objects must be stateless, other environments
// can be statefull or stateless.
#define RUN_STATELESS	1	

int main( int argc, char* argv[])
{
    try 
    {
	cout << "Initializing ORB" << endl;
	CORBA::ORB_var orb = CORBA::ORB_init(argc, argv);

	CORBA::Object_var        objref;
	ExCORBA::MyCORBAObject_var CustObject1;


#ifdef	___OBJECTBROKER___			// Get an object through Tuxedo
	Tobj_Bootstrap bootstrap(orb.in(), "");
	CORBA::Object_var v_fact_finder_oref =
	bootstrap.resolve_initial_references("FactoryFinder");
	Tobj::FactoryFinder_var v_fact_finder_ref =
	Tobj::FactoryFinder::_narrow(v_fact_finder_oref.in());
    CORBA::Object_var object= v_fact_finder_ref->find_one_factory_by_id( ExCORBA::_tc_MyCORBAObject->id()	);
#else
	cout << "Get a CORBA object reference through an IOR file" << endl;
	const int MAXBUF = 1024;
	char ior[MAXBUF];
	ifstream in("../ExCORBA.ior");
	in.getline(ior, MAXBUF);
	CORBA::Object_var object = orb->string_to_object(ior);
#endif

	
	// narrow the object into a MyCORBAObject that we call CustObject1 
	CustObject1 = ExCORBA::MyCORBAObject::_narrow(object);
	if (CORBA::is_nil(CustObject1))
	{
	    cerr << "Could not _narrow object to type MyCORBAObject" << endl;
	    return 1;
	}



    // 1.  Assign some state in a native IIOP call
	cout << "1.  Setting object state through native IIOP" << endl;
#ifdef RUN_STATELESS
	CustObject1->setState2(777,"Root",777);
#else
	CustObject1->setState("Root",777);
#endif


	// 2.  View the state of the object in XML
    cout << "2. getting object state in XML" << endl;
	CORBA::String_var s;
#ifdef RUN_STATELESS
	CustObject1->getXMLState2(777,s);
#else
	CustObject1->getXMLState(s);
#endif
	cout << (const char *)s << endl;

	// 3.  Update the state of the object through XML
	// change "Root" to "SuperUser" on customer ID 777
    cout << "3. setting object state through XML" << endl;
#ifdef RUN_STATELESS
	CustObject1->setXMLState2(777,"<MyCorbaImpl><CustomerID>777</CustomerID><CustomerName>SuperUser</CustomerName></MyCorbaImpl>");
#else
	CustObject1->setXMLState("<MyCorbaImpl><CustomerID>777</CustomerID><CustomerName>SuperUser</CustomerName></MyCorbaImpl>");
#endif


	// 4. (like 2) view the modified state of the object in XML
    cout << "4. view the modified object state in XML" << endl;
#ifdef RUN_STATELESS
	CustObject1->getXMLState2(777,s);
#else
	CustObject1->getXMLState(s);
#endif
	cout << (const char *)s;

	// 5. Create and initialize 2 new CORBA objects through XML
    cout << endl << "5. Add CORBA Sub-Objects through XML" << endl;
#ifdef RUN_STATELESS
	CustObject1->setXMLState2( 777,
		"<MyCorbaImpl>"
			"<CustomerID>777</CustomerID>"
			"<MyCorbaImpl>"
				"<CustomerID>123</CustomerID>"
				"<CustomerName>Al Gore</CustomerName>"
			"</MyCorbaImpl>"
			"<MyCorbaImpl>"
				"<CustomerID>456</CustomerID>"
				"<CustomerName>George Bush Jr.</CustomerName>"
			"</MyCorbaImpl>"
		"</MyCorbaImpl>");
#else
	CustObject1->setXMLState(
		"<MyCorbaImpl>"
			"<CustomerID>777</CustomerID>"
			"<MyCorbaImpl>"
				"<CustomerID>123</CustomerID>"
				"<CustomerName>Al Gore</CustomerName>"
			"</MyCorbaImpl>"
			"<MyCorbaImpl>"
				"<CustomerID>456</CustomerID>"
				"<CustomerName>George Bush Jr.</CustomerName>"
			"</MyCorbaImpl>"
		"</MyCorbaImpl>");
#endif

	// 6. (like 4 & 2) now view the base object's XML 
	// state which contains the new objects 123 & 456
    cout << "6. view new object state in XML" << endl;
#ifdef RUN_STATELESS
	CustObject1->getXMLState2(777,s);
#else
	CustObject1->getXMLState(s);
#endif
	cout << (const char *)s;

	// 7. Get a CORBA object reference for object instance 456.
    cout << endl << "7. Get a CORBA object reference for object instance 456" << endl;
	ExCORBA::MyCORBAObject_var CustObject2;
#ifdef RUN_STATELESS
	CustObject2 = CustObject1->getSubObjectIOR2(777,456);
#else
	CustObject2 = CustObject1->getSubObjectIOR(456);
#endif
	
	// 8. Dump the state of the Sub-Object using the IOR step 7 returned.
    cout << endl << "8. use an IOR of an object created in step 5" << endl;
	if (CustObject2 != CORBA::Object::_nil())
	{
#ifdef RUN_STATELESS
		CustObject2->getXMLState2(456, s);
#else
		CustObject2->getXMLState(s);
#endif
		cout << (const char *)s;
	}
	else
	{
		cout << "FAILED to retrieve an IOR for sub-object 456" << endl;
	}

	// 9. Step 5 added SubObjects through XML, add this one through native IIOP.
    cout << endl << "9. Add a Sub-Object without using XML....";
#ifdef RUN_STATELESS
	CustObject1->addSubObject2(777, "Michelangelo",1475);
#else
	CustObject1->addSubObject("Michelangelo",1475);
#endif

    cout << endl << "done" << endl;
	
	// . (like 7) Get a CORBA object reference for object instance 1475.
    cout << "10a. Get the IOR of the object we just created.....";
#ifdef RUN_STATELESS
	CustObject2 = CustObject1->getSubObjectIOR2(777,1475);
#else
	CustObject2 = CustObject1->getSubObjectIOR(1475);
#endif
    cout << "done" << endl;

	// 10. Dump the state of the Sub-Object 
    cout << "10. use an IOR of an object created in step 9" << endl;
	if (CustObject2 != CORBA::Object::_nil())
	{
#ifdef RUN_STATELESS
		CustObject2->getXMLState2(1475, s);
#else
		CustObject2->getXMLState(s);
#endif
		cout << (const char *)s;
	}else
	{
		cout << "FAILED to retrieve an IOR for sub-object 1475" << endl;
	}

	// 11. Free all the Sub-Objects
	cout << endl << "11. Deleting Sub objects" << endl;
#ifdef RUN_STATELESS
	CustObject1->delSubObjects2(777);
#else
	CustObject1->delSubObjects();
#endif

	// 12. get the state (now without the sub-objects)
	cout << "12. write out the state in XML (now without the sub-objects)" << endl;
#ifdef RUN_STATELESS
	CustObject1->getXMLState2(777, s);
#else
	CustObject1->getXMLState(s);
#endif
	cout << (const char *)s;

    }
    catch (const CORBA::Exception& e)
    {
#ifdef ___OBJECTBROKER___
	cout << "Exception occurred: " << e.get_id() <<  endl;
#else
	cout << "Exception occurred: " << e <<  endl; 
#endif
    }
    return 1;
}

