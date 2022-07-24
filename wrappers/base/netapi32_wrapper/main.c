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

#include <wine/config.h>

#include <ntstatus.h>
#define WIN32_NO_STATUS

#include <wine/debug.h>

#include <winbase.h>
#include <ntsecapi.h>
#include <bcrypt.h>
#include <stdlib.h>
#include <stdio.h>
#include <ndk/rtlfuncs.h>
#include <ndk/iofuncs.h>
#include <ndk/obfuncs.h>
#include <strsafe.h>

WINE_DEFAULT_DEBUG_CHANNEL(bcrypt);

BOOL WINAPI DllMain(HINSTANCE hInstDLL, DWORD fdwReason, LPVOID lpv)
{
    TRACE("fdwReason %u\n", fdwReason);

    switch(fdwReason)
    {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hInstDLL);
            break;
    }

    return TRUE;
}

HRESULT WINAPI I_NetWkstaResetDfsCache()
{
  HRESULT result; // eax@1
  DWORD OptionsSupported; // [sp+18h] [bp-1Ch]@1

  OptionsSupported = 0;
  result = 2470;
  if ( result == 2470 )
    OptionsSupported = 1;
  if ( OptionsSupported )
    result = 50;
  return result;
}

/************************************************************
 *                DavGetHTTPFromUNCPath (NETAPI32.@)
 */
DWORD WINAPI DavGetHTTPFromUNCPath(const WCHAR *unc_path, WCHAR *buf, DWORD *buflen)
{
    static const WCHAR httpW[] = L"http://";
    static const WCHAR httpsW[] = L"https://";
    const WCHAR *p = unc_path, *q, *server, *path, *scheme = httpW;
    UINT i, len_server, len_path = 0, len_port = 0, len, port = 0;
    WCHAR *end, portbuf[12];

    TRACE("(%s %p %p)\n", debugstr_w(unc_path), buf, buflen);

    if (p[0] != '\\' || p[1] != '\\' || !p[2]) return ERROR_INVALID_PARAMETER;
    q = p += 2;
    while (*q && *q != '\\' && *q != '/' && *q != '@') q++;
    server = p;
    len_server = q - p;
    if (*q == '@')
    {
        p = ++q;
        while (*p && (*p != '\\' && *p != '/' && *p != '@')) p++;
        if (p - q == 3 && !_wcsnicmp( q, L"SSL", 3 ))
        {
            scheme = httpsW;
            q = p;
        }
        else if ((port = wcstol( q, &end, 10 ))) q = end;
        else return ERROR_INVALID_PARAMETER;
    }
    if (*q == '@')
    {
        if (!(port = wcstol( ++q, &end, 10 ))) return ERROR_INVALID_PARAMETER;
        q = end;
    }
    if (*q == '\\' || *q  == '/') q++;
    path = q;
    while (*q++) len_path++;
    if (len_path && (path[len_path - 1] == '\\' || path[len_path - 1] == '/'))
        len_path--; /* remove trailing slash */

    swprintf( portbuf, L":%u", port );
    if (scheme == httpsW)
    {
        len = wcslen( httpsW );
        if (port && port != 443) len_port = wcslen( portbuf );
    }
    else
    {
        len = wcslen( httpW );
        if (port && port != 80) len_port = wcslen( portbuf );
    }
    len += len_server;
    len += len_port;
    if (len_path) len += len_path + 1; /* leading '/' */
    len++; /* nul */

    if (*buflen < len)
    {
        *buflen = len;
        return ERROR_INSUFFICIENT_BUFFER;
    }

    memcpy( buf, scheme, wcslen(scheme) * sizeof(WCHAR) );
    buf += wcslen( scheme );
    memcpy( buf, server, len_server * sizeof(WCHAR) );
    buf += len_server;
    if (len_port)
    {
        memcpy( buf, portbuf, len_port * sizeof(WCHAR) );
        buf += len_port;
    }
    if (len_path)
    {
        *buf++ = '/';
        for (i = 0; i < len_path; i++)
        {
            if (path[i] == '\\') *buf++ = '/';
            else *buf++ = path[i];
        }
    }
    *buf = 0;
    *buflen = len;

    return ERROR_SUCCESS;
}

/************************************************************
 *                DavGetUNCFromHTTPPath (NETAPI32.@)
 */
DWORD WINAPI DavGetUNCFromHTTPPath(const WCHAR *http_path, WCHAR *buf, DWORD *buflen)
{
    static const WCHAR httpW[] = {'h','t','t','p'};
    static const WCHAR httpsW[] = {'h','t','t','p','s'};
    static const WCHAR davrootW[] = {'\\','D','a','v','W','W','W','R','o','o','t'};
    static const WCHAR sslW[] = {'@','S','S','L'};
    static const WCHAR port80W[] = {'8','0'};
    static const WCHAR port443W[] = {'4','4','3'};
    const WCHAR *p = http_path, *server, *port = NULL, *path = NULL;
    DWORD i, len = 0, len_server = 0, len_port = 0, len_path = 0;
    BOOL ssl;

    TRACE("(%s %p %p)\n", debugstr_w(http_path), buf, buflen);

    while (*p && *p != ':') { p++; len++; };
    if (len == ARRAY_SIZE(httpW) && !_wcsnicmp( http_path, httpW, len )) ssl = FALSE;
    else if (len == ARRAY_SIZE(httpsW) && !_wcsnicmp( http_path, httpsW, len )) ssl = TRUE;
    else return ERROR_INVALID_PARAMETER;

    if (p[0] != ':' || p[1] != '/' || p[2] != '/') return ERROR_INVALID_PARAMETER;
    server = p += 3;

    while (*p && *p != ':' && *p != '/') { p++; len_server++; };
    if (!len_server) return ERROR_BAD_NET_NAME;
    if (*p == ':')
    {
        port = ++p;
        while (*p >= '0' && *p <= '9') { p++; len_port++; };
        if (len_port == 2 && !ssl && !memcmp( port, port80W, sizeof(port80W) )) port = NULL;
        else if (len_port == 3 && ssl && !memcmp( port, port443W, sizeof(port443W) )) port = NULL;
        path = p;
    }
    else if (*p == '/') path = p;

    while (*p)
    {
        if (p[0] == '/' && p[1] == '/') return ERROR_BAD_NET_NAME;
        p++; len_path++;
    }
    if (len_path && path[len_path - 1] == '/') len_path--;

    len = len_server + 2; /* \\ */
    if (ssl) len += 4; /* @SSL */
    if (port) len += len_port + 1 /* @ */;
    len += ARRAY_SIZE(davrootW);
    len += len_path + 1; /* nul */

    if (*buflen < len)
    {
        *buflen = len;
        return ERROR_INSUFFICIENT_BUFFER;
    }

    buf[0] = buf[1] = '\\';
    buf += 2;
    memcpy( buf, server, len_server * sizeof(WCHAR) );
    buf += len_server;
    if (ssl)
    {
        memcpy( buf, sslW, sizeof(sslW) );
        buf += 4;
    }
    if (port)
    {
        *buf++ = '@';
        memcpy( buf, port, len_port * sizeof(WCHAR) );
        buf += len_port;
    }
    memcpy( buf, davrootW, sizeof(davrootW) );
    buf += ARRAY_SIZE(davrootW);
    for (i = 0; i < len_path; i++)
    {
        if (path[i] == '/') *buf++ = '\\';
        else *buf++ = path[i];
    }

    *buf = 0;
    *buflen = len;

    return ERROR_SUCCESS;
}

