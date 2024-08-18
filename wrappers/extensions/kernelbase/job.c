/*++

Copyright (c) 2024  Shorthorn Project

Module Name:

    job.c

Abstract:

    Support for the Job Object

Author:

    Skulltrail 15-August-2024

Revision History:

--*/

#include "main.h"

/*
 * @implemented
 */
BOOL
WINAPI
IsProcessInJobInternal(IN HANDLE ProcessHandle,
               IN HANDLE JobHandle,
               OUT PBOOL Result)
{
    NTSTATUS Status;

    Status = NtIsProcessInJob(ProcessHandle, JobHandle);
    if (NT_SUCCESS(Status))
    {
        *Result = (Status == STATUS_PROCESS_IN_JOB);
        return TRUE;
    }

    BaseSetLastNTError(Status);
    return FALSE;
}

/*
 * @implemented
 */
BOOL
WINAPI
AssignProcessToJobObjectInternal(IN HANDLE hJob,
                         IN HANDLE hProcess)
{
    NTSTATUS Status;

    Status = NtAssignProcessToJobObject(hJob, hProcess);
    if (!NT_SUCCESS(Status))
    {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    return TRUE;
}

BOOL 
WINAPI 
IsProcessInJob(HANDLE ProcessHandle, HANDLE JobHandle, PBOOL Result) {
    if (JobHandle == NULL && Result != NULL && ProcessHandle == NtCurrentProcess()) {
        //DbgPrint("IsProcessInJob_Internal :: VxKex method Triggered. Remove when not needed.");
        *Result = TRUE;
        return TRUE;
    }
    return IsProcessInJobInternal(ProcessHandle, JobHandle, Result);
}

BOOL 
WINAPI 
AssignProcessToJobObject(HANDLE hJob, HANDLE hProcess) {
    BOOL Result = AssignProcessToJobObjectInternal(hJob, hProcess);
    if (Result == FALSE && GetLastError() == ERROR_ACCESS_DENIED) {
        //DbgPrint("AssignProjectToJobObject_Internal :: Detected the use of Nested Job Objects. Lie about it.");
        Result = TRUE;
        SetLastError(0);
    }
    return Result;
}