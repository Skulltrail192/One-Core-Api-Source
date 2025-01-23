/*++

Copyright (c) 2025 Shorthorn Project

Module Name:

    critical.c

Abstract:

    Implement Critical Sections functions

Author:

    Skulltrail 23-Jaunary-2025

Revision History:

--*/
  
#define NDEBUG

#include <main.h>
#include <debug.h>

#define MAX_STATIC_CS_DEBUG_OBJECTS 64

static RTL_CRITICAL_SECTION RtlCriticalSectionLock;
static LIST_ENTRY RtlCriticalSectionList = {&RtlCriticalSectionList, &RtlCriticalSectionList};
static BOOLEAN RtlpCritSectInitialized = FALSE;
static BOOLEAN RtlpDebugInfoFreeList[MAX_STATIC_CS_DEBUG_OBJECTS];
static RTL_CRITICAL_SECTION_DEBUG RtlpStaticDebugInfo[MAX_STATIC_CS_DEBUG_OBJECTS];

/*++
 * RtlGetCriticalSectionRecursionCount
 * @implemented NT5.2 SP1
 *
 *     Retrieves the recursion count of a given critical section.
 *
 * Params:
 *     CriticalSection - Critical section to retrieve its recursion count.
 *
 * Returns:
 *     The recursion count.
 *
 * Remarks:
 *     We return the recursion count of the critical section if it is owned
 *     by the current thread, and otherwise we return zero.
 *
 *--*/
NTSYSAPI 
LONG
NTAPI
RtlGetCriticalSectionRecursionCount(PRTL_CRITICAL_SECTION CriticalSection)
{
    if (CriticalSection->OwningThread == NtCurrentTeb()->ClientId.UniqueThread)
    {
        /*
         * The critical section is owned by the current thread,
         * therefore retrieve its actual recursion count.
         */
        return CriticalSection->RecursionCount;
    }
    else
    {
        /*
         * It is not owned by the current thread, so
         * for this thread there is no recursion.
         */
        return 0;
    }
}

/***********************************************************************
 *           RtlIsCriticalSectionLockedByThread   (NTDLL.@)
 *
 * Checks if the critical section is locked by the current thread.
 *
 * PARAMS
 *  crit [I/O] Critical section to check.
 *
 * RETURNS
 *  Success: TRUE. The critical section is locked.
 *  Failure: FALSE. The critical section is not locked.
 */
NTSYSAPI 
ULONG 
WINAPI 
RtlIsCriticalSectionLockedByThread( RTL_CRITICAL_SECTION *CriticalSection )
{
    return CriticalSection->OwningThread == NtCurrentTeb()->ClientId.UniqueThread &&
           CriticalSection->RecursionCount != 0;
}

/*++
 * RtlpAllocateDebugInfo
 *
 *     Finds or allocates memory for a Critical Section Debug Object
 *
 * Params:
 *     None
 *
 * Returns:
 *     A pointer to an empty Critical Section Debug Object.
 *
 * Remarks:
 *     For optimization purposes, the first 64 entries can be cached. From
 *     then on, future Critical Sections will allocate memory from the heap.
 *
 *--*/
PRTL_CRITICAL_SECTION_DEBUG
NTAPI
RtlpAllocateDebugInfo(VOID)
{
    ULONG i;

    /* Try to allocate from our buffer first */
    for (i = 0; i < MAX_STATIC_CS_DEBUG_OBJECTS; i++)
    {
        /* Check if Entry is free */
        if (!RtlpDebugInfoFreeList[i])
        {
            /* Mark entry in use */
            DPRINT("Using entry: %lu. Buffer: %p\n", i, &RtlpStaticDebugInfo[i]);
            RtlpDebugInfoFreeList[i] = TRUE;

            /* Use free entry found */
            return &RtlpStaticDebugInfo[i];
        }
    }

    /* We are out of static buffer, allocate dynamic */
    return RtlAllocateHeap(RtlGetProcessHeap(),
                           0,
                           sizeof(RTL_CRITICAL_SECTION_DEBUG));
}


/*++
 * RtlpFreeDebugInfo
 *
 *     Frees the memory for a Critical Section Debug Object
 *
 * Params:
 *     DebugInfo - Pointer to Critical Section Debug Object to free.
 *
 * Returns:
 *     None.
 *
 * Remarks:
 *     If the pointer is part of the static buffer, then the entry is made
 *     free again. If not, the object is de-allocated from the heap.
 *
 *--*/
