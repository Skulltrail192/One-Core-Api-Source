/*++

Copyright (c) 2025 Shorthorn Project

Module Name:

    notification.c

Abstract:

    This module implements RTL WNF Notification APIs

Author:

    Skulltrail 23-Jaunary-2025

Revision History:

--*/
 
#define NDEBUG

#include <main.h> 

NTSTATUS 
NTAPI 
RtlUnsubscribeWnfStateChangeNotification(
	PVOID RegistrationHandle
)
{
	DbgPrint("UNIMPLEMENTED: RtlUnsubscribeWnfStateChangeNotification");	
	return STATUS_SUCCESS;
}