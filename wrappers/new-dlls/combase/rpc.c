/*++

Copyright (c) 2023 Shorthorn Project

Module Name:

    main.c

Abstract:

    This module implements COM RPC functions APIs

Author:

    Skulltrail 12-October-2023

Revision History:

--*/
 
#include "main.h"
 
WINE_DEFAULT_DEBUG_CHANNEL(rpc);

static struct list registered_classes;
static CRITICAL_SECTION registered_classes_cs;
 
 static RPC_BINDING_HANDLE get_rpc_handle(unsigned short *protseq, unsigned short *endpoint)
{
    RPC_BINDING_HANDLE handle = NULL;
    RPC_STATUS status;
    RPC_WSTR binding;

    status = RpcStringBindingComposeW(NULL, protseq, NULL, endpoint, NULL, &binding);
    if (status == RPC_S_OK)
    {
        status = RpcBindingFromStringBindingW(binding, &handle);
        RpcStringFreeW(&binding);
    }

    return handle;
}
 
 static RPC_BINDING_HANDLE get_irpcss_handle(void)
{
    static RPC_BINDING_HANDLE irpcss_handle;

    if (!irpcss_handle)
    {
        unsigned short protseq[] = IRPCSS_PROTSEQ;
        unsigned short endpoint[] = IRPCSS_ENDPOINT;

        RPC_BINDING_HANDLE new_handle = get_rpc_handle(protseq, endpoint);
        if (InterlockedCompareExchangePointer(&irpcss_handle, new_handle, NULL))
            /* another thread beat us to it */
            RpcBindingFree(&new_handle);
    }
    return irpcss_handle;
}

static void scm_revoke_class(struct registered_class *_class)
{
    list_remove(&_class->entry);
    free(_class->object);
    free(_class);
}
 
 /* From rpcss */
HRESULT __cdecl irpcss_server_revoke(handle_t h, unsigned int cookie)
{
    struct registered_class *cur;

    EnterCriticalSection(&registered_classes_cs);

    LIST_FOR_EACH_ENTRY(cur, &registered_classes, struct registered_class, entry)
    {
        if (cur->cookie == cookie)
        {
            scm_revoke_class(cur);
            break;
        }
    }

    LeaveCriticalSection(&registered_classes_cs);

    return S_OK;
}

HRESULT rpc_revoke_local_server(unsigned int cookie)
{
    RPCSS_CALL_START
    hr = irpcss_server_revoke(get_irpcss_handle(), cookie);
    RPCSS_CALL_END
}


static BOOL start_rpcss(void)
{
    SERVICE_STATUS_PROCESS status;
    SC_HANDLE scm, service;
    BOOL ret = FALSE;

    TRACE("\n");

    if (!(scm = OpenSCManagerW(NULL, NULL, 0)))
    {
        ERR("Failed to open service manager\n");
        return FALSE;
    }

    if (!(service = OpenServiceW(scm, L"RpcSs", SERVICE_START | SERVICE_QUERY_STATUS)))
    {
        ERR("Failed to open RpcSs service\n");
        CloseServiceHandle( scm );
        return FALSE;
    }

    if (StartServiceW(service, 0, NULL) || GetLastError() == ERROR_SERVICE_ALREADY_RUNNING)
    {
        ULONGLONG start_time = GetTickCount();
        do
        {
            DWORD dummy;

            if (!QueryServiceStatusEx(service, SC_STATUS_PROCESS_INFO, (BYTE *)&status, sizeof(status), &dummy))
                break;
            if (status.dwCurrentState == SERVICE_RUNNING)
            {
                ret = TRUE;
                break;
            }
            if (GetTickCount() - start_time > 30000) break;
            Sleep( 100 );

        } while (status.dwCurrentState == SERVICE_START_PENDING);

        if (status.dwCurrentState != SERVICE_RUNNING)
            WARN("RpcSs failed to start %lu\n", status.dwCurrentState);
    }
    else
        ERR("Failed to start RpcSs service\n");

    CloseServiceHandle(service);
    CloseServiceHandle(scm);
    return ret;
}

static LONG WINAPI rpc_filter(EXCEPTION_POINTERS *eptr)
{
    return I_RpcExceptionFilter(eptr->ExceptionRecord->ExceptionCode);
}


/******************************************************************************
 *            CoDecodeProxy    (combase.@)
 */
HRESULT WINAPI CoDecodeProxy(DWORD client_pid, UINT64 proxy_addr, ServerInformation *server_info)
{
    FIXME("%lx, %s, %p.\n", client_pid, proxy_addr, server_info);
    return E_NOTIMPL;
}
