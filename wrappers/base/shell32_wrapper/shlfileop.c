/*
 * SHFileOperation
 *
 * Copyright 2000 Juergen Schmied
 * Copyright 2002 Andriy Palamarchuk
 * Copyright 2004 Dietrich Teickner (from Odin)
 * Copyright 2004 Rolf Kalbermatter
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
 */

#define COBJMACROS

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "shellapi.h"
#include "wingdi.h"
#include "winuser.h"
#include "shlobj.h"
#include "shresdef.h"
#define NO_SHLWAPI_STREAM
#include "shlwapi.h"
#include "shell32_main.h"
#include "main.h"
#include "shfldr.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(shell);

#define IsAttrib(x, y)  ((INVALID_FILE_ATTRIBUTES != (x)) && ((x) & (y)))
#define IsAttribFile(x) (!((x) & FILE_ATTRIBUTE_DIRECTORY))
#define IsAttribDir(x)  IsAttrib(x, FILE_ATTRIBUTE_DIRECTORY)
#define IsDotDir(x)     ((x[0] == '.') && ((x[1] == 0) || ((x[1] == '.') && (x[2] == 0))))

#define FO_MASK         0xF

#define DE_SAMEFILE      0x71
#define DE_DESTSAMETREE  0x7D

#define FO_NEW_ITEM 5

static DWORD SHNotifyCreateDirectoryA(LPCSTR path, LPSECURITY_ATTRIBUTES sec);
static DWORD SHNotifyCreateDirectoryW(LPCWSTR path, LPSECURITY_ATTRIBUTES sec);
static DWORD SHNotifyRemoveDirectoryA(LPCSTR path);
static DWORD SHNotifyRemoveDirectoryW(LPCWSTR path);
static DWORD SHNotifyDeleteFileA(LPCSTR path);
static DWORD SHNotifyDeleteFileW(LPCWSTR path);
static DWORD SHNotifyMoveFileW(LPCWSTR src, LPCWSTR dest);
static DWORD SHNotifyCopyFileW(LPCWSTR src, LPCWSTR dest, BOOL bFailIfExists);
static DWORD SHFindAttrW(LPCWSTR pName, BOOL fileOnly);

typedef struct
{
    SHFILEOPSTRUCTW *req;
    DWORD dwYesToAllMask;
    BOOL bManyItems;
    BOOL bCancelled;
} FILE_OPERATION;

/* Confirm dialogs with an optional "Yes To All" as used in file operations confirmations
 */
struct confirm_msg_info
{
    LPWSTR lpszText;
    LPWSTR lpszCaption;
    HICON hIcon;
    BOOL bYesToAll;
};

/* as some buttons may be hidden and the dialog height may change we may need
 * to move the controls */
static void confirm_msg_move_button(HWND hDlg, INT iId, INT *xPos, INT yOffset, BOOL bShow)
{
    HWND hButton = GetDlgItem(hDlg, iId);
    RECT r;

    if (bShow) {
        int width;

        GetWindowRect(hButton, &r);
        MapWindowPoints( 0, hDlg, (POINT *)&r, 2 );
        width = r.right - r.left;
        SetWindowPos(hButton, 0, *xPos - width, r.top - yOffset, 0, 0,
                     SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOREDRAW );
        *xPos -= width + 5;
    }
    else
        ShowWindow(hButton, SW_HIDE);
}

/* Note: we paint the text manually and don't use the static control to make
 * sure the text has the same height as the one computed in WM_INITDIALOG
 */
static INT_PTR ConfirmMsgBox_Paint(HWND hDlg)
{
    PAINTSTRUCT ps;
    HFONT hOldFont;
    RECT r;
    HDC hdc;

    BeginPaint(hDlg, &ps);
    hdc = ps.hdc;
    SetBkMode(hdc, TRANSPARENT);

    GetClientRect(GetDlgItem(hDlg, IDD_MESSAGE), &r);
    /* this will remap the rect to dialog coords */
    MapWindowPoints(GetDlgItem(hDlg, IDD_MESSAGE), hDlg, (LPPOINT)&r, 2);
    hOldFont = SelectObject(hdc, (HFONT)SendDlgItemMessageW(hDlg, IDD_MESSAGE, WM_GETFONT, 0, 0));
    DrawTextW(hdc, GetPropW(hDlg, L"WINE_CONFIRM"), -1, &r, DT_NOPREFIX | DT_PATH_ELLIPSIS | DT_WORDBREAK);
    SelectObject(hdc, hOldFont);
    EndPaint(hDlg, &ps);
    return TRUE;
}

static INT_PTR ConfirmMsgBox_Init(HWND hDlg, LPARAM lParam)
{
    struct confirm_msg_info *info = (struct confirm_msg_info *)lParam;
    INT xPos, yOffset;
    int width, height;
    HFONT hOldFont;
    HDC hdc;
    RECT r;

    SetWindowTextW(hDlg, info->lpszCaption);
    ShowWindow(GetDlgItem(hDlg, IDD_MESSAGE), SW_HIDE);
    SetPropW(hDlg, L"WINE_CONFIRM", info->lpszText);
    SendDlgItemMessageW(hDlg, IDD_ICON, STM_SETICON, (WPARAM)info->hIcon, 0);

    /* compute the text height and resize the dialog */
    GetClientRect(GetDlgItem(hDlg, IDD_MESSAGE), &r);
    hdc = GetDC(hDlg);
    yOffset = r.bottom;
    hOldFont = SelectObject(hdc, (HFONT)SendDlgItemMessageW(hDlg, IDD_MESSAGE, WM_GETFONT, 0, 0));
    DrawTextW(hdc, info->lpszText, -1, &r, DT_NOPREFIX | DT_PATH_ELLIPSIS | DT_WORDBREAK | DT_CALCRECT);
    SelectObject(hdc, hOldFont);
    yOffset -= r.bottom;
    yOffset = min(yOffset, 35);  /* don't make the dialog too small */
    ReleaseDC(hDlg, hdc);

    GetClientRect(hDlg, &r);
    xPos = r.right - 7;
    GetWindowRect(hDlg, &r);
    width = r.right - r.left;
    height = r.bottom - r.top - yOffset;
    MoveWindow(hDlg, (GetSystemMetrics(SM_CXSCREEN) - width)/2,
        (GetSystemMetrics(SM_CYSCREEN) - height)/2, width, height, FALSE);

    confirm_msg_move_button(hDlg, IDCANCEL,     &xPos, yOffset, info->bYesToAll);
    confirm_msg_move_button(hDlg, IDNO,         &xPos, yOffset, TRUE);
    confirm_msg_move_button(hDlg, IDD_YESTOALL, &xPos, yOffset, info->bYesToAll);
    confirm_msg_move_button(hDlg, IDYES,        &xPos, yOffset, TRUE);
    return TRUE;
}

static INT_PTR CALLBACK ConfirmMsgBoxProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
            return ConfirmMsgBox_Init(hDlg, lParam);
        case WM_PAINT:
            return ConfirmMsgBox_Paint(hDlg);
        case WM_COMMAND:
            EndDialog(hDlg, wParam);
            break;
        case WM_CLOSE:
            EndDialog(hDlg, IDCANCEL);
            break;
    }
    return FALSE;
}

static int SHELL_ConfirmMsgBox(HWND hWnd, LPWSTR lpszText, LPWSTR lpszCaption, HICON hIcon, BOOL bYesToAll)
{
    struct confirm_msg_info info;

    info.lpszText = lpszText;
    info.lpszCaption = lpszCaption;
    info.hIcon = hIcon;
    info.bYesToAll = bYesToAll;
    return DialogBoxParamW(shell32_hInstance, L"SHELL_YESTOALL_MSGBOX", hWnd, ConfirmMsgBoxProc, (LPARAM)&info);
}

