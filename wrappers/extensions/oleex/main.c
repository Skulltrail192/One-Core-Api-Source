/*++

Copyright (c) 2023 Shorthorn Project

Module Name:

    main.c

Abstract:

    This module implements COM Main functions APIs

Author:

    Skulltrail 12-October-2023

Revision History:

--*/

#define WIN32_NO_STATUS

#include "main.h"
#include <stdarg.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "initguid.h"
#include "ocidl.h"
#include "shellscalingapi.h"
#include "shlwapi.h"
#include "unknwn.h"

#include "wine/debug.h"
#include "wine/heap.h"

WINE_DEFAULT_DEBUG_CHANNEL(ole32);

DEFINE_GUID(IID_IUnknown, 0x00000000, 0x0000, 0x0000, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46);

static struct list registered_classes = LIST_INIT(registered_classes);

typedef interface IUnknown IActivationFilter;

IActivationFilter globalActivationFilter = {0};

static CRITICAL_SECTION registered_classes_cs;
static CRITICAL_SECTION_DEBUG registered_classes_cs_debug =
{
    0, 0, &registered_classes_cs,
    { &registered_classes_cs_debug.ProcessLocksList, &registered_classes_cs_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": registered_classes_cs") }
};
static CRITICAL_SECTION registered_classes_cs = { &registered_classes_cs_debug, -1, 0, 0, 0, 0 };

/* will create if necessary */
static inline struct oletls *COM_CurrentInfo(void)
{
    if (!NtCurrentTeb()->ReservedForOle)
        NtCurrentTeb()->ReservedForOle = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(struct oletls));

    return NtCurrentTeb()->ReservedForOle;
}

HRESULT 
WINAPI
CoDisconnectContext(
  _In_ DWORD dwTimeout
)
{
	return S_OK;
}

/***********************************************************************
 *      CoGetActivationState (ole32.@)
 */
HRESULT WINAPI CoGetActivationState(GUID guid, DWORD unknown, DWORD *unknown2)
{
    return E_NOTIMPL;
}

/***********************************************************************
 *      CoGetCallState (ole32.@)
 */
HRESULT WINAPI CoGetCallState(int unknown, PULONG unknown2)
{
    return E_NOTIMPL;
}

HRESULT WINAPI CoRegisterActivationFilter(IActivationFilter *pActivationFilter)
{
  IActivationFilter *activationFilter; // rax

  if ( !pActivationFilter )
    return 0x80070057;
  activationFilter = (IActivationFilter *)_InterlockedCompareExchange64(
                                            (signed __int64*)&globalActivationFilter,
                                            (signed __int64)pActivationFilter,
                                            0i64);
  if ( !activationFilter || activationFilter == pActivationFilter )
    return 0;
  else
    return 0x80004021;
}

typedef interface IAgileReferenceW7 IAgileReferenceW7;

/*****************************************************************************
 * IAgileReference interface
 */
#ifndef __IAgileReference_INTERFACE_DEFINED__
#define __IAgileReference_INTERFACE_DEFINED__

DEFINE_GUID(IID_IAgileReference, 0xc03f6a43, 0x65a4, 0x9818, 0x98,0x7e, 0xe0,0xb8,0x10,0xd2,0xa6,0xf2);
#if defined(__cplusplus) && !defined(CINTERFACE)
MIDL_INTERFACE("c03f6a43-65a4-9818-987e-e0b810d2a6f2")
IAgileReferenceW7 : public IUnknown
{
    virtual HRESULT STDMETHODCALLTYPE Resolve(
        REFIID riid,
        void **ppv) = 0;

};
#ifdef __CRT_UUID_DECL
__CRT_UUID_DECL(IAgileReferenceW7, 0xc03f6a43, 0x65a4, 0x9818, 0x98,0x7e, 0xe0,0xb8,0x10,0xd2,0xa6,0xf2)
#endif
#else
typedef struct IAgileReferenceVtblW7 {
    BEGIN_INTERFACE

    /*** IUnknown methods ***/
    HRESULT (STDMETHODCALLTYPE *QueryInterface)(
        IAgileReferenceW7 *This,
        REFIID riid,
        void **ppvObject);

    ULONG (STDMETHODCALLTYPE *AddRef)(
        IAgileReferenceW7 *This);

    ULONG (STDMETHODCALLTYPE *Release)(
        IAgileReferenceW7 *This);

    /*** IAgileReference methods ***/
    HRESULT (STDMETHODCALLTYPE *Resolve)(
        IAgileReferenceW7 *This,
        REFIID riid,
        void **ppv);

    END_INTERFACE
} IAgileReferenceVtblW7;

interface IAgileReferenceW7 {
    IAgileReferenceVtblW7 lpVtbl;
	ULONG cRef;
	IUnknown *agileObject;	
};

#endif
#endif

STDMETHODIMP IAgileReferenceW7_QueryInterface(IAgileReferenceW7 *lpMyObj, REFIID riid,
                                   LPVOID *lppvObj);
								   
