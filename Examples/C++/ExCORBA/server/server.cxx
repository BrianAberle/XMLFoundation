#include <it_cal/iostream.h>
#include <it_cal/fstream.h>
#include <string.h>
#include <stdlib.h>

#define IT_TS_DEFAULTED		
#include <omg/orb.hh>
#include <omg/PortableServer.hh>
#include <it_ts/termination_handler.h>
#include "ExCORBA_impl.h"

IT_USING_NAMESPACE_STD

// global_orb -- make ORB global so signal handler can find it.
// 
CORBA::ORB_var global_orb = CORBA::ORB::_nil();

// termination_handler_callback() -- handle fatal signals/events gracefully.
//
void 
termination_handler_callback(
    long sig_type
)
{
    cout << "Received shutdown signal" << endl;
    if (!CORBA::is_nil(global_orb))
    {
	cout << "Shutting down ORB." << endl;
	global_orb->shutdown(0);
    }
    else 
    {
	cout << "ORB not initialised, aborting." << endl;
	abort();
    }
}

// export_object()
// This function takes a poa and object id, builds an object
// reference representing that object, and exports the object
// reference to a file.
//
void
export_object(
    PortableServer::POA_ptr         poa,
    const PortableServer::ObjectId& oid,
    const char*                     objref_file,
    const char*                     type_id
)
{
    CORBA::Object_var ref = poa->create_reference_with_id(oid, type_id);
    CORBA::String_var stringified_ref = global_orb->object_to_string(ref);
    cout << "Writing stringified object reference to " << objref_file << endl;
    
    ofstream os(objref_file);
    os << stringified_ref;
    if (!os.good())
    {
	cerr << "Failed to write to " << objref_file << endl;
    }
}

int main( int argc, char* argv[])
{
    // Handle signals gracefully.
    //
    IT_TerminationHandler termination_handler(termination_handler_callback);

    // Handle CORBA exceptions (none expected)
    // 
    try 
    {

	// Initialise the ORB.
	//
	cout << "Initializing the ORB" << endl;
	global_orb = CORBA::ORB_init(argc, argv);
	CORBA::Object_var poa_obj =
	    global_orb->resolve_initial_references("RootPOA");
	PortableServer::POA_var root_poa = 
	    PortableServer::POA::_narrow(poa_obj);
	assert(!CORBA::is_nil(root_poa));

	// Create servants, activate them, write their IORs to files.
	//
	cout << "Creating objects" << endl;
	
	// Using POA inheritance
	ExCORBAImpl inherit_servant;
	PortableServer::ObjectId_var inherit_oid = 
	    root_poa->activate_object(&inherit_servant);
	export_object(
	    root_poa, 
	    inherit_oid, 
	    "../ExCORBA.ior", 
	    ExCORBA::_tc_MyCORBAObject->id()
	);


	// Activate the POA Manager to allow new requests to arrive
	//
	cout << "Activating the POA Manager" << endl;
	PortableServer::POAManager_var poa_manager = root_poa->the_POAManager();
	poa_manager->activate();

	// Give control to the ORB to let it process incoming requests
	//
	cout << "Giving control to the ORB to process requests" << endl;
	global_orb->run();
	cout << "Done" << endl;
	return 0;
    }
    catch (CORBA::Exception& e)
    {
	cout << "Error occurred: " << e << endl;
    }
    return 1;
}