/*++

Copyright (c) 2017 Shorthorn Project

Module Name:

    pathmisc.c

Abstract:

    Win32 misceleneous path functions

Author:

    Skulltrail 25-June-2017

Revision History:

--*/

#include "main.h"

WINE_DEFAULT_DEBUG_CHANNEL(kernel32file);

HMODULE hshlwapi;

static LPWSTR (WINAPI *pPathCombineW)(LPWSTR ,LPCWSTR, LPCWSTR);

static HRESULT (WINAPI *pPathCreateFromUrlW)(PCWSTR, PWSTR, DWORD, DWORD);

#define FS_VOLUME_BUFFER_SIZE (MAX_PATH * sizeof(WCHAR) + sizeof(FILE_FS_VOLUME_INFORMATION))

#define FS_ATTRIBUTE_BUFFER_SIZE (MAX_PATH * sizeof(WCHAR) + sizeof(FILE_FS_ATTRIBUTE_INFORMATION))

/*
 * @implemented
 */
BOOL
WINAPI
GetVolumeInformationByHandleW(
	IN HANDLE hFile,
    IN LPWSTR lpVolumeNameBuffer,
    IN DWORD nVolumeNameSize,
    OUT LPDWORD lpVolumeSerialNumber OPTIONAL,
    OUT LPDWORD lpMaximumComponentLength OPTIONAL,
    OUT LPDWORD lpFileSystemFlags OPTIONAL,
    OUT LPWSTR lpFileSystemNameBuffer OPTIONAL,
    IN DWORD nFileSystemNameSize)
{
  PFILE_FS_VOLUME_INFORMATION FileFsVolume;
  PFILE_FS_ATTRIBUTE_INFORMATION FileFsAttribute;
  IO_STATUS_BLOCK IoStatusBlock;
  UCHAR Buffer[max(FS_VOLUME_BUFFER_SIZE, FS_ATTRIBUTE_BUFFER_SIZE)];

  NTSTATUS errCode;

  FileFsVolume = (PFILE_FS_VOLUME_INFORMATION)Buffer;
  FileFsAttribute = (PFILE_FS_ATTRIBUTE_INFORMATION)Buffer;

  TRACE("FileFsVolume %p\n", FileFsVolume);
  TRACE("FileFsAttribute %p\n", FileFsAttribute);

  if (hFile == INVALID_HANDLE_VALUE)
    {
      return FALSE;
    }

  TRACE("hFile: %p\n", hFile);
  errCode = NtQueryVolumeInformationFile(hFile,
                                         &IoStatusBlock,
                                         FileFsVolume,
                                         FS_VOLUME_BUFFER_SIZE,
                                         FileFsVolumeInformation);
  if ( !NT_SUCCESS(errCode) )
    {
      WARN("Status: %x\n", errCode);
      CloseHandle(hFile);
      BaseSetLastNTError (errCode);
      return FALSE;
    }

  if (lpVolumeSerialNumber)
    *lpVolumeSerialNumber = FileFsVolume->VolumeSerialNumber;

  if (lpVolumeNameBuffer)
    {
      if (nVolumeNameSize * sizeof(WCHAR) >= FileFsVolume->VolumeLabelLength + sizeof(WCHAR))
        {
	  memcpy(lpVolumeNameBuffer,
		 FileFsVolume->VolumeLabel,
		 FileFsVolume->VolumeLabelLength);
	  lpVolumeNameBuffer[FileFsVolume->VolumeLabelLength / sizeof(WCHAR)] = 0;
	}
      else
        {
	  CloseHandle(hFile);
	  SetLastError(ERROR_MORE_DATA);
	  return FALSE;
	}
    }

  errCode = NtQueryVolumeInformationFile (hFile,
	                                  &IoStatusBlock,
	                                  FileFsAttribute,
	                                  FS_ATTRIBUTE_BUFFER_SIZE,
	                                  FileFsAttributeInformation);
  CloseHandle(hFile);
  if (!NT_SUCCESS(errCode))
    {
      WARN("Status: %x\n", errCode);
      BaseSetLastNTError (errCode);
      return FALSE;
    }

  if (lpFileSystemFlags)
    *lpFileSystemFlags = FileFsAttribute->FileSystemAttributes;
  if (lpMaximumComponentLength)
    *lpMaximumComponentLength = FileFsAttribute->MaximumComponentNameLength;
  if (lpFileSystemNameBuffer)
    {
      if (nFileSystemNameSize * sizeof(WCHAR) >= FileFsAttribute->FileSystemNameLength + sizeof(WCHAR))
        {
	  memcpy(lpFileSystemNameBuffer,
		 FileFsAttribute->FileSystemName,
		 FileFsAttribute->FileSystemNameLength);
	  lpFileSystemNameBuffer[FileFsAttribute->FileSystemNameLength / sizeof(WCHAR)] = 0;
	}
      else
        {
	  SetLastError(ERROR_MORE_DATA);
	  return FALSE;
	}
    }
  return TRUE;
}

