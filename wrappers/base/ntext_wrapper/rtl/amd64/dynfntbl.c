/*
 * PROJECT:     ReactOS RTL
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Dynamic function table support routines
 * COPYRIGHT:   Copyright 2022 Timo Kreuzer (timo.kreuzer@reactos.org)
 */

#include <main.h>
#include <debug.h>

#define TAG_RTLDYNFNTBL 'tfDP'

typedef
_Function_class_(GET_RUNTIME_FUNCTION_CALLBACK)
PRUNTIME_FUNCTION
GET_RUNTIME_FUNCTION_CALLBACK(
    _In_ DWORD64 ControlPc,
    _In_opt_ PVOID Context);
typedef GET_RUNTIME_FUNCTION_CALLBACK *PGET_RUNTIME_FUNCTION_CALLBACK;

typedef
_Function_class_(OUT_OF_PROCESS_FUNCTION_TABLE_CALLBACK)
DWORD
OUT_OF_PROCESS_FUNCTION_TABLE_CALLBACK(
    _In_ HANDLE Process,
    _In_ PVOID TableAddress,
    _Out_ PDWORD Entries,
    _Out_ PRUNTIME_FUNCTION* Functions);
typedef OUT_OF_PROCESS_FUNCTION_TABLE_CALLBACK *POUT_OF_PROCESS_FUNCTION_TABLE_CALLBACK;

typedef enum _FUNCTION_TABLE_TYPE
{
    RF_SORTED = 0x0,
    RF_UNSORTED = 0x1,
    RF_CALLBACK = 0x2,
    RF_KERNEL_DYNAMIC = 0x3,
} FUNCTION_TABLE_TYPE;

typedef struct _DYNAMIC_FUNCTION_TABLE
{
    LIST_ENTRY ListEntry;
    PRUNTIME_FUNCTION FunctionTable;
    LARGE_INTEGER TimeStamp;
    ULONG64 MinimumAddress;
    ULONG64 MaximumAddress;
    ULONG64 BaseAddress;
    PGET_RUNTIME_FUNCTION_CALLBACK Callback;
    PVOID Context;
    PWCHAR OutOfProcessCallbackDll;
    FUNCTION_TABLE_TYPE Type;
    ULONG EntryCount;
#if (NTDDI_VERSION <= NTDDI_WIN10)
    // FIXME: RTL_BALANCED_NODE is defined in ntdef.h, it's impossible to get included here due to precompiled header
    //RTL_BALANCED_NODE TreeNode;
#else
    //RTL_BALANCED_NODE TreeNodeMin;
    //RTL_BALANCED_NODE TreeNodeMax;
#endif
} DYNAMIC_FUNCTION_TABLE, *PDYNAMIC_FUNCTION_TABLE;

RTL_SRWLOCK RtlpDynamicFunctionTableLock = { 0 };
LIST_ENTRY RtlpDynamicFunctionTableList = { &RtlpDynamicFunctionTableList, &RtlpDynamicFunctionTableList };

static __inline
VOID
AcquireDynamicFunctionTableLockExclusive()
{
    RtlAcquireSRWLockExclusive(&RtlpDynamicFunctionTableLock);
}

static __inline
VOID
ReleaseDynamicFunctionTableLockExclusive()
{
    RtlReleaseSRWLockExclusive(&RtlpDynamicFunctionTableLock);
}

static __inline
VOID
AcquireDynamicFunctionTableLockShared()
{
    RtlAcquireSRWLockShared(&RtlpDynamicFunctionTableLock);
}

static __inline
VOID
ReleaseDynamicFunctionTableLockShared()
{
    RtlReleaseSRWLockShared(&RtlpDynamicFunctionTableLock);
}

/*
 * https://docs.microsoft.com/en-us/windows/win32/devnotes/rtlgetfunctiontablelisthead 
 */
PLIST_ENTRY
NTAPI
RtlGetFunctionTableListHead(void)
{
    return &RtlpDynamicFunctionTableList;
}

static
VOID
RtlpInsertDynamicFunctionTable(PDYNAMIC_FUNCTION_TABLE DynamicTable)
{
    //LARGE_INTEGER TimeStamp;

    AcquireDynamicFunctionTableLockExclusive();

    /* Insert it into the list */
    InsertTailList(&RtlpDynamicFunctionTableList, &DynamicTable->ListEntry);

    // TODO: insert into RB-trees

    ReleaseDynamicFunctionTableLockExclusive();
}

// DWORD
// NTAPI
// RtlAddGrowableFunctionTable(
    // _Out_ PVOID  *pDynamicTable,
    // _In_ PRUNTIME_FUNCTION FunctionTable,
    // _In_ DWORD EntryCount,
    // _In_ DWORD MaximumEntryCount,
    // _In_ ULONG_PTR RangeBase,
    // _In_ ULONG_PTR RangeEnd)
