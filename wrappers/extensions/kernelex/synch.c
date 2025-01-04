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
 *           InitializeCriticalSectionEx   (kernelbase.@)
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
    return CreateEventW(lpEventAttributes,
                        (dwFlags & CREATE_EVENT_MANUAL_RESET) != 0,
                        (dwFlags & CREATE_EVENT_INITIAL_SET) != 0,
                        lpName);
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
	return CreateSemaphoreW(sa,
						    initial,
						    max,
						    name);
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
    // forward to CreateMutexW.
    return CreateMutexW(lpMutexAttributes, (dwFlags & CREATE_MUTEX_INITIAL_OWNER) != 0, lpName);
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
 *	GetOverlappedResultEx   (kernelbase.@)
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

void ConvertOverlappedToEntry(ULONG_PTR lpCompletionKey, LPOVERLAPPED src, LPOVERLAPPED_ENTRY dst) {
    dst->lpOverlapped = src;
    dst->dwNumberOfBytesTransferred = src->InternalHigh;
    dst->lpCompletionKey = lpCompletionKey;
    dst->Internal = 0; // we can use the 'Internal' for OCA purposes, but right now we don't do anything.
}

//Generated by ChatGPT. Thank you AI
BOOL WINAPI GetQueuedCompletionStatusEx(
    HANDLE hCompletionPort,
    LPOVERLAPPED_ENTRY lpCompletionPortEntries,
    ULONG ulCount,
    PULONG ulNumEntriesRemoved,
    DWORD dwMilliseconds,
    BOOL bAlertable
) {
    DWORD dwBytesTransferred;
    ULONG_PTR lpCompletionKey;
    LPOVERLAPPED lpOverlapped;
	ULONG i;
	BOOL _bRet;
	DWORD _uStartTick;
	DWORD _uResult;
	DWORD _uTickSpan;
	//OVERLAPPED_ENTRY _Entry;
	DWORD kMaxSleepTime;
	
    if (!lpCompletionPortEntries || ulCount == 0 || !ulNumEntriesRemoved) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    *ulNumEntriesRemoved = 0;
	
	//_Entry = lpCompletionPortEntries[0];
	
        if (bAlertable)
        {
            kMaxSleepTime = 10;
            // 使用 SleepEx 进行等待触发 APC
            if (dwMilliseconds == INFINITE)
            {
                for (;;)
                {
					_bRet = GetQueuedCompletionStatus(
							hCompletionPort,
							&dwBytesTransferred,
							&lpCompletionKey,
							&lpOverlapped,
							0);
							
                    if (_bRet)
                    {
						lpCompletionPortEntries[*ulNumEntriesRemoved].lpCompletionKey = lpCompletionKey;
						lpCompletionPortEntries[*ulNumEntriesRemoved].lpOverlapped = lpOverlapped;
						lpCompletionPortEntries[*ulNumEntriesRemoved].dwNumberOfBytesTransferred = dwBytesTransferred;
						(*ulNumEntriesRemoved)++;						
                        break;
                    }

                    if (GetLastError() != WAIT_TIMEOUT)
                    {
                        return FALSE;
                    }

                    if (SleepEx(kMaxSleepTime, TRUE) == WAIT_IO_COMPLETION)
                    {
                        SetLastError(WAIT_IO_COMPLETION);
                        return FALSE;
                    }
                }
            }
			else
			{				
				// 使用 WaitForSingleObjectEx 进行等待触发 APC
				_uStartTick = GetTickCount();
				for (;;)
				{
					_uResult = WaitForSingleObjectEx(hCompletionPort, dwMilliseconds, TRUE);
					if (_uResult == WAIT_OBJECT_0)
					{
						_bRet = GetQueuedCompletionStatus(
							hCompletionPort,
							&dwBytesTransferred,
							&lpCompletionKey,
							&lpOverlapped,
							dwMilliseconds
						);					
						// 完成端口有数据了
						// _bRet = GetQueuedCompletionStatus(CompletionPort, &_Entry.dwNumberOfBytesTransferred, &_Entry.lpCompletionKey, &_Entry.lpOverlapped, 0);
						// if (_bRet)
						// {
							// *ulNumEntriesRemoved = 1;
							// break;
						// }
						if (_bRet || lpOverlapped) {
							lpCompletionPortEntries[*ulNumEntriesRemoved].lpCompletionKey = lpCompletionKey;
							lpCompletionPortEntries[*ulNumEntriesRemoved].lpOverlapped = lpOverlapped;
							lpCompletionPortEntries[*ulNumEntriesRemoved].dwNumberOfBytesTransferred = dwBytesTransferred;
							(*ulNumEntriesRemoved)++;

							// Continue collecting if we haven't hit the desired count yet
							if (*ulNumEntriesRemoved < ulCount) {
								dwMilliseconds = 0; // Set timeout to zero after the first iteration
								continue;
							}
						}					

						if (GetLastError() != WAIT_TIMEOUT)
						{
							return FALSE;
						}

						// 无限等待时无脑继续等即可。
						if (dwMilliseconds == INFINITE)
						{
							continue;
						}

						// 计算剩余等待时间，如果剩余等待时间归零则返回
						_uTickSpan = GetTickCount() - _uStartTick;
						if (_uTickSpan >= dwMilliseconds)
						{
							SetLastError(WAIT_TIMEOUT);
							return FALSE;
						}
						dwMilliseconds -= _uTickSpan;
						_uStartTick += _uTickSpan;
						continue;
					}
					else if (_uResult == WAIT_IO_COMPLETION || _uResult == WAIT_TIMEOUT)
					{
						// 很奇怪，微软原版遇到 APC唤醒直接会设置 LastError WAIT_IO_COMPLETION
						// 遇到超时，LastError WAIT_TIMEOUT（注意不是预期的 ERROR_TIMEOUT）不知道是故意还是有意。
						SetLastError(_uResult);
						return FALSE;
					}
					else if (_uResult == WAIT_ABANDONED)
					{
						SetLastError(ERROR_ABANDONED_WAIT_0);
						return FALSE;
					}
					else if (_uResult == WAIT_FAILED)
					{
						// LastError
						return FALSE;
					}
					else
					{
						// LastError ???
						return FALSE;
					}
				}
			}

            return TRUE;
        }else{
			for (i = 0; i < ulCount; i++) {
				_bRet = GetQueuedCompletionStatus(
					hCompletionPort,
					&dwBytesTransferred,
					&lpCompletionKey,
					&lpOverlapped,
					dwMilliseconds
				);

				if (_bRet || lpOverlapped) {
					lpCompletionPortEntries[*ulNumEntriesRemoved].lpCompletionKey = lpCompletionKey;
					lpCompletionPortEntries[*ulNumEntriesRemoved].lpOverlapped = lpOverlapped;
					lpCompletionPortEntries[*ulNumEntriesRemoved].dwNumberOfBytesTransferred = dwBytesTransferred;
					(*ulNumEntriesRemoved)++;

					// Continue collecting if we haven't hit the desired count yet
					if (*ulNumEntriesRemoved < ulCount) {
						dwMilliseconds = 0; // Set timeout to zero after the first iteration
						continue;
					}
					return TRUE; // All requested entries were retrieved
				} else {
					// Error occurred or timeout
					if (*ulNumEntriesRemoved > 0) {
						return TRUE; // Partial success
					}
					return FALSE; // Complete failure
				}
			}			
		}

    return TRUE;
}

