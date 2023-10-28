/*++

Copyright (c) 2022 Shorthorn Project

Module Name:

    xstate.c

Abstract:

    This module implements Context of Xstate

Author:

    Skulltrail 26-September-2022

Revision History:

--*/
 
#define NDEBUG

#include <main.h>

WINE_DEFAULT_DEBUG_CHANNEL(ntext);

#define CONTEXT_AMD64   0x00100000

typedef struct _I386_CONTEXT
{
    DWORD   ContextFlags;  /* 000 */

    /* These are selected by CONTEXT_DEBUG_REGISTERS */
    DWORD   Dr0;           /* 004 */
    DWORD   Dr1;           /* 008 */
    DWORD   Dr2;           /* 00c */
    DWORD   Dr3;           /* 010 */
    DWORD   Dr6;           /* 014 */
    DWORD   Dr7;           /* 018 */

    /* These are selected by CONTEXT_FLOATING_POINT */
    I386_FLOATING_SAVE_AREA FloatSave; /* 01c */

    /* These are selected by CONTEXT_SEGMENTS */
    DWORD   SegGs;         /* 08c */
    DWORD   SegFs;         /* 090 */
    DWORD   SegEs;         /* 094 */
    DWORD   SegDs;         /* 098 */

    /* These are selected by CONTEXT_INTEGER */
    DWORD   Edi;           /* 09c */
    DWORD   Esi;           /* 0a0 */
    DWORD   Ebx;           /* 0a4 */
    DWORD   Edx;           /* 0a8 */
    DWORD   Ecx;           /* 0ac */
    DWORD   Eax;           /* 0b0 */

    /* These are selected by CONTEXT_CONTROL */
    DWORD   Ebp;           /* 0b4 */
    DWORD   Eip;           /* 0b8 */
    DWORD   SegCs;         /* 0bc */
    DWORD   EFlags;        /* 0c0 */
    DWORD   Esp;           /* 0c4 */
    DWORD   SegSs;         /* 0c8 */

    BYTE    ExtendedRegisters[I386_MAXIMUM_SUPPORTED_EXTENSION];  /* 0xcc */
} I386_CONTEXT;

struct context_copy_range
{
    ULONG start;
    ULONG flag;
};

static const struct context_copy_range copy_ranges_amd64[] =
{
    {0x38, 0x1}, {0x3a, 0x4}, { 0x42, 0x1}, { 0x48, 0x10}, { 0x78,  0x2}, { 0x98, 0x1},
    {0xa0, 0x2}, {0xf8, 0x1}, {0x100, 0x8}, {0x2a0,    0}, {0x4b0, 0x10}, {0x4d0,   0}
};

static const struct context_copy_range copy_ranges_x86[] =
{
    {  0x4, 0x10}, {0x1c, 0x8}, {0x8c, 0x4}, {0x9c, 0x2}, {0xb4, 0x1}, {0xcc, 0x20}, {0x1ec, 0},
    {0x2cc,    0},
};

static const struct context_parameters
{
    ULONG arch_flag;
    ULONG supported_flags;
    ULONG context_size;    /* sizeof(CONTEXT) */
    ULONG legacy_size;     /* Legacy context size */
    ULONG context_ex_size; /* sizeof(CONTEXT_EX) */
    ULONG alignment;       /* Used when computing size of context. */
    ULONG true_alignment;  /* Used for actual alignment. */
    ULONG flags_offset;
    const struct context_copy_range *copy_ranges;
}
arch_context_parameters[] =
{
    {
        CONTEXT_AMD64,
        0xd8000000 | CONTEXT_AMD64_ALL | CONTEXT_AMD64_XSTATE,
        sizeof(AMD64_CONTEXT),
        sizeof(AMD64_CONTEXT),
        0x20,
        7,
        TYPE_ALIGNMENT(AMD64_CONTEXT) - 1,
        offsetof(AMD64_CONTEXT,ContextFlags),
        copy_ranges_amd64
    },
    {
        CONTEXT_i386,
        0xd8000000 | CONTEXT_I386_ALL | CONTEXT_I386_XSTATE,
        sizeof(I386_CONTEXT),
        offsetof(I386_CONTEXT,ExtendedRegisters),
        0x18,
        3,
        TYPE_ALIGNMENT(I386_CONTEXT) - 1,
        offsetof(I386_CONTEXT,ContextFlags),
        copy_ranges_x86
    },
};

