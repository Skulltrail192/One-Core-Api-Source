#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "windns.h"
#include "wine/debug.h"

//
//  DNS response codes.
//
//  Sent in the "ResponseCode" field of a DNS_HEADER.
//

#define DNS_RCODE_NOERROR       0
#define DNS_RCODE_FORMERR       1       // Format error
#define DNS_RCODE_SERVFAIL      2       // Server failure
#define DNS_RCODE_NXDOMAIN      3       // Name error
#define DNS_RCODE_NOTIMPL       4       // Not implemented
#define DNS_RCODE_REFUSED       5       // Refused
#define DNS_RCODE_YXDOMAIN      6       // Domain name should not exist
#define DNS_RCODE_YXRRSET       7       // RR set should not exist
#define DNS_RCODE_NXRRSET       8       // RR set does not exist
#define DNS_RCODE_NOTAUTH       9       // Not authoritative for zone
#define DNS_RCODE_NOTZONE       10      // Name is not zone
#define DNS_RCODE_MAX           15

//
//  Mappings to friendly names
//

#define DNS_RCODE_NO_ERROR          DNS_RCODE_NOERROR
#define DNS_RCODE_FORMAT_ERROR      DNS_RCODE_FORMERR
#define DNS_RCODE_SERVER_FAILURE    DNS_RCODE_SERVFAIL
#define DNS_RCODE_NAME_ERROR        DNS_RCODE_NXDOMAIN
#define DNS_RCODE_NOT_IMPLEMENTED   DNS_RCODE_NOTIMPL


typedef struct _DNS_QUERY_RESULT
{
    ULONG           Version;
    DNS_STATUS      QueryStatus;
    ULONG64         QueryOptions;
    PDNS_RECORD     pQueryRecords;
    PVOID           Reserved;
}
DNS_QUERY_RESULT, *PDNS_QUERY_RESULT;


typedef
VOID
WINAPI
DNS_QUERY_COMPLETION_ROUTINE(
    _In_        PVOID               pQueryContext,
    _Inout_     PDNS_QUERY_RESULT   pQueryResults
);

typedef DNS_QUERY_COMPLETION_ROUTINE *PDNS_QUERY_COMPLETION_ROUTINE;

typedef struct _DNS_QUERY_REQUEST {
  ULONG                         Version;
  PCWSTR                        QueryName;
  WORD                          QueryType;
  ULONG64                       QueryOptions;
  PDNS_ADDR_ARRAY               pDnsServerList;
  ULONG                         InterfaceIndex;
  PDNS_QUERY_COMPLETION_ROUTINE pQueryCompletionCallback;
  PVOID                         pQueryContext;
} DNS_QUERY_REQUEST, *PDNS_QUERY_REQUEST;


typedef struct _DNS_QUERY_CANCEL {
  CHAR Reserved[32];
} DNS_QUERY_CANCEL, *PDNS_QUERY_CANCEL;