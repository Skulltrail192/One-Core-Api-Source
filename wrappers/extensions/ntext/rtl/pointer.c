/*++

Copyright (c) 2025 Shorthorn Project

Module Name:

    pointer.c

Abstract:

    This module implements RTL Pointer APIs

Author:

    Skulltrail 23-Jaunary-2025

Revision History:

--*/
 
#define NDEBUG

#include <main.h>

PVOID NTAPI RtlEncodePointer(PVOID ptr) {
	return (PVOID)((UINT_PTR)ptr ^ 0xDEADBEEF);
}

PVOID NTAPI RtlDecodePointer(PVOID ptr) {
	return (PVOID)((UINT_PTR)ptr ^ 0xDEADBEEF);
}

PVOID NTAPI RtlEncodeSystemPointer(PVOID ptr) {
	return (PVOID)((UINT_PTR)ptr ^ 0xDEADBEEF);
}

PVOID NTAPI RtlDecodeSystemPointer(PVOID ptr) {
	return (PVOID)((UINT_PTR)ptr ^ 0xDEADBEEF);
}