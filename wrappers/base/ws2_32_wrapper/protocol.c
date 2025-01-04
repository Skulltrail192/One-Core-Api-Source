/*++

Copyright (c) 2022 Shorthorn Project

Module Name:

    protocol.c

Abstract:

    This module implements Protocol APIs

Author:

    Skulltrail 25-September-2022

Revision History:

--*/

#include "main.h"

WINE_DEFAULT_DEBUG_CHANNEL(ws2_32);

/***********************************************************************
 *      GetHostNameW   (ws2_32.@)
 */
int WSAAPI GetHostNameW( WCHAR *name, int namelen )
{
    char buf[256];
    struct gethostname_params params = { buf, sizeof(buf) };
    int ret;

    TRACE( "name %p, len %d\n", name, namelen );

    if (!name)
    {
        SetLastError( WSAEFAULT );
        return -1;
    }

    if ((ret = gethostname( params.name, params.size )))
    {
        SetLastError( ret );
        return -1;
    }

    if (MultiByteToWideChar( CP_ACP, 0, buf, -1, NULL, 0 ) > namelen)
    {
        SetLastError( WSAEFAULT );
        return -1;
    }
    MultiByteToWideChar( CP_ACP, 0, buf, -1, name, namelen );
    return 0;
}