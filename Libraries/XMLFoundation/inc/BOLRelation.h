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

// (Business Object Link)Relation



#ifdef _SDK_COM_ATL_												


#define IMP_BOL_RELATION(iFaceImpl, BOLBusObj)						\
																	\
iFaceImpl::BOLSmartPointer::~BOLSmartPointer()						\
{																	\
	m_pBOLBusinessObject->Release();								\
}																	\
																	\
RegisterInterface iFaceImpl##Unique(								\
				iFaceImpl::FactoryCreate,							\
				BOLBusObj::FactoryCreate,							\
				#iFaceImpl,											\
				#BOLBusObj);										\
																	\
CComObject<iFaceImpl>* iFaceImpl::getCOMObject(BOLBusObj *pBusObj)	\
{																	\
	if (pBusObj)													\
	{																\
		return (CComObject<iFaceImpl>*)								\
							pBusObj->getInterfaceImplementation();	\
	}																\
	return 0;														\
}																	\
																	\
void* iFaceImpl::FactoryCreate( void *pTheBOLObject )				\
{																	\
	CComObject<iFaceImpl>* pImpl = NULL;							\
	CComObject<iFaceImpl>::CreateInstance(&pImpl);					\
	BOLBusObj *pBOL = ((BOLBusObj *)pTheBOLObject);					\
	pImpl->setBOLBusinessObject( pBOL );							\
	pBOL->AddRef();													\
	return (void *)pImpl;											\
}																	


#elif defined _SDK_CORBA_


#define IMP_BOL_RELATION(iFaceImpl, BOLBusObj)						\
																	\
iFaceImpl::BOLSmartPointer::~BOLSmartPointer()						\
{																	\
	m_pBOLBusinessObject->Release();								\
}																	\
																	\
RegisterInterface iFaceImpl##Unique(								\
				iFaceImpl::FactoryCreate,							\
				BOLBusObj::FactoryCreate,							\
				#iFaceImpl,											\
				#BOLBusObj);										\
																	\
iFaceImpl* iFaceImpl::getCORBAObject(BOLBusObj *pBusObj)			\
{																	\
	if (pBusObj)													\
	{																\
		return (iFaceImpl*)pBusObj->getInterfaceImplementation();	\
	}																\
	return 0;														\
}																	\
																	\
void* iFaceImpl::FactoryCreate( void *pTheBOLObject )				\
{																	\
	iFaceImpl *pImpl = new iFaceImpl();								\
	BOLBusObj *pBOL = ((BOLBusObj *)pTheBOLObject);					\
	pImpl->setBOLBusinessObject( pBOL );							\
	pBOL->AddRef();													\
	return (void *)pImpl;											\
}																	\


#endif




#ifdef _SDK_CORBA_
//
// If you get a compile error "missing ; before *" in VC++ you probably 
// need to forward declare the BOL type in your COM/CORBA header file.
//
#define DEF_BOL_RELATION(iFaceImpl, BOLBusObj)							\
	struct BOLSmartPointer												\
	{																	\
		BOLBusObj *m_pBOLBusinessObject;								\
		BOLSmartPointer::~BOLSmartPointer();							\
	} m_SmartPtrInstance;												\
public:																	\
	void setBOLBusinessObject(BOLBusObj *pTheBOLObject)					\
	{																	\
		m_SmartPtrInstance.m_pBOLBusinessObject = pTheBOLObject;		\
	}																	\
	BOLBusObj *getBusObj()												\
	{																	\
		return m_SmartPtrInstance.m_pBOLBusinessObject;					\
	}																	\
																		\
	static iFaceImpl* getCORBAObject(BOLBusObj *pBusObj);				\
	static void* FactoryCreate( void *pTheBOLObject );					\

#elif defined _SDK_COM_ATL_												

//
// If you get a compile error "missing ; before *" in VC++ you probably 
// need to forward declare the BOL type in your COM/CORBA header file.
//
#define DEF_BOL_RELATION(iFaceImpl, BOLBusObj)							\
	struct BOLSmartPointer												\
	{																	\
		BOLBusObj *m_pBOLBusinessObject;								\
		BOLSmartPointer::~BOLSmartPointer();							\
	} m_SmartPtrInstance;												\
public:																	\
	void setBOLBusinessObject(BOLBusObj *pTheBOLObject)					\
	{																	\
		m_SmartPtrInstance.m_pBOLBusinessObject = pTheBOLObject;		\
	}																	\
	BOLBusObj *getBusObj()												\
	{																	\
		return m_SmartPtrInstance.m_pBOLBusinessObject;					\
	}																	\
	static CComObject<iFaceImpl>* getCOMObject(BOLBusObj *pBusObj);		\
	static void* FactoryCreate( void *pTheBOLObject );					\

#else																	
	#define DEF_BOL_RELATION(a,b)	Define_SDK_CORBA_or_SDK_COM_ATL_
#endif																	