DWORD 
DavGetExtendedError(
	HANDLE hFile, 
	DWORD *ExtError, 
	LPWSTR ExtErrorString, 
	DWORD *cChSize
)
{
  NTSTATUS Status; // eax MAPDST
  struct _OBJECT_ATTRIBUTES ObjectAttributes; // [esp+Ch] [ebp-844h]
  LSA_UNICODE_STRING DestinationString; // [esp+24h] [ebp-82Ch]
  struct _IO_STATUS_BLOCK IoStatusBlock; // [esp+2Ch] [ebp-824h]
  BOOL bPresent; // [esp+38h] [ebp-818h]
  STRSAFE_LPWSTR pszDest; // [esp+3Ch] [ebp-814h]
  HANDLE FileHandle; // [esp+40h] [ebp-810h]
  DWORD Response; // [esp+44h] [ebp-80Ch] MAPDST
  DWORD OutputBuffer; // [esp+48h] [ebp-808h]
  STRSAFE_LPCWSTR Dst; // [esp+4Ch] [ebp-804h]

  FileHandle = hFile;
  pszDest = ExtErrorString;
  ObjectAttributes.Length = 0;
  ObjectAttributes.RootDirectory = 0;
  ObjectAttributes.ObjectName = 0;
  ObjectAttributes.Attributes = 0;
  ObjectAttributes.SecurityDescriptor = 0;
  ObjectAttributes.SecurityQualityOfService = 0;
  IoStatusBlock.Status = 0;
  IoStatusBlock.Information = 0;
  DestinationString.Length = 0;
  DestinationString.MaximumLength = 0;
  DestinationString.Buffer = 0;
  OutputBuffer = 0;
  memset(&Dst, 0, 0x800u);
  bPresent = 0;
  Response = 0;
  if ( !cChSize || !pszDest || !ExtError )
    return 87;
  if ( *cChSize >= 0x400 )
  {
    if ( FileHandle == (HANDLE)-1 )
    {
      RtlInitUnicodeString(&DestinationString, L"\\device\\webdavredirector");
      ObjectAttributes.ObjectName = &DestinationString;
      ObjectAttributes.Length = 24;
      ObjectAttributes.RootDirectory = 0;
      ObjectAttributes.Attributes = 0;
      ObjectAttributes.SecurityDescriptor = 0;
      ObjectAttributes.SecurityQualityOfService = 0;
      IoStatusBlock.Status = 0;
      IoStatusBlock.Information = 0;
      Status = NtCreateFile(&FileHandle, 0x80000000, &ObjectAttributes, &IoStatusBlock, 0, 0, 7u, 1u, 0, 0, 0);
      if ( Status < 0 )
        return RtlNtStatusToDosError(Status);
      bPresent = 1;
    }
    Status = NtFsControlFile(FileHandle, 0, 0, 0, &IoStatusBlock, 0x4000080u, 0, 0, &OutputBuffer, 0x804u);
    if ( Status >= 0 )
    {
      *ExtError = OutputBuffer;
      Response = StringCchCopyW(pszDest, 0x400u, Dst);
      if ( (Response & 0x80000000) == 0 )
        *cChSize = wcslen(pszDest) + 1;
      else
        Response = (unsigned __int16)Response;
    }
    else
    {
      Response = RtlNtStatusToDosError(Status);
    }
    if ( bPresent )
      NtClose(FileHandle);
    return Response;
  }
  *cChSize = 1024;
  return 122;
}

ULONG 
DavFlushFile(
	HANDLE FileHandle
)
{
  NTSTATUS Status; // eax
  ULONG result; // eax
  struct _IO_STATUS_BLOCK IoStatusBlock; // [esp+8h] [ebp-8h]

  IoStatusBlock.Status = 0;
  IoStatusBlock.Information = 0;
  Status = NtFsControlFile(FileHandle, 0, 0, 0, &IoStatusBlock, 0x400007Fu, 0, 0, 0, 0);
  if ( Status >= 0 )
    result = 0;
  else
    result = RtlNtStatusToDosError(Status);
  return result;
}

ULONG DavDeleteConnection(HANDLE FileHandle)
{
  ULONG Response; // esi
  NTSTATUS Status; // eax
  struct _IO_STATUS_BLOCK IoStatusBlock; // [esp+4h] [ebp-Ch]
  int InputBuffer; // [esp+Ch] [ebp-4h]

  Response = 0;
  InputBuffer = 2;
  Status = NtFsControlFile(FileHandle, 0, 0, 0, &IoStatusBlock, 0x4000020u, &InputBuffer, 4u, 0, 0);
  if ( Status < 0 )
    Response = RtlNtStatusToDosError(Status);
  return Response;
}