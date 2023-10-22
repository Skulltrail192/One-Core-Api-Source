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

#include <ntstatus.h>
#define WIN32_NO_STATUS

#include <wine/debug.h>

#include <winbase.h>
#include <ntsecapi.h>
#include <winuser.h>
#include <shlwapi.h>

WINE_DEFAULT_DEBUG_CHANNEL(shlwapi);

typedef enum  { 
  SFBS_FLAGS_ROUND_TO_NEAREST_DISPLAYED_DIGIT     = 0x00000001,
  SFBS_FLAGS_TRUNCATE_UNDISPLAYED_DECIMAL_DIGITS  = 0x00000002
} SFBS_FLAGS;

HRESULT 
WINAPI 
PSStrFormatByteSizeEx(
        ULONGLONG  ull,
        SFBS_FLAGS flags,
  _Out_ PWSTR      pszBuf,
        UINT       cchBuf
);

BOOL WINAPI DllMain(HINSTANCE hInstDLL, DWORD fdwReason, LPVOID lpv)
{
    TRACE("fdwReason %u\n", fdwReason);

    switch(fdwReason)
    {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hInstDLL);
            break;
    }

    return TRUE;
}

HRESULT 
WINAPI
PathMatchSpecExA(
  _In_ LPCTSTR pszFile,
  _In_ LPCTSTR pszSpec,
  _In_ DWORD   dwFlags
)
{
	return PathMatchSpecA(pszFile, pszSpec);
}

HRESULT 
WINAPI
PathMatchSpecExW(
  _In_ LPWSTR pszFile,
  _In_ LPWSTR pszSpec,
  _In_ DWORD   dwFlags
)
{
	return PathMatchSpecW(pszFile, pszSpec);
}

HRESULT 
WINAPI 
SHAutoCompGetPidl(
	HWND hWnd, 
	LPARAM windowParam, 
	int Unknown, 
	WPARAM wParam
)
{
  UINT WindowMessage; // eax@1
  LRESULT LocalResult; // eax@3
  HRESULT result; // eax@4
  LPARAM lParam; // [sp+0h] [bp-8h]@1
  int v8; // [sp+4h] [bp-4h]@1

  WindowMessage = RegisterWindowMessageA("AC_GetPidl");
  lParam = (LPARAM)&windowParam;
  v8 = Unknown;
  if ( !WindowMessage )
    WindowMessage = 33070;
  LocalResult = SendMessageA(hWnd, WindowMessage, wParam, (LPARAM)&lParam);
  if ( LocalResult )
    result = LocalResult != 1 ? LocalResult : 0;
  else
    result = 0x80070057u;
  return result;
}

/*************************************************************************
 * PathCreateFromUrlAlloc   [SHLWAPI.@]
 */
HRESULT 
WINAPI 
PathCreateFromUrlAlloc(
	LPCWSTR pszUrl, 
	LPWSTR *pszPath,
    DWORD dwReserved
)
{
    WCHAR pathW[MAX_PATH];
    DWORD size;
    HRESULT hr;

    size = MAX_PATH;
    hr = PathCreateFromUrlW(pszUrl, pathW, &size, dwReserved);
    if (SUCCEEDED(hr))
    {
        /* Yes, this is supposed to crash if pszPath is NULL */
        *pszPath = StrDupW(pathW);
    }
    return hr;
}

HRESULT 
WINAPI 
StrFormatByteSizeEx(
        ULONGLONG  ull,
        SFBS_FLAGS flags,
  _Out_ PWSTR      pszBuf,
        UINT       cchBuf
)
{
	return PSStrFormatByteSizeEx(ull, flags, pszBuf, cchBuf);	
}

DWORD __stdcall GetLastErrorError()
{
  DWORD result; // eax

  result = GetLastError();
  if ( !result )
    return 1;
  return result;
}

signed int __stdcall HRESULTFromLastErrorError()
{
  signed int result; // eax

  result = GetLastErrorError();
  if ( result > 0 )
    return (unsigned __int16)result | 0x80070000;
  return result;
}

int GetModuleResourceData(HMODULE hModule, LPCWSTR lpName, LPCWSTR lpType, DWORD *a4, DWORD *a5)
{
  HRSRC ResourceW; // eax
  HRSRC v6; // esi
  DWORD v7; // edi
  HGLOBAL Resource; // eax
  LPVOID v9; // eax

  ResourceW = FindResourceW(hModule, lpName, lpType);
  v6 = ResourceW;
  if ( !ResourceW )
    return HRESULTFromLastErrorError();
  v7 = SizeofResource(hModule, ResourceW);
  if ( !v7 )
    return HRESULTFromLastErrorError();
  Resource = LoadResource(hModule, v6);
  if ( !Resource )
    return HRESULTFromLastErrorError();
  v9 = LockResource(Resource);
  if ( !v9 )
    return HRESULTFromLastErrorError();
  *a4 = (DWORD)v9;
  *a5 = v7;
  return 0;
}

int __stdcall CreateStreamSTOnModuleResource(HMODULE hModule, BYTE *pInit, const WCHAR *cbInit, IStream **a4)
{
  int ModuleResourceData; // esi
  IStream *v5; // eax

  ModuleResourceData = GetModuleResourceData(hModule, (LPCWSTR)pInit, cbInit, (DWORD*)&pInit, (DWORD*)&cbInit);
  if ( ModuleResourceData >= 0 )
  {
    v5 = SHCreateMemStream(pInit, (UINT)cbInit);
    *a4 = v5;
    if ( !v5 )
      return 0x8007000E;
  }
  return ModuleResourceData;
}

int __stdcall SHCreateStreamOnModuleResourceW(HMODULE a1, BYTE *a2, const WCHAR *a3, IStream **a4)
{
  return CreateStreamSTOnModuleResource(a1, a2, a3, a4);
}