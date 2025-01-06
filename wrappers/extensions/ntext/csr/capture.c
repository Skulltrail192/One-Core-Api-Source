/*++

Copyright (c) 2025 Shorthorn Project

Module Name:

    api.c

Abstract:

    Routines for probing and capturing CSR API Messages

Author:

    Skulltrail 04-Jaunary-2025

Revision History:

--*/
 
#define NDEBUG

#include <main.h> 

/*
 * @implemented
 */
VOID
NTAPI
CsrProbeForRead(IN PVOID Address,
                IN ULONG Length,
                IN ULONG Alignment)
{
    volatile UCHAR *Pointer;
    UCHAR Data;

    /* Validate length */
    if (Length == 0) return;

    /* Validate alignment */
    if ((ULONG_PTR)Address & (Alignment - 1))
    {
        /* Raise exception if it doesn't match */
        RtlRaiseStatus(STATUS_DATATYPE_MISALIGNMENT);
    }

    /* Probe first byte */
    Pointer = Address;
    Data = *Pointer;

    /* Probe last byte */
    Pointer = (PUCHAR)Address + Length - 1;
    Data = *Pointer;
    (void)Data;
}

/*
 * @implemented
 */
VOID
NTAPI
CsrProbeForWrite(IN PVOID Address,
                 IN ULONG Length,
                 IN ULONG Alignment)
{
    volatile UCHAR *Pointer;

    /* Validate length */
    if (Length == 0) return;

    /* Validate alignment */
    if ((ULONG_PTR)Address & (Alignment - 1))
    {
        /* Raise exception if it doesn't match */
        RtlRaiseStatus(STATUS_DATATYPE_MISALIGNMENT);
    }

    /* Probe first byte */
    Pointer = Address;
    *Pointer = *Pointer;

    /* Probe last byte */
    Pointer = (PUCHAR)Address + Length - 1;
    *Pointer = *Pointer;
}
