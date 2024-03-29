/*
 * Network Store Interface
 *
 * Copyright 2021 Huw Davies
 *
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
 */
#include "winsock2.h"
#include "wine/winternl.h"
#include "ws2ipdef.h"
#include "iphlpapi.h"
#include "netioapi.h"
#include "iptypes.h"
#include "netiodef.h"
#include "wine/nsi.h"
#include "wine/heap.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(nsi);

static inline HANDLE get_nsi_device( void )
{
    return CreateFileW( L"\\\\.\\Nsi", 0, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL );
}

DWORD WINAPI NsiAllocateAndGetTable( DWORD unk, const NPI_MODULEID *module, DWORD table, void **key_data, DWORD key_size,
                                     void **rw_data, DWORD rw_size, void **dynamic_data, DWORD dynamic_size,
                                     void **static_data, DWORD static_size, DWORD *count, DWORD unk2 )
{
    DWORD err, num = 0;
    void *data[4] = { NULL };
    DWORD sizes[4] = { key_size, rw_size, dynamic_size, static_size };
    int i, attempt;

    TRACE( "%d %p %d %p %d %p %d %p %d %p %d %p %d\n", unk, module, table, key_data, key_size,
           rw_data, rw_size, dynamic_data, dynamic_size, static_data, static_size, count, unk2 );

    for (attempt = 0; attempt < 5; attempt++)
    {
        err = NsiEnumerateObjectsAllParameters( unk, 0, module, table, NULL, 0, NULL, 0, NULL, 0, NULL, 0, &num );
        if (err) return err;

        for (i = 0; i < ARRAY_SIZE(data); i++)
        {
            if (sizes[i])
            {
                data[i] = heap_alloc( sizes[i] * num );
                if (!data[i])
                {
                    err = ERROR_OUTOFMEMORY;
                    goto err;
                }
            }
        }

        err = NsiEnumerateObjectsAllParameters( unk, 0, module, table, data[0], sizes[0], data[1], sizes[1],
                                                data[2], sizes[2], data[3], sizes[3], &num );
        if (err != ERROR_MORE_DATA) break;

        NsiFreeTable( data[0], data[1], data[2], data[3] );
        memset( data, 0, sizeof(data) );
    }

    if (!err)
    {
        if (sizes[0]) *key_data = data[0];
        if (sizes[1]) *rw_data = data[1];
        if (sizes[2]) *dynamic_data = data[2];
        if (sizes[3]) *static_data = data[3];
        *count = num;
    }

err:
    if (err) NsiFreeTable( data[0], data[1], data[2], data[3] );
    return err;
}

DWORD WINAPI NsiEnumerateObjectsAllParameters( DWORD unk, DWORD unk2, const NPI_MODULEID *module, DWORD table,
                                               void *key_data, DWORD key_size, void *rw_data, DWORD rw_size,
                                               void *dynamic_data, DWORD dynamic_size, void *static_data, DWORD static_size,
                                               DWORD *count )
{
    struct nsi_enumerate_all_ex params;
    DWORD err;

    TRACE( "%d %d %p %d %p %d %p %d %p %d %p %d %p\n", unk, unk2, module, table, key_data, key_size,
           rw_data, rw_size, dynamic_data, dynamic_size, static_data, static_size, count );

    params.unknown[0] = 0;
    params.unknown[1] = 0;
    params.module = module;
    params.table = table;
    params.first_arg = unk;
    params.second_arg = unk2;
    params.key_data = key_data;
    params.key_size = key_size;
    params.rw_data = rw_data;
    params.rw_size = rw_size;
    params.dynamic_data = dynamic_data;
    params.dynamic_size = dynamic_size;
    params.static_data = static_data;
    params.static_size = static_size;
    params.count = *count;

    err = NsiEnumerateObjectsAllParametersEx( &params );
    *count = params.count;
    return err;
}

DWORD WINAPI NsiEnumerateObjectsAllParametersEx( struct nsi_enumerate_all_ex *params )
{
    DWORD out_size, received, err = ERROR_SUCCESS;
    HANDLE device = get_nsi_device();
    struct nsiproxy_enumerate_all in;
    BYTE *out, *ptr;

    if (device == INVALID_HANDLE_VALUE) return GetLastError();

    out_size = sizeof(DWORD) +
        (params->key_size + params->rw_size + params->dynamic_size + params->static_size) * params->count;

    out = heap_alloc( out_size );
    if (!out)
    {
        CloseHandle( device );
        return ERROR_OUTOFMEMORY;
    }

    in.module = *params->module;
    in.first_arg = params->first_arg;
    in.second_arg = params->second_arg;
    in.table = params->table;
    in.key_size = params->key_size;
    in.rw_size = params->rw_size;
    in.dynamic_size = params->dynamic_size;
    in.static_size = params->static_size;
    in.count = params->count;

    if (!DeviceIoControl( device, IOCTL_NSIPROXY_WINE_ENUMERATE_ALL, &in, sizeof(in), out, out_size, &received, NULL ))
        err = GetLastError();
    if (err == ERROR_SUCCESS || err == ERROR_MORE_DATA)
    {
        params->count = *(DWORD *)out;
        ptr = out + sizeof(DWORD);
        if (params->key_size) memcpy( params->key_data, ptr, params->key_size * params->count );
        ptr += params->key_size * in.count;
        if (params->rw_size) memcpy( params->rw_data, ptr, params->rw_size * params->count );
        ptr += params->rw_size * in.count;
        if (params->dynamic_size) memcpy( params->dynamic_data, ptr, params->dynamic_size * params->count );
        ptr += params->dynamic_size * in.count;
        if (params->static_size) memcpy( params->static_data, ptr, params->static_size * params->count );
    }

    heap_free( out );
    CloseHandle( device );

    return err;
}

