/*++

Copyright (c) 2022 Shorthorn Project

Module Name:

    xstate.c

Abstract:

    This module implements Context of Xstate

Author:

    Skulltrail 26-September-2022

Revision History:

--*/
 
#define NDEBUG

#include <main.h> 

VOID
NTAPI
RtlSetExtendedFeaturesMask (
    __out PCONTEXT ContextEx,
    __in DWORD64 FeatureMask
)
{
	DbgPrint("UNIMPLEMENTED: RtlSetExtendedFeaturesMask");
	;
}

DWORD   
NTAPI
RtlInitializeExtendedContext (
    __out PVOID Context,
    __in DWORD ContextFlags,
    __out PCONTEXT* ContextEx
)
{
	DbgPrint("UNIMPLEMENTED: RtlInitializeExtendedContext");
	return 0;
}

NTSTATUS 
NTAPI 
RtlCopyContext(
	PCONTEXT Destination, 
	DWORD ContextFlags, 
	PCONTEXT Source
)
{
	DbgPrint("UNIMPLEMENTED: RtlCopyContext");	
	return STATUS_SUCCESS;
}

PCONTEXT 
NTAPI 
RtlLocateLegacyContext(
	PCONTEXT oldContext, 
	BOOL other
)
{
	DbgPrint("UNIMPLEMENTED: RtlLocateLegacyContext");	
	return oldContext;
}

NTSTATUS 
NTAPI 
RtlGetExtendedContextLength(
	DWORD flags, 
	LPDWORD ContextFlags)
{
	DbgPrint("UNIMPLEMENTED: RtlGetExtendedContextLength");	
	return STATUS_SUCCESS;
}