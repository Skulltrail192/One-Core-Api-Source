/*++

Copyright (c) 2017 Shorthorn Project

Module Name:

    mem.c

Abstract:

    This module contains the Win32 Global Memory Management APIs

Author:

    Skulltrail 19-March-2017

Revision History:

--*/

#include "main.h"

#define DIV 1024

SIZE_T
WINAPI
GetLargePageMinimum (
    VOID
)
{
    return (SIZE_T) SharedUserData->LargePageMinimum;
}

BOOL 
WINAPI 
GetPhysicallyInstalledSystemMemory(
  _Out_  PULONGLONG TotalMemoryInKilobytes
)
{
	MEMORYSTATUSEX memory;
	memory.dwLength = sizeof(MEMORYSTATUSEX);
	GlobalMemoryStatusEx(&memory);
	*TotalMemoryInKilobytes = memory.ullTotalPhys/DIV;
	return TRUE;
}

BOOL 
WINAPI 
AllocateUserPhysicalPagesNuma(
	HANDLE hProcess, 
	PULONG_PTR NumberOfPages, 
	PULONG_PTR PageArray, 
	DWORD nndPreferred
)
{
  DWORD_PTR ThreadAffinitiyMask;
  NTSTATUS Status; 
  HANDLE CurrentThread; 
  ULONGLONG ProcessorMask; 
  
  DbgPrint("AllocateUserPhysicalPagesNuma called\n");

  ThreadAffinitiyMask = 0;
  if ( nndPreferred != -1 )
  {
    if ( !GetNumaNodeProcessorMask(nndPreferred, &ProcessorMask) )
      return AllocateUserPhysicalPages(hProcess, NumberOfPages, PageArray);
    ThreadAffinitiyMask = ProcessorMask;
    CurrentThread = GetCurrentThread();
    ThreadAffinitiyMask = SetThreadAffinityMask(CurrentThread, ThreadAffinitiyMask);
    if ( !ThreadAffinitiyMask )
      return FALSE;
  }
  Status = NtAllocateUserPhysicalPages(hProcess, NumberOfPages, PageArray);
  if ( ThreadAffinitiyMask )
  {
    CurrentThread = GetCurrentThread();
    SetThreadAffinityMask(CurrentThread, ThreadAffinitiyMask);
  }
  if ( NT_SUCCESS(Status) )
    return TRUE;
  BaseSetLastNTError(Status);
  return FALSE;
}

LPVOID 
WINAPI 
VirtualAllocExNuma(
	HANDLE ProcessHandle, 
	LPVOID BaseAddress, 
	SIZE_T AllocationSize, 
	DWORD AllocationType, 
	DWORD Protect, 
	DWORD nndPreferred
)
{
	if ( nndPreferred != -1 )
      AllocationType |= nndPreferred + 1;
    return VirtualAllocEx(ProcessHandle,
						  BaseAddress,
						  AllocationSize,
						  AllocationType,
						  Protect);
}

/***********************************************************************
 *             PrefetchVirtualMemory   (kernelbase.@)
 */
BOOL 
WINAPI 
DECLSPEC_HOTPATCH 
PrefetchVirtualMemory( 
	HANDLE _hProcess, 
	ULONG_PTR _uNumberOfEntries,
    WIN32_MEMORY_RANGE_ENTRY *_pVirtualAddresses, 
	ULONG _fFlags 
)
{
	UNREFERENCED_PARAMETER(_hProcess);
	UNREFERENCED_PARAMETER(_uNumberOfEntries);
	UNREFERENCED_PARAMETER(_pVirtualAddresses);
	UNREFERENCED_PARAMETER(_fFlags);

	// 假装自己预取成功
	return TRUE;
}

// /***********************************************************************
 // *             DiscardVirtualMemory   (kernelbase.@)
 // */
// DWORD WINAPI DECLSPEC_HOTPATCH DiscardVirtualMemory( void *addr, SIZE_T size )
// {
    // NTSTATUS status;
    // LPVOID ret = addr;

    // status = NtAllocateVirtualMemory( GetCurrentProcess(), &ret, 0, &size, MEM_RESET, PAGE_NOACCESS );
    // return RtlNtStatusToDosError( status );
// }

DWORD
WINAPI
OfferVirtualMemory(
	_Inout_updates_(_uSize) PVOID _pVirtualAddress,
	_In_ SIZE_T _uSize,
	_In_ OFFER_PRIORITY _ePriority
)
{
	// 低版本系统不支持这个机制，所以暂时假装内存充足，不触发回收
	MEMORY_BASIC_INFORMATION _Info;
	
	UNREFERENCED_PARAMETER(_ePriority);
	if (VirtualQuery(_pVirtualAddress, &_Info, sizeof(_Info)) == 0)
		return GetLastError();

	if (_Info.State != MEM_COMMIT)
		return ERROR_INVALID_PARAMETER;


	if ((char*)_pVirtualAddress + _uSize > (char*)_Info.BaseAddress + _Info.RegionSize)
		return ERROR_INVALID_PARAMETER;

	return ERROR_SUCCESS;
}

DWORD
WINAPI
ReclaimVirtualMemory(
	_In_reads_(_uSize) void const* _pVirtualAddress,
	_In_ SIZE_T _uSize
)
{
	MEMORY_BASIC_INFORMATION _Info;
	if (VirtualQuery(_pVirtualAddress, &_Info, sizeof(_Info)) == 0)
		return GetLastError();

	if (_Info.State != MEM_COMMIT)
		return ERROR_INVALID_PARAMETER;


	if ((char*)_pVirtualAddress + _uSize > (char*)_Info.BaseAddress + _Info.RegionSize)
		return ERROR_INVALID_PARAMETER;
			
	return ERROR_SUCCESS;
}

DWORD
WINAPI
DiscardVirtualMemory(
	_Inout_updates_(_uSize) PVOID _pVirtualAddress,
	_In_ SIZE_T _uSize
)
{
	// if (const auto _pfnDiscardVirtualMemory = try_get_DiscardVirtualMemory())
	// {
		// return _pfnDiscardVirtualMemory(_pVirtualAddress, _uSize);
	// }

	if (!VirtualAlloc(_pVirtualAddress, _uSize, MEM_RESET, PAGE_NOACCESS))
		return GetLastError();

	return ERROR_SUCCESS;
}