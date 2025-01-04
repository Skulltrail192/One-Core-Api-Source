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

// Virtually every single 100% Rust-based application needs this to make any HTTP requests.
// While supported in Vista+, since Windows 8, those four I/O control codes simply return back the original socket, thus considered 'deprecated'
int WSAAPI WSAIoctlInternal(
  SOCKET                             s,
  DWORD                              dwIoControlCode,
  LPVOID                             lpvInBuffer,
  DWORD                              cbInBuffer,
  LPVOID                             lpvOutBuffer,
  DWORD                              cbOutBuffer,
  LPDWORD                            lpcbBytesReturned,
  LPWSAOVERLAPPED                    lpOverlapped,
  LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
) {
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
	// fall back into original function
	return WSAIoctl(s, dwIoControlCode, lpvInBuffer, cbInBuffer, lpvOutBuffer, cbOutBuffer, lpcbBytesReturned, lpOverlapped, lpCompletionRoutine); 
}

SOCKET WSAAPI WSASocketAInternal(int af, int type, int protocol,
                         LPWSAPROTOCOL_INFOA lpProtocolInfo,
                         GROUP g, DWORD dwFlags)
{
        SOCKET curSock;
    if(dwFlags & WSA_FLAG_NO_HANDLE_INHERIT){
                
        dwFlags ^= WSA_FLAG_NO_HANDLE_INHERIT;
                curSock = WSASocketA(af, type, protocol, lpProtocolInfo, g, dwFlags);
                if (curSock != INVALID_SOCKET) {
                    SetHandleInformation((HANDLE)curSock, HANDLE_FLAG_INHERIT, 0);
                }
        DbgPrint("WSASocketWInternal: flag is WSA_FLAG_NO_HANDLE_INHERIT\n");
                return curSock;
    }
    return WSASocketA(af, type, protocol, lpProtocolInfo, g, dwFlags);
}

SOCKET WSAAPI WSASocketWInternal(int af, int type, int protocol,
                         LPWSAPROTOCOL_INFOW lpProtocolInfo,
                         GROUP g, DWORD dwFlags)
{
        SOCKET curSock;
    if(dwFlags & WSA_FLAG_NO_HANDLE_INHERIT){
                
        dwFlags ^= WSA_FLAG_NO_HANDLE_INHERIT;
                curSock = WSASocketW(af, type, protocol, lpProtocolInfo, g, dwFlags);
                if (curSock != INVALID_SOCKET) {
                    SetHandleInformation((HANDLE)curSock, HANDLE_FLAG_INHERIT, 0);
                }
        DbgPrint("WSASocketWInternal: flag is WSA_FLAG_NO_HANDLE_INHERIT\n");
                return curSock;
    }
    return WSASocketW(af, type, protocol, lpProtocolInfo, g, dwFlags);
}

INT
WSAAPI
setsockoptInternal(
	IN SOCKET s,
    IN INT level,
    IN INT optname,
    IN CONST CHAR FAR* optval,
    IN INT optlen)
{
	if(level == IPPROTO_IPV6 && optname == IPV6_V6ONLY){
		return S_OK;
	}
	
	return setsockopt(s, level, optname, optval, optlen);
}