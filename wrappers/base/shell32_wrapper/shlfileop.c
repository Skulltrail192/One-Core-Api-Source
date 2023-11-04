/*
 * SHFileOperation
 *
 * Copyright 2000 Juergen Schmied
 * Copyright 2002 Andriy Palamarchuk
 * Copyright 2004 Dietrich Teickner (from Odin)
 * Copyright 2004 Rolf Kalbermatter
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

#define COBJMACROS

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "shellapi.h"
#include "wingdi.h"
#include "winuser.h"
#include "shlobj.h"
#include "shresdef.h"
#define NO_SHLWAPI_STREAM
#include "shlwapi.h"
#include "shell32_main.h"
#include "main.h"
#include "shfldr.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(shell);

#define IsAttrib(x, y)  ((INVALID_FILE_ATTRIBUTES != (x)) && ((x) & (y)))
#define IsAttribFile(x) (!((x) & FILE_ATTRIBUTE_DIRECTORY))
#define IsAttribDir(x)  IsAttrib(x, FILE_ATTRIBUTE_DIRECTORY)
#define IsDotDir(x)     ((x[0] == '.') && ((x[1] == 0) || ((x[1] == '.') && (x[2] == 0))))

#define FO_MASK         0xF

#define DE_SAMEFILE      0x71
#define DE_DESTSAMETREE  0x7D

#define FO_NEW_ITEM 5

struct file_operation
{
    IFileOperation IFileOperation_iface;
    LONG ref;
	SHFILEOPSTRUCTW fileOperation;
	DWORD operationFlags;
	DWORD fileAttributes;
	BOOL aborted;
	DWORD flags;
	DWORD cookie;
	IFileOperationProgressSink *sink;
};

static inline struct file_operation *impl_from_IFileOperation(IFileOperation *iface)
{
    return CONTAINING_RECORD(iface, struct file_operation, IFileOperation_iface);
}

static HRESULT WINAPI file_operation_QueryInterface(IFileOperation *iface, REFIID riid, void **out)
{
    struct file_operation *operation = impl_from_IFileOperation(iface);

    DbgPrint("(%p, %s, %p).\n", iface, debugstr_guid(riid), out);

    if (IsEqualIID(&IID_IFileOperation, riid) ||
        IsEqualIID(&IID_IUnknown, riid))
        *out = &operation->IFileOperation_iface;
    else
    {
        FIXME("not implemented for %s.\n", debugstr_guid(riid));
        *out = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown *)*out);
    return S_OK;
}

static ULONG WINAPI file_operation_AddRef(IFileOperation *iface)
{
    struct file_operation *operation = impl_from_IFileOperation(iface);
    ULONG ref = InterlockedIncrement(&operation->ref);

    DbgPrint("(%p): ref=%lu.\n", iface, ref);

    return ref;
}

static ULONG WINAPI file_operation_Release(IFileOperation *iface)
{
    struct file_operation *operation = impl_from_IFileOperation(iface);
    ULONG ref = InterlockedDecrement(&operation->ref);

    DbgPrint("(%p): ref=%lu.\n", iface, ref);

    if (!ref)
    {
        free(operation);
    }

    return ref;
}

static HRESULT WINAPI file_operation_Advise(IFileOperation *iface, IFileOperationProgressSink *sink, DWORD *cookie)
{
	struct file_operation *operation = impl_from_IFileOperation(iface);
    FIXME("(%p, %p, %p): stub.\n", iface, sink, cookie);
	
	*cookie = operation->cookie;

    return S_OK;
}

static HRESULT WINAPI file_operation_Unadvise(IFileOperation *iface, DWORD cookie)
{
	struct file_operation *operation = impl_from_IFileOperation(iface);
    FIXME("(%p, %lx): stub.\n", iface, cookie);
	
	operation->cookie = cookie;

    return S_OK;
}

static HRESULT WINAPI file_operation_SetOperationFlags(IFileOperation *iface, DWORD flags)
{
	struct file_operation *operation = impl_from_IFileOperation(iface);
    //FIXME("(%p, %lx): stub.\n", iface, flags);
	DbgPrint("IFileOperation::SetOperationFlags called\n");
	
	operation->flags = flags;

    return S_OK;
}

static HRESULT WINAPI file_operation_SetProgressMessage(IFileOperation *iface, LPCWSTR message)
{
	// struct file_operation *operation = impl_from_IFileOperation(iface);
    FIXME("(%p, %s): stub.\n", iface, debugstr_w(message));

    return S_OK;
}

static HRESULT WINAPI file_operation_SetProgressDialog(IFileOperation *iface, IOperationsProgressDialog *dialog)
{
	// struct file_operation *operation = impl_from_IFileOperation(iface);
    FIXME("(%p, %p): stub.\n", iface, dialog);

    return S_OK;
}

static HRESULT WINAPI file_operation_SetProperties(IFileOperation *iface, IPropertyChangeArray *array)
{
	// struct file_operation *operation = impl_from_IFileOperation(iface);
    FIXME("(%p, %p): stub.\n", iface, array);

    return S_OK;
}

static HRESULT WINAPI file_operation_SetOwnerWindow(IFileOperation *iface, HWND owner)
{
	struct file_operation *operation = impl_from_IFileOperation(iface);
	
	DbgPrint("IFileOperation::SetOwnerWindow called\n");
	
	operation->fileOperation.hwnd = owner;
    FIXME("(%p, %p): stub.\n", iface, owner);

    return S_OK;
}

static HRESULT WINAPI file_operation_ApplyPropertiesToItem(IFileOperation *iface, IShellItem *item)
{
	DbgPrint("IFileOperation::ApplyPropertiesToItem called\n");
	// struct file_operation *operation = impl_from_IFileOperation(iface);
    FIXME("(%p, %p): stub.\n", iface, item);

    return S_OK;
}

static HRESULT WINAPI file_operation_ApplyPropertiesToItems(IFileOperation *iface, IUnknown *items)
{
	DbgPrint("IFileOperation::ApplyPropertiesToItems called\n");
	// struct file_operation *operation = impl_from_IFileOperation(iface);
    FIXME("(%p, %p): stub.\n", iface, items);

    return S_OK;
}

int 
configureAndDoFileOperation(
	SHFILEOPSTRUCTW fileOperation,
	IShellItem *item, 
	IShellItem *folder, 
	LPCWSTR name, 
	UINT operation,
	DWORD flags
){
    LPWSTR srcBuffer;
    LPWSTR dstBuffer;
	WCHAR wszFrom[MAX_PATH] = { 0 };
	WCHAR wszTo[MAX_PATH] = { 0 };
    
	//Get Source path and destination Path(without filename)
    IShellItem2_GetDisplayName(item, SIGDN_FILESYSPATH, &srcBuffer);
	//Copy source and destination buffers to posterior copy memory
	StrCpyW(wszFrom, srcBuffer);	
	//Concat \0\0 to end of strings by CopyMemory
	CopyMemory(wszFrom + lstrlenW(wszFrom), "\0\0", 2);		
	if(folder && name){
		IShellItem2_GetDisplayName(folder, SIGDN_FILESYSPATH, &dstBuffer);
		//Concat '\\' to path destination and filename too
		wcscat(dstBuffer, L"\\");	
		wcscat(dstBuffer, name);
		//Copy current source and destination buffers to posterior copy memory
		StrCpyW(wszTo, dstBuffer);
		CopyMemory(wszTo + lstrlenW(wszTo), "\0\0", 2);	
	}	
	//Fill fileOperation structure
    fileOperation.wFunc = operation;
    fileOperation.fFlags = flags;
    fileOperation.pFrom = wszFrom;
    fileOperation.pTo =  wszTo;
	
	return SHFileOperationW(&fileOperation);
}

static HRESULT WINAPI file_operation_RenameItem(IFileOperation *iface, IShellItem *item, LPCWSTR name,
        IFileOperationProgressSink *sink)
{
    struct file_operation *operation = impl_from_IFileOperation(iface);
	DWORD flags = FOF_MULTIDESTFILES          |
                  FOF_NOCONFIRMATION          |
                  FOF_NOCONFIRMMKDIR          |
                  FOF_NOCOPYSECURITYATTRIBS   |
                  FOF_NOERRORUI               |
                  FOF_RENAMEONCOLLISION;
	int resp;

    operation->sink = sink;
	resp = configureAndDoFileOperation(operation->fileOperation, item, NULL, name, FO_RENAME, flags);
	IFileOperationProgressSink_FinishOperations(sink, S_OK);
	DbgPrint("IFileOperation::MoveItem:SHFileOperationW: (%d)\n", resp);
	operation->aborted = FALSE;		

    FIXME("(%p, %p, %s, %p): stub.\n", iface, item, debugstr_w(name), sink);
	
	return S_OK;
}

static HRESULT WINAPI file_operation_RenameItems(IFileOperation *iface, IUnknown *items, LPCWSTR name)
{
	DbgPrint("IFileOperation::RenameItems called\n");
	// struct file_operation *operation = impl_from_IFileOperation(iface);
	// operation->fileOperation.wFunc = FO_RENAME;
	// operation->fileOperation.fFlags = FOF_MULTIDESTFILES;
	
    // FIXME("(%p, %p, %s): stub.\n", iface, items, debugstr_w(name));

    return S_OK;
}

static HRESULT WINAPI file_operation_MoveItem(IFileOperation *iface, IShellItem *item, IShellItem *folder,
        LPCWSTR name, IFileOperationProgressSink *sink)
{
    struct file_operation *operation = impl_from_IFileOperation(iface);
	DWORD flags = FOF_MULTIDESTFILES          |
                  FOF_NOCONFIRMATION          |
                  FOF_NOCONFIRMMKDIR          |
                  FOF_NOCOPYSECURITYATTRIBS   |
                  FOF_NOERRORUI               |
                  FOF_RENAMEONCOLLISION;
	int resp;

    operation->sink = sink;
	resp = configureAndDoFileOperation(operation->fileOperation, item, folder, name, FO_MOVE, flags);
	IFileOperationProgressSink_FinishOperations(sink, S_OK);
	DbgPrint("IFileOperation::MoveItem:SHFileOperationW: (%d)\n", resp);
	operation->aborted = FALSE;		

    FIXME("(%p, %p, %p, %s, %p): stub.\n", iface, item, folder, debugstr_w(name), sink);
    
    return S_OK;
}

static HRESULT WINAPI file_operation_MoveItems(IFileOperation *iface, IUnknown *items, IShellItem *folder)
{
	DbgPrint("IFileOperation::MoveItems called\n");
	// struct file_operation *operation = impl_from_IFileOperation(iface);
	// operation->fileOperation.wFunc = FO_MOVE;
	// operation->fileOperation.fFlags = FOF_MULTIDESTFILES;
	// operation->operation = FO_MOVE;
	
    // FIXME("(%p, %p, %p): stub.\n", iface, items, folder);

    return S_OK;
}

static HRESULT WINAPI file_operation_CopyItem(IFileOperation *iface, IShellItem *item, IShellItem *folder,
        LPCWSTR name, IFileOperationProgressSink *sink)
{	
    struct file_operation *operation = impl_from_IFileOperation(iface);
	DWORD flags = FOF_MULTIDESTFILES          |
                  FOF_NOCONFIRMATION          |
                  FOF_NOCONFIRMMKDIR          |
                  FOF_NOCOPYSECURITYATTRIBS   |
                  FOF_NOERRORUI               |
                  FOF_RENAMEONCOLLISION;
	int resp;
	
	DbgPrint("IFileOperation::CopyItem called\n");

    operation->sink = sink;
	resp = configureAndDoFileOperation(operation->fileOperation, item, folder, name, FO_COPY, flags);
	IFileOperationProgressSink_FinishOperations(sink, S_OK);
	DbgPrint("IFileOperation::CopyItem:SHFileOperationW: (%d)\n", resp);
	operation->aborted = FALSE;		

    FIXME("(%p, %p, %p, %s, %p): stub.\n", iface, item, folder, debugstr_w(name), sink);
    
    return S_OK;
}

static HRESULT WINAPI file_operation_CopyItems(IFileOperation *iface, IUnknown *items, IShellItem *folder)
{
	DbgPrint("IFileOperation::CopyItems called\n");
	// struct file_operation *operation = impl_from_IFileOperation(iface);
	// operation->fileOperation.wFunc = FO_COPY;
	// operation->operation = FO_COPY;
	// operation->fileOperation.fFlags = FOF_NOCONFIRMMKDIR;
	
    // FIXME("(%p, %p, %p): stub.\n", iface, items, folder);

    return S_OK;
}

static HRESULT WINAPI file_operation_DeleteItem(IFileOperation *iface, IShellItem *item,
        IFileOperationProgressSink *sink)
{
    struct file_operation *operation = impl_from_IFileOperation(iface);
	DWORD flags = FOF_MULTIDESTFILES          |
                  FOF_NOCONFIRMATION          |
                  FOF_NOCONFIRMMKDIR          |
                  FOF_NOCOPYSECURITYATTRIBS   |
                  FOF_NOERRORUI               |
                  FOF_RENAMEONCOLLISION;
	int resp;
	
	DbgPrint("IFileOperation::DeleteItem called\n");

    operation->sink = sink;
	resp = configureAndDoFileOperation(operation->fileOperation, item, NULL, NULL, FO_DELETE, flags);
	IFileOperationProgressSink_FinishOperations(sink, S_OK);
	DbgPrint("IFileOperation::DeleteItem:SHFileOperationW: (%d)\n", resp);
	operation->aborted = FALSE;		

    FIXME("(%p, %p, %p): stub.\n", iface, item, sink);
    
    return S_OK;
}

static HRESULT WINAPI file_operation_DeleteItems(IFileOperation *iface, IUnknown *items)
{
	DbgPrint("IFileOperation::DeleteItems called\n");
	// struct file_operation *operation = impl_from_IFileOperation(iface);
	// operation->fileOperation.wFunc = FO_DELETE;
	// operation->fileOperation.fFlags = FOF_MULTIDESTFILES;
	// operation->operation = FO_DELETE;
	
    // FIXME("(%p, %p): stub.\n", iface, items);

    return S_OK;
}

static HRESULT WINAPI file_operation_NewItem(IFileOperation *iface, IShellItem *folder, DWORD attributes,
        LPCWSTR name, LPCWSTR template, IFileOperationProgressSink *sink)
{
	DbgPrint("IFileOperation::NewItem called\n");
	// struct file_operation *operation = impl_from_IFileOperation(iface);
	// operation->operation = FO_NEW_ITEM;	
	// // struct file_operation *operation = impl_from_IFileOperation(iface);
	
    // FIXME("(%p, %p, %lx, %s, %s, %p): stub.\n", iface, folder, attributes,
          // debugstr_w(name), debugstr_w(template), sink);

    return S_OK;
}

static HRESULT WINAPI file_operation_PerformOperations(IFileOperation *iface)
{
	DbgPrint("IFileOperation::PerformOperations called\n");
	
	//Just return ok, because the real operation was done in operation
	
    return S_OK;
}

static HRESULT WINAPI file_operation_GetAnyOperationsAborted(IFileOperation *iface, BOOL *aborted)
{
	struct file_operation *operation = impl_from_IFileOperation(iface);
	
	DbgPrint("IFileOperation::GetAnyOperationsAborted called\n");
	
	*aborted = operation->aborted;
    FIXME("(%p, %p): stub.\n", iface, aborted);

    return S_OK;
}

static const IFileOperationVtbl file_operation_vtbl =
{
    file_operation_QueryInterface,
    file_operation_AddRef,
    file_operation_Release,
    file_operation_Advise,
    file_operation_Unadvise,
    file_operation_SetOperationFlags,
    file_operation_SetProgressMessage,
    file_operation_SetProgressDialog,
    file_operation_SetProperties,
    file_operation_SetOwnerWindow,
    file_operation_ApplyPropertiesToItem,
    file_operation_ApplyPropertiesToItems,
    file_operation_RenameItem,
    file_operation_RenameItems,
    file_operation_MoveItem,
    file_operation_MoveItems,
    file_operation_CopyItem,
    file_operation_CopyItems,
    file_operation_DeleteItem,
    file_operation_DeleteItems,
    file_operation_NewItem,
    file_operation_PerformOperations,
    file_operation_GetAnyOperationsAborted
};

HRESULT WINAPI IFileOperation_Constructor(IUnknown *outer, REFIID riid, void **out)
{
    struct file_operation *object;
    HRESULT hr;
	SHFILEOPSTRUCTW fileOperation = {0};	

	DbgPrint("IFileOperation_Constructor called\n");

    object = calloc(1, sizeof(*object));
    if (!object)
        return E_OUTOFMEMORY;

	ZeroMemory(&fileOperation, sizeof(fileOperation));
    object->IFileOperation_iface.lpVtbl = &file_operation_vtbl;
    object->ref = 1;
	object->fileOperation = fileOperation;
    object->aborted = FALSE;	

    hr = IFileOperation_QueryInterface(&object->IFileOperation_iface, riid, out);
    IFileOperation_Release(&object->IFileOperation_iface);
	
	DbgPrint("IFileOperation_Constructor end\n");

    return hr;
}