static const struct context_parameters *context_get_parameters( ULONG context_flags )
{
    unsigned int i;

    for (i = 0; i < ARRAY_SIZE(arch_context_parameters); ++i)
    {
        if (context_flags & arch_context_parameters[i].arch_flag)
            return context_flags & ~arch_context_parameters[i].supported_flags ? NULL : &arch_context_parameters[i];
    }
    return NULL;
}


static void context_copy_ranges( BYTE *d, DWORD context_flags, BYTE *s, const struct context_parameters *p )
{
    const struct context_copy_range *range;
    unsigned int start;

    *((ULONG *)(d + p->flags_offset)) |= context_flags;

    start = 0;
    range = p->copy_ranges;
    do
    {
        if (range->flag & context_flags)
        {
            if (!start)
                start = range->start;
        }
        else if (start)
        {
            memcpy( d + start, s + start, range->start - start );
            start = 0;
        }
    }
    while (range++->start != p->context_size);
}

/**********************************************************************
 *              RtlSetExtendedFeaturesMask  (NTDLL.@)
 */
void NTAPI RtlSetExtendedFeaturesMask( CONTEXT_EX *context_ex, ULONG64 feature_mask )
{
    XSTATE *xs = (XSTATE *)((BYTE *)context_ex + context_ex->XState.Offset);

    xs->Mask = RtlGetEnabledExtendedFeatures( feature_mask ) & ~(ULONG64)3;
}


/**********************************************************************
 *              RtlInitializeExtendedContext2    (NTDLL.@)
 */
NTSTATUS NTAPI RtlInitializeExtendedContext2( void *context, ULONG context_flags, CONTEXT_EX **context_ex,
        ULONG64 compaction_mask )
{
    const struct context_parameters *p;
    ULONG64 supported_mask = 0;
    CONTEXT_EX *c_ex;

    if (!(p = context_get_parameters( context_flags )))
        return STATUS_INVALID_PARAMETER;

    if ((context_flags & 0x40) && !(supported_mask = RtlGetEnabledExtendedFeatures( ~(ULONG64)0 )))
        return STATUS_NOT_SUPPORTED;

    context = (void *)(((ULONG_PTR)context + p->true_alignment) & ~(ULONG_PTR)p->true_alignment);
    *(ULONG *)((BYTE *)context + p->flags_offset) = context_flags;

    *context_ex = c_ex = (CONTEXT_EX *)((BYTE *)context + p->context_size);
    c_ex->Legacy.Offset = c_ex->All.Offset = -(LONG)p->context_size;
    c_ex->Legacy.Length = context_flags & 0x20 ? p->context_size : p->legacy_size;

    if (context_flags & 0x40)
    {
        XSTATE *xs;

        compaction_mask &= supported_mask;

        xs = (XSTATE *)(((ULONG_PTR)c_ex + p->context_ex_size + 63) & ~(ULONG_PTR)63);

        c_ex->XState.Offset = (ULONG_PTR)xs - (ULONG_PTR)c_ex;
        c_ex->XState.Length = offsetof(XSTATE, YmmContext);
        compaction_mask &= supported_mask;

        if (compaction_mask & (1 << XSTATE_AVX))
            c_ex->XState.Length += sizeof(YMMCONTEXT);

        memset( xs, 0, c_ex->XState.Length );
		//TODO: Add XStates field to USER_SHARED_DATA
        // if (user_shared_data->XState.CompactionEnabled)
            // xs->CompactionMask = ((ULONG64)1 << 63) | compaction_mask;

        c_ex->All.Length = p->context_size + c_ex->XState.Offset + c_ex->XState.Length;
    }
    else
    {
        c_ex->XState.Offset = 25; /* According to the tests, it is just 25 if CONTEXT_XSTATE is not specified. */
        c_ex->XState.Length = 0;
        c_ex->All.Length = p->context_size + 24; /* sizeof(CONTEXT_EX) minus 8 alignment bytes on x64. */
    }

    return STATUS_SUCCESS;
}

