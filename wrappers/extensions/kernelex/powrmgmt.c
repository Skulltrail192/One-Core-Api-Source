/*++

Copyright (c) 2017 Shorthorn Project

Module Name:

    powrmgmt.c

Abstract:

    This module implements Power functions for the Win32 APIs.

Author:

    Skulltrail 14-May-2017

Revision History:

--*/

#include <main.h>

// Server 2003 DDK: PowerRequestCreate/Action do not exist. Stubbed them to fix Wine bug 48521
BOOL 
WINAPI 
PowerSetRequest(
    HANDLE PowerRequest, 
    POWER_REQUEST_TYPE RequestType
)
{
  SetLastError(0);
  return TRUE;
}

BOOL 
WINAPI 
PowerClearRequest(
    HANDLE PowerRequest, 
    POWER_REQUEST_TYPE RequestType
)
{
  SetLastError(0);
  return TRUE;
}

HANDLE 
WINAPI 
PowerCreateRequest(
    REASON_CONTEXT *context
)
{
  SetLastError(0);
  return (HANDLE)1;
}