/*++

Copyright (c) 2023 Shorthorn Project

Module Name:

    main.c

Abstract:

    This module implements COM Marshaling functions APIs

Author:

    Skulltrail 07-November-2023

Revision History:

--*/

#define WIN32_NO_STATUS

#include "main.h"
#define COBJMACROS
#include "objbase.h"
#include "initguid.h"
#include "roapi.h"
#include "roparameterizediid.h"
#include "roerrorapi.h"
#include "winstring.h"

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

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(combase);

#define COINIT_DISABLEOLE1DDE 4

const GUID GUID_NULL = { 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0, 0 } };

static const char *debugstr_hstring(HSTRING hstr)
{
    const WCHAR *str;
    UINT32 len;
    if (hstr && !((ULONG_PTR)hstr >> 16)) return "(invalid)";
    str = WindowsGetStringRawBuffer(hstr, &len);
    return wine_dbgstr_wn(str, len);
}

static HRESULT get_library_for_classid(const WCHAR *classid, WCHAR **out)
{
    HKEY hkey_root, hkey_class;
    DWORD type, size;
    HRESULT hr;
    WCHAR *buf = NULL;

    *out = NULL;

    /* load class registry key */
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\WindowsRuntime\\ActivatableClassId",
                      0, KEY_READ, &hkey_root))
        return REGDB_E_READREGDB;
    if (RegOpenKeyExW(hkey_root, classid, 0, KEY_READ, &hkey_class))
    {
        WARN("Class %s not found in registry\n", debugstr_w(classid));
        RegCloseKey(hkey_root);
        return REGDB_E_CLASSNOTREG;
    }
    RegCloseKey(hkey_root);

    /* load (and expand) DllPath registry value */
    if (RegQueryValueExW(hkey_class, L"DllPath", NULL, &type, NULL, &size))
    {
        hr = REGDB_E_READREGDB;
        goto done;
    }
    if (type != REG_SZ && type != REG_EXPAND_SZ)
    {
        hr = REGDB_E_READREGDB;
        goto done;
    }
    if (!(buf = HeapAlloc(GetProcessHeap(), 0, size)))
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }
    if (RegQueryValueExW(hkey_class, L"DllPath", NULL, NULL, (BYTE *)buf, &size))
    {
        hr = REGDB_E_READREGDB;
        goto done;
    }
    if (type == REG_EXPAND_SZ)
    {
        WCHAR *expanded;
        DWORD len = ExpandEnvironmentStringsW(buf, NULL, 0);
        if (!(expanded = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR))))
        {
            hr = E_OUTOFMEMORY;
            goto done;
        }
        ExpandEnvironmentStringsW(buf, expanded, len);
        HeapFree(GetProcessHeap(), 0, buf);
        buf = expanded;
    }

    *out = buf;
    return S_OK;

done:
    HeapFree(GetProcessHeap(), 0, buf);
    RegCloseKey(hkey_class);
    return hr;
}

/***********************************************************************
 *      CleanupTlsOleState (combase.@)
 */
void WINAPI CleanupTlsOleState(void *unknown)
{
    FIXME("(%p): stub\n", unknown);
}

HRESULT WINAPI RoInitialize(RO_INIT_TYPE type)
{
    switch (type) {
    case RO_INIT_SINGLETHREADED:
        return CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLEOLE1DDE);
    case RO_INIT_MULTITHREADED:
        return CoInitializeEx(NULL, COINIT_MULTITHREADED | COINIT_DISABLEOLE1DDE);
    default:
        // Multithreaded by default!
        return CoInitializeEx(NULL, COINIT_MULTITHREADED | COINIT_DISABLEOLE1DDE);
    }
}

void WINAPI RoUninitialize(void)
{
    CoUninitialize();
}

/***********************************************************************
 *      RoGetActivationFactory (combase.@)
 */
DECLSPEC_HOTPATCH 
HRESULT 
WINAPI 
RoGetActivationFactory(HSTRING classid, REFIID iid, void **class_factory)
{
    PFNGETACTIVATIONFACTORY pDllGetActivationFactory;
    IActivationFactory *factory;
    WCHAR *library;
    HMODULE module;
    HRESULT hr;

    FIXME("(%s, %s, %p): semi-stub\n", debugstr_hstring(classid), debugstr_guid(iid), class_factory);

    if (!iid || !class_factory)
        return E_INVALIDARG;

    hr = get_library_for_classid(WindowsGetStringRawBuffer(classid, NULL), &library);
    if (FAILED(hr))
    {
        ERR("Failed to find library for %s\n", debugstr_hstring(classid));
        return hr;
    }

    if (!(module = LoadLibraryW(library)))
    {
        ERR("Failed to load module %s\n", debugstr_w(library));
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto done;
    }

    if (!(pDllGetActivationFactory = (void *)GetProcAddress(module, "DllGetActivationFactory")))
    {
        ERR("Module %s does not implement DllGetActivationFactory\n", debugstr_w(library));
        hr = E_FAIL;
        goto done;
    }

    TRACE("Found library %s for class %s\n", debugstr_w(library), debugstr_hstring(classid));

    hr = pDllGetActivationFactory(classid, &factory);
    if (SUCCEEDED(hr))
    {
        hr = IActivationFactory_QueryInterface(factory, iid, class_factory);
        if (SUCCEEDED(hr))
        {
            TRACE("Created interface %p\n", *class_factory);
            module = NULL;
        }
        IActivationFactory_Release(factory);
    }

done:
    HeapFree(GetProcessHeap(), 0, library);
    if (module) FreeLibrary(module);
    return hr;
}

