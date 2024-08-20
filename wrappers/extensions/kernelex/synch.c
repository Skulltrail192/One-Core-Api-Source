/*++

Copyright (c) 2017  Shorthorn Project

Module Name:

    synch.c

Abstract:

    This module implements all Win32 syncronization
    objects.

Author:

    Skulltrail 16-March-2017

Revision History:

--*/
 
#define NDEBUG

#include <stdio.h>
#include "main.h"
#include <stdlib.h>
#include <malloc.h>
#include <winnt.h>

WINE_DEFAULT_DEBUG_CHANNEL(synch);

/* returns directory handle to \\BaseNamedObjects */
HANDLE get_BaseNamedObjects_handle(void)
{
    static HANDLE handle = NULL;
    static const WCHAR basenameW[] = {'\\','S','e','s','s','i','o','n','s','\\','%','u',
                                      '\\','B','a','s','e','N','a','m','e','d','O','b','j','e','c','t','s',0};
    WCHAR buffer[64];
    UNICODE_STRING str;
    OBJECT_ATTRIBUTES attr;

    if (!handle)
    {
        HANDLE dir;

        sprintfW( buffer, basenameW, NtCurrentTeb()->ProcessEnvironmentBlock->SessionId );
        RtlInitUnicodeString( &str, buffer );
        InitializeObjectAttributes(&attr, &str, 0, 0, NULL);
        NtOpenDirectoryObject(&dir, DIRECTORY_CREATE_OBJECT|DIRECTORY_TRAVERSE,
                              &attr);
        if (InterlockedCompareExchangePointer( &handle, dir, 0 ) != 0)
        {
            /* someone beat us here... */
            CloseHandle( dir );
        }
    }
    return handle;
}

/***********************************************************************
 *           InitializeCriticalSectionEx   (kernelex.@)
 */
BOOL WINAPI InitializeCriticalSectionEx(LPCRITICAL_SECTION lpCriticalSection,DWORD dwSpinCount,DWORD Flags)
{
	NTSTATUS RtlStatus=STATUS_SUCCESS;
	if (Flags&RTL_CRITICAL_SECTION_FLAG_RESERVED)
		RtlStatus=STATUS_INVALID_PARAMETER_3;
	if (dwSpinCount&0xFF000000)	//dwSpinCount>0x00FFFFFF
		RtlStatus=STATUS_INVALID_PARAMETER_2;
	if (NT_SUCCESS(RtlStatus))
	{
		if (Flags&RTL_CRITICAL_SECTION_FLAG_DYNAMIC_SPIN)
			dwSpinCount=dwSpinCount&0x00FFFFFF;
		else
			dwSpinCount=2000;
		//RTL_CRITICAL_SECTION_FLAG_static_INIT的效果是不初始化直接返回
		//RTL_CRITICAL_SECTION_FLAG_NO_DEBUG_INFO的效果是不分配DebugInfo的内存
		//由于不知道XP是否支持这些行为，稳妥起见不应用标记
		RtlStatus=RtlInitializeCriticalSectionAndSpinCount((PRTL_CRITICAL_SECTION)lpCriticalSection,dwSpinCount);
	}
	if (NT_SUCCESS(RtlStatus))
		return TRUE;
	BaseSetLastNTError(RtlStatus);
	return FALSE;
}
/***********************************************************************
 *           SleepConditionVariableCS   (KERNEL32.@)
 */
BOOL WINAPI SleepConditionVariableCS(PCONDITION_VARIABLE ConditionVariable,PCRITICAL_SECTION CriticalSection,DWORD dwMilliseconds)
{
	LARGE_INTEGER Timeout;
	NTSTATUS Result=RtlSleepConditionVariableCS(ConditionVariable,(PRTL_CRITICAL_SECTION)CriticalSection,BaseFormatTimeOut(&Timeout,dwMilliseconds));
	BaseSetLastNTError(Result);
	if (NT_SUCCESS(Result) && Result!=STATUS_TIMEOUT)
		return TRUE;
	return FALSE;
}

/***********************************************************************
 *           SleepConditionVariableSRW   (KERNEL32.@)
 */
