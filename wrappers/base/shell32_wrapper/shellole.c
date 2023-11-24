/*
 * see www.geocities.com/SiliconValley/4942/filemenu.html
 *
 * Copyright 1999, 2000 Juergen Schmied
 * Copyright 2011 Jay Yang
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
 */

#include <stdarg.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "objbase.h"
#include "rpcproxy.h"
#include "commdlg.h"
#include "cderr.h"
#include "wine/debug.h"
#include "wine/heap.h"

#define NDEBUG
#include <debug.h>
#include <main.h>

WINE_DEFAULT_DEBUG_CHANNEL(shell);

/**************************************************************************
 * Default ClassFactory types
 */
typedef HRESULT (CALLBACK *LPFNCREATEINSTANCE)(IUnknown* pUnkOuter, REFIID riid, LPVOID* ppvObject);
static IClassFactory * IDefClF_fnConstructor(LPFNCREATEINSTANCE lpfnCI, PLONG pcRefDll, REFIID riidInst);
/* this table contains all CLSIDs of shell32 objects */
static const struct {
	REFIID			clsid;
	LPFNCREATEINSTANCE	lpfnCI;
} InterfaceTable[] = {

	{&CLSID_ApplicationAssociationRegistration, ApplicationAssociationRegistration_Constructor},
	{&CLSID_ApplicationDestinations, ApplicationDestinations_Constructor},
	{&CLSID_ApplicationDocumentLists, ApplicationDocumentLists_Constructor},
	// {&CLSID_AutoComplete,   IAutoComplete_Constructor},
	// {&CLSID_ControlPanel,	IControlPanel_Constructor},
	// {&CLSID_DragDropHelper, IDropTargetHelper_Constructor},
	// {&CLSID_FolderShortcut, FolderShortcut_Constructor},
	// {&CLSID_MyComputer,	ISF_MyComputer_Constructor},
	// {&CLSID_MyDocuments,    MyDocuments_Constructor},
	// {&CLSID_NetworkPlaces,  ISF_NetworkPlaces_Constructor},
	// {&CLSID_Printers,       Printers_Constructor},
	{&CLSID_QueryAssociations, QueryAssociations_Constructor},
	// {&CLSID_RecycleBin,     RecycleBin_Constructor},
	// {&CLSID_ShellDesktop,	ISF_Desktop_Constructor},
	// {&CLSID_ShellFSFolder,	IFSFolder_Constructor},
	{&CLSID_ShellItem,	IShellItem_Constructor},
	{&CLSID_ShellLink,	IShellLink_Constructor},
	{&CLSID_ExplorerBrowser,ExplorerBrowser_Constructor},
	{&CLSID_KnownFolderManager, KnownFolderManager_Constructor},
	// {&CLSID_Shell,          IShellDispatch_Constructor},
	{&CLSID_DestinationList, CustomDestinationList_Constructor},
	// {&CLSID_ShellImageDataFactory, ShellImageDataFactory_Constructor},
	{&CLSID_FileOperation, IFileOperation_Constructor},
	{NULL, NULL}
};

/*************************************************************************
 * DllGetClassObject     [SHELL32.@]
 * SHDllGetClassObject   [SHELL32.128]
 */
HRESULT WINAPI DllGetClassObjectInternal(REFCLSID rclsid, REFIID iid, LPVOID *ppv)
{
	IClassFactory * pcf = NULL;
	HRESULT	hres;
	int i;
    OSVERSIONINFO osvi;
    BOOL bIsWindowsVistaorLater;

    ZeroMemory(&osvi, sizeof(OSVERSIONINFO));
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

    GetVersionEx(&osvi);

    bIsWindowsVistaorLater = 
       ( (osvi.dwMajorVersion > 6) ||
       ( (osvi.dwMajorVersion == 6) && (osvi.dwMinorVersion >= 0) ));	

	TRACE("CLSID:%s,IID:%s\n",shdebugstr_guid(rclsid),shdebugstr_guid(iid));

	if (!ppv) return E_INVALIDARG;
	*ppv = NULL;

	/* search our internal interface table */
	for(i=0;InterfaceTable[i].clsid;i++) {
	    if(IsEqualIID(InterfaceTable[i].clsid, rclsid)) {
	        TRACE("index[%u]\n", i);
			if(IsEqualIID(&CLSID_ShellLink, rclsid))
			{
				if(bIsWindowsVistaorLater){
					pcf = IDefClF_fnConstructor(InterfaceTable[i].lpfnCI, NULL, NULL);
					break;
				}else{
					continue;
				}				
			}else{
				pcf = IDefClF_fnConstructor(InterfaceTable[i].lpfnCI, NULL, NULL);
				break;				
			}
	    }			
	}		

    if (!pcf) {
	    FIXME("failed for CLSID=%s\n", shdebugstr_guid(rclsid));
	    return DllGetClassObject(rclsid, iid, ppv);//return DllGetClassObjectInternal;
	}

	hres = IClassFactory_QueryInterface(pcf, iid, ppv);
	IClassFactory_Release(pcf);

	TRACE("-- pointer to class factory: %p\n",*ppv);
	return hres;
}

