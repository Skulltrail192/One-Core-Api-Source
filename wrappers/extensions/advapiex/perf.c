/*++

Copyright (c) 2021  Shorthorn Project

Module Name:

    perf.c

Abstract:

    Performance counter functions
	
Author:

    Skulltrail 18-July-2021

Revision History:

--*/

#include "main.h"

WINE_DEFAULT_DEBUG_CHANNEL(perf);

struct counterset_template
{
    PERF_COUNTERSET_INFO counterset;
    PERF_COUNTER_INFO counter[1];
};

struct counterset_instance
{
    struct list entry;
    struct counterset_template *template;
    PERF_COUNTERSET_INSTANCE instance;
};

struct perf_provider
{
    GUID guid;
    PERFLIBREQUEST callback;
    struct counterset_template **countersets;
    unsigned int counterset_count;

    struct list instance_list;
};

static struct perf_provider *perf_provider_from_handle(HANDLE prov)
{
    return (struct perf_provider *)prov;
}

typedef struct _PERF_INSTANCE_HEADER {
    ULONG Size;
    ULONG InstanceId;
} PERF_INSTANCE_HEADER, *PPERF_INSTANCE_HEADER;

typedef enum _PerfRegInfoType {
    PERF_REG_COUNTERSET_STRUCT = 1,
    PERF_REG_COUNTER_STRUCT,
    PERF_REG_COUNTERSET_NAME_STRING,
    PERF_REG_COUNTERSET_HELP_STRING,
    PERF_REG_COUNTER_NAME_STRINGS,
    PERF_REG_COUNTER_HELP_STRINGS,
    PERF_REG_PROVIDER_NAME,
    PERF_REG_PROVIDER_GUID,
    PERF_REG_COUNTERSET_ENGLISH_NAME,
    PERF_REG_COUNTER_ENGLISH_NAMES
} PerfRegInfoType;

/***********************************************************************
 *           PerfCreateInstance   (kernelex.@)
 */
PPERF_COUNTERSET_INSTANCE WINAPI PerfCreateInstance( 
	HANDLE handle, 
	const GUID *guid,
    const WCHAR *name, 
	ULONG id )
{
    struct perf_provider *prov = perf_provider_from_handle( handle );
    struct counterset_template *template;
    struct counterset_instance *inst;
    unsigned int i;
    ULONG size;

    FIXME( "handle %p, guid %s, name %s, id %u semi-stub.\n", handle, debugstr_guid(guid), debugstr_w(name), id );

    if (!prov || !guid || !name)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return NULL;
    }

    for (i = 0; i < prov->counterset_count; ++i)
        if (IsEqualGUID(guid, &prov->countersets[i]->counterset.CounterSetGuid)) break;

    if (i == prov->counterset_count)
    {
        SetLastError( ERROR_NOT_FOUND );
        return NULL;
    }

    template = prov->countersets[i];

    LIST_FOR_EACH_ENTRY(inst, &prov->instance_list, struct counterset_instance, entry)
    {
        if (inst->template == template && inst->instance.InstanceId == id)
        {
            SetLastError( ERROR_ALREADY_EXISTS );
            return NULL;
        }
    }

    size = (sizeof(PERF_COUNTERSET_INSTANCE) + template->counterset.NumCounters * sizeof(UINT64)
            + (lstrlenW( name ) + 1) * sizeof(WCHAR) + 7) & ~7;
    inst = heap_alloc_zero( offsetof(struct counterset_instance, instance) + size );
    if (!inst)
    {
        SetLastError( ERROR_OUTOFMEMORY );
        return NULL;
    }

    inst->template = template;
    inst->instance.CounterSetGuid = *guid;
    inst->instance.dwSize = size;
    inst->instance.InstanceId = id;
    inst->instance.InstanceNameOffset = sizeof(PERF_COUNTERSET_INSTANCE)
                                        + template->counterset.NumCounters * sizeof(UINT64);
    inst->instance.InstanceNameSize = (lstrlenW( name ) + 1) * sizeof(WCHAR);
    memcpy( (BYTE *)&inst->instance + inst->instance.InstanceNameOffset, name, inst->instance.InstanceNameSize );
    list_add_tail( &prov->instance_list, &inst->entry );

    return &inst->instance;
}


/***********************************************************************
 *           PerfDeleteInstance   (kernelex.@)
 */
