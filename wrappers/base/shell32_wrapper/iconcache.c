/*
 *	shell icon cache (SIC)
 *
 * Copyright 1998, 1999 Juergen Schmied
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

#include "wine/config.h"
#include "wine/port.h"


#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#define COBJMACROS
#include "main.h"
#include <commctrl.h>

WINE_DEFAULT_DEBUG_CHANNEL(shell);

WCHAR swShell32Name[MAX_PATH];

static inline BOOL SHELL_OsIsUnicode(void)
{
    /* if high-bit of version is 0, we are emulating NT */
    return !(GetVersion() & 0x80000000);
}

/*************************************************************************
 * SHELL_IsShortcut		[internal]
 *
 * Decide if an item id list points to a shell shortcut
 */
BOOL SHELL_IsShortcut(LPCITEMIDLIST pidlLast)
{
    char szTemp[MAX_PATH];
    HKEY keyCls;
    BOOL ret = FALSE;

    if (_ILGetExtension(pidlLast, szTemp, MAX_PATH) &&
          HCR_MapTypeToValueA(szTemp, szTemp, MAX_PATH, TRUE))
    {
        if (ERROR_SUCCESS == RegOpenKeyExA(HKEY_CLASSES_ROOT, szTemp, 0, KEY_QUERY_VALUE, &keyCls))
        {
          if (ERROR_SUCCESS == RegQueryValueExA(keyCls, "IsShortcut", NULL, NULL, NULL, NULL))
            ret = TRUE;

          RegCloseKey(keyCls);
        }
    }

    return ret;
}

/********************** THE ICON CACHE ********************************/

#define INVALID_INDEX -1

typedef struct
{
	LPWSTR sSourceFile;	/* file (not path!) containing the icon */
	DWORD dwSourceIndex;	/* index within the file, if it is a resource ID it will be negated */
	DWORD dwListIndex;	/* index within the iconlist */
	DWORD dwFlags;		/* GIL_* flags */
	DWORD dwAccessTime;
} SIC_ENTRY, * LPSIC_ENTRY;

static HDPA sic_hdpa;
static HIMAGELIST ShellSmallIconList;
static HIMAGELIST ShellBigIconList;

static CRITICAL_SECTION SHELL32_SicCS;
static CRITICAL_SECTION_DEBUG critsect_debug =
{
    0, 0, &SHELL32_SicCS,
    { &critsect_debug.ProcessLocksList, &critsect_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": SHELL32_SicCS") }
};
static CRITICAL_SECTION SHELL32_SicCS = { &critsect_debug, -1, 0, 0, 0, 0 };

/*****************************************************************************
 * SIC_CompareEntries
 *
 * NOTES
 *  Callback for DPA_Search
 */
static INT CALLBACK SIC_CompareEntries( LPVOID p1, LPVOID p2, LPARAM lparam)
{
        LPSIC_ENTRY e1 = p1, e2 = p2;

	TRACE("%p %p %8lx\n", p1, p2, lparam);

	/* Icons in the cache are keyed by the name of the file they are
	 * loaded from, their resource index and the fact if they have a shortcut
	 * icon overlay or not. 
	 */
	if (e1->dwSourceIndex != e2->dwSourceIndex || /* first the faster one */
	    (e1->dwFlags & GIL_FORSHORTCUT) != (e2->dwFlags & GIL_FORSHORTCUT)) 
	  return 1;

	if (strcmpiW(e1->sSourceFile,e2->sSourceFile))
	  return 1;

	return 0;
}

/* declare SIC_LoadOverlayIcon() */
static int SIC_LoadOverlayIcon(int icon_idx);

/*****************************************************************************
 * SIC_OverlayShortcutImage			[internal]
 *
 * NOTES
 *  Creates a new icon as a copy of the passed-in icon, overlayed with a
 *  shortcut image. 
 */