BOOL WINAPI SleepConditionVariableSRW(PCONDITION_VARIABLE ConditionVariable,PSRWLOCK SRWLock,DWORD dwMilliseconds,ULONG Flags)
{
	LARGE_INTEGER Timeout;
	NTSTATUS Result=RtlSleepConditionVariableSRW(ConditionVariable,SRWLock,BaseFormatTimeOut(&Timeout,dwMilliseconds),Flags);
	BaseSetLastNTError(Result);
	if (NT_SUCCESS(Result) && Result!=STATUS_TIMEOUT)
		return TRUE;
	return FALSE;
}

/***********************************************************************
  *           InitOnceExecuteOnce    (KERNEL32.@)
  */
BOOL 
WINAPI 
InitOnceExecuteOnce( 
	INIT_ONCE *once, 
	PINIT_ONCE_FN func, 
	void *param, 
	void **context 
)
{	
	BOOL ret;
	
	//DbgPrint("InitOnceExecuteOnce called\n");
	
	ret = !RtlRunOnceExecuteOnce( once, (PRTL_RUN_ONCE_INIT_FN)func, param, context );
	
	//DbgPrint("InitOnceExecuteOnce:: ret is %d\n", ret);
	
    return ret;
}

/***********************************************************************
 *           CreateEventExW    (KERNEL32.@)
 */
HANDLE 
WINAPI 
CreateEventExW(
	LPSECURITY_ATTRIBUTES lpEventAttributes, 
	PCWSTR lpName, 
	DWORD dwFlags, 
	ACCESS_MASK DesiredAccess
)
{
  HANDLE Handle; // esi
  NTSTATUS Status; // eax
  OBJECT_ATTRIBUTES Obja; // [esp+4h] [ebp-20h]
  POBJECT_ATTRIBUTES pObja;
  LSA_UNICODE_STRING ObjectName; // [esp+1Ch] [ebp-8h]

  if ( dwFlags & 0xFFFFFFFC )
  {
    BaseSetLastNTError(STATUS_INVALID_PARAMETER_3);
    return NULL;
  }
  if ( ARGUMENT_PRESENT(lpName) )
  {
    RtlInitUnicodeString(&ObjectName, lpName);
    pObja = BaseFormatObjectAttributes(&Obja, lpEventAttributes, &ObjectName);
  }
  else
  {
    pObja = BaseFormatObjectAttributes(&Obja, lpEventAttributes, NULL);
  }
  Status = NtCreateEvent(
                   &Handle,
                   DesiredAccess,
                   pObja,
                   dwFlags & CREATE_EVENT_MANUAL_RESET ? NotificationEvent : SynchronizationEvent,
                   (BOOLEAN)dwFlags & CREATE_EVENT_INITIAL_SET);
	if ( NT_SUCCESS(Status) ) {
        if ( Status == STATUS_OBJECT_NAME_EXISTS ) {
            SetLastError(ERROR_ALREADY_EXISTS);
            }
        else {
            SetLastError(0);
            }
        return Handle;
        }
    else {
        BaseSetLastNTError(Status);
        return NULL;
        }
}

/***********************************************************************
 *           CreateEventExA    (KERNEL32.@)
 */
HANDLE 
WINAPI 
DECLSPEC_HOTPATCH 
CreateEventExA( 
	SECURITY_ATTRIBUTES *sa, 
	LPCSTR name, 
	DWORD flags, 
	DWORD access 
)
{
	WCHAR buffer[MAX_PATH];
	
    if (!name) return CreateEventExW( sa, NULL, flags, access );

	if (!MultiByteToWideChar( CP_ACP, 0, name, -1, buffer, MAX_PATH ))
	{
        SetLastError( ERROR_FILENAME_EXCED_RANGE );
        return 0;
    }
    return CreateEventExW( sa, buffer, flags, access );
}

/***********************************************************************
 *           CreateSemaphoreExW   (KERNEL32.@)
 */
