/*++

Copyright (c) 2023  Shorthorn Project

Module Name:

    efs.c

Abstract:

    EFS (Encrypting File System) API Interfaces

Author:

    Skulltrail 01-October-2023

Revision History:

--*/

#include "main.h"

DWORD
WINAPI
SetUserFileEncryptionKeyEx(
    PENCRYPTION_CERTIFICATE     pEncryptionCertificate,
    DWORD                       dwCapabilities, 
    DWORD                       dwFlags,
    LPVOID                      pvReserved
)
{
	return SetUserFileEncryptionKey(pEncryptionCertificate);
}