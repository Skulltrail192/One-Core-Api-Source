/*++

Copyright (c) 2025 Shorthorn Project

Module Name:

    transaction.c

Abstract:

    This module implements RTL Transaction APIs

Author:

    Skulltrail 23-Jaunary-2025

Revision History:

--*/

#include <rtl.h>

/*
 * @implemented - new
*/
PVOID NTAPI RtlGetCurrentTransaction()
{
  return NtCurrentTeb()->CurrentTransactionHandle;
}
 
BOOL NTAPI RtlSetCurrentTransaction(PVOID new_transaction)
{
  BOOL result; // eax@2

  if ( new_transaction == (PVOID)-1 )
  {
    result = FALSE;
  }
  else
  {
    NtCurrentTeb()->CurrentTransactionHandle = new_transaction;
    result = TRUE;
  }
  return result;
}