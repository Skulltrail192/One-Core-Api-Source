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

WINE_DEFAULT_DEBUG_CHANNEL(ole32);

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

DEFINE_GUID(IID_IAgileReference, 0xC03F6A43, 0x65A4, 0x98, 0x18, 0x98, 0x7E, 0xE0, 0xB8, 0x10, 0xD2, 0xA6, 0xF2);
DEFINE_GUID(IID_INoMarshal, 0xecc8691b, 0xc1db, 0x4dc0, 0x85, 0x5e, 0x65, 0xf6, 0xc5, 0x51, 0xaf, 0x49);

/*
	This script is entirely dedicated to the implementation of COM-based functions.
	These functions is ever-more needed by applications, and you just can't stub it!
	
	Here's the APIs supported:
		RoGetAgileReference [not compared with real Win10 implementation]
		
*/
typedef struct {
	void(**lpVtbl)();
	ULONG cRef;
	IUnknown *agileObject;
} IAgileReferenceW7;

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

 	    typedef struct IAgileReferenceVtbl
 	    {
 	        BEGIN_INTERFACE
 	
 	        HRESULT ( STDMETHODCALLTYPE *QueryInterface )(
 	            IAgileReference * This,
 	            /* [in] */ REFIID riid,
 	            /* [annotation][iid_is][out] */
 	            _COM_Outptr_  void **ppvObject);
 	
 	        ULONG ( STDMETHODCALLTYPE *AddRef )(
 	            IAgileReference * This);
 	
 	        ULONG ( STDMETHODCALLTYPE *Release )(
 	            IAgileReference * This);
 	
 	        HRESULT ( STDMETHODCALLTYPE *Resolve )(
 	            IAgileReference * This,
 	            /* [in] */ REFIID riid,
 	            /* [iid_is][retval][out] */ void **ppvObjectReference);
 	
 	        END_INTERFACE
 	    } IAgileReferenceVtbl;

static const void *IAgileReferenceW7_Vtable = {
    IAgileReferenceW7_QueryInterface,
    IAgileReferenceW7_AddRef,
    IAgileReferenceW7_Release,
    IAgileReferenceW7_Resolve
};

// The function itself.
HRESULT WINAPI RoGetAgileReference(int options, // Ignored
	REFIID riid, // Ignored also
	IUnknown *pUnk,
	IAgileReference **ppAgileReference
	) {
	if (pUnk == 0)
		return E_INVALIDARG;
	if (ppAgileReference == 0)
		return E_INVALIDARG;
	INoMarshal *INo;
	if (IUnknown_QueryInterface(pUnk, &IID_INoMarshal, &INo)) {
		INoMarshal_Release(INo);
		return CO_E_NOT_SUPPORTED;
	}
	// Allocate a memory pointer.
	IAgileReferenceW7 *Reference = CoTaskMemAlloc(sizeof(IAgileReferenceW7));
	if (Reference == 0) {
		return E_OUTOFMEMORY;
	}
	Reference->cRef = 1;
	Reference->lpVtbl = IAgileReferenceW7_Vtable;
	Reference->agileObject = pUnk;
	IUnknown_AddRef(pUnk);
	*ppAgileReference = Reference;
	return 0;
}