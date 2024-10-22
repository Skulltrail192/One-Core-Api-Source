/*
 *  ReactOS kernel
 *  Copyright (C) 2004 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            dll/win32/userenv/userenv.c
 * PURPOSE:         DLL initialization code
 * PROGRAMMER:      Eric Kohl
 */

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
            return E_NOTIMPL;
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

            return E_NOTIMPL;
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
            return E_NOTIMPL;
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
            return E_NOTIMPL;
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
            return E_NOTIMPL;
        }