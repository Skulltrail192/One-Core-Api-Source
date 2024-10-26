/*++

Copyright (c) 2024 Shorthorn Project

Module Name:

    hooks.c

Abstract:

    This module implements Hooks of Native Windows Sockets 2 APIs

Author:

    Skulltrail 23-October-2024

Revision History:

--*/
 
#include "main.h"
#include "stubs.h"
#include "stdio.h"

/*
 * @implemented
 */
INT
WSAAPI
WSAIoctl(IN SOCKET s,
         IN DWORD dwIoControlCode,
         IN LPVOID lpvInBuffer,
         IN DWORD cbInBuffer,
         OUT LPVOID lpvOutBuffer,
         IN DWORD cbOutBuffer,
         OUT LPDWORD lpcbBytesReturned,
         IN LPWSAOVERLAPPED lpOverlapped,
         IN LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
)
{
    PWSSOCKET Socket;
    INT Status;
    INT ErrorCode;
    LPWSATHREADID ThreadId;
	
    DPRINT("WSAIoctl: %lx, %lx\n", s, dwIoControlCode);
	
	if (dwIoControlCode == SIO_BASE_HANDLE
        || dwIoControlCode == SIO_BSP_HANDLE
        || dwIoControlCode == SIO_BSP_HANDLE_SELECT
        || dwIoControlCode == SIO_BSP_HANDLE_POLL) { 
		if (lpvOutBuffer == NULL || lpcbBytesReturned == NULL) {
			WSASetLastError(WSAEFAULT);
			return SOCKET_ERROR;
		}

		*(SOCKET*)lpvOutBuffer = s;
		*lpcbBytesReturned = sizeof(SOCKET);
		return 0;
	}


    /* Check for WSAStartup */
    if ((ErrorCode = WsQuickPrologTid(&ThreadId)) == ERROR_SUCCESS)
    {
        /* Get the Socket Context */
        if ((Socket = WsSockGetSocket(s)))
        {
            /* Make the call */
            Status = Socket->Provider->Service.lpWSPIoctl(s,
                                                  dwIoControlCode,
                                                  lpvInBuffer,
                                                  cbInBuffer,
                                                  lpvOutBuffer,
                                                  cbOutBuffer,
                                                  lpcbBytesReturned,
                                                  lpOverlapped,
                                                  lpCompletionRoutine,
                                                  ThreadId,
                                                  &ErrorCode);

            /* Deference the Socket Context */
            WsSockDereference(Socket);

            /* Return Provider Value */
            if (Status == ERROR_SUCCESS) return Status;
        }
        else
        {
            /* No Socket Context Found */
            ErrorCode = WSAENOTSOCK;
        }
    }

    /* Return with an Error */
    SetLastError(ErrorCode);
    return SOCKET_ERROR;	
}