/* confirmation dialogs content */
typedef struct
{
        HINSTANCE hIconInstance;
        UINT icon_resource_id;
	UINT caption_resource_id, text_resource_id;
} SHELL_ConfirmIDstruc;

static BOOL SHELL_ConfirmIDs(int nKindOfDialog, SHELL_ConfirmIDstruc *ids)
{
        ids->hIconInstance = shell32_hInstance;
	switch (nKindOfDialog) {
	  case ASK_DELETE_FILE:
            ids->icon_resource_id = IDI_SHELL_CONFIRM_DELETE;
	    ids->caption_resource_id  = IDS_DELETEITEM_CAPTION;
	    ids->text_resource_id  = IDS_DELETEITEM_TEXT;
	    return TRUE;
	  case ASK_DELETE_FOLDER:
            ids->icon_resource_id = IDI_SHELL_CONFIRM_DELETE;
	    ids->caption_resource_id  = IDS_DELETEFOLDER_CAPTION;
	    ids->text_resource_id  = IDS_DELETEITEM_TEXT;
	    return TRUE;
	  case ASK_DELETE_MULTIPLE_ITEM:
            ids->icon_resource_id = IDI_SHELL_CONFIRM_DELETE;
	    ids->caption_resource_id  = IDS_DELETEITEM_CAPTION;
	    ids->text_resource_id  = IDS_DELETEMULTIPLE_TEXT;
	    return TRUE;
          case ASK_TRASH_FILE:
            ids->icon_resource_id = IDI_SHELL_TRASH_FILE;
            ids->caption_resource_id = IDS_DELETEITEM_CAPTION;
            ids->text_resource_id = IDS_TRASHITEM_TEXT;
            return TRUE;
          case ASK_TRASH_FOLDER:
            ids->icon_resource_id = IDI_SHELL_TRASH_FILE;
            ids->caption_resource_id = IDS_DELETEFOLDER_CAPTION;
            ids->text_resource_id = IDS_TRASHFOLDER_TEXT;
            return TRUE;
          case ASK_TRASH_MULTIPLE_ITEM:
            ids->icon_resource_id = IDI_SHELL_TRASH_FILE;
            ids->caption_resource_id = IDS_DELETEITEM_CAPTION;
            ids->text_resource_id = IDS_TRASHMULTIPLE_TEXT;
            return TRUE;
          case ASK_CANT_TRASH_ITEM:
            ids->icon_resource_id = IDI_SHELL_CONFIRM_DELETE;
            ids->caption_resource_id  = IDS_DELETEITEM_CAPTION;
            ids->text_resource_id  = IDS_CANTTRASH_TEXT;
            return TRUE;
	  case ASK_DELETE_SELECTED:
            ids->icon_resource_id = IDI_SHELL_CONFIRM_DELETE;
            ids->caption_resource_id  = IDS_DELETEITEM_CAPTION;
            ids->text_resource_id  = IDS_DELETESELECTED_TEXT;
            return TRUE;
	  case ASK_OVERWRITE_FILE:
            ids->hIconInstance = NULL;
            ids->icon_resource_id = (UINT)IDI_WARNING;
	    ids->caption_resource_id  = IDS_OVERWRITEFILE_CAPTION;
	    ids->text_resource_id  = IDS_OVERWRITEFILE_TEXT;
            return TRUE;
	  case ASK_OVERWRITE_FOLDER:
            ids->hIconInstance = NULL;
            ids->icon_resource_id = (UINT)IDI_WARNING;
            ids->caption_resource_id  = IDS_OVERWRITEFILE_CAPTION;
            ids->text_resource_id  = IDS_OVERWRITEFOLDER_TEXT;
            return TRUE;
	  default:
	    FIXME(" Unhandled nKindOfDialog %d stub\n", nKindOfDialog);
	}
	return FALSE;
}

static BOOL SHELL_ConfirmDialogW(HWND hWnd, int nKindOfDialog, LPCWSTR szDir, FILE_OPERATION *op)
{
	WCHAR szCaption[255], szText[255], szBuffer[MAX_PATH + 256];
	SHELL_ConfirmIDstruc ids;
	DWORD_PTR args[1];
	HICON hIcon;
	int ret;

        assert(nKindOfDialog >= 0 && nKindOfDialog < 32);
        if (op && (op->dwYesToAllMask & (1 << nKindOfDialog)))
            return TRUE;

        if (!SHELL_ConfirmIDs(nKindOfDialog, &ids)) return FALSE;

        LoadStringW(shell32_hInstance, ids.caption_resource_id, szCaption, ARRAY_SIZE(szCaption));
        LoadStringW(shell32_hInstance, ids.text_resource_id, szText, ARRAY_SIZE(szText));

        args[0] = (DWORD_PTR)szDir;
        FormatMessageW(FORMAT_MESSAGE_FROM_STRING|FORMAT_MESSAGE_ARGUMENT_ARRAY,
            szText, 0, 0, szBuffer, ARRAY_SIZE(szBuffer), (va_list*)args);

        hIcon = LoadIconW(ids.hIconInstance, (LPWSTR)MAKEINTRESOURCE(ids.icon_resource_id));

        ret = SHELL_ConfirmMsgBox(hWnd, szBuffer, szCaption, hIcon, op && op->bManyItems);
        if (op) {
            if (ret == IDD_YESTOALL) {
                op->dwYesToAllMask |= (1 << nKindOfDialog);
                ret = IDYES;
            }
            if (ret == IDCANCEL)
                op->bCancelled = TRUE;
            if (ret != IDYES)
                op->req->fAnyOperationsAborted = TRUE;
        }
        return ret == IDYES;
}

BOOL SHELL_ConfirmYesNoW(HWND hWnd, int nKindOfDialog, LPCWSTR szDir)
{
    return SHELL_ConfirmDialogW(hWnd, nKindOfDialog, szDir, NULL);
}

static DWORD SHELL32_AnsiToUnicodeBuf(LPCSTR aPath, LPWSTR *wPath, DWORD minChars)
{
	DWORD len = MultiByteToWideChar(CP_ACP, 0, aPath, -1, NULL, 0);

	if (len < minChars)
	  len = minChars;

	*wPath = malloc(len * sizeof(WCHAR));
	if (*wPath)
	{
	  MultiByteToWideChar(CP_ACP, 0, aPath, -1, *wPath, len);
	  return NO_ERROR;
	}
	return E_OUTOFMEMORY;
}

HRESULT WINAPI SHIsFileAvailableOffline(LPCWSTR path, LPDWORD status)
{
    FIXME("(%s, %p) stub\n", debugstr_w(path), status);
    return E_FAIL;
}

/**************************************************************************
 * SHELL_DeleteDirectory()  [internal]
 *
 * Asks for confirmation when bShowUI is true and deletes the directory and
 * all its subdirectories and files if necessary.
 */
static DWORD SHELL_DeleteDirectoryW(HWND hwnd, LPCWSTR pszDir, BOOL bShowUI)
{
    DWORD    ret = 0;
    HANDLE  hFind;
    WIN32_FIND_DATAW wfd;
    WCHAR   szTemp[MAX_PATH];

    PathCombineW(szTemp, pszDir, L"*");
    hFind = FindFirstFileW(szTemp, &wfd);

    if (hFind != INVALID_HANDLE_VALUE) {
        if (!bShowUI || SHELL_ConfirmDialogW(hwnd, ASK_DELETE_FOLDER, pszDir, NULL)) {
            do {
                if (IsDotDir(wfd.cFileName))
                    continue;
                PathCombineW(szTemp, pszDir, wfd.cFileName);
                if (FILE_ATTRIBUTE_DIRECTORY & wfd.dwFileAttributes)
                    ret = SHELL_DeleteDirectoryW(hwnd, szTemp, FALSE);
                else
                    ret = SHNotifyDeleteFileW(szTemp);
            } while (!ret && FindNextFileW(hFind, &wfd));
        }
        FindClose(hFind);
    }
    if (ret == ERROR_SUCCESS)
        ret = SHNotifyRemoveDirectoryW(pszDir);

    return ret == ERROR_PATH_NOT_FOUND ?
        0x7C: /* DE_INVALIDFILES (legacy Windows error) */
        ret;
}