static HICON SIC_OverlayShortcutImage(HICON SourceIcon, BOOL large)
{	ICONINFO SourceIconInfo, ShortcutIconInfo, TargetIconInfo;
	HICON ShortcutIcon, TargetIcon;
	BITMAP SourceBitmapInfo, ShortcutBitmapInfo;
	HDC SourceDC = NULL,
	  ShortcutDC = NULL,
	  TargetDC = NULL,
	  ScreenDC = NULL;
	HBITMAP OldSourceBitmap = NULL,
	  OldShortcutBitmap = NULL,
	  OldTargetBitmap = NULL;

	static int s_imgListIdx = -1;

	/* Get information about the source icon and shortcut overlay */
	if (! GetIconInfo(SourceIcon, &SourceIconInfo)
	    || 0 == GetObjectW(SourceIconInfo.hbmColor, sizeof(BITMAP), &SourceBitmapInfo))
	{
	  return NULL;
	}

	/* search for the shortcut icon only once */
	if (s_imgListIdx == -1)
	    s_imgListIdx = SIC_LoadOverlayIcon(- IDI_SHELL_SHORTCUT);
                           /* FIXME should use icon index 29 instead of the
                              resource id, but not all icons are present yet
                              so we can't use icon indices */

	if (s_imgListIdx != -1)
	{
	    if (large)
	        ShortcutIcon = ImageList_GetIcon(ShellBigIconList, s_imgListIdx, ILD_TRANSPARENT);
	    else
	        ShortcutIcon = ImageList_GetIcon(ShellSmallIconList, s_imgListIdx, ILD_TRANSPARENT);
	} else
	    ShortcutIcon = NULL;

	if (NULL == ShortcutIcon
	    || ! GetIconInfo(ShortcutIcon, &ShortcutIconInfo)
	    || 0 == GetObjectW(ShortcutIconInfo.hbmColor, sizeof(BITMAP), &ShortcutBitmapInfo))
	{
	  return NULL;
	}

	TargetIconInfo = SourceIconInfo;
	TargetIconInfo.hbmMask = NULL;
	TargetIconInfo.hbmColor = NULL;

	/* Setup the source, shortcut and target masks */
	SourceDC = CreateCompatibleDC(NULL);
	if (NULL == SourceDC) goto fail;
	OldSourceBitmap = SelectObject(SourceDC, SourceIconInfo.hbmMask);
	if (NULL == OldSourceBitmap) goto fail;

	ShortcutDC = CreateCompatibleDC(NULL);
	if (NULL == ShortcutDC) goto fail;
	OldShortcutBitmap = SelectObject(ShortcutDC, ShortcutIconInfo.hbmMask);
	if (NULL == OldShortcutBitmap) goto fail;

	TargetDC = CreateCompatibleDC(NULL);
	if (NULL == TargetDC) goto fail;
	TargetIconInfo.hbmMask = CreateCompatibleBitmap(TargetDC, SourceBitmapInfo.bmWidth,
	                                                SourceBitmapInfo.bmHeight);
	if (NULL == TargetIconInfo.hbmMask) goto fail;
	ScreenDC = GetDC(NULL);
	if (NULL == ScreenDC) goto fail;
	TargetIconInfo.hbmColor = CreateCompatibleBitmap(ScreenDC, SourceBitmapInfo.bmWidth,
	                                                 SourceBitmapInfo.bmHeight);
	ReleaseDC(NULL, ScreenDC);
	if (NULL == TargetIconInfo.hbmColor) goto fail;
	OldTargetBitmap = SelectObject(TargetDC, TargetIconInfo.hbmMask);
	if (NULL == OldTargetBitmap) goto fail;

	/* Create the target mask by ANDing the source and shortcut masks */
	if (! BitBlt(TargetDC, 0, 0, SourceBitmapInfo.bmWidth, SourceBitmapInfo.bmHeight,
	             SourceDC, 0, 0, SRCCOPY) ||
	    ! BitBlt(TargetDC, 0, SourceBitmapInfo.bmHeight - ShortcutBitmapInfo.bmHeight,
	             ShortcutBitmapInfo.bmWidth, ShortcutBitmapInfo.bmHeight,
	             ShortcutDC, 0, 0, SRCAND))
	{
	  goto fail;
	}

	/* Setup the source and target xor bitmap */
	if (NULL == SelectObject(SourceDC, SourceIconInfo.hbmColor) ||
	    NULL == SelectObject(TargetDC, TargetIconInfo.hbmColor))
	{
	  goto fail;
	}

	/* Copy the source xor bitmap to the target and clear out part of it by using
	   the shortcut mask */
	if (! BitBlt(TargetDC, 0, 0, SourceBitmapInfo.bmWidth, SourceBitmapInfo.bmHeight,
	             SourceDC, 0, 0, SRCCOPY) ||
	    ! BitBlt(TargetDC, 0, SourceBitmapInfo.bmHeight - ShortcutBitmapInfo.bmHeight,
	             ShortcutBitmapInfo.bmWidth, ShortcutBitmapInfo.bmHeight,
	             ShortcutDC, 0, 0, SRCAND))
	{
	  goto fail;
	}

	if (NULL == SelectObject(ShortcutDC, ShortcutIconInfo.hbmColor)) goto fail;

	/* Now put in the shortcut xor mask */
	if (! BitBlt(TargetDC, 0, SourceBitmapInfo.bmHeight - ShortcutBitmapInfo.bmHeight,
	             ShortcutBitmapInfo.bmWidth, ShortcutBitmapInfo.bmHeight,
	             ShortcutDC, 0, 0, SRCINVERT))
	{
	  goto fail;
	}

	/* Clean up, we're not goto'ing to 'fail' after this so we can be lazy and not set
	   handles to NULL */
	SelectObject(TargetDC, OldTargetBitmap);
	DeleteObject(TargetDC);
	SelectObject(ShortcutDC, OldShortcutBitmap);
	DeleteObject(ShortcutDC);
	SelectObject(SourceDC, OldSourceBitmap);
	DeleteObject(SourceDC);

	/* Create the icon using the bitmaps prepared earlier */
	TargetIcon = CreateIconIndirect(&TargetIconInfo);

	/* CreateIconIndirect copies the bitmaps, so we can release our bitmaps now */
	DeleteObject(TargetIconInfo.hbmColor);
	DeleteObject(TargetIconInfo.hbmMask);

	return TargetIcon;

fail:
	/* Clean up scratch resources we created */
	if (NULL != OldTargetBitmap) SelectObject(TargetDC, OldTargetBitmap);
	if (NULL != TargetIconInfo.hbmColor) DeleteObject(TargetIconInfo.hbmColor);
	if (NULL != TargetIconInfo.hbmMask) DeleteObject(TargetIconInfo.hbmMask);
	if (NULL != TargetDC) DeleteObject(TargetDC);
	if (NULL != OldShortcutBitmap) SelectObject(ShortcutDC, OldShortcutBitmap);
	if (NULL != ShortcutDC) DeleteObject(ShortcutDC);
	if (NULL != OldSourceBitmap) SelectObject(SourceDC, OldSourceBitmap);
	if (NULL != SourceDC) DeleteObject(SourceDC);

	return NULL;
}

/*****************************************************************************
 * SIC_IconAppend			[internal]
 *
 * NOTES
 *  appends an icon pair to the end of the cache
 */
static INT SIC_IconAppend (LPCWSTR sSourceFile, INT dwSourceIndex, HICON hSmallIcon, HICON hBigIcon, DWORD dwFlags)
{	LPSIC_ENTRY lpsice;
	INT ret, index, index1;
	WCHAR path[MAX_PATH];
	TRACE("%s %i %p %p\n", debugstr_w(sSourceFile), dwSourceIndex, hSmallIcon ,hBigIcon);

	lpsice = SHAlloc(sizeof(SIC_ENTRY));

	GetFullPathNameW(sSourceFile, MAX_PATH, path, NULL);
	lpsice->sSourceFile = HeapAlloc( GetProcessHeap(), 0, (strlenW(path)+1)*sizeof(WCHAR) );
	strcpyW( lpsice->sSourceFile, path );

	lpsice->dwSourceIndex = dwSourceIndex;
	lpsice->dwFlags = dwFlags;

	EnterCriticalSection(&SHELL32_SicCS);

	index = DPA_InsertPtr(sic_hdpa, 0x7fff, lpsice);
	if ( INVALID_INDEX == index )
	{
	  HeapFree(GetProcessHeap(), 0, lpsice->sSourceFile);
	  SHFree(lpsice);
	  ret = INVALID_INDEX;
	}
	else
	{
	  index = ImageList_AddIcon (ShellSmallIconList, hSmallIcon);
	  index1= ImageList_AddIcon (ShellBigIconList, hBigIcon);

	  if (index!=index1)
	  {
	    FIXME("iconlists out of sync 0x%x 0x%x\n", index, index1);
	  }
	  lpsice->dwListIndex = index;
	  ret = lpsice->dwListIndex;
	}

	LeaveCriticalSection(&SHELL32_SicCS);
	return ret;
}
/****************************************************************************
 * SIC_LoadIcon				[internal]
 *
 * NOTES
 *  gets small/big icon by number from a file
 */
static INT SIC_LoadIcon (LPCWSTR sSourceFile, INT dwSourceIndex, DWORD dwFlags)
{	HICON	hiconLarge=0;
	HICON	hiconSmall=0;
	HICON 	hiconLargeShortcut;
	HICON	hiconSmallShortcut;

        PrivateExtractIconsW( sSourceFile, dwSourceIndex, 32, 32, &hiconLarge, 0, 1, 0 );
        PrivateExtractIconsW( sSourceFile, dwSourceIndex, 16, 16, &hiconSmall, 0, 1, 0 );

	if ( !hiconLarge ||  !hiconSmall)
	{
	  WARN("failure loading icon %i from %s (%p %p)\n", dwSourceIndex, debugstr_w(sSourceFile), hiconLarge, hiconSmall);
	  return -1;
	}

	if (0 != (dwFlags & GIL_FORSHORTCUT))
	{
	  hiconLargeShortcut = SIC_OverlayShortcutImage(hiconLarge, TRUE);
	  hiconSmallShortcut = SIC_OverlayShortcutImage(hiconSmall, FALSE);
	  if (NULL != hiconLargeShortcut && NULL != hiconSmallShortcut)
	  {
	    hiconLarge = hiconLargeShortcut;
	    hiconSmall = hiconSmallShortcut;
	  }
	  else
	  {
	    WARN("Failed to create shortcut overlayed icons\n");
	    if (NULL != hiconLargeShortcut) DestroyIcon(hiconLargeShortcut);
	    if (NULL != hiconSmallShortcut) DestroyIcon(hiconSmallShortcut);
	    dwFlags &= ~ GIL_FORSHORTCUT;
	  }
	}

	return SIC_IconAppend (sSourceFile, dwSourceIndex, hiconSmall, hiconLarge, dwFlags);
}
/*****************************************************************************
 * SIC_Initialize			[internal]
 */
static BOOL WINAPI SIC_Initialize( INIT_ONCE *once, void *param, void **context )
{
	HICON		hSm, hLg;
	int		cx_small, cy_small;
	int		cx_large, cy_large;

	cx_small = GetSystemMetrics(SM_CXSMICON);
	cy_small = GetSystemMetrics(SM_CYSMICON);
	cx_large = GetSystemMetrics(SM_CXICON);
	cy_large = GetSystemMetrics(SM_CYICON);

	TRACE("\n");

	sic_hdpa = DPA_Create(16);

	if (!sic_hdpa)
	{
	  return(FALSE);
	}

        ShellSmallIconList = ImageList_Create(cx_small,cy_small,ILC_COLOR32|ILC_MASK,0,0x20);
        ShellBigIconList = ImageList_Create(cx_large,cy_large,ILC_COLOR32|ILC_MASK,0,0x20);

        ImageList_SetBkColor(ShellSmallIconList, CLR_NONE);
        ImageList_SetBkColor(ShellBigIconList, CLR_NONE);

        /* Load the document icon, which is used as the default if an icon isn't found. */
        hSm = LoadImageA(shell32_hInstance, MAKEINTRESOURCEA(IDI_SHELL_DOCUMENT),
                                IMAGE_ICON, cx_small, cy_small, LR_SHARED);
        hLg = LoadImageA(shell32_hInstance, MAKEINTRESOURCEA(IDI_SHELL_DOCUMENT),
                                IMAGE_ICON, cx_large, cy_large, LR_SHARED);

        if (!hSm || !hLg) 
        {
          FIXME("Failed to load IDI_SHELL_DOCUMENT icon!\n");
          return FALSE;
        }

        SIC_IconAppend (swShell32Name, IDI_SHELL_DOCUMENT-1, hSm, hLg, 0);
        SIC_IconAppend (swShell32Name, -IDI_SHELL_DOCUMENT, hSm, hLg, 0);
   
	TRACE("hIconSmall=%p hIconBig=%p\n",ShellSmallIconList, ShellBigIconList);

	return TRUE;
}
/*************************************************************************
 * SIC_Destroy
 *
 * frees the cache
 */
static INT CALLBACK sic_free( LPVOID ptr, LPVOID lparam )
{
	HeapFree(GetProcessHeap(), 0, ((LPSIC_ENTRY)ptr)->sSourceFile);
	SHFree(ptr);
	return TRUE;
}

