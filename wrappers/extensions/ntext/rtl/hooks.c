/*++

Copyright (c) 2024 Shorthorn Project

Module Name:

    hooks.c

Abstract:

    Implement Hooks of Rtl functions

Author:

    Skulltrail 07-November-2024

Revision History:

--*/

#include "main.h"
#include <ntstrsafe.h>

BOOLEAN
ReadEmulatedVersion(
    PUNICODE_STRING EmulatedVersion
    )
{
    PKEY_VALUE_PARTIAL_INFORMATION KeyInfo;
    UNICODE_STRING valueKeyName;
    ULONG informationLength;
    NTSTATUS status;
	WCHAR buffer[ 128 ];
    OBJECT_ATTRIBUTES Obj;
    HANDLE  Handle = NULL;	
	UNICODE_STRING UnicodeKey;

    /*Initialize the Unicode String with allocation*/
	UnicodeKey.Length = 0;
	UnicodeKey.MaximumLength = ( wcslen ( L"\\REGISTRY\\MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\" ) * sizeof ( WCHAR ));
	UnicodeKey.Buffer = (PWSTR)RtlAllocateHeap( RtlProcessHeap(), 0, UnicodeKey.MaximumLength );

	if ( UnicodeKey.Buffer != NULL )
	{
		/*Copy key String into buffer of the Unicode String*/
		RtlAppendUnicodeToString ( &UnicodeKey, L"\\REGISTRY\\MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\" );

		InitializeObjectAttributes( &Obj,
								    &UnicodeKey,
								    OBJ_CASE_INSENSITIVE,
								    NULL,
									NULL
									);

        /*Open the Registry key*/ 
		status = NtOpenKey(	&Handle,
							GENERIC_READ,
							&Obj
							);
		
        /*Fail on open the Registry key operation*/ 		
		if (!NT_SUCCESS( status ))
			goto end;								
	
	}

    /*Copy Value String into buffer of the Unicode String*/
    RtlInitUnicodeString( &valueKeyName,
                          L"EmulatedVersion" );

    KeyInfo = (PKEY_VALUE_PARTIAL_INFORMATION)buffer;

    /*Query the Registry key value*/ 
    status = NtQueryValueKey( Handle,
                              &valueKeyName,
                              KeyValuePartialInformation,
                              (PVOID)KeyInfo,
                              sizeof(buffer),
                              &informationLength );
							  
	if (!NT_SUCCESS( status ))
		goto end;									  

    if (status == STATUS_SUCCESS &&
        KeyInfo->Type == REG_SZ ||
        KeyInfo->Type == REG_MULTI_SZ)
    {
		/*Everything seem ok, so, copy the registry key value into unicode string parameter*/ 
		RtlInitUnicodeString(EmulatedVersion, (PWSTR)KeyInfo->Data);
		/*Closing Reg key Handle*/
		if (Handle != NULL)
		{
			ZwClose(Handle);
			Handle = NULL;
		}	

        /*Freeing Unicode String key*/
        RtlFreeHeap(RtlProcessHeap(), 0, UnicodeKey.Buffer);		
		return TRUE;
    }
end:

    if (Handle != NULL)
    {
		/*Closing Reg key Handle*/
        ZwClose(Handle);
        Handle = NULL;
    }
	
    /*Freeing Unicode String key*/
    RtlFreeHeap(RtlProcessHeap(), 0, UnicodeKey.Buffer);	

    return FALSE;

} 

ULONG
GetNextPointValue (
    WCHAR **p,
    ULONG *len
    )
{
    ULONG num;

    num = 0;

    while (*len && (UNICODE_NULL != **p) && **p != L'.') {
        if ( L' ' != **p ) {
            num = (num * 10) + ( (ULONG)**p - L'0' );
        }

        (*p)++;
        (*len)--;
    }

    if ((*len) && (L'.' == **p)) {
        (*p)++;
        (*len)--;
    }

    return num;
}

/*
 * @implemented
 * @note User-mode version of RtlGetVersion in ntoskrnl/rtl/misc.c
 */
NTSTATUS 
NTAPI
RtlGetVersionInternal(IN OUT PRTL_OSVERSIONINFOW lpVersionInformation)
{
    SIZE_T Length;
    PPEB Peb = NtCurrentPeb();
	ULONG len;
    UNICODE_STRING EmulatedVersion;
	PWCHAR p;	

    if (lpVersionInformation->dwOSVersionInfoSize != sizeof(RTL_OSVERSIONINFOW) &&
        lpVersionInformation->dwOSVersionInfoSize != sizeof(RTL_OSVERSIONINFOEXW))
    {
        return STATUS_INVALID_PARAMETER;
    }
	
	if(ReadEmulatedVersion(&EmulatedVersion)){
		p = EmulatedVersion.Buffer;
		len = EmulatedVersion.Length / sizeof(WCHAR); 
		lpVersionInformation->dwMajorVersion = GetNextPointValue( &p, &len );
		lpVersionInformation->dwMinorVersion = GetNextPointValue( &p, &len );
		lpVersionInformation->dwBuildNumber  = GetNextPointValue( &p, &len );		
	}else{
		lpVersionInformation->dwMajorVersion = Peb->OSMajorVersion;
		lpVersionInformation->dwMinorVersion = Peb->OSMinorVersion;
		lpVersionInformation->dwBuildNumber = Peb->OSBuildNumber;		
	}

    lpVersionInformation->dwPlatformId = Peb->OSPlatformId;
    RtlZeroMemory(lpVersionInformation->szCSDVersion, sizeof(lpVersionInformation->szCSDVersion));

    /* If we have a CSD version string, initialized by Application Compatibility... */
    if (Peb->CSDVersion.Length && Peb->CSDVersion.Buffer && Peb->CSDVersion.Buffer[0] != UNICODE_NULL)
    {
        /* ... copy it... */
        Length = min(wcslen(Peb->CSDVersion.Buffer), ARRAYSIZE(lpVersionInformation->szCSDVersion) - 1);
        wcsncpy(lpVersionInformation->szCSDVersion, Peb->CSDVersion.Buffer, Length);
    }
    else
    {
        /* ... otherwise we just null-terminate it */
        Length = 0;
    }

    /* Always null-terminate the user CSD version string */
    lpVersionInformation->szCSDVersion[Length] = UNICODE_NULL;

    if (lpVersionInformation->dwOSVersionInfoSize == sizeof(RTL_OSVERSIONINFOEXW))
    {
        PRTL_OSVERSIONINFOEXW InfoEx = (PRTL_OSVERSIONINFOEXW)lpVersionInformation;
        InfoEx->wServicePackMajor = (Peb->OSCSDVersion >> 8) & 0xFF;
        InfoEx->wServicePackMinor = Peb->OSCSDVersion & 0xFF;
        InfoEx->wSuiteMask = SharedUserData->SuiteMask & 0xFFFF;
        InfoEx->wProductType = SharedUserData->NtProductType;
        InfoEx->wReserved = 0;

        /* HACK: ReactOS specific changes, see bug-reports CORE-6611 and CORE-4620 (aka. #5003) */
        //SetRosSpecificInfo(InfoEx);
    }

    return STATUS_SUCCESS;
}