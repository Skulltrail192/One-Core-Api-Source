/*
 * Copyright 2009 Henri Verbeet for CodeWeavers
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 *
 */

#include <main.h>
 
WINE_DEFAULT_DEBUG_CHANNEL(shell);

FORCEINLINE HRESULT IPropertyStore_QueryInterface(IPropertyStore* This,REFIID riid,void **ppvObject) {
    return This->lpVtbl->QueryInterface(This,riid,ppvObject);
}
FORCEINLINE ULONG IPropertyStore_AddRef(IPropertyStore* This) {
    return This->lpVtbl->AddRef(This);
}
FORCEINLINE ULONG IPropertyStore_Release(IPropertyStore* This) {
    return This->lpVtbl->Release(This);
}

typedef struct _NOTIFYICONIDENTIFIER {
  DWORD cbSize;
  HWND  hWnd;
  UINT  uID;
  GUID  guidItem;
} NOTIFYICONIDENTIFIER, *PNOTIFYICONIDENTIFIER;

typedef enum _ASSOCCLASS{
	ASSOCCLASS_APP_KEY,
	ASSOCCLASS_CLSID_KEY,
	ASSOCCLASS_CLSID_STR,
	ASSOCCLASS_PROGID_KEY,
	ASSOCCLASS_SHELL_KEY,
	ASSOCCLASS_PROGID_STR,
	ASSOCCLASS_SYSTEM_STR,
	ASSOCCLASS_APP_STR,
	ASSOCCLASS_FOLDER,
	ASSOCCLASS_STAR,
	ASSOCCLASS_FIXED_PROGID_STR,
	ASSOCCLASS_PROTOCOL_STR,
}ASSOCCLASS;

typedef enum tagSCNRT_STATUS { 
  SCNRT_ENABLE   = 0,
  SCNRT_DISABLE  = 1
} SCNRT_STATUS;

typedef struct _ASSOCIATIONELEMENT {
  ASSOCCLASS ac;
  HKEY       hkClass;
  PCWSTR     pszClass;
} ASSOCIATIONELEMENT;

struct window_prop_store
{
    IPropertyStore IPropertyStore_iface;
    LONG           ref;
};

static inline struct window_prop_store *impl_from_IPropertyStore(IPropertyStore *iface)
{
    return CONTAINING_RECORD(iface, struct window_prop_store, IPropertyStore_iface);
}

static ULONG WINAPI window_prop_store_AddRef(IPropertyStore *iface)
{
    struct window_prop_store *store = impl_from_IPropertyStore(iface);
    LONG ref = InterlockedIncrement(&store->ref);
    TRACE("returning %ld\n", ref);
    return ref;
}

static ULONG WINAPI window_prop_store_Release(IPropertyStore *iface)
{
    struct window_prop_store *store = impl_from_IPropertyStore(iface);
    LONG ref = InterlockedDecrement(&store->ref);
    if (!ref) heap_free(store);
    TRACE("returning %ld\n", ref);
    return ref;
}

static HRESULT WINAPI window_prop_store_QueryInterface(IPropertyStore *iface, REFIID iid, void **obj)
{
    struct window_prop_store *store = impl_from_IPropertyStore(iface);
    TRACE("%p, %s, %p\n", iface, debugstr_guid(iid), obj);

    if (!obj) return E_POINTER;
    if (IsEqualIID(iid, &IID_IUnknown) || IsEqualIID(iid, &IID_IPropertyStore))
    {
        *obj = &store->IPropertyStore_iface;
    }
    else
    {
        FIXME("no interface for %s\n", debugstr_guid(iid));
        *obj = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown *)*obj);
    return S_OK;
}

static HRESULT WINAPI window_prop_store_GetCount(IPropertyStore *iface, DWORD *count)
{
    FIXME("%p, %p\n", iface, count);
    return E_NOTIMPL;
}

static HRESULT WINAPI window_prop_store_GetAt(IPropertyStore *iface, DWORD prop, PROPERTYKEY *key)
{
    FIXME("%p, %lu,%p\n", iface, prop, key);
    return E_NOTIMPL;
}

static HRESULT WINAPI window_prop_store_GetValue(IPropertyStore *iface, const PROPERTYKEY *key, PROPVARIANT *var)
{
    FIXME("%p, {%s,%lu}, %p\n", iface, debugstr_guid(&key->fmtid), key->pid, var);
    return E_NOTIMPL;
}

