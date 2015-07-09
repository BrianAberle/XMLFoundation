

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 8.00.0595 */
/* at Thu Apr 02 19:51:00 2015
 */
/* Compiler settings for ATLExample2012.idl:
    Oicf, W1, Zp8, env=Win64 (32b run), target_arch=AMD64 8.00.0595 
    protocol : dce , ms_ext, c_ext, robust
    error checks: allocation ref bounds_check enum stub_data 
    VC __declspec() decoration level: 
         __declspec(uuid()), __declspec(selectany), __declspec(novtable)
         DECLSPEC_UUID(), MIDL_INTERFACE()
*/
/* @@MIDL_FILE_HEADING(  ) */

#pragma warning( disable: 4049 )  /* more than 64k source lines */


/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 475
#endif

#include "rpc.h"
#include "rpcndr.h"

#ifndef __RPCNDR_H_VERSION__
#error this stub requires an updated version of <rpcndr.h>
#endif // __RPCNDR_H_VERSION__

#ifndef COM_NO_WINDOWS_H
#include "windows.h"
#include "ole2.h"
#endif /*COM_NO_WINDOWS_H*/

#ifndef __ATLExample2012_i_h__
#define __ATLExample2012_i_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __ICustomer_FWD_DEFINED__
#define __ICustomer_FWD_DEFINED__
typedef interface ICustomer ICustomer;

#endif 	/* __ICustomer_FWD_DEFINED__ */


#ifndef __IAddress_FWD_DEFINED__
#define __IAddress_FWD_DEFINED__
typedef interface IAddress IAddress;

#endif 	/* __IAddress_FWD_DEFINED__ */


#ifndef __IOrder_FWD_DEFINED__
#define __IOrder_FWD_DEFINED__
typedef interface IOrder IOrder;

#endif 	/* __IOrder_FWD_DEFINED__ */


#ifndef __ILineItem_FWD_DEFINED__
#define __ILineItem_FWD_DEFINED__
typedef interface ILineItem ILineItem;

#endif 	/* __ILineItem_FWD_DEFINED__ */


#ifndef __Customer_FWD_DEFINED__
#define __Customer_FWD_DEFINED__

#ifdef __cplusplus
typedef class Customer Customer;
#else
typedef struct Customer Customer;
#endif /* __cplusplus */

#endif 	/* __Customer_FWD_DEFINED__ */


#ifndef __Address_FWD_DEFINED__
#define __Address_FWD_DEFINED__

#ifdef __cplusplus
typedef class Address Address;
#else
typedef struct Address Address;
#endif /* __cplusplus */

#endif 	/* __Address_FWD_DEFINED__ */


#ifndef __Order_FWD_DEFINED__
#define __Order_FWD_DEFINED__

#ifdef __cplusplus
typedef class Order Order;
#else
typedef struct Order Order;
#endif /* __cplusplus */

#endif 	/* __Order_FWD_DEFINED__ */


#ifndef __LineItem_FWD_DEFINED__
#define __LineItem_FWD_DEFINED__

#ifdef __cplusplus
typedef class LineItem LineItem;
#else
typedef struct LineItem LineItem;
#endif /* __cplusplus */

#endif 	/* __LineItem_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"

