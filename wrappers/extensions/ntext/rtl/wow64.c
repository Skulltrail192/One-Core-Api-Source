/*++

Copyright (c) 2022 Shorthorn Project

Module Name:

    wow64.c

Abstract:

    Implement functions for Management of Wow64

Author:

    Skulltrail 31-July-2022

Revision History:

--*/
 
#define NDEBUG

#include <main.h>

//
// File system redirection values
//

#define WOW64_FILE_SYSTEM_ENABLE_REDIRECT          (UlongToPtr(0x00))   // enable file-system redirection for the currently executing thread
#define WOW64_FILE_SYSTEM_DISABLE_REDIRECT         (UlongToPtr(0x01))   // disable file-system redirection for the currently executing thread

static UNICODE_STRING NtDllName = RTL_CONSTANT_STRING(L"ntdll.dll");
static ANSI_STRING RtlWow64EnableFsRedirectionProcName = RTL_CONSTANT_STRING("RtlWow64EnableFsRedirection");
static ANSI_STRING RtlWow64EnableFsRedirectionExProcName = RTL_CONSTANT_STRING("RtlWow64EnableFsRedirectionEx");

NTSTATUS (NTAPI *RtlpWow64EnableFsRedirection)(IN BOOLEAN  Wow64FsEnableRedirection);
NTSTATUS (NTAPI *RtlpWow64EnableFsRedirectionEx)(IN PVOID Wow64FsEnableRedirection, OUT PVOID * OldFsRedirectionLevel);

inline
PVOID 
Wow64SetFilesystemRedirectorEx (
    IN PVOID NewValue
    )
{
    PVOID OldValue;
    OldValue = (PVOID)(ULONG_PTR)NtCurrentTeb()->TlsSlots[WOW64_TLS_FILESYSREDIR];
    NtCurrentTeb()->TlsSlots[WOW64_TLS_FILESYSREDIR] = NewValue;//(ULONGLONG)PtrToUlong(NewValue);
    return OldValue;
}

BOOL
NTAPI
RtlIsWow64Process(    
	HANDLE hProcess,
    PBOOL Wow64Process)
{
	NTSTATUS NtStatus;
    ULONG_PTR Peb32;

    NtStatus = NtQueryInformationProcess (
        hProcess,
        ProcessWow64Information,
        &Peb32,
        sizeof (Peb32),
        NULL
        );

    if (!NT_SUCCESS (NtStatus)) {

        return STATUS_UNSUCCESSFUL;
    } else {

        if (Peb32 == 0) {
            *Wow64Process = FALSE;
        } else {
            *Wow64Process = TRUE;
        }
    }

    return NtStatus;
}

NTSYSAPI
NTSTATUS 
NTAPI 
RtlWow64EnableFsRedirectionEx(
	IN PVOID Wow64FsEnableRedirection,
	OUT PVOID * OldFsRedirectionLevel 
) 		
{
#ifndef _M_AMD64
	BOOL isWow64;
	RtlIsWow64Process(NtCurrentProcess(),&isWow64);
	
	if(isWow64){		
		NTSTATUS st;
		PVOID Wow64Handle;
		
		st = LdrLoadDll(NULL, NULL, &NtDllName, &Wow64Handle);
		
		if (!NT_SUCCESS(st)) {
		   DbgPrint("RtlWow64EnableFsRedirectionEx: Ntdll.dll not found.  Status=%x\n", st);
		   return st;
		}	
		
		st = LdrGetProcedureAddress (Wow64Handle,
									&RtlWow64EnableFsRedirectionExProcName,
									 0,
									(PVOID*)&RtlpWow64EnableFsRedirectionEx);
									
		if (!NT_SUCCESS(st)) {
		   DbgPrint("RtlWow64EnableFsRedirectionEx: RtlWow64EnableFsRedirectionEx function not found.  Status=%x\n", st);
		   return st;
		}

		return (*RtlpWow64EnableFsRedirectionEx)(Wow64FsEnableRedirection, OldFsRedirectionLevel);
	}else{
		return STATUS_NOT_IMPLEMENTED;	
	}
#else
	return STATUS_NOT_IMPLEMENTED;	
#endif
}

NTSYSAPI
NTSTATUS 
NTAPI 
RtlWow64EnableFsRedirection(
	IN BOOLEAN  Wow64FsEnableRedirection
) 	
{
#ifndef _M_AMD64	
	BOOL isWow64;
	RtlIsWow64Process(NtCurrentProcess(),&isWow64);
	
	if(isWow64){
		NTSTATUS st;
		PVOID Wow64Handle;
		
		st = LdrLoadDll(NULL, NULL, &NtDllName, &Wow64Handle);
		
		if (!NT_SUCCESS(st)) {
		   DbgPrint("RtlWow64EnableFsRedirection: Ntdll.dll not found.  Status=%x\n", st);
		   return st;
		}	
		
		st = LdrGetProcedureAddress (Wow64Handle,
									&RtlWow64EnableFsRedirectionProcName,
									 0,
									(PVOID*)&RtlpWow64EnableFsRedirection);
									
		if (!NT_SUCCESS(st)) {
		   DbgPrint("RtlWow64EnableFsRedirection: RtlWow64EnableFsRedirection function not found.  Status=%x\n", st);
		   return st;
		}	

		return (*RtlpWow64EnableFsRedirection)(Wow64FsEnableRedirection);
	}else{
		return STATUS_NOT_IMPLEMENTED;
	}	
#else
	return STATUS_NOT_IMPLEMENTED;	
#endif
}