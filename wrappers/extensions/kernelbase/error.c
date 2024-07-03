/*++

Copyright (c) 2017 Shorthorn Project

Module Name:

    error.c

Abstract:

    This module contains the Win32 error APIs.

Author:

    Skulltrail 19-March-2017

Revision History:

--*/

#include "main.h"

WINE_DEFAULT_DEBUG_CHANNEL(error); 

/*
* @implemented
*/
DWORD
WINAPI
BaseSetLastNTError(
	IN NTSTATUS Status
)
{
    DWORD dwErrCode;

    /* Convert from NT to Win32, then set */
    dwErrCode = RtlNtStatusToDosError(Status);
    SetLastError(dwErrCode);
    return dwErrCode;
}

UINT
WINAPI
GetErrorMode(VOID)
{
    NTSTATUS Status;
    UINT ErrMode;

    /* Query the current setting */
    Status = NtQueryInformationProcess(NtCurrentProcess(),
                                       ProcessDefaultHardErrorMode,
                                       &ErrMode,
                                       sizeof(ErrMode),
                                       NULL);
    if (!NT_SUCCESS(Status))
    {
        /* Fail if we couldn't query */
        BaseSetLastNTError(Status);
        return 0;
    }

    /* Check if NOT failing critical errors was requested */
    if (ErrMode & SEM_FAILCRITICALERRORS)
    {
        /* Mask it out, since the native API works differently */
        ErrMode &= ~SEM_FAILCRITICALERRORS;
    }
    else
    {
        /* OR it if the caller didn't, due to different native semantics */
        ErrMode |= SEM_FAILCRITICALERRORS;
    }

    /* Return the mode */
    return ErrMode;
}