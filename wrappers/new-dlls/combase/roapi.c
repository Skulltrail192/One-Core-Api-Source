/*++

Copyright (c) 2023 Shorthorn Project

Module Name:

    main.c

Abstract:

    This module implements COM Marshaling functions APIs

Author:

    Skulltrail 07-November-2023

Revision History:

--*/

#define WIN32_NO_STATUS

#include "main.h"
#include "roapi.h"
#include "roparameterizediid.h"
#include "roerrorapi.h"
#include "winstring.h"

WINE_DEFAULT_DEBUG_CHANNEL(combase);

#define COINIT_DISABLEOLE1DDE 4

/***********************************************************************
 *      CleanupTlsOleState (combase.@)
 */
void WINAPI CleanupTlsOleState(void *unknown)
{
    FIXME("(%p): stub\n", unknown);
}

HRESULT WINAPI RoInitialize(RO_INIT_TYPE type)
{
    switch (type) {
    case RO_INIT_SINGLETHREADED:
        return CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLEOLE1DDE);
    case RO_INIT_MULTITHREADED:
        return CoInitializeEx(NULL, COINIT_MULTITHREADED | COINIT_DISABLEOLE1DDE);
    default:
        // Multithreaded by default!
        return CoInitializeEx(NULL, COINIT_MULTITHREADED | COINIT_DISABLEOLE1DDE);
    }
}

void WINAPI RoUninitialize(void)
{
    CoUninitialize();
}

HRESULT
WINAPI
RoGetActivationFactory(
	_In_ HSTRING activatableClassId,
	_In_ REFIID iid,
	_COM_Outptr_ void** factory
)
{
	// if (auto const pRoGetActivationFactory = try_get_RoGetActivationFactory())
	// {
		// return pRoGetActivationFactory(activatableClassId, iid, factory);
	// }

	if (factory)
		*factory = NULL;

	return E_NOTIMPL;
}

HRESULT
WINAPI
RoActivateInstance(
	_In_ HSTRING activatableClassId,
	_COM_Outptr_ IInspectable** instance
)
{
	// if (auto const pRoActivateInstance = try_get_RoActivateInstance())
	// {
		// return pRoActivateInstance(activatableClassId, instance);
	// }

	if (instance)
		*instance = NULL;

	return E_NOTIMPL;
}