void SIC_Destroy(void)
{
	TRACE("\n");

	EnterCriticalSection(&SHELL32_SicCS);

	if (sic_hdpa) DPA_DestroyCallback(sic_hdpa, sic_free, NULL );

	if (ShellSmallIconList)
	    ImageList_Destroy(ShellSmallIconList);
	if (ShellBigIconList)
	    ImageList_Destroy(ShellBigIconList);

	LeaveCriticalSection(&SHELL32_SicCS);
	DeleteCriticalSection(&SHELL32_SicCS);
}

/*****************************************************************************
 * SIC_GetIconIndex			[internal]
 *
 * Parameters
 *	sSourceFile	[IN]	filename of file containing the icon
 *	index		[IN]	index/resID (negated) in this file
 *
 * NOTES
 *  look in the cache for a proper icon. if not available the icon is taken
 *  from the file and cached
 */
INT SIC_GetIconIndex (LPCWSTR sSourceFile, INT dwSourceIndex, DWORD dwFlags )
{
	SIC_ENTRY sice;
	INT ret, index = INVALID_INDEX;
	WCHAR path[MAX_PATH];

	TRACE("%s %i\n", debugstr_w(sSourceFile), dwSourceIndex);

	GetFullPathNameW(sSourceFile, MAX_PATH, path, NULL);
	sice.sSourceFile = path;
	sice.dwSourceIndex = dwSourceIndex;
	sice.dwFlags = dwFlags;

        //InitOnceExecuteOnce( &sic_init_once, SIC_Initialize, NULL, NULL );

	EnterCriticalSection(&SHELL32_SicCS);

	if (NULL != DPA_GetPtr (sic_hdpa, 0))
	{
	  /* search linear from position 0*/
	  index = DPA_Search (sic_hdpa, &sice, 0, SIC_CompareEntries, 0, 0);
	}

	if ( INVALID_INDEX == index )
	{
          ret = SIC_LoadIcon (sSourceFile, dwSourceIndex, dwFlags);
	}
	else
	{
	  TRACE("-- found\n");
	  ret = ((LPSIC_ENTRY)DPA_GetPtr(sic_hdpa, index))->dwListIndex;
	}

	LeaveCriticalSection(&SHELL32_SicCS);
	return ret;
}

/*****************************************************************************
 * SIC_LoadOverlayIcon			[internal]
 *
 * Load a shell overlay icon and return its icon cache index.
 */
static int SIC_LoadOverlayIcon(int icon_idx)
{
	WCHAR buffer[1024], wszIdx[8];
	HKEY hKeyShellIcons;
	LPCWSTR iconPath;
	int iconIdx;

	static const WCHAR wszShellIcons[] = {
	    'S','o','f','t','w','a','r','e','\\','M','i','c','r','o','s','o','f','t','\\',
	    'W','i','n','d','o','w','s','\\','C','u','r','r','e','n','t','V','e','r','s','i','o','n','\\',
	    'E','x','p','l','o','r','e','r','\\','S','h','e','l','l',' ','I','c','o','n','s',0
	}; 
	static const WCHAR wszNumFmt[] = {'%','d',0};

	iconPath = swShell32Name;	/* default: load icon from shell32.dll */
	iconIdx = icon_idx;

	if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, wszShellIcons, 0, KEY_READ, &hKeyShellIcons) == ERROR_SUCCESS)
	{
	    DWORD count = sizeof(buffer);

	    sprintfW(wszIdx, wszNumFmt, icon_idx);

	    /* read icon path and index */
	    if (RegQueryValueExW(hKeyShellIcons, wszIdx, NULL, NULL, (LPBYTE)buffer, &count) == ERROR_SUCCESS)
	    {
		LPWSTR p = strchrW(buffer, ',');

		if (!p)
		{
		    ERR("Icon index in %s/%s corrupted, no comma.\n", debugstr_w(wszShellIcons),debugstr_w(wszIdx));
		    RegCloseKey(hKeyShellIcons);
		    return -1;
		}
		*p++ = 0;
		iconPath = buffer;
		iconIdx = atoiW(p);
	    }

	    RegCloseKey(hKeyShellIcons);
	}

        //InitOnceExecuteOnce( &sic_init_once, SIC_Initialize, NULL, NULL );

	return SIC_LoadIcon(iconPath, iconIdx, 0);
}

/*************************************************************************
 * PidlToSicIndex			[INTERNAL]
 *
 * PARAMETERS
 *	sh	[IN]	IShellFolder
 *	pidl	[IN]
 *	bBigIcon [IN]
 *	uFlags	[IN]	GIL_*
 *	pIndex	[OUT]	index within the SIC
 *
 */
