/*++

Copyright (c) 2024 Shorthorn Project

Module Name:

    main.c

Abstract:

    Implement Windows 8 functions for Userenv

Author:

    Skulltrail 16-November-2024

Revision History:

--*/

#include "precomp.h"

#define NDEBUG
#include <debug.h>

HINSTANCE hInstance = NULL;

BOOL
WINAPI
DllMain(HINSTANCE hinstDLL,
        DWORD fdwReason,
        LPVOID lpvReserved)
{
    if (fdwReason == DLL_PROCESS_ATTACH)
    {
        hInstance = hinstDLL;
    }

    return TRUE;
}
		
HRESULT
WINAPI
CreateAppContainerProfile(
   _In_ PCWSTR _szAppContainerName,
   _In_ PCWSTR _szDisplayName,
   _In_ PCWSTR _szDescription,
   _In_reads_opt_(_uCapabilityCount) PSID_AND_ATTRIBUTES _pCapabilities,
   _In_ DWORD _uCapabilityCount,
   _Outptr_ PSID* _ppSidAppContainerSid
)
{
            // if (const auto _pfnCreateAppContainerProfile = try_get_CreateAppContainerProfile())
            // {
                // return _pfnCreateAppContainerProfile(_szAppContainerName, _szDisplayName, _szDescription, _pCapabilities, _uCapabilityCount, _ppSidAppContainerSid);
            // }

    if (!_ppSidAppContainerSid)
       return E_INVALIDARG;
    *_ppSidAppContainerSid = NULL;
    return S_OK;
}

// 最低受支持的客户端	Windows 8 [仅限桌面应用]
// 最低受支持的服务器	Windows Server 2012[仅限桌面应用]
HRESULT
WINAPI
DeleteAppContainerProfile(
   _In_ PCWSTR _szAppContainerName
)
{
   // if (const auto _pfnDeleteAppContainerProfile = try_get_DeleteAppContainerProfile())
   // {
   // return _pfnDeleteAppContainerProfile(_szAppContainerName);
   // }

    return S_OK;
}

// 最低受支持的客户端	Windows 8 [仅限桌面应用]
// 最低受支持的服务器	Windows Server 2012[仅限桌面应用]
HRESULT
WINAPI
DeriveAppContainerSidFromAppContainerName(
   _In_ PCWSTR _szAppContainerName,
   _Outptr_ PSID* _ppsidAppContainerSid
)
{
    // if (const auto _pfnDeriveAppContainerSidFromAppContainerName = try_get_DeriveAppContainerSidFromAppContainerName())
    // {
    // return _pfnDeriveAppContainerSidFromAppContainerName(_szAppContainerName, _ppsidAppContainerSid);
    // }
    if (!_ppsidAppContainerSid)
        return E_INVALIDARG;
    *_ppsidAppContainerSid = NULL;
    return S_OK;
}

// 最低受支持的客户端	Windows 8 [仅限桌面应用]
// 最低受支持的服务器	Windows Server 2012[仅限桌面应用]
HRESULT
WINAPI
GetAppContainerFolderPath(
    _In_ PCWSTR _szAppContainerSid,
    _Outptr_ PWSTR* _ppszPath
)
{
    // if (const auto _pfnGetAppContainerFolderPath = try_get_GetAppContainerFolderPath())
    // {
    // return _pfnGetAppContainerFolderPath(_szAppContainerSid, _ppszPath);
    // }
    if (!_ppszPath)
        return E_INVALIDARG;
    *_ppszPath = NULL;
    return S_OK;
}

// 最低受支持的客户端	Windows 8 [仅限桌面应用]
// 最低受支持的服务器	Windows Server 2012[仅限桌面应用]
HRESULT
WINAPI
GetAppContainerRegistryLocation(
    _In_ REGSAM _DesiredAccess,
    _Outptr_ PHKEY _phAppContainerKey
)
{
    // if (const auto _pfnGetAppContainerRegistryLocation = try_get_GetAppContainerRegistryLocation())
    // {
    // return _pfnGetAppContainerRegistryLocation(_DesiredAccess, _phAppContainerKey);
    // }
    if (!_phAppContainerKey)
        return E_INVALIDARG;
    *_phAppContainerKey = NULL;
    return S_OK;
}