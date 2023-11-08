/*++

Copyright (c) 2023 Shorthorn Project

Module Name:

    main.c

Abstract:

    This module implements COM Marshaling functions APIs

Author:

    Skulltrail 07-November-2023

Revision History:

--*/

#define WIN32_NO_STATUS

#include "main.h"

WINE_DEFAULT_DEBUG_CHANNEL(combase);

/***********************************************************************
 *      CleanupTlsOleState (combase.@)
 */
void WINAPI CleanupTlsOleState(void *unknown)
{
    FIXME("(%p): stub\n", unknown);
}

