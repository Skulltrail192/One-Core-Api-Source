/*++

Copyright (c) 2025 Shorthorn Project

Module Name:

    ldrrsrc.c

Abstract:

    This module implements RTL Miscellaneous APIs

Author:

    Skulltrail 23-Jaunary-2025

Revision History:

--*/
 
#define NDEBUG

#include <main.h>

NTSTATUS 
NTAPI
RtlRegisterThreadWithCsrss()
{
	// NTSTATUS Status;
	// PCSR_API_MESSAGE ApiMessage;
	// if ( !CsrClientProcess && CsrInitOnceDone && CsrServerApiRoutine ){
		// ApiMessage->Header.ClientId = NtCurrentTeb()->ClientId;
	// }else{
		// Status = STATUS_SUCCESS;
	// }
	DbgPrint("UNIMPLEMENTED: RtlRegisterThreadWithCsrss");
	return ERROR_SUCCESS;	
}

NTSTATUS
NTAPI
RtlRemovePrivileges(
	HANDLE TokenHandle, 
	PULONG PrivilegesToKeep, 
	ULONG PrivilegeCount
)
{
	DbgPrint("UNIMPLEMENTED: RtlRemovePrivileges");
	return ERROR_SUCCESS;	
}

// Required for cabinet.dll Windows 8, which unlocks an useful, next-generation compression API used by programs.
// Why I think it is fine: The extra parameter "WorkSpace" is application-allocated. The non-Ex API allocates the size automatically and frees automatically. So, only very slight memory penalties exist. The application eventually frees the buffer when it finishes it's decompression operations, so no behavior changes.
NTSTATUS 
NTAPI 
RtlDecompressBufferEx(
	USHORT CompressionFormat, 
	PUCHAR UncompressedBuffer, 
	ULONG UncompressedBufferSize, 
	PUCHAR CompressedBuffer, 
	ULONG  CompressedBufferSize, 
	PULONG FinalUncompressedSize, 
	PVOID  WorkSpace
) {
    return RtlDecompressBuffer(CompressionFormat, UncompressedBuffer, UncompressedBufferSize, CompressedBuffer, CompressedBufferSize, FinalUncompressedSize);
}