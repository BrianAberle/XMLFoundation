/* this ALWAYS GENERATED file contains the definitions for the interfaces */


/* File created by MIDL compiler version 5.01.0164 */
/* at Tue Jan 05 07:46:02 2016
 */
/* Compiler settings for C:\XMLFoundation\Examples\C++\ExATLCOM\ExATLCOM.idl:
    Oicf (OptLev=i2), W1, Zp8, env=Win32, ms_ext, c_ext
    error checks: allocation ref bounds_check enum stub_data 
*/
//@@MIDL_FILE_HEADING(  )


/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 440
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

#ifndef __ExATLCOM_h__
#define __ExATLCOM_h__

#ifdef __cplusplus
extern "C"{
#endif 

/* Forward Declarations */ 

#ifndef __IMyATLObj_FWD_DEFINED__
#define __IMyATLObj_FWD_DEFINED__
typedef interface IMyATLObj IMyATLObj;
#endif 	/* __IMyATLObj_FWD_DEFINED__ */


#ifndef __MyATLObj_FWD_DEFINED__
#define __MyATLObj_FWD_DEFINED__

#ifdef __cplusplus
typedef class MyATLObj MyATLObj;
#else
typedef struct MyATLObj MyATLObj;
#endif /* __cplusplus */

#endif 	/* __MyATLObj_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 

#ifndef __IMyATLObj_INTERFACE_DEFINED__
#define __IMyATLObj_INTERFACE_DEFINED__

/* interface IMyATLObj */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IMyATLObj;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("49B53179-A937-4560-A006-64142A62ED4F")
    IMyATLObj : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_TheCount( 
            /* [retval][out] */ long __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_TheCount( 
            /* [in] */ long newVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_XMLState( 
            /* [retval][out] */ BSTR __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_XMLState( 
            /* [in] */ BSTR newVal) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE SetValues( 
            /* [in] */ long lValue,
            /* [in] */ BSTR strValue) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_TheString( 
            /* [retval][out] */ BSTR __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_TheString( 
            /* [in] */ BSTR newVal) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE GetMyInterface( 
            /* [in] */ long Key,
            /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *pI) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Dump( 
            /* [retval][out] */ BSTR __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE RemoveSubObjects( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IMyATLObjVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IMyATLObj __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IMyATLObj __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IMyATLObj __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IMyATLObj __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IMyATLObj __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IMyATLObj __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IMyATLObj __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_TheCount )( 
            IMyATLObj __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_TheCount )( 
            IMyATLObj __RPC_FAR * This,
            /* [in] */ long newVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_XMLState )( 
            IMyATLObj __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_XMLState )( 
            IMyATLObj __RPC_FAR * This,
            /* [in] */ BSTR newVal);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetValues )( 
            IMyATLObj __RPC_FAR * This,
            /* [in] */ long lValue,
            /* [in] */ BSTR strValue);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_TheString )( 
            IMyATLObj __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_TheString )( 
            IMyATLObj __RPC_FAR * This,
            /* [in] */ BSTR newVal);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetMyInterface )( 
            IMyATLObj __RPC_FAR * This,
            /* [in] */ long Key,
            /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *pI);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Dump )( 
            IMyATLObj __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pVal);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *RemoveSubObjects )( 
            IMyATLObj __RPC_FAR * This);
        
        END_INTERFACE
    } IMyATLObjVtbl;

    interface IMyATLObj
    {
        CONST_VTBL struct IMyATLObjVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IMyATLObj_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IMyATLObj_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IMyATLObj_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IMyATLObj_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IMyATLObj_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IMyATLObj_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IMyATLObj_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IMyATLObj_get_TheCount(This,pVal)	\
    (This)->lpVtbl -> get_TheCount(This,pVal)

#define IMyATLObj_put_TheCount(This,newVal)	\
    (This)->lpVtbl -> put_TheCount(This,newVal)

#define IMyATLObj_get_XMLState(This,pVal)	\
    (This)->lpVtbl -> get_XMLState(This,pVal)

#define IMyATLObj_put_XMLState(This,newVal)	\
    (This)->lpVtbl -> put_XMLState(This,newVal)

#define IMyATLObj_SetValues(This,lValue,strValue)	\
    (This)->lpVtbl -> SetValues(This,lValue,strValue)

#define IMyATLObj_get_TheString(This,pVal)	\
    (This)->lpVtbl -> get_TheString(This,pVal)

#define IMyATLObj_put_TheString(This,newVal)	\
    (This)->lpVtbl -> put_TheString(This,newVal)

#define IMyATLObj_GetMyInterface(This,Key,pI)	\
    (This)->lpVtbl -> GetMyInterface(This,Key,pI)

#define IMyATLObj_get_Dump(This,pVal)	\
    (This)->lpVtbl -> get_Dump(This,pVal)

#define IMyATLObj_RemoveSubObjects(This)	\
    (This)->lpVtbl -> RemoveSubObjects(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IMyATLObj_get_TheCount_Proxy( 
    IMyATLObj __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *pVal);


void __RPC_STUB IMyATLObj_get_TheCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IMyATLObj_put_TheCount_Proxy( 
    IMyATLObj __RPC_FAR * This,
    /* [in] */ long newVal);


void __RPC_STUB IMyATLObj_put_TheCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IMyATLObj_get_XMLState_Proxy( 
    IMyATLObj __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pVal);


void __RPC_STUB IMyATLObj_get_XMLState_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IMyATLObj_put_XMLState_Proxy( 
    IMyATLObj __RPC_FAR * This,
    /* [in] */ BSTR newVal);


void __RPC_STUB IMyATLObj_put_XMLState_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IMyATLObj_SetValues_Proxy( 
    IMyATLObj __RPC_FAR * This,
    /* [in] */ long lValue,
    /* [in] */ BSTR strValue);


void __RPC_STUB IMyATLObj_SetValues_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IMyATLObj_get_TheString_Proxy( 
    IMyATLObj __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pVal);


void __RPC_STUB IMyATLObj_get_TheString_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IMyATLObj_put_TheString_Proxy( 
    IMyATLObj __RPC_FAR * This,
    /* [in] */ BSTR newVal);


void __RPC_STUB IMyATLObj_put_TheString_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IMyATLObj_GetMyInterface_Proxy( 
    IMyATLObj __RPC_FAR * This,
    /* [in] */ long Key,
    /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *pI);


void __RPC_STUB IMyATLObj_GetMyInterface_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IMyATLObj_get_Dump_Proxy( 
    IMyATLObj __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pVal);


void __RPC_STUB IMyATLObj_get_Dump_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IMyATLObj_RemoveSubObjects_Proxy( 
    IMyATLObj __RPC_FAR * This);


void __RPC_STUB IMyATLObj_RemoveSubObjects_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IMyATLObj_INTERFACE_DEFINED__ */



#ifndef __EXATLCOMLib_LIBRARY_DEFINED__
#define __EXATLCOMLib_LIBRARY_DEFINED__

/* library EXATLCOMLib */
/* [helpstring][version][uuid] */ 


EXTERN_C const IID LIBID_EXATLCOMLib;

EXTERN_C const CLSID CLSID_MyATLObj;

#ifdef __cplusplus

class DECLSPEC_UUID("7373BC84-8181-47CC-B4CF-532D55E4B828")
MyATLObj;
#endif
#endif /* __EXATLCOMLib_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

unsigned long             __RPC_USER  BSTR_UserSize(     unsigned long __RPC_FAR *, unsigned long            , BSTR __RPC_FAR * ); 
unsigned char __RPC_FAR * __RPC_USER  BSTR_UserMarshal(  unsigned long __RPC_FAR *, unsigned char __RPC_FAR *, BSTR __RPC_FAR * ); 
unsigned char __RPC_FAR * __RPC_USER  BSTR_UserUnmarshal(unsigned long __RPC_FAR *, unsigned char __RPC_FAR *, BSTR __RPC_FAR * ); 
void                      __RPC_USER  BSTR_UserFree(     unsigned long __RPC_FAR *, BSTR __RPC_FAR * ); 

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif
