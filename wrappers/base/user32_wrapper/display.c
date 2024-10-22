/*++

Copyright (c) 2022 Shorthorn Project

Module Name:

    display.c

Abstract:

    Implement Display Information functions

Author:

    Skulltrail 14-February-2022

Revision History:

--*/

#include <main.h>

WINE_DEFAULT_DEBUG_CHANNEL(user32);

LONG WINAPI DisplayConfigGetDeviceInfo(
  _Inout_  DISPLAYCONFIG_DEVICE_INFO_HEADER *requestPacket
)
{
	DbgPrint("DisplayConfigGetDeviceInfo is UNIMPLEMENTED\n");	
	return ERROR_SUCCESS;
}

LONG WINAPI DisplayConfigSetDeviceInfo(
  _Inout_  DISPLAYCONFIG_DEVICE_INFO_HEADER *setPacket
)
{
	DbgPrint("DisplayConfigSetDeviceInfo is UNIMPLEMENTED\n");	
	return ERROR_SUCCESS;
}

/**********************************************************************
  * GetDisplayConfigBufferSizes [USER32.@]
  */
LONG WINAPI GetDisplayConfigBufferSizes(UINT32 flags, UINT32 *num_path_info, UINT32 *num_mode_info)
{
     DbgPrint("(0x%x %p %p): stub\n", flags, num_path_info, num_mode_info);
 
     if (!num_path_info || !num_mode_info)
         return ERROR_INVALID_PARAMETER;
 
     *num_path_info = 0;
     *num_mode_info = 0;
    return ERROR_NOT_SUPPORTED;
}

LONG WINAPI QueryDisplayConfig(
  _In_       UINT32 Flags,
  _Inout_    UINT32 *pNumPathArrayElements,
  _Out_      DISPLAYCONFIG_PATH_INFO *pPathInfoArray,
  _Inout_    UINT32 *pNumModeInfoArrayElements,
  _Out_      DISPLAYCONFIG_MODE_INFO *pModeInfoArray,
  _Out_opt_  DISPLAYCONFIG_TOPOLOGY_ID *pCurrentTopologyId
)
{
	DbgPrint("QueryDisplayConfig is UNIMPLEMENTED\n");	
	return ERROR_CALL_NOT_IMPLEMENTED;
}

LONG 
WINAPI 
SetDisplayConfig(
  _In_      UINT32 numPathArrayElements,
  _In_opt_  DISPLAYCONFIG_PATH_INFO *pathArray,
  _In_      UINT32 numModeInfoArrayElements,
  _In_opt_  DISPLAYCONFIG_MODE_INFO *modeInfoArray,
  _In_      UINT32 Flags
)
{
	DbgPrint("SetDisplayConfig is UNIMPLEMENTED\n");	
	return ERROR_SUCCESS;
}

BOOL WINAPI GetWindowDisplayAffinity(
  _In_   HWND hWnd,
  _Out_  DWORD *dwAffinity
)
{
    DbgPrint("(%p, %p): stub\n", hWnd, dwAffinity);
    
    if (!hWnd || !dwAffinity) {
        SetLastError(hWnd ? ERROR_NOACCESS : ERROR_INVALID_WINDOW_HANDLE);
        return FALSE;
    }
    SetLastError(0); // Some password managers such as PWSafe throw an error when this does not exist.
    *dwAffinity = WDA_NONE;
    return TRUE;
}

BOOL WINAPI SetWindowDisplayAffinity(
  _In_  HWND hWnd,
  _In_  DWORD dwAffinity
)
{
        DbgPrint("(%p, %u): stub\n", hWnd, dwAffinity);
        SetLastError(0);
        return TRUE; // Some password managers throw an error when this code fails
}