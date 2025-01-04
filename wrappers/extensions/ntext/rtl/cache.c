/*++

Copyright (c) 2018 Shorthorn Project

Module Name:

    cache.c

Abstract:

    Implement secure memory callback functions

Author:

    Skulltrail 18-March-2018

Revision History:

--*/ 
 
#define NDEBUG

#include <main.h>

BOOL 
NTAPI
RtlDeregisterSecureMemoryCacheCallback(
  _In_  PRTL_SECURE_MEMORY_CACHE_CALLBACK pfnCallBack
)
{
	return TRUE;
}