// {
    // PDYNAMIC_FUNCTION_TABLE dynamicTable;
    // //ULONG i;

    // /* Allocate a dynamic function table */
    // dynamicTable = RtlpAllocateMemory(sizeof(*dynamicTable), TAG_RTLDYNFNTBL);
    // if (dynamicTable == NULL)
    // {
        // DPRINT1("Failed to allocate dynamic function table\n");
        // return FALSE;
    // }

    // /* Initialize fields */
    // dynamicTable->FunctionTable = FunctionTable;
    // dynamicTable->EntryCount = EntryCount;
    // dynamicTable->BaseAddress = RangeBase;
    // dynamicTable->Callback = NULL;
    // dynamicTable->Context = NULL;
    // dynamicTable->Type = RF_UNSORTED;

    // /* Loop all entries to find the margins */
    // dynamicTable->MinimumAddress = ULONG64_MAX;
    // dynamicTable->MaximumAddress = RangeEnd;
    // // for (i = 0; i < EntryCount; i++)
    // // {
        // // dynamicTable->MinimumAddress = min(dynamicTable->MinimumAddress,
                                           // // FunctionTable[i].BeginAddress);
        // // dynamicTable->MaximumAddress = max(dynamicTable->MaximumAddress,
                                           // // FunctionTable[i].EndAddress);
    // // }

    // /* Insert the table into the list */
    // RtlpInsertDynamicFunctionTable(dynamicTable);
	
	// *pDynamicTable = dynamicTable;

    // return TRUE;
// }


// BOOLEAN
// NTAPI
// RtlDeleteGrowableFunctionTable(
    // _In_ PRUNTIME_FUNCTION FunctionTable)
// {
    // PLIST_ENTRY listLink;
    // PDYNAMIC_FUNCTION_TABLE dynamicTable;
    // BOOL removed = FALSE;

    // AcquireDynamicFunctionTableLockExclusive();

    // /* Loop all tables to find the one to delete */
    // for (listLink = RtlpDynamicFunctionTableList.Flink;
         // listLink != &RtlpDynamicFunctionTableList;
         // listLink = listLink->Flink)
    // {
        // dynamicTable = CONTAINING_RECORD(listLink, DYNAMIC_FUNCTION_TABLE, ListEntry);

        // if (dynamicTable->FunctionTable == FunctionTable)
        // {
            // RemoveEntryList(&dynamicTable->ListEntry);
            // removed = TRUE;
            // break;
        // }
    // }

    // ReleaseDynamicFunctionTableLockExclusive();

    // /* If we were successful, free the memory */
    // if (removed)
    // {
        // RtlpFreeMemory(dynamicTable, TAG_RTLDYNFNTBL);
    // }

    // return removed;
// }


struct dynamic_unwind_entry
{
    struct list       entry;
    ULONG_PTR         base;
    ULONG_PTR         end;
    RUNTIME_FUNCTION *table;
    DWORD             count;
    DWORD             max_count;
    PGET_RUNTIME_FUNCTION_CALLBACK callback;
    PVOID             context;
};

static struct list dynamic_unwind_list = LIST_INIT(dynamic_unwind_list);

static RTL_CRITICAL_SECTION dynamic_unwind_section;
static RTL_CRITICAL_SECTION_DEBUG dynamic_unwind_debug =
{
    0, 0, &dynamic_unwind_section,
    { &dynamic_unwind_debug.ProcessLocksList, &dynamic_unwind_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": dynamic_unwind_section") }
};
static RTL_CRITICAL_SECTION dynamic_unwind_section = { &dynamic_unwind_debug, -1, 0, 0, 0, 0 };

/*************************************************************************
 *              RtlAddGrowableFunctionTable   (NTDLL.@)
 */
DWORD WINAPI RtlAddGrowableFunctionTable( void **table, RUNTIME_FUNCTION *functions, DWORD count,
                                          DWORD max_count, ULONG_PTR base, ULONG_PTR end )
{
    struct dynamic_unwind_entry *entry;

    entry = RtlAllocateHeap( RtlGetProcessHeap(), 0, sizeof(*entry) );
    if (!entry)
        return STATUS_NO_MEMORY;

    entry->base      = base;
    entry->end       = end;
    entry->table     = functions;
    entry->count     = count;
    entry->max_count = max_count;
    entry->callback  = NULL;
    entry->context   = NULL;

    RtlEnterCriticalSection( &dynamic_unwind_section );
    list_add_tail( &dynamic_unwind_list, &entry->entry );
    RtlLeaveCriticalSection( &dynamic_unwind_section );

    *table = entry;

    return STATUS_SUCCESS;
}


/*************************************************************************
 *              RtlGrowFunctionTable   (NTDLL.@)
 */
void WINAPI RtlGrowFunctionTable( void *table, DWORD count )
{
    struct dynamic_unwind_entry *entry;

    RtlEnterCriticalSection( &dynamic_unwind_section );
    LIST_FOR_EACH_ENTRY( entry, &dynamic_unwind_list, struct dynamic_unwind_entry, entry )
    {
        if (entry == table)
        {
            if (count > entry->count && count <= entry->max_count)
                entry->count = count;
            break;
        }
    }
    RtlLeaveCriticalSection( &dynamic_unwind_section );
}


/*************************************************************************
 *              RtlDeleteGrowableFunctionTable   (NTDLL.@)
 */
void WINAPI RtlDeleteGrowableFunctionTable( void *table )
{
    struct dynamic_unwind_entry *entry, *to_free = NULL;

    RtlEnterCriticalSection( &dynamic_unwind_section );
    LIST_FOR_EACH_ENTRY( entry, &dynamic_unwind_list, struct dynamic_unwind_entry, entry )
    {
        if (entry == table)
        {
            to_free = entry;
            list_remove( &entry->entry );
            break;
        }
    }
    RtlLeaveCriticalSection( &dynamic_unwind_section );

    RtlFreeHeap( RtlGetProcessHeap(), 0, to_free );
}
