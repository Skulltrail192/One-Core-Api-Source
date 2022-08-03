/*++

Copyright (c) 2022 Shorthorn Project

Module Name:

    wer.c

Abstract:

    Implement functions for Windows Error Reporting 

Author:

    Skulltrail 30-November-2015

Revision History:

--*/
  
#define NDEBUG

#include <main.h>

NTSTATUS globalStatus;

void 
NTAPI 
RtlReportErrorOrigination(LPSTR a1, WCHAR string, int a3, NTSTATUS status)
{
  if ( status == globalStatus )
    DbgBreakPoint();
  if ( status == STATUS_INTERNAL_ERROR )
  {
    DbgPrintEx(99, 0, "\n*** Assertion failed: %s\n***   Source File: %s, line %ld\n\n", string);
    RtlRaiseStatus(status);
  }
}

void 
NTAPI 
RtlReportErrorPropagation(LPSTR a1, int a2, int a3, int a4)
{
  ;
}

NTSTATUS 
WINAPI 
RtlParseDefinitionIdentity(int a, int b, BOOL c)
{
	return STATUS_SUCCESS;
}

NTSTATUS 
NTAPI 
RtlCloseDefinitionIdentityHandle(HANDLE handle, int verification)
{
  /*if ( handle )
    result = RtlCloseBaseIdentityHandle(handle, verification);
  else
    result = RtlRaiseStatus(0xC000000Du);
  return result;*/
  return STATUS_SUCCESS;
}

NTSTATUS 
NTAPI 
RtlGetDefinitionIdentityAttributeValue(HANDLE handle, int two, int three, int four)
{
  NTSTATUS result; // eax@4
  NTSTATUS otherStatus; // eax@5
  NTSTATUS status; // esi@5

  if ( !handle && four )
  {
    otherStatus = STATUS_SUCCESS;
    status = otherStatus;
    if ( otherStatus >= 0 )
    {
      result = 0;
    }
    else
    {
      RtlReportErrorPropagation("z:\\nt\\public\\internal\\base\\inc\\ntehmacros.h", 0, 92, otherStatus);
      result = status;
    }
  }
  else
  {
    result = STATUS_INVALID_PARAMETER;
  }
  return result;
}

DWORD WINAPI RtlInternString(int a, int b)
{
	return STATUS_SUCCESS;
}

void WINAPI RtlCloseStringHandle(ULONG64 a)
{
	;
}

DWORD WINAPI RtlGetStringPointer(int a, int b, int c, int d)
{
	return STATUS_SUCCESS;
}

/*subimplemented*/
DWORD WINAPI RtlReleaseStringPointer(PVOID this, int a, int b)
{
	return STATUS_SUCCESS;
}