/*++

Copyright (c) 2017 Shorthorn Project

Module Name:

    filemisc.c

Abstract:

    Misc file operations for Win32

Author:

    Skulltrail 01-December-2017

Revision History:

--*/

#include <main.h>

WINE_DEFAULT_DEBUG_CHANNEL(filemisc); 

BOOL 
WINAPI 
MoveFileTransactedA(
	LPCTSTR lpExistingFileName, 
	LPCTSTR lpNewFileName, 
	LPPROGRESS_ROUTINE lpProgressRoutine, 
	LPVOID lpData, 
	DWORD dwFlags, 
	HANDLE hTransaction
)
{
	return MoveFileWithProgressA(lpExistingFileName,
								lpNewFileName,
								lpProgressRoutine,
								lpData,
								dwFlags);
}

BOOL 
WINAPI 
MoveFileTransactedW(
	LPCWSTR lpExistingFileName, 
	LPCWSTR lpNewFileName, 
	LPPROGRESS_ROUTINE lpProgressRoutine, 
	LPVOID lpData, 
	DWORD dwFlags, 
	HANDLE hTransaction
)
{
	return MoveFileWithProgressW(lpExistingFileName,
								lpNewFileName,
								lpProgressRoutine,
								lpData,
								dwFlags);
}

BOOL 
WINAPI
DeleteFileTransactedW(
	LPCWSTR lpFileName, 
	HANDLE hTransaction
)
{
    return DeleteFileW(lpFileName);
}

BOOL 
WINAPI
DeleteFileTransactedA(
	LPCSTR lpFileName, 
	HANDLE hTransaction
)
{
    return DeleteFileA(lpFileName);
}

BOOL 
WINAPI
SetFileAttributesTransactedA(
  _In_ LPCTSTR lpFileName,
  _In_ DWORD   dwFileAttributes,
  _In_ HANDLE  hTransaction
)
{
	return SetFileAttributesA(lpFileName, dwFileAttributes);
}

BOOL 
WINAPI
SetFileAttributesTransactedW(
  _In_ LPCWSTR lpFileName,
  _In_ DWORD   dwFileAttributes,
  _In_ HANDLE  hTransaction
)
{
	return SetFileAttributesW(lpFileName, dwFileAttributes);
}

DWORD 
WINAPI 
GetCompressedFileSizeTransactedW(
	LPCWSTR lpFileName,
	LPDWORD lpFileSizeHigh, 
	HANDLE hTransaction
)
{
	return GetCompressedFileSizeW(lpFileName, lpFileSizeHigh);
}

DWORD 
WINAPI 
GetCompressedFileSizeTransactedA(
	LPCTSTR lpFileName, 
	LPDWORD lpFileSizeHigh, 
	HANDLE hTransaction
)
{
	return GetCompressedFileSizeA(lpFileName, lpFileSizeHigh);
}

/*
 * @implemented - need test
 */
BOOL 
WINAPI 
GetFileAttributesTransactedW(
	LPCWSTR lpFileName, 
	GET_FILEEX_INFO_LEVELS fInfoLevelId, 
	LPVOID lpFileInformation,
	HANDLE hTransaction
)
{
    return GetFileAttributesExW(lpFileName, fInfoLevelId, lpFileInformation);
}


BOOL 
WINAPI 
GetFileAttributesTransactedA(
	LPCSTR lpFileName, 
	GET_FILEEX_INFO_LEVELS fInfoLevelId, 
	LPVOID lpFileInformation, 
	HANDLE hTransaction
)
{
    return GetFileAttributesExA(lpFileName, fInfoLevelId, lpFileInformation);
}

HANDLE WINAPI FindFirstFileNameW(
  LPCWSTR lpFileName,
  DWORD   dwFlags,
  LPDWORD StringLength,
  PWSTR   LinkName
)
{
	return (HANDLE)-1;
}

BOOL WINAPI FindNextFileNameW(
  HANDLE  hFindStream,
  LPDWORD StringLength,
  PWSTR   LinkName
)
{
	return FALSE;
}

/*
 * @unimplemented
 */
BOOL
WINAPI
SetFileBandwidthReservation(IN HANDLE hFile,
                            IN DWORD nPeriodMilliseconds,
                            IN DWORD nBytesPerPeriod,
                            IN BOOL bDiscardable,
                            OUT LPDWORD lpTransferSize,
                            OUT LPDWORD lpNumOutstandingRequests)
{
    UNIMPLEMENTED;
    return FALSE;
}


/*
 * @unimplemented
 */
BOOL
WINAPI
GetFileBandwidthReservation(IN HANDLE hFile,
                            OUT LPDWORD lpPeriodMilliseconds,
                            OUT LPDWORD lpBytesPerPeriod,
                            OUT LPBOOL pDiscardable,
                            OUT LPDWORD lpTransferSize,
                            OUT LPDWORD lpNumOutstandingRequests)
{
    UNIMPLEMENTED;
    return FALSE;
}

HANDLE 
WINAPI 
FindFirstFileNameTransactedW(
  LPCWSTR lpFileName,
  DWORD   dwFlags,
  LPDWORD StringLength,
  PWSTR   LinkName,
  HANDLE  hTransaction
)
{
	return (HANDLE)-1;
}