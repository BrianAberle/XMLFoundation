/*
 * Copyright 2001 Borland Software Corporation, Inc.
 *
 * GENERATED CODE --- DO NOT EDIT
 * 
 */

#include "ExCORBA_c.hh"

ExCORBA::MyCORBAObject_ptr ExCORBA::MyCORBAObject_var::_duplicate(ExCORBA::MyCORBAObject_ptr _p) {
  return ExCORBA::MyCORBAObject::_duplicate(_p);
}

void
ExCORBA::MyCORBAObject_var::_release(ExCORBA::MyCORBAObject_ptr _p) {
  CORBA::release((CORBA::Object_ptr)_p);
}

ExCORBA::MyCORBAObject_var::MyCORBAObject_var()
  : _ptr(ExCORBA::MyCORBAObject::_nil()) {}

ExCORBA::MyCORBAObject_var::MyCORBAObject_var(ExCORBA::MyCORBAObject_ptr _p)
  : _ptr(_p) {}

ExCORBA::MyCORBAObject_var::MyCORBAObject_var(const ExCORBA::MyCORBAObject_var& _var)
  : _ptr(ExCORBA::MyCORBAObject::_duplicate((ExCORBA::MyCORBAObject_ptr) _var)) {}

ExCORBA::MyCORBAObject_var::~MyCORBAObject_var() {
  CORBA::release((CORBA::Object_ptr)_ptr);
}

ExCORBA::MyCORBAObject_var&
ExCORBA::MyCORBAObject_var::operator=(const ExCORBA::MyCORBAObject_var& _var) {
    _release(_ptr);
    _ptr = _duplicate(_var._ptr);
    return *this;
}

ExCORBA::MyCORBAObject_var&
ExCORBA::MyCORBAObject_var::operator=(ExCORBA::MyCORBAObject_ptr _p) {
  CORBA::release((CORBA::Object_ptr)_ptr);
  _ptr = _p;
  return *this;
}

ExCORBA::MyCORBAObject_ptr& ExCORBA::MyCORBAObject_var::out() {
  _release(_ptr);
  _ptr = (ExCORBA::MyCORBAObject_ptr)NULL;
  return _ptr;
}

ExCORBA::MyCORBAObject_ptr ExCORBA::MyCORBAObject_var::_retn() {
  ExCORBA::MyCORBAObject_ptr _tmp_ptr = _ptr;
  _ptr = (ExCORBA::MyCORBAObject_ptr)NULL;
  return _tmp_ptr;
}

VISistream& operator>>(VISistream& _strm, ExCORBA::MyCORBAObject_var& _var) {
  _strm >> _var._ptr;
  return _strm;
}

VISostream& operator<<(VISostream& _strm, const ExCORBA::MyCORBAObject_var& _var) {
  _strm << _var._ptr;
  return _strm;
}

Istream& operator>>(Istream& _strm, ExCORBA::MyCORBAObject_var& _var) {
  VISistream _istrm(_strm);
  _istrm >> _var._ptr;
  return _strm;
}

Ostream& operator<<(Ostream& _strm, const ExCORBA::MyCORBAObject_var& _var) {
  _strm << (CORBA::Object_ptr)_var._ptr;
  return _strm;
}

// If dllimport is specified, you might probably want to
// disable these definitions
// disable the const definitions
const VISOps_Info ExCORBA::MyCORBAObject_ops::_ops_info("ExCORBA::MyCORBAObject_ops");
const VISOps_Info *ExCORBA::MyCORBAObject_ops::_desc() { return &_ops_info; }
ExCORBA::MyCORBAObject_ops_ptr ExCORBA::MyCORBAObject_ops::_downcast(PortableServer::ServantBase* _servant) {
  if (_servant == (PortableServer::ServantBase*)NULL)
    return ExCORBA::MyCORBAObject_ops::_nil();
  return (ExCORBA::MyCORBAObject_ops_ptr)_servant->_safe_downcast_ops(_ops_info);
}
const CORBA::TypeInfo
ExCORBA::MyCORBAObject::_class_info("ExCORBA::MyCORBAObject",
                                    "IDL:ExCORBA/MyCORBAObject:1.0", NULL,
                                    ExCORBA::MyCORBAObject::_factory, NULL, 0,
                                    NULL, 0, CORBA::Object::_desc(), 0L);

const CORBA::TypeInfo *ExCORBA::MyCORBAObject::_desc() { return &_class_info; }
const CORBA::TypeInfo *ExCORBA::MyCORBAObject::_type_info() const {
  return &_class_info;
}

VISistream& operator>>(VISistream& _strm, ExCORBA::MyCORBAObject_ptr& _obj) {
  CORBA::Object_var _var_obj(_obj);
  _var_obj = CORBA::Object::_read(_strm, ExCORBA::MyCORBAObject::_desc());
  _obj = ExCORBA::MyCORBAObject::_narrow(_var_obj);
  return _strm;
}

VISostream& operator<<(VISostream& _strm, const ExCORBA::MyCORBAObject_ptr _obj) {
  _strm << (CORBA::Object_ptr)_obj;
  return _strm;
}
void* ExCORBA::MyCORBAObject::_safe_narrow(const CORBA::TypeInfo& _info) const {
  if (_info == _class_info)
    return (void *)this;
  return CORBA_Object::_safe_narrow(_info);
}

CORBA::Object *ExCORBA::MyCORBAObject::_factory() {
  return new ExCORBA::MyCORBAObject;
}

ExCORBA::MyCORBAObject_ptr ExCORBA::MyCORBAObject::_this() {
  return ExCORBA::MyCORBAObject::_duplicate(this);
}

ExCORBA::MyCORBAObject_ptr ExCORBA::MyCORBAObject::_narrow(CORBA::Object * _obj) {
  if (_obj == ExCORBA::MyCORBAObject::_nil())
    return ExCORBA::MyCORBAObject::_nil();
  else
    return ExCORBA::MyCORBAObject::_duplicate((ExCORBA::MyCORBAObject_ptr)_obj->_safe_narrow(_class_info));
}

Ostream& operator<<(Ostream& _strm, const ExCORBA::MyCORBAObject_ptr _obj) {
  _strm << (CORBA::Object_ptr) _obj;
  return _strm;
}
Istream& operator>>(Istream& _strm, ExCORBA::MyCORBAObject_ptr& _obj) {
  VISistream _istrm(_strm);
  _istrm >> _obj;
  return _strm;
}

ExCORBA::MyCORBAObject *ExCORBA::MyCORBAObject::_bind(const char *_object_name,
                                                      const char *_host_name,
                                                      const CORBA::BindOptions *_opt,
                                                      CORBA::ORB_ptr _orb)
{
  VISCLEAR_EXCEP
  CORBA::Object_var _obj = CORBA::Object::_bind_to_object(
          "IDL:ExCORBA/MyCORBAObject:1.0", _object_name, _host_name, _opt,
_orb);
  return MyCORBAObject::_narrow(_obj);
}