/**************************************************************************
 * Default ClassFactory Implementation
 *
 * SHCreateDefClassObject
 *
 * NOTES
 *  Helper function for dlls without their own classfactory.
 *  A generic classfactory is returned.
 *  When the CreateInstance of the cf is called the callback is executed.
 */

typedef struct
{
    IClassFactory               IClassFactory_iface;
    LONG                        ref;
    CLSID			*rclsid;
    LPFNCREATEINSTANCE		lpfnCI;
    const IID *			riidInst;
    LONG *			pcRefDll; /* pointer to refcounter in external dll (ugrrr...) */
} IDefClFImpl;

static inline IDefClFImpl *impl_from_IClassFactory(IClassFactory *iface)
{
	return CONTAINING_RECORD(iface, IDefClFImpl, IClassFactory_iface);
}

static const IClassFactoryVtbl dclfvt;

/**************************************************************************
 *  IDefClF_fnConstructor
 */

static IClassFactory * IDefClF_fnConstructor(LPFNCREATEINSTANCE lpfnCI, PLONG pcRefDll, REFIID riidInst)
{
	IDefClFImpl* lpclf;

	lpclf = malloc(sizeof(*lpclf));
	lpclf->ref = 1;
	lpclf->IClassFactory_iface.lpVtbl = &dclfvt;
	lpclf->lpfnCI = lpfnCI;
	lpclf->pcRefDll = pcRefDll;

	if (pcRefDll) InterlockedIncrement(pcRefDll);
	lpclf->riidInst = riidInst;

	TRACE("(%p)%s\n",lpclf, shdebugstr_guid(riidInst));
	return &lpclf->IClassFactory_iface;
}
/**************************************************************************
 *  IDefClF_fnQueryInterface
 */
static HRESULT WINAPI IDefClF_fnQueryInterface(
  LPCLASSFACTORY iface, REFIID riid, LPVOID *ppvObj)
{
	IDefClFImpl *This = impl_from_IClassFactory(iface);

	TRACE("(%p)->(%s)\n",This,shdebugstr_guid(riid));

	*ppvObj = NULL;

	if(IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IClassFactory)) {
	  *ppvObj = This;
	  InterlockedIncrement(&This->ref);
	  return S_OK;
	}

	TRACE("-- E_NOINTERFACE\n");
	return E_NOINTERFACE;
}
/******************************************************************************
 * IDefClF_fnAddRef
 */
static ULONG WINAPI IDefClF_fnAddRef(LPCLASSFACTORY iface)
{
	IDefClFImpl *This = impl_from_IClassFactory(iface);
	ULONG refCount = InterlockedIncrement(&This->ref);

	TRACE("(%p)->(count=%lu)\n", This, refCount - 1);

	return refCount;
}
/******************************************************************************
 * IDefClF_fnRelease
 */
static ULONG WINAPI IDefClF_fnRelease(LPCLASSFACTORY iface)
{
	IDefClFImpl *This = impl_from_IClassFactory(iface);
	ULONG refCount = InterlockedDecrement(&This->ref);

	TRACE("(%p)->(count=%lu)\n", This, refCount + 1);

	if (!refCount)
	{
	  if (This->pcRefDll) InterlockedDecrement(This->pcRefDll);

	  TRACE("-- destroying IClassFactory(%p)\n",This);
	  free(This);
	}

	return refCount;
}
/******************************************************************************
 * IDefClF_fnCreateInstance
 */
static HRESULT WINAPI IDefClF_fnCreateInstance(
  LPCLASSFACTORY iface, LPUNKNOWN pUnkOuter, REFIID riid, LPVOID *ppvObject)
{
	IDefClFImpl *This = impl_from_IClassFactory(iface);

	TRACE("%p->(%p,%s,%p)\n",This,pUnkOuter,shdebugstr_guid(riid),ppvObject);

	*ppvObject = NULL;

	if ( This->riidInst==NULL ||
	     IsEqualCLSID(riid, This->riidInst) ||
	     IsEqualCLSID(riid, &IID_IUnknown) )
	{
	  return This->lpfnCI(pUnkOuter, riid, ppvObject);
	}

	ERR("unknown IID requested %s\n",shdebugstr_guid(riid));
	return E_NOINTERFACE;
}
/******************************************************************************
 * IDefClF_fnLockServer
 */
static HRESULT WINAPI IDefClF_fnLockServer(LPCLASSFACTORY iface, BOOL fLock)
{
	IDefClFImpl *This = impl_from_IClassFactory(iface);
	TRACE("%p->(0x%x), not implemented\n",This, fLock);
	return E_NOTIMPL;
}

static const IClassFactoryVtbl dclfvt =
{
  IDefClF_fnQueryInterface,
  IDefClF_fnAddRef,
  IDefClF_fnRelease,
  IDefClF_fnCreateInstance,
  IDefClF_fnLockServer
};
