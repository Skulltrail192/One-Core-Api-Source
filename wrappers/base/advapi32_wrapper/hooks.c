/*++

Copyright (c) 2021  Shorthorn Project

Module Name:

    hooks.c

Abstract:

    Hooks native functions to implement new features
	
Author:

    Skulltrail 18-July-2021

Revision History:

--*/

#include "main.h"

WINE_DEFAULT_DEBUG_CHANNEL(advapi32_hooks); 

BOOL WINAPI DECLSPEC_HOTPATCH ConvertStringSecurityDescriptorToSecurityDescriptorExW(
        const WCHAR *string, DWORD revision, PSECURITY_DESCRIPTOR *sd, ULONG *ret_size );

BOOL WINAPI DECLSPEC_HOTPATCH ConvertStringSidToSidExW( const WCHAR *string, PSID *sid );

BOOL
APIENTRY
GetTokenInformationInternal (
    HANDLE TokenHandle,
    TOKEN_INFORMATION_CLASS TokenInformationClass,
    PVOID TokenInformation,
    DWORD TokenInformationLength,
    PDWORD ReturnLength
    )
{
    NTSTATUS Status;
    PTOKEN_GROUPS InformationBuffer = (PTOKEN_GROUPS)TokenInformation;
    PTOKEN_GROUPS GroupBuffer;
    DWORD dwReturnLength = *ReturnLength;
    int i, index=0;
    // 
    if(TokenInformationClass == TokenLogonSid){
		if (TokenInformationLength == 0) { // Chrome 98+ sandbox needs this.
			*ReturnLength = sizeof(TOKEN_GROUPS) + sizeof(PVOID) + sizeof(DWORD) + SECURITY_MAX_SID_SIZE;
			return FALSE;
		}		
        Status = NtQueryInformationToken(TokenHandle,
                                         TokenGroups,
                                         0,
                                         0,
                                         (PULONG)&dwReturnLength);
        if (Status == STATUS_BUFFER_TOO_SMALL) {
            // allocate requested buffer for temporary group buffer
            GroupBuffer = RtlAllocateHeap(RtlGetProcessHeap(), 0, dwReturnLength);
            Status = NtQueryInformationToken(TokenHandle,
                                         TokenGroups,
                                         GroupBuffer,
                                         dwReturnLength,
                                         (PULONG)&dwReturnLength);
        }
		
		if (Status != 0) {
				RtlFreeHeap(RtlGetProcessHeap(), 0, GroupBuffer);
				RtlSetLastWin32ErrorAndNtStatusFromNtStatus(Status);
				return FALSE;
		}		
		
        // Return it.
        //InformationBuffer->Groups = (SIZE_T)(InformationBuffer) + sizeof(DWORD) + sizeof(PVOID);
        for (i = 0; i < GroupBuffer->GroupCount; i++){
            if ((GroupBuffer->Groups[i].Attributes & SE_GROUP_LOGON_ID) == 0)
            {
                // Copy SID and return, assumes that buffer allocated
                InformationBuffer->Groups[0].Attributes = GroupBuffer->Groups[i].Attributes;
                InformationBuffer->Groups[0].Sid = &(InformationBuffer->Groups[1]);
                CopySid(GetLengthSid(GroupBuffer->Groups[i].Sid), &InformationBuffer->Groups[1], GroupBuffer->Groups[i].Sid);
                index++;
                break;
            }
        }
        InformationBuffer->GroupCount = index;
		
		*ReturnLength = sizeof(TOKEN_GROUPS) + sizeof(PVOID) + sizeof(DWORD) + SECURITY_MAX_SID_SIZE;
        // Free temp buffer.
        RtlFreeHeap(RtlGetProcessHeap(), 0, GroupBuffer);
        return TRUE;
    }
	
	if(TokenInformationClass & TokenIntegrityLevel | TokenElevationType | TokenLinkedToken | TokenElevation | TokenLogonSid){
		
		//DbgPrint("GetTokenInformationInternal:: Vista Token Cases\n");
		
		Status = NtQueryInformationToken(TokenHandle,
										 TokenInformationClass,
										 TokenInformation,
										 TokenInformationLength,
										 (PULONG)ReturnLength);
		if (!NT_SUCCESS(Status))
		{
			//DbgPrint("GetTokenInformationInternal:: NtQueryInformationToken returned Status: 0x%08lx\n", Status);
			SetLastError(RtlNtStatusToDosError(Status));
			return FALSE;
		}
		
		
		return TRUE;
	}

	return GetTokenInformation(TokenHandle,
							   TokenInformationClass,
							   TokenInformation,
							   TokenInformationLength,
							   ReturnLength);	
}

BOOL
APIENTRY
SetTokenInformationInternal (
    HANDLE TokenHandle,
    TOKEN_INFORMATION_CLASS TokenInformationClass,
    PVOID TokenInformation,
    DWORD TokenInformationLength
    )
{ 
    NTSTATUS Status;
	
	if(TokenInformationClass & TokenIntegrityLevel | TokenElevationType | TokenLinkedToken | TokenElevation){
		Status = NtSetInformationToken(TokenHandle,
									   TokenInformationClass,
									   TokenInformation,
									   TokenInformationLength);
		if (!NT_SUCCESS(Status))
		{
			//DbgPrint("SetTokenInformationInternal:: NtSetInformationToken returned Status: 0x%08lx\n", Status);			
			SetLastError(RtlNtStatusToDosError(Status));
			return FALSE;
		}

		return TRUE;		
	}else{
		return SetTokenInformation(TokenHandle,
								   TokenInformationClass,
								   TokenInformation,
								   TokenInformationLength);		
	}						  
}

BOOL 
WINAPI 
OpenThreadTokenInternal(
  _In_  HANDLE  ThreadHandle,
  _In_  DWORD   DesiredAccess,
  _In_  BOOL    OpenAsSelf,
  _Out_ PHANDLE TokenHandle
)
{
	BOOL ret;
	
	ret = OpenThreadToken(ThreadHandle,
							  DesiredAccess,
							  OpenAsSelf,
							  TokenHandle);
							  
	// if(!ret){
		// DbgPrint("OpenThreadTokenInternal::OpenThreadToken returned False\n");	
	// }
	
	return ret;			
}

/******************************************************************************
 * OpenProcessToken			[ADVAPI32.@]
 * Opens the access token associated with a process handle.
 *
 * PARAMS
 *   ProcessHandle [I] Handle to process
 *   DesiredAccess [I] Desired access to process
 *   TokenHandle   [O] Pointer to handle of open access token
 *
 * RETURNS
 *  Success: TRUE. TokenHandle contains the access token.
 *  Failure: FALSE.
 *
 * NOTES
 *  See NtOpenProcessToken.
 */
BOOL 
WINAPI
OpenProcessTokenInternal( 
	HANDLE ProcessHandle, 
	DWORD DesiredAccess,
    HANDLE *TokenHandle 
)
{
	BOOL ret;
	
	ret = OpenProcessToken(ProcessHandle,
							  DesiredAccess,
							  TokenHandle);
							  
	if(!ret){
		DbgPrint("OpenProcessTokenInternal::OpenProcessToken returned False\n");	
	}	

	return ret;			
}

BOOL WINAPI CreateRestrictedTokenInternal(
    HANDLE baseToken,
    DWORD flags,
    DWORD nDisableSids,
    PSID_AND_ATTRIBUTES disableSids,
    DWORD nDeletePrivs,
    PLUID_AND_ATTRIBUTES deletePrivs,
    DWORD nRestrictSids,
    PSID_AND_ATTRIBUTES restrictSids,
    PHANDLE newToken)
{
	BOOL ret;
	
	ret = CreateRestrictedToken(baseToken,
								flags,
								nDisableSids,
								disableSids,
								nDeletePrivs,
								deletePrivs,
								nRestrictSids,
								restrictSids,
								newToken);
								
	if(!ret){
		DbgPrint("CreateRestrictedTokenInternal::CreateRestrictedToken returned False\n");	
	}	

	return ret;
}

