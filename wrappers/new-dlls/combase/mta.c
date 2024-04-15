/*++

Copyright (c) 2023 Shorthorn Project

Module Name:

    mta.c

Abstract:

    This module implements COM MTA functions APIs

Author:

    Skulltrail 12-October-2023

Revision History:

--*/

#define WIN32_NO_STATUS

#include "main.h"

#define NT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0)

typedef struct _MTA_USAGE_INCREMENTOR
{
  ULONG ulTime;
  DWORD dwThreadId;
}MTA_USAGE_INCREMENTOR, *PMTA_USAGE_INCREMENTOR; // This struct came from the public symbol for combase.
// Very little information or none at all is stripped from the symbol, including TEB data.

DWORD MTAThreadToRemove = 0;

DWORD WINAPI KxCompMTAUsageIncrementerThread(
    HANDLE Handle)
{
    CoInitializeEx(NULL, COINIT_MULTITHREADED);
    SetEvent(Handle);
    SleepEx(INFINITE, TRUE);
    CoUninitialize();
    return 0;
}

HRESULT WINAPI CoIncrementMTAUsage(
    CO_MTA_USAGE_COOKIE *Cookie)
{
	HANDLE Event;
    if (!Cookie) {
        return E_INVALIDARG;
    }
    Event = CreateEventW(NULL, TRUE, FALSE, NULL);
    if (!Event) {
        return HRESULT_FROM_WIN32(GetLastError());
    }
    *Cookie = CreateThread(
        NULL,
        8192,
        KxCompMTAUsageIncrementerThread,
        Event,
        0,
        NULL);
    if (!*Cookie) {
        CloseHandle(Event);
        return HRESULT_FROM_WIN32(GetLastError());
    }
    WaitForSingleObject(Event, INFINITE);
    CloseHandle(Event);
    return S_OK;
}

HRESULT WINAPI CoDecrementMTAUsage(
    IN    CO_MTA_USAGE_COOKIE        Cookie)
{
    NTSTATUS Status;

    if (!Cookie) {
        return E_INVALIDARG;
    }

    Status = NtAlertThread(Cookie);
    CloseHandle(Cookie);

    if (NT_SUCCESS(Status)) {
        return S_OK;
    } else {
        return HRESULT_FROM_WIN32(RtlNtStatusToDosError(Status));
    }
}