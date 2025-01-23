 /*++

Copyright (c) 2025 Shorthorn Project

Module Name:

    interlck.c

Abstract:

    This module implements RTL Interlocked APIs

Author:

    Skulltrail 23-Jaunary-2025

Revision History:

--*/
  
#define NDEBUG

#include <main.h>

LONGLONG
NTAPI
RtlInterlockedCompareExchange64(LONGLONG volatile *Destination,
                                 LONGLONG Exchange,
                                 LONGLONG Comparand)
{
     /* Just call the intrinsic */
     return _InterlockedCompareExchange64(Destination, Exchange, Comparand);
}