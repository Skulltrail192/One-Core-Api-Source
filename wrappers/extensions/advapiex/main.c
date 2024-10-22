/*
 * Copyright 2009 Henri Verbeet for CodeWeavers
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 *
 */

/* INCLUDES ******************************************************************/

/* C Headers */
#include "main.h"

extern BOOL RegInitialize(VOID);

WINE_DEFAULT_DEBUG_CHANNEL(main); 	

BOOL WINAPI DllMain(HINSTANCE hInstDLL, DWORD fdwReason, LPVOID lpv)
{
    switch(fdwReason)
    {
        case DLL_PROCESS_ATTACH:
           RegInitialize();		
           break;
    }

    return TRUE;
}

/*
DWORD WINAPI NotifyServiceStatusChangeW(
  _In_  SC_HANDLE hService,
  _In_  DWORD dwNotifyMask,
  _In_  PSERVICE_NOTIFY pNotifyBuffer
)
{
	return ERROR_SUCCESS;
}
*/

NTSTATUS 
WINAPI
LsaLookupSids2(
  _In_  LSA_HANDLE                  PolicyHandle,
  _In_  ULONG                       LookupOptions,
  _In_  ULONG                       Count,
  _In_  PSID                        *Sids,
  _Out_ PLSA_REFERENCED_DOMAIN_LIST *ReferencedDomains,
  _Out_ PLSA_TRANSLATED_NAME        *Names
)
{
	return LsaLookupSids(PolicyHandle,
						 Count,
						 Sids,
						 ReferencedDomains,
						 Names);
}

ULONG
WINAPI
WmiSendDataBlock(ULONG *InputBuffer, int a2, int a3, int a4, int a5)
{
	return ERROR_SUCCESS;
}


/******************************************************************************
 * QueryWindows31FilesMigration [ADVAPI32.@]
 *
 * PARAMS
 *   x1 []
 */
DWORD WINAPI
QueryWindows31FilesMigration( DWORD x1 )
{
	//Need a getprocaddress
	FIXME("(%d):stub\n",x1);
	return 0;
}

/******************************************************************************
 * SynchronizeWindows31FilesAndWindowsNTRegistry [ADVAPI32.@]
 *
 * PARAMS
 *   x1 []
 *   x2 []
 *   x3 []
 *   x4 []
 */
BOOL WINAPI
SynchronizeWindows31FilesAndWindowsNTRegistry( DWORD x1, DWORD x2, DWORD x3,
                                               DWORD x4 )
{
	//Need a getprocaddress
	FIXME("(0x%08x,0x%08x,0x%08x,0x%08x):stub\n",x1,x2,x3,x4);
	return TRUE;
}