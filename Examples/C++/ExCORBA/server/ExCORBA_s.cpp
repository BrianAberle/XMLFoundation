/*
 * Copyright 2001 Borland Software Corporation, Inc.
 *
 * GENERATED CODE --- DO NOT EDIT
 * 
 */

#include "ExCORBA_s.hh"
static CORBA::MethodDescription _sk_ExCORBA_MyCORBAObject_methods[] = {
  {"getXMLState", POA_ExCORBA::MyCORBAObject::_getXMLState},
  {"setXMLState", POA_ExCORBA::MyCORBAObject::_setXMLState},
  {"setState", POA_ExCORBA::MyCORBAObject::_setState},
  {"addSubObject", POA_ExCORBA::MyCORBAObject::_addSubObject},
  {"delSubObjects", POA_ExCORBA::MyCORBAObject::_delSubObjects},
  {"getSubObjectIOR", POA_ExCORBA::MyCORBAObject::_getSubObjectIOR},
  {"dumpState", POA_ExCORBA::MyCORBAObject::_dumpState}
};

const CORBA::TypeInfo POA_ExCORBA::MyCORBAObject::_skel_info(
 "ExCORBA::MyCORBAObject", NULL, (CORBA::ULong) 7,
 _sk_ExCORBA_MyCORBAObject_methods         , NULL, 0, NULL);
         
const CORBA::TypeInfo *POA_ExCORBA::MyCORBAObject::_type_info() const {
  return &_skel_info;
}

ExCORBA::MyCORBAObject_ptr POA_ExCORBA::MyCORBAObject::_this() {
  return (ExCORBA::MyCORBAObject *)(PortableServer_ServantBase::_this()->_safe_narrow(*ExCORBA::MyCORBAObject::_desc()));
}

void *POA_ExCORBA::MyCORBAObject::_safe_narrow(const CORBA::TypeInfo& _info) const {
  if (_info == _skel_info) {
    return (void *)this;
  }

  if (_info == PortableServer_ServantBase::_skel_info) {
    return (void *)(PortableServer_ServantBase *)this;
  }
  return 0;
}

POA_ExCORBA::MyCORBAObject * POA_ExCORBA::MyCORBAObject::_narrow(PortableServer::ServantBase *_obj) {
  if (!_obj) {
    return (POA_ExCORBA::MyCORBAObject*)NULL;
  } else {
    return (MyCORBAObject*)_obj->_safe_narrow(_skel_info);
  }
}

void POA_ExCORBA::MyCORBAObject::_getXMLState (void *_obj,
                                               CORBA::MarshalInBuffer &_istrm,
                                               const char *_oper,
                                               VISReplyHandler& _handler) {
  VISCLEAR_EXCEP
  VISistream& _vistrm = _istrm;
  POA_ExCORBA::MyCORBAObject *_impl = (POA_ExCORBA::MyCORBAObject*)_obj;
  CORBA::String_var _local_s;

_impl->getXMLState( _local_s.out());
  CORBA::MarshalOutBuffer_var _obuf = _handler.create_reply();
  VISostream& _ostrm = *VISostream::_downcast(_obuf);
  VISIF_EXCEP(return;)
  _ostrm << _local_s;
}

void POA_ExCORBA::MyCORBAObject::_setXMLState (void *_obj,
                                               CORBA::MarshalInBuffer &_istrm,
                                               const char *_oper,
                                               VISReplyHandler& _handler) {
  VISCLEAR_EXCEP
  VISistream& _vistrm = _istrm;
  POA_ExCORBA::MyCORBAObject *_impl = (POA_ExCORBA::MyCORBAObject*)_obj;
  CORBA::String_var _local_s;
    _vistrm >> _local_s;

_impl->setXMLState( _local_s.in());
  CORBA::MarshalOutBuffer_var _obuf = _handler.create_reply();
  VISostream& _ostrm = *VISostream::_downcast(_obuf);
  VISIF_EXCEP(return;)
}

