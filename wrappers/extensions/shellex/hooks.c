/*++

Copyright (c) 2024  Shorthorn Project

Module Name:

    hooks.c

Abstract:

    Hook native functions 

Author:

    Skulltrail 20-September-2024

Revision History:

--*/

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

BOOL WINAPI Shell_NotifyIconWInternal(DWORD dwMessage, PNOTIFYICONDATAW lpData) {
    if (lpData->cbSize > NOTIFYICONDATAW_V2_SIZE) {
        NOTIFYICONDATAW lpXPData;
        memcpy(&lpXPData, lpData, NOTIFYICONDATAW_V2_SIZE);
        lpXPData.cbSize = NOTIFYICONDATAW_V2_SIZE;
        // Remove Vista flags.
        if (lpXPData.uFlags & 0x80) { // NIF_SHOWTIP
            lpXPData.uFlags ^= 0x80;
        }
        if (lpXPData.uFlags & 0x40) { // NIF_REALTIME
            lpXPData.uFlags ^= 0x40;
        }
        if (lpXPData.dwInfoFlags & 0x20) {
            // I hope it picks the right icon.
            lpXPData.dwInfoFlags ^= 0x20;
        }
        if (lpXPData.dwInfoFlags & 0x80) {
            lpXPData.dwInfoFlags ^= 0x80;
        }
        if (lpXPData.uVersion > 3)
            lpXPData.uVersion = 3;
        return Shell_NotifyIconWNative(dwMessage, &lpXPData);
    }
    return Shell_NotifyIconWNative(dwMessage, lpData);
}

BOOL WINAPI Shell_NotifyIconAInternal(DWORD dwMessage, PNOTIFYICONDATAA lpData) {
    // if (lpData->cbSize > NOTIFYICONDATAA_V2_SIZE) {
        // NOTIFYICONDATAA lpXPData;
        // memcpy(&lpXPData, lpData, NOTIFYICONDATAA_V2_SIZE);
        // lpXPData.cbSize = NOTIFYICONDATAW_V2_SIZE;
        // // Remove Vista flags.
        // if (lpXPData.uFlags & 0x80) { // NIF_SHOWTIP
            // lpXPData.uFlags ^= 0x80;
        // }
        // if (lpXPData.uFlags & 0x40) { // NIF_REALTIME
            // lpXPData.uFlags ^= 0x40;
        // }
        // if (lpXPData.dwInfoFlags & 0x20) {
            // // I hope it picks the right icon.
            // lpXPData.dwInfoFlags ^= 0x20;
        // }
        // if (lpXPData.dwInfoFlags & 0x80) {
            // lpXPData.dwInfoFlags ^= 0x80;
        // }
        // if (lpXPData.uVersion > 3)
            // lpXPData.uVersion = 3;
        // return Shell_NotifyIconANative(dwMessage, &lpXPData);
    // }
    return Shell_NotifyIconANative(dwMessage, lpData);
}

BOOLEAN 
CheckIfIsExplorer(){
    // Get the current Process ID
    DWORD currentProcessId = GetCurrentProcessId();

    // Abrir o processo
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, currentProcessId);

    // Buffer to save the executable patch
    WCHAR exePath[MAX_PATH];
    
    // Get the path of process 
    DWORD size = GetModuleFileNameExW(hProcess, NULL, exePath, MAX_PATH);
    if (size == 0) {
        return FALSE;
    }

    // // Obter o nome do executável do processo atual
    // if (GetModuleFileName(NULL, processName, sizeof(processName)) == 0) {
        // DbgPrint("Falha ao obter o nome do processo.\n");
    // } else {
        // DbgPrint("Processo atual: %s\n", processName);
        // DbgPrint("ID do processo atual: %lu\n", processId);
		// DbgPrint("O nome somente do executável e: %s\n", PathFindFileName(processName));
    // }

    // Compare executable name with "explorer.exe"
    if (wcsicmp(PathFindFileNameW(exePath), L"EXPLORER.EXE") == 0) {
        return TRUE;
    } else {
		return FALSE;
    }	
}

/*************************************************************************
 * DllGetClassObject     [SHELL32.@]
 * SHDllGetClassObject   [SHELL32.128]
 */
