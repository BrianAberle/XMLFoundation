#include "ExCORBA_impl.h"
#include "GString.h"
#include "StringAbstractionGeneric.h"
#include "GException.h"
#include "FrameworkAuditLog.h"

#ifdef ___VISIBROKER___
 USE_STD_NS
 #include "ExCORBA_s.hh"
#elif  ___ORBIX___
 IT_USING_NAMESPACE_STD
 #include "ExCORBA.hh"
#elif ___OBJECTBROKER___ // weblogic
 #include "ExCORBA_c.h"
 #include<iostream.h>
#endif

IMPLEMENT_ORB_FACTORY(ExCORBAImpl,MyCORBAImpl)

ExCORBAImpl::~ExCORBAImpl()
{
	DecRef(); // remove 'this' from the Instance Cache.
}

ExCORBAImpl::ExCORBAImpl()
{
}

void ExCORBAImpl::MapXMLTagsToMembers()
{
	MapObjectID("CustomerID",1);	// object id's are optional.
	MapMember(&_nCustID,  "CustomerID");
	MapMember(&_strCustName, "CustomerName", &gGenericStrHandler);
	MapMember(&m_lstCMyImplObjs,	ExCORBAImpl::GetStaticTag(),  &gGListHandler,0);
}

ExCORBA::MyCORBAObject_ptr ExCORBAImpl::getSubObjectIOR(CORBA::Long CustomerID)IT_THROW_DECL((CORBA::SystemException))
{
	TRACE_MESSAGE("ExCORBAImpl::getSubObjectIOR()");
	cout << "entering getSubObjectIOR()" << endl;
	// walk the list of children Objects
	GListIterator it(&m_lstCMyImplObjs);
	while(it())
	{
		TRACE_MESSAGE("Iterate()");

		XMLObject *pO = (XMLObject *)it++;
		ExCORBAImpl*pIO = (ExCORBAImpl*)pO->GetInterfaceObject();
		if (pIO->GetCustomerID() == CustomerID)
		{
			TRACE_MESSAGE("return interface");
			cout << "return interface" << endl;

#ifdef ___VISIBROKER___
			// Activate it on the default POA which is root POA for this servant
			PortableServer::POA_var default_poa = _default_POA();
			pIO->_add_ref();
			CORBA::Object_var ref = default_poa->servant_to_reference(pIO);
			return ExCORBA::MyCORBAObject::_narrow(ref);
#elif  defined ___ORBIX___ || defined ___OBJECTBROKER___
			return pIO->_this();
#endif
		}
	}
	TRACE_MESSAGE("interface not found");
	cout << "interface not found" << endl;
	return ExCORBA::MyCORBAObject::_nil();
}

void ExCORBAImpl::getXMLState( CORBA::String_out  s) IT_THROW_DECL((CORBA::SystemException))
{
	TRACE_MESSAGE("ExCORBAImpl::getXMLState()");
    cout << "Calling getXMLState()" << endl;
	try
	{
		const char *p = ToXML();
		s = CORBA::string_dup(p);
	}catch(GException &ex)
	{
		s = CORBA::string_dup(ex.GetDescription());
	}
}
void ExCORBAImpl::setXMLState( const char* pzXML   ) IT_THROW_DECL((CORBA::SystemException))
{
	TRACE_MESSAGE("ExCORBAImpl----setXMLState()----");
 	TRACE_MESSAGE(pzXML);
    cout << "Calling setXMLState(" << pzXML << ")" << endl;
	try
	{
		FromXML( pzXML );
		ToXMLFile("c:\\out.txt");

	}catch(GException &ex)	
	{
		cout << ex.GetDescription();
	}
}
void ExCORBAImpl::setState( const char*  s,   CORBA::Long l) IT_THROW_DECL((CORBA::SystemException))
{
	TRACE_MESSAGE("ExCORBAImpl::setState()");
	cout << "Calling setState(" << s << ", " << l << ")" << endl;
    _strCustName = s;
	_nCustID = l;
}

