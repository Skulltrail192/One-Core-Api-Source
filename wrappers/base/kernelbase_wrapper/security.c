/*++

Copyright (c) 2017 Shorthorn Project

Module Name:

    security.c

Abstract:

    This module implements the Win32 security checks.

Author:

    Skulltrail 20-May-2017

Revision History:

--*/

#include <main.h>

WINE_DEFAULT_DEBUG_CHANNEL(kernel32);

VOID BaseRestoreImpersonation(HANDLE ThreadInformation)
{
  NTSTATUS Status; // eax

  if ( ThreadInformation )
  {
    Status = NtSetInformationThread((HANDLE)0xFFFFFFFE, ThreadImpersonationToken, &ThreadInformation, 4u);
    if ( !NT_SUCCESS(Status) )
      RtlRaiseStatus(Status);
  }
}

NTSTATUS __stdcall BaseRevertToSelf(void **a1)
{
  struct _TEB *Teb; // eax
  NTSTATUS Status; // ebx
  int ThreadInformation; // [esp+Ch] [ebp-8h] BYREF
  void *TokenHandle; // [esp+10h] [ebp-4h] BYREF

  Teb = NtCurrentTeb();
  Status = 0;
  TokenHandle = 0;
  ThreadInformation = 0;
  *a1 = 0;
  if ( Teb->IsImpersonating )
  {
    Status = NtOpenThreadToken((HANDLE)0xFFFFFFFE, 0x2000004u, 1u, &TokenHandle);
    if ( Status >= 0 )
    {
      Status = NtSetInformationThread((HANDLE)0xFFFFFFFE, ThreadImpersonationToken, &ThreadInformation, 4u);
      if ( Status >= 0 )
      {
        *a1 = TokenHandle;
        TokenHandle = 0;
      }
    }
    if ( TokenHandle )
      NtClose(TokenHandle);
  }
  return Status;
}

/*
* @unimplemented
*/
DWORD 
WINAPI 
CheckElevationEnabled(
    BOOL *pResult
) 
{
	*pResult = FALSE;
	return S_OK;
}

/*
* @unimplemented
*/
DWORD 
WINAPI 
CheckElevation(
   LPCWSTR lpApplicationName,
   PDWORD pdwFlags,
   HANDLE hChildToken,
   PDWORD pdwRunLevel,
   PDWORD pdwReason
)
{
	HANDLE TokenHandle =0;
	NTSTATUS Status;
	HANDLE QueryToken;
	
	*pdwRunLevel = 0;
	
    if ( BaseRevertToSelf(&QueryToken) < 0 || !QueryToken )
		TokenHandle = 0;
    Status = NtOpenProcessToken(hChildToken, 0x80u, &TokenHandle);
    BaseRestoreImpersonation(hChildToken);
	return S_OK;
}

BOOL 
AccessCheckByTypeResultList(
	PSECURITY_DESCRIPTOR pSecurityDescriptor, 
	PSID PrincipalSelfSid, 
	HANDLE ClientToken, 
	DWORD DesiredAccess, 
	POBJECT_TYPE_LIST ObjectTypeList, 
	DWORD ObjectTypeListLength, 
	PGENERIC_MAPPING GenericMapping, 
	PPRIVILEGE_SET PrivilegeSet, 
	LPDWORD PrivilegeSetLength, 
	LPDWORD GrantedAccessList, 
	LPDWORD AccessStatusList)
{
  NTSTATUS Status ; // eax
  BOOL result; // eax
  ULONG i; // esi

  Status  = NtAccessCheckByTypeResultList(
          pSecurityDescriptor,
          PrincipalSelfSid,
          ClientToken,
          DesiredAccess,
          ObjectTypeList,
          ObjectTypeListLength,
          GenericMapping,
          PrivilegeSet,
          PrivilegeSetLength,
          GrantedAccessList,
          (PNTSTATUS)AccessStatusList);
  if ( Status  >= 0 )
  {
    i = 0;
    if ( ObjectTypeListLength )
    {
      do
      {
        if ( AccessStatusList[i] )
          AccessStatusList[i] = RtlNtStatusToDosError(AccessStatusList[i]);
        else
          AccessStatusList[i] = 0;
        ++i;
      }
      while ( i < ObjectTypeListLength );
    }
    result = TRUE;
  }
  else
  {
    BaseSetLastNTError(Status);
    result = FALSE;
  }
  return result;
}

/******************************************************************************
 * CreateBoundaryDescriptorW    (kernelbase.@)
 */
HANDLE 
WINAPI 
CreateBoundaryDescriptorW( 
	LPCWSTR name, 
	ULONG flags 
)
{
    FIXME("%s %lu - stub\n", debugstr_w(name), flags);
    return NULL;
}

/******************************************************************************
 * CreateBoundaryDescriptorA    (kernelbase.@)
 */
HANDLE 
WINAPI 
CreateBoundaryDescriptorA( 
	LPCSTR name, 
	ULONG flags 
)
{
    return NULL;
}

HANDLE 
CreatePrivateNamespaceW(
  LPSECURITY_ATTRIBUTES lpPrivateNamespaceAttributes,
  LPVOID                lpBoundaryDescriptor,
  LPCWSTR               lpAliasPrefix
)
{
	return NULL;
}

HANDLE 
OpenPrivateNamespaceW(
  LPVOID  lpBoundaryDescriptor,
  LPCWSTR lpAliasPrefix
)
{
	return NULL;
}

BOOLEAN 
ClosePrivateNamespace(
  HANDLE Handle,
  ULONG  Flags
)
{
	return TRUE;
}

// The only flag is UWP app containers, we don't care. Needs to be in advapi32, and kernel32/kernelbase needs to export it.
BOOL WINAPI CheckTokenMembershipEx(HANDLE TokenHandle, PSID SidToCheck, DWORD Flags, PBOOL IsMember) {
    return CheckTokenMembership(TokenHandle, SidToCheck, IsMember);
}