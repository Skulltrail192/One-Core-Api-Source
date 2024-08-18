/*++

Copyright (c) 2017 Shorthorn Project

Module Name:

    snapshot.c

Abstract:

    This module implements the Win32 Snapshot APIs.

Author:

    Skulltrail 08-July-2024

Revision History:

--*/

#include <main.h>
#include <tlhelp32.h>

WINE_DEFAULT_DEBUG_CHANNEL(kernel32);

typedef struct _HPSSW7 {
   HANDLE snapshot;
   HANDLE proc;
   PROCESSENTRY32 pe;
   DWORD ParentID;
} HPSSW7, *PHPSSW7;

// PSS APIs. These APIs solely exist to make Python 3.12 work.
DWORD 
WINAPI 
PssCaptureSnapshot(
    HANDLE ProcessHandle,
    PSS_CAPTURE_FLAGS CaptureFlags,
    DWORD ThreadContextFlags,
    HPSS *SnapshotHandle
) 
{
    DWORD mypid = GetProcessId(ProcessHandle);
    DWORD result;
    HANDLE Snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    BOOL have_record;
    PROCESSENTRY32 pe;
    PHPSSW7 SH;
   
    if (CaptureFlags != PSS_CAPTURE_NONE) {
        // This class is Not supported. Add a debug print in there.
		return STATUS_NOT_IMPLEMENTED;
    }

    if (Snapshot == INVALID_HANDLE_VALUE) {
		return HRESULT_FROM_WIN32(GetLastError());
    }

    pe.dwSize = sizeof(pe);
    have_record = Process32First(Snapshot, &pe);
    while (have_record) {
		if (mypid == pe.th32ProcessID) {
			/* We could cache the ulong value in a static variable. */
			result = pe.th32ParentProcessID;
			break;
        }
		have_record = Process32Next(Snapshot, &pe);
    }
    SH = HeapAlloc(GetProcessHeap(), 0, sizeof(HPSSW7));
    if (SH == NULL) {
        CloseHandle(Snapshot);
		return 0x8007007A;
    }
    SH->snapshot = Snapshot;
    SH->pe = pe;
    SH->ParentID = result;
    *SnapshotHandle = SH;
    return 0;
}

DWORD 
WINAPI 
PssFreeSnapshot(
	HANDLE ProcessHandle, 
	HPSS SnapshotHandle
) 
{
	PHPSSW7 SSW7 = SnapshotHandle;
	CloseHandle(SSW7->snapshot);
	HeapFree( GetProcessHeap(), 0, SSW7 );
    return TRUE;
}

DWORD 
WINAPI 
PssQuerySnapshot(
	HPSS SnapshotHandle, 
	PSS_QUERY_INFORMATION_CLASS Class, 
	void *Buffer, 
	DWORD Length
)
{
	PHPSSW7 SSW7 = SnapshotHandle;
	if (Class == PSS_QUERY_PROCESS_INFORMATION) {
		if (Length == sizeof(PSS_PROCESS_INFORMATION)) {
			PSS_PROCESS_INFORMATION* Val = Buffer;
			Val->ParentProcessId = SSW7->ParentID;
			return 0;
		}
	} else {
		// This class is Not supported. Add a debug print in there.
		return STATUS_NOT_IMPLEMENTED;
	}
	return 0x80070057;
}