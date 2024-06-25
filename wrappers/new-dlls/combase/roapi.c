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

const GUID GUID_NULL = { 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0, 0 } };

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

/***********************************************************************
 *      RoGetParameterizedTypeInstanceIID (combase.@)
 */
HRESULT WINAPI RoGetParameterizedTypeInstanceIID(UINT32 name_element_count, const WCHAR **name_elements,
                                                 const IRoMetaDataLocator *meta_data_locator, GUID *iid,
                                                 ROPARAMIIDHANDLE *hiid)
{
    FIXME("stub: %d %p %p %p %p\n", name_element_count, name_elements, meta_data_locator, iid, hiid);
    if (iid) *iid = GUID_NULL;
    if (hiid) *hiid = INVALID_HANDLE_VALUE;
    return E_NOTIMPL;
}

/***********************************************************************
 *      RoGetApartmentIdentifier (combase.@)
 */
HRESULT WINAPI RoGetApartmentIdentifier(UINT64 *identifier)
{
    FIXME("(%p): stub\n", identifier);

    if (!identifier)
        return E_INVALIDARG;

    *identifier = 0xdeadbeef;
    return S_OK;
}

/***********************************************************************
 *      RoGetServerActivatableClasses (combase.@)
 */
HRESULT WINAPI RoGetServerActivatableClasses(HSTRING name, HSTRING **classes, DWORD *count)
{
    FIXME("(%p, %p, %p): stub\n", name, classes, count);

    if (count)
        *count = 0;
    return S_OK;
}

HRESULT
WINAPI
RoRegisterActivationFactories(
	_In_reads_(count) HSTRING* activatableClassIds,
	_In_reads_(count) PFNGETACTIVATIONFACTORY* activationFactoryCallbacks,
	_In_ UINT32 count,
	_Out_ RO_REGISTRATION_COOKIE* cookie
)
{
	if (cookie)
	   *cookie = NULL;

	return E_NOTIMPL;
}

/***********************************************************************
 *      RoRegisterForApartmentShutdown (combase.@)
 */
HRESULT WINAPI RoRegisterForApartmentShutdown(IApartmentShutdown *callback,
        UINT64 *identifier, APARTMENT_SHUTDOWN_REGISTRATION_COOKIE *cookie)
{
    HRESULT hr;

    FIXME("(%p, %p, %p): stub\n", callback, identifier, cookie);

    hr = RoGetApartmentIdentifier(identifier);
    if (FAILED(hr))
        return hr;

    if (cookie)
        *cookie = (void *)0xcafecafe;
    return S_OK;
}


/***********************************************************************
 *      RoOriginateError (combase.@)
 */
BOOL WINAPI RoOriginateError(HRESULT error, HSTRING message)
{
    FIXME("%#lx, %s: stub\n", error,message);
    return FALSE;
}

/***********************************************************************
 *      GetRestrictedErrorInfo (combase.@)
 */
HRESULT WINAPI GetRestrictedErrorInfo(IRestrictedErrorInfo **info)
{
    FIXME( "(%p)\n", info );
    return S_OK;
}

/***********************************************************************
 *      RoOriginateLanguageException (combase.@)
 */
BOOL WINAPI RoOriginateLanguageException(HRESULT error, HSTRING message, IUnknown *language_exception)
{
    FIXME("%#lx, %s, %p: stub\n", error, message, language_exception);
    return TRUE;
}
