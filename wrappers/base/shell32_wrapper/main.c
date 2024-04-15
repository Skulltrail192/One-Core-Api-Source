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

#include "main.h"
 
WINE_DEFAULT_DEBUG_CHANNEL(shell);

HINSTANCE shell32_hInstance = 0;	

BOOL WINAPI DllMain(HINSTANCE hInstDLL, DWORD fdwReason, LPVOID lpv)
{
	DWORD bufferSize = 65535;
	LPWSTR AppData;
	
    TRACE("fdwReason %u\n", fdwReason);

    switch(fdwReason)
    {
        case DLL_PROCESS_ATTACH:
			shell32_hInstance = hInstDLL;
			DisableThreadLibraryCalls(shell32_hInstance);		
			AppData = (LPWSTR)HeapAlloc(GetProcessHeap(), 8, MAX_PATH * 2);
			if (!AppData)
				return E_OUTOFMEMORY;
			
			if(GetEnvironmentVariableW(L"APPDATA", AppData, bufferSize) > 0 && GetEnvironmentVariableW(L"LOCALAPPDATA", AppData, bufferSize) == 0){
				SetEnvironmentVariableW(L"LOCALAPPDATA", AppData);
			}
			//Hack to disable sandbox for Firefox 73+
			if(GetEnvironmentVariableW(L"MOZ_DISABLE_CONTENT_SANDBOX", AppData, bufferSize) == 0){
				SetEnvironmentVariableW(L"MOZ_DISABLE_CONTENT_SANDBOX", L"1");
			}
			
			HeapFree(GetProcessHeap(), 0, AppData);
            break;
    }

    return TRUE;
}

/************************************************************************/

BOOL WINAPI StrRetToStrNW(LPWSTR dest, DWORD len, LPSTRRET src, const ITEMIDLIST *pidl)
{

    if (!dest)
        return FALSE;

    switch (src->uType)
    {
        case STRRET_WSTR:
            lstrcpynW(dest, src->pOleStr, len);
            CoTaskMemFree(src->pOleStr);
            break;
        case STRRET_CSTR:
            if (!MultiByteToWideChar(CP_ACP, 0, src->cStr, -1, dest, len) && len)
                dest[len-1] = 0;
            break;
        case STRRET_OFFSET:
            if (!MultiByteToWideChar(CP_ACP, 0, ((LPCSTR)&pidl->mkid)+src->uOffset, -1, dest, len)
                    && len)
                dest[len-1] = 0;
            break;
        default:
            FIXME("unknown type %u!\n", src->uType);
            if (len)
                *dest = '\0';
            return FALSE;
    }
    return TRUE;
}

WNDPROC lpPrevWndFunc;

LRESULT 
WINAPI 
NotificationWindowCallback(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
  LRESULT result; // eax@2
  unsigned __int16 cursorPosition; // [sp+0h] [bp-Ch]@7 MAPDST

  if ( Msg == 1024 )
  {
    if ( lParam == 516 )
    {
      GetCursorPos((LPPOINT)&cursorPosition);
      result = CallWindowProcW(lpPrevWndFunc, hWnd, 0x400u, cursorPosition | (cursorPosition << 16), 123);
    }
    else if ( lParam == 517 )
    {
      result = 0;
    }
    else
    {
      result = CallWindowProcW(lpPrevWndFunc, hWnd, 0x400u, wParam, lParam);
    }
  }
  else
  {
    result = DefWindowProcW(hWnd, Msg, wParam, lParam);
  }
  return result;
}

HWND globalWindow;
HWND hWnd;

