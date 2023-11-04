/*++

Copyright (c) 2023 Shorthorn Project

Module Name:

    main.c

Abstract:

    This module implements COM Marshaling functions APIs

Author:

    Skulltrail 12-October-2023

Revision History:

--*/

#define WIN32_NO_STATUS

#include "main.h"

WINE_DEFAULT_DEBUG_CHANNEL(ole32);

static HRESULT ifproxy_release_public_refs(struct ifproxy * This)
{
    HRESULT hr = S_OK;
    LONG public_refs;

    if (WAIT_OBJECT_0 != WaitForSingleObject(This->parent->remoting_mutex, INFINITE))
    {
        ERR("Wait failed for ifproxy %p\n", This);
        return E_UNEXPECTED;
    }

    public_refs = This->refs;
    if (public_refs > 0)
    {
        IRemUnknown *remunk = NULL;

        TRACE("releasing %ld refs\n", public_refs);

        hr = proxy_manager_get_remunknown(This->parent, &remunk);
        if (hr == S_OK)
        {
            REMINTERFACEREF rif;
            rif.ipid = This->stdobjref.ipid;
            rif.cPublicRefs = public_refs;
            rif.cPrivateRefs = 0;
            hr = IRemUnknown_RemRelease(remunk, 1, &rif);
            IRemUnknown_Release(remunk);
            if (hr == S_OK)
                InterlockedExchangeAdd((LONG *)&This->refs, -public_refs);
            else if (hr == RPC_E_DISCONNECTED)
                WARN("couldn't release references because object was "
                     "disconnected: oxid = %s, oid = %s\n",
                     wine_dbgstr_longlong(This->parent->oxid),
                     wine_dbgstr_longlong(This->parent->oid));
            else
                ERR("IRemUnknown_RemRelease failed with error %#lx\n", hr);
        }
    }
    ReleaseMutex(This->parent->remoting_mutex);

    return hr;
}

/* should be called inside This->parent->cs critical section */
static void ifproxy_disconnect(struct ifproxy * This)
{
    ifproxy_release_public_refs(This);
    if (This->proxy) IRpcProxyBuffer_Disconnect(This->proxy);

    IRpcChannelBuffer_Release(This->chan);
    This->chan = NULL;
}

static void proxy_manager_disconnect(struct proxy_manager * This)
{
    struct ifproxy *ifproxy;

    TRACE("oxid = %s, oid = %s\n", wine_dbgstr_longlong(This->oxid),
        wine_dbgstr_longlong(This->oid));

    EnterCriticalSection(&This->cs);

    /* SORFP_NOLIFTIMEMGMT proxies (for IRemUnknown) shouldn't be
     * disconnected - it won't do anything anyway, except cause
     * problems for other objects that depend on this proxy always
     * working */
    if (!(This->sorflags & SORFP_NOLIFETIMEMGMT))
    {
        LIST_FOR_EACH_ENTRY(ifproxy, &This->interfaces, struct ifproxy, entry)
        {
            ifproxy_disconnect(ifproxy);
        }
    }

    /* apartment is being destroyed so don't keep a pointer around to it */
    This->parent = NULL;

    LeaveCriticalSection(&This->cs);
}

HRESULT apartment_disconnectproxies(struct apartment *apt)
{
    struct proxy_manager *proxy;

    LIST_FOR_EACH_ENTRY(proxy, &apt->proxies, struct proxy_manager, entry)
    {
        proxy_manager_disconnect(proxy);
    }

    return S_OK;
}