// Objects are added to the Runtime Object Model 1 of 2 ways.
// Either they are factory created through FromXML(), or they
// manually added as this example shows.  Manually created objects 
// must call RegisterObject().  You can put that call in the 
// constructor of the *Impl, or call it directly as shown here.
void ExCORBAImpl::addSubObject( const char*  s, CORBA::Long l ) IT_THROW_DECL((CORBA::SystemException))
{
	TRACE_MESSAGE("ExCORBAImpl::addSubObject()");
    cout << "Calling addSubObject(" << s << ", " << l << ")" << endl;
	ExCORBAImpl *p = new ExCORBAImpl;
	p->RegisterObject(); 

	p->_nCustID = l;
	p->_strCustName = s;
	m_lstCMyImplObjs.AddLast(p);
}

void ExCORBAImpl::delSubObjects() IT_THROW_DECL((CORBA::SystemException))
{
	TRACE_MESSAGE("ExCORBAImpl::delSubObjects()");
	GListIterator it(&m_lstCMyImplObjs);
	while(it())
	{
		XMLObject *pO = (XMLObject *)it++;
		ExCORBAImpl*pIO = (ExCORBAImpl*)pO->GetInterfaceObject();
		
		// DecRef() will destroy 'this' when the ref count hits 0.
		// PO's virtual destructor will cleanup pIO.
		pO->DecRef();
	}
	m_lstCMyImplObjs.RemoveAll();
}

void ExCORBAImpl::dumpState( CORBA::String_out  s) IT_THROW_DECL((CORBA::SystemException))
{
	TRACE_MESSAGE("ExCORBAImpl::dumpState()");
	s = CORBA::string_dup(Dump());
}


/* Begin Stateless Implementation */
ExCORBA::MyCORBAObject_ptr ExCORBAImpl::getSubObjectIOR2(CORBA::Long h, CORBA::Long CustomerID) IT_THROW_DECL((CORBA::SystemException))
{
	ReStoreState(h);
	return getSubObjectIOR( CustomerID );
}
void ExCORBAImpl::getXMLState2( CORBA::Long h, CORBA::String_out  s) IT_THROW_DECL((CORBA::SystemException))
{
	// in a true stateless environment, the instance cache will be empty
	TRACE_MESSAGE("####################################################################");
	cacheManager.dumpCache();
	cacheManager.dumpStateCache();
	TRACE_MESSAGE("####################################################################");

	ReStoreState(h);
	getXMLState(s);
	StoreState();
}
void ExCORBAImpl::setXMLState2( CORBA::Long h, const char*  s ) IT_THROW_DECL((CORBA::SystemException))
{
	ReStoreState(h);
	setXMLState(s);
	StoreState();

	// object instances are still in the cache, until destruction
	TRACE_MESSAGE("********************************************************************");
	cacheManager.dumpCache();
	cacheManager.dumpStateCache();
	TRACE_MESSAGE("********************************************************************");
}
void ExCORBAImpl::setState2( CORBA::Long h, const char*  s,   CORBA::Long l) IT_THROW_DECL((CORBA::SystemException))
{
	ReStoreState(h);
	setState(s,l);
	StoreState();
}
void ExCORBAImpl::addSubObject2( CORBA::Long h, const char*  s, CORBA::Long l ) IT_THROW_DECL((CORBA::SystemException))
{
	ReStoreState(h);
	addSubObject(s,l);
	StoreState();
}
void ExCORBAImpl::delSubObjects2(CORBA::Long h) IT_THROW_DECL((CORBA::SystemException))
{
	ReStoreState(h);
	delSubObjects();
	StoreState();
}
void ExCORBAImpl::dumpState2( CORBA::Long h, CORBA::String_out  s) IT_THROW_DECL((CORBA::SystemException))
{
	ReStoreState(h);
	dumpState(s);
	StoreState();
}
/* End Stateless Implementation*/