#ifdef __cplusplus
extern "C"{
#endif 


#ifndef __ICustomer_INTERFACE_DEFINED__
#define __ICustomer_INTERFACE_DEFINED__

/* interface ICustomer */
/* [unique][nonextensible][dual][uuid][object] */ 


EXTERN_C const IID IID_ICustomer;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("D337309F-5308-4592-B7E9-50D48F04D3C7")
    ICustomer : public IDispatch
    {
    public:
    };
    
    
#else 	/* C style interface */

    typedef struct ICustomerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICustomer * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICustomer * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICustomer * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ICustomer * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ICustomer * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ICustomer * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [range][in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ICustomer * This,
            /* [annotation][in] */ 
            _In_  DISPID dispIdMember,
            /* [annotation][in] */ 
            _In_  REFIID riid,
            /* [annotation][in] */ 
            _In_  LCID lcid,
            /* [annotation][in] */ 
            _In_  WORD wFlags,
            /* [annotation][out][in] */ 
            _In_  DISPPARAMS *pDispParams,
            /* [annotation][out] */ 
            _Out_opt_  VARIANT *pVarResult,
            /* [annotation][out] */ 
            _Out_opt_  EXCEPINFO *pExcepInfo,
            /* [annotation][out] */ 
            _Out_opt_  UINT *puArgErr);
        
        END_INTERFACE
    } ICustomerVtbl;

    interface ICustomer
    {
        CONST_VTBL struct ICustomerVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICustomer_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ICustomer_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ICustomer_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ICustomer_GetTypeInfoCount(This,pctinfo)	\
    ( (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo) ) 

#define ICustomer_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    ( (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo) ) 

#define ICustomer_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    ( (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId) ) 

#define ICustomer_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    ( (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr) ) 


#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ICustomer_INTERFACE_DEFINED__ */


#ifndef __IAddress_INTERFACE_DEFINED__
#define __IAddress_INTERFACE_DEFINED__

/* interface IAddress */
/* [unique][nonextensible][dual][uuid][object] */ 


EXTERN_C const IID IID_IAddress;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("0AAA5725-A22D-4519-922E-7F78B322B71B")
    IAddress : public IDispatch
    {
    public:
    };
    
    
#else 	/* C style interface */

    typedef struct IAddressVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IAddress * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IAddress * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IAddress * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IAddress * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IAddress * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IAddress * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [range][in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IAddress * This,
            /* [annotation][in] */ 
            _In_  DISPID dispIdMember,
            /* [annotation][in] */ 
            _In_  REFIID riid,
            /* [annotation][in] */ 
            _In_  LCID lcid,
            /* [annotation][in] */ 
            _In_  WORD wFlags,
            /* [annotation][out][in] */ 
            _In_  DISPPARAMS *pDispParams,
            /* [annotation][out] */ 
            _Out_opt_  VARIANT *pVarResult,
            /* [annotation][out] */ 
            _Out_opt_  EXCEPINFO *pExcepInfo,
            /* [annotation][out] */ 
            _Out_opt_  UINT *puArgErr);
        
        END_INTERFACE
    } IAddressVtbl;

    interface IAddress
    {
        CONST_VTBL struct IAddressVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IAddress_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IAddress_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IAddress_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IAddress_GetTypeInfoCount(This,pctinfo)	\
    ( (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo) ) 

#define IAddress_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    ( (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo) ) 

#define IAddress_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    ( (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId) ) 

#define IAddress_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    ( (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr) ) 


#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IAddress_INTERFACE_DEFINED__ */


#ifndef __IOrder_INTERFACE_DEFINED__
#define __IOrder_INTERFACE_DEFINED__

/* interface IOrder */
/* [unique][nonextensible][dual][uuid][object] */ 


EXTERN_C const IID IID_IOrder;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("BD0971CB-34C3-4B7B-BB5F-24D0D2216A8E")
    IOrder : public IDispatch
    {
    public:
    };
    
    
#else 	/* C style interface */

    typedef struct IOrderVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IOrder * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IOrder * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IOrder * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IOrder * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IOrder * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IOrder * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [range][in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IOrder * This,
            /* [annotation][in] */ 
            _In_  DISPID dispIdMember,
            /* [annotation][in] */ 
            _In_  REFIID riid,
            /* [annotation][in] */ 
            _In_  LCID lcid,
            /* [annotation][in] */ 
            _In_  WORD wFlags,
            /* [annotation][out][in] */ 
            _In_  DISPPARAMS *pDispParams,
            /* [annotation][out] */ 
            _Out_opt_  VARIANT *pVarResult,
            /* [annotation][out] */ 
            _Out_opt_  EXCEPINFO *pExcepInfo,
            /* [annotation][out] */ 
            _Out_opt_  UINT *puArgErr);
        
        END_INTERFACE
    } IOrderVtbl;

    interface IOrder
    {
        CONST_VTBL struct IOrderVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IOrder_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IOrder_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IOrder_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IOrder_GetTypeInfoCount(This,pctinfo)	\
    ( (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo) ) 

#define IOrder_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    ( (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo) ) 

#define IOrder_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    ( (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId) ) 

#define IOrder_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    ( (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr) ) 


#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IOrder_INTERFACE_DEFINED__ */


#ifndef __ILineItem_INTERFACE_DEFINED__
#define __ILineItem_INTERFACE_DEFINED__

/* interface ILineItem */
/* [unique][nonextensible][dual][uuid][object] */ 


EXTERN_C const IID IID_ILineItem;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("8BDC2E69-E5E7-4720-995D-5CB90FE4F03D")
    ILineItem : public IDispatch
    {
    public:
    };
    
    
#else 	/* C style interface */

    typedef struct ILineItemVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ILineItem * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ILineItem * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ILineItem * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ILineItem * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ILineItem * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ILineItem * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [range][in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ILineItem * This,
            /* [annotation][in] */ 
            _In_  DISPID dispIdMember,
            /* [annotation][in] */ 
            _In_  REFIID riid,
            /* [annotation][in] */ 
            _In_  LCID lcid,
            /* [annotation][in] */ 
            _In_  WORD wFlags,
            /* [annotation][out][in] */ 
            _In_  DISPPARAMS *pDispParams,
            /* [annotation][out] */ 
            _Out_opt_  VARIANT *pVarResult,
            /* [annotation][out] */ 
            _Out_opt_  EXCEPINFO *pExcepInfo,
            /* [annotation][out] */ 
            _Out_opt_  UINT *puArgErr);
        
        END_INTERFACE
    } ILineItemVtbl;

    interface ILineItem
    {
        CONST_VTBL struct ILineItemVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ILineItem_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ILineItem_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ILineItem_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ILineItem_GetTypeInfoCount(This,pctinfo)	\
    ( (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo) ) 

#define ILineItem_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    ( (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo) ) 

#define ILineItem_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    ( (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId) ) 

#define ILineItem_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    ( (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr) ) 


#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ILineItem_INTERFACE_DEFINED__ */



#ifndef __ATLExample2012Lib_LIBRARY_DEFINED__
#define __ATLExample2012Lib_LIBRARY_DEFINED__

/* library ATLExample2012Lib */
/* [version][uuid] */ 


EXTERN_C const IID LIBID_ATLExample2012Lib;

EXTERN_C const CLSID CLSID_Customer;

#ifdef __cplusplus

class DECLSPEC_UUID("A3ABB036-B85F-4F80-BFFF-0E5F848D6FED")
Customer;
#endif

EXTERN_C const CLSID CLSID_Address;

#ifdef __cplusplus

class DECLSPEC_UUID("E64952BE-1E0C-474A-914A-56DD94F54750")
Address;
#endif

EXTERN_C const CLSID CLSID_Order;

#ifdef __cplusplus

class DECLSPEC_UUID("71536243-68E6-4E0F-896B-5117C800557E")
Order;
#endif

EXTERN_C const CLSID CLSID_LineItem;

#ifdef __cplusplus

class DECLSPEC_UUID("8380C58C-B9CF-4700-881D-5F336F8785F3")
LineItem;
#endif
#endif /* __ATLExample2012Lib_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