/*************************************************************************
 * StrCpyNW	[SHLWAPI.@]
 *
 * Copy a string to another string, up to a maximum number of characters.
 *
 * PARAMS
 *  dst    [O] Destination string
 *  src    [I] Source string
 *  count  [I] Maximum number of chars to copy
 *
 * RETURNS
 *  dst.
 */
LPWSTR WINAPI StrCpyNW(LPWSTR dst, LPCWSTR src, int count)
{
  LPWSTR d = dst;
  LPCWSTR s = src;

  TRACE("(%p,%s,%i)\n", dst, debugstr_w(src), count);

  if (s)
  {
    while ((count > 1) && *s)
    {
      count--;
      *d++ = *s++;
    }
  }
  if (count) *d = 0;

  return dst;
}

/*************************************************************************
 * StrCpyW	[SHLWAPI.@]
 *
 * Copy a string to another string.
 *
 * PARAMS
 *  lpszStr [O] Destination string
 *  lpszSrc [I] Source string
 *
 * RETURNS
 *  lpszStr.
 */
LPWSTR WINAPI StrCpyW(LPWSTR lpszStr, LPCWSTR lpszSrc)
{
  TRACE("(%p,%s)\n", lpszStr, debugstr_w(lpszSrc));

  if (lpszStr && lpszSrc)
    strcpyW(lpszStr, lpszSrc);
  return lpszStr;
}

/*************************************************************************
 * StrToInt64ExW	[SHLWAPI.@]
 *
 * See StrToIntExA.
 */
BOOL WINAPI StrToInt64ExW(LPCWSTR lpszStr, DWORD dwFlags, LONGLONG *lpiRet)
{
  BOOL bNegative = FALSE;
  LONGLONG iRet = 0;

  TRACE("(%s,%08X,%p)\n", debugstr_w(lpszStr), dwFlags, lpiRet);

  if (!lpszStr || !lpiRet)
  {
    WARN("Invalid parameter would crash under Win32!\n");
    return FALSE;
  }
  if (dwFlags > STIF_SUPPORT_HEX) WARN("Unknown flags %08x\n", dwFlags);

  /* Skip leading space, '+', '-' */
  while (isspaceW(*lpszStr)) lpszStr++;

  if (*lpszStr == '-')
  {
    bNegative = TRUE;
    lpszStr++;
  }
  else if (*lpszStr == '+')
    lpszStr++;

  if (dwFlags & STIF_SUPPORT_HEX &&
      *lpszStr == '0' && tolowerW(lpszStr[1]) == 'x')
  {
    /* Read hex number */
    lpszStr += 2;

    if (!isxdigitW(*lpszStr))
      return FALSE;

    while (isxdigitW(*lpszStr))
    {
      iRet = iRet * 16;
      if (isdigitW(*lpszStr))
        iRet += (*lpszStr - '0');
      else
        iRet += 10 + (tolowerW(*lpszStr) - 'a');
      lpszStr++;
    }
    *lpiRet = iRet;
    return TRUE;
  }

  /* Read decimal number */
  if (!isdigitW(*lpszStr))
    return FALSE;

  while (isdigitW(*lpszStr))
  {
    iRet = iRet * 10;
    iRet += (*lpszStr - '0');
    lpszStr++;
  }
  *lpiRet = bNegative ? -iRet : iRet;
  return TRUE;
}

/*************************************************************************
 * StrToIntExW	[SHLWAPI.@]
 *
 * See StrToIntExA.
 */
BOOL WINAPI StrToIntExW(LPCWSTR lpszStr, DWORD dwFlags, LPINT lpiRet)
{
  LONGLONG li;
  BOOL bRes;

  TRACE("(%s,%08X,%p)\n", debugstr_w(lpszStr), dwFlags, lpiRet);

  bRes = StrToInt64ExW(lpszStr, dwFlags, &li);
  if (bRes) *lpiRet = li;
  return bRes;
}

/*************************************************************************
 * StrDupW	[SHLWAPI.@]
 *
 * See StrDupA.
 */
LPWSTR WINAPI StrDupW(LPCWSTR lpszStr)
{
  int iLen;
  LPWSTR lpszRet;

  TRACE("(%s)\n",debugstr_w(lpszStr));

  iLen = (lpszStr ? strlenW(lpszStr) + 1 : 1) * sizeof(WCHAR);
  lpszRet = LocalAlloc(LMEM_FIXED, iLen);

  if (lpszRet)
  {
    if (lpszStr)
      memcpy(lpszRet, lpszStr, iLen);
    else
      *lpszRet = '\0';
  }
  return lpszRet;
}