ULONG WINAPI PerfDeleteInstance(HANDLE provider, PERF_COUNTERSET_INSTANCE *block)
{
    struct perf_provider *prov = perf_provider_from_handle( provider );
    struct counterset_instance *inst;

    TRACE( "provider %p, block %p.\n", provider, block );

    if (!prov || !block) return ERROR_INVALID_PARAMETER;

    inst = CONTAINING_RECORD(block, struct counterset_instance, instance);
    list_remove( &inst->entry );
    heap_free( inst );

    return ERROR_SUCCESS;
}

/***********************************************************************
 *           PerfSetCounterSetInfo   (kernelex.@)
 */
ULONG WINAPI PerfSetCounterSetInfo( HANDLE handle, PERF_COUNTERSET_INFO *template, ULONG size )
{
    struct perf_provider *prov = perf_provider_from_handle( handle );
    struct counterset_template **new_array;
    struct counterset_template *new;
    unsigned int i;

    FIXME( "handle %p, template %p, size %u semi-stub.\n", handle, template, size );

    if (!prov || !template) return ERROR_INVALID_PARAMETER;
    if (!template->NumCounters) return ERROR_INVALID_PARAMETER;
    if (size < sizeof(*template) || (size - (sizeof(*template))) / sizeof(PERF_COUNTER_INFO) < template->NumCounters)
        return ERROR_INVALID_PARAMETER;

    for (i = 0; i < prov->counterset_count; ++i)
    {
        if (IsEqualGUID( &template->CounterSetGuid, &prov->countersets[i]->counterset.CounterSetGuid ))
            return ERROR_ALREADY_EXISTS;
    }

    size = offsetof( struct counterset_template, counter[template->NumCounters] );
    if (!(new = heap_alloc( size ))) return ERROR_OUTOFMEMORY;

    if (prov->counterset_count)
        new_array = heap_realloc( prov->countersets, sizeof(*prov->countersets) * (prov->counterset_count + 1) );
    else
        new_array = heap_alloc( sizeof(*prov->countersets) );

    if (!new_array)
    {
        heap_free( new );
        return ERROR_OUTOFMEMORY;
    }
    memcpy( new, template, size );
    for (i = 0; i < template->NumCounters; ++i)
        new->counter[i].Offset = i * sizeof(UINT64);
    new_array[prov->counterset_count++] = new;
    prov->countersets = new_array;

    return STATUS_SUCCESS;
}


/***********************************************************************
 *           PerfSetCounterRefValue   (kernelex.@)
 */
ULONG WINAPI PerfSetCounterRefValue(HANDLE provider, PERF_COUNTERSET_INSTANCE *instance,
                                    ULONG counterid, void *address)
{
    struct perf_provider *prov = perf_provider_from_handle( provider );
    struct counterset_template *template;
    struct counterset_instance *inst;
    unsigned int i;

    FIXME( "provider %p, instance %p, counterid %u, address %p semi-stub.\n",
           provider, instance, counterid, address );

    if (!prov || !instance || !address) return ERROR_INVALID_PARAMETER;

    inst = CONTAINING_RECORD(instance, struct counterset_instance, instance);
    template = inst->template;

    for (i = 0; i < template->counterset.NumCounters; ++i)
        if (template->counter[i].CounterId == counterid) break;

    if (i == template->counterset.NumCounters) return ERROR_NOT_FOUND;
    *(void **)((BYTE *)&inst->instance + sizeof(PERF_COUNTERSET_INSTANCE) + template->counter[i].Offset) = address;

    return STATUS_SUCCESS;
}

/***********************************************************************
 *           PerfStartProviderEx   (kernelex.@)
 */
ULONG WINAPI PerfStartProviderEx( GUID *guid, PERF_PROVIDER_CONTEXT *context, HANDLE *provider )
{
    struct perf_provider *prov;

    FIXME( "guid %s, context %p, provider %p semi-stub.\n", debugstr_guid(guid), context, provider );

    if (!guid || !context || !provider) return ERROR_INVALID_PARAMETER;
    if (context->ContextSize < sizeof(*context)) return ERROR_INVALID_PARAMETER;

    if (context->MemAllocRoutine || context->MemFreeRoutine)
        FIXME("Memory allocation routine is not supported.\n");

    if (!(prov = heap_alloc_zero( sizeof(*prov) ))) return ERROR_OUTOFMEMORY;
    list_init( &prov->instance_list );
    memcpy( &prov->guid, guid, sizeof(prov->guid) );
    prov->callback = context->ControlCallback;
    *provider = prov;

    return STATUS_SUCCESS;
}

