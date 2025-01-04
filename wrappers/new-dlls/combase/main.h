#include <assert.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#define COBJMACROS
#define NONAMELESSUNION

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "wingdi.h"
#include "winuser.h"
#include "winnls.h"
#include "winreg.h"
#include "ole2.h"
#include "ole2ver.h"

#include "wine/unicode.h"
#include "olestd.h"

#include "wine/list.h"

#include "windef.h"
#include "winbase.h"
#include "wtypes.h"
#include "dcomcombase.h"
#include "winreg.h"
#include "wine/winternl.h"
#include "wine/debug.h"
#include "wine/exception.h"
#include "servprov.h"
#include "combaseapi.h"

#define APTTYPEQUALIFIER_APPLICATION_STA 6
#define APTTYPEQUALIFIER_RESERVED_1 7
#define ACTIVATION_CONTEXT_SECTION_WINRT_ACTIVATABLE_CLASSES     12
#define RTL_CRITICAL_SECTION_FLAG_FORCE_DEBUG_INFO 0x10000000

#define IRPCSS_PROTSEQ {'n','c','a','l','r','p','c',0}
#define IRPCSS_ENDPOINT {'i','r','p','c','s','s',0}

#define RPCSS_CALL_START \
    HRESULT hr; \
    for (;;) { \
        __TRY {

#define RPCSS_CALL_END \
        } __EXCEPT(rpc_filter) { \
            hr = HRESULT_FROM_WIN32(GetExceptionCode()); \
        } \
        __ENDTRY \
        if (hr == HRESULT_FROM_WIN32(RPC_S_SERVER_UNAVAILABLE)) { \
            if (start_rpcss()) \
                continue; \
        } \
        break; \
    } \
    return hr;

/* private flag indicating that the caller does not want to notify the stub
 * when the proxy disconnects or is destroyed */
#define SORFP_NOLIFETIMEMGMT SORF_OXRES2


/* Since Visual Studio 2012, volatile accesses do not always imply acquire and
 * release semantics.  We explicitly use ISO volatile semantics, manually
 * placing barriers as appropriate.
 */
#define __WINE_LOAD32_NO_FENCE(src) (*(src))

enum comclass_threadingmodel
{
    ThreadingModel_Apartment = 1,
    ThreadingModel_Free      = 2,
    ThreadingModel_No        = 3,
    ThreadingModel_Both      = 4,
    ThreadingModel_Neutral   = 5
};

struct apartment;
typedef struct apartment APARTMENT;
typedef struct LocalServer LocalServer;

/*
 * This is a marshallable object exposing registered local servers.
 * IServiceProvider is used only because it happens meet requirements
 * and already has proxy/stub code. If more functionality is needed,
 * a custom interface may be used instead.
 */
struct LocalServer
{
    PVOID IServiceProvider_iface;//IServiceProvider IServiceProvider_iface; Modified to not implement really functions
    LONG ref;
    APARTMENT *apt;
    IStream *marshal_stream;
};

struct apartment
{
    struct list entry;

    LONG  refs;              /* refcount of the apartment (LOCK) */
    BOOL multi_threaded;     /* multi-threaded or single-threaded apartment? (RO) */
    DWORD tid;               /* thread id (RO) */
    OXID oxid;               /* object exporter ID (RO) */
    LONG ipidc;              /* interface pointer ID counter, starts at 1 (LOCK) */
    CRITICAL_SECTION cs;     /* thread safety */
    struct list proxies;     /* imported objects (CS cs) */
    struct list stubmgrs;    /* stub managers for exported objects (CS cs) */
    BOOL remunk_exported;    /* has the IRemUnknown interface for this apartment been created yet? (CS cs) */
    LONG remoting_started;   /* has the RPC system been started for this apartment? (LOCK) */
    struct list loaded_dlls; /* list of dlls loaded by this apartment (CS cs) */
    DWORD host_apt_tid;      /* thread ID of apartment hosting objects of differing threading model (CS cs) */
    HWND host_apt_hwnd;      /* handle to apartment window of host apartment (CS cs) */
    struct local_server *local_server; /* A marshallable object exposing local servers (CS cs) */
    BOOL being_destroyed;    /* is currently being destroyed */