static HRESULT proxy_manager_get_remunknown(struct proxy_manager * This, IRemUnknown **remunk)
{
    HRESULT hr = S_OK;
    struct apartment *apt;
    BOOL called_in_original_apt;

    /* we don't want to try and unmarshal or use IRemUnknown if we don't want
     * lifetime management */
    if (This->sorflags & SORFP_NOLIFETIMEMGMT)
        return S_FALSE;

    if (!(apt = apartment_get_current_or_mta()))
        return CO_E_NOTINITIALIZED;

    called_in_original_apt = This->parent && (This->parent->oxid == apt->oxid);

    EnterCriticalSection(&This->cs);
    /* only return the cached object if called from the original apartment.
     * in future, we might want to make the IRemUnknown proxy callable from any
     * apartment to avoid these checks */
    if (This->remunk && called_in_original_apt)
    {
        /* already created - return existing object */
        *remunk = This->remunk;
        IRemUnknown_AddRef(*remunk);
    }
    else if (!This->parent)
    {
        /* disconnected - we can't create IRemUnknown */
        *remunk = NULL;
        hr = S_FALSE;
    }
    else
    {
        STDOBJREF stdobjref;
        /* Don't want IRemUnknown lifetime management as this is IRemUnknown!
         * We also don't care about whether or not the stub is still alive */
        stdobjref.flags = SORFP_NOLIFETIMEMGMT | SORF_NOPING;
        stdobjref.cPublicRefs = 1;
        /* oxid of destination object */
        stdobjref.oxid = This->oxid;
        /* FIXME: what should be used for the oid? The DCOM draft doesn't say */
        stdobjref.oid = (OID)-1;
        stdobjref.ipid = This->oxid_info.ipidRemUnknown;

        /* do the unmarshal */
        hr = unmarshal_object(&stdobjref, apt, This->dest_context,
                              This->dest_context_data, &IID_IRemUnknown,
                              &This->oxid_info, (void**)remunk);
        if (hr == S_OK && called_in_original_apt)
        {
            This->remunk = *remunk;
            IRemUnknown_AddRef(This->remunk);
        }
    }
    LeaveCriticalSection(&This->cs);
    apartment_release(apt);

    TRACE("got IRemUnknown* pointer %p, hr = %#lx\n", *remunk, hr);

    return hr;
}

/*
 * Safely increment the reference count of a proxy manager obtained from an
 * apartment proxy list.
 *
 * This function shall be called inside the apartment's critical section.
 */
static LONG proxy_manager_addref_if_alive(struct proxy_manager * This)
{
    LONG refs = ReadNoFence(&This->refs);
    LONG old_refs, new_refs;

    do
    {
        if (refs == 0)
        {
            /* This proxy manager is about to be destroyed */
            return 0;
        }

        old_refs = refs;
        new_refs = refs + 1;
    } while ((refs = InterlockedCompareExchange(&This->refs, new_refs, old_refs)) != old_refs);

    return new_refs;
}

/* finds the proxy manager corresponding to a given OXID and OID that has
 * been unmarshaled in the specified apartment. The caller must release the
 * reference to the proxy_manager when the object is no longer used. */
static BOOL find_proxy_manager(struct apartment * apt, OXID oxid, OID oid, struct proxy_manager ** proxy_found)
{
    struct proxy_manager *proxy;
    BOOL found = FALSE;

    EnterCriticalSection(&apt->cs);
    LIST_FOR_EACH_ENTRY(proxy, &apt->proxies, struct proxy_manager, entry)
    {
        if ((oxid == proxy->oxid) && (oid == proxy->oid))
        {
            /* be careful of a race with ClientIdentity_Release, which would
             * cause us to return a proxy which is in the process of being
             * destroyed */
            if (proxy_manager_addref_if_alive(proxy) != 0)
            {
                *proxy_found = proxy;
                found = TRUE;
                break;
            }
        }
    }
    LeaveCriticalSection(&apt->cs);
    return found;
}