static HRESULT WINAPI window_prop_store_SetValue(IPropertyStore *iface, const PROPERTYKEY *key, const PROPVARIANT *var)
{
    FIXME("%p, {%s,%lu}, %p\n", iface, debugstr_guid(&key->fmtid), key->pid, var);
    return S_OK;
}

static HRESULT WINAPI window_prop_store_Commit(IPropertyStore *iface)
{
    FIXME("%p\n", iface);
    return S_OK;
}

static const IPropertyStoreVtbl window_prop_store_vtbl =
{
    window_prop_store_QueryInterface,
    window_prop_store_AddRef,
    window_prop_store_Release,
    window_prop_store_GetCount,
    window_prop_store_GetAt,
    window_prop_store_GetValue,
    window_prop_store_SetValue,
    window_prop_store_Commit
};

static HRESULT create_window_prop_store(IPropertyStore **obj)
{
    struct window_prop_store *store;

    if (!(store = heap_alloc(sizeof(*store)))) return E_OUTOFMEMORY;
    store->IPropertyStore_iface.lpVtbl = &window_prop_store_vtbl;
    store->ref = 1;

    *obj = &store->IPropertyStore_iface;
    return S_OK;
}

HRESULT 
WINAPI 
SHGetPropertyStoreForWindow(
  _In_   HWND hwnd,
  _In_   REFIID riid,
  _Out_  void **ppv
)
{
    IPropertyStore *store;
    HRESULT hr;

    FIXME("(%p %p %p) stub!\n", hwnd, riid, ppv);

    if ((hr = create_window_prop_store( &store )) != S_OK) return hr;
    hr = IPropertyStore_QueryInterface( store, riid, ppv );
    IPropertyStore_Release( store );
    return hr;
}

static const GUID CLSID_Bind_Query = 
{0x000214e6, 0x0000, 0x0000, {0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46}};

HRESULT WINAPI SHBindToFolderIDListParent(
  _In_opt_   IShellFolder *psfRoot,
  _In_       PCUIDLIST_RELATIVE pidl,
  _In_       REFIID riid,
  _Out_      void **ppv,
  _Out_opt_  PCUITEMID_CHILD *ppidlLast
)
{
	 ppv = NULL;
	 return E_FAIL;
}

EXTERN_C HANDLE WINAPI IsEnabled()
{
	return NULL;
}

/***********************************************************************
 *              SHCreateDataObject (SHELL32.@)
 */
HRESULT WINAPI SHCreateDataObject(PCIDLIST_ABSOLUTE pidl_folder, UINT count, PCUITEMID_CHILD_ARRAY pidl_array,
                                  IDataObject *object, REFIID riid, void **ppv)
{
    FIXME("%p %d %p %p %s %p: stub\n", pidl_folder, count, pidl_array, object, debugstr_guid(riid), ppv);
    return E_NOTIMPL;
}

//@unimplemented
HRESULT WINAPI SHEvaluateSystemCommandTemplate(
  _In_       PCWSTR pszCmdTemplate,
  _Out_      PWSTR *ppszApplication,
  _Out_opt_  PWSTR *ppszCommandLine,
  _Out_opt_  PWSTR *ppszParameters
)
{
    return E_NOTIMPL;
}

HRESULT WINAPI SHCreateDefaultExtractIcon(
  REFIID riid,
  _Out_  void **ppv
)
{
    return E_NOTIMPL;
}

HRESULT 
WINAPI 
SHSetTemporaryPropertyForItem(
  _In_  IShellItem *psi,
  _In_  REFPROPERTYKEY propkey,
  _In_  REFPROPVARIANT propvar
)
{
	return E_FAIL;
}

/***********************************************************************
 *              SHGetLocalizedName (SHELL32.@)
 */
HRESULT WINAPI SHGetLocalizedName(LPCWSTR path, LPWSTR module, UINT size, INT *res)
{
    FIXME("%s %p %u %p: stub\n", debugstr_w(path), module, size, res);
    return E_NOTIMPL;
}

HRESULT 
WINAPI 
ILLoadFromStreamEx(
  _In_   IStream *pstm,
  _Out_  PITEMID_CHILD *ppidl
)
{
	return E_FAIL;
}

/***********************************************************************
 *              InitNetworkAddressControl (SHELL32.@)
 */
BOOL WINAPI InitNetworkAddressControl(void)
{
    FIXME("stub\n");
    return FALSE;
}

