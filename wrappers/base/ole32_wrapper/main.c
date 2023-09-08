/*
 *     Copyright 2002 Juergen Schmied
 *     Copyright 2002 Marcus Meissner
 *     Copyright 2004 Mike Hearn, for CodeWeavers
 *     Copyright 2004 Rob Shearman, for CodeWeavers
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

#define WIN32_NO_STATUS

#include "main.h"

WINE_DEFAULT_DEBUG_CHANNEL(ole32);

static struct list registered_classes = LIST_INIT(registered_classes);

static CRITICAL_SECTION registered_classes_cs;
static CRITICAL_SECTION_DEBUG registered_classes_cs_debug =
{
    0, 0, &registered_classes_cs,
    { &registered_classes_cs_debug.ProcessLocksList, &registered_classes_cs_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": registered_classes_cs") }
};
static CRITICAL_SECTION registered_classes_cs = { &registered_classes_cs_debug, -1, 0, 0, 0, 0 };

/* will create if necessary */
static inline struct oletls *COM_CurrentInfo(void)
{
    if (!NtCurrentTeb()->ReservedForOle)
        NtCurrentTeb()->ReservedForOle = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(struct oletls));

    return NtCurrentTeb()->ReservedForOle;
}

/***********************************************************************
 *           CoGetApartmentType [OLE32.@]
 */
HRESULT WINAPI CoGetApartmentType(APTTYPE *type, APTTYPEQUALIFIER *qualifier)
{
    struct oletls *info = COM_CurrentInfo();

    //FIXME("(%p, %p): semi-stub\n", type, qualifier);

    if (!type || !qualifier)
        return E_INVALIDARG;

    if (!info)
        return E_OUTOFMEMORY;

    if (!info->apt)
        *type = APTTYPE_CURRENT;
    else if (info->apt->multi_threaded)
        *type = APTTYPE_MTA;
    else if (info->apt->main)
        *type = APTTYPE_MAINSTA;
    else
        *type = APTTYPE_STA;

    *qualifier = APTTYPEQUALIFIER_NONE;

    return info->apt ? S_OK : CO_E_NOTINITIALIZED;
}


HRESULT 
WINAPI
CoDisconnectContext(
  _In_ DWORD dwTimeout
)
{
	return S_OK;
}

/***********************************************************************
 *      CoGetActivationState (ole32.@)
 */
HRESULT WINAPI CoGetActivationState(GUID guid, DWORD unknown, DWORD *unknown2)
{
    return E_NOTIMPL;
}

/***********************************************************************
 *      CoGetCallState (ole32.@)
 */
HRESULT WINAPI CoGetCallState(int unknown, PULONG unknown2)
{
    return E_NOTIMPL;
}

/***********************************************************************
 *           CoIncrementMTAUsage    (combase.@)
 */
HRESULT WINAPI CoIncrementMTAUsage(CO_MTA_USAGE_COOKIE *cookie)
{
    //return apartment_increment_mta_usage(cookie);
	return E_NOTIMPL;
}

/***********************************************************************
 *           CoDecrementMTAUsage    (combase.@)
 */
HRESULT WINAPI CoDecrementMTAUsage(CO_MTA_USAGE_COOKIE cookie)
{
    //apartment_decrement_mta_usage(cookie);
    //return S_OK;
	return E_NOTIMPL;
}