VOID
NTAPI
RtlpFreeDebugInfo(PRTL_CRITICAL_SECTION_DEBUG DebugInfo)
{
    SIZE_T EntryId;

    /* Is it part of our cached entries? */
    if ((DebugInfo >= RtlpStaticDebugInfo) &&
        (DebugInfo <= &RtlpStaticDebugInfo[MAX_STATIC_CS_DEBUG_OBJECTS-1]))
    {
        /* Yes. zero it out */
        RtlZeroMemory(DebugInfo, sizeof(RTL_CRITICAL_SECTION_DEBUG));

        /* Mark as free */
        EntryId = (DebugInfo - RtlpStaticDebugInfo);
        DPRINT("Freeing from Buffer: %p. Entry: %Iu inside Process: %p\n",
               DebugInfo,
               EntryId,
               NtCurrentTeb()->ClientId.UniqueProcess);
        RtlpDebugInfoFreeList[EntryId] = FALSE;

    }
    else if (!DebugInfo->Flags)
    {
        /* It's a dynamic one, so free from the heap */
        DPRINT("Freeing from Heap: %p inside Process: %p\n",
               DebugInfo,
               NtCurrentTeb()->ClientId.UniqueProcess);
        RtlFreeHeap(NtCurrentPeb()->ProcessHeap, 0, DebugInfo);
    }
    else
    {
        /* Wine stores a section name pointer in the Flags member */
        DPRINT("Assuming static: %p inside Process: %p\n",
               DebugInfo,
               NtCurrentTeb()->ClientId.UniqueProcess);
    }
}

/*++
 * RtlpInitDeferredCriticalSection
 *
 *     Initializes the Critical Section implementation.
 *
 * Params:
 *     None
 *
 * Returns:
 *     None.
 *
 * Remarks:
 *     After this call, the Process Critical Section list is protected.
 *
 *--*/
VOID
NTAPI
RtlpInitDeferredCriticalSection(VOID)
{
    /* Initialize the CS Protecting the List */
    RtlInitializeCriticalSection(&RtlCriticalSectionLock);

    /* It's now safe to enter it */
    RtlpCritSectInitialized = TRUE;
}

static void *no_debug_info_marker = (void *)(ULONG_PTR)-1;

/******************************************************************************
 *      RtlInitializeCriticalSectionEx   (NTDLL.@)
 */
NTSTATUS WINAPI RtlInitializeCriticalSectionEx( RTL_CRITICAL_SECTION *crit, ULONG spincount, ULONG flags )
{
    if (flags & (RTL_CRITICAL_SECTION_FLAG_DYNAMIC_SPIN|RTL_CRITICAL_SECTION_FLAG_STATIC_INIT))
        DbgPrint("(%p,%lu,0x%08lx) semi-stub\n", crit, spincount, flags);

    /* FIXME: if RTL_CRITICAL_SECTION_FLAG_STATIC_INIT is given, we should use
     * memory from a static pool to hold the debug info. Then heap.c could pass
     * this flag rather than initialising the process heap CS by hand. If this
     * is done, then debug info should be managed through Rtlp[Allocate|Free]DebugInfo
     * so (e.g.) MakeCriticalSectionGlobal() doesn't free it using HeapFree().
     */
    if (flags & RTL_CRITICAL_SECTION_FLAG_NO_DEBUG_INFO)
        crit->DebugInfo = no_debug_info_marker;
    else
    {
        crit->DebugInfo = RtlAllocateHeap( NtCurrentTeb()->ProcessEnvironmentBlock->ProcessHeap, 0, sizeof(RTL_CRITICAL_SECTION_DEBUG ));
        if (crit->DebugInfo)
        {
            crit->DebugInfo->Type = 0;
            crit->DebugInfo->CreatorBackTraceIndex = 0;
            crit->DebugInfo->CriticalSection = crit;
            crit->DebugInfo->ProcessLocksList.Blink = &crit->DebugInfo->ProcessLocksList;
            crit->DebugInfo->ProcessLocksList.Flink = &crit->DebugInfo->ProcessLocksList;
            crit->DebugInfo->EntryCount = 0;
            crit->DebugInfo->ContentionCount = 0;
            //memset( crit->DebugInfo->Spare, 0, sizeof(crit->DebugInfo->Spare) );
        }
    }
    crit->LockCount      = -1;
    crit->RecursionCount = 0;
    crit->OwningThread   = 0;
    crit->LockSemaphore  = 0;
    if (NtCurrentTeb()->ProcessEnvironmentBlock->NumberOfProcessors <= 1) spincount = 0;
    crit->SpinCount = spincount & ~0x80000000;
    return STATUS_SUCCESS;
}