// BOOL WINAPI GetQueuedCompletionStatusEx(
    // HANDLE hCompletionPort,
    // LPOVERLAPPED_ENTRY lpCompletionPortEntries,
    // ULONG ulCount,
    // PULONG ulNumEntriesRemoved,
    // DWORD dwMilliseconds,
    // BOOL bAlertable // This parameter is not present in Longhorn 4074 -> BOOM CRASH. I hope no pre reset targeted applications use this function.
// ) {
    // DWORD dwBytesTransferred;
    // ULONG_PTR lpCompletionKey;
    // LPOVERLAPPED lpOverlapped;
	// BOOL result;
    
    // if (bAlertable) 
        // DbgPrint("GetQueuedCompletionStatusEx: Called as alertable!\n");

    // if (!lpCompletionPortEntries || ulCount == 0 || !ulNumEntriesRemoved) {
        // SetLastError(ERROR_INVALID_PARAMETER);
        // return FALSE;
    // }
    
    // result = GetQueuedCompletionStatus(
        // hCompletionPort,
        // &dwBytesTransferred,
        // &lpCompletionKey,
        // &lpOverlapped,
        // dwMilliseconds
    // );
    // if (!result || !lpOverlapped) {
        // *ulNumEntriesRemoved = 0;
        // return FALSE; // All requested entries were retrieved
    // }
    // lpCompletionPortEntries[0].lpCompletionKey = lpCompletionKey;
    // lpCompletionPortEntries[0].lpOverlapped = lpOverlapped;
    // lpCompletionPortEntries[0].dwNumberOfBytesTransferred = dwBytesTransferred;
    // *ulNumEntriesRemoved = 1;
    // return TRUE;
// }

// BOOL 
// WINAPI 
// GetQueuedCompletionStatusEx(
  // HANDLE             CompletionPort,
  // LPOVERLAPPED_ENTRY lpCompletionPortEntries,
  // ULONG              ulCount,
  // PULONG             ulNumEntriesRemoved,
  // DWORD              dwMilliseconds,
  // BOOL               fAlertable