signed int  Internal_Shell_NotifyIcon(PNOTIFYICONDATA lpdata, DWORD dwMessage, __int16 flags)
{
  void *allocation; // edi@1
  HWND findWindow; // eax@7
  HWND otherWindow; // ST28_4@14
  DWORD localMessage; // [sp-8h] [bp-38h]@5
  struct _NOTIFYICONDATAA *lpDataPointer; // [sp-4h] [bp-34h]@3
  WNDCLASSW WndClass; // [sp+8h] [bp-28h]@14

  allocation = malloc(0x10u);
  wsprintfW((LPWSTR)allocation, L"%x", lpdata->hWnd);
  if ( dwMessage )
  {
    if ( dwMessage != 2 )
    {
      free(allocation);
      lpdata->hWnd = globalWindow;
      lpDataPointer = (struct _NOTIFYICONDATAA *)lpdata;
      if ( flags != 1 )
        return Shell_NotifyIconW(dwMessage, (PNOTIFYICONDATAW)lpdata);
      localMessage = dwMessage;
      return Shell_NotifyIconA(localMessage, lpDataPointer);
    }
    findWindow = FindWindowW((LPCWSTR)allocation, 0);
    globalWindow = findWindow;
    if ( findWindow )
    {
      DestroyWindow(findWindow);
      findWindow = globalWindow;
    }
    lpdata->hWnd = findWindow;
    free(allocation);
    lpDataPointer = (struct _NOTIFYICONDATAA *)lpdata;
    localMessage = 2;
  }
  else
  {
    if ( hWnd || lpPrevWndFunc )
    {
      free(allocation);
      return 183;
    }
    otherWindow = lpdata->hWnd;
    WndClass.hInstance = (HINSTANCE)268435456;
    WndClass.lpszClassName = (LPCWSTR)allocation;
    WndClass.lpfnWndProc = NotificationWindowCallback;
    WndClass.style = 0;
    WndClass.hIcon = 0;
    WndClass.hCursor = 0;
    WndClass.lpszMenuName = 0;
    WndClass.hbrBackground = 0;
    WndClass.cbClsExtra = 0;
    WndClass.cbWndExtra = 0;
    lpPrevWndFunc = (WNDPROC)GetWindowLongW(otherWindow, -4);
    hWnd = lpdata->hWnd;
    RegisterClassW(&WndClass);
    globalWindow = CreateWindowExW(0, (LPCWSTR)allocation, 0, 0, 0, 0, 0, 0, HWND_MESSAGE, 0, (HINSTANCE)0x10000000, 0);
    lpdata->hWnd = globalWindow;
    free(allocation);
    lpDataPointer = (struct _NOTIFYICONDATAA *)lpdata;
    localMessage = 0;
  }
  if ( flags == 1 )
    return Shell_NotifyIconA(localMessage, lpDataPointer);
  return Shell_NotifyIconW(localMessage, (PNOTIFYICONDATAW)lpDataPointer);
}

BOOL __stdcall Shell_NotifyIconInternal(DWORD dwMessage, PNOTIFYICONDATA lpdata)
{
  return Internal_Shell_NotifyIcon(lpdata, dwMessage, 0);
}

BOOL __stdcall Shell_NotifyIconInternalA(DWORD dwMessage, PNOTIFYICONDATAA lpData)
{
  return Internal_Shell_NotifyIcon((PNOTIFYICONDATA)lpData, dwMessage, 1);
}

BOOL __stdcall Shell_NotifyIconInternalW(DWORD dwMessage, PNOTIFYICONDATAW lpData)
{
  return Internal_Shell_NotifyIcon((PNOTIFYICONDATA)lpData, dwMessage, 2);
}

/*************************************************************************
 * CommandLineToArgvW            [SHCORE.@]
 *
 * We must interpret the quotes in the command line to rebuild the argv
 * array correctly:
 * - arguments are separated by spaces or tabs
 * - quotes serve as optional argument delimiters
 *   '"a b"'   -> 'a b'
 * - escaped quotes must be converted back to '"'
 *   '\"'      -> '"'
 * - consecutive backslashes preceding a quote see their number halved with
 *   the remainder escaping the quote:
 *   2n   backslashes + quote -> n backslashes + quote as an argument delimiter
 *   2n+1 backslashes + quote -> n backslashes + literal quote
 * - backslashes that are not followed by a quote are copied literally:
 *   'a\b'     -> 'a\b'
 *   'a\\b'    -> 'a\\b'
 * - in quoted strings, consecutive quotes see their number divided by three
 *   with the remainder modulo 3 deciding whether to close the string or not.
 *   Note that the opening quote must be counted in the consecutive quotes,
 *   that's the (1+) below:
 *   (1+) 3n   quotes -> n quotes
 *   (1+) 3n+1 quotes -> n quotes plus closes the quoted string
 *   (1+) 3n+2 quotes -> n+1 quotes plus closes the quoted string
 * - in unquoted strings, the first quote opens the quoted string and the
 *   remaining consecutive quotes follow the above rule.
 */
