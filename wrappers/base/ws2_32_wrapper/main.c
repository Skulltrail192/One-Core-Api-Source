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


#include <main.h>

WINE_DEFAULT_DEBUG_CHANNEL(ws2_32);

int 
WINAPI
WSCInstallProviderAndChains(
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

int 
WINAPI 
WSCSetProviderInfo(
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