// ) 
// {
    // NTSTATUS Status;
    // IO_STATUS_BLOCK IoStatus;
    // ULONG_PTR CompletionKey;
    // LARGE_INTEGER Time;
    // PLARGE_INTEGER TimePtr;
	// OVERLAPPED_ENTRY lpCompletionPortEntriesNew;
	// OVERLAPPED_ENTRY lpCompletionPortEntry;
    // int i;

	// for(i=0;i<ulCount;i++){
		// /* Convert the timeout and then call the native API */
		// TimePtr = BaseFormatTimeOut(&Time, dwMilliseconds);
		// Status = NtRemoveIoCompletion(CompletionPort,
									  // (PVOID*)&lpCompletionPortEntries->lpCompletionKey,
									  // (PVOID*)&lpCompletionPortEntries->lpOverlapped,
									  // &IoStatus,
									  // TimePtr);
		// if (!(NT_SUCCESS(Status)) || (Status == STATUS_TIMEOUT))
		// {
			// /* Clear out the overlapped output */
			// *lpOverlapped = NULL;

			// /* Check what kind of error we got */
			// if (Status == STATUS_TIMEOUT)
			// {
				// /* Timeout error is set directly since there's no conversion */
				// SetLastError(WAIT_TIMEOUT);
			// }
			// else
			// {
				// /* Any other error gets converted */
				// BaseSetLastNTError(Status);
			// }

			// /* This is a failure case */
			// return FALSE;
		// }
		
		// ConvertOverlappedToEntry(lpCompletionPortEntries->lpCompletionKey, &lpCompletionPortEntries->lpOverlapped, &lpCompletionPortEntry);

		// /* Check for error */
		// if (!NT_SUCCESS(IoStatus.Status))
		// {
			// /* Convert and fail */
			// BaseSetLastNTError(IoStatus.Status);
			// return FALSE;
		// }
		// lpCompletionPortEntries++;
	// }

    // /* Return success */
    // return TRUE;	
	
	
	// int i = 0;
	// LPOVERLAPPED_ENTRY currentEntry;
    // NTSTATUS status;
    // DWORD ret;	
    // LARGE_INTEGER TimeOut;
    // PLARGE_INTEGER pTimeOut;
	
	// pTimeOut = BaseFormatTimeOut(&TimeOut, dwMilliseconds);	
	
	// // validate arguments
	// if(!lpCompletionPortEntries
	// || !ulCount || !ulNumEntriesRemoved) {
		// RtlSetLastWin32Error(ERROR_INVALID_PARAMETER);
		// return FALSE; 
	// }	

		// //DbgPrint("GetQueuedCompletionStatusEx: fAlertable");
		
	// // retrieve multiple entries
	// for(i = 0;i < ulCount; i++)
	// {	
		// currentEntry = lpCompletionPortEntries+i;
		// status = currentEntry->Internal;
		// if (status == STATUS_PENDING)
		// {
			// if (!dwMilliseconds)
			// {
				// SetLastError( ERROR_IO_INCOMPLETE );
				// return FALSE;
			// }
			// ret = WaitForSingleObjectEx( currentEntry->lpOverlapped->hEvent ? currentEntry->lpOverlapped->hEvent : CompletionPort, dwMilliseconds, fAlertable );
			// if (ret == WAIT_FAILED)
				// return FALSE;
			// else if (ret)
			// {
				// SetLastError( ret );
				// return FALSE;
			// }

			// status = currentEntry->Internal;
			// //if (status == STATUS_PENDING) status = STATUS_SUCCESS;
			// if (status != WAIT_OBJECT_0) break;	
		// }	
		// if(!getQueuedCompletionStatus(CompletionPort, 
		// currentEntry, dwMilliseconds)) break;
		// dwMilliseconds = 0;
	// }

	// *ulNumEntriesRemoved = i;

	// return TRUE;
//}


/******************************************************************************
 *           WaitForDebugEventEx   (kernelbase.@)
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

// /***********************************************************************
 // *           WaitOnAddress   (kernelbase.@)
 // */
// BOOL WINAPI DECLSPEC_HOTPATCH WaitOnAddress(
  // volatile VOID *Address,
  // PVOID         CompareAddress,
  // SIZE_T        AddressSize,
  // DWORD         dwMilliseconds
// )
// {
  // LARGE_INTEGER timeout; 
  // NTSTATUS Status; 
  // BOOL result;

  // BaseFormatTimeOut(&timeout, dwMilliseconds);
  // Status = RtlWaitOnAddress((const void*)Address, CompareAddress, AddressSize, &timeout);
  // BaseSetLastNTError(Status);
  // result = FALSE;
  // if ( NT_SUCCESS( Status) )
    // result = Status != 0x102;
  // return result;
// }

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

//
// This function is a wrapper around (Kex)RtlWaitOnAddress.
//
BOOL WINAPI WaitOnAddress(
    IN    volatile VOID    *Address,
    IN    PVOID            CompareAddress,
    IN    SIZE_T            AddressSize,
    IN    DWORD            Milliseconds OPTIONAL)
{
    NTSTATUS Status;
    PLARGE_INTEGER TimeOutPointer;
    LARGE_INTEGER TimeOut;

    TimeOutPointer = BaseFormatTimeOut(&TimeOut, Milliseconds);

    Status = RtlWaitOnAddress(
        Address,
        CompareAddress,
        AddressSize,
        TimeOutPointer);

    BaseSetLastNTError(Status);
    
    if (NT_SUCCESS(Status) && Status != STATUS_TIMEOUT) {
        return TRUE;
    } else {
        return FALSE;
    }
}