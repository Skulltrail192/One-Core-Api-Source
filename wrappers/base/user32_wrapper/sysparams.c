/*++

Copyright (c) 2022 Shorthorn Project

Module Name:

    syspams.c

Abstract:

    Implement System parameters functions

Author:

    Skulltrail 14-February-2022

Revision History:

--*/

#include <main.h>

WINE_DEFAULT_DEBUG_CHANNEL(user32);

BOOL WINAPI GetDisplayAutoRotationPreferences(
  _Out_  ORIENTATION_PREFERENCE *pOrientation
)
{
	*pOrientation = ORIENTATION_PREFERENCE_NONE;
	DbgPrint("GetDisplayAutoRotationPreferences is UNIMPLEMENTED\n");	
	return TRUE;
}

/**********************************************************************
  *              GetAutoRotationState [USER32.@]
  */
BOOL WINAPI GetAutoRotationState( AR_STATE *state )
{
    FIXME("(%p): stub\n", state);
    *state = AR_NOT_SUPPORTED;
    return TRUE;
 }

BOOL WINAPI SetDisplayAutoRotationPreferences(
  _In_  ORIENTATION_PREFERENCE pOrientation
)
{
	Orientation = pOrientation;
	DbgPrint("SetDisplayAutoRotationPreferences is UNIMPLEMENTED\n");	
	return TRUE;
}