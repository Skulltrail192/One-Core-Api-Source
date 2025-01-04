/*++

Copyright (c) 2024 Shorthorn Project

Module Name:

    thread.c

Abstract:

    This module implements Win32 Shell String Functions

Author:

    Skulltrail 10-September-2024

Revision History:

--*/

#include "main.h"
#include <unknwn.h>

WINE_DEFAULT_DEBUG_CHANNEL(shlwapi);

/* Internal thread information structure */
typedef struct tagSHLWAPI_THREAD_INFO
{
  LPTHREAD_START_ROUTINE pfnThreadProc; /* Thread start */
  LPTHREAD_START_ROUTINE pfnCallback;   /* Thread initialisation */
  PVOID     pData;                      /* Application specific data */
  BOOL      bInitCom;                   /* Initialise COM for the thread? */
  HANDLE    hEvent;                     /* Signal for creator to continue */
  IUnknown *refThread;                  /* Reference to thread creator */
  IUnknown *refIE;                      /* Reference to the IE process */
} SHLWAPI_THREAD_INFO, *LPSHLWAPI_THREAD_INFO;

typedef struct
{
  IUnknown IUnknown_iface;
  LONG  *ref;
} threadref;

static inline threadref *impl_from_IUnknown(IUnknown *iface)
{
  return CONTAINING_RECORD(iface, threadref, IUnknown_iface);
}

static HRESULT WINAPI threadref_QueryInterface(IUnknown *iface, REFIID riid, LPVOID *ppvObj)
{
  threadref * This = impl_from_IUnknown(iface);

  TRACE("(%p, %s, %p)\n", This, debugstr_guid(riid), ppvObj);

  if (ppvObj == NULL)
    return E_POINTER;

  if (IsEqualGUID(&IID_IUnknown, riid)) {
    TRACE("(%p)->(IID_IUnknown %p)\n", This, ppvObj);
    *ppvObj = This;
    IUnknown_AddRef((IUnknown*)*ppvObj);
    return S_OK;
  }

  *ppvObj = NULL;
  FIXME("(%p, %s, %p) interface not supported\n", This, debugstr_guid(riid), ppvObj);
  return E_NOINTERFACE;
}

static ULONG WINAPI threadref_AddRef(IUnknown *iface)
{
  threadref * This = impl_from_IUnknown(iface);

  TRACE("(%p)\n", This);
  return InterlockedIncrement(This->ref);
}

static ULONG WINAPI threadref_Release(IUnknown *iface)
{
  LONG refcount;
  threadref * This = impl_from_IUnknown(iface);

  TRACE("(%p)\n", This);

  refcount = InterlockedDecrement(This->ref);
  if (!refcount)
      HeapFree(GetProcessHeap(), 0, This);

  return refcount;
}

/* VTable */
static const IUnknownVtbl threadref_vt =
{
  threadref_QueryInterface,
  threadref_AddRef,
  threadref_Release,
};

/*************************************************************************
 * SHCreateThreadRef [SHLWAPI.@]
 *
 * Create a per-thread IUnknown object
 *
 * PARAMS
 *   lprefcount [I] Pointer to a LONG to be used as refcount
 *   lppUnknown [O] Destination to receive the created object reference
 *
 * RETURNS
 *   Success: S_OK. lppUnknown is set to the object reference.
 *   Failure: E_INVALIDARG, if a parameter is NULL
 */
HRESULT WINAPI SHCreateThreadRef(LONG *lprefcount, IUnknown **lppUnknown)
{
  threadref * This;
  TRACE("(%p, %p)\n", lprefcount, lppUnknown);

  if (!lprefcount || !lppUnknown)
    return E_INVALIDARG;

  This = HeapAlloc(GetProcessHeap(), 0, sizeof(threadref));
  This->IUnknown_iface.lpVtbl = &threadref_vt;
  This->ref = lprefcount;

  *lprefcount = 1;
  *lppUnknown = &This->IUnknown_iface;
  TRACE("=> returning S_OK with %p\n", This);
  return S_OK;
}