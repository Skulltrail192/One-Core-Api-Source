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

#include <wine/config.h>

#include <ntstatus.h>
#define WIN32_NO_STATUS

#include <wine/debug.h>

#include <windef.h>
#include <winbase.h>
#include <wingdi.h>
#include <winuser.h>
#include <winnls.h>
#include <windowsx.h>
#include <undocuser.h>
#include <uxtheme.h>

HTHEME 
WINAPI 
OpenThemeDataNative(
  HWND    hwnd,
  LPCWSTR pszClassList
);

HTHEME 
WINAPI 
OpenThemeDataInternal(
  HWND    hwnd,
  LPCWSTR pszClassList
)
{
	if(wcscmp(pszClassList, L"TASKDIALOG") == 0){
		return OpenThemeDataNative(hwnd, L"HEADER");
	}
	
	return OpenThemeDataNative(hwnd, pszClassList);
}