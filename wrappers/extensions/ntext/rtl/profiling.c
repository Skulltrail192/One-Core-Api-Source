/*++

Copyright (c) 2025 Shorthorn Project

Module Name:

    profiling.c

Abstract:

    This module implements RTL Profiling APIs

Author:

    Skulltrail 23-Jaunary-2025

Revision History:

--*/
 
#define NDEBUG

#include <main.h> 

NTSTATUS 
NTAPI 
RtlDisableThreadProfiling(
	HANDLE PerformanceDataHandle
)
{
	DbgPrint("UNIMPLEMENTED: RtlDisableThreadProfiling");	
	return STATUS_SUCCESS;
}

NTSTATUS 
NTAPI 
RtlEnableThreadProfiling(
	HANDLE ThreadHandle, 
	DWORD Flags, 
	DWORD64 HardwareCounters, 
	HANDLE PerformanceDataHandle
)
{
	DbgPrint("UNIMPLEMENTED: RtlEnableThreadProfiling");	
	return STATUS_SUCCESS;
}

NTSTATUS 
NTAPI 
RtlQueryThreadProfiling(
	HANDLE HANDLE, 
	PBOOLEAN Enabled
)
{
	DbgPrint("UNIMPLEMENTED: RtlQueryThreadProfiling");	
	return STATUS_SUCCESS;
}

NTSTATUS 
NTAPI 
RtlReadThreadProfilingData(
	HANDLE PerformanceDataHandle, 
	DWORD Flags, 
	PPERFORMANCE_DATA PerformanceData
)
{
	DbgPrint("UNIMPLEMENTED: RtlReadThreadProfilingData");	
	return STATUS_SUCCESS;
}