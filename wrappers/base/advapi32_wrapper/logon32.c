/*++

Copyright (c) 2023  Shorthorn Project

Module Name:

    logon32.c

Abstract:

    Logon relative functions
	
Author:

    Skulltrail 24-Febraury-2023

Revision History:

--*/

#include "main.h"

BOOL WINAPI LogonUserExExW(
  _In_      LPWSTR        lpszUsername,
  _In_opt_  LPWSTR        lpszDomain,
  _In_opt_  LPWSTR        lpszPassword,
  _In_      DWORD         dwLogonType,
  _In_      DWORD         dwLogonProvider,
  _In_opt_  PTOKEN_GROUPS pTokenGroups,
  _Out_opt_ PHANDLE       phToken,
  _Out_opt_ PSID          *ppLogonSid,
  _Out_opt_ PVOID         *ppProfileBuffer,
  _Out_opt_ LPDWORD       pdwProfileLength,
  _Out_opt_ PQUOTA_LIMITS pQuotaLimits
)
{
	return LogonUserExW(lpszUsername,
						lpszDomain,
						lpszPassword,
						dwLogonType,
						dwLogonProvider,
						phToken,
						ppLogonSid,
						ppProfileBuffer,
						pdwProfileLength,
						pQuotaLimits);
}