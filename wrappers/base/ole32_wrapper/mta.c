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

typedef struct _MTA_USAGE_INCREMENTOR
{
  ULONG ulTime;
  DWORD dwThreadId;
}MTA_USAGE_INCREMENTOR, *PMTA_USAGE_INCREMENTOR; // This struct came from the public symbol for combase.
// Very little information or none at all is stripped from the symbol, including TEB data.

DWORD MTAThreadToRemove = 0;

void CreateMTAUsageThread()
// Suggestion to self: implement mutex object and use WaitForSingleObject instead.
// But this does not seem necessary at this time.
{
	DWORD Tid;
	CoInitializeEx(NULL, 0);
	Tid = GetCurrentThreadId();
	while(MTAThreadToRemove != Tid)
		Sleep(1); // Sleep(0) is equivalent to taking your CPU hostage when it is unnecessary
    MTAThreadToRemove = 0;
	CoUninitialize();
}

HRESULT WINAPI CoIncrementMTAUsage(CO_MTA_USAGE_COOKIE *pCookie) 
/*
   My earlier drafts of this function did not create a thread, but used internal functions and incremented
   a global value in ole32.dll. Those are not suitable for wrappers.
   
   As the main intent of this function is to keep MTA alive when no MTA threads are running, the function
   creates a thread that initializes a MTA then suspends immediately. The thread is resumed by a calloc
   to CoDecrementMTAUsage.
*/
{
	PMTA_USAGE_INCREMENTOR MTAIncrementor;
	DWORD ThreadId;
	MTAIncrementor = CoTaskMemAlloc(sizeof(MTA_USAGE_INCREMENTOR));
	CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)CreateMTAUsageThread, 0, 0, &ThreadId);
	MTAIncrementor->dwThreadId = ThreadId;
	MTAIncrementor->ulTime = GetTickCount();

	*pCookie = (CO_MTA_USAGE_COOKIE)MTAIncrementor;

	return S_OK;
}

HRESULT WINAPI CoDecrementMTAUsage(CO_MTA_USAGE_COOKIE *pCookie)
{
	PMTA_USAGE_INCREMENTOR MTAIncrementor;
	if(pCookie)
	{
		MTAIncrementor = (PMTA_USAGE_INCREMENTOR)*pCookie;
		MTAThreadToRemove = MTAIncrementor->dwThreadId;

		CoTaskMemFree(*pCookie);
		return S_OK;
	}
	else
		return E_INVALIDARG;
}