HRESULT WINAPI DllGetClassObjectInternal(REFCLSID rclsid, REFIID iid, LPVOID *ppv)
{
	IClassFactory * pcf = NULL;
	HRESULT	hres;
	int i;
	
	TRACE("CLSID:%s,IID:%s\n",shdebugstr_guid(rclsid),shdebugstr_guid(iid));

	if (!ppv) return E_INVALIDARG;
	*ppv = NULL;

	/* search our internal interface table */
	for(i=0;InterfaceTable[i].clsid;i++) {
	    if(IsEqualIID(InterfaceTable[i].clsid, rclsid)) {
	        //TRACE("index[%u]\n", i);
			if(IsEqualIID(&CLSID_ShellLink, rclsid))
			{
				if(!CheckIfIsExplorer()){
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
	    //FIXME("failed for CLSID=%s\n", shdebugstr_guid(rclsid));
	    return DllGetClassObjectNative(rclsid, iid, ppv);//return DllGetClassObjectInternal;
	}

	hres = IClassFactory_QueryInterface(pcf, iid, ppv);
	IClassFactory_Release(pcf);

	//TRACE("-- pointer to class factory: %p\n",*ppv);
	return hres;
}

/*************************************************************************
 * ShellExecuteW			[SHELL32.294]
 * from shellapi.h
 * WINSHELLAPI HINSTANCE APIENTRY ShellExecuteW(HWND hwnd, LPCWSTR lpVerb,
 * LPCWSTR lpFile, LPCWSTR lpParameters, LPCWSTR lpDirectory, INT nShowCmd);
 */
HINSTANCE WINAPI ShellExecuteWInternal(HWND hwnd, LPCWSTR lpVerb, LPCWSTR lpFile,
                               LPCWSTR lpParameters, LPCWSTR lpDirectory, INT nShowCmd)
{
	//PathCchCanonicalize(lpFile, MAX_PATH, lpFile);
	return ShellExecuteWNative(hwnd, lpVerb, lpFile, lpParameters, lpDirectory, nShowCmd);

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


/*************************************************************************
 * CommandLineToArgvW            [SHCORE.@]
 *
 * We must interpret the quotes in the command line to rebuild the argv
 * array correctly:
 * - arguments are separated by spaces or tabs
 * - quotes serve as optional argument delimiters
 *   '"a b"'   -> 'a b'
 * - escaped quotes must be converted back to '"'
 *   '\"'      -> '"'
 * - consecutive backslashes preceding a quote see their number halved with
 *   the remainder escaping the quote:
 *   2n   backslashes + quote -> n backslashes + quote as an argument delimiter
 *   2n+1 backslashes + quote -> n backslashes + literal quote
 * - backslashes that are not followed by a quote are copied literally:
 *   'a\b'     -> 'a\b'
 *   'a\\b'    -> 'a\\b'
 * - in quoted strings, consecutive quotes see their number divided by three
 *   with the remainder modulo 3 deciding whether to close the string or not.
 *   Note that the opening quote must be counted in the consecutive quotes,
 *   that's the (1+) below:
 *   (1+) 3n   quotes -> n quotes
 *   (1+) 3n+1 quotes -> n quotes plus closes the quoted string
 *   (1+) 3n+2 quotes -> n+1 quotes plus closes the quoted string
 * - in unquoted strings, the first quote opens the quoted string and the
 *   remaining consecutive quotes follow the above rule.
 */
WCHAR** WINAPI CommandLineToArgvWInternal(const WCHAR *cmdline, int *numargs)
{
    int qcount, bcount;
    const WCHAR *s;
    WCHAR **argv;
    DWORD argc;
    WCHAR *d;

    if (!numargs)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return NULL;
    }

    if (*cmdline == 0)
    {
        /* Return the path to the executable */
        DWORD len, deslen = MAX_PATH, size;

        size = sizeof(WCHAR *) * 2 + deslen * sizeof(WCHAR);
        for (;;)
        {
            if (!(argv = LocalAlloc(LMEM_FIXED, size))) return NULL;
            len = GetModuleFileNameW(0, (WCHAR *)(argv + 2), deslen);
            if (!len)
            {
                LocalFree(argv);
                return NULL;
            }
            if (len < deslen) break;
            deslen *= 2;
            size = sizeof(WCHAR *) * 2 + deslen * sizeof(WCHAR);
            LocalFree(argv);
        }
        argv[0] = (WCHAR *)(argv + 2);
        argv[1] = NULL;
        *numargs = 1;

        return argv;
    }

    /* --- First count the arguments */
    argc = 1;
    s = cmdline;
    /* The first argument, the executable path, follows special rules */
    if (*s == '"')
    {
        /* The executable path ends at the next quote, no matter what */
        s++;
        while (*s)
            if (*s++ == '"')
                break;
    }
    else
    {
        /* The executable path ends at the next space, no matter what */
        while (*s && *s != ' ' && *s != '\t')
            s++;
    }
    /* skip to the first argument, if any */
    while (*s == ' ' || *s == '\t')
        s++;
    if (*s)
        argc++;

    /* Analyze the remaining arguments */
    qcount = bcount = 0;
    while (*s)
    {
        if ((*s == ' ' || *s == '\t') && qcount == 0)
        {
            /* skip to the next argument and count it if any */
            while (*s == ' ' || *s == '\t')
                s++;
            if (*s)
                argc++;
            bcount = 0;
        }
        else if (*s == '\\')
        {
            /* '\', count them */
            bcount++;
            s++;
        }
        else if (*s == '"')
        {
            /* '"' */
            if ((bcount & 1) == 0)
                qcount++; /* unescaped '"' */
            s++;
            bcount = 0;
            /* consecutive quotes, see comment in copying code below */
            while (*s == '"')
            {
                qcount++;
                s++;
            }
            qcount = qcount % 3;
            if (qcount == 2)
                qcount = 0;
        }
        else
        {
            /* a regular character */
            bcount = 0;
            s++;
        }
    }

    /* Allocate in a single lump, the string array, and the strings that go
     * with it. This way the caller can make a single LocalFree() call to free
     * both, as per MSDN.
     */
    argv = LocalAlloc(LMEM_FIXED, (argc + 1) * sizeof(WCHAR *) + (lstrlenW(cmdline) + 1) * sizeof(WCHAR));
    if (!argv)
        return NULL;

    /* --- Then split and copy the arguments */
    argv[0] = d = lstrcpyW((WCHAR *)(argv + argc + 1), cmdline);
    argc = 1;
    /* The first argument, the executable path, follows special rules */
    if (*d == '"')
    {
        /* The executable path ends at the next quote, no matter what */
        s = d + 1;
        while (*s)
        {
            if (*s == '"')
            {
                s++;
                break;
            }
            *d++ = *s++;
        }
    }
    else
    {
        /* The executable path ends at the next space, no matter what */
        while (*d && *d != ' ' && *d != '\t')
            d++;
        s = d;
        if (*s)
            s++;
    }
    /* close the executable path */
    *d++ = 0;
    /* skip to the first argument and initialize it if any */
    while (*s == ' ' || *s == '\t')
        s++;
    if (!*s)
    {
        /* There are no parameters so we are all done */
        argv[argc] = NULL;
        *numargs = argc;
        return argv;
    }

    /* Split and copy the remaining arguments */
    argv[argc++] = d;
    qcount = bcount = 0;
    while (*s)
    {
        if ((*s == ' ' || *s == '\t') && qcount == 0)
        {
            /* close the argument */
            *d++ = 0;
            bcount = 0;

            /* skip to the next one and initialize it if any */
            do {
                s++;
            } while (*s == ' ' || *s == '\t');
            if (*s)
                argv[argc++] = d;
        }
        else if (*s=='\\')
        {
            *d++ = *s++;
            bcount++;
        }
        else if (*s == '"')
        {
            if ((bcount & 1) == 0)
            {
                /* Preceded by an even number of '\', this is half that
                 * number of '\', plus a quote which we erase.
                 */
                d -= bcount / 2;
                qcount++;
            }
            else
            {
                /* Preceded by an odd number of '\', this is half that
                 * number of '\' followed by a '"'
                 */
                d = d - bcount / 2 - 1;
                *d++ = '"';
            }
            s++;
            bcount = 0;
            /* Now count the number of consecutive quotes. Note that qcount
             * already takes into account the opening quote if any, as well as
             * the quote that lead us here.
             */
            while (*s == '"')
            {
                if (++qcount == 3)
                {
                    *d++ = '"';
                    qcount = 0;
                }
                s++;
            }
            if (qcount == 2)
                qcount = 0;
        }
        else
        {
            /* a regular character */
            *d++ = *s++;
            bcount = 0;
        }
    }
    *d = '\0';
    argv[argc] = NULL;
    *numargs = argc;

    return argv;
}