HANDLE 
WINAPI 
DECLSPEC_HOTPATCH 
CreateSemaphoreExW( 
	SECURITY_ATTRIBUTES *sa, 
	LONG initial, 
	LONG max,
    LPCWSTR name, 
	DWORD flags, 
	DWORD access 
)
{
    HANDLE ret = 0;
    UNICODE_STRING nameW;
    OBJECT_ATTRIBUTES attr;
    NTSTATUS status;

    attr.Length                   = sizeof(attr);
    attr.RootDirectory            = 0;
    attr.ObjectName               = NULL;
    attr.Attributes               = OBJ_OPENIF | ((sa && sa->bInheritHandle) ? OBJ_INHERIT : 0);
    attr.SecurityDescriptor       = sa ? sa->lpSecurityDescriptor : NULL;
    attr.SecurityQualityOfService = NULL;
    if (name)
    {
        RtlInitUnicodeString( &nameW, name );
        attr.ObjectName = &nameW;
        attr.RootDirectory = get_BaseNamedObjects_handle();
    }

    status = NtCreateSemaphore( &ret, access, &attr, initial, max );
	
	DbgPrint("CreateSemaphoreExW :: NtCreateSemaphore Status: %0x%08x\n", status);
	
    if (status == STATUS_OBJECT_NAME_EXISTS)
        SetLastError( ERROR_ALREADY_EXISTS );
    else
        SetLastError( RtlNtStatusToDosError(status) );
    return ret;
}

/***********************************************************************
 *           CreateSemaphoreExA   (KERNEL32.@)
 */
HANDLE 
WINAPI 
DECLSPEC_HOTPATCH 
CreateSemaphoreExA( 
	SECURITY_ATTRIBUTES *sa, 
	LONG initial, 
	LONG max,
    LPCSTR name, 
	DWORD flags, 
	DWORD access 
)
{
    WCHAR buffer[MAX_PATH];

    if (!name) return CreateSemaphoreExW( sa, initial, max, NULL, flags, access );

    if (!MultiByteToWideChar( CP_ACP, 0, name, -1, buffer, MAX_PATH ))
    {
        SetLastError( ERROR_FILENAME_EXCED_RANGE );
        return 0;
    }
    return CreateSemaphoreExW( sa, initial, max, buffer, flags, access );
}

BOOL 
WINAPI 
SetWaitableTimerEx(
  _In_  HANDLE hTimer,
  _In_  const LARGE_INTEGER *lpDueTime,
  _In_  LONG lPeriod,
  _In_  PTIMERAPCROUTINE pfnCompletionRoutine,
  _In_  LPVOID lpArgToCompletionRoutine,
  _In_  PREASON_CONTEXT WakeContext,
  _In_  ULONG TolerableDelay
)
{
	return SetWaitableTimer(hTimer, 
							lpDueTime, 
							lPeriod, 
							pfnCompletionRoutine, 
							lpArgToCompletionRoutine, 
							TRUE);
}

HANDLE
APIENTRY
CreateMutexExW(
    LPSECURITY_ATTRIBUTES lpMutexAttributes,
    LPCWSTR lpName,
    DWORD                 dwFlags,
    DWORD                 dwDesiredAccess	
)
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES Obja;
    POBJECT_ATTRIBUTES pObja;
    HANDLE Handle;
    UNICODE_STRING ObjectName;

    if ( ARGUMENT_PRESENT(lpName) ) {
        RtlInitUnicodeString(&ObjectName,lpName);
        pObja = BaseFormatObjectAttributes(&Obja,lpMutexAttributes,&ObjectName);
        }
    else {
        pObja = BaseFormatObjectAttributes(&Obja,lpMutexAttributes,NULL);
        }

    Status = NtCreateMutant(
                &Handle,
                dwDesiredAccess,
                pObja,
                (dwFlags & CREATE_MUTEX_INITIAL_OWNER) != 0
                );

    if ( NT_SUCCESS(Status) ) {
        if ( Status == STATUS_OBJECT_NAME_EXISTS ) {
            SetLastError(ERROR_ALREADY_EXISTS);
            }
        else {
            SetLastError(0);
            }
        return Handle;
        }
    else {
        BaseSetLastNTError(Status);
        return NULL;
        }
}

HANDLE 
WINAPI 
CreateMutexExA( 
	LPSECURITY_ATTRIBUTES sa, 
	LPCSTR name, 
	DWORD flags, 
	DWORD access 
)
{
    ANSI_STRING nameA;
    NTSTATUS status;

    if (ARGUMENT_PRESENT(name)) 
		return CreateMutexExW( sa, NULL, flags, access );

    RtlInitAnsiString( &nameA, name );
    status = RtlAnsiStringToUnicodeString( &NtCurrentTeb()->StaticUnicodeString, &nameA, FALSE );
    if (status != STATUS_SUCCESS)
    {
        SetLastError( ERROR_FILENAME_EXCED_RANGE );
        return NULL;
    }
    return CreateMutexExW( sa, NtCurrentTeb()->StaticUnicodeString.Buffer, flags, access );
}

