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

#include <wine/config.h>

//#include <ntstatus.h>
#define WIN32_NO_STATUS

#include <wine/debug.h>

#include <winbase.h>
#include <shobjidl.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <shellapi.h>
#include <malloc.h>
#include <winuser.h>
#include <strsafe.h>
#include <secext.h>
#include <Lmaccess.h>
#include <Lmapibuf.h>
#include <objbase.h>
#include <rpcproxy.h>
#include <security.h>
#include <shlobj_undoc.h>
//#include <shlwapi_undoc.h>
#include <unicode.h>
#include <sddl.h>
#include <rtlfuncs.h>
#include <direct.h>
#include <pidl.h>
#include <prsht.h>
#include <winreg.h>
#include <shresdef.h>
#include <objbase.h>
#include <shlguid.h>
#include <undocshell.h>
#include <stdarg.h>
#include <string.h>
#include <sys/types.h>
#include <wingdi.h>
#include <commdlg.h>
#include <wine/heap.h>
#include <shell32_classes_p.h>

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#define COBJMACROS
#define NONAMELESSUNION
#include "propsys.h"

#define IDD_ICON                0x4300
#define IDD_MESSAGE             0x4301

//DEFINITIONS
#define IDS_DESKTOP		20
#define CP_UNIXCP     65010 
#define PT_CPLAPPLET	0x00
#define PT_GUID		0x1F
#define PT_DRIVE	0x23
#define PT_DRIVE2	0x25
#define PT_DRIVE3	0x29
#define PT_SHELLEXT	0x2E
#define PT_DRIVE1	0x2F
#define PT_FOLDER1	0x30
#define PT_FOLDER	0x31
#define PT_VALUE	0x32
#define PT_VALUEW	0x34
#define PT_FOLDERW	0x35
#define PT_WORKGRP	0x41
#define PT_COMP		0x42
#define PT_YAGUID	0x70 
#define FOF_SILENT                 0x0004
#define FOF_NOCONFIRMATION         0x0010
#define FOF_NOERRORUI              0x0400
#define FOF_NOCONFIRMMKDIR         0x0200
#define FOF_NO_UI                  (FOF_SILENT | FOF_NOCONFIRMATION | FOF_NOERRORUI | FOF_NOCONFIRMMKDIR)
#define MAX_EXTENSION_LENGTH 20

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

//We don't know where it is implemented
#define IUnknown_AddRef(T) (T)->lpVtbl->AddRef(T)
#define IApplicationAssociationRegistration_QueryInterface( x, p1, p2 ) \
            (x)->lpVtbl->QueryInterface( x, p1, p2 )
#define IEnumAssocHandlers_AddRef( x ) \
            (x)->lpVtbl->AddRef( x )	
			
#define SLDF_HAS_LOGO3ID 0x00000800

#define FWF_NONE 0x00000000

/* file operation */
#define ASK_DELETE_FILE           1
#define ASK_DELETE_FOLDER         2
#define ASK_DELETE_MULTIPLE_ITEM  3
#define ASK_CREATE_FOLDER         4
#define ASK_OVERWRITE_FILE        5
#define ASK_DELETE_SELECTED       6
#define ASK_TRASH_FILE            7
#define ASK_TRASH_FOLDER          8
#define ASK_TRASH_MULTIPLE_ITEM   9
#define ASK_CANT_TRASH_ITEM      10
#define ASK_OVERWRITE_FOLDER     11

typedef BYTE PIDLTYPE;

HINSTANCE shell32_hInstance;

const char * shdebugstr_guid( const struct _GUID *id );

BOOL HCR_GetClassNameW(REFIID riid, LPWSTR szDest, DWORD len);

BOOL HCR_GetClassNameA(REFIID riid, LPSTR szDest, DWORD len);

HRESULT ItemMenu_Constructor(IShellFolder *parent, LPCITEMIDLIST pidl, const LPCITEMIDLIST *apidl, UINT cidl,
    REFIID riid, void **pObj);

BOOL PidlToSicIndex (
	IShellFolder * sh,
	LPCITEMIDLIST pidl,
	BOOL bBigIcon,
	UINT uFlags,
	int * pIndex);

BOOL HCR_MapTypeToValueA(LPCSTR szExtension, LPSTR szFileType, LONG len, BOOL bPrependDot);

HRESULT 
WINAPI 
PSFormatForDisplay(
  _In_   REFPROPERTYKEY propkey,
  _In_   REFPROPVARIANT propvar,
  _In_   PROPDESC_FORMAT_FLAGS pdfFlags,
  _Out_  LPWSTR pwszText,
  _In_   DWORD cchText
);

HRESULT 
WINAPI 
SHGetKnownFolderIDList(
	REFKNOWNFOLDERID rfid, 
	DWORD flags, 
	HANDLE token, 
	PIDLIST_ABSOLUTE *pidl
);

HRESULT WINAPI ApplicationAssociationRegistration_Constructor(IUnknown *outer, REFIID riid, LPVOID *ppv);

HRESULT WINAPI ApplicationDestinations_Constructor(IUnknown *outer, REFIID riid, LPVOID *ppv);
HRESULT WINAPI ApplicationDocumentLists_Constructor(IUnknown *outer, REFIID riid, LPVOID *ppv);
HRESULT WINAPI QueryAssociations_Constructor(IUnknown *pUnkOuter, REFIID riid, LPVOID *ppOutput);
HRESULT WINAPI IShellItem_Constructor(IUnknown * pUnkOuter, REFIID riid, LPVOID * ppv);
HRESULT WINAPI IShellLink_Constructor(IUnknown * pUnkOuter, REFIID riid, LPVOID * ppv);
HRESULT WINAPI ExplorerBrowser_Constructor(IUnknown *pUnkOuter, REFIID riid, LPVOID *ppv);
HRESULT WINAPI KnownFolderManager_Constructor(IUnknown *pUnkOuter, REFIID riid, LPVOID *ppv);
HRESULT WINAPI IFileOperation_Constructor(IUnknown *outer, REFIID riid, void **out);
HRESULT WINAPI CustomDestinationList_Constructor(IUnknown *outer, REFIID riid, void **obj);
HRESULT WINAPI IFileSystemBindData_Constructor(const WIN32_FIND_DATAW *pfd, LPBC *ppV);