/***********************************************************************
 *           PerfStartProvider   (kernelex.@)
 */
ULONG WINAPI PerfStartProvider( GUID *guid, PERFLIBREQUEST callback, HANDLE *provider )
{
    PERF_PROVIDER_CONTEXT ctx;

    FIXME( "guid %s, callback %p, provider %p semi-stub.\n", debugstr_guid(guid), callback, provider );

    memset( &ctx, 0, sizeof(ctx) );
    ctx.ContextSize = sizeof(ctx);
    ctx.ControlCallback = callback;

    return PerfStartProviderEx( guid, &ctx, provider );
}

/***********************************************************************
 *           PerfStopProvider   (kernelex.@)
 */
ULONG WINAPI PerfStopProvider(HANDLE handle)
{
    struct perf_provider *prov = perf_provider_from_handle( handle );
    struct counterset_instance *inst, *next;
    unsigned int i;

    TRACE( "handle %p.\n", handle );

    if (!list_empty( &prov->instance_list ))
        WARN( "Stopping provider with active counter instances.\n" );

    LIST_FOR_EACH_ENTRY_SAFE(inst, next, &prov->instance_list, struct counterset_instance, entry)
    {
        list_remove( &inst->entry );
        heap_free( inst );
    }

    for (i = 0; i < prov->counterset_count; ++i)
        heap_free( prov->countersets[i] );
    heap_free( prov->countersets );
    heap_free( prov );
    return STATUS_SUCCESS;
}

LONG WINAPI PerfCloseQueryHandle( HANDLE query )
{
    FIXME( "query %p stub.\n", query );

    return ERROR_SUCCESS;
}

ULONG WINAPI PerfOpenQueryHandle( const WCHAR *machine, HANDLE *query )
{
    FIXME( "machine %s, query %p.\n", debugstr_w(machine), query );

    if (!query) return ERROR_INVALID_PARAMETER;
    *query = (HANDLE)0xdeadbeef;

    return ERROR_SUCCESS;
}

ULONG WINAPI PerfAddCounters( HANDLE query, PERF_COUNTER_IDENTIFIER *id, DWORD size )
{
    FIXME( "query %p, id %p, size %lu stub.\n", query, id, size );

    if (!id || size < sizeof(*id) || id->Size < sizeof(*id)) return ERROR_INVALID_PARAMETER;

    id->Status = ERROR_WMI_GUID_NOT_FOUND;
    return ERROR_SUCCESS;
}

ULONG WINAPI PerfQueryCounterData( HANDLE query, PERF_DATA_HEADER *data, DWORD data_size, DWORD *size_needed )
{
    FIXME( "query %p, data %p, data_size %lu, size_needed %p stub.\n", query, data, data_size, size_needed );

    if (!size_needed) return ERROR_INVALID_PARAMETER;

    *size_needed = sizeof(PERF_DATA_HEADER);

    if (!data || data_size < sizeof(PERF_DATA_HEADER)) return ERROR_NOT_ENOUGH_MEMORY;

    data->dwTotalSize = sizeof(PERF_DATA_HEADER);
    data->dwNumCounters = 0;
    QueryPerformanceCounter( (LARGE_INTEGER *)&data->PerfTimeStamp );
    QueryPerformanceFrequency( (LARGE_INTEGER *)&data->PerfFreq );
    GetSystemTimeAsFileTime( (FILETIME *)&data->PerfTime100NSec );
    FileTimeToSystemTime( (FILETIME *)&data->PerfTime100NSec, &data->SystemTime );

    return ERROR_SUCCESS;
}

ULONG WINAPI PerfSetULongCounterValue(
  HANDLE                    Provider,
  PPERF_COUNTERSET_INSTANCE Instance,
  ULONG                     CounterId,
  ULONG                     Value
)
{
    return ERROR_SUCCESS;
}

ULONG WINAPI PerfSetULongLongCounterValue(
  HANDLE                    Provider,
  PPERF_COUNTERSET_INSTANCE Instance,
  ULONG                     CounterId,
  ULONGLONG                 Value
)
{
    return ERROR_SUCCESS;
}

ULONG WINAPI PerfDecrementULongCounterValue(
  HANDLE                    Provider,
  PPERF_COUNTERSET_INSTANCE Instance,
  ULONG                     CounterId,
  ULONG                     Value
)
{
    return ERROR_SUCCESS;
}

