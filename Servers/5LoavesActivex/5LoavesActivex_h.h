

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 8.00.0595 */
/* at Thu Nov 07 16:04:57 2013
 */
/* Compiler settings for 5LoavesActivex.odl:
    Oicf, W1, Zp8, env=Win32 (32b run), target_arch=X86 8.00.0595 
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


#ifndef ___5LoavesActivex_h_h__
#define ___5LoavesActivex_h_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef ___D5Loaves_FWD_DEFINED__
#define ___D5Loaves_FWD_DEFINED__
typedef interface _D5Loaves _D5Loaves;

#endif 	/* ___D5Loaves_FWD_DEFINED__ */


#ifndef ___D5LoavesEvents_FWD_DEFINED__
#define ___D5LoavesEvents_FWD_DEFINED__
typedef interface _D5LoavesEvents _D5LoavesEvents;

#endif 	/* ___D5LoavesEvents_FWD_DEFINED__ */


#ifndef __FiveLoaves_FWD_DEFINED__
#define __FiveLoaves_FWD_DEFINED__

#ifdef __cplusplus
typedef class FiveLoaves FiveLoaves;
#else
typedef struct FiveLoaves FiveLoaves;
#endif /* __cplusplus */

#endif 	/* __FiveLoaves_FWD_DEFINED__ */


#ifdef __cplusplus
extern "C"{
#endif 


/* interface __MIDL_itf__5LoavesActivex_0000_0000 */
/* [local] */ 

#pragma once
#pragma region Desktop Family
#pragma endregion


extern RPC_IF_HANDLE __MIDL_itf__5LoavesActivex_0000_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf__5LoavesActivex_0000_0000_v0_0_s_ifspec;


#ifndef __FiveLOAVESLib_LIBRARY_DEFINED__
#define __FiveLOAVESLib_LIBRARY_DEFINED__

/* library FiveLOAVESLib */
/* [control][helpstring][helpfile][version][uuid] */ 


EXTERN_C const IID LIBID_FiveLOAVESLib;

#ifndef ___D5Loaves_DISPINTERFACE_DEFINED__
#define ___D5Loaves_DISPINTERFACE_DEFINED__

/* dispinterface _D5Loaves */
/* [hidden][helpstring][uuid] */ 


EXTERN_C const IID DIID__D5Loaves;

#if defined(__cplusplus) && !defined(CINTERFACE)

    MIDL_INTERFACE("C947F177-CAA4-4A05-ABCC-94B0B4CE3E3A")
    _D5Loaves : public IDispatch
    {
    };
    
#else 	/* C style interface */

    typedef struct _D5LoavesVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            _D5Loaves * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            _D5Loaves * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            _D5Loaves * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            _D5Loaves * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            _D5Loaves * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            _D5Loaves * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [range][in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            _D5Loaves * This,
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
    } _D5LoavesVtbl;

    interface _D5Loaves
    {
        CONST_VTBL struct _D5LoavesVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define _D5Loaves_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define _D5Loaves_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define _D5Loaves_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define _D5Loaves_GetTypeInfoCount(This,pctinfo)	\
    ( (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo) ) 

#define _D5Loaves_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    ( (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo) ) 

#define _D5Loaves_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    ( (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId) ) 

#define _D5Loaves_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    ( (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */


#endif 	/* ___D5Loaves_DISPINTERFACE_DEFINED__ */


#ifndef ___D5LoavesEvents_DISPINTERFACE_DEFINED__
#define ___D5LoavesEvents_DISPINTERFACE_DEFINED__

/* dispinterface _D5LoavesEvents */
/* [helpstring][uuid] */ 


EXTERN_C const IID DIID__D5LoavesEvents;

#if defined(__cplusplus) && !defined(CINTERFACE)

    MIDL_INTERFACE("C55CB491-48F6-4161-ADDE-C0CE8B30B9E3")
    _D5LoavesEvents : public IDispatch
    {
    };
    
#else 	/* C style interface */

    typedef struct _D5LoavesEventsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            _D5LoavesEvents * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            _D5LoavesEvents * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            _D5LoavesEvents * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            _D5LoavesEvents * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            _D5LoavesEvents * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            _D5LoavesEvents * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [range][in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            _D5LoavesEvents * This,
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
    } _D5LoavesEventsVtbl;

    interface _D5LoavesEvents
    {
        CONST_VTBL struct _D5LoavesEventsVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define _D5LoavesEvents_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define _D5LoavesEvents_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define _D5LoavesEvents_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define _D5LoavesEvents_GetTypeInfoCount(This,pctinfo)	\
    ( (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo) ) 

#define _D5LoavesEvents_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    ( (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo) ) 

#define _D5LoavesEvents_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    ( (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId) ) 

#define _D5LoavesEvents_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    ( (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */


#endif 	/* ___D5LoavesEvents_DISPINTERFACE_DEFINED__ */


EXTERN_C const CLSID CLSID_FiveLoaves;

#ifdef __cplusplus

class DECLSPEC_UUID("3F917A51-066B-4AD7-99D2-95349DA3C176")
FiveLoaves;
#endif
#endif /* __FiveLOAVESLib_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