ExCORBA::MyCORBAObject *ExCORBA::MyCORBAObject::_bind(const char *_poa_name,
                                                      const CORBA::OctetSequence& _id,
                                                      const char *_host_name,
                                                      const CORBA::BindOptions *_opt,
                                                      CORBA::ORB_ptr _orb)
{
  VISCLEAR_EXCEP
  CORBA::Object_var _obj = CORBA::Object::_bind_to_object(
   "IDL:ExCORBA/MyCORBAObject:1.0", _poa_name, _id, _host_name, _opt, _orb);
  return MyCORBAObject::_narrow(_obj);
}
void ExCORBA::MyCORBAObject::getXMLState(CORBA::String_out _arg_s ) {
  CORBA_MarshalInBuffer_var _ibuf;

  while (1) {
    VISCLEAR_EXCEP 

    if (_is_local()) {
      PortableServer::ServantBase_ptr _servant;
      VISTRY {
        _servant = _preinvoke("getXMLState");
      }
      VISCATCH(VISRemarshal, _vis_except) { continue; } 
      VISEND_CATCH

      ExCORBA::MyCORBAObject_ops*  _mycorbaobject = ExCORBA::MyCORBAObject_ops::_downcast(_servant);
      if (!_mycorbaobject) {
        if ((PortableServer::ServantBase*)_servant)
          _postinvoke(_servant, "getXMLState");
        VISTHROW(CORBA::BAD_OPERATION());
        VISRETURN(return;)
      }

      VISTRY {
        _mycorbaobject->getXMLState(_arg_s );
      }
      VISCATCH_ALL{
        _postinvoke(_servant, "getXMLState"); 
        VISTHROW_LAST;
        VISRETURN(return;)
      }
      VISEND_CATCH
      _postinvoke(_servant, "getXMLState");
      return;
    }

    CORBA::MarshalOutBuffer_var _obuf;
    _obuf = _request("getXMLState", 1);
    VISIF_EXCEP(return;)

    VISTRY {
      _ibuf = _invoke(_obuf);
      VISIFNOT_EXCEP
      break;
      VISEND_IFNOT_EXCEP
    } VISCATCH(VISRemarshal, _vis_except) { continue; } 
    VISEND_CATCH
    VISRETURN(return;)
  }

  VISistream& _istrm = *VISistream::_downcast(_ibuf);
    _istrm >> _arg_s;

}

void ExCORBA::MyCORBAObject::setXMLState(const char* _arg_s ) {
  CORBA_MarshalInBuffer_var _ibuf;

  while (1) {
    VISCLEAR_EXCEP 

    if (_is_local()) {
      PortableServer::ServantBase_ptr _servant;
      VISTRY {
        _servant = _preinvoke("setXMLState");
      }
      VISCATCH(VISRemarshal, _vis_except) { continue; } 
      VISEND_CATCH

      ExCORBA::MyCORBAObject_ops*  _mycorbaobject = ExCORBA::MyCORBAObject_ops::_downcast(_servant);
      if (!_mycorbaobject) {
        if ((PortableServer::ServantBase*)_servant)
          _postinvoke(_servant, "setXMLState");
        VISTHROW(CORBA::BAD_OPERATION());
        VISRETURN(return;)
      }

      VISTRY {
        _mycorbaobject->setXMLState(_arg_s );
      }
      VISCATCH_ALL{
        _postinvoke(_servant, "setXMLState"); 
        VISTHROW_LAST;
        VISRETURN(return;)
      }
      VISEND_CATCH
      _postinvoke(_servant, "setXMLState");
      return;
    }

    CORBA::MarshalOutBuffer_var _obuf;
    _obuf = _request("setXMLState", 1);
    VISIF_EXCEP(return;)
    VISostream& _ostrm = *VISostream::_downcast(_obuf);
    _ostrm << _arg_s;

    VISTRY {
      _ibuf = _invoke(_obuf);
      VISIFNOT_EXCEP
      break;
      VISEND_IFNOT_EXCEP
    } VISCATCH(VISRemarshal, _vis_except) { continue; } 
    VISEND_CATCH
    VISRETURN(return;)
  }

}

void ExCORBA::MyCORBAObject::setState(const char* _arg_s , CORBA::Long _arg_l ) {
  CORBA_MarshalInBuffer_var _ibuf;

  while (1) {
    VISCLEAR_EXCEP 

    if (_is_local()) {
      PortableServer::ServantBase_ptr _servant;
      VISTRY {
        _servant = _preinvoke("setState");
      }
      VISCATCH(VISRemarshal, _vis_except) { continue; } 
      VISEND_CATCH

      ExCORBA::MyCORBAObject_ops*  _mycorbaobject = ExCORBA::MyCORBAObject_ops::_downcast(_servant);
      if (!_mycorbaobject) {
        if ((PortableServer::ServantBase*)_servant)
          _postinvoke(_servant, "setState");
        VISTHROW(CORBA::BAD_OPERATION());
        VISRETURN(return;)
      }

      VISTRY {
        _mycorbaobject->setState(_arg_s , _arg_l );
      }
      VISCATCH_ALL{
        _postinvoke(_servant, "setState"); 
        VISTHROW_LAST;
        VISRETURN(return;)
      }
      VISEND_CATCH
      _postinvoke(_servant, "setState");
      return;
    }

    CORBA::MarshalOutBuffer_var _obuf;
    _obuf = _request("setState", 1);
    VISIF_EXCEP(return;)
    VISostream& _ostrm = *VISostream::_downcast(_obuf);
    _ostrm << _arg_s;
    _ostrm << _arg_l;

    VISTRY {
      _ibuf = _invoke(_obuf);
      VISIFNOT_EXCEP
      break;
      VISEND_IFNOT_EXCEP
    } VISCATCH(VISRemarshal, _vis_except) { continue; } 
    VISEND_CATCH
    VISRETURN(return;)
  }

}