WCHAR** WINAPI CommandLineToArgvW(const WCHAR *cmdline, int *numargs)
{
    int qcount, bcount;
    const WCHAR *s;
    WCHAR **argv;
    DWORD argc;
    WCHAR *d;

    if (!numargs)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return NULL;
    }

    if (*cmdline == 0)
    {
        /* Return the path to the executable */
        DWORD len, deslen = MAX_PATH, size;

        size = sizeof(WCHAR *) * 2 + deslen * sizeof(WCHAR);
        for (;;)
        {
            if (!(argv = LocalAlloc(LMEM_FIXED, size))) return NULL;
            len = GetModuleFileNameW(0, (WCHAR *)(argv + 2), deslen);
            if (!len)
            {
                LocalFree(argv);
                return NULL;
            }
            if (len < deslen) break;
            deslen *= 2;
            size = sizeof(WCHAR *) * 2 + deslen * sizeof(WCHAR);
            LocalFree(argv);
        }
        argv[0] = (WCHAR *)(argv + 2);
        argv[1] = NULL;
        *numargs = 1;

        return argv;
    }

    /* --- First count the arguments */
    argc = 1;
    s = cmdline;
    /* The first argument, the executable path, follows special rules */
    if (*s == '"')
    {
        /* The executable path ends at the next quote, no matter what */
        s++;
        while (*s)
            if (*s++ == '"')
                break;
    }
    else
    {
        /* The executable path ends at the next space, no matter what */
        while (*s && *s != ' ' && *s != '\t')
            s++;
    }
    /* skip to the first argument, if any */
    while (*s == ' ' || *s == '\t')
        s++;
    if (*s)
        argc++;

    /* Analyze the remaining arguments */
    qcount = bcount = 0;
    while (*s)
    {
        if ((*s == ' ' || *s == '\t') && qcount == 0)
        {
            /* skip to the next argument and count it if any */
            while (*s == ' ' || *s == '\t')
                s++;
            if (*s)
                argc++;
            bcount = 0;
        }
        else if (*s == '\\')
        {
            /* '\', count them */
            bcount++;
            s++;
        }
        else if (*s == '"')
        {
            /* '"' */
            if ((bcount & 1) == 0)
                qcount++; /* unescaped '"' */
            s++;
            bcount = 0;
            /* consecutive quotes, see comment in copying code below */
            while (*s == '"')
            {
                qcount++;
                s++;
            }
            qcount = qcount % 3;
            if (qcount == 2)
                qcount = 0;
        }
        else
        {
            /* a regular character */
            bcount = 0;
            s++;
        }
    }

    /* Allocate in a single lump, the string array, and the strings that go
     * with it. This way the caller can make a single LocalFree() call to free
     * both, as per MSDN.
     */
    argv = LocalAlloc(LMEM_FIXED, (argc + 1) * sizeof(WCHAR *) + (lstrlenW(cmdline) + 1) * sizeof(WCHAR));
    if (!argv)
        return NULL;

    /* --- Then split and copy the arguments */
    argv[0] = d = lstrcpyW((WCHAR *)(argv + argc + 1), cmdline);
    argc = 1;
    /* The first argument, the executable path, follows special rules */
    if (*d == '"')
    {
        /* The executable path ends at the next quote, no matter what */
        s = d + 1;
        while (*s)
        {
            if (*s == '"')
            {
                s++;
                break;
            }
            *d++ = *s++;
        }
    }
    else
    {
        /* The executable path ends at the next space, no matter what */
        while (*d && *d != ' ' && *d != '\t')
            d++;
        s = d;
        if (*s)
            s++;
    }
    /* close the executable path */
    *d++ = 0;
    /* skip to the first argument and initialize it if any */
    while (*s == ' ' || *s == '\t')
        s++;
    if (!*s)
    {
        /* There are no parameters so we are all done */
        argv[argc] = NULL;
        *numargs = argc;
        return argv;
    }

    /* Split and copy the remaining arguments */
    argv[argc++] = d;
    qcount = bcount = 0;
    while (*s)
    {
        if ((*s == ' ' || *s == '\t') && qcount == 0)
        {
            /* close the argument */
            *d++ = 0;
            bcount = 0;

            /* skip to the next one and initialize it if any */
            do {
                s++;
            } while (*s == ' ' || *s == '\t');
            if (*s)
                argv[argc++] = d;
        }
        else if (*s=='\\')
        {
            *d++ = *s++;
            bcount++;
        }
        else if (*s == '"')
        {
            if ((bcount & 1) == 0)
            {
                /* Preceded by an even number of '\', this is half that
                 * number of '\', plus a quote which we erase.
                 */
                d -= bcount / 2;
                qcount++;
            }
            else
            {
                /* Preceded by an odd number of '\', this is half that
                 * number of '\' followed by a '"'
                 */
                d = d - bcount / 2 - 1;
                *d++ = '"';
            }
            s++;
            bcount = 0;
            /* Now count the number of consecutive quotes. Note that qcount
             * already takes into account the opening quote if any, as well as
             * the quote that lead us here.
             */
            while (*s == '"')
            {
                if (++qcount == 3)
                {
                    *d++ = '"';
                    qcount = 0;
                }
                s++;
            }
            if (qcount == 2)
                qcount = 0;
        }
        else
        {
            /* a regular character */
            *d++ = *s++;
            bcount = 0;
        }
    }
    *d = '\0';
    argv[argc] = NULL;
    *numargs = argc;

    return argv;
}