    /* FIXME: OIDs should be given out by RPCSS */
    OID oidc;                /* object ID counter, starts at 1, zero is invalid OID (CS cs) */

    /* STA-only fields */
    HWND win;                /* message window (LOCK) */
    IMessageFilter *filter;  /* message filter (CS cs) */
    BOOL main;               /* is this a main-threaded-apartment? (RO) */

    /* MTA-only */
    struct list usage_cookies; /* Used for refcount control with CoIncrementMTAUsage()/CoDecrementMTAUsage(). */
};

/* this is what is stored in TEB->ReservedForOle */
struct oletls
{
    struct apartment *apt;
    IErrorInfo       *errorinfo;   /* see errorinfo.c */
    IUnknown         *state;       /* see CoSetState */
    DWORD             apt_mask;    /* apartment mask (+0Ch on x86) */
    IInitializeSpy   *spy;         /* The "SPY" from CoInitializeSpy */
    DWORD            inits;        /* number of times CoInitializeEx called */
    DWORD            ole_inits;    /* number of times OleInitialize called */
    GUID             causality_id; /* unique identifier for each COM call */
    LONG             pending_call_count_client; /* number of client calls pending */
    LONG             pending_call_count_server; /* number of server calls pending */
    DWORD            unknown;
    IObjContext     *context_token; /* (+38h on x86) */
    IUnknown        *call_state;    /* current call context (+3Ch on x86) */
    DWORD            unknown2[46];
    IUnknown        *cancel_object; /* cancel object set by CoSetCancelObject (+F8h on x86) */
};

typedef HRESULT (WINAPI *DllGetClassObjectFunc)(REFCLSID clsid, REFIID iid, void **obj);
typedef HRESULT (WINAPI *DllCanUnloadNowFunc)(void);

struct opendll
{
    LONG refs;
    LPWSTR library_name;
    HANDLE library;
    DllGetClassObjectFunc DllGetClassObject;
    DllCanUnloadNowFunc DllCanUnloadNow;
    struct list entry;
};

struct apartment_loaded_dll
{
    struct list entry;
    struct opendll *dll;
    DWORD unload_time;
    BOOL multi_threaded;
};

struct registered_class
{
    struct list entry;
    CLSID clsid;
    OXID apartment_id;
    IUnknown *object;
    DWORD clscontext;
    DWORD flags;
    unsigned int cookie;
    unsigned int rpcss_cookie;
};


/* imported object / proxy manager */
struct proxy_manager
{
    IMultiQI IMultiQI_iface;
    IMarshal IMarshal_iface;
    IClientSecurity IClientSecurity_iface;
    struct apartment *parent; /* owning apartment (RO) */
    struct list entry;        /* entry in apartment (CS parent->cs) */
    OXID oxid;                /* object exported ID (RO) */
    OXID_INFO oxid_info;      /* string binding, ipid of rem unknown and other information (RO) */
    OID oid;                  /* object ID (RO) */
    struct list interfaces;   /* imported interfaces (CS cs) */
    LONG refs;                /* proxy reference count (LOCK); 0 if about to be removed from list */
    CRITICAL_SECTION cs;      /* thread safety for this object and children */
    ULONG sorflags;           /* STDOBJREF flags (RO) */
    IRemUnknown *remunk;      /* proxy to IRemUnknown used for lifecycle management (CS cs) */
    HANDLE remoting_mutex;    /* mutex used for synchronizing access to IRemUnknown */
    MSHCTX dest_context;      /* context used for activating optimisations (LOCK) */
    void *dest_context_data;  /* reserved context value (LOCK) */
};

/* imported interface proxy */
struct ifproxy
{
    struct list entry;       /* entry in proxy_manager list (CS parent->cs) */
    struct proxy_manager *parent; /* owning proxy_manager (RO) */
    void *iface;             /* interface pointer (RO) */
    STDOBJREF stdobjref;     /* marshal data that represents this object (RO) */
    IID iid;                 /* interface ID (RO) */
    IRpcProxyBuffer *proxy;  /* interface proxy (RO) */
    ULONG refs;              /* imported (public) references (LOCK) */
    IRpcChannelBuffer *chan; /* channel to object (CS parent->cs) */
};

