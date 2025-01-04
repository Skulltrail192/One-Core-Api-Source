/*
 * Copyright 2014 Martin Storsjo
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

#include <string.h>
#include <wchar.h>

#include "windows.h"
#include "winerror.h"
#include "hstring.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(winstring);

/***********************************************************************
 *           CoCreateInstanceFromApp    (combase.@)
 */
HRESULT WINAPI CoCreateInstanceFromApp(REFCLSID rclsid, IUnknown *outer, DWORD cls_context,
        void *server_info, ULONG count, MULTI_QI *results)
{
    TRACE("%s, %p, %#lx, %p, %lu, %p\n", debugstr_guid(rclsid), outer, cls_context, server_info,
            count, results);

    return CoCreateInstanceEx(rclsid, outer, cls_context | CLSCTX_APPCONTAINER, server_info,
            count, results);
}

/***********************************************************************
 *      CoGetCallState (ole32.@)
 */
HRESULT WINAPI CoGetCallState(int unknown, PULONG unknown2)
{
    return E_NOTIMPL;
}

/***********************************************************************
 *      CoGetActivationState (ole32.@)
 */
HRESULT WINAPI CoGetActivationState(GUID guid, DWORD unknown, DWORD *unknown2)
{
    return E_NOTIMPL;
}