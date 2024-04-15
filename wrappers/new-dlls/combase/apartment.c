/*++

Copyright (c) 2023 Shorthorn Project

Module Name:

    main.c

Abstract:

    This module implements COM Apartment functions APIs

Author:

    Skulltrail 12-October-2023

Revision History:

--*/

#include "main.h"

WINE_DEFAULT_DEBUG_CHANNEL(ole32);

BOOLEAN g_cMTAInits;  // Global variables within ole32 itself
DWORD gdwMainThreadId;

HRESULT WINAPI CoGetApartmentType(APTTYPE *pAptType, APTTYPEQUALIFIER *pAptQualifier)
/* 
    win32 - March 7 2021
	An important OLE function introduced in Windows 7, but should be usable in NT 4 and above.
	Modified to better support wrappers
*/
{
  IUnknown ContextToken;
  HRESULT Status; 
  SOleTlsData* ReservedForOle; 
  DWORD CurrentThreadId; 
  DWORD OleFlags; 
  APTTYPE AptTypeTemp; 
  APTTYPEQUALIFIER AptQualifierTemp; 
  TEB *CurrentThreadInfo;
  
  Status = S_OK;
  if ( !pAptType || !pAptQualifier )
    return E_INVALIDARG;
  *pAptType = APTTYPE_CURRENT;
  *pAptQualifier = APTTYPEQUALIFIER_NONE;
  CurrentThreadInfo = NtCurrentTeb();
  ReservedForOle = (SOleTlsData*) CurrentThreadInfo->ReservedForOle;
  CurrentThreadId = GetCurrentThreadId();

  if ( !ReservedForOle )
  {
	// Try to see if COM is initialized
    if ( CoGetContextToken((ULONG_PTR*)&ContextToken) == 0)
      goto ImplicitMTA;
    return CO_E_NOTINITIALIZED;
  }
  OleFlags = ReservedForOle->dwFlags;
  if ( (OleFlags & 0x800) == 0 )
  {
    if ( (OleFlags & 0x80) != 0 )
    {
      AptTypeTemp = APTTYPE_STA;
      if (FALSE) // not supported yet if ( CurrentThreadId == gdwMainThreadId )
        AptTypeTemp = APTTYPE_MAINSTA;
      *pAptType = AptTypeTemp;
      if ( (OleFlags & 0x400000) != 0 )
        *pAptQualifier = APTTYPEQUALIFIER_APPLICATION_STA;
   
      if ( (OleFlags & 0x40000000) != 0 )
        *pAptQualifier = APTTYPEQUALIFIER_RESERVED_1;
      return Status;
    }
    if ( (OleFlags & 0x1100) != 0 )
    {
      *pAptType = APTTYPE_MTA;
      return Status;
    }
    if ( CoGetContextToken((ULONG_PTR*)&ContextToken) == 0 )
    {
ImplicitMTA:
      *pAptType = APTTYPE_MTA;
      *pAptQualifier = APTTYPEQUALIFIER_IMPLICIT_MTA;
      return Status;
    }
    return CO_E_NOTINITIALIZED;
  }
  AptQualifierTemp = APTTYPEQUALIFIER_NA_ON_MTA;
  *pAptType = APTTYPE_NA;
  if ( (OleFlags & 0x80) != 0 )
  {
    AptQualifierTemp = APTTYPEQUALIFIER_NA_ON_STA;
    if (FALSE) // not supported yet if ( CurrentThreadId == gdwMainThreadId )
      AptQualifierTemp = APTTYPEQUALIFIER_NA_ON_MAINSTA;
    *pAptQualifier = AptQualifierTemp;
	return Status;
  }
  if ( (OleFlags & 0x1100) != 0 )
  {
    *pAptQualifier = AptQualifierTemp;
    return Status;
  }
  if ( TRUE ) // TODO: make replacement for this.
  {
    *pAptQualifier = APTTYPEQUALIFIER_NA_ON_IMPLICIT_MTA;
    return Status;
  }
  return E_FAIL;
}
