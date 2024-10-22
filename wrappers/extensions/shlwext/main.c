/*++

Copyright (c) 2024 Shorthorn Project

Module Name:

    main.c

Abstract:

    This module implements Win32 Shell Main Functions

Author:

    Skulltrail 10-September-2024

Revision History:

--*/

#include "main.h"

WINE_DEFAULT_DEBUG_CHANNEL(shlwapi);

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

HRESULT 
WINAPI
PathMatchSpecExA(
  _In_ LPCTSTR pszFile,
  _In_ LPCTSTR pszSpec,
  _In_ DWORD   dwFlags
)
{
	return PathMatchSpecA(pszFile, pszSpec);
}

HRESULT 
WINAPI
PathMatchSpecExW(
  _In_ LPWSTR pszFile,
  _In_ LPWSTR pszSpec,
  _In_ DWORD   dwFlags
)
{
	return PathMatchSpecW(pszFile, pszSpec);
}

HRESULT 
WINAPI 
SHAutoCompGetPidl(
	HWND hWnd, 
	LPARAM windowParam, 
	int Unknown, 
	WPARAM wParam
)
{
  UINT WindowMessage; // eax@1
  LRESULT LocalResult; // eax@3
  HRESULT result; // eax@4
  LPARAM lParam; // [sp+0h] [bp-8h]@1
  int v8; // [sp+4h] [bp-4h]@1

  WindowMessage = RegisterWindowMessageA("AC_GetPidl");
  lParam = (LPARAM)&windowParam;
  v8 = Unknown;
  if ( !WindowMessage )
    WindowMessage = 33070;
  LocalResult = SendMessageA(hWnd, WindowMessage, wParam, (LPARAM)&lParam);
  if ( LocalResult )
    result = LocalResult != 1 ? LocalResult : 0;
  else
    result = 0x80070057u;
  return result;
}

/*************************************************************************
 * PathCreateFromUrlAlloc   [SHLWAPI.@]
 */
HRESULT 
WINAPI 
PathCreateFromUrlAlloc(
	LPCWSTR pszUrl, 
	LPWSTR *pszPath,
    DWORD dwReserved
)
{
    WCHAR pathW[MAX_PATH];
    DWORD size;
    HRESULT hr;

    size = MAX_PATH;
    hr = PathCreateFromUrlW(pszUrl, pathW, &size, dwReserved);
    if (SUCCEEDED(hr))
    {
        /* Yes, this is supposed to crash if pszPath is NULL */
        *pszPath = StrDupW(pathW);
    }
    return hr;
}

static void FillNumberFmt(NUMBERFMTW *fmt, LPWSTR decimal_buffer, int decimal_bufwlen,
                          LPWSTR thousand_buffer, int thousand_bufwlen)
{
  WCHAR grouping[64];
  WCHAR *c;

  GetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_ILZERO|LOCALE_RETURN_NUMBER, (LPWSTR)&fmt->LeadingZero, sizeof(fmt->LeadingZero)/sizeof(WCHAR));
  GetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_INEGNUMBER|LOCALE_RETURN_NUMBER, (LPWSTR)&fmt->NegativeOrder, sizeof(fmt->NegativeOrder)/sizeof(WCHAR));
  fmt->NumDigits = 0;
  GetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_SDECIMAL, decimal_buffer, decimal_bufwlen);
  GetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_STHOUSAND, thousand_buffer, thousand_bufwlen);
  fmt->lpThousandSep = thousand_buffer;
  fmt->lpDecimalSep = decimal_buffer;

  /* 
   * Converting grouping string to number as described on 
   * http://blogs.msdn.com/oldnewthing/archive/2006/04/18/578251.aspx
   */
  fmt->Grouping = 0;
  GetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_SGROUPING, grouping, ARRAY_SIZE(grouping));
  for (c = grouping; *c; c++)
    if (*c >= '0' && *c < '9')
    {
      fmt->Grouping *= 10;
      fmt->Grouping += *c - '0';
    }

  if (fmt->Grouping % 10 == 0)
    fmt->Grouping /= 10;
  else
    fmt->Grouping *= 10;
}

/*************************************************************************
 * FormatDouble   [internal]
 *
 * Format an integer according to the current locale. Prints the specified number of digits
 * after the decimal point
 *
 * RETURNS
 *  The number of characters written on success or 0 on failure
 */
