/*++

Copyright (c) 2022 Shorthorn Project

Module Name:

    input.c

Abstract:

    Implement Input functions

Author:

    Skulltrail 14-February-2022

Revision History:

--*/

#include <main.h>

/***********************************************************************
 *		EnableMouseInPointer (USER32.@)
 */
BOOL WINAPI EnableMouseInPointer(BOOL enable)
{
    DbgPrint("EnableMouseInPointer stub\n", enable);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

// HSYNTHETICPOINTERDEVICE WINAPI CreateSyntheticPointerDevice(POINTER_INPUT_TYPE type, ULONG max_count, POINTER_FEEDBACK_MODE mode)
// {
    // FIXME( "type %ld, max_count %ld, mode %d stub!\n", type, max_count, mode);
    // SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    // return NULL;
// }

// BOOL WINAPI InjectSyntheticPointerInput(
   // HSYNTHETICPOINTERDEVICE device,
   // const POINTER_TYPE_INFO *pointerInfo,
   // UINT32                  count
// )
// {
	// return FALSE;
// }
