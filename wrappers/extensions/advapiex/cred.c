/*++

Copyright (c) 2023  Shorthorn Project

Module Name:

    cred.c

Abstract:

    Credential Protection API Interfaces

Author:

    Skulltrail 01-October-2023

Revision History:

--*/

#include "main.h"
#include "wincred.h"

WINE_DEFAULT_DEBUG_CHANNEL(cred);

// Should be most relevant Vista credential API additions.

BOOL 
WINAPI 
CredProtectA(
  _In_     BOOL fAsSelf,
  _In_     LPTSTR pszCredentials,
  _In_     DWORD cchCredentials,
  _Out_    LPTSTR pszProtectedCredentials,
  _Inout_  DWORD *pcchMaxChars,
  _Out_    CRED_PROTECTION_TYPE *ProtectionType
)
{
	if (!pszProtectedCredentials || !pszCredentials) {
		SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;
	}
	if (pcchMaxChars && *pcchMaxChars < cchCredentials) {
		SetLastError(ERROR_INSUFFICIENT_BUFFER);
		*pcchMaxChars = cchCredentials;
		return FALSE;
	}
	memcpy(pszProtectedCredentials, pszCredentials, cchCredentials);
	if (ProtectionType)
		*ProtectionType = CredUnprotected; // It's in plain text.
	return TRUE;
}

BOOL 
WINAPI 
CredProtectW(
  _In_     BOOL fAsSelf,
  _In_     LPWSTR pszCredentials,
  _In_     DWORD cchCredentials,
  _Out_    LPWSTR pszProtectedCredentials,
  _Inout_  DWORD *pcchMaxChars,
  _Out_    CRED_PROTECTION_TYPE *ProtectionType
)
{
	if (!pszProtectedCredentials || !pszCredentials) {
		SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;
	}
	if (pcchMaxChars && *pcchMaxChars < cchCredentials) {
		SetLastError(ERROR_INSUFFICIENT_BUFFER);
		*pcchMaxChars = cchCredentials;
		return FALSE;
	}
	memcpy(pszProtectedCredentials, pszCredentials, cchCredentials * 2);
	if (ProtectionType)
		*ProtectionType = CredUnprotected; // It's in plain text.
	return TRUE;
}

BOOL 
WINAPI 
CredUnprotectA(
  _In_     BOOL fAsSelf,
  _In_     LPTSTR pszProtectedCredentials,
  _In_     DWORD cchCredentials,
  _Out_    LPTSTR pszCredentials,
  _Inout_  DWORD *pcchMaxChars
)
{
	if (!pszProtectedCredentials || !pszCredentials) {
		SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;
	}
	if (pcchMaxChars && *pcchMaxChars < cchCredentials) {
		SetLastError(ERROR_INSUFFICIENT_BUFFER);
		*pcchMaxChars = cchCredentials;
		return FALSE;
	}
	memcpy(pszCredentials, pszProtectedCredentials, cchCredentials);
	return TRUE;
}

BOOL 
WINAPI 
CredUnprotectW(
  _In_     BOOL fAsSelf,
  _In_     LPWSTR pszProtectedCredentials,
  _In_     DWORD cchCredentials,
  _Out_    LPWSTR pszCredentials,
  _Inout_  DWORD *pcchMaxChars
)
{
	if (!pszProtectedCredentials || !pszCredentials) {
		SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;
	}
	if (pcchMaxChars && *pcchMaxChars < cchCredentials) {
		SetLastError(ERROR_INSUFFICIENT_BUFFER);
		*pcchMaxChars = cchCredentials;
		return FALSE;
	}
	memcpy(pszCredentials, pszProtectedCredentials, cchCredentials * 2);
	return TRUE;
}

BOOL 
WINAPI 
CredIsProtectedA(
  _In_   LPWSTR pszProtectedCredentials,
  _Out_  CRED_PROTECTION_TYPE *pProtectionType
) {
	if (!pProtectionType) {
		SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;
	}
	*pProtectionType = CredUnprotected;
	return TRUE;
}

BOOL 
WINAPI 
CredIsProtectedW(
  _In_   LPWSTR pszProtectedCredentials,
  _Out_  CRED_PROTECTION_TYPE *pProtectionType
) {
	if (!pProtectionType) {
		SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;
	}
	*pProtectionType = CredUnprotected;
	return TRUE;
}

BOOL 
WINAPI 
CredFindBestCredentialA(
  _In_   LPCTSTR TargetName,
  _In_   DWORD Type,
  _In_   DWORD Flags,
  _Out_  PCREDENTIALA *Credential
)
{
	WARN("UNIMPL: CredFindBestCredential called\n");
	*Credential = NULL;
	return TRUE;
}

BOOL 
WINAPI 
CredFindBestCredentialW(
  _In_   LPCWSTR TargetName,
  _In_   DWORD Type,
  _In_   DWORD Flags,
  _Out_  PCREDENTIALW *Credential
)
{
	WARN("UNIMPL: CredFindBestCredential called\n");
	*Credential = NULL;
	return TRUE;
}