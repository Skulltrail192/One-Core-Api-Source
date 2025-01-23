/*++

Copyright (c) 2023 Shorthorn Project

Module Name:

    heap.c

Abstract:

    This module implements RTL Heap Internal APIs

Author:

    Skulltrail 23-Jaunary-2025

Revision History:

--*/

#define NDEBUG

#include <main.h>

VOID 
NTAPI
RtlpFreeMemory( 	
	PVOID  	Mem,
	ULONG  	Tag 
) 		
{
    UNREFERENCED_PARAMETER(Tag);

    RtlFreeHeap(RtlGetProcessHeap(),
                0,
                Mem);
}

PVOID 
NTAPI 
RtlpAllocateMemory( 	
	SIZE_T  	Bytes,
	ULONG  	Tag 
) 		
{
    UNREFERENCED_PARAMETER(Tag);

    return RtlAllocateHeap(RtlGetProcessHeap(),
                           0,
                           Bytes);
}