/***********************************************************************
 *      RoActivateInstance (combase.@)
 */
HRESULT WINAPI RoActivateInstance(HSTRING classid, IInspectable **instance)
{
    IActivationFactory *factory;
    HRESULT hr;

    FIXME("(%p, %p): semi-stub\n", classid, instance);

    hr = RoGetActivationFactory(classid, &IID_IActivationFactory, (void **)&factory);
    if (SUCCEEDED(hr))
    {
        hr = IActivationFactory_ActivateInstance(factory, instance);
        IActivationFactory_Release(factory);
    }

    return hr;
}

/***********************************************************************
 *      RoGetParameterizedTypeInstanceIID (combase.@)
 */
HRESULT WINAPI RoGetParameterizedTypeInstanceIID(UINT32 name_element_count, const WCHAR **name_elements,
                                                 const IRoMetaDataLocator *meta_data_locator, GUID *iid,
                                                 ROPARAMIIDHANDLE *hiid)
{
    FIXME("stub: %d %p %p %p %p\n", name_element_count, name_elements, meta_data_locator, iid, hiid);
    if (iid) *iid = GUID_NULL;
    if (hiid) *hiid = INVALID_HANDLE_VALUE;
    return E_NOTIMPL;
}

/***********************************************************************
 *      RoGetApartmentIdentifier (combase.@)
 */
HRESULT WINAPI RoGetApartmentIdentifier(UINT64 *identifier)
{
    FIXME("(%p): stub\n", identifier);

    if (!identifier)
        return E_INVALIDARG;

    *identifier = 0xdeadbeef;
    return S_OK;
}

/***********************************************************************
 *      RoGetServerActivatableClasses (combase.@)
 */
HRESULT WINAPI RoGetServerActivatableClasses(HSTRING name, HSTRING **classes, DWORD *count)
{
    FIXME("(%p, %p, %p): stub\n", name, classes, count);

    if (count)
        *count = 0;
    return S_OK;
}

HRESULT
WINAPI
RoRegisterActivationFactories(
	_In_reads_(count) HSTRING* activatableClassIds,
	_In_reads_(count) PFNGETACTIVATIONFACTORY* activationFactoryCallbacks,
	_In_ UINT32 count,
	_Out_ RO_REGISTRATION_COOKIE* cookie
)
{
	if (cookie)
	   *cookie = NULL;

	return E_NOTIMPL;
}

/***********************************************************************
 *      RoRegisterForApartmentShutdown (combase.@)
 */
HRESULT WINAPI RoRegisterForApartmentShutdown(IApartmentShutdown *callback,
        UINT64 *identifier, APARTMENT_SHUTDOWN_REGISTRATION_COOKIE *cookie)
{
    HRESULT hr;

    FIXME("(%p, %p, %p): stub\n", callback, identifier, cookie);

    hr = RoGetApartmentIdentifier(identifier);
    if (FAILED(hr))
        return hr;

    if (cookie)
        *cookie = (void *)0xcafecafe;
    return S_OK;
}


/***********************************************************************
 *      RoOriginateError (combase.@)
 */
BOOL WINAPI RoOriginateError(HRESULT error, HSTRING message)
{
    FIXME("%#lx, %s: stub\n", error,message);
    return FALSE;
}

/***********************************************************************
 *      GetRestrictedErrorInfo (combase.@)
 */
HRESULT WINAPI GetRestrictedErrorInfo(IRestrictedErrorInfo **info)
{
    FIXME( "(%p)\n", info );
    return S_OK;
}

/***********************************************************************
 *      RoOriginateLanguageException (combase.@)
 */
BOOL WINAPI RoOriginateLanguageException(HRESULT error, HSTRING message, IUnknown *language_exception)
{
    FIXME("%#lx, %s, %p: stub\n", error, message, language_exception);
    return TRUE;
}

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