/**********************************************************************/

static DWORD SHNotifyCreateDirectoryW(LPCWSTR path, LPSECURITY_ATTRIBUTES sec)
{
	TRACE("(%s, %p)\n", debugstr_w(path), sec);

	if (CreateDirectoryW(path, sec))
	{
	  SHChangeNotify(SHCNE_MKDIR, SHCNF_PATHW, path, NULL);
	  return ERROR_SUCCESS;
	}
	return GetLastError();
}

/***********************************************************************/

static DWORD SHNotifyRemoveDirectoryW(LPCWSTR path)
{
	BOOL ret;
	TRACE("(%s)\n", debugstr_w(path));

	ret = RemoveDirectoryW(path);
	if (!ret)
	{
	  /* Directory may be write protected */
	  DWORD dwAttr = GetFileAttributesW(path);
	  if (IsAttrib(dwAttr, FILE_ATTRIBUTE_READONLY))
	    if (SetFileAttributesW(path, dwAttr & ~FILE_ATTRIBUTE_READONLY))
	      ret = RemoveDirectoryW(path);
	}
	if (ret)
	{
	  SHChangeNotify(SHCNE_RMDIR, SHCNF_PATHW, path, NULL);
	  return ERROR_SUCCESS;
	}
	return GetLastError();
}

/***********************************************************************/

static DWORD SHNotifyDeleteFileW(LPCWSTR path)
{
	BOOL ret;

	TRACE("(%s)\n", debugstr_w(path));

	ret = DeleteFileW(path);
	if (!ret)
	{
	  /* File may be write protected or a system file */
	  DWORD dwAttr = GetFileAttributesW(path);
	  if (IsAttrib(dwAttr, FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_SYSTEM))
	    if (SetFileAttributesW(path, dwAttr & ~(FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_SYSTEM)))
	      ret = DeleteFileW(path);
	}
	if (ret)
	{
	  SHChangeNotify(SHCNE_DELETE, SHCNF_PATHW, path, NULL);
	  return ERROR_SUCCESS;
	}
	return GetLastError();
}

/************************************************************************
 * SHNotifyMoveFile          [internal]
 *
 * Moves a file. Also triggers a change notify if one exists.
 *
 * PARAMS
 *  src        [I]   path to source file to move
 *  dest       [I]   path to target file to move to
 *
 * RETURNS
 *  ERROR_SUCCESS if successful
 */
static DWORD SHNotifyMoveFileW(LPCWSTR src, LPCWSTR dest)
{
	BOOL ret;

	TRACE("(%s %s)\n", debugstr_w(src), debugstr_w(dest));

        ret = MoveFileExW(src, dest, MOVEFILE_REPLACE_EXISTING);

        /* MOVEFILE_REPLACE_EXISTING fails with dirs, so try MoveFile */
        if (!ret)
            ret = MoveFileW(src, dest);

	if (!ret)
	{
	  DWORD dwAttr;

	  dwAttr = SHFindAttrW(dest, FALSE);
	  if (INVALID_FILE_ATTRIBUTES == dwAttr)
	  {
	    /* Source file may be write protected or a system file */
	    dwAttr = GetFileAttributesW(src);
	    if (IsAttrib(dwAttr, FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_SYSTEM))
	      if (SetFileAttributesW(src, dwAttr & ~(FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_SYSTEM)))
	        ret = MoveFileW(src, dest);
	  }
	}
	if (ret)
	{
	  SHChangeNotify(SHCNE_RENAMEITEM, SHCNF_PATHW, src, dest);
	  return ERROR_SUCCESS;
	}
	return GetLastError();
}

/************************************************************************
 * SHNotifyCopyFile          [internal]
 *
 * Copies a file. Also triggers a change notify if one exists.
 *
 * PARAMS
 *  src           [I]   path to source file to move
 *  dest          [I]   path to target file to move to
 *  bFailIfExists [I]   if TRUE, the target file will not be overwritten if
 *                      a file with this name already exists
 *
 * RETURNS
 *  ERROR_SUCCESS if successful
 */
static DWORD SHNotifyCopyFileW(LPCWSTR src, LPCWSTR dest, BOOL bFailIfExists)
{
	BOOL ret;
	DWORD attribs;

	TRACE("(%s %s %s)\n", debugstr_w(src), debugstr_w(dest), bFailIfExists ? "failIfExists" : "");

        /* Destination file may already exist with read only attribute */
        attribs = GetFileAttributesW(dest);
        if (IsAttrib(attribs, FILE_ATTRIBUTE_READONLY))
          SetFileAttributesW(dest, attribs & ~FILE_ATTRIBUTE_READONLY);

	ret = CopyFileW(src, dest, bFailIfExists);
	if (ret)
	{
	  SHChangeNotify(SHCNE_CREATE, SHCNF_PATHW, dest, NULL);
	  return ERROR_SUCCESS;
	}

	return GetLastError();
}


/*************************************************************************
 * SHFindAttrW      [internal]
 *
 * Get the Attributes for a file or directory. The difference to GetAttributes()
 * is that this function will also work for paths containing wildcard characters
 * in its filename.

 * PARAMS
 *  path       [I]   path of directory or file to check
 *  fileOnly   [I]   TRUE if only files should be found
 *
 * RETURNS
 *  INVALID_FILE_ATTRIBUTES if the path does not exist, the actual attributes of
 *  the first file or directory found otherwise
 */
static DWORD SHFindAttrW(LPCWSTR pName, BOOL fileOnly)
{
	WIN32_FIND_DATAW wfd;
	BOOL b_FileMask = fileOnly && (NULL != StrPBrkW(pName, L"*?"));
	DWORD dwAttr = INVALID_FILE_ATTRIBUTES;
	HANDLE hFind = FindFirstFileW(pName, &wfd);

	TRACE("%s %d\n", debugstr_w(pName), fileOnly);
	if (INVALID_HANDLE_VALUE != hFind)
	{
	  do
	  {
	    if (b_FileMask && IsAttribDir(wfd.dwFileAttributes))
	       continue;
	    dwAttr = wfd.dwFileAttributes;
	    break;
	  }
	  while (FindNextFileW(hFind, &wfd));
	  FindClose(hFind);
	}
	return dwAttr;
}

/*************************************************************************
 *
 * SHNameTranslate HelperFunction for SHFileOperationA
 *
 * Translates a list of 0 terminated ANSI strings into Unicode. If *wString
 * is NULL, only the necessary size of the string is determined and returned,
 * otherwise the ANSI strings are copied into it and the buffer is increased
 * to point to the location after the final 0 termination char.
 */
static DWORD SHNameTranslate(LPWSTR* wString, LPCWSTR* pWToFrom, BOOL more)
{
	DWORD size = 0, aSize = 0;
	LPCSTR aString = (LPCSTR)*pWToFrom;

	if (aString)
	{
	  do
	  {
	    size = lstrlenA(aString) + 1;
	    aSize += size;
	    aString += size;
	  } while ((size != 1) && more);
	  /* The two sizes might be different in the case of multibyte chars */
	  size = MultiByteToWideChar(CP_ACP, 0, (LPCSTR)*pWToFrom, aSize, *wString, 0);
	  if (*wString) /* only in the second loop */
	  {
	    MultiByteToWideChar(CP_ACP, 0, (LPCSTR)*pWToFrom, aSize, *wString, size);
	    *pWToFrom = *wString;
	    *wString += size;
	  }
	}
	return size;
}