enum tlsdata_flags
{
    OLETLS_UUIDINITIALIZED = 0x2,
    OLETLS_DISABLE_OLE1DDE = 0x40,
    OLETLS_APARTMENTTHREADED = 0x80,
    OLETLS_MULTITHREADED = 0x100,
};

/* this is what is stored in TEB->ReservedForOle */
struct tlsdata
{
    struct apartment *apt;
    IErrorInfo       *errorinfo;
    DWORD             thread_seqid;  /* returned with CoGetCurrentProcess */
    DWORD             flags;         /* tlsdata_flags (+0Ch on x86) */
    void             *unknown0;
    DWORD             inits;         /* number of times CoInitializeEx called */
    DWORD             ole_inits;     /* number of times OleInitialize called */
    GUID              causality_id;  /* unique identifier for each COM call */
    LONG              pending_call_count_client; /* number of client calls pending */
    LONG              pending_call_count_server; /* number of server calls pending */
    DWORD             unknown;
    IObjContext      *context_token; /* (+38h on x86) */
    IUnknown         *call_state;    /* current call context (+3Ch on x86) */
    DWORD             unknown2[46];
    IUnknown         *cancel_object; /* cancel object set by CoSetCancelObject (+F8h on x86) */
    IUnknown         *state;         /* see CoSetState */
    struct list       spies;         /* Spies installed with CoRegisterInitializeSpy */
    DWORD             spies_lock;
    DWORD             cancelcount;
    CO_MTA_USAGE_COOKIE implicit_mta_cookie; /* mta referenced by roapi from sta thread */
};

typedef struct _SOleTlsData {
  void  *pvReserved0[2];
  DWORD dwReserved0[3];
  void  *pvReserved1[1];
  DWORD dwReserved1[3];
  void  *pvReserved2[4];
  DWORD dwReserved2[1];
  DWORD dwFlags;
  void  *pCurrentCtx;
} SOleTlsData, *PSOleTlsData;

struct activatable_class_data
{
    ULONG size;
    DWORD unk;
    DWORD module_len;
    DWORD module_offset;
    DWORD threading_model;
};

extern HRESULT WINAPI InternalTlsAllocData(struct tlsdata **data);

static inline HRESULT com_get_tlsdata(struct tlsdata **data)
{
    *data = NtCurrentTeb()->ReservedForOle;
    return *data ? S_OK : InternalTlsAllocData(data);
}

static inline struct apartment* com_get_current_apt(void)
{
    struct tlsdata *tlsdata = NULL;
    com_get_tlsdata(&tlsdata);
    return tlsdata->apt;
}

HRESULT apartment_increment_mta_usage(CO_MTA_USAGE_COOKIE *cookie);

void apartment_decrement_mta_usage(CO_MTA_USAGE_COOKIE cookie);

void apartment_revoke_all_classes(const struct apartment *apt);

LONG WINAPI rpc_filter(EXCEPTION_POINTERS *eptr);

BOOL start_rpcss(void);

HRESULT apartment_disconnectproxies(struct apartment *apt);

static DWORD apartment_addref(struct apartment *apt);

struct apartment * apartment_get_mta(void);

static HRESULT proxy_manager_get_remunknown(struct proxy_manager * This, IRemUnknown **remunk);

HRESULT rpc_revoke_local_server(unsigned int cookie);

struct apartment * apartment_get_current_or_mta(void);

static HRESULT unmarshal_object(const STDOBJREF *stdobjref, struct apartment *apt,
                                MSHCTX dest_context, void *dest_context_data,
                                REFIID riid, const OXID_INFO *oxid_info,
                                void **object);
								
void apartment_release(struct apartment *apt);

static FORCEINLINE LONG ReadNoFence( LONG const volatile *src )
{
    LONG value = __WINE_LOAD32_NO_FENCE( (int const volatile *)src );
    return value;
}

//External kernel32
/*
* @implemented
*/
BOOL WINAPI InitializeCriticalSectionEx(OUT LPCRITICAL_SECTION lpCriticalSection,
                                        IN DWORD dwSpinCount,
                                        IN DWORD flags);

HRESULT ensure_mta(void);

/* shared variables*/

extern struct list registered_classes;

extern CRITICAL_SECTION registered_classes_cs;