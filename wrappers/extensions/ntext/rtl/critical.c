 /*
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 *
 * Copyright (c) 2015  Microsoft Corporation 
 *  
 * Module Name:
 *
 *  critical.c 
 */
  
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

/*++
 * RtlInitializeCriticalSectionEx
 * @implemented NT6.0
 *
 *     Initialises a new critical section.
 *
 * Params:
 *     CriticalSection - Critical section to initialise
 *
 *     SpinCount - Spin count for the critical section.
 *
 *     Flags - Flags for initialization.
 *
 * Returns:
 *     STATUS_SUCCESS.
 *
 * Remarks:
 *     SpinCount is ignored on single-processor systems.
 *
 *--*/
// NTSTATUS
// NTAPI
// RtlInitializeCriticalSectionEx(
    // _Out_ PRTL_CRITICAL_SECTION CriticalSection,
    // _In_ ULONG SpinCount,
    // _In_ ULONG Flags)
// {
    // PRTL_CRITICAL_SECTION_DEBUG CritcalSectionDebugData;
    // ULONG AllowedFlags;
    // ULONG OsVersion;

    // /* Remove lower bits from flags */
    // Flags &= RTL_CRITICAL_SECTION_ALL_FLAG_BITS;

    // /* These flags generally allowed */
    // AllowedFlags = RTL_CRITICAL_SECTION_FLAG_NO_DEBUG_INFO |
                   // RTL_CRITICAL_SECTION_FLAG_DYNAMIC_SPIN |
                   // RTL_CRITICAL_SECTION_FLAG_STATIC_INIT;

    // /* Flags for Windows 7+ (CHECKME) */
    // OsVersion = NtCurrentPeb()->OSMajorVersion << 8 |
                // NtCurrentPeb()->OSMinorVersion;
    // if (OsVersion >= _WIN32_WINNT_WIN7)
    // {
        // AllowedFlags |= RTL_CRITICAL_SECTION_FLAG_RESOURCE_TYPE |
                        // RTL_CRITICAL_SECTION_FLAG_FORCE_DEBUG_INFO;
    // }

    // if (Flags & ~AllowedFlags)
    // {
        // return STATUS_INVALID_PARAMETER_3;
    // }

    // if (SpinCount & RTL_CRITICAL_SECTION_ALL_FLAG_BITS)
    // {
        // return STATUS_INVALID_PARAMETER_2;
    // }

    // /* First things first, set up the Object */
    // DPRINT("Initializing Critical Section: %p\n", CriticalSection);
    // CriticalSection->LockCount = -1;
    // CriticalSection->RecursionCount = 0;
    // CriticalSection->OwningThread = 0;
    // CriticalSection->SpinCount = (NtCurrentPeb()->NumberOfProcessors > 1) ? SpinCount : 0;
    // CriticalSection->LockSemaphore = 0;

    // if (Flags & RTL_CRITICAL_SECTION_FLAG_NO_DEBUG_INFO)
    // {
        // CriticalSection->DebugInfo = LongToPtr(-1);
    // }
    // else
    // {
        // /* Allocate the Debug Data */
        // CritcalSectionDebugData = RtlpAllocateDebugInfo();
        // DPRINT("Allocated Debug Data: %p inside Process: %p\n",
               // CritcalSectionDebugData,
               // NtCurrentTeb()->ClientId.UniqueProcess);

        // if (!CritcalSectionDebugData)
        // {
            // /* This is bad! */
            // DPRINT1("Couldn't allocate Debug Data for: %p\n", CriticalSection);
            // return STATUS_NO_MEMORY;
        // }

        // /* Set it up */
        // CritcalSectionDebugData->Type = RTL_CRITSECT_TYPE;
        // CritcalSectionDebugData->ContentionCount = 0;
        // CritcalSectionDebugData->EntryCount = 0;
        // CritcalSectionDebugData->CriticalSection = CriticalSection;
        // CritcalSectionDebugData->Flags = 0;
        // CriticalSection->DebugInfo = CritcalSectionDebugData;

        // /*
         // * Add it to the List of Critical Sections owned by the process.
         // * If we've initialized the Lock, then use it. If not, then probably
         // * this is the lock initialization itself, so insert it directly.
         // */
        // if ((CriticalSection != &RtlCriticalSectionLock) && (RtlpCritSectInitialized))
        // {
            // DPRINT("Securely Inserting into ProcessLocks: %p, %p, %p\n",
                   // &CritcalSectionDebugData->ProcessLocksList,
                   // CriticalSection,
                   // &RtlCriticalSectionList);

            // /* Protect List */
            // RtlEnterCriticalSection(&RtlCriticalSectionLock);

            // /* Add this one */
            // InsertTailList(&RtlCriticalSectionList, &CritcalSectionDebugData->ProcessLocksList);

            // /* Unprotect */
            // RtlLeaveCriticalSection(&RtlCriticalSectionLock);
        // }
        // else
        // {
            // DPRINT("Inserting into ProcessLocks: %p, %p, %p\n",
                   // &CritcalSectionDebugData->ProcessLocksList,
                   // CriticalSection,
                   // &RtlCriticalSectionList);

            // /* Add it directly */
            // InsertTailList(&RtlCriticalSectionList, &CritcalSectionDebugData->ProcessLocksList);
        // }
    // }

    // return STATUS_SUCCESS;
// }

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