#define ERROR_SHELL_INTERNAL_FILE_NOT_FOUND 1026

typedef struct
{
    DWORD attributes;
    LPWSTR szDirectory;
    LPWSTR szFilename;
    LPWSTR szFullPath;
    BOOL bFromWildcard;
    BOOL bFromRelative;
    BOOL bExists;
} FILE_ENTRY;

typedef struct
{
    FILE_ENTRY *feFiles;
    DWORD num_alloc;
    DWORD dwNumFiles;
    BOOL bAnyFromWildcard;
    BOOL bAnyDirectories;
    BOOL bAnyDontExist;
} FILE_LIST;


static inline void grow_list(FILE_LIST *list)
{
    FILE_ENTRY *new = (FILE_ENTRY *)HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, list->feFiles,
                                  list->num_alloc * 2 * sizeof(*new) );
    list->feFiles = new;
    list->num_alloc *= 2;
}

/* adds a file to the FILE_ENTRY struct
 */
static void add_file_to_entry(FILE_ENTRY *feFile, LPCWSTR szFile)
{
    DWORD dwLen = lstrlenW(szFile) + 1;
    LPCWSTR ptr;

    feFile->szFullPath = malloc(dwLen * sizeof(WCHAR));
    lstrcpyW(feFile->szFullPath, szFile);

    ptr = StrRChrW(szFile, NULL, '\\');
    if (ptr)
    {
        dwLen = ptr - szFile + 1;
        feFile->szDirectory = malloc(dwLen * sizeof(WCHAR));
        lstrcpynW(feFile->szDirectory, szFile, dwLen);

        dwLen = lstrlenW(feFile->szFullPath) - dwLen + 1;
        feFile->szFilename = malloc(dwLen * sizeof(WCHAR));
        lstrcpyW(feFile->szFilename, ptr + 1); /* skip over backslash */
    }
    feFile->bFromWildcard = FALSE;
}

static LPWSTR wildcard_to_file(LPCWSTR szWildCard, LPCWSTR szFileName)
{
    LPCWSTR ptr;
    LPWSTR szFullPath;
    DWORD dwDirLen, dwFullLen;

    ptr = StrRChrW(szWildCard, NULL, '\\');
    dwDirLen = ptr - szWildCard + 1;

    dwFullLen = dwDirLen + lstrlenW(szFileName) + 1;
    szFullPath = malloc(dwFullLen * sizeof(WCHAR));

    lstrcpynW(szFullPath, szWildCard, dwDirLen + 1);
    lstrcatW(szFullPath, szFileName);

    return szFullPath;
}

static void parse_wildcard_files(FILE_LIST *flList, LPCWSTR szFile, LPDWORD pdwListIndex)
{
    WIN32_FIND_DATAW wfd;
    HANDLE hFile = FindFirstFileW(szFile, &wfd);
    FILE_ENTRY *file;
    LPWSTR szFullPath;
    BOOL res;

    if (hFile == INVALID_HANDLE_VALUE) return;

    for (res = TRUE; res; res = FindNextFileW(hFile, &wfd))
    {
        if (IsDotDir(wfd.cFileName)) continue;
        if (*pdwListIndex >= flList->num_alloc) grow_list( flList );
        szFullPath = wildcard_to_file(szFile, wfd.cFileName);
        file = &flList->feFiles[(*pdwListIndex)++];
        add_file_to_entry(file, szFullPath);
        file->bFromWildcard = TRUE;
        file->attributes = wfd.dwFileAttributes;
        if (IsAttribDir(file->attributes)) flList->bAnyDirectories = TRUE;
        free(szFullPath);
    }

    FindClose(hFile);
}

/* takes the null-separated file list and fills out the FILE_LIST */
static HRESULT parse_file_list(FILE_LIST *flList, LPCWSTR szFiles)
{
    LPCWSTR ptr = szFiles;
    WCHAR szCurFile[MAX_PATH];
    WCHAR *p;
    DWORD i = 0;

    if (!szFiles)
        return ERROR_INVALID_PARAMETER;

    flList->bAnyFromWildcard = FALSE;
    flList->bAnyDirectories = FALSE;
    flList->bAnyDontExist = FALSE;
    flList->num_alloc = 32;
    flList->dwNumFiles = 0;

    /* empty list */
    if (!szFiles[0])
        return ERROR_ACCESS_DENIED;

    flList->feFiles = calloc(flList->num_alloc, sizeof(FILE_ENTRY));

    while (*ptr)
    {
        if (i >= flList->num_alloc) grow_list( flList );

        /* change relative to absolute path */
        if (PathIsRelativeW(ptr))
        {
            GetCurrentDirectoryW(MAX_PATH, szCurFile);
            PathCombineW(szCurFile, szCurFile, ptr);
            flList->feFiles[i].bFromRelative = TRUE;
        }
        else
        {
            lstrcpyW(szCurFile, ptr);
            flList->feFiles[i].bFromRelative = FALSE;
        }

        for (p = szCurFile; *p; p++) if (*p == '/') *p = '\\';

        /* parse wildcard files if they are in the filename */
        if (StrPBrkW(szCurFile, L"*?"))
        {
            parse_wildcard_files(flList, szCurFile, &i);
            flList->bAnyFromWildcard = TRUE;
            i--;
        }
        else
        {
            FILE_ENTRY *file = &flList->feFiles[i];
            add_file_to_entry(file, szCurFile);
            file->attributes = GetFileAttributesW( file->szFullPath );
            file->bExists = (file->attributes != INVALID_FILE_ATTRIBUTES);
            if (!file->bExists) flList->bAnyDontExist = TRUE;
            if (IsAttribDir(file->attributes)) flList->bAnyDirectories = TRUE;
        }

        /* advance to the next string */
        ptr += lstrlenW(ptr) + 1;
        i++;
    }
    flList->dwNumFiles = i;

    return S_OK;
}

/* free the FILE_LIST */
static void destroy_file_list(FILE_LIST *flList)
{
    DWORD i;

    if (!flList || !flList->feFiles)
        return;

    for (i = 0; i < flList->dwNumFiles; i++)
    {
        free(flList->feFiles[i].szDirectory);
        free(flList->feFiles[i].szFilename);
        free(flList->feFiles[i].szFullPath);
    }

    free(flList->feFiles);
}

static void copy_dir_to_dir(FILE_OPERATION *op, const FILE_ENTRY *feFrom, LPCWSTR szDestPath)
{
    WCHAR szFrom[MAX_PATH], szTo[MAX_PATH];
    SHFILEOPSTRUCTW fileOp;

    if (IsDotDir(feFrom->szFilename))
        return;

    if (PathFileExistsW(szDestPath))
        PathCombineW(szTo, szDestPath, feFrom->szFilename);
    else
        lstrcpyW(szTo, szDestPath);

    if (!(op->req->fFlags & FOF_NOCONFIRMATION) && PathFileExistsW(szTo)) {
        if (!SHELL_ConfirmDialogW(op->req->hwnd, ASK_OVERWRITE_FOLDER, feFrom->szFilename, op))
        {
            /* Vista returns an ERROR_CANCELLED even if user pressed "No" */
            if (!op->bManyItems)
                op->bCancelled = TRUE;
            return;
        }
    }

    szTo[lstrlenW(szTo) + 1] = '\0';
    SHNotifyCreateDirectoryW(szTo, NULL);

    PathCombineW(szFrom, feFrom->szFullPath, L"*.*");
    szFrom[lstrlenW(szFrom) + 1] = '\0';

    fileOp = *op->req;
    fileOp.pFrom = szFrom;
    fileOp.pTo = szTo;
    fileOp.fFlags &= ~FOF_MULTIDESTFILES; /* we know we're copying to one dir */

    /* Don't ask the user about overwriting files when he accepted to overwrite the
       folder. FIXME: this is not exactly what Windows does - e.g. there would be
       an additional confirmation for a nested folder */
    fileOp.fFlags |= FOF_NOCONFIRMATION;  

    SHFileOperationW(&fileOp);
}

