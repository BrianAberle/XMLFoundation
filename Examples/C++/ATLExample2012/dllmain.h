// dllmain.h : Declaration of module class.

class CATLExample2012Module : public ATL::CAtlDllModuleT< CATLExample2012Module >
{
public :
	DECLARE_LIBID(LIBID_ATLExample2012Lib)
	DECLARE_REGISTRY_APPID_RESOURCEID(IDR_ATLEXAMPLE2012, "{8B5F5C8C-7173-4006-9581-E725FDBD621B}")
};

extern class CATLExample2012Module _AtlModule;