ULONG WINAPI PerfDecrementULongLongCounterValue(
  HANDLE                    Provider,
  PPERF_COUNTERSET_INSTANCE Instance,
  ULONG                     CounterId,
  ULONGLONG                 Value
)
{
    return ERROR_SUCCESS;
}

ULONG WINAPI PerfDeleteCounters(
  HANDLE                   hQuery,
  PPERF_COUNTER_IDENTIFIER pCounters,
  DWORD                    cbCounters
)
{
    return ERROR_SUCCESS;
}

ULONG WINAPI PerfEnumerateCounterSet(
   LPCWSTR szMachine,
   LPGUID  pCounterSetIds,
   DWORD   cCounterSetIds,
   DWORD *pcCounterSetIdsActual
)
{
    return ERROR_SUCCESS;
}

ULONG WINAPI PerfEnumerateCounterSetInstances(
   LPCWSTR               szMachine,
   const GUID            *pCounterSetId,
   PERF_INSTANCE_HEADER  pInstances,
   DWORD                 cbInstances,
   DWORD               *pcbInstancesActual
)
{
    return ERROR_SUCCESS;
}

ULONG WINAPI PerfIncrementULongCounterValue(
   HANDLE                    Provider,
   PPERF_COUNTERSET_INSTANCE Instance,
   ULONG                     CounterId,
   ULONG                     Value
)
{
    return ERROR_SUCCESS;
}

ULONG WINAPI PerfIncrementULongLongCounterValue(
   HANDLE                    Provider,
   PPERF_COUNTERSET_INSTANCE Instance,
   ULONG                     CounterId,
   ULONGLONG                 Value
)
{
    return ERROR_SUCCESS;
}

PPERF_COUNTERSET_INSTANCE WINAPI PerfQueryInstance(
	HANDLE handle, 
	const GUID *guid,
    const WCHAR *name, 
	ULONG id 
)
{
    struct perf_provider *prov = perf_provider_from_handle( handle );
    struct counterset_template *template;
    struct counterset_instance *inst;
    unsigned int i;

    FIXME( "handle %p, guid %s, name %s, id %u semi-stub.\n", handle, debugstr_guid(guid), debugstr_w(name), id );

    if (!prov || !guid || !name)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return NULL;
    }

    for (i = 0; i < prov->counterset_count; ++i)
        if (IsEqualGUID(guid, &prov->countersets[i]->counterset.CounterSetGuid)) break;

    if (i == prov->counterset_count)
    {
        SetLastError( ERROR_NOT_FOUND );
        return NULL;
    }

    template = prov->countersets[i];

    LIST_FOR_EACH_ENTRY(inst, &prov->instance_list, struct counterset_instance, entry)
    {
        if (inst->template == template && inst->instance.InstanceId == id)
        {
            return &inst->instance;
        }
    }

    /*Not found the instance*/
	SetLastError(ERROR_NOT_FOUND);
    return NULL;
}

ULONG WINAPI PerfQueryCounterInfo( HANDLE query, PPERF_COUNTER_IDENTIFIER data, DWORD data_size, DWORD *size_needed )
{
    FIXME( "query %p, data %p, data_size %lu, size_needed %p stub.\n", query, data, data_size, size_needed );

    // if (!size_needed) return ERROR_INVALID_PARAMETER;

    // *size_needed = sizeof(PERF_COUNTER_IDENTIFIER);

    // if (!data || data_size < sizeof(PERF_COUNTER_IDENTIFIER)) return ERROR_NOT_ENOUGH_MEMORY;

    // data->dwTotalSize = sizeof(PERF_COUNTER_IDENTIFIER );
    // data->dwNumCounters = 0;
    // QueryPerformanceCounter( (LARGE_INTEGER *)&data->PerfTimeStamp );
    // QueryPerformanceFrequency( (LARGE_INTEGER *)&data->PerfFreq );
    // GetSystemTimeAsFileTime( (FILETIME *)&data->PerfTime100NSec );
    // FileTimeToSystemTime( (FILETIME *)&data->PerfTime100NSec, &data->SystemTime );

    return ERROR_NOT_FOUND;
}

ULONG PerfQueryCounterSetRegistrationInfo(
    LPCWSTR         szMachine,
    LPCGUID         pCounterSetId,
    PerfRegInfoType requestCode,
    DWORD           requestLangId,
    LPBYTE          pbRegInfo,
    DWORD           cbRegInfo,
    DWORD           *pcbRegInfoActual
)
{
	 return ERROR_NOT_FOUND;
}
