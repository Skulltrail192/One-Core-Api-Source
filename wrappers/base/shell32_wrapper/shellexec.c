/*
 * 				Shell Library Functions
 *
 * Copyright 1998 Marcus Meissner
 * Copyright 2002 Eric Pouech
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

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <ctype.h>
#include <assert.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "winreg.h"
#include "winuser.h"
#include "shlwapi.h"
#include "ddeml.h"

#include "main.h"
#include "pidl.h"
#include "shresdef.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(exec);

#define SEE_MASK_CLASSALL (SEE_MASK_CLASSNAME | SEE_MASK_CLASSKEY)

typedef UINT_PTR (*SHELL_ExecuteW32)(const WCHAR *lpCmd, WCHAR *env, BOOL shWait,
			    const SHELLEXECUTEINFOW *sei, LPSHELLEXECUTEINFOW sei_out);

HRESULT WINAPI PathCchCanonicalize(WCHAR *out, SIZE_T size, const WCHAR *in);

/*************************************************************************
 * ShellExecuteW			[SHELL32.294]
 * from shellapi.h
 * WINSHELLAPI HINSTANCE APIENTRY ShellExecuteW(HWND hwnd, LPCWSTR lpVerb,
 * LPCWSTR lpFile, LPCWSTR lpParameters, LPCWSTR lpDirectory, INT nShowCmd);
 */
HINSTANCE WINAPI ShellExecuteWInternal(HWND hwnd, LPCWSTR lpVerb, LPCWSTR lpFile,
                               LPCWSTR lpParameters, LPCWSTR lpDirectory, INT nShowCmd)
{
	PathCchCanonicalize(lpFile, MAX_PATH, lpFile);
	return ShellExecuteW(hwnd, lpVerb, lpFile, lpParameters, lpDirectory, nShowCmd);

}