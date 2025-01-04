/*++

Copyright (c) 2021  Shorthorn Project

Module Name:

    security.c

Abstract:

    This module contains security functions

Author:

    Skulltrail 18-July-2021

Revision History:

--*/

#include "main.h"

WINE_DEFAULT_DEBUG_CHANNEL(security); 

/******************************************************************************
 * SID functions
 ****************************************************************************/

BOOL 
WINAPI 
AddMandatoryAce(
	PACL pAcl, 
	DWORD dwAceRevision, 
	DWORD AceFlags, 
	DWORD MandatoryPolicy, 
	PSID pLabelSid
)
{
  NTSTATUS Status; // eax@1
  BOOL result; // eax@2

  Status = RtlAddMandatoryAce(pAcl, 
							  dwAceRevision, 
							  AceFlags, 
							  MandatoryPolicy, 
							  SYSTEM_MANDATORY_LABEL_ACE_TYPE, 
							  pLabelSid);
  if ( !NT_SUCCESS(Status))
  {
    result = FALSE;
  }
  else
  {
    result = TRUE;
  }
  return result;
}

/*
 * @implemented
 */
VOID
WINAPI
SetSecurityAccessMask(IN SECURITY_INFORMATION SecurityInformation,
                      OUT LPDWORD DesiredAccess)
{
    *DesiredAccess = 0;

    if (SecurityInformation & (OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION | LABEL_SECURITY_INFORMATION))
        *DesiredAccess |= WRITE_OWNER;

    if (SecurityInformation & DACL_SECURITY_INFORMATION)
        *DesiredAccess |= WRITE_DAC;

    if (SecurityInformation & SACL_SECURITY_INFORMATION)
        *DesiredAccess |= ACCESS_SYSTEM_SECURITY;
}

/*
 * @implemented
 */
VOID
WINAPI
QuerySecurityAccessMask(IN SECURITY_INFORMATION SecurityInformation,
                        OUT LPDWORD DesiredAccess)
{
    *DesiredAccess = 0;

    if (SecurityInformation & (OWNER_SECURITY_INFORMATION |
                               GROUP_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION | LABEL_SECURITY_INFORMATION))
    {
        *DesiredAccess |= READ_CONTROL;
    }

    if (SecurityInformation & SACL_SECURITY_INFORMATION)
        *DesiredAccess |= ACCESS_SYSTEM_SECURITY;
}

/******************************************************************************
 * TreeSetNamedSecurityInfoW   [ADVAPI32.@]
 */
DWORD WINAPI TreeSetNamedSecurityInfoW(WCHAR *name, SE_OBJECT_TYPE type, SECURITY_INFORMATION info,
                                       SID *owner, SID *group, ACL *dacl, ACL *sacl, DWORD action,
                                       FN_PROGRESS progress, PROG_INVOKE_SETTING pis, void *args)
{
    FIXME("(%s, %d, %lu, %p, %p, %p, %p, %lu, %p, %d, %p) stub\n",
          debugstr_w(name), type, info, owner, group, dacl, sacl, action, progress, pis, args);

    return ERROR_CALL_NOT_IMPLEMENTED;
}