/**********************************************************************
 *              RtlInitializeExtendedContext    (NTDLL.@)
 */
NTSTATUS NTAPI RtlInitializeExtendedContext( void *context, ULONG context_flags, CONTEXT_EX **context_ex )
{
    return RtlInitializeExtendedContext2( context, context_flags, context_ex, ~(ULONG64)0 );
}

/**********************************************************************
 *              RtlLocateLegacyContext      (NTDLL.@)
 */
void * NTAPI RtlLocateLegacyContext( CONTEXT_EX *context_ex, ULONG *length )
{
    if (length)
        *length = context_ex->Legacy.Length;

    return (BYTE *)context_ex + context_ex->Legacy.Offset;
}

/**********************************************************************
 *              RtlCopyExtendedContext      (NTDLL.@)
 */
NTSTATUS NTAPI RtlCopyExtendedContext( CONTEXT_EX *dst, ULONG context_flags, CONTEXT_EX *src )
{
    const struct context_parameters *p;
    XSTATE *dst_xs, *src_xs;
    ULONG64 feature_mask;

    DbgPrint( "dst %p, context_flags %#lx, src %p.\n", dst, context_flags, src );

    if (!(p = context_get_parameters( context_flags )))
        return STATUS_INVALID_PARAMETER;

    if (!(feature_mask = RtlGetEnabledExtendedFeatures( ~(ULONG64)0 )) && context_flags & 0x40)
        return STATUS_NOT_SUPPORTED;

    context_copy_ranges( RtlLocateLegacyContext( dst, NULL ), context_flags, RtlLocateLegacyContext( src, NULL ), p );

    if (!(context_flags & 0x40))
        return STATUS_SUCCESS;

    if (dst->XState.Length < offsetof(XSTATE, YmmContext))
        return STATUS_BUFFER_OVERFLOW;

    dst_xs = (XSTATE *)((BYTE *)dst + dst->XState.Offset);
    src_xs = (XSTATE *)((BYTE *)src + src->XState.Offset);

    memset(dst_xs, 0, offsetof(XSTATE, YmmContext));
    dst_xs->Mask = (src_xs->Mask & ~(ULONG64)3) & feature_mask;
    dst_xs->CompactionMask = //user_shared_data->XState.CompactionEnabled
             ((ULONG64)1 << 63) | (src_xs->CompactionMask & feature_mask);// : 0;

    if (dst_xs->Mask & 4 && src->XState.Length >= sizeof(XSTATE) && dst->XState.Length >= sizeof(XSTATE))
        memcpy( &dst_xs->YmmContext, &src_xs->YmmContext, sizeof(dst_xs->YmmContext) );
    return STATUS_SUCCESS;
}

/***********************************************************************
 *              RtlCopyContext  (NTDLL.@)
 */
