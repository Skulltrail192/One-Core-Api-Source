/*++

Copyright (c) 2021 Shorthorn Project

Module Name:

    exception.c

Abstract:

    Excepetion handling for user mode.

Author:

    Skulltrail 25-June-2021

Revision History:

--*/
   
#define NDEBUG

#include <main.h>

PRTLP_UNHANDLED_EXCEPTION_FILTER RtlpUnhandledExceptionFilter; 

NTSYSAPI
VOID 
NTAPI 
RtlSetUnhandledExceptionFilter(
	IN PRTLP_UNHANDLED_EXCEPTION_FILTER  	TopLevelExceptionFilter	
) 	
{
    /* Set the filter which is used by the CriticalSection package */
    RtlpUnhandledExceptionFilter = RtlEncodePointer(TopLevelExceptionFilter);
}

// NTSYSAPI 
// NTSTATUS 
// NTAPI 
// RtlReportException( 	_In_ PEXCEPTION_RECORD  	ExceptionRecord,
		// _In_ PCONTEXT  	ContextRecord,
		// _In_ ULONG  	Flags 
// )
// {
	// return RtlDispatchException(ExceptionRecord, ContextRecord);
// } 	