/***********************************************************************
*           InitOnceBeginInitialize    (KERNEL32.@)
*/
BOOL 
WINAPI 
InitOnceBeginInitialize( 
	INIT_ONCE *once, 
	DWORD flags, 
	BOOL *pending, 
	void **context 
)
{
     NTSTATUS status = RtlRunOnceBeginInitialize( once, flags, context );
     if (NT_SUCCESS(status)) 
	 {
		*pending = (status == STATUS_PENDING);
	 }else{ 
		SetLastError( RtlNtStatusToDosError(status) );
	 }
     return status >= 0;
}

/***********************************************************************
  *           InitOnceComplete    (KERNEL32.@)
  */
BOOL 
WINAPI 
InitOnceComplete( 
	INIT_ONCE *once, 
	DWORD flags, 
	void *context 
)
{
     NTSTATUS status = RtlRunOnceComplete( once, flags, context );
     if (status != STATUS_SUCCESS) SetLastError( RtlNtStatusToDosError(status) );
     return !status;
}

/***********************************************************************
 *           CreateWaitableTimerExW    (KERNEL32.@)
 */
HANDLE WINAPI CreateWaitableTimerExW( SECURITY_ATTRIBUTES *sa, LPCWSTR name, DWORD flags, DWORD access )
{
    HANDLE handle;
    NTSTATUS status;
    UNICODE_STRING nameW;
    OBJECT_ATTRIBUTES attr;

    attr.Length                   = sizeof(attr);
    attr.RootDirectory            = 0;
    attr.ObjectName               = NULL;
    attr.Attributes               = OBJ_OPENIF | ((sa && sa->bInheritHandle) ? OBJ_INHERIT : 0);
    attr.SecurityDescriptor       = sa ? sa->lpSecurityDescriptor : NULL;
    attr.SecurityQualityOfService = NULL;
    if (name)
    {
        RtlInitUnicodeString( &nameW, name );
        attr.ObjectName = &nameW;
        attr.RootDirectory = get_BaseNamedObjects_handle();
    }

    status = NtCreateTimer( &handle, access, &attr,
                 (flags & CREATE_WAITABLE_TIMER_MANUAL_RESET) ? NotificationTimer : SynchronizationTimer );
    if (status == STATUS_OBJECT_NAME_EXISTS)
        SetLastError( ERROR_ALREADY_EXISTS );
    else
        SetLastError( RtlNtStatusToDosError(status) );
    return handle;
}

/***********************************************************************
 *           CreateWaitableTimerExA    (KERNEL32.@)
 */
HANDLE WINAPI CreateWaitableTimerExA( SECURITY_ATTRIBUTES *sa, LPCSTR name, DWORD flags, DWORD access )
{
    WCHAR buffer[MAX_PATH];

    if (!name) return CreateWaitableTimerExW( sa, NULL, flags, access );

    if (!MultiByteToWideChar( CP_ACP, 0, name, -1, buffer, MAX_PATH ))
    {
        SetLastError( ERROR_FILENAME_EXCED_RANGE );
        return 0;
    }
    return CreateWaitableTimerExW( sa, buffer, flags, access );
}

BOOL 
WINAPI 
InitializeSynchronizationBarrier(
	LPSYNCHRONIZATION_BARRIER lpBarrier, 
	LONG lTotalThreads, 
	LONG lSpinCount
)
{
	SYSTEM_INFO sysinfo;
	HANDLE hEvent0;
	HANDLE hEvent1;

	if (!lpBarrier || lTotalThreads < 1 || lSpinCount < -1)
	{
		SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;
	}

	ZeroMemory(lpBarrier, sizeof(SYNCHRONIZATION_BARRIER));

	if (lSpinCount == -1)
		lSpinCount = 2000;

	if (!(hEvent0 = CreateEvent(NULL, TRUE, FALSE, NULL)))
		return FALSE;

	if (!(hEvent1 = CreateEvent(NULL, TRUE, FALSE, NULL)))
	{
		CloseHandle(hEvent0);
		return FALSE;
	}

	GetNativeSystemInfo(&sysinfo);

	lpBarrier->Reserved1 = lTotalThreads;
	lpBarrier->Reserved2 = lTotalThreads;
	lpBarrier->Reserved3[0] = (ULONG_PTR)hEvent0;
	lpBarrier->Reserved3[1] = (ULONG_PTR)hEvent1;
	lpBarrier->Reserved4 = sysinfo.dwNumberOfProcessors;
	lpBarrier->Reserved5 = lSpinCount;

	return TRUE;
}