static BOOL copy_file_to_file(FILE_OPERATION *op, const WCHAR *szFrom, const WCHAR *szTo)
{
    if (!(op->req->fFlags & FOF_NOCONFIRMATION) && PathFileExistsW(szTo))
    {
        if (!SHELL_ConfirmDialogW(op->req->hwnd, ASK_OVERWRITE_FILE, PathFindFileNameW(szTo), op))
            return FALSE;
    }

    return SHNotifyCopyFileW(szFrom, szTo, FALSE) == 0;
}

/* copy a file or directory to another directory */
static void copy_to_dir(FILE_OPERATION *op, const FILE_ENTRY *feFrom, const FILE_ENTRY *feTo)
{
    if (!PathFileExistsW(feTo->szFullPath))
        SHNotifyCreateDirectoryW(feTo->szFullPath, NULL);

    if (IsAttribFile(feFrom->attributes))
    {
        WCHAR szDestPath[MAX_PATH];

        PathCombineW(szDestPath, feTo->szFullPath, feFrom->szFilename);
        copy_file_to_file(op, feFrom->szFullPath, szDestPath);
    }
    else if (!(op->req->fFlags & FOF_FILESONLY && feFrom->bFromWildcard))
        copy_dir_to_dir(op, feFrom, feTo->szFullPath);
}

static void create_dest_dirs(LPCWSTR szDestDir)
{
    WCHAR dir[MAX_PATH];
    LPCWSTR ptr = StrChrW(szDestDir, '\\');

    /* make sure all directories up to last one are created */
    while (ptr && (ptr = StrChrW(ptr + 1, '\\')))
    {
        lstrcpynW(dir, szDestDir, ptr - szDestDir + 1);

        if (!PathFileExistsW(dir))
            SHNotifyCreateDirectoryW(dir, NULL);
    }

    /* create last directory */
    if (!PathFileExistsW(szDestDir))
        SHNotifyCreateDirectoryW(szDestDir, NULL);
}

/* the FO_COPY operation */
static int copy_files(FILE_OPERATION *op, const FILE_LIST *flFrom, FILE_LIST *flTo)
{
    DWORD i;
    const FILE_ENTRY *entryToCopy;
    const FILE_ENTRY *fileDest = &flTo->feFiles[0];

    if (flFrom->bAnyDontExist)
        return ERROR_SHELL_INTERNAL_FILE_NOT_FOUND;

    if (flTo->dwNumFiles == 0)
    {
        /* If the destination is empty, SHFileOperation should use the current directory */
        WCHAR curdir[MAX_PATH+1];

        GetCurrentDirectoryW(MAX_PATH, curdir);
        curdir[lstrlenW(curdir)+1] = 0;

        destroy_file_list(flTo);
        ZeroMemory(flTo, sizeof(FILE_LIST));
        parse_file_list(flTo, curdir);
        fileDest = &flTo->feFiles[0];
    }

    if (op->req->fFlags & FOF_MULTIDESTFILES && flTo->dwNumFiles > 1)
    {
        if (flFrom->bAnyFromWildcard)
            return ERROR_CANCELLED;

        if (flFrom->dwNumFiles != flTo->dwNumFiles)
        {
            if (flFrom->dwNumFiles != 1 && !IsAttribDir(fileDest->attributes))
                return ERROR_CANCELLED;

            /* Free all but the first entry. */
            for (i = 1; i < flTo->dwNumFiles; i++)
            {
                free(flTo->feFiles[i].szDirectory);
                free(flTo->feFiles[i].szFilename);
                free(flTo->feFiles[i].szFullPath);
            }

            flTo->dwNumFiles = 1;
        }
        else if (IsAttribDir(fileDest->attributes))
        {
            for (i = 1; i < flTo->dwNumFiles; i++)
                if (!IsAttribDir(flTo->feFiles[i].attributes) ||
                    !IsAttribDir(flFrom->feFiles[i].attributes))
                {
                    return ERROR_CANCELLED;
                }
        }
    }
    else if (flFrom->dwNumFiles != 1)
    {
        if (flTo->dwNumFiles != 1 && !IsAttribDir(fileDest->attributes))
            return ERROR_CANCELLED;

        if (PathFileExistsW(fileDest->szFullPath) &&
            IsAttribFile(fileDest->attributes))
        {
            return ERROR_CANCELLED;
        }

        if (flTo->dwNumFiles == 1 && fileDest->bFromRelative &&
            !PathFileExistsW(fileDest->szFullPath))
        {
            return ERROR_CANCELLED;
        }
    }

    for (i = 0; i < flFrom->dwNumFiles; i++)
    {
        entryToCopy = &flFrom->feFiles[i];

        if ((op->req->fFlags & FOF_MULTIDESTFILES) &&
            flTo->dwNumFiles > 1)
        {
            fileDest = &flTo->feFiles[i];
        }

        if (IsAttribDir(entryToCopy->attributes) &&
            !lstrcmpiW(entryToCopy->szFullPath, fileDest->szDirectory))
        {
            return ERROR_SUCCESS;
        }

        create_dest_dirs(fileDest->szDirectory);

        if (!lstrcmpiW(entryToCopy->szFullPath, fileDest->szFullPath))
        {
            if (IsAttribFile(entryToCopy->attributes))
                return ERROR_NO_MORE_SEARCH_HANDLES;
            else
                return ERROR_SUCCESS;
        }

        if ((flFrom->dwNumFiles > 1 && flTo->dwNumFiles == 1) ||
            IsAttribDir(fileDest->attributes))
        {
            copy_to_dir(op, entryToCopy, fileDest);
        }
        else if (IsAttribDir(entryToCopy->attributes))
        {
            copy_dir_to_dir(op, entryToCopy, fileDest->szFullPath);
        }
        else
        {
            if (!copy_file_to_file(op, entryToCopy->szFullPath, fileDest->szFullPath))
            {
                op->req->fAnyOperationsAborted = TRUE;
                return ERROR_CANCELLED;
            }
        }

        /* Vista return code. XP would return e.g. ERROR_FILE_NOT_FOUND, ERROR_ALREADY_EXISTS */
        if (op->bCancelled)
            return ERROR_CANCELLED;
    }

    /* Vista return code. On XP if the used pressed "No" for the last item,
     * ERROR_ARENA_TRASHED would be returned */
    return ERROR_SUCCESS;
}

static BOOL confirm_delete_list(HWND hWnd, DWORD fFlags, BOOL fTrash, const FILE_LIST *flFrom)
{
    if (flFrom->dwNumFiles > 1)
    {
        WCHAR tmp[12];

        swprintf(tmp, L"%d", flFrom->dwNumFiles);
        return SHELL_ConfirmDialogW(hWnd, (fTrash?ASK_TRASH_MULTIPLE_ITEM:ASK_DELETE_MULTIPLE_ITEM), tmp, NULL);
    }
    else
    {
        const FILE_ENTRY *fileEntry = &flFrom->feFiles[0];

        if (IsAttribFile(fileEntry->attributes))
            return SHELL_ConfirmDialogW(hWnd, (fTrash?ASK_TRASH_FILE:ASK_DELETE_FILE), fileEntry->szFullPath, NULL);
        else if (!(fFlags & FOF_FILESONLY && fileEntry->bFromWildcard))
            return SHELL_ConfirmDialogW(hWnd, (fTrash?ASK_TRASH_FOLDER:ASK_DELETE_FOLDER), fileEntry->szFullPath, NULL);
    }
    return TRUE;
}

