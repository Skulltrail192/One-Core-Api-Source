/*
 *  ReactOS kernel
 *  Copyright (C) 2004 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            dll/win32/userenv/userenv.c
 * PURPOSE:         DLL initialization code
 * PROGRAMMER:      Eric Kohl
 */

#include "precomp.h"
#include <winhttp.h>

#define NDEBUG
#include <wine/debug.h>

HINSTANCE hInstance = NULL;

WINE_DEFAULT_DEBUG_CHANNEL(winhttp);

typedef struct _WINHTTP_PROXY_RESULT_ENTRY
{
    BOOL            fProxy;
    BOOL            fBypass;
    INTERNET_SCHEME ProxyScheme;
    PWSTR           pwszProxy;
    INTERNET_PORT   ProxyPort;
} WINHTTP_PROXY_RESULT_ENTRY;

typedef struct _WINHTTP_PROXY_RESULT
{
    DWORD cEntries;
    WINHTTP_PROXY_RESULT_ENTRY *pEntries;
} WINHTTP_PROXY_RESULT;

BOOL
WINAPI
DllMain(HINSTANCE hinstDLL,
        DWORD fdwReason,
        LPVOID lpvReserved)
{
    if (fdwReason == DLL_PROCESS_ATTACH)
    {
        hInstance = hinstDLL;
    }

    return TRUE;
}
		
DWORD WINAPI WinHttpWebSocketClose( HINTERNET hsocket, USHORT status, void *reason, DWORD len ) {
    return WinHttpCloseHandle(hsocket);
}

DWORD WINAPI WinHttpWebSocketReceive(HINTERNET hWebSocket, PVOID pvBuffer, DWORD dwBufferLength, DWORD *pdwBytesRead, enum WINHTTP_WEB_SOCKET_BUFFER_TYPE *peBufferType) {
	return WinHttpReceiveResponse(hWebSocket, pvBuffer);
}

DWORD WINAPI WinHttpWebSocketSend(HINTERNET hWebSocket, enum WINHTTP_WEB_SOCKET_BUFFER_TYPE *eBufferType, PVOID pvBuffer, DWORD dwBufferLength) {
	return WinHttpSendRequest(hWebSocket, ((LPCWSTR)pvBuffer), dwBufferLength, pvBuffer, 0, 0, 0);	
}

HINTERNET WINAPI WinHttpWebSocketCompleteUpgrade(HINTERNET hRequest, DWORD_PTR pContext) {
	return WinHttpSetStatusCallback(hRequest, NULL, WINHTTP_CALLBACK_FLAG_SENDREQUEST_COMPLETE, 0);
}


/***********************************************************************
 *          WinHttpCreateProxyResolver (winhttp.@)
 */
DWORD WINAPI WinHttpCreateProxyResolver( HINTERNET hsession, HINTERNET *hresolver )
{
    FIXME("%p, %p\n", hsession, hresolver);
    return ERROR_WINHTTP_AUTO_PROXY_SERVICE_ERROR;
}

/***********************************************************************
 *          WinHttpFreeProxyResult (winhttp.@)
 */
void WINAPI WinHttpFreeProxyResult( WINHTTP_PROXY_RESULT *result )
{
    FIXME("%p\n", result);
}


/***********************************************************************
 *          WinHttpGetProxyForUrlEx (winhttp.@)
 */
DWORD WINAPI WinHttpGetProxyForUrlEx( HINTERNET hresolver, const WCHAR *url, WINHTTP_AUTOPROXY_OPTIONS *options,
                                      DWORD_PTR ctx )
{
    FIXME( "%p, %s, %p, %Ix\n", hresolver, debugstr_w(url), options, ctx );
    return ERROR_WINHTTP_AUTO_PROXY_SERVICE_ERROR;
}


/***********************************************************************
 *          WinHttpGetProxyResult (winhttp.@)
 */
DWORD WINAPI WinHttpGetProxyResult( HINTERNET hresolver, WINHTTP_PROXY_RESULT *result )
{
    FIXME("%p, %p\n", hresolver, result);
    return ERROR_WINHTTP_AUTO_PROXY_SERVICE_ERROR;
}
