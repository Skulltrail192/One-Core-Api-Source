/*++

Copyright (c) 2017 Shorthorn Project

Module Name:

    fileinfo.c

Abstract:

    This module implements Win32 File Information for others functions APIs

Author:

    Skulltrail 25-March-2017

Revision History:

--*/

#include "main.h"

BOOL bIsFileApiAnsi = TRUE; // set the file api to ansi or oem

static BOOL (WINAPI *pSetFileCompletionNotificationModes)(HANDLE, UCHAR);

PWCHAR
FilenameA2W(
	LPCSTR NameA, 
	BOOL alloc
)
{
   ANSI_STRING str;
   UNICODE_STRING strW;
   PUNICODE_STRING pstrW;
   NTSTATUS Status;

   ASSERT(NtCurrentTeb()->StaticUnicodeString.MaximumLength == sizeof(NtCurrentTeb()->StaticUnicodeBuffer));

   RtlInitAnsiString(&str, NameA);
   pstrW = alloc ? &strW : &NtCurrentTeb()->StaticUnicodeString;

   if (bIsFileApiAnsi)
        Status= RtlAnsiStringToUnicodeString( pstrW, &str, (BOOLEAN)alloc );
   else
        Status= RtlOemStringToUnicodeString( pstrW, &str, (BOOLEAN)alloc );

    if (NT_SUCCESS(Status))
       return pstrW->Buffer;

    if (Status== STATUS_BUFFER_OVERFLOW)
        SetLastError( ERROR_FILENAME_EXCED_RANGE );
    else
        BaseSetLastNTError(Status);

    return NULL;
}

/**************************************************************************
 *           SetFileCompletionNotificationModes   (KERNEL32.@)
 *
 *
 * @implemented
 */
BOOL
WINAPI
SetFileCompletionNotificationModes(IN HANDLE FileHandle,
                                   IN UCHAR Flags)
{
    NTSTATUS Status;
    FILE_IO_COMPLETION_NOTIFICATION_INFORMATION FileInformation;
    IO_STATUS_BLOCK IoStatusBlock;

    if (Flags & ~(FILE_SKIP_COMPLETION_PORT_ON_SUCCESS | FILE_SKIP_SET_EVENT_ON_HANDLE))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    FileInformation.Flags = Flags;

    Status = NtSetInformationFile(FileHandle,
                                  &IoStatusBlock,
                                  &FileInformation,
                                  sizeof(FileInformation),
                                  FileIoCompletionNotificationInformation);
    if (!NT_SUCCESS(Status))
    {
		DbgPrint("SetFileCompletionNotificationModes::NtSetInformationFile failed with status: %08x\n", Status);
        BaseSetLastNTError(Status);
        return FALSE;
    }

    return TRUE;
}