NTSTATUS NTAPI RtlCopyContext( CONTEXT *dst, DWORD context_flags, CONTEXT *src )
{
    DWORD context_size, arch_flag, flags_offset, dst_flags, src_flags;
    static const DWORD arch_mask = CONTEXT_i386 | CONTEXT_AMD64;
    const struct context_parameters *p;
    BYTE *d, *s;

    DbgPrint("dst %p, context_flags %#lx, src %p.\n", dst, context_flags, src);

    if (context_flags & 0x40 && !RtlGetEnabledExtendedFeatures( ~(ULONG64)0 )) return STATUS_NOT_SUPPORTED;

    arch_flag = context_flags & arch_mask;
    switch (arch_flag)
    {
    case CONTEXT_i386:
        context_size = sizeof( I386_CONTEXT );
        flags_offset = offsetof( I386_CONTEXT, ContextFlags );
        break;
    case CONTEXT_AMD64:
        context_size = sizeof( AMD64_CONTEXT );
        flags_offset = offsetof( AMD64_CONTEXT, ContextFlags );
        break;
    default:
        return STATUS_INVALID_PARAMETER;
    }

    d = (BYTE *)dst;
    s = (BYTE *)src;
    dst_flags = *(DWORD *)(d + flags_offset);
    src_flags = *(DWORD *)(s + flags_offset);

    if ((dst_flags & arch_mask) != arch_flag || (src_flags & arch_mask) != arch_flag)
        return STATUS_INVALID_PARAMETER;

    context_flags &= src_flags;
    if (context_flags & ~dst_flags & 0x40) return STATUS_BUFFER_OVERFLOW;

    if (context_flags & 0x40)
        return RtlCopyExtendedContext( (CONTEXT_EX *)(d + context_size), context_flags,
                                       (CONTEXT_EX *)(s + context_size) );

    if (!(p = context_get_parameters( context_flags )))
        return STATUS_INVALID_PARAMETER;

    context_copy_ranges( d, context_flags, s, p );
    return STATUS_SUCCESS;
}

/**********************************************************************
 *              RtlGetExtendedContextLength2    (NTDLL.@)
 */
NTSTATUS NTAPI RtlGetExtendedContextLength2( ULONG context_flags, ULONG *length, ULONG64 compaction_mask )
{
    const struct context_parameters *p;
    ULONG64 supported_mask;
    ULONG64 size;

    if (!(p = context_get_parameters( context_flags )))
        return STATUS_INVALID_PARAMETER;

    if (!(context_flags & 0x40))
    {
        *length = p->context_size + p->context_ex_size + p->alignment;
        return STATUS_SUCCESS;
    }

    if (!(supported_mask = RtlGetEnabledExtendedFeatures( ~(ULONG64)0) ))
        return STATUS_NOT_SUPPORTED;

    compaction_mask &= supported_mask;

    size = p->context_size + p->context_ex_size + offsetof(XSTATE, YmmContext) + 63;

    if (compaction_mask & supported_mask & (1 << XSTATE_AVX))
        size += sizeof(YMMCONTEXT);

    *length = size;
    return STATUS_SUCCESS;
}


/**********************************************************************
 *              RtlGetExtendedContextLength    (NTDLL.@)
 */
NTSTATUS NTAPI RtlGetExtendedContextLength( ULONG context_flags, ULONG *length )
{
    return RtlGetExtendedContextLength2( context_flags, length, ~(ULONG64)0 );
}

/**********************************************************************
 *              RtlLocateExtendedFeature2    (NTDLL.@)
 */
void * NTAPI RtlLocateExtendedFeature2( CONTEXT_EX *context_ex, ULONG feature_id,
        XSTATE_CONFIGURATION *xstate_config, ULONG *length )
{
    DbgPrint( "context_ex %p, feature_id %lu, xstate_config %p, length %p.\n",
            context_ex, feature_id, xstate_config, length );

    if (!xstate_config)
    {
        DbgPrint("RtlLocateExtendedFeature2::NULL xstate_config.\n" );
        return NULL;
    }

    // if (xstate_config != &user_shared_data->XState)
    // {
        // FIXME( "Custom xstate configuration is not supported.\n" );
        // return NULL;
    // }

    if (feature_id != XSTATE_AVX)
        return NULL;

    if (length)
        *length = sizeof(YMMCONTEXT);

    if (context_ex->XState.Length < sizeof(XSTATE))
        return NULL;

    return (BYTE *)context_ex + context_ex->XState.Offset + offsetof(XSTATE, YmmContext);
}


/**********************************************************************
 *              RtlLocateExtendedFeature    (NTDLL.@)
 */
void * NTAPI RtlLocateExtendedFeature( CONTEXT_EX *context_ex, ULONG feature_id,
        ULONG *length )
{
    return RtlLocateExtendedFeature2( context_ex, feature_id, (XSTATE_CONFIGURATION*)&SharedUserData->ProcessorFeatures, length );
}
