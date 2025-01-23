/*++

Copyright (c) 2025 Shorthorn Project

Module Name:

    unicode.c

Abstract:

    This module implements RTL Unicode APIs

Author:

    Skulltrail 23-Jaunary-2025

Revision History:

--*/
 
#define NDEBUG

#include <main.h>

NTSYSAPI
NTSTATUS 
NTAPI 
RtlInitAnsiStringEx(
	IN OUT PANSI_STRING  	DestinationString,
	IN PCSZ  	SourceString 
) 		
{
    SIZE_T Size;

    if (SourceString)
    {
        Size = strlen(SourceString);
        if (Size > (MAXUSHORT - sizeof(CHAR))) return STATUS_NAME_TOO_LONG;
        DestinationString->Length = (USHORT)Size;
        DestinationString->MaximumLength = (USHORT)Size + sizeof(CHAR);
    }
    else
    {
        DestinationString->Length = 0;
        DestinationString->MaximumLength = 0;
    }

    DestinationString->Buffer = (PCHAR)SourceString;
    return STATUS_SUCCESS;
}

NTSTATUS 
NTAPI 
RtlInitLUnicodeStringFromNullTerminatedString(
	PUNICODE_STRING string, 
	PWSTR constantString
)
{
  SIZE_T size; // eax@3
  NTSTATUS status; // esi@4
  SIZE_T length; // eax@7

  if ( !string )
  {
    status = STATUS_INVALID_PARAMETER;
    RtlReportErrorOrigination("d:\\main\\base\\ntos\\rtl\\lstring.cpp", 0, 395, STATUS_INVALID_PARAMETER);
    return status;
  }
  string->Length = 0;
  string->Buffer = 0;
  string->MaximumLength = 0;
  if ( constantString )
  {
    size = wcslen(constantString);
    if ( size > 0x7FFFFFFE )
    {
      status = STATUS_INCOMPATIBLE_DRIVER_BLOCKED;
      RtlReportErrorOrigination("d:\\main\\base\\ntos\\rtl\\lstring.cpp", 0, 401, STATUS_INCOMPATIBLE_DRIVER_BLOCKED);
      return status;
    }
    length = 2 * size;
    string->Length = length;
    string->Buffer = constantString;
    string->MaximumLength = (length + 2);
  }
  return 0;
}