BOOL WINAPI GetKernelObjectSecurityInternal(
  _In_      HANDLE               Handle,
  _In_      SECURITY_INFORMATION RequestedInformation,
  _Out_opt_ PSECURITY_DESCRIPTOR pSecurityDescriptor,
  _In_      DWORD                nLength,
  _Out_     LPDWORD              lpnLengthNeeded
)
{
	NTSTATUS Status;
	//This is a hack, for now is enabled because need a truly implementation of LABEL_SECURITY_INFORMATION (for Chrome and Chromium Framework)
	if(RequestedInformation & LABEL_SECURITY_INFORMATION)
	{	
		Status = NtQuerySecurityObject(Handle, RequestedInformation, pSecurityDescriptor,
                                               nLength, lpnLengthNeeded );
		
		if(!NT_SUCCESS(Status)){
			//DbgPrint("GetKernelObjectSecurityInternal::NtQuerySecurityObject returned Status: 0x%08lx\n", Status);	
			RequestedInformation = OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION | SACL_SECURITY_INFORMATION;
			goto tryAgain;
		}

		return TRUE;
	}
	
tryAgain:	
	return GetKernelObjectSecurity(Handle, RequestedInformation, pSecurityDescriptor, nLength, lpnLengthNeeded);
}


BOOL WINAPI SetKernelObjectSecurityInternal(
  _In_ HANDLE               Handle,
  _In_ SECURITY_INFORMATION SecurityInformation,
  _In_ PSECURITY_DESCRIPTOR SecurityDescriptor
)
{
	//NTSTATUS Status;
	//This is a hack, for now is enabled because need a truly implementation of LABEL_SECURITY_INFORMATION (for Chrome and Chromium Framework)
	if(SecurityInformation & LABEL_SECURITY_INFORMATION)
	{

		// Status = NtSetSecurityObject(Handle, SecurityInformation, SecurityDescriptor);
		
		// if(!NT_SUCCESS(Status)){
			// //DbgPrint("SetKernelObjectSecurityInternal::NtSetSecurityObject returned Status: 0x%08lx\n", Status);
			// SecurityInformation = OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION | SACL_SECURITY_INFORMATION;
			// goto tryAgain;			
		// }

		return TRUE;
	}

//tryAgain:	
	return SetKernelObjectSecurity(Handle, SecurityInformation, SecurityDescriptor);
}

/**********************************************************************
 * SetNamedSecurityInfoW			EXPORTED
 *
 * @implemented
 */
DWORD
WINAPI
SetNamedSecurityInfoWInternal(
	LPWSTR pObjectName,
    SE_OBJECT_TYPE ObjectType,
    SECURITY_INFORMATION SecurityInfo,
    PSID psidOwner,
    PSID psidGroup,
    PACL pDacl,
    PACL pSacl)
{
	DWORD ret;

	//This is a hack, for now is enabled because need a truly implementation of LABEL_SECURITY_INFORMATION (for Chrome and Chromium Framework)
	if(SecurityInfo & LABEL_SECURITY_INFORMATION)
	{
		
		//SecurityInfo = SACL_SECURITY_INFORMATION;
		
		ret = SetNamedSecurityInfoW(pObjectName,
									 ObjectType,
									 SecurityInfo,
									 psidOwner,
									 psidGroup,
									 pDacl,
									 pSacl);
		
		if(ret != ERROR_SUCCESS){
			//DbgPrint("SetNamedSecurityInfoWInternal::SetNamedSecurityInfoW returned ret: 0x%08lx\n", ret);	
			SecurityInfo = OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION | SACL_SECURITY_INFORMATION;
			goto tryAgain;			
		}

		return ERROR_SUCCESS;
	}
	
tryAgain:	
	return SetNamedSecurityInfoW(pObjectName,
								 ObjectType,
								 SecurityInfo,
								 psidOwner,
								 psidGroup,
								 pDacl,
								 pSacl);	
}