static int FormatDouble(double value, int decimals, LPWSTR pszBuf, int cchBuf)
{
  static const WCHAR flfmt[] = {'%','f',0};
  WCHAR buf[64];
  NUMBERFMTW fmt;
  WCHAR decimal[8], thousand[8];
  
  swprintf(buf, flfmt, value);

  FillNumberFmt(&fmt, decimal, ARRAY_SIZE(decimal), thousand, ARRAY_SIZE(thousand));
  fmt.NumDigits = decimals;
  return GetNumberFormatW(LOCALE_USER_DEFAULT, 0, buf, &fmt, pszBuf, cchBuf);
}

/*************************************************************************
 * StrFormatByteSizeEx  [SHLWAPI.@]
 *
 */

HRESULT WINAPI StrFormatByteSizeEx(LONGLONG llBytes, SFBS_FLAGS flags, LPWSTR lpszDest,
                                   UINT cchMax)
{
#define KB ((ULONGLONG)1024)
#define MB (KB*KB)
#define GB (KB*KB*KB)
#define TB (KB*KB*KB*KB)
#define PB (KB*KB*KB*KB*KB)

  static  SHLWAPI_BYTEFORMATS bfFormats[] =
  {
    { 10*KB, 10.24, 100.0, 2, 'K' }, /* 10 KB */
    { 100*KB, 102.4, 10.0, 1, 'K' }, /* 100 KB */
    { 1000*KB, 1024.0, 1.0, 0, 'K' }, /* 1000 KB */
    { 10*MB, 10485.76, 100.0, 2, 'M' }, /* 10 MB */
    { 100*MB, 104857.6, 10.0, 1, 'M' }, /* 100 MB */
    { 1000*MB, 1048576.0, 1.0, 0, 'M' }, /* 1000 MB */
    { 10*GB, 10737418.24, 100.0, 2, 'G' }, /* 10 GB */
    { 100*GB, 107374182.4, 10.0, 1, 'G' }, /* 100 GB */
    { 1000*GB, 1073741824.0, 1.0, 0, 'G' }, /* 1000 GB */
    { 10*TB, 10485.76, 100.0, 2, 'T' }, /* 10 TB */
    { 100*TB, 104857.6, 10.0, 1, 'T' }, /* 100 TB */
    { 1000*TB, 1048576.0, 1.0, 0, 'T' }, /* 1000 TB */
    { 10*PB, 10737418.24, 100.00, 2, 'P' }, /* 10 PB */
    { 100*PB, 107374182.4, 10.00, 1, 'P' }, /* 100 PB */
    { 1000*PB, 1073741824.0, 1.00, 0, 'P' }, /* 1000 PB */
    { 0, 10995116277.76, 100.00, 2, 'E' } /* EB's, catch all */
  };
  WCHAR wszAdd[] = {' ','?','B',0};
  double dBytes;
  UINT i = 0;
  HINSTANCE hInst = LoadLibraryW(L"shlwapibase.dll");

  TRACE("(0x%s,%d,%p,%d)\n", wine_dbgstr_longlong(llBytes), flags, lpszDest, cchMax);

  if (!cchMax)
    return E_INVALIDARG;

  if (llBytes < 1024)  /* 1K */
  {
    WCHAR wszBytesFormat[64];
    LoadStringW(hInst, IDS_BYTES_FORMAT, wszBytesFormat, 64);
    swprintf(lpszDest,  wszBytesFormat, (int)llBytes);
    return S_OK;
  }

  /* Note that if this loop completes without finding a match, i will be
   * pointing at the last entry, which is a catch all for > 1000 PB
   */
  while (i < ARRAY_SIZE(bfFormats) - 1)
  {
    if (llBytes < bfFormats[i].dLimit)
      break;
    i++;
  }
  /* Above 1 TB we encounter problems with FP accuracy. So for amounts above
   * this number we integer shift down by 1 MB first. The table above has
   * the divisors scaled down from the '< 10 TB' entry onwards, to account
   * for this. We also add a small fudge factor to get the correct result for
   * counts that lie exactly on a 1024 byte boundary.
   */
  if (i > 8)
    dBytes = (double)(llBytes >> 20) + 0.001; /* Scale down by 1 MB */
  else
    dBytes = (double)llBytes + 0.00001;

  switch(flags)
  {
  case SFBS_FLAGS_ROUND_TO_NEAREST_DISPLAYED_DIGIT:
      dBytes = round(dBytes / bfFormats[i].dDivisor) / bfFormats[i].dNormaliser;
      break;
  case SFBS_FLAGS_TRUNCATE_UNDISPLAYED_DECIMAL_DIGITS:
      dBytes = floor(dBytes / bfFormats[i].dDivisor) / bfFormats[i].dNormaliser;
      break;
  default:
      return E_INVALIDARG;
  }

  if (!FormatDouble(dBytes, bfFormats[i].nDecimals, lpszDest, cchMax))
    return E_FAIL;

  wszAdd[1] = bfFormats[i].wPrefix;
  StrCatBuffW(lpszDest, wszAdd, cchMax);
  return S_OK;
}

