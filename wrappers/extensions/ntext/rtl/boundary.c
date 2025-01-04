/*++

Copyright (c) 2018 Shorthorn Project

Module Name:

    boundary.c

Abstract:

    Implement Boundary functions

Author:

    Skulltrail 18-March-2018

Revision History:

--*/ 
 
#define NDEBUG

#include <main.h>

NTSTATUS 
NTAPI
RtlAddIntegrityLabelToBoundaryDescriptor(
	HANDLE *BoundaryDescriptor, 
	PSID IntegrityLabel
)
{
	DbgPrint("UNIMPLEMENTED: RtlAddIntegrityLabelToBoundaryDescriptor\n");
	return STATUS_SUCCESS;
}

NTSTATUS 
NTAPI
RtlAddSIDToBoundaryDescriptor(
  _Inout_  HANDLE *BoundaryDescriptor,
  _In_     PSID RequiredSid
)
{
	DbgPrint("UNIMPLEMENTED: RtlAddSIDToBoundaryDescriptor");
	return STATUS_SUCCESS;
}

HANDLE 
NTAPI
RtlCreateBoundaryDescriptor(
	LSA_UNICODE_STRING *string, 
	ULONG Flags
)
{
	return NULL;
}

VOID 
WINAPI 
RtlDeleteBoundaryDescriptor(
  _In_ HANDLE BoundaryDescriptor
)
{
	RtlFreeHeap(NtCurrentTeb()->ProcessEnvironmentBlock->ProcessHeap, 8, BoundaryDescriptor);
}