void WINAPI NsiFreeTable( void *key_data, void *rw_data, void *dynamic_data, void *static_data )
{
    TRACE( "%p %p %p %p\n", key_data, rw_data, dynamic_data, static_data );
    heap_free( key_data );
    heap_free( rw_data );
    heap_free( dynamic_data );
    heap_free( static_data );
}

DWORD WINAPI NsiGetAllParameters( DWORD unk, const NPI_MODULEID *module, DWORD table, const void *key, DWORD key_size,
                                  void *rw_data, DWORD rw_size, void *dynamic_data, DWORD dynamic_size,
                                  void *static_data, DWORD static_size )
{
    struct nsi_get_all_parameters_ex params;

    TRACE( "%d %p %d %p %d %p %d %p %d %p %d\n", unk, module, table, key, key_size,
           rw_data, rw_size, dynamic_data, dynamic_size, static_data, static_size );

    params.unknown[0] = 0;
    params.unknown[1] = 0;
    params.module = module;
    params.table = table;
    params.first_arg = unk;
    params.unknown2 = 0;
    params.key = key;
    params.key_size = key_size;
    params.rw_data = rw_data;
    params.rw_size = rw_size;
    params.dynamic_data = dynamic_data;
    params.dynamic_size = dynamic_size;
    params.static_data = static_data;
    params.static_size = static_size;

    return NsiGetAllParametersEx( &params );
}

DWORD WINAPI NsiGetAllParametersEx( struct nsi_get_all_parameters_ex *params )
{
    HANDLE device = get_nsi_device();
    struct nsiproxy_get_all_parameters *in;
    ULONG in_size = FIELD_OFFSET( struct nsiproxy_get_all_parameters, key[params->key_size] ), received;
    ULONG out_size = params->rw_size + params->dynamic_size + params->static_size;
    DWORD err = ERROR_SUCCESS;
    BYTE *out, *ptr;

    if (device == INVALID_HANDLE_VALUE) return GetLastError();

    in = heap_alloc( in_size );
    out = heap_alloc( out_size );
    if (!in || !out)
    {
        err = ERROR_OUTOFMEMORY;
        goto err;
    }

    in->module = *params->module;
    in->first_arg = params->first_arg;
    in->table = params->table;
    in->key_size = params->key_size;
    in->rw_size = params->rw_size;
    in->dynamic_size = params->dynamic_size;
    in->static_size = params->static_size;
    memcpy( in->key, params->key, params->key_size );

    if (!DeviceIoControl( device, IOCTL_NSIPROXY_WINE_GET_ALL_PARAMETERS, in, in_size, out, out_size, &received, NULL ))
        err = GetLastError();
    if (err == ERROR_SUCCESS)
    {
        ptr = out;
        if (params->rw_size) memcpy( params->rw_data, ptr, params->rw_size );
        ptr += params->rw_size;
        if (params->dynamic_size) memcpy( params->dynamic_data, ptr, params->dynamic_size );
        ptr += params->dynamic_size;
        if (params->static_size) memcpy( params->static_data, ptr, params->static_size );
    }

err:
    heap_free( out );
    heap_free( in );
    CloseHandle( device );
    return err;
}

DWORD WINAPI NsiGetParameter( DWORD unk, const NPI_MODULEID *module, DWORD table, const void *key, DWORD key_size,
                              DWORD param_type, void *data, DWORD data_size, DWORD data_offset )
{
    struct nsi_get_parameter_ex params;

    TRACE( "%d %p %d %p %d %d %p %d %d\n", unk, module, table, key, key_size,
           param_type, data, data_size, data_offset );

    params.unknown[0] = 0;
    params.unknown[1] = 0;
    params.module = module;
    params.table = table;
    params.first_arg = unk;
    params.unknown2 = 0;
    params.key = key;
    params.key_size = key_size;
    params.param_type = param_type;
    params.data = data;
    params.data_size = data_size;
    params.data_offset = data_offset;
    return NsiGetParameterEx( &params );
}

DWORD WINAPI NsiGetParameterEx( struct nsi_get_parameter_ex *params )
{
    HANDLE device = get_nsi_device();
    struct nsiproxy_get_parameter *in;
    ULONG in_size = FIELD_OFFSET( struct nsiproxy_get_parameter, key[params->key_size] ), received;
    DWORD err = ERROR_SUCCESS;

    if (device == INVALID_HANDLE_VALUE) return GetLastError();

    in = heap_alloc( in_size );
    if (!in)
    {
        err = ERROR_OUTOFMEMORY;
        goto err;
    }
    in->module = *params->module;
    in->first_arg = params->first_arg;
    in->table = params->table;
    in->key_size = params->key_size;
    in->param_type = params->param_type;
    in->data_offset = params->data_offset;
    memcpy( in->key, params->key, params->key_size );

    if (!DeviceIoControl( device, IOCTL_NSIPROXY_WINE_GET_PARAMETER, in, in_size, params->data, params->data_size, &received, NULL ))
        err = GetLastError();

err:
    heap_free( in );
    CloseHandle( device );
    return err;
}
