/*++

Copyright (c) 2017  Shorthorn Project

Module Name:

    sysinfo.c

Abstract:

    This module implements System Information APIs for Win32

Author:

    Skulltrail 11-April-2017

Revision History:

--*/

#include <main.h>

static BOOL (WINAPI *pSetSystemFileCacheSize)(SIZE_T, SIZE_T,int Flags);

/*
* @unimplemented - need implementation
*/
BOOL 
WINAPI 
GetOSProductNameW(
	PCWSTR Source, 
	ULONG var, 
	ULONG parameter
)
{
	Source = L"Microsoft Windows Codename \"Longhorn\" Professional Version 2003 Copyright ";
	return TRUE;
}

/*
* @unimplemented - need implementation
*/
BOOL 
WINAPI 
GetOSProductNameA(
	PCSTR Source, 
	ULONG var, 
	ULONG parameter
)
{
	Source = "Microsoft Windows Codename \"Longhorn\" Professional Version 2003 Copyright ";
	return TRUE;
}

/*
 * @implemented - new
 */
BOOL 
WINAPI 
GetProductInfo(
  _In_   DWORD dwOSMajorVersion,
  _In_   DWORD dwOSMinorVersion,
  _In_   DWORD dwSpMajorVersion,
  _In_   DWORD dwSpMinorVersion,
  _Out_  PDWORD pdwReturnedProductType
)
{
	return RtlGetProductInfo(dwOSMajorVersion, 
							 dwOSMinorVersion,
                             dwSpMajorVersion, 
							 dwSpMinorVersion, 
							 pdwReturnedProductType);
}

BOOL 
WINAPI 
SetSystemFileCacheSize(
	SIZE_T MinimumFileCacheSize, 
	SIZE_T MaximumFileCacheSize, 
	int Flags
)
{
	NTSTATUS Status; 
	BOOL result; 
	SYSTEM_FILECACHE_INFORMATION SystemInformation; 
	
	SystemInformation.MinimumWorkingSet = MinimumFileCacheSize;
	SystemInformation.MaximumWorkingSet = MaximumFileCacheSize;
	SystemInformation.Flags = Flags;
	
	Status = NtSetSystemInformation(SystemFileCacheInformationEx, &SystemInformation, sizeof(SYSTEM_FILECACHE_INFORMATION));
	if ( !NT_SUCCESS(Status) )
	{
		BaseSetLastNTError(Status);
		result = FALSE;
	}
	else
	{
		result = TRUE;
	}
	return result;	
}

// Required for Firefox 133.
BOOL WINAPI GetSystemCpuSetInformation(
    PSYSTEM_CPU_SET_INFORMATION  Information,
    ULONG                        BufferLength,
    PULONG                       ReturnedLength,
    HANDLE                       Process,
    ULONG                        Flags
) {
    
    if (ReturnedLength)
        *ReturnedLength = sizeof(SYSTEM_CPU_SET_INFORMATION);
    
    if (BufferLength < sizeof(SYSTEM_CPU_SET_INFORMATION)) {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }
    
    if (!Information) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    
    // CPU Sets are not a thing prior to Windows 10. Pretend that there is only a single "CPU Set".
    memset(Information, 0, sizeof(SYSTEM_CPU_SET_INFORMATION)); // already fills out all the fields we need anyway
    Information->Size = sizeof(SYSTEM_CPU_SET_INFORMATION);
    return TRUE;
}