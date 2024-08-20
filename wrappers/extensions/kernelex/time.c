/*++

Copyright (c) 2017 Shorthorn Project

Module Name:

    time.c

Abstract:

    This module implements Time functions for the Win32 APIs.

Author:

    Skulltrail 06-May-2017

Revision History:

--*/

#include <main.h>

WINE_DEFAULT_DEBUG_CHANNEL(kernelex);

/******************************************************************************
 *           QueryInterruptTime  (kernelex.@)
 */
void WINAPI DECLSPEC_HOTPATCH QueryInterruptTime( ULONGLONG *time )
{
    ULONG high, low;

    do
    {
        high = SharedUserData->InterruptTime.High1Time;
        low = SharedUserData->InterruptTime.LowPart;
    }
    while (high != SharedUserData->InterruptTime.High2Time);
    *time = (ULONGLONG)high << 32 | low;
}


/******************************************************************************
 *           QueryInterruptTimePrecise  (kernelex.@)
 */
void WINAPI DECLSPEC_HOTPATCH QueryInterruptTimePrecise( ULONGLONG *time )
{
    static int once;
    if (!once++) FIXME( "(%p) semi-stub\n", time );

    QueryInterruptTime( time );
}


/***********************************************************************
 *           QueryUnbiasedInterruptTimePrecise  (kernelex.@)
 */
void WINAPI DECLSPEC_HOTPATCH QueryUnbiasedInterruptTimePrecise( ULONGLONG *time )
{
    static int once;
    if (!once++) FIXME( "(%p): semi-stub.\n", time );

    RtlQueryUnbiasedInterruptTime( time );
}

/***********************************************************************
 *           QueryUnbiasedInterruptTime   (KERNEL32.@)
 */
BOOL 
WINAPI 
QueryUnbiasedInterruptTime(ULONGLONG *time)
{
    if (!time) return FALSE;
    RtlQueryUnbiasedInterruptTime(time);
    return TRUE;
}