static HRESULT proxy_manager_construct(
    struct apartment * apt, ULONG sorflags, OXID oxid, OID oid,
    const OXID_INFO *oxid_info, struct proxy_manager ** proxy_manager)
{
    struct proxy_manager * This = malloc(sizeof(*This));
    if (!This) return E_OUTOFMEMORY;

    This->remoting_mutex = CreateMutexW(NULL, FALSE, NULL);
    if (!This->remoting_mutex)
    {
        free(This);
        return HRESULT_FROM_WIN32(GetLastError());
    }

    if (oxid_info)
    {
        This->oxid_info.dwPid = oxid_info->dwPid;
        This->oxid_info.dwTid = oxid_info->dwTid;
        This->oxid_info.ipidRemUnknown = oxid_info->ipidRemUnknown;
        This->oxid_info.dwAuthnHint = oxid_info->dwAuthnHint;
        This->oxid_info.psa = NULL /* FIXME: copy from oxid_info */;
    }
    else
    {
        HRESULT hr = rpc_resolve_oxid(oxid, &This->oxid_info);
        if (FAILED(hr))
        {
            CloseHandle(This->remoting_mutex);
            free(This);
            return hr;
        }
    }

    This->IMultiQI_iface.lpVtbl = &ClientIdentity_Vtbl;
    This->IMarshal_iface.lpVtbl = &ProxyMarshal_Vtbl;
    This->IClientSecurity_iface.lpVtbl = &ProxyCliSec_Vtbl;

    list_init(&This->entry);
    list_init(&This->interfaces);

    InitializeCriticalSection(&This->cs);
    This->cs.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": proxy_manager");

    /* the apartment the object was unmarshaled into */
    This->parent = apt;

    /* the source apartment and id of the object */
    This->oxid = oxid;
    This->oid = oid;

    This->refs = 1;

    /* the DCOM draft specification states that the SORF_NOPING flag is
     * proxy manager specific, not ifproxy specific, so this implies that we
     * should store the STDOBJREF flags here in the proxy manager. */
    This->sorflags = sorflags;

    /* we create the IRemUnknown proxy on demand */
    This->remunk = NULL;

    /* initialise these values to the weakest values and they will be
     * overwritten in proxy_manager_set_context */
    This->dest_context = MSHCTX_INPROC;
    This->dest_context_data = NULL;

    EnterCriticalSection(&apt->cs);
    /* FIXME: we are dependent on the ordering in here to make sure a proxy's
     * IRemUnknown proxy doesn't get destroyed before the regular proxy does
     * because we need the IRemUnknown proxy during the destruction of the
     * regular proxy. Ideally, we should maintain a separate list for the
     * IRemUnknown proxies that need late destruction */
    list_add_tail(&apt->proxies, &This->entry);
    LeaveCriticalSection(&apt->cs);

    TRACE("%p created for OXID %s, OID %s\n", This,
        wine_dbgstr_longlong(oxid), wine_dbgstr_longlong(oid));

    *proxy_manager = This;
    return S_OK;
}


/* helper for StdMarshalImpl_UnmarshalInterface - does the unmarshaling with
 * no questions asked about the rules surrounding same-apartment unmarshals
 * and table marshaling */
static HRESULT unmarshal_object(const STDOBJREF *stdobjref, struct apartment *apt, MSHCTX dest_context,
        void *dest_context_data, REFIID riid, const OXID_INFO *oxid_info, void **object)
{
    struct proxy_manager *proxy_manager = NULL;
    HRESULT hr = S_OK;

    assert(apt);

    TRACE("stdobjref: flags = %#lx cPublicRefs = %ld oxid = %s oid = %s ipid = %s\n",
        stdobjref->flags, stdobjref->cPublicRefs,
        wine_dbgstr_longlong(stdobjref->oxid),
        wine_dbgstr_longlong(stdobjref->oid),
        debugstr_guid(&stdobjref->ipid));

    /* create a new proxy manager if one doesn't already exist for the
     * object */
    if (!find_proxy_manager(apt, stdobjref->oxid, stdobjref->oid, &proxy_manager))
    {
        hr = proxy_manager_construct(apt, stdobjref->flags,
                                     stdobjref->oxid, stdobjref->oid, oxid_info,
                                     &proxy_manager);
    }
    else
        TRACE("proxy manager already created, using\n");

    if (hr == S_OK)
    {
        struct ifproxy * ifproxy;

        proxy_manager_set_context(proxy_manager, dest_context, dest_context_data);

        hr = proxy_manager_find_ifproxy(proxy_manager, riid, &ifproxy);
        if (hr == E_NOINTERFACE)
        {
            IRpcChannelBuffer *chanbuf;
            hr = rpc_create_clientchannel(&stdobjref->oxid, &stdobjref->ipid,
                    &proxy_manager->oxid_info, riid, proxy_manager->dest_context,
                    proxy_manager->dest_context_data, &chanbuf, apt);
            if (hr == S_OK)
                hr = proxy_manager_create_ifproxy(proxy_manager, stdobjref, riid, chanbuf, &ifproxy);
        }
        else
            IUnknown_AddRef((IUnknown *)ifproxy->iface);

        if (hr == S_OK)
        {
            InterlockedExchangeAdd((LONG *)&ifproxy->refs, stdobjref->cPublicRefs);
            /* get at least one external reference to the object to keep it alive */
            hr = ifproxy_get_public_ref(ifproxy);
            if (FAILED(hr))
                ifproxy_destroy(ifproxy);
        }

        if (hr == S_OK)
            *object = ifproxy->iface;
    }

    /* release our reference to the proxy manager - the client/apartment
     * will hold on to the remaining reference for us */
    if (proxy_manager) IMultiQI_Release(&proxy_manager->IMultiQI_iface);

    return hr;
}
