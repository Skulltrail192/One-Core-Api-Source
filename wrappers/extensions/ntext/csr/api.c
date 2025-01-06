/*++

Copyright (c) 2025 Shorthorn Project

Module Name:

    api.c

Abstract:

    CSR APIs exported through NTDLL

Author:

    Skulltrail 04-Jaunary-2025

Revision History:

--*/
 
#define NDEBUG

#include <main.h> 


/* GLOBALS ********************************************************************/

extern HANDLE CsrApiPort;

typedef NTSTATUS
(NTAPI *PCSR_NEW_THREAD_API_ROUTINE)(VOID);

PCSR_NEW_THREAD_API_ROUTINE CsrNewThreadApiRoutine;

/* FUNCTIONS ******************************************************************/

/*
 * @implemented
 */
NTSTATUS
NTAPI
CsrNewThread(VOID)
{
    UNICODE_STRING NtdllName;
	ANSI_STRING CsrNewThreadRoutineName;
	HANDLE hNtdll;
	NTSTATUS Status;
    /* We're inside, so let's find csrsrv */
    DbgPrint("Next-GEN CSRSS support\n");
	
    RtlInitUnicodeString(&NtdllName, L"ntdll");
    Status = LdrGetDllHandle(NULL,
                                 NULL,
                                 &NtdllName,
                                 &hNtdll);

    /* Now get the Server to Server routine */
    RtlInitAnsiString(&CsrNewThreadRoutineName, "CsrNewThread");
    Status = LdrGetProcedureAddress(hNtdll,
                                    &CsrNewThreadRoutineName,
                                    0L,
                                    (PVOID*)&CsrNewThreadApiRoutine);
									
	if(NT_SUCCESS(Status)){
		return CsrNewThreadApiRoutine();
	}								

    /* Register the termination port to CSR's */
    return NtRegisterThreadTerminatePort(/*CsrApiPort*/NULL);
}