void POA_ExCORBA::MyCORBAObject::_setState (void *_obj,
                                            CORBA::MarshalInBuffer &_istrm,
                                            const char *_oper,
                                            VISReplyHandler& _handler) {
  VISCLEAR_EXCEP
  VISistream& _vistrm = _istrm;
  POA_ExCORBA::MyCORBAObject *_impl = (POA_ExCORBA::MyCORBAObject*)_obj;
  CORBA::String_var _local_s;
  CORBA::Long _local_l;
    _vistrm >> _local_s;

    _vistrm >> _local_l;

_impl->setState( _local_s.in(),  _local_l);
  CORBA::MarshalOutBuffer_var _obuf = _handler.create_reply();
  VISostream& _ostrm = *VISostream::_downcast(_obuf);
  VISIF_EXCEP(return;)
}

void POA_ExCORBA::MyCORBAObject::_addSubObject (void *_obj,
                                                CORBA::MarshalInBuffer &_istrm,
                                                const char *_oper,
                                                VISReplyHandler& _handler) {
  VISCLEAR_EXCEP
  VISistream& _vistrm = _istrm;
  POA_ExCORBA::MyCORBAObject *_impl = (POA_ExCORBA::MyCORBAObject*)_obj;
  CORBA::String_var _local_s;
  CORBA::Long _local_l;
    _vistrm >> _local_s;

    _vistrm >> _local_l;

_impl->addSubObject( _local_s.in(),  _local_l);
  CORBA::MarshalOutBuffer_var _obuf = _handler.create_reply();
  VISostream& _ostrm = *VISostream::_downcast(_obuf);
  VISIF_EXCEP(return;)
}

void POA_ExCORBA::MyCORBAObject::_delSubObjects (void *_obj,
                                                 CORBA::MarshalInBuffer &_istrm,
                                                 const char *_oper,
                                                 VISReplyHandler& _handler) {
  VISCLEAR_EXCEP
  VISistream& _vistrm = _istrm;
  POA_ExCORBA::MyCORBAObject *_impl = (POA_ExCORBA::MyCORBAObject*)_obj;
_impl->delSubObjects();
  CORBA::MarshalOutBuffer_var _obuf = _handler.create_reply();
  VISostream& _ostrm = *VISostream::_downcast(_obuf);
  VISIF_EXCEP(return;)
}

void POA_ExCORBA::MyCORBAObject::_getSubObjectIOR (void *_obj,
                                                   CORBA::MarshalInBuffer &_istrm,
                                                   const char *_oper,
                                                   VISReplyHandler& _handler) {
  VISCLEAR_EXCEP
  VISistream& _vistrm = _istrm;
  POA_ExCORBA::MyCORBAObject *_impl = (POA_ExCORBA::MyCORBAObject*)_obj;
  CORBA::Long _local_l;
    _vistrm >> _local_l;

ExCORBA::MyCORBAObject_var _ret = _impl->getSubObjectIOR( _local_l);
  CORBA::MarshalOutBuffer_var _obuf = _handler.create_reply();
  VISostream& _ostrm = *VISostream::_downcast(_obuf);
  VISIF_EXCEP(return;)
  _ostrm << _ret;
}

void POA_ExCORBA::MyCORBAObject::_dumpState (void *_obj,
                                             CORBA::MarshalInBuffer &_istrm,
                                             const char *_oper,
                                             VISReplyHandler& _handler) {
  VISCLEAR_EXCEP
  VISistream& _vistrm = _istrm;
  POA_ExCORBA::MyCORBAObject *_impl = (POA_ExCORBA::MyCORBAObject*)_obj;
  CORBA::String_var _local_s;

_impl->dumpState( _local_s.out());
  CORBA::MarshalOutBuffer_var _obuf = _handler.create_reply();
  VISostream& _ostrm = *VISostream::_downcast(_obuf);
  VISIF_EXCEP(return;)
  _ostrm << _local_s;
}