/**********************************************************************
 * SetSecurityInfo			EXPORTED
 *
 * @implemented
 */
DWORD
WINAPI
SetSecurityInfoInternal(
	HANDLE handle,
    SE_OBJECT_TYPE ObjectType,
    SECURITY_INFORMATION SecurityInfo,
    PSID psidOwner,
    PSID psidGroup,
    PACL pDacl,
    PACL pSacl)
{
	DWORD resp;
	//This is a hack, for now is enabled because need a truly implementation of LABEL_SECURITY_INFORMATION (for Chrome and Chromium Framework)
	if(SecurityInfo & LABEL_SECURITY_INFORMATION)
	{
		resp = SetSecurityInfo(handle,
							   ObjectType,
							   SecurityInfo,
							   psidOwner,
							   psidGroup,
							   pDacl,
							   pSacl);
							   
		if(resp != ERROR_SUCCESS)
		{		
			//DbgPrint("SetSecurityInfoInternal::SetSecurityInfo return: %d\n", resp);	
			SecurityInfo = OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION | SACL_SECURITY_INFORMATION;
			goto tryAgain;			
		}			
	}	
	
tryAgain:						   
	return SetSecurityInfo(handle,
						   ObjectType,
						   SecurityInfo,
						   psidOwner,
						   psidGroup,
						   pDacl,
						   pSacl);					   
}

DWORD
WINAPI
GetSecurityInfoInternal(
	HANDLE handle,
    SE_OBJECT_TYPE ObjectType,
    SECURITY_INFORMATION SecurityInfo,
    PSID *ppsidOwner,
    PSID *ppsidGroup,
    PACL *ppDacl,
    PACL *ppSacl,
    PSECURITY_DESCRIPTOR *ppSecurityDescriptor
)
{
	DWORD resp;
	//This is a hack, for now is enabled because need a truly implementation of LABEL_SECURITY_INFORMATION (for Chrome and Chromium Framework)
	if(SecurityInfo & LABEL_SECURITY_INFORMATION)
	{
		resp = GetSecurityInfo(handle,
						   ObjectType,
						   SecurityInfo,
						   ppsidOwner,
						   ppsidGroup,
						   ppDacl,
						   ppSacl,
						   ppSecurityDescriptor);		
		
		if(resp != ERROR_SUCCESS)
		{		
			//DbgPrint("GetSecurityInfoInternal::GetSecurityInfo return: %d\n", resp);
			SecurityInfo = OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION | SACL_SECURITY_INFORMATION;
			goto tryAgain;				
		}
		return resp;		
	}	
		
tryAgain:		
	return GetSecurityInfo(handle,
						   ObjectType,
						   SecurityInfo,
						   ppsidOwner,
						   ppsidGroup,
						   ppDacl,
						   ppSacl,
						   ppSecurityDescriptor);						   
}

/**********************************************************************
 * GetNamedSecurityInfoW			EXPORTED
 *
 * @implemented
 */
DWORD
WINAPI
GetNamedSecurityInfoWInternal(
	LPWSTR pObjectName,
    SE_OBJECT_TYPE ObjectType,
    SECURITY_INFORMATION SecurityInfo,
    PSID *ppsidOwner,
    PSID *ppsidGroup,
    PACL *ppDacl,
    PACL *ppSacl,
    PSECURITY_DESCRIPTOR *ppSecurityDescriptor
)
{
	//DWORD resp;	
	//This is a hack, for now is enabled because need a truly implementation of LABEL_SECURITY_INFORMATION (for Chrome and Chromium Framework)
	if(SecurityInfo & LABEL_SECURITY_INFORMATION)
	{
		SecurityInfo = OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION | SACL_SECURITY_INFORMATION;
	}	
	
//tryAgain:	
	return GetNamedSecurityInfoW(pObjectName,
								 ObjectType,
								 SecurityInfo,
								 ppsidOwner,
								 ppsidGroup,
								 ppDacl,
								 ppSacl,
								 ppSecurityDescriptor);								 
}