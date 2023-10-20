/*++

Copyright (c) 2022 Shorthorn Project

Module Name:

    xstate.c

Abstract:

    This module implements Context of Xstate

Author:

    Skulltrail 03-August-2022

Revision History:

--*/

#include "main.h"

//Wine version
// /***********************************************************************
 // *             GetEnabledXStateFeatures   (kernelbase.@)
 // */
// DWORD64 WINAPI GetEnabledXStateFeatures(void)
// {
    // TRACE( "\n" );
    // return RtlGetEnabledExtendedFeatures( ~(ULONG64)0 );
// }

DWORD64 WINAPI GetEnabledXStateFeatures()
{
	DWORD64 XState = 0;
	
	XState = 3; // bits 0 and 1 represent X87 and SSE respectively, which all AMD64 CPUs support.
	       
	return XState;
}

// BOOL WINAPI SetXStateFeaturesMask(PCONTEXT Context, DWORD64 FeatureMask)
// // AMD64 version of the function. I don't see the need to modify it for other archs for the purpose of the extended kernel yet.
// {
	// if(!(Context->ContextFlags & CONTEXT_AMD64))
	// {
		// SetLastError(ERROR_INVALID_PARAMETER);
		// return FALSE;
	// }
	
	// if(FeatureMask & 3)
		// Context->ContextFlags |= CONTEXT_FLOATING_POINT;
	
	// return TRUE;
// }

/***********************************************************************
 *           LocateXStateFeature   (kernelbase.@)
 */
void * WINAPI LocateXStateFeature( CONTEXT *context, DWORD feature_id, DWORD *length )
{
    if (!(context->ContextFlags & CONTEXT_AMD64))
        return NULL;

    if (feature_id >= 2)
        return ((context->ContextFlags & CONTEXT_XSTATE) == CONTEXT_XSTATE)
                ? RtlLocateExtendedFeature( (CONTEXT_EX *)(context + 1), feature_id, length ) : NULL;

    if (feature_id == 1)
    {
        if (length)
            *length = sizeof(M128A) * 16;
#if defined(_X86_)
        return &context->FloatSave.RegisterArea;
#else
        return &context->FltSave.XmmRegisters;	
#endif	
    }

    if (length)
        *length = offsetof(XSAVE_FORMAT, XmmRegisters);

#if defined(_X86_)
        return &context->FloatSave;
#else
        return &context->FltSave;	
#endif
}

/***********************************************************************
 *           SetXStateFeaturesMask (kernelbase.@)
 */
BOOL WINAPI SetXStateFeaturesMask( CONTEXT *context, DWORD64 feature_mask )
{
    if (!(context->ContextFlags & CONTEXT_AMD64))
        return FALSE;

    if (feature_mask & 0x3)
        context->ContextFlags |= CONTEXT_FLOATING_POINT;

    if ((context->ContextFlags & CONTEXT_XSTATE) != CONTEXT_XSTATE)
        return !(feature_mask & ~(DWORD64)3);

    RtlSetExtendedFeaturesMask( (CONTEXT_EX *)(context + 1), feature_mask );
    return TRUE;
}

/***********************************************************************
 *           GetXStateFeaturesMask (kernelbase.@)
 */
BOOL WINAPI GetXStateFeaturesMask( CONTEXT *context, DWORD64 *feature_mask )
{
    if (!(context->ContextFlags & CONTEXT_AMD64))
        return FALSE;

    *feature_mask = (context->ContextFlags & CONTEXT_FLOATING_POINT) == CONTEXT_FLOATING_POINT
            ? 3 : 0;

    if ((context->ContextFlags & CONTEXT_XSTATE) == CONTEXT_XSTATE)
        *feature_mask |= RtlGetExtendedFeaturesMask( (CONTEXT_EX *)(context + 1) );

    return TRUE;
}