HRESULT WINAPI SHGetTemporaryPropertyForItem(
  _In_   IShellItem *psi,
  REFPROPERTYKEY pk,
  _Out_  PROPVARIANT *ppropvarInk
)
{
	return E_FAIL;
}

HRESULT WINAPI SHGetPropertyStoreFromIDList(
  _In_   PCIDLIST_ABSOLUTE pidl,
  _In_   GETPROPERTYSTOREFLAGS flags,
  _In_   REFIID riid,
  _Out_  void **ppv
)
{
	return E_FAIL;
}

HRESULT WINAPI AssocCreateForClasses(
  _In_   const ASSOCIATIONELEMENT *rgClasses,
  _In_   ULONG cClasses,
  _In_   REFIID riid,
  _Out_  void **ppv
)
{
	return E_FAIL;
}

HRESULT WINAPI AssocGetDetailsOfPropKey(
  _In_   IShellFolder *psf,
  _In_   PCUITEMID_CHILD pidl,
  _In_   PROPERTYKEY *pkey,
  _Out_  VARIANT *pv,
  _Out_  BOOL *pfFoundPropKey
)
{
	return E_FAIL;
}

HRESULT WINAPI SHAddDefaultPropertiesByExt(
  _In_  PCWSTR pszExt,
  _In_  IPropertyStore *pPropStore
)
{
	return E_FAIL;
}

/*************************************************************************
 *              SHRemoveLocalizedName (SHELL32.@)
 */
HRESULT WINAPI SHRemoveLocalizedName(const WCHAR *path)
{
    FIXME("%s stub\n", debugstr_w(path));
    return S_OK;
}

HRESULT 
WINAPI 
SHGetDriveMedia(
  _In_   PCWSTR pszDrive,
  _Out_  DWORD *pdwMediaContent
)
{
	return E_FAIL;
}

void WINAPI SHChangeNotifyRegisterThread(
  SCNRT_STATUS status
)
{
	;
}

HRESULT WINAPI SHCreateThreadUndoManager(int a1, int a2)
{
	return E_FAIL;	
}

HRESULT WINAPI SHGetThreadUndoManager(int a1, int a2)
{
	return E_FAIL;	
}

HRESULT 
WPC_InstallState(
	DWORD *pdwState
){
	*pdwState = 2;
	return S_OK;
}

/*************************************************************************
 * Shell_NotifyIconGetRect		[SHELL32.@]
 */
HRESULT WINAPI Shell_NotifyIconGetRect(const NOTIFYICONIDENTIFIER* identifier, RECT* icon_location)
{
    FIXME("stub (%p) (%p)\n", identifier, icon_location);
    return E_NOTIMPL;
}

HRESULT 
WINAPI 
StgMakeUniqueName(
  _In_  IStorage *pstgParent,
  _In_  PCWSTR   pszFileSpec,
  _In_  DWORD    grfMode,
  _In_  REFIID   riid,
  _Out_ void     **ppv
)
{
	return S_OK;
}

BOOL 
IsElevationRequired(LPCWSTR pszPath)
{
	return FALSE;
}

HRESULT WINAPI SHSetDefaultProperties(
  _In_opt_ HWND                       hwnd,
  _In_     IShellItem                 *psi,
           DWORD                      dwFileOpFlags,
  _In_opt_ IFileOperationProgressSink **pfops
)
{
	return S_OK;
}

HRESULT SHUserSetPasswordHint(LPCWSTR lpStart, LPCWSTR lpString)
{
	return S_OK;	
}

HRESULT
WINAPI
StampIconForElevation(HICON icon, int x, int y){
	return S_OK;
}

HRESULT WINAPI SetCurrentProcessExplicitAppUserModelID(const WCHAR *appid)
{
    FIXME("%s: stub\n", debugstr_w(appid));
    return S_OK;
}

HRESULT WINAPI GetCurrentProcessExplicitAppUserModelID(const WCHAR **appid)
{
    FIXME("%p: stub\n", appid);
    *appid = NULL;
    return E_NOTIMPL;
}

//For Longhorn/Vista comdlg32
void SHChangeNotifyDeregisterWindow(HWND hwnd)
{
	;
}

int SHMapIDListToSystemImageListIndexAsync(int a1, struct IShellFolder *a2, LPCITEMIDLIST pidl2, PVOID a4, void *a5, void *a6, int a7, struct IRunnableTask *a8)
{
	return 0;
}