BOOL 
WINAPI 
EnterSynchronizationBarrier(
	LPSYNCHRONIZATION_BARRIER lpBarrier, 
	DWORD dwFlags
)
{
	LONG remainingThreads;
	HANDLE hCurrentEvent;
	HANDLE hDormantEvent;

	if (!lpBarrier)
		return FALSE;

	/**
	 * dwFlags according to https://msdn.microsoft.com/en-us/library/windows/desktop/hh706889(v=vs.85).aspx
	 *
	 * SYNCHRONIZATION_BARRIER_FLAGS_BLOCK_ONLY (0x01)
	 * Specifies that the thread entering the barrier should block
	 * immediately until the last thread enters the barrier.
	 *
	 * SYNCHRONIZATION_BARRIER_FLAGS_SPIN_ONLY (0x02)
	 * Specifies that the thread entering the barrier should spin until the
	 * last thread enters the barrier, even if the spinning thread exceeds
	 * the barrier's maximum spin count.
	 *
	 * SYNCHRONIZATION_BARRIER_FLAGS_NO_DELETE (0x04)
	 * Specifies that the function can skip the work required to ensure
	 * that it is safe to delete the barrier, which can improve
	 * performance. All threads that enter this barrier must specify the
	 * flag; otherwise, the flag is ignored. This flag should be used only
	 * if the barrier will never be deleted.
	 */

	hCurrentEvent = (HANDLE)lpBarrier->Reserved3[0];
	hDormantEvent = (HANDLE)lpBarrier->Reserved3[1];

	remainingThreads = InterlockedDecrement((LONG*)&lpBarrier->Reserved1);

	assert(remainingThreads >= 0);

	if (remainingThreads > 0)
	{
		DWORD dwProcessors = lpBarrier->Reserved4;
		BOOL spinOnly = dwFlags & SYNCHRONIZATION_BARRIER_FLAGS_SPIN_ONLY;
		BOOL blockOnly = dwFlags & SYNCHRONIZATION_BARRIER_FLAGS_BLOCK_ONLY;
		BOOL block = TRUE;

		/**
		 * If SYNCHRONIZATION_BARRIER_FLAGS_SPIN_ONLY is set we will
		 * always spin and trust that the user knows what he/she/it
		 * is doing. Otherwise we'll only spin if the flag
		 * SYNCHRONIZATION_BARRIER_FLAGS_BLOCK_ONLY is not set and
		 * the number of remaining threads is less than the number
		 * of processors.
		 */

		if (spinOnly || (remainingThreads < dwProcessors && !blockOnly))
		{
			DWORD dwSpinCount = lpBarrier->Reserved5;
			DWORD sp = 0;
			/**
			 * nb: we must let the compiler know that our comparand
			 * can change between the iterations in the loop below
			 */
			volatile ULONG_PTR* cmp = &lpBarrier->Reserved3[0];
			/* we spin until the last thread _completed_ the event switch */
			while ((block = (*cmp == (ULONG_PTR)hCurrentEvent)))
				if (!spinOnly && ++sp > dwSpinCount)
					break;
		}

		if (block)
			WaitForSingleObject(hCurrentEvent, INFINITE);

		return FALSE;
	}

	/* reset the dormant event first */
	ResetEvent(hDormantEvent);

	/* reset the remaining counter */
	lpBarrier->Reserved1 = lpBarrier->Reserved2;

	/* switch events - this will also unblock the spinning threads */
	lpBarrier->Reserved3[1] = (ULONG_PTR)hCurrentEvent;
	lpBarrier->Reserved3[0] = (ULONG_PTR)hDormantEvent;

	/* signal the blocked threads */
	SetEvent(hCurrentEvent);

	return TRUE;
}