DWORD __stdcall GetLastErrorError()
{
  DWORD result; // eax

  result = GetLastError();
  if ( !result )
    return 1;
  return result;
}

HRESULT WINAPI HRESULTFromLastErrorError()
{
  signed int result; // eax

  result = GetLastErrorError();
  if ( result > 0 )
    return (unsigned __int16)result | 0x80070000;
  return result;
}

HRESULT GetModuleResourceData(HMODULE hModule, LPCWSTR lpName, LPCWSTR lpType, DWORD *a4, DWORD *a5)
{
  HRSRC ResourceW; // eax
  HRSRC v6; // esi
  DWORD v7; // edi
  HGLOBAL Resource; // eax
  LPVOID v9; // eax

  ResourceW = FindResourceW(hModule, lpName, lpType);
  v6 = ResourceW;
  if ( !ResourceW )
    return HRESULTFromLastErrorError();
  v7 = SizeofResource(hModule, ResourceW);
  if ( !v7 )
    return HRESULTFromLastErrorError();
  Resource = LoadResource(hModule, v6);
  if ( !Resource )
    return HRESULTFromLastErrorError();
  v9 = LockResource(Resource);
  if ( !v9 )
    return HRESULTFromLastErrorError();
  *a4 = (DWORD)v9;
  *a5 = v7;
  return 0;
}

HRESULT WINAPI CreateStreamSTOnModuleResource(HMODULE hModule, BYTE *pInit, const WCHAR *cbInit, IStream **a4)
{
  HRESULT ModuleResourceData; // esi
  IStream *Stream; // eax

  ModuleResourceData = GetModuleResourceData(hModule, (LPCWSTR)pInit, cbInit, (DWORD*)&pInit, (DWORD*)&cbInit);
  if ( ModuleResourceData >= 0 )
  {
    Stream = SHCreateMemStream(pInit, (UINT)cbInit);
    *a4 = Stream;
    if ( !Stream )
      return 0x8007000E;
  }
  return ModuleResourceData;
}

#ifdef _M_IX86
HRESULT WINAPI SHCreateStreamOnModuleResourceW(HMODULE a1, BYTE *a2, const WCHAR *a3, IStream **a4)
{
	return CreateStreamSTOnModuleResource(a1, a2, a3, a4); 
}
#else
HRESULT WINAPI SHCreateStreamOnModuleResourceW(HMODULE hModule, BYTE *pInit, const WCHAR *cbInit, PVOID **Cstream)
{
  HRESULT ModuleResourceData; // esi
  PVOID *Stream; // eax

  ModuleResourceData = GetModuleResourceData(hModule, (LPCWSTR)pInit, cbInit, (DWORD*)&pInit, (DWORD*)&cbInit);
  if ( ModuleResourceData >= 0 )
  {
    Stream = (PVOID*)SHCreateMemStream(pInit, (UINT)cbInit);
    *Cstream = Stream;
    if ( !Stream )
      return 0x8007000E;
  }
  return ModuleResourceData;
}
#endif

HRESULT WINAPI IStream_Copy(IStream *pstmFrom, IStream *pstmTo, DWORD cb)
{
  HRESULT result; // eax
  int v4[2]; // [esp+Ch] [ebp-Ch] BYREF

  result = ((int (__stdcall *)(IStream *, IStream *, DWORD, DWORD, DWORD, int *))pstmFrom->lpVtbl->CopyTo)(
             pstmFrom,
             pstmTo,
             cb,
             0,
             0,
             v4);

  return result;
}

/*************************************************************************
 * PathIsRootW		[SHLWAPI.@]
 *
 * See PathIsRootA.
 */
