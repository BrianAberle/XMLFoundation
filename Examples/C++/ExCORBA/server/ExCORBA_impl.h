#ifndef _MY_IMPL_H_
#define _MY_IMPL_H_

#ifdef ___VISIBROKER___
 #include "ExCORBA_s.hh"
 #define IT_THROW_DECL( a )
#elif  ___ORBIX___
 #include "ExCORBAS.hh"
#elif ___OBJECTBROKER___	// (weblogic)
 #include "ExCORBA_s.h"
 #define IT_THROW_DECL( a )
#endif




#include "xmlObject.h"
#include "GString.h"
#include "GList.h"


// This implementation class derives multiply from the POA 
// base class created by the IDL compiler, and XMLObject.
class ExCORBAImpl : 	 public POA_ExCORBA::MyCORBAObject
						,public XMLObject
{
	// for visibroker
#ifdef ___VISIBROKER___
	PortableServer::Current_var    _poa_current;
#endif

	GString _strCustName;
	long	_nCustID;
	GList m_lstCMyImplObjs; // contains ExCORBAImpl *'s as XMLObject *'s,
							// objects may be put into this list by the XML 
							// factory as shown by ExCORBAImpl::setXMLState()
							// or directly as shown by ExCORBAImpl::addSubObject()

  public:  
    ExCORBAImpl();
    ~ExCORBAImpl();
#ifdef ___VISIBROKER___	
	ExCORBAImpl(PortableServer::Current_ptr cur)  
  :  _poa_current(PortableServer::Current::_duplicate(cur))
    {}
#endif
	long GetCustomerID() {return _nCustID;}

	DECLARE_FACTORY(ExCORBAImpl,MyCORBAImpl)
	void MapXMLTagsToMembers();

	
	// statefull implementation
	ExCORBA::MyCORBAObject_ptr getSubObjectIOR(CORBA::Long CustomerID) IT_THROW_DECL((CORBA::SystemException));
	void getXMLState( CORBA::String_out  s) IT_THROW_DECL((CORBA::SystemException));
	void setXMLState( const char*  s ) IT_THROW_DECL((CORBA::SystemException));
	void setState( const char*  s,   CORBA::Long l) IT_THROW_DECL((CORBA::SystemException));
	void addSubObject( const char*  s, CORBA::Long l ) IT_THROW_DECL((CORBA::SystemException));
	void delSubObjects() IT_THROW_DECL((CORBA::SystemException));
	void dumpState( CORBA::String_out  s) IT_THROW_DECL((CORBA::SystemException));

	// stateless implementation
	ExCORBA::MyCORBAObject_ptr getSubObjectIOR2(CORBA::Long h, CORBA::Long CustomerID) IT_THROW_DECL((CORBA::SystemException));
	void getXMLState2( CORBA::Long h, CORBA::String_out  s) IT_THROW_DECL((CORBA::SystemException));
	void setXMLState2( CORBA::Long h, const char*  s ) IT_THROW_DECL((CORBA::SystemException));
	void setState2( CORBA::Long h, const char*  s,   CORBA::Long l) IT_THROW_DECL((CORBA::SystemException));
	void addSubObject2( CORBA::Long h, const char*  s, CORBA::Long l ) IT_THROW_DECL((CORBA::SystemException));
	void delSubObjects2(CORBA::Long h) IT_THROW_DECL((CORBA::SystemException));
	void dumpState2( CORBA::Long h, CORBA::String_out  s) IT_THROW_DECL((CORBA::SystemException));
};

#endif
