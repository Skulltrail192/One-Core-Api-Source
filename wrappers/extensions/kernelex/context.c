/*++

Copyright (c) 2023 Shorthorn Project

Module Name:

    context.c

Abstract:

    Context functions for use in kernel32.dll

Author:

    Skulltrail 19-October-2023

Revision History:

--*/

#include <main.h>

WINE_DEFAULT_DEBUG_CHANNEL(kernel32);

/***********************************************************************
 *             InitializeContext2         (kernelex.@)
 */
BOOL WINAPI InitializeContext2( void *buffer, DWORD context_flags, CONTEXT **context, DWORD *length,
        ULONG64 compaction_mask )
{
    ULONG orig_length;
    NTSTATUS status;

    TRACE( "buffer %p, context_flags %#lx, context %p, ret_length %p, compaction_mask %s.\n",
            buffer, context_flags, context, length, wine_dbgstr_longlong(compaction_mask) );

    orig_length = *length;

    if ((status = RtlGetExtendedContextLength2( context_flags, length, compaction_mask )))
    {
        if (status == STATUS_NOT_SUPPORTED && context_flags & 0x40)
        {
            context_flags &= ~0x40;
            status = RtlGetExtendedContextLength2( context_flags, length, compaction_mask );
        }

        if (status)
            return set_ntstatus( status );
    }

    if (!buffer || orig_length < *length)
    {
        SetLastError( ERROR_INSUFFICIENT_BUFFER );
        return FALSE;
    }

    if ((status = RtlInitializeExtendedContext2( buffer, context_flags, (CONTEXT_EX **)context, compaction_mask )))
        return set_ntstatus( status );

    *context = (CONTEXT *)((BYTE *)*context + (*(CONTEXT_EX **)context)->Legacy.Offset);

    return TRUE;
}

// /***********************************************************************
 // *             InitializeContext               (kernelex.@)
 // */
// BOOL WINAPI InitializeContext( void *buffer, DWORD context_flags, CONTEXT **context, DWORD *length )
// {
    // return InitializeContext2( buffer, context_flags, context, length, ~(ULONG64)0 );
// }

BOOL WINAPI InitializeContext(PVOID Buffer, DWORD ContextFlags, PCONTEXT *Context, PDWORD ContextLength)
/*
  This function only exists because the size of the CONTEXT_XSTATE area can vary depending on the 
  type of CPU features available/enabled. There is no variability in a pre-XState/AVX context, so it simply
  initializes a regular context.
*/
{
	PCONTEXT ctxint;
	if(!Buffer || *ContextLength < sizeof(CONTEXT))
	{
		SetLastError(ERROR_INSUFFICIENT_BUFFER);
		*ContextLength = sizeof(CONTEXT);
		return FALSE;
	}
	
	Buffer = (PVOID)&ctxint;
	
	RtlZeroMemory(ctxint, sizeof(CONTEXT));
	
	*Context = (PCONTEXT)Buffer;
	
	ctxint->ContextFlags = ContextFlags;
	
	return TRUE;
}

/***********************************************************************
 *           CopyContext                       (kernelex.@)
 */
BOOL WINAPI CopyContext( CONTEXT *dst, DWORD context_flags, CONTEXT *src )
{
    return set_ntstatus( RtlCopyContext( dst, context_flags, src ));
}

