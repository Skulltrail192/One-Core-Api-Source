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

EXECUTION_STATE
WINAPI
SetThreadExecutionState(EXECUTION_STATE esFlags);

#define ES_SYSTEM_REQUIRED   ((DWORD)0x00000001)
#define ES_DISPLAY_REQUIRED  ((DWORD)0x00000002)
#define ES_USER_PRESENT      ((DWORD)0x00000004)
#define ES_AWAYMODE_REQUIRED ((DWORD)0x00000040)
#define ES_CONTINUOUS        ((DWORD)0x80000000)

BOOL 
WINAPI 
PowerSetRequest(
	HANDLE             PowerRequest,
	POWER_REQUEST_TYPE RequestType
)
{
	//LONG dwQuantity;
	//DWORD dwSize;
	ULONG PreviousRequest;
	
	switch (RequestType)
	{
	case PowerRequestDisplayRequired:
		PreviousRequest = SetThreadExecutionState(ES_CONTINUOUS | ES_DISPLAY_REQUIRED);
		SetThreadExecutionState(ES_CONTINUOUS | ES_DISPLAY_REQUIRED | PreviousRequest);
		return TRUE;
	case PowerRequestSystemRequired:
		PreviousRequest = SetThreadExecutionState(ES_CONTINUOUS | ES_SYSTEM_REQUIRED);
		SetThreadExecutionState(ES_CONTINUOUS | ES_SYSTEM_REQUIRED | PreviousRequest);
		return TRUE;
	case PowerRequestAwayModeRequired:
		PreviousRequest = SetThreadExecutionState(ES_CONTINUOUS | ES_AWAYMODE_REQUIRED);
		SetThreadExecutionState(ES_CONTINUOUS | ES_AWAYMODE_REQUIRED | PreviousRequest);
		return TRUE;
	case PowerRequestExecutionRequired:
		PreviousRequest = SetThreadExecutionState(ES_CONTINUOUS | ES_SYSTEM_REQUIRED);
		SetThreadExecutionState(ES_CONTINUOUS | ES_SYSTEM_REQUIRED | PreviousRequest);
		return TRUE;
	}
	SetLastError(ERROR_INVALID_PARAMETER);
	return FALSE;
}

BOOL 
WINAPI 
PowerClearRequest(
	HANDLE             PowerRequest,
	POWER_REQUEST_TYPE RequestType
)
{
	// LONG dwQuantity;
	// DWORD dwSize;
	switch (RequestType)
	{
	case PowerRequestDisplayRequired:
		SetThreadExecutionState(SetThreadExecutionState(ES_CONTINUOUS) & ~ES_DISPLAY_REQUIRED);
		return TRUE;
	case PowerRequestSystemRequired:
		SetThreadExecutionState(SetThreadExecutionState(ES_CONTINUOUS) & ~ES_SYSTEM_REQUIRED);
		return TRUE;
	case PowerRequestAwayModeRequired:
		SetThreadExecutionState(SetThreadExecutionState(ES_CONTINUOUS) & ~ES_AWAYMODE_REQUIRED);
		return TRUE;
	case PowerRequestExecutionRequired:
		SetThreadExecutionState(SetThreadExecutionState(ES_CONTINUOUS) & ~ES_SYSTEM_REQUIRED);
		return TRUE;
	}
	SetLastError(ERROR_INVALID_PARAMETER);
	return FALSE;
}

HANDLE 
WINAPI 
PowerCreateRequest(
	PREASON_CONTEXT Context
)
{
	return CreateSemaphoreA(NULL, 0, 1, NULL);
}