void ExCORBA::MyCORBAObject::addSubObject(const char* _arg_s ,
                                          CORBA::Long _arg_l ) {
  CORBA_MarshalInBuffer_var _ibuf;

  while (1) {
    VISCLEAR_EXCEP 

    if (_is_local()) {
      PortableServer::ServantBase_ptr _servant;
      VISTRY {
        _servant = _preinvoke("addSubObject");
      }
      VISCATCH(VISRemarshal, _vis_except) { continue; } 
      VISEND_CATCH

      ExCORBA::MyCORBAObject_ops*  _mycorbaobject = ExCORBA::MyCORBAObject_ops::_downcast(_servant);
      if (!_mycorbaobject) {
        if ((PortableServer::ServantBase*)_servant)
          _postinvoke(_servant, "addSubObject");
        VISTHROW(CORBA::BAD_OPERATION());
        VISRETURN(return;)
      }

      VISTRY {
        _mycorbaobject->addSubObject(_arg_s , _arg_l );
      }
      VISCATCH_ALL{
        _postinvoke(_servant, "addSubObject"); 
        VISTHROW_LAST;
        VISRETURN(return;)
      }
      VISEND_CATCH
      _postinvoke(_servant, "addSubObject");
      return;
    }

    CORBA::MarshalOutBuffer_var _obuf;
    _obuf = _request("addSubObject", 1);
    VISIF_EXCEP(return;)
    VISostream& _ostrm = *VISostream::_downcast(_obuf);
    _ostrm << _arg_s;
    _ostrm << _arg_l;

    VISTRY {
      _ibuf = _invoke(_obuf);
      VISIFNOT_EXCEP
      break;
      VISEND_IFNOT_EXCEP
    } VISCATCH(VISRemarshal, _vis_except) { continue; } 
    VISEND_CATCH
    VISRETURN(return;)
  }

}

void ExCORBA::MyCORBAObject::delSubObjects() {
  CORBA_MarshalInBuffer_var _ibuf;

  while (1) {
    VISCLEAR_EXCEP 

    if (_is_local()) {
      PortableServer::ServantBase_ptr _servant;
      VISTRY {
        _servant = _preinvoke("delSubObjects");
      }
      VISCATCH(VISRemarshal, _vis_except) { continue; } 
      VISEND_CATCH

      ExCORBA::MyCORBAObject_ops*  _mycorbaobject = ExCORBA::MyCORBAObject_ops::_downcast(_servant);
      if (!_mycorbaobject) {
        if ((PortableServer::ServantBase*)_servant)
          _postinvoke(_servant, "delSubObjects");
        VISTHROW(CORBA::BAD_OPERATION());
        VISRETURN(return;)
      }

      VISTRY {
        _mycorbaobject->delSubObjects();
      }
      VISCATCH_ALL{
        _postinvoke(_servant, "delSubObjects"); 
        VISTHROW_LAST;
        VISRETURN(return;)
      }
      VISEND_CATCH
      _postinvoke(_servant, "delSubObjects");
      return;
    }

    CORBA::MarshalOutBuffer_var _obuf;
    _obuf = _request("delSubObjects", 1);
    VISIF_EXCEP(return;)

    VISTRY {
      _ibuf = _invoke(_obuf);
      VISIFNOT_EXCEP
      break;
      VISEND_IFNOT_EXCEP
    } VISCATCH(VISRemarshal, _vis_except) { continue; } 
    VISEND_CATCH
    VISRETURN(return;)
  }

}