/* the FO_DELETE operation */
static int delete_files(LPSHFILEOPSTRUCTW lpFileOp, const FILE_LIST *flFrom)
{
    const FILE_ENTRY *fileEntry;
    DWORD i;
    int ret;
    BOOL bTrash;

    if (!flFrom->dwNumFiles)
        return ERROR_SUCCESS;

    /* Windows also checks only the first item */
    bTrash = (lpFileOp->fFlags & FOF_ALLOWUNDO);
    //bTrash = (lpFileOp->fFlags & FOF_ALLOWUNDO) && is_trash_available();

    if (!(lpFileOp->fFlags & FOF_NOCONFIRMATION) || (!bTrash && lpFileOp->fFlags & FOF_WANTNUKEWARNING))
        if (!confirm_delete_list(lpFileOp->hwnd, lpFileOp->fFlags, bTrash, flFrom))
        {
            lpFileOp->fAnyOperationsAborted = TRUE;
            return 0;
        }

    for (i = 0; i < flFrom->dwNumFiles; i++)
    {
        fileEntry = &flFrom->feFiles[i];

        if (!IsAttribFile(fileEntry->attributes) &&
            (lpFileOp->fFlags & FOF_FILESONLY && fileEntry->bFromWildcard))
            continue;

        if (bTrash)
        {
            BOOL bDelete;
            // if (trash_file(fileEntry->szFullPath))
                // continue;

            /* Note: Windows silently deletes the file in such a situation, we show a dialog */
            if (!(lpFileOp->fFlags & FOF_NOCONFIRMATION) || (lpFileOp->fFlags & FOF_WANTNUKEWARNING))
                bDelete = SHELL_ConfirmDialogW(lpFileOp->hwnd, ASK_CANT_TRASH_ITEM, fileEntry->szFullPath, NULL);
            else
                bDelete = TRUE;

            if (!bDelete)
            {
                lpFileOp->fAnyOperationsAborted = TRUE;
                break;
            }
        }
        
        /* delete the file or directory */
        if (IsAttribFile(fileEntry->attributes))
            ret = DeleteFileW(fileEntry->szFullPath) ?
                    ERROR_SUCCESS : GetLastError();
        else
            ret = SHELL_DeleteDirectoryW(lpFileOp->hwnd, fileEntry->szFullPath, FALSE);

        if (ret)
            return ret;
    }

    return ERROR_SUCCESS;
}

/* moves a file or directory to another directory */
static void move_to_dir(LPSHFILEOPSTRUCTW lpFileOp, const FILE_ENTRY *feFrom, const FILE_ENTRY *feTo)
{
    WCHAR szDestPath[MAX_PATH];

    PathCombineW(szDestPath, feTo->szFullPath, feFrom->szFilename);
    SHNotifyMoveFileW(feFrom->szFullPath, szDestPath);
}

/* the FO_MOVE operation */
static int move_files(LPSHFILEOPSTRUCTW lpFileOp, const FILE_LIST *flFrom, const FILE_LIST *flTo)
{
    DWORD i;
    INT mismatched = 0;
    const FILE_ENTRY *entryToMove;
    const FILE_ENTRY *fileDest;

    if (!flFrom->dwNumFiles)
        return ERROR_SUCCESS;

    if (!flTo->dwNumFiles)
        return ERROR_FILE_NOT_FOUND;

    if (!(lpFileOp->fFlags & FOF_MULTIDESTFILES) &&
        flTo->dwNumFiles > 1 && flFrom->dwNumFiles > 1)
    {
        return ERROR_CANCELLED;
    }

    if (!(lpFileOp->fFlags & FOF_MULTIDESTFILES) &&
        !flFrom->bAnyDirectories &&
        flFrom->dwNumFiles > flTo->dwNumFiles)
    {
        return ERROR_CANCELLED;
    }

    if (!PathFileExistsW(flTo->feFiles[0].szDirectory))
        return ERROR_CANCELLED;

    if (lpFileOp->fFlags & FOF_MULTIDESTFILES)
        mismatched = flFrom->dwNumFiles - flTo->dwNumFiles;

    fileDest = &flTo->feFiles[0];
    for (i = 0; i < flFrom->dwNumFiles; i++)
    {
        entryToMove = &flFrom->feFiles[i];

        if (!PathFileExistsW(fileDest->szDirectory))
            return ERROR_CANCELLED;

        if (lpFileOp->fFlags & FOF_MULTIDESTFILES)
        {
            if (i >= flTo->dwNumFiles)
                break;
            fileDest = &flTo->feFiles[i];
            if (mismatched && !fileDest->bExists)
            {
                create_dest_dirs(flTo->feFiles[i].szFullPath);
                flTo->feFiles[i].bExists = TRUE;
                flTo->feFiles[i].attributes = FILE_ATTRIBUTE_DIRECTORY;
            }
        }

        if (fileDest->bExists && IsAttribDir(fileDest->attributes))
            move_to_dir(lpFileOp, entryToMove, fileDest);
        else
            SHNotifyMoveFileW(entryToMove->szFullPath, fileDest->szFullPath);
    }

    if (mismatched > 0)
    {
        if (flFrom->bAnyDirectories)
            return DE_DESTSAMETREE;
        else
            return DE_SAMEFILE;
    }

    return ERROR_SUCCESS;
}

/* the FO_RENAME files */
static int rename_files(LPSHFILEOPSTRUCTW lpFileOp, const FILE_LIST *flFrom, const FILE_LIST *flTo)
{
    const FILE_ENTRY *feFrom;
    const FILE_ENTRY *feTo;

    if (flFrom->dwNumFiles != 1)
        return ERROR_GEN_FAILURE;

    if (flTo->dwNumFiles != 1)
        return ERROR_CANCELLED;

    feFrom = &flFrom->feFiles[0];
    feTo= &flTo->feFiles[0];

    /* fail if destination doesn't exist */
    if (!feFrom->bExists)
        return ERROR_SHELL_INTERNAL_FILE_NOT_FOUND;

    /* fail if destination already exists */
    if (feTo->bExists)
        return ERROR_ALREADY_EXISTS;

    return SHNotifyMoveFileW(feFrom->szFullPath, feTo->szFullPath);
}

/* alert the user if an unsupported flag is used */
static void check_flags(FILEOP_FLAGS fFlags)
{
    WORD wUnsupportedFlags = FOF_NO_CONNECTED_ELEMENTS |
        FOF_NOCOPYSECURITYATTRIBS | FOF_NORECURSEREPARSE |
        FOF_RENAMEONCOLLISION | FOF_WANTMAPPINGHANDLE;

    if (fFlags & wUnsupportedFlags)
        FIXME("Unsupported flags: %04x\n", fFlags);
}

/*************************************************************************
 * SHFileOperationW          [SHELL32.@]
 *
 * See SHFileOperationA
 */