STDMETHODIMP IAgileReferenceW7_QueryInterface(IAgileReferenceW7 *lpMyObj, REFIID riid,
                                   LPVOID *lppvObj)
{
	if (!lpMyObj) 
		return HRESULT_FROM_WIN32(E_INVALIDARG);
	
	if (!riid) 
		return HRESULT_FROM_WIN32(E_INVALIDARG);
	
	if (IsEqualIID(riid, &IID_IAgileReference) ||
		IsEqualIID(riid, &IID_IUnknown)
		) {
		InterlockedIncrement(&(lpMyObj->cRef));
		*lppvObj = lpMyObj;
		return 0;
	}
    return HRESULT_FROM_WIN32(E_NOINTERFACE);
}

STDMETHODIMP_(ULONG) IAgileReferenceW7_AddRef(IAgileReferenceW7 *lpMyObj)
{
    return InterlockedIncrement(&(lpMyObj->cRef));
}

STDMETHODIMP_(ULONG) IAgileReferenceW7_Release(IAgileReferenceW7 *lpMyObj)
{
	ULONG NewRef = InterlockedDecrement(&(lpMyObj->cRef));
	if (NewRef == 0) {
		IUnknown_Release(lpMyObj->agileObject);
		CoTaskMemFree(lpMyObj);
	}
    return NewRef;
}

STDMETHODIMP IAgileReferenceW7_Resolve(IAgileReferenceW7 *lpMyObj,
	REFIID Ref,
	void **ppvObjectReference)
{
	return IUnknown_QueryInterface(lpMyObj->agileObject, Ref, ppvObjectReference);
}

static IAgileReferenceVtblW7 vTable = {
    IAgileReferenceW7_QueryInterface,
    IAgileReferenceW7_AddRef,
    IAgileReferenceW7_Release,
    IAgileReferenceW7_Resolve
};

typedef interface INoMarshal INoMarshal;

#ifndef __INoMarshal_INTERFACE_DEFINED__
#define __INoMarshal_INTERFACE_DEFINED__

DEFINE_GUID(IID_INoMarshal, 0xecc8691b, 0xc1db, 0x4dc0, 0x85,0x5e, 0x65,0xf6,0xc5,0x51,0xaf,0x49);
#if defined(__cplusplus) && !defined(CINTERFACE)
MIDL_INTERFACE("ecc8691b-c1db-4dc0-855e-65f6c551af49")
INoMarshal : public IUnknown
{
};
#ifdef __CRT_UUID_DECL
__CRT_UUID_DECL(INoMarshal, 0xecc8691b, 0xc1db, 0x4dc0, 0x85,0x5e, 0x65,0xf6,0xc5,0x51,0xaf,0x49)
#endif
#else
typedef struct INoMarshalVtbl {
    BEGIN_INTERFACE

    /*** IUnknown methods ***/
    HRESULT (STDMETHODCALLTYPE *QueryInterface)(
        INoMarshal* This,
        REFIID riid,
        void **ppvObject);

    ULONG (STDMETHODCALLTYPE *AddRef)(
        INoMarshal* This);

    ULONG (STDMETHODCALLTYPE *Release)(
        INoMarshal* This);

    END_INTERFACE
} INoMarshalVtbl;
interface INoMarshal {
    CONST_VTBL INoMarshalVtbl* lpVtbl;
	
};

#endif
#endif

#ifdef COBJMACROS
#ifndef WIDL_C_INLINE_WRAPPERS
/*** IUnknown methods ***/
#define INoMarshal_QueryInterface(This,riid,ppvObject) (This)->lpVtbl->QueryInterface(This,riid,ppvObject)
#define INoMarshal_AddRef(This) (This)->lpVtbl->AddRef(This)
#define INoMarshal_Release(This) (This)->lpVtbl->Release(This)
#else
/*** IUnknown methods ***/
static FORCEINLINE HRESULT INoMarshal_QueryInterface(INoMarshal* This,REFIID riid,void **ppvObject) {
    return This->lpVtbl->QueryInterface(This,riid,ppvObject);
}
static FORCEINLINE ULONG INoMarshal_AddRef(INoMarshal* This) {
    return This->lpVtbl->AddRef(This);
}
static FORCEINLINE ULONG INoMarshal_Release(INoMarshal* This) {
    return This->lpVtbl->Release(This);
}
#endif
#endif

// The function itself.
HRESULT 
WINAPI 
RoGetAgileReference(
    int options, // Ignored
	REFIID riid, // Ignored also
	IUnknown *pUnk,
	IAgileReferenceW7 **ppAgileReference
) 
{
	INoMarshal *INo;
	IAgileReferenceW7 *Reference;
	if (pUnk == 0)
		return E_INVALIDARG;
	if (ppAgileReference == 0)
		return E_INVALIDARG;
	
	if (IUnknown_QueryInterface(pUnk, &IID_INoMarshal, &INo)) {
		INoMarshal_Release(INo);
		return CO_E_NOT_SUPPORTED;
	}
	// Allocate a memory pointer.
	Reference = CoTaskMemAlloc(sizeof(IAgileReferenceW7));
	if (Reference == 0) {
		return E_OUTOFMEMORY;
	}
	Reference->cRef = 1;
	Reference->lpVtbl = vTable;
	Reference->agileObject = pUnk;
	IUnknown_AddRef(pUnk);
	*ppAgileReference = Reference;
	return 0;
}