BOOL 
WINAPI 
DeleteSynchronizationBarrier(
	LPSYNCHRONIZATION_BARRIER lpBarrier
)
{
	/**
	 * According to https://msdn.microsoft.com/en-us/library/windows/desktop/hh706887(v=vs.85).aspx
	 * Return value:
	 * The DeleteSynchronizationBarrier function always returns TRUE.
	 */

	if (!lpBarrier)
		return TRUE;

	while (lpBarrier->Reserved1 != lpBarrier->Reserved2)
		SwitchToThread();

	if (lpBarrier->Reserved3[0])
		CloseHandle((HANDLE)lpBarrier->Reserved3[0]);

	if (lpBarrier->Reserved3[1])
		CloseHandle((HANDLE)lpBarrier->Reserved3[1]);

	ZeroMemory(lpBarrier, sizeof(SYNCHRONIZATION_BARRIER));

	return TRUE;
}

/***********************************************************************
 *	GetOverlappedResultEx   (kernelex.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH GetOverlappedResultEx( HANDLE file, OVERLAPPED *overlapped,
                                                     DWORD *result, DWORD timeout, BOOL alertable )
{
    NTSTATUS status;
    DWORD ret;

    TRACE( "(%p %p %p %u %d)\n", file, overlapped, result, timeout, alertable );

    status = overlapped->Internal;
    if (status == STATUS_PENDING)
    {
        if (!timeout)
        {
            SetLastError( ERROR_IO_INCOMPLETE );
            return FALSE;
        }
        ret = WaitForSingleObjectEx( overlapped->hEvent ? overlapped->hEvent : file, timeout, alertable );
        if (ret == WAIT_FAILED)
            return FALSE;
        else if (ret)
        {
            SetLastError( ret );
            return FALSE;
        }

        status = overlapped->Internal;
        if (status == STATUS_PENDING) status = STATUS_SUCCESS;
    }

    *result = overlapped->InternalHigh;
	
	if(!NT_SUCCESS(status))
	{
		BaseSetLastNTError(status);
        return FALSE;
	}else{
		return TRUE;
	}
}

static 
BOOL getQueuedCompletionStatus(
	HANDLE CompletionPort,
	LPOVERLAPPED_ENTRY lpEnt,
	DWORD dwMilliseconds
) {
	return GetQueuedCompletionStatus(CompletionPort, 
		&lpEnt->dwNumberOfBytesTransferred,
		&lpEnt->lpCompletionKey,
		&lpEnt->lpOverlapped, dwMilliseconds);
}

BOOL 
WINAPI 
GetQueuedCompletionStatusEx(
  HANDLE             CompletionPort,
  LPOVERLAPPED_ENTRY lpCompletionPortEntries,
  ULONG              ulCount,
  PULONG             ulNumEntriesRemoved,
  DWORD              dwMilliseconds,
  BOOL               fAlertable
) 
{
	int i = 0;
	LPOVERLAPPED_ENTRY currentEntry;
    NTSTATUS status;
    DWORD ret;	
    LARGE_INTEGER TimeOut;
    PLARGE_INTEGER pTimeOut;
	
	pTimeOut = BaseFormatTimeOut(&TimeOut, dwMilliseconds);	
	
	// validate arguments
	if(!lpCompletionPortEntries
	|| !ulCount || !ulNumEntriesRemoved) {
		RtlSetLastWin32Error(ERROR_INVALID_PARAMETER);
		return FALSE; 
	}	

		//DbgPrint("GetQueuedCompletionStatusEx: fAlertable");
		
	// retrieve multiple entries
	for(i = 0;i < ulCount; i++)
	{	
		currentEntry = lpCompletionPortEntries+i;
		status = currentEntry->Internal;
		if (status == STATUS_PENDING)
		{
			if (!dwMilliseconds)
			{
				SetLastError( ERROR_IO_INCOMPLETE );
				return FALSE;
			}
			ret = WaitForSingleObjectEx( currentEntry->lpOverlapped->hEvent ? currentEntry->lpOverlapped->hEvent : CompletionPort, dwMilliseconds, fAlertable );
			if (ret == WAIT_FAILED)
				return FALSE;
			else if (ret)
			{
				SetLastError( ret );
				return FALSE;
			}

			status = currentEntry->Internal;
			//if (status == STATUS_PENDING) status = STATUS_SUCCESS;
			if (status != WAIT_OBJECT_0) break;	
		}	
		if(!getQueuedCompletionStatus(CompletionPort, 
		currentEntry, dwMilliseconds)) break;
		dwMilliseconds = 0;
	}

	*ulNumEntriesRemoved = i;

	return TRUE;
}

/******************************************************************************
 *           WaitForDebugEventEx   (kernelex.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH WaitForDebugEventEx( DEBUG_EVENT *event, DWORD timeout )
{
    NTSTATUS status;
    LARGE_INTEGER time;
    DBGUI_WAIT_STATE_CHANGE state;

    for (;;)
    {
        status = DbgUiWaitStateChange( &state, get_nt_timeout( &time, timeout ) );
        switch (status)
        {
        case STATUS_SUCCESS:
            DbgUiConvertStateChangeStructure( &state, event );
            return TRUE;
        case STATUS_USER_APC:
            continue;
        case STATUS_TIMEOUT:
            SetLastError( ERROR_SEM_TIMEOUT );
            return FALSE;
        default:
            return set_ntstatus( status );
        }
    }
}	

/***********************************************************************
 *           WaitOnAddress   (kernelex.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH WaitOnAddress(
  volatile VOID *Address,
  PVOID         CompareAddress,
  SIZE_T        AddressSize,
  DWORD         dwMilliseconds
)
{
  LARGE_INTEGER timeout; 
  NTSTATUS Status; 
  BOOL result;

  BaseFormatTimeOut(&timeout, dwMilliseconds);
  Status = RtlWaitOnAddress((const void*)Address, CompareAddress, AddressSize, &timeout);
  BaseSetLastNTError(Status);
  result = FALSE;
  if ( NT_SUCCESS( Status) )
    result = Status != 0x102;
  return result;
}

static inline INT HashAddress(
	IN	LPVOID	lpAddr)
{
	return (((ULONG_PTR) lpAddr) >> 4) % ARRAYSIZE(WaitOnAddressHashTable);
}

// Return TRUE if memory is the same. FALSE if memory is different.
#pragma warning(disable:4715) // not all control paths return a value
static inline BOOL CompareVolatileMemory(
	IN	const volatile LPVOID A1,
	IN	const LPVOID A2,
	IN	SIZE_T size)
{
	ASSERT(size == 1 || size == 2 || size == 4 || size == 8);

	switch (size) {
	case 1:		return (*(const LPBYTE)A1 == *(const LPBYTE)A2);
	case 2:		return (*(const LPWORD)A1 == *(const LPWORD)A2);
	case 4:		return (*(const LPDWORD)A1 == *(const LPDWORD)A2);
	case 8:		return (*(const LPQWORD)A1 == *(const LPQWORD)A2);
	}
}
#pragma warning(default:4715)

static LPACVAHASHTABLEADDRESSLISTENTRY FindACVAListEntryForAddress(
	IN	LPACVAHASHTABLEENTRY	lpHashTableEntry,
	IN	LPVOID					lpAddr)
{
	LPACVAHASHTABLEADDRESSLISTENTRY lpListEntry;

	ForEachListEntry(&lpHashTableEntry->Addresses, lpListEntry) {
		if (lpListEntry->lpAddr == lpAddr) {
			return lpListEntry;
		}
	}

	return NULL;
}

static inline LPACVAHASHTABLEADDRESSLISTENTRY CreateACVAListEntryForAddress(
	IN	LPACVAHASHTABLEENTRY	lpHashTableEntry,
	IN	LPVOID					lpAddr)
{
	LPACVAHASHTABLEADDRESSLISTENTRY lpListEntry;
	lpListEntry = (LPACVAHASHTABLEADDRESSLISTENTRY) HeapAlloc(GetProcessHeap(), 0, sizeof(ACVAHASHTABLEADDRESSLISTENTRY));

	if (!lpListEntry) {
		return NULL;
	}

	lpListEntry->lpAddr = lpAddr;
	lpListEntry->dwWaiters = 0;
	RtlInitializeConditionVariable(&lpListEntry->CVar);
	InsertHeadList(&lpHashTableEntry->Addresses, (PLIST_ENTRY) lpListEntry);

	return lpListEntry;
}

static inline LPACVAHASHTABLEADDRESSLISTENTRY FindOrCreateACVAListEntryForAddress(
	IN	LPACVAHASHTABLEENTRY	lpHashTableEntry,
	IN	LPVOID					lpAddr)
{
	LPACVAHASHTABLEADDRESSLISTENTRY lpListEntry = FindACVAListEntryForAddress(lpHashTableEntry, lpAddr);

	if (!lpListEntry) {
		lpListEntry = CreateACVAListEntryForAddress(lpHashTableEntry, lpAddr);
	}

	return lpListEntry;
}

static inline VOID DeleteACVAListEntry(
	IN	LPACVAHASHTABLEADDRESSLISTENTRY	lpListEntry)
{
	RemoveEntryList((PLIST_ENTRY) lpListEntry);
	HeapFree(GetProcessHeap(), 0, lpListEntry);
}

// WINBASEAPI BOOL WINAPI WaitOnAddress(
	// IN	volatile LPVOID	lpAddr,					// address to wait on
	// IN	LPVOID			lpCompare,				// pointer to location of old value of lpAddr
	// IN	SIZE_T			cb,						// number of bytes to compare
	// IN	DWORD			dwMilliseconds OPTIONAL)// maximum number of milliseconds to wait
// {
	// LPACVAHASHTABLEENTRY lpHashTableEntry = &WaitOnAddressHashTable[HashAddress(lpAddr)];
	// LPACVAHASHTABLEADDRESSLISTENTRY lpListEntry;
	// DWORD dwLastError;
	// BOOL bSuccess;

	// DbgPrint("(%p, %p, %Iu, %I32u)", lpAddr, lpCompare, cb, dwMilliseconds);

	// if (!lpAddr || !lpCompare) {
		// SetLastError(ERROR_INVALID_PARAMETER);
		// return FALSE;
	// } else if (!(cb == 1 || cb == 2 || cb == 4 || cb == 8)) {
		// SetLastError(ERROR_INVALID_PARAMETER);
		// return FALSE;
	// }

	// EnterCriticalSection(&lpHashTableEntry->Lock);
	
	// if (!CompareVolatileMemory(lpAddr, lpCompare, cb)) {
		// LeaveCriticalSection(&lpHashTableEntry->Lock);
		// SetLastError(ERROR_SUCCESS);
		// return TRUE;
	// }

	// lpListEntry = FindOrCreateACVAListEntryForAddress(lpHashTableEntry, lpAddr);
	// lpListEntry->dwWaiters++;
	// bSuccess = SleepConditionVariableCS(&lpListEntry->CVar, &lpHashTableEntry->Lock, dwMilliseconds);
	// dwLastError = GetLastError();

	// if (--lpListEntry->dwWaiters == 0) {
		// DeleteACVAListEntry(lpListEntry);
	// }

	// LeaveCriticalSection(&lpHashTableEntry->Lock);
	// SetLastError(dwLastError);
	// return bSuccess;
// }

WINBASEAPI VOID WINAPI WakeByAddressSingle(
	IN	LPVOID	lpAddr)
{
	LPACVAHASHTABLEENTRY lpHashTableEntry = &WaitOnAddressHashTable[HashAddress(lpAddr)];
	LPACVAHASHTABLEADDRESSLISTENTRY lpListEntry;
	
	DbgPrint("(%p)", lpAddr);

	EnterCriticalSection(&lpHashTableEntry->Lock);
	lpListEntry = FindACVAListEntryForAddress(lpHashTableEntry, lpAddr);

	if (lpListEntry) {
		RtlWakeConditionVariable(&lpListEntry->CVar);
	}

	LeaveCriticalSection(&lpHashTableEntry->Lock);
}

WINBASEAPI VOID WINAPI WakeByAddressAll(
	IN	LPVOID	lpAddr)
{
	LPACVAHASHTABLEENTRY lpHashTableEntry = &WaitOnAddressHashTable[HashAddress(lpAddr)];
	LPACVAHASHTABLEADDRESSLISTENTRY lpListEntry;

	DbgPrint("(%p)", lpAddr);
	
	EnterCriticalSection(&lpHashTableEntry->Lock);
	lpListEntry = FindACVAListEntryForAddress(lpHashTableEntry, lpAddr);

	if (lpListEntry) {
		RtlWakeAllConditionVariable(&lpListEntry->CVar);
	}

	LeaveCriticalSection(&lpHashTableEntry->Lock);
}