BOOL WINAPI PathIsRootW(LPCWSTR lpszPath)
{
  TRACE("(%s)\n", debugstr_w(lpszPath));

  if (lpszPath && *lpszPath)
  {
    if (*lpszPath == '\\')
    {
      if (!lpszPath[1])
        return TRUE; /* \ */
      else if (lpszPath[1]=='\\')
      {
        BOOL bSeenSlash = FALSE;
        lpszPath += 2;

        /* Check for UNC root path */
        while (*lpszPath)
        {
          if (*lpszPath == '\\')
          {
            if (bSeenSlash)
              return FALSE;
            bSeenSlash = TRUE;
          }
          lpszPath++;
        }
        return TRUE;
      }
    }
    else if (lpszPath[1] == ':' && lpszPath[2] == '\\' && lpszPath[3] == '\0')
      return TRUE; /* X:\ */
  }
  return FALSE;
}

/*************************************************************************
 * PathIsRootA		[SHLWAPI.@]
 *
 * Determine if a path is a root path.
 *
 * PARAMS
 *  lpszPath [I] Path to check
 *
 * RETURNS
 *  TRUE  If lpszPath is valid and a root path,
 *  FALSE Otherwise
 */
BOOL WINAPI PathIsRootA(LPCSTR lpszPath)
{
  TRACE("(%s)\n", debugstr_a(lpszPath));

  if (lpszPath && *lpszPath)
  {
    if (*lpszPath == '\\')
    {
      if (!lpszPath[1])
        return TRUE; /* \ */
      else if (lpszPath[1]=='\\')
      {
        BOOL bSeenSlash = FALSE;
        lpszPath += 2;

        /* Check for UNC root path */
        while (*lpszPath)
        {
          if (*lpszPath == '\\')
          {
            if (bSeenSlash)
              return FALSE;
            bSeenSlash = TRUE;
          }
          lpszPath = CharNextA(lpszPath);
        }
        return TRUE;
      }
    }
    else if (lpszPath[1] == ':' && lpszPath[2] == '\\' && lpszPath[3] == '\0')
      return TRUE; /* X:\ */
  }
  return FALSE;
}

/*************************************************************************
 * PathRemoveFileSpecW	[SHLWAPI.@]
 *
 * See PathRemoveFileSpecA.
 */
BOOL WINAPI PathRemoveFileSpecW(LPWSTR lpszPath)
{
  LPWSTR lpszFileSpec = lpszPath;
  BOOL bModified = FALSE;

  TRACE("(%s)\n",debugstr_w(lpszPath));

  if(lpszPath)
  {
    /* Skip directory or UNC path */
    if (*lpszPath == '\\')
      lpszFileSpec = ++lpszPath;
    if (*lpszPath == '\\')
      lpszFileSpec = ++lpszPath;

    while (*lpszPath)
    {
      if(*lpszPath == '\\')
        lpszFileSpec = lpszPath; /* Skip dir */
      else if(*lpszPath == ':')
      {
        lpszFileSpec = ++lpszPath; /* Skip drive */
        if (*lpszPath == '\\')
          lpszFileSpec++;
      }
      lpszPath++;
    }

    if (*lpszFileSpec)
    {
      *lpszFileSpec = '\0';
      bModified = TRUE;
    }
  }
  return bModified;
}

/*************************************************************************
 * PathStripToRootW	[SHLWAPI.@]
 *
 * See PathStripToRootA.
 */
BOOL WINAPI PathStripToRootW(LPWSTR lpszPath)
{
  TRACE("(%s)\n", debugstr_w(lpszPath));

  if (!lpszPath)
    return FALSE;
  while(!PathIsRootW(lpszPath))
    if (!PathRemoveFileSpecW(lpszPath))
      return FALSE;
  return TRUE;
}

/*************************************************************************
 * PathStripToRootA	[SHLWAPI.@]
 *
 * Reduce a path to its root.
 *
 * PARAMS
 *  lpszPath [I/O] the path to reduce
 *
 * RETURNS
 *  Success: TRUE if the stripped path is a root path
 *  Failure: FALSE if the path cannot be stripped or is NULL
 */
BOOL WINAPI PathStripToRootA(LPSTR lpszPath)
{
  TRACE("(%s)\n", debugstr_a(lpszPath));

  if (!lpszPath)
    return FALSE;
  while(!PathIsRootA(lpszPath))
    if (!PathRemoveFileSpecA(lpszPath))
      return FALSE;
  return TRUE;
}

BOOL WINAPI SHWindowsPolicy (REFGUID rpolid)
{
	SetLastError(0);
	return FALSE;
}