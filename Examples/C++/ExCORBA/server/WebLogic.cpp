#include "ExCORBA_c.h"
#include "ExCORBA_s.h"
#include "ExCORBA_impl.h"
#include "FrameworkAuditLog.h"
#include <iostream.h>
#include <Server.h>
#include <TP.h>


#include <time.h>   // for time() and getpid()
#include <fstream.h>

#ifdef WIN32
#include <process.h>
#include <strstrea.h>
#else
#include <unistd.h>
#include <strstream.h>
#endif



static CORBA::Object_var s_v_fact_ref;

static int   s_pid       = 0; // this process id
static long  s_birth_sec = 0; // the second when this process started

//--------------------------------------------------------------------
// Method to start up the server
//--------------------------------------------------------------------

CORBA::Boolean Server::initialize(int argc, char* argv[])
{
    // create the factory object reference

    // For now use a single SimpleFactory, hence a non-unique
    // "simple_factory" for the object_id.

    // To make simpapp scalable, you must allow many SimpleFactories
    // to co-exist. So generate and use unique object_ids.

    s_pid       = getpid();     // get my process id
    s_birth_sec = time(0);      // get the current time

    s_v_fact_ref =
        TP::create_object_reference(
		ExCORBA::_tc_MyCORBAObject->id(), // factory interface id
//            "simple_factory",        // object id
            "MyCORBAObject_factory",        // object id
            CORBA::NVList::_nil()    // routing criteria
        );

    // register the factory reference with the factory finder:
    TP::register_factory(
        s_v_fact_ref.in(),      // factory object reference
        ExCORBA::_tc_MyCORBAObject->id() // factory interface id
    );

    // we've succeeded:
    return CORBA_TRUE;
}

//--------------------------------------------------------------------
// Method to shutdown the server
//--------------------------------------------------------------------

void Server::release()
{
    // this will only be called if Server::initialize succeeded
    // thus we know that the factory has been registered with
    // the factory finder.

    // unregister the factory.
    // use a try block since cleanup code shouldn't throw exceptions
    try {
        TP::unregister_factory(
            s_v_fact_ref.in(),      // factory object reference
            ExCORBA::_tc_MyCORBAObject->id() // factory interface id
        );
    }
    catch (...) {
        TP::userlog("Couldn't unregistering the SimpleFactory");
    }
}

//--------------------------------------------------------------------
// Method to create servants
//--------------------------------------------------------------------

Tobj_Servant Server::create_servant(const char* intf_repos_id)
{
    if (!strcmp(intf_repos_id, ExCORBA::_tc_MyCORBAObject->id())) {
        ExCORBAImpl *pO = new ExCORBAImpl();
		pO->RegisterObject(); 
		return pO;
    }
    return 0; // unknown interface
}