int WINAPI SHFileOperationW(LPSHFILEOPSTRUCTW lpFileOp)
{
    FILE_OPERATION op;
    FILE_LIST flFrom, flTo;
    int ret = 0;

    if (!lpFileOp)
        return ERROR_INVALID_PARAMETER;

    check_flags(lpFileOp->fFlags);

    ZeroMemory(&flFrom, sizeof(FILE_LIST));
    ZeroMemory(&flTo, sizeof(FILE_LIST));

    if ((ret = parse_file_list(&flFrom, lpFileOp->pFrom)))
        return ret;

    if (lpFileOp->wFunc != FO_DELETE)
        parse_file_list(&flTo, lpFileOp->pTo);

    ZeroMemory(&op, sizeof(op));
    op.req = lpFileOp;
    op.bManyItems = (flFrom.dwNumFiles > 1);
    lpFileOp->fAnyOperationsAborted = FALSE;

    switch (lpFileOp->wFunc)
    {
        case FO_COPY:
            ret = copy_files(&op, &flFrom, &flTo);
            break;
        case FO_DELETE:
            ret = delete_files(lpFileOp, &flFrom);
            break;
        case FO_MOVE:
            ret = move_files(lpFileOp, &flFrom, &flTo);
            break;
        case FO_RENAME:
            ret = rename_files(lpFileOp, &flFrom, &flTo);
            break;
        default:
            ret = ERROR_INVALID_PARAMETER;
            break;
    }

    destroy_file_list(&flFrom);

    if (lpFileOp->wFunc != FO_DELETE)
        destroy_file_list(&flTo);

    if (ret == ERROR_CANCELLED)
        lpFileOp->fAnyOperationsAborted = TRUE;

    SetLastError(ERROR_SUCCESS);
    return ret;
}

struct file_operation
{
    IFileOperation IFileOperation_iface;
    LONG ref;
	SHFILEOPSTRUCTW fileOperation;
	DWORD operationFlags;
	UINT operation;
	DWORD fileAttributes;
	BOOL aborted;
	IFileOperationProgressSink *sink;
};

static inline struct file_operation *impl_from_IFileOperation(IFileOperation *iface)
{
    return CONTAINING_RECORD(iface, struct file_operation, IFileOperation_iface);
}

static HRESULT WINAPI file_operation_QueryInterface(IFileOperation *iface, REFIID riid, void **out)
{
    struct file_operation *operation = impl_from_IFileOperation(iface);

    TRACE("(%p, %s, %p).\n", iface, debugstr_guid(riid), out);

    if (IsEqualIID(&IID_IFileOperation, riid) ||
        IsEqualIID(&IID_IUnknown, riid))
        *out = &operation->IFileOperation_iface;
    else
    {
        FIXME("not implemented for %s.\n", debugstr_guid(riid));
        *out = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown *)*out);
    return S_OK;
}

static ULONG WINAPI file_operation_AddRef(IFileOperation *iface)
{
    struct file_operation *operation = impl_from_IFileOperation(iface);
    ULONG ref = InterlockedIncrement(&operation->ref);

    TRACE("(%p): ref=%lu.\n", iface, ref);

    return ref;
}

static ULONG WINAPI file_operation_Release(IFileOperation *iface)
{
    struct file_operation *operation = impl_from_IFileOperation(iface);
    ULONG ref = InterlockedDecrement(&operation->ref);

    TRACE("(%p): ref=%lu.\n", iface, ref);

    if (!ref)
    {
        free(operation);
    }

    return ref;
}

static HRESULT WINAPI file_operation_Advise(IFileOperation *iface, IFileOperationProgressSink *sink, DWORD *cookie)
{
	// struct file_operation *operation = impl_from_IFileOperation(iface);
    FIXME("(%p, %p, %p): stub.\n", iface, sink, cookie);

    return S_OK;
}

static HRESULT WINAPI file_operation_Unadvise(IFileOperation *iface, DWORD cookie)
{
	// struct file_operation *operation = impl_from_IFileOperation(iface);
    FIXME("(%p, %lx): stub.\n", iface, cookie);

    return S_OK;
}

static HRESULT WINAPI file_operation_SetOperationFlags(IFileOperation *iface, DWORD flags)
{
	// struct file_operation *operation = impl_from_IFileOperation(iface);
    FIXME("(%p, %lx): stub.\n", iface, flags);

    return S_OK;
}

static HRESULT WINAPI file_operation_SetProgressMessage(IFileOperation *iface, LPCWSTR message)
{
	// struct file_operation *operation = impl_from_IFileOperation(iface);
    FIXME("(%p, %s): stub.\n", iface, debugstr_w(message));

    return S_OK;
}

static HRESULT WINAPI file_operation_SetProgressDialog(IFileOperation *iface, IOperationsProgressDialog *dialog)
{
	// struct file_operation *operation = impl_from_IFileOperation(iface);
    FIXME("(%p, %p): stub.\n", iface, dialog);

    return S_OK;
}

static HRESULT WINAPI file_operation_SetProperties(IFileOperation *iface, IPropertyChangeArray *array)
{
	// struct file_operation *operation = impl_from_IFileOperation(iface);
    FIXME("(%p, %p): stub.\n", iface, array);

    return S_OK;
}

static HRESULT WINAPI file_operation_SetOwnerWindow(IFileOperation *iface, HWND owner)
{
	struct file_operation *operation = impl_from_IFileOperation(iface);
	
	operation->fileOperation.hwnd = owner;
    FIXME("(%p, %p): stub.\n", iface, owner);

    return S_OK;
}

static HRESULT WINAPI file_operation_ApplyPropertiesToItem(IFileOperation *iface, IShellItem *item)
{
	// struct file_operation *operation = impl_from_IFileOperation(iface);
    FIXME("(%p, %p): stub.\n", iface, item);

    return S_OK;
}

static HRESULT WINAPI file_operation_ApplyPropertiesToItems(IFileOperation *iface, IUnknown *items)
{
	// struct file_operation *operation = impl_from_IFileOperation(iface);
    FIXME("(%p, %p): stub.\n", iface, items);

    return S_OK;
}

static HRESULT WINAPI file_operation_RenameItem(IFileOperation *iface, IShellItem *item, LPCWSTR name,
        IFileOperationProgressSink *sink)
{
	struct file_operation *operation = impl_from_IFileOperation(iface);
	operation->fileOperation.wFunc = FO_RENAME;	
    FIXME("(%p, %p, %s, %p): stub.\n", iface, item, debugstr_w(name), sink);

    return S_OK;
}

static HRESULT WINAPI file_operation_RenameItems(IFileOperation *iface, IUnknown *items, LPCWSTR name)
{
	struct file_operation *operation = impl_from_IFileOperation(iface);
	operation->fileOperation.wFunc = FO_RENAME;
	operation->fileOperation.fFlags = FOF_MULTIDESTFILES;
	operation->operation = FO_RENAME;
	
    FIXME("(%p, %p, %s): stub.\n", iface, items, debugstr_w(name));

    return S_OK;
}

static HRESULT WINAPI file_operation_MoveItem(IFileOperation *iface, IShellItem *item, IShellItem *folder,
        LPCWSTR name, IFileOperationProgressSink *sink)
{
    struct file_operation *operation = impl_from_IFileOperation(iface);
    LPWSTR srcBuffer;
    LPWSTR dstBuffer;
    
    srcBuffer = (LPWSTR)HeapAlloc(GetProcessHeap(), 8, MAX_PATH * 2);
    if (!srcBuffer)
        return E_OUTOFMEMORY;
    dstBuffer = (LPWSTR)HeapAlloc(GetProcessHeap(), 8, MAX_PATH * 2);
    if (!dstBuffer) {
        HeapFree(GetProcessHeap(), 0, srcBuffer);
        return E_OUTOFMEMORY;
    }

    IShellItem2_GetDisplayName(item, SIGDN_FILESYSPATH, &srcBuffer);
    IShellItem2_GetDisplayName(folder, SIGDN_FILESYSPATH, &dstBuffer);

    operation->fileOperation.wFunc = FO_MOVE;
    operation->fileOperation.fFlags = FOF_SILENT | FOF_NOCONFIRMATION | FOF_NOERRORUI |
                     FOF_NOCONFIRMMKDIR | FOF_NOCOPYSECURITYATTRIBS;

	StringCchCatW(dstBuffer, wcslen(srcBuffer) + 1, L"\\");
	StringCchCatW(dstBuffer, wcslen(srcBuffer) + wcslen(name) + 1, name);
    operation->fileOperation.pFrom = srcBuffer;
    operation->fileOperation.pTo =    dstBuffer;

    operation->operation = FO_MOVE;
    operation->sink = sink;
    IFileOperationProgressSink_FinishOperations(sink, S_OK);

    FIXME("(%p, %p, %p, %s, %p): stub.\n", iface, item, folder, debugstr_w(name), sink);
    
    return S_OK;
}

