/*++

Copyright (c) 2024 Shorthorn Project

Module Name:

    string.c

Abstract:

    This module implements Win32 Shell String Functions

Author:

    Skulltrail 10-September-2024

Revision History:

--*/

#include "main.h"

BOOL WINAPI IsCharSpaceA(CHAR c)
{
    WORD CharType;
    return GetStringTypeA(GetSystemDefaultLCID(), CT_CTYPE1, &c, 1, &CharType) && (CharType & C1_SPACE);
}