ExCORBA::MyCORBAObject_ptr ExCORBA::MyCORBAObject::getSubObjectIOR(CORBA::Long _arg_l ) {
  ExCORBA::MyCORBAObject_ptr _ret = (ExCORBA::MyCORBAObject_ptr) NULL;
  CORBA_MarshalInBuffer_var _ibuf;

  while (1) {
    VISCLEAR_EXCEP 

    if (_is_local()) {
      PortableServer::ServantBase_ptr _servant;
      VISTRY {
        _servant = _preinvoke("getSubObjectIOR");
      }
      VISCATCH(VISRemarshal, _vis_except) { continue; } 
      VISEND_CATCH

      ExCORBA::MyCORBAObject_ops*  _mycorbaobject = ExCORBA::MyCORBAObject_ops::_downcast(_servant);
      if (!_mycorbaobject) {
        if ((PortableServer::ServantBase*)_servant)
          _postinvoke(_servant, "getSubObjectIOR");
        VISTHROW(CORBA::BAD_OPERATION());
        VISRETURN(return _ret;)
      }

      VISTRY {
         _ret = _mycorbaobject->getSubObjectIOR(_arg_l );
      }
      VISCATCH_ALL{
        _postinvoke(_servant, "getSubObjectIOR"); 
        VISTHROW_LAST;
        VISRETURN(return _ret;)
      }
      VISEND_CATCH
      _postinvoke(_servant, "getSubObjectIOR");
      return _ret;
    }

    CORBA::MarshalOutBuffer_var _obuf;
    _obuf = _request("getSubObjectIOR", 1);
    VISIF_EXCEP(return _ret;)
    VISostream& _ostrm = *VISostream::_downcast(_obuf);
    _ostrm << _arg_l;

    VISTRY {
      _ibuf = _invoke(_obuf);
      VISIFNOT_EXCEP
      break;
      VISEND_IFNOT_EXCEP
    } VISCATCH(VISRemarshal, _vis_except) { continue; } 
    VISEND_CATCH
    VISRETURN(return _ret;)
  }

  VISistream& _istrm = *VISistream::_downcast(_ibuf);
  _istrm >> _ret;

  return _ret;
}

void ExCORBA::MyCORBAObject::dumpState(CORBA::String_out _arg_s ) {
  CORBA_MarshalInBuffer_var _ibuf;

  while (1) {
    VISCLEAR_EXCEP 

    if (_is_local()) {
      PortableServer::ServantBase_ptr _servant;
      VISTRY {
        _servant = _preinvoke("dumpState");
      }
      VISCATCH(VISRemarshal, _vis_except) { continue; } 
      VISEND_CATCH

      ExCORBA::MyCORBAObject_ops*  _mycorbaobject = ExCORBA::MyCORBAObject_ops::_downcast(_servant);
      if (!_mycorbaobject) {
        if ((PortableServer::ServantBase*)_servant)
          _postinvoke(_servant, "dumpState");
        VISTHROW(CORBA::BAD_OPERATION());
        VISRETURN(return;)
      }

      VISTRY {
        _mycorbaobject->dumpState(_arg_s );
      }
      VISCATCH_ALL{
        _postinvoke(_servant, "dumpState"); 
        VISTHROW_LAST;
        VISRETURN(return;)
      }
      VISEND_CATCH
      _postinvoke(_servant, "dumpState");
      return;
    }

    CORBA::MarshalOutBuffer_var _obuf;
    _obuf = _request("dumpState", 1);
    VISIF_EXCEP(return;)

    VISTRY {
      _ibuf = _invoke(_obuf);
      VISIFNOT_EXCEP
      break;
      VISEND_IFNOT_EXCEP
    } VISCATCH(VISRemarshal, _vis_except) { continue; } 
    VISEND_CATCH
    VISRETURN(return;)
  }

  VISistream& _istrm = *VISistream::_downcast(_ibuf);
    _istrm >> _arg_s;

}

