/*++

Copyright (c) 2023  Shorthorn Project

Module Name:

    firmware.c

Abstract:

    This module implements Win32 firmware access APIs

Author:

    Skulltrail 26-August-2023

Revision History:

--*/

#include "main.h"

BOOL 
WINAPI 
GetFirmwareType(
	WORD* firmwareType
	)
{
    HKEY hKey;
    DWORD dwType = 0;
    DWORD dwSize = sizeof(DWORD);
    DWORD firmwareValue;

    // Abrir a chave do registro do sistema que contÃ©m o valor do tipo de firmware
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Control\\SystemInformation", 0, KEY_READ, &hKey) == ERROR_SUCCESS)
    {
        // Ler o valor do tipo de firmware
        if (RegQueryValueEx(hKey, "SystemBiosVersion", NULL, &dwType, (LPBYTE)&firmwareValue, &dwSize) == ERROR_SUCCESS)
        {
            // Verificar o valor do tipo de firmware
            if (firmwareValue == 1)
            {
                *firmwareType = FirmwareTypeBios;
            }
            else if (firmwareValue == 2)
            {
                *firmwareType = FirmwareTypeUefi;
            }
            else
            {
                *firmwareType = FirmwareTypeUnknown;
            }

            RegCloseKey(hKey);
            return TRUE;
        }

        RegCloseKey(hKey);
    }

    return FALSE;
}