static HRESULT WINAPI file_operation_MoveItems(IFileOperation *iface, IUnknown *items, IShellItem *folder)
{
	struct file_operation *operation = impl_from_IFileOperation(iface);
	operation->fileOperation.wFunc = FO_MOVE;
	operation->fileOperation.fFlags = FOF_MULTIDESTFILES;
	operation->operation = FO_MOVE;
	
    FIXME("(%p, %p, %p): stub.\n", iface, items, folder);

    return S_OK;
}

static HRESULT WINAPI file_operation_CopyItem(IFileOperation *iface, IShellItem *item, IShellItem *folder,
        LPCWSTR name, IFileOperationProgressSink *sink)
{
	struct file_operation *operation = impl_from_IFileOperation(iface);
    LPWSTR dstBuffer;
    
    srcBuffer = (LPWSTR)HeapAlloc(GetProcessHeap(), 8, MAX_PATH * 2);
    if (!srcBuffer)
        return E_OUTOFMEMORY;
    dstBuffer = (LPWSTR)HeapAlloc(GetProcessHeap(), 8, MAX_PATH * 2);
    if (!dstBuffer) {
        HeapFree(GetProcessHeap(), 0, srcBuffer);
        return E_OUTOFMEMORY;
    }

    IShellItem2_GetDisplayName(item, SIGDN_FILESYSPATH, &srcBuffer);
    IShellItem2_GetDisplayName(folder, SIGDN_FILESYSPATH, &dstBuffer);

    operation->fileOperation.wFunc = FO_COPY;
    operation->fileOperation.fFlags = FOF_SILENT | FOF_NOCONFIRMATION | FOF_NOERRORUI |
                     FOF_NOCONFIRMMKDIR | FOF_NOCOPYSECURITYATTRIBS;

	StringCchCatW(dstBuffer, wcslen(srcBuffer) + 1, L"\\");
	StringCchCatW(dstBuffer, wcslen(srcBuffer) + wcslen(name) + 1, name);
    operation->fileOperation.pFrom = srcBuffer;
    operation->fileOperation.pTo =    dstBuffer;

    operation->operation = FO_MOVE;
    operation->sink = sink;
    IFileOperationProgressSink_FinishOperations(sink, S_OK);
	
    FIXME("(%p, %p, %p, %s, %p): stub.\n", iface, item, folder, debugstr_w(name), sink);

    return S_OK;
}

static HRESULT WINAPI file_operation_CopyItems(IFileOperation *iface, IUnknown *items, IShellItem *folder)
{
	struct file_operation *operation = impl_from_IFileOperation(iface);
	operation->fileOperation.wFunc = FO_COPY;
	operation->operation = FO_COPY;
	operation->fileOperation.fFlags = FOF_NOCONFIRMMKDIR;
	
    FIXME("(%p, %p, %p): stub.\n", iface, items, folder);

    return S_OK;
}

static HRESULT WINAPI file_operation_DeleteItem(IFileOperation *iface, IShellItem *item,
        IFileOperationProgressSink *sink)
{
	struct file_operation *operation = impl_from_IFileOperation(iface);
	operation->fileOperation.wFunc = FO_DELETE;
	operation->operation = FO_DELETE;
	
    FIXME("(%p, %p, %p): stub.\n", iface, item, sink);

    return S_OK;
}

static HRESULT WINAPI file_operation_DeleteItems(IFileOperation *iface, IUnknown *items)
{
	struct file_operation *operation = impl_from_IFileOperation(iface);
	operation->fileOperation.wFunc = FO_DELETE;
	operation->fileOperation.fFlags = FOF_MULTIDESTFILES;
	operation->operation = FO_DELETE;
	
    FIXME("(%p, %p): stub.\n", iface, items);

    return S_OK;
}

static HRESULT WINAPI file_operation_NewItem(IFileOperation *iface, IShellItem *folder, DWORD attributes,
        LPCWSTR name, LPCWSTR template, IFileOperationProgressSink *sink)
{
	struct file_operation *operation = impl_from_IFileOperation(iface);
	operation->operation = FO_NEW_ITEM;	
	// struct file_operation *operation = impl_from_IFileOperation(iface);
	
    FIXME("(%p, %p, %lx, %s, %s, %p): stub.\n", iface, folder, attributes,
          debugstr_w(name), debugstr_w(template), sink);

    return S_OK;
}

static HRESULT WINAPI file_operation_PerformOperations(IFileOperation *iface)
{
	struct file_operation *operation = impl_from_IFileOperation(iface);
	int resp;
    FIXME("(%p): stub.\n", iface);
	
	resp = SHFileOperationW(&operation->fileOperation);
	operation->aborted = FALSE;	
	
	if(resp == 0){
		return S_OK;
	}

    return S_OK;
}

static HRESULT WINAPI file_operation_GetAnyOperationsAborted(IFileOperation *iface, BOOL *aborted)
{
	struct file_operation *operation = impl_from_IFileOperation(iface);
	
	*aborted = operation->aborted;
    FIXME("(%p, %p): stub.\n", iface, aborted);

    return S_OK;
}

static const IFileOperationVtbl file_operation_vtbl =
{
    file_operation_QueryInterface,
    file_operation_AddRef,
    file_operation_Release,
    file_operation_Advise,
    file_operation_Unadvise,
    file_operation_SetOperationFlags,
    file_operation_SetProgressMessage,
    file_operation_SetProgressDialog,
    file_operation_SetProperties,
    file_operation_SetOwnerWindow,
    file_operation_ApplyPropertiesToItem,
    file_operation_ApplyPropertiesToItems,
    file_operation_RenameItem,
    file_operation_RenameItems,
    file_operation_MoveItem,
    file_operation_MoveItems,
    file_operation_CopyItem,
    file_operation_CopyItems,
    file_operation_DeleteItem,
    file_operation_DeleteItems,
    file_operation_NewItem,
    file_operation_PerformOperations,
    file_operation_GetAnyOperationsAborted
};

HRESULT WINAPI IFileOperation_Constructor(IUnknown *outer, REFIID riid, void **out)
{
    struct file_operation *object;
    HRESULT hr;
	
    SHFILEOPSTRUCTW fileOperation = {0};

    ZeroMemory(&fileOperation, sizeof(fileOperation));

	DbgPrint("IFileOperation_Constructor called\n");

    object = calloc(1, sizeof(*object));
    if (!object)
        return E_OUTOFMEMORY;

    object->IFileOperation_iface.lpVtbl = &file_operation_vtbl;
    object->ref = 1;
    object->aborted = FALSE;
    object->fileOperation = fileOperation;

    hr = IFileOperation_QueryInterface(&object->IFileOperation_iface, riid, out);
    IFileOperation_Release(&object->IFileOperation_iface);

    return hr;
}
