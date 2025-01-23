/*++

Copyright (c) 2025 Shorthorn Project

Module Name:

    processor.c

Abstract:

    This module implements RTL Processor APIs

Author:

    Skulltrail 23-Jaunary-2025

Revision History:

--*/

#define NDEBUG

#include <main.h>

ULONG 
NTAPI 
RtlpGetCurrentProcessorNumber()
{
  return __segmentlimit(59u) >> 14;
}

VOID 
NTAPI 
RtlGetCurrentProcessorNumberEx(
  _Out_  PPROCESSOR_NUMBER ProcNumber
)
{
	ProcNumber->Group = 0;
	ProcNumber->Number = RtlpGetCurrentProcessorNumber();
	ProcNumber->Reserved = 0;
}