/*++

Copyright (c) 2022 Shorthorn Project

Module Name:

    socket.c

Abstract:

    This module main functions of ws2_32 DLL

Author:

    Skulltrail 25-September-2022

Revision History:

--*/

#define WIN32_NO_STATUS

#include <stdarg.h>
#include <math.h>

#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#include <windef.h>
#include <winbase.h>

#include <pdh.h>
#include <pdhmsg.h>
//#include "winperf.h"

#include <wine/debug.h>
#include <wine/list.h>
#include <wine/unicode.h>
#include <ws2spi.h>

WINE_DEFAULT_DEBUG_CHANNEL(ws2_32);

typedef enum _WSC_PROVIDER_INFO_TYPE {
   ProviderInfoLspCategories,
   ProviderInfoAudit
} WSC_PROVIDER_INFO_TYPE ;

int WSCInstallProviderAndChains(
  LPGUID              lpProviderId,
  const LPWSTR        lpszProviderDllPath,
  const LPWSTR        lpszLspName,
  DWORD               dwServiceFlags,
  LPWSAPROTOCOL_INFOW lpProtocolInfoList,
  DWORD               dwNumberOfEntries,
  LPDWORD             lpdwCatalogEntryId,
  LPINT               lpErrno
)
{
	//Hack, i don't know if work
	return WSCInstallProvider(lpProviderId,
							  lpszProviderDllPath,
							  lpProtocolInfoList,
							  dwNumberOfEntries,
							  lpErrno);
}

int WSCSetProviderInfo(
  _In_  LPGUID                 lpProviderId,
  _In_  WSC_PROVIDER_INFO_TYPE InfoType,
  _In_  PBYTE                  Info,
  _In_  size_t                 InfoSize,
  _In_  DWORD                  Flags,
  _Out_ LPINT                  lpErrno
)
{
	//Only stub
	return ERROR_CALL_NOT_IMPLEMENTED;
}