BOOL PidlToSicIndex (
	IShellFolder * sh,
	LPCITEMIDLIST pidl,
	BOOL bBigIcon,
	UINT uFlags,
	int * pIndex)
{
	IExtractIconW	*ei;
	WCHAR		szIconFile[MAX_PATH];	/* file containing the icon */
	INT		iSourceIndex;		/* index or resID(negated) in this file */
	BOOL		ret = FALSE;
	UINT		dwFlags = 0;
	int		iShortcutDefaultIndex = INVALID_INDEX;

	TRACE("sf=%p pidl=%p %s\n", sh, pidl, bBigIcon?"Big":"Small");

        //InitOnceExecuteOnce( &sic_init_once, SIC_Initialize, NULL, NULL );

	if (SUCCEEDED (IShellFolder_GetUIObjectOf(sh, 0, 1, &pidl, &IID_IExtractIconW, 0, (void **)&ei)))
	{
	  if (SUCCEEDED(IExtractIconW_GetIconLocation(ei, uFlags, szIconFile, MAX_PATH, &iSourceIndex, &dwFlags)))
	  {
	    *pIndex = SIC_GetIconIndex(szIconFile, iSourceIndex, uFlags);
	    ret = TRUE;
	  }
	  IExtractIconW_Release(ei);
	}

	if (INVALID_INDEX == *pIndex)	/* default icon when failed */
	{
	  if (0 == (uFlags & GIL_FORSHORTCUT))
	  {
	    *pIndex = 0;
	  }
	  else
	  {
	    if (INVALID_INDEX == iShortcutDefaultIndex)
	    {
	      iShortcutDefaultIndex = SIC_LoadIcon(swShell32Name, 0, GIL_FORSHORTCUT);
	    }
	    *pIndex = (INVALID_INDEX != iShortcutDefaultIndex ? iShortcutDefaultIndex : 0);
	  }
	}

	return ret;

}

/*************************************************************************
 * Shell_GetCachedImageIndexA        
 *
 */
INT WINAPI Shell_GetCachedImageIndexA(LPCSTR szPath, INT nIndex, UINT bSimulateDoc)
{
    INT ret, len;
    PCWSTR szTemp;

    WARN("(%s,%08x,%08x) semi-stub.\n",debugstr_a(szPath), nIndex, bSimulateDoc);

    len = MultiByteToWideChar( CP_ACP, 0, szPath, -1, NULL, 0 );
    szTemp = (LPWSTR)HeapAlloc( GetProcessHeap(), 0, len * sizeof(WCHAR) );
    MultiByteToWideChar( CP_ACP, 0, szPath, -1, szTemp, len );

    ret = Shell_GetCachedImageIndex ( szTemp, nIndex, bSimulateDoc );

    HeapFree( GetProcessHeap(), 0, szTemp );

    return ret;
}


/****************************************************************************
 * SHGetStockIconInfo [SHELL32.@]
 *
 * Receive information for builtin icons
 *
 * PARAMS
 *  id      [I]  selected icon-id to get information for
 *  flags   [I]  selects the information to receive
 *  sii     [IO] SHSTOCKICONINFO structure to fill
 *
 * RETURNS
 *  Success: S_OK
 *  Failure: A HRESULT failure code
 *
 */
HRESULT WINAPI SHGetStockIconInfo(SHSTOCKICONID id, UINT flags, SHSTOCKICONINFO *sii)
{
    static const WCHAR shell32dll[] = {'\\','s','h','e','l','l','3','2','.','d','l','l',0};

    FIXME("(%d, 0x%x, %p) semi-stub\n", id, flags, sii);
    if ((id < 0) || (id >= SIID_MAX_ICONS) || !sii || (sii->cbSize != sizeof(SHSTOCKICONINFO))) {
        return E_INVALIDARG;
    }

    GetSystemDirectoryW(sii->szPath, MAX_PATH);

    /* no icons defined: use default */
    sii->iIcon = -IDI_SHELL_DOCUMENT;
    lstrcatW(sii->szPath, shell32dll);

    if (flags)
        FIXME("flags 0x%x not implemented\n", flags);

    sii->hIcon = NULL;
    sii->iSysImageIndex = -1;

    TRACE("%3d: returning %s (%d)\n", id, debugstr_w(sii->szPath), sii->iIcon);

    return S_OK;
}
