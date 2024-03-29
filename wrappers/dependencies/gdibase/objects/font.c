/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/gdi32/object/font.c
 * PURPOSE:
 * PROGRAMMER:
 *
 */

#include <precomp.h>

#define NDEBUG
#include <debug.h>

/*
 *  For TranslateCharsetInfo
 */
#define MAXTCIINDEX 32
static const CHARSETINFO FONT_tci[MAXTCIINDEX] =
{
    /* ANSI */
    { ANSI_CHARSET, 1252, {{0,0,0,0},{FS_LATIN1,0}} },
    { EASTEUROPE_CHARSET, 1250, {{0,0,0,0},{FS_LATIN2,0}} },
    { RUSSIAN_CHARSET, 1251, {{0,0,0,0},{FS_CYRILLIC,0}} },
    { GREEK_CHARSET, 1253, {{0,0,0,0},{FS_GREEK,0}} },
    { TURKISH_CHARSET, 1254, {{0,0,0,0},{FS_TURKISH,0}} },
    { HEBREW_CHARSET, 1255, {{0,0,0,0},{FS_HEBREW,0}} },
    { ARABIC_CHARSET, 1256, {{0,0,0,0},{FS_ARABIC,0}} },
    { BALTIC_CHARSET, 1257, {{0,0,0,0},{FS_BALTIC,0}} },
    { VIETNAMESE_CHARSET, 1258, {{0,0,0,0},{FS_VIETNAMESE,0}} },
    /* reserved by ANSI */
    { DEFAULT_CHARSET, 0, {{0,0,0,0},{FS_LATIN1,0}} },
    { DEFAULT_CHARSET, 0, {{0,0,0,0},{FS_LATIN1,0}} },
    { DEFAULT_CHARSET, 0, {{0,0,0,0},{FS_LATIN1,0}} },
    { DEFAULT_CHARSET, 0, {{0,0,0,0},{FS_LATIN1,0}} },
    { DEFAULT_CHARSET, 0, {{0,0,0,0},{FS_LATIN1,0}} },
    { DEFAULT_CHARSET, 0, {{0,0,0,0},{FS_LATIN1,0}} },
    { DEFAULT_CHARSET, 0, {{0,0,0,0},{FS_LATIN1,0}} },
    /* ANSI and OEM */
    { THAI_CHARSET, 874, {{0,0,0,0},{FS_THAI,0}} },
    { SHIFTJIS_CHARSET, 932, {{0,0,0,0},{FS_JISJAPAN,0}} },
    { GB2312_CHARSET, 936, {{0,0,0,0},{FS_CHINESESIMP,0}} },
    { HANGEUL_CHARSET, 949, {{0,0,0,0},{FS_WANSUNG,0}} },
    { CHINESEBIG5_CHARSET, 950, {{0,0,0,0},{FS_CHINESETRAD,0}} },
    { JOHAB_CHARSET, 1361, {{0,0,0,0},{FS_JOHAB,0}} },
    /* reserved for alternate ANSI and OEM */
    { DEFAULT_CHARSET, 0, {{0,0,0,0},{FS_LATIN1,0}} },
    { DEFAULT_CHARSET, 0, {{0,0,0,0},{FS_LATIN1,0}} },
    { DEFAULT_CHARSET, 0, {{0,0,0,0},{FS_LATIN1,0}} },
    { DEFAULT_CHARSET, 0, {{0,0,0,0},{FS_LATIN1,0}} },
    { DEFAULT_CHARSET, 0, {{0,0,0,0},{FS_LATIN1,0}} },
    { DEFAULT_CHARSET, 0, {{0,0,0,0},{FS_LATIN1,0}} },
    { DEFAULT_CHARSET, 0, {{0,0,0,0},{FS_LATIN1,0}} },
    { DEFAULT_CHARSET, 0, {{0,0,0,0},{FS_LATIN1,0}} },
    /* reserved for system */
    { DEFAULT_CHARSET, 0, {{0,0,0,0},{FS_LATIN1,0}} },
    { SYMBOL_CHARSET, CP_SYMBOL, {{0,0,0,0},{FS_SYMBOL,0}} }
};

#define INITIAL_FAMILY_COUNT 64

/***********************************************************************
 *              TEXTMETRIC conversion functions.
 */
VOID
FASTCALL
FONT_TextMetricWToA(const TEXTMETRICW *ptmW, LPTEXTMETRICA ptmA )
{
    ptmA->tmHeight = ptmW->tmHeight;
    ptmA->tmAscent = ptmW->tmAscent;
    ptmA->tmDescent = ptmW->tmDescent;
    ptmA->tmInternalLeading = ptmW->tmInternalLeading;
    ptmA->tmExternalLeading = ptmW->tmExternalLeading;
    ptmA->tmAveCharWidth = ptmW->tmAveCharWidth;
    ptmA->tmMaxCharWidth = ptmW->tmMaxCharWidth;
    ptmA->tmWeight = ptmW->tmWeight;
    ptmA->tmOverhang = ptmW->tmOverhang;
    ptmA->tmDigitizedAspectX = ptmW->tmDigitizedAspectX;
    ptmA->tmDigitizedAspectY = ptmW->tmDigitizedAspectY;
    ptmA->tmFirstChar = min(ptmW->tmFirstChar, 255);
    if (ptmW->tmCharSet == SYMBOL_CHARSET)
    {
        ptmA->tmFirstChar = 0x1e;
        ptmA->tmLastChar = 0xff;  /* win9x behaviour - we need the OS2 table data to calculate correctly */
    }
    else
    {
        ptmA->tmFirstChar = ptmW->tmDefaultChar - 1;
        ptmA->tmLastChar = min(ptmW->tmLastChar, 0xff);
    }
    ptmA->tmDefaultChar = (CHAR)ptmW->tmDefaultChar;
    ptmA->tmBreakChar = (CHAR)ptmW->tmBreakChar;
    ptmA->tmItalic = ptmW->tmItalic;
    ptmA->tmUnderlined = ptmW->tmUnderlined;
    ptmA->tmStruckOut = ptmW->tmStruckOut;
    ptmA->tmPitchAndFamily = ptmW->tmPitchAndFamily;
    ptmA->tmCharSet = ptmW->tmCharSet;
}

/***********************************************************************
 *           FONT_mbtowc
 *
 * Returns a Unicode translation of str using the charset of the
 * currently selected font in hdc.  If count is -1 then str is assumed
 * to be '\0' terminated, otherwise it contains the number of bytes to
 * convert.  If plenW is non-NULL, on return it will point to the
 * number of WCHARs that have been written.  If pCP is non-NULL, on
 * return it will point to the codepage used in the conversion.  The
 * caller should free the returned LPWSTR from the process heap
 * itself.
 */
static LPWSTR FONT_mbtowc(HDC hdc, LPCSTR str, INT count, INT *plenW, UINT *pCP)
{
    UINT cp = GdiGetCodePage( hdc );
    INT lenW;
    LPWSTR strW;

    if(count == -1) count = strlen(str);
    lenW = MultiByteToWideChar(cp, 0, str, count, NULL, 0);
    strW = HeapAlloc(GetProcessHeap(), 0, lenW*sizeof(WCHAR));
    if (!strW)
        return NULL;
    MultiByteToWideChar(cp, 0, str, count, strW, lenW);
    DPRINT("mapped %s -> %S\n", str, strW);
    if(plenW) *plenW = lenW;
    if(pCP) *pCP = cp;
    return strW;
}

static LPSTR FONT_GetCharsByRangeA(HDC hdc, UINT firstChar, UINT lastChar, PINT pByteLen)
{
    INT i, count = lastChar - firstChar + 1;
    UINT c;
    LPSTR str;

    if (count <= 0)
        return NULL;

    switch (GdiGetCodePage(hdc))
    {
    case 932:
    case 936:
    case 949:
    case 950:
    case 1361:
        if (lastChar > 0xffff)
            return NULL;
        if ((firstChar ^ lastChar) > 0xff)
            return NULL;
        break;
    default:
        if (lastChar > 0xff)
            return NULL;
        break;
    }

    str = HeapAlloc(GetProcessHeap(), 0, count * 2 + 1);
    if (str == NULL)
        return NULL;

    for(i = 0, c = firstChar; c <= lastChar; i++, c++)
    {
        if (c > 0xff)
            str[i++] = (BYTE)(c >> 8);
        str[i] = (BYTE)c;
    }
    str[i] = '\0';

    *pByteLen = i;

    return str;
}

VOID FASTCALL
NewTextMetricW2A(NEWTEXTMETRICA *tma, NEWTEXTMETRICW *tmw)
{
    FONT_TextMetricWToA((TEXTMETRICW *) tmw, (TEXTMETRICA *) tma);
    tma->ntmFlags = tmw->ntmFlags;
    tma->ntmSizeEM = tmw->ntmSizeEM;
    tma->ntmCellHeight = tmw->ntmCellHeight;
    tma->ntmAvgWidth = tmw->ntmAvgWidth;
}

VOID FASTCALL
NewTextMetricExW2A(NEWTEXTMETRICEXA *tma, NEWTEXTMETRICEXW *tmw)
{
    NewTextMetricW2A(&tma->ntmTm, &tmw->ntmTm);
    tma->ntmFontSig = tmw->ntmFontSig;
}

static int FASTCALL
IntEnumFontFamilies(HDC Dc, LPLOGFONTW LogFont, PVOID EnumProc, LPARAM lParam,
                    BOOL Unicode)
{
    int FontFamilyCount;
    int FontFamilySize;
    PFONTFAMILYINFO Info;
    int Ret = 0;
    int i;
    ENUMLOGFONTEXA EnumLogFontExA;
    NEWTEXTMETRICEXA NewTextMetricExA;
    LOGFONTW lfW;

    Info = RtlAllocateHeap(GetProcessHeap(), 0,
                           INITIAL_FAMILY_COUNT * sizeof(FONTFAMILYINFO));
    if (NULL == Info)
    {
        return 0;
    }

    if (!LogFont)
    {
        lfW.lfCharSet = DEFAULT_CHARSET;
        lfW.lfPitchAndFamily = 0;
        lfW.lfFaceName[0] = 0;
        LogFont = &lfW;
    }

    FontFamilyCount = NtGdiGetFontFamilyInfo(Dc, LogFont, Info, (LPLONG)INITIAL_FAMILY_COUNT);
    if (FontFamilyCount < 0)
    {
        RtlFreeHeap(GetProcessHeap(), 0, Info);
        return 0;
    }
    if (INITIAL_FAMILY_COUNT < FontFamilyCount)
    {
        FontFamilySize = FontFamilyCount;
        RtlFreeHeap(GetProcessHeap(), 0, Info);
        Info = RtlAllocateHeap(GetProcessHeap(), 0,
                               FontFamilyCount * sizeof(FONTFAMILYINFO));
        if (NULL == Info)
        {
            return 0;
        }
        FontFamilyCount = NtGdiGetFontFamilyInfo(Dc, LogFont, Info, (LPLONG)FontFamilySize);
        if (FontFamilyCount < 0 || FontFamilySize < FontFamilyCount)
        {
            RtlFreeHeap(GetProcessHeap(), 0, Info);
            return 0;
        }
    }

    for (i = 0; i < FontFamilyCount; i++)
    {
        if (Unicode)
        {
            Ret = ((FONTENUMPROCW) EnumProc)(
                      (VOID*)&Info[i].EnumLogFontEx,
                      (VOID*)&Info[i].NewTextMetricEx,
                      Info[i].FontType, lParam);
        }
        else
        {
            // Could use EnumLogFontExW2A here?
            LogFontW2A(&EnumLogFontExA.elfLogFont, &Info[i].EnumLogFontEx.elfLogFont);
            WideCharToMultiByte(CP_THREAD_ACP, 0, Info[i].EnumLogFontEx.elfFullName, -1,
                                (LPSTR)EnumLogFontExA.elfFullName, LF_FULLFACESIZE, NULL, NULL);
            WideCharToMultiByte(CP_THREAD_ACP, 0, Info[i].EnumLogFontEx.elfStyle, -1,
                                (LPSTR)EnumLogFontExA.elfStyle, LF_FACESIZE, NULL, NULL);
            WideCharToMultiByte(CP_THREAD_ACP, 0, Info[i].EnumLogFontEx.elfScript, -1,
                                (LPSTR)EnumLogFontExA.elfScript, LF_FACESIZE, NULL, NULL);
            NewTextMetricExW2A(&NewTextMetricExA,
                               &Info[i].NewTextMetricEx);
            Ret = ((FONTENUMPROCA) EnumProc)(
                      (VOID*)&EnumLogFontExA,
                      (VOID*)&NewTextMetricExA,
                      Info[i].FontType, lParam);
        }
    }

    RtlFreeHeap(GetProcessHeap(), 0, Info);

    return Ret;
}

/*
 * @implemented
 */
int WINAPI
EnumFontFamiliesExW(HDC hdc, LPLOGFONTW lpLogfont, FONTENUMPROCW lpEnumFontFamExProc,
                    LPARAM lParam, DWORD dwFlags)
{
    return IntEnumFontFamilies(hdc, lpLogfont, lpEnumFontFamExProc, lParam, TRUE);
}


/*
 * @implemented
 */
int WINAPI
EnumFontFamiliesW(HDC hdc, LPCWSTR lpszFamily, FONTENUMPROCW lpEnumFontFamProc,
                  LPARAM lParam)
{
    LOGFONTW LogFont;

    ZeroMemory(&LogFont, sizeof(LOGFONTW));
    LogFont.lfCharSet = DEFAULT_CHARSET;
    if (NULL != lpszFamily)
    {
        if (!*lpszFamily) return 1;
        lstrcpynW(LogFont.lfFaceName, lpszFamily, LF_FACESIZE);
    }

    return IntEnumFontFamilies(hdc, &LogFont, lpEnumFontFamProc, lParam, TRUE);
}


/*
 * @implemented
 */
int WINAPI
EnumFontFamiliesExA (HDC hdc, LPLOGFONTA lpLogfont, FONTENUMPROCA lpEnumFontFamExProc,
                     LPARAM lParam, DWORD dwFlags)
{
    LOGFONTW LogFontW, *pLogFontW;

    if (lpLogfont)
    {
        LogFontA2W(&LogFontW,lpLogfont);
        pLogFontW = &LogFontW;
    }
    else pLogFontW = NULL;

    /* no need to convert LogFontW back to lpLogFont b/c it's an [in] parameter only */
    return IntEnumFontFamilies(hdc, pLogFontW, lpEnumFontFamExProc, lParam, FALSE);
}


/*
 * @implemented
 */
int WINAPI
EnumFontFamiliesA(HDC hdc, LPCSTR lpszFamily, FONTENUMPROCA lpEnumFontFamProc,
                  LPARAM lParam)
{
    LOGFONTW LogFont;

    ZeroMemory(&LogFont, sizeof(LOGFONTW));
    LogFont.lfCharSet = DEFAULT_CHARSET;
    if (NULL != lpszFamily)
    {
        if (!*lpszFamily) return 1;
        MultiByteToWideChar(CP_THREAD_ACP, 0, lpszFamily, -1, LogFont.lfFaceName, LF_FACESIZE);
    }

    return IntEnumFontFamilies(hdc, &LogFont, lpEnumFontFamProc, lParam, FALSE);
}


/*
 * @implemented
 */
DWORD
WINAPI
GetCharacterPlacementA(
    HDC hdc,
    LPCSTR lpString,
    INT uCount,
    INT nMaxExtent,
    GCP_RESULTSA *lpResults,
    DWORD dwFlags)
{
    WCHAR *lpStringW;
    INT uCountW;
    GCP_RESULTSW resultsW;
    DWORD ret;
    UINT font_cp;

    if ( !lpString || uCount <= 0 || (nMaxExtent < 0 && nMaxExtent != -1 ) )
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }
    /*    TRACE("%s, %d, %d, 0x%08x\n",
              debugstr_an(lpString, uCount), uCount, nMaxExtent, dwFlags);
    */
    /* both structs are equal in size */
    memcpy(&resultsW, lpResults, sizeof(resultsW));

    lpStringW = FONT_mbtowc(hdc, lpString, uCount, &uCountW, &font_cp);
    if (lpStringW == NULL)
    {
        return 0;
    }
    if(lpResults->lpOutString)
    {
        resultsW.lpOutString = HeapAlloc(GetProcessHeap(), 0, sizeof(WCHAR)*uCountW);
        if (resultsW.lpOutString == NULL)
        {
            HeapFree(GetProcessHeap(), 0, lpStringW);
            return 0;
        }
    }

    ret = GetCharacterPlacementW(hdc, lpStringW, uCountW, nMaxExtent, &resultsW, dwFlags);

    lpResults->nGlyphs = resultsW.nGlyphs;
    lpResults->nMaxFit = resultsW.nMaxFit;

    if(lpResults->lpOutString)
    {
        WideCharToMultiByte(font_cp, 0, resultsW.lpOutString, uCountW,
                            lpResults->lpOutString, uCount, NULL, NULL );
    }

    HeapFree(GetProcessHeap(), 0, lpStringW);
    HeapFree(GetProcessHeap(), 0, resultsW.lpOutString);

    return ret;
}


/*
 * @implemented
 */
DWORD
WINAPI
GetCharacterPlacementW(
    HDC hdc,
    LPCWSTR lpString,
    INT uCount,
    INT nMaxExtent,
    GCP_RESULTSW *lpResults,
    DWORD dwFlags
)
{
    DWORD ret=0;
    SIZE size;
    UINT i, nSet;
    DPRINT("GetCharacterPlacementW\n");

    if(dwFlags&(~GCP_REORDER)) DPRINT("flags 0x%08lx ignored\n", dwFlags);
    if(lpResults->lpClass) DPRINT("classes not implemented\n");
    if (lpResults->lpCaretPos && (dwFlags & GCP_REORDER))
        DPRINT("Caret positions for complex scripts not implemented\n");

    nSet = (UINT)uCount;
    if(nSet > lpResults->nGlyphs)
        nSet = lpResults->nGlyphs;

    /* return number of initialized fields */
    lpResults->nGlyphs = nSet;

    /*if((dwFlags&GCP_REORDER)==0 || !BidiAvail)
      {*/
    /* Treat the case where no special handling was requested in a fastpath way */
    /* copy will do if the GCP_REORDER flag is not set */
    if(lpResults->lpOutString)
        lstrcpynW( lpResults->lpOutString, lpString, nSet );

    if(lpResults->lpGlyphs)
        lstrcpynW( lpResults->lpGlyphs, lpString, nSet );

    if(lpResults->lpOrder)
    {
        for(i = 0; i < nSet; i++)
            lpResults->lpOrder[i] = i;
    }
    /*} else
      {
          BIDI_Reorder( lpString, uCount, dwFlags, WINE_GCPW_FORCE_LTR, lpResults->lpOutString,
                        nSet, lpResults->lpOrder );
      }*/

    /* FIXME: Will use the placement chars */
    if (lpResults->lpDx)
    {
        int c;
        for (i = 0; i < nSet; i++)
        {
            if (GetCharWidth32W(hdc, lpString[i], lpString[i], &c))
                lpResults->lpDx[i]= c;
        }
    }

    if (lpResults->lpCaretPos && !(dwFlags & GCP_REORDER))
    {
        int pos = 0;

        lpResults->lpCaretPos[0] = 0;
        for (i = 1; i < nSet; i++)
            if (GetTextExtentPoint32W(hdc, &(lpString[i - 1]), 1, &size))
                lpResults->lpCaretPos[i] = (pos += size.cx);
    }

    /*if(lpResults->lpGlyphs)
      NtGdiGetGlyphIndicesW(hdc, lpString, nSet, lpResults->lpGlyphs, 0);*/

    if (GetTextExtentPoint32W(hdc, lpString, uCount, &size))
        ret = MAKELONG(size.cx, size.cy);

    return ret;
}

DWORD
WINAPI
NewGetCharacterPlacementW(
    HDC hdc,
    LPCWSTR lpString,
    INT uCount,
    INT nMaxExtent,
    GCP_RESULTSW *lpResults,
    DWORD dwFlags
)
{
    ULONG nSet;
    SIZE Size = {0,0};

    if ( !lpString || uCount <= 0 || (nMaxExtent < 0 && nMaxExtent != -1 ) )
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    if ( !lpResults )
    {
        if ( GetTextExtentPointW(hdc, lpString, uCount, &Size) )
        {
            return MAKELONG(Size.cx, Size.cy);
        }
        return 0;
    }

    nSet = uCount;
    if ( nSet > lpResults->nGlyphs )
        nSet = lpResults->nGlyphs;

    return NtGdiGetCharacterPlacementW( hdc,
                                        (LPWSTR)lpString,
                                        nSet,
                                        nMaxExtent,
                                        lpResults,
                                        dwFlags);
}

/*
 * @implemented
 *
 */
BOOL
WINAPI
GetCharABCWidthsFloatW(HDC hdc,
                       UINT FirstChar,
                       UINT LastChar,
                       LPABCFLOAT abcF)
{
    DPRINT("GetCharABCWidthsFloatW\n");
    if ((!abcF) || (FirstChar > LastChar))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    return NtGdiGetCharABCWidthsW( hdc,
                                   FirstChar,
                                   (ULONG)(LastChar - FirstChar + 1),
                                   (PWCHAR) NULL,
                                   0,
                                   (PVOID)abcF);
}

/*
 * @implemented
 *
 */
BOOL
WINAPI
GetCharWidthFloatW(HDC hdc,
                   UINT iFirstChar,
                   UINT iLastChar,
                   PFLOAT pxBuffer)
{
    DPRINT("GetCharWidthsFloatW\n");
    if ((!pxBuffer) || (iFirstChar > iLastChar))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    return NtGdiGetCharWidthW( hdc,
                               iFirstChar,
                               (ULONG)(iLastChar - iFirstChar + 1),
                               (PWCHAR) NULL,
                               0,
                               (PVOID) pxBuffer);
}

/*
 * @implemented
 *
 */
BOOL
WINAPI
GetCharWidthW(HDC hdc,
              UINT iFirstChar,
              UINT iLastChar,
              LPINT lpBuffer)
{
    DPRINT("GetCharWidthsW\n");
    if ((!lpBuffer) || (iFirstChar > iLastChar))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    return NtGdiGetCharWidthW( hdc,
                               iFirstChar,
                               (ULONG)(iLastChar - iFirstChar + 1),
                               (PWCHAR) NULL,
                               GCW_NOFLOAT,
                               (PVOID) lpBuffer);
}

/*
 * @implemented
 *
 */
BOOL
WINAPI
GetCharWidth32W(HDC hdc,
                UINT iFirstChar,
                UINT iLastChar,
                LPINT lpBuffer)
{
    DPRINT("GetCharWidths32W\n");
    if ((!lpBuffer) || (iFirstChar > iLastChar))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    return NtGdiGetCharWidthW( hdc,
                               iFirstChar,
                               (ULONG)(iLastChar - iFirstChar + 1),
                               (PWCHAR) NULL,
                               GCW_NOFLOAT|GCW_WIN32,
                               (PVOID) lpBuffer);
}


/*
 * @implemented
 *
 */
BOOL
WINAPI
GetCharABCWidthsW(HDC hdc,
                  UINT FirstChar,
                  UINT LastChar,
                  LPABC lpabc)
{
    DPRINT("GetCharABCWidthsW\n");
    if ((!lpabc) || (FirstChar > LastChar))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    return NtGdiGetCharABCWidthsW( hdc,
                                   FirstChar,
                                   (ULONG)(LastChar - FirstChar + 1),
                                   (PWCHAR) NULL,
                                   GCABCW_NOFLOAT,
                                   (PVOID)lpabc);
}

/*
 * @implemented
 */
BOOL
WINAPI
GetCharWidthA(
    HDC	hdc,
    UINT	iFirstChar,
    UINT	iLastChar,
    LPINT	lpBuffer
)
{
    INT wlen, count = 0;
    LPSTR str;
    LPWSTR wstr;
    BOOL ret = TRUE;

    DPRINT("GetCharWidthsA\n");

    str = FONT_GetCharsByRangeA(hdc, iFirstChar, iLastChar, &count);
    if (!str)
        return FALSE;

    wstr = FONT_mbtowc(hdc, str, count+1, &wlen, NULL);
    if (!wstr)
    {
        HeapFree(GetProcessHeap(), 0, str);
        return FALSE;
    }

    ret = NtGdiGetCharWidthW( hdc,
                              wstr[0],
                              (ULONG) count,
                              (PWCHAR) wstr,
                              GCW_NOFLOAT,
                              (PVOID) lpBuffer);

    HeapFree(GetProcessHeap(), 0, str);
    HeapFree(GetProcessHeap(), 0, wstr);

    return ret;
}

/*
 * @implemented
 */
BOOL
WINAPI
GetCharWidth32A(
    HDC	hdc,
    UINT	iFirstChar,
    UINT	iLastChar,
    LPINT	lpBuffer
)
{
    INT wlen, count = 0;
    LPSTR str;
    LPWSTR wstr;
    BOOL ret = TRUE;

    DPRINT("GetCharWidths32A\n");

    str = FONT_GetCharsByRangeA(hdc, iFirstChar, iLastChar, &count);
    if (!str)
        return FALSE;

    wstr = FONT_mbtowc(hdc, str, count+1, &wlen, NULL);
    if (!wstr)
    {
        HeapFree(GetProcessHeap(), 0, str);
        return FALSE;
    }

    ret = NtGdiGetCharWidthW( hdc,
                              wstr[0],
                              (ULONG) count,
                              (PWCHAR) wstr,
                              GCW_NOFLOAT|GCW_WIN32,
                              (PVOID) lpBuffer);

    HeapFree(GetProcessHeap(), 0, str);
    HeapFree(GetProcessHeap(), 0, wstr);

    return ret;
}

/*
 * @implemented
 */
BOOL
APIENTRY
GetCharWidthFloatA(
    HDC	hdc,
    UINT	iFirstChar,
    UINT	iLastChar,
    PFLOAT	pxBuffer
)
{
    INT wlen, count = 0;
    LPSTR str;
    LPWSTR wstr;
    BOOL ret = TRUE;

    DPRINT("GetCharWidthsFloatA\n");

    str = FONT_GetCharsByRangeA(hdc, iFirstChar, iLastChar, &count);
    if (!str)
        return FALSE;

    wstr = FONT_mbtowc(hdc, str, count+1, &wlen, NULL);
    if (!wstr)
    {
        HeapFree(GetProcessHeap(), 0, str);
        return FALSE;
    }
    ret = NtGdiGetCharWidthW( hdc, wstr[0], (ULONG) count, (PWCHAR) wstr, 0, (PVOID) pxBuffer);

    HeapFree(GetProcessHeap(), 0, str);
    HeapFree(GetProcessHeap(), 0, wstr);

    return ret;
}

/*
 * @implemented
 */
BOOL
APIENTRY
GetCharABCWidthsA(
    HDC	hdc,
    UINT	iFirstChar,
    UINT	iLastChar,
    LPABC	lpabc
)
{
    INT wlen, count = 0;
    LPSTR str;
    LPWSTR wstr;
    BOOL ret = TRUE;

    DPRINT("GetCharABCWidthsA\n");

    str = FONT_GetCharsByRangeA(hdc, iFirstChar, iLastChar, &count);
    if (!str)
        return FALSE;

    wstr = FONT_mbtowc(hdc, str, count+1, &wlen, NULL);
    if (!wstr)
    {
        HeapFree(GetProcessHeap(), 0, str);
        return FALSE;
    }

    ret = NtGdiGetCharABCWidthsW( hdc,
                                  wstr[0],
                                  (ULONG)count,
                                  (PWCHAR)wstr,
                                  GCABCW_NOFLOAT,
                                  (PVOID)lpabc);

    HeapFree(GetProcessHeap(), 0, str);
    HeapFree(GetProcessHeap(), 0, wstr);

    return ret;
}

/*
 * @implemented
 */
BOOL
APIENTRY
GetCharABCWidthsFloatA(
    HDC		hdc,
    UINT		iFirstChar,
    UINT		iLastChar,
    LPABCFLOAT	lpABCF
)
{
    INT wlen, count = 0;
    LPSTR str;
    LPWSTR wstr;
    BOOL ret = TRUE;

    DPRINT("GetCharABCWidthsFloatA\n");

    str = FONT_GetCharsByRangeA(hdc, iFirstChar, iLastChar, &count);
    if (!str)
        return FALSE;

    wstr = FONT_mbtowc( hdc, str, count+1, &wlen, NULL );
    if (!wstr)
    {
        HeapFree( GetProcessHeap(), 0, str );
        return FALSE;
    }
    ret = NtGdiGetCharABCWidthsW( hdc,wstr[0],(ULONG)count, (PWCHAR)wstr, 0, (PVOID)lpABCF);

    HeapFree( GetProcessHeap(), 0, str );
    HeapFree( GetProcessHeap(), 0, wstr );

    return ret;
}

/*
 * @implemented
 */
BOOL
WINAPI
GetCharABCWidthsI(HDC hdc,
                  UINT giFirst,
                  UINT cgi,
                  LPWORD pgi,
                  LPABC lpabc)
{
    DPRINT("GetCharABCWidthsI\n");
    return NtGdiGetCharABCWidthsW( hdc,
                                   giFirst,
                                   (ULONG) cgi,
                                   (PWCHAR) pgi,
                                   GCABCW_NOFLOAT|GCABCW_INDICES,
                                   (PVOID)lpabc);
}

/*
 * @implemented
 */
BOOL
WINAPI
GetCharWidthI(HDC hdc,
              UINT giFirst,
              UINT cgi,
              LPWORD pgi,
              LPINT lpBuffer
             )
{
    DPRINT("GetCharWidthsI\n");
    if (!lpBuffer || (!pgi && (giFirst == MAXUSHORT))) // Cannot be at max.
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    if (!cgi) return TRUE;
    return NtGdiGetCharWidthW( hdc,
                               giFirst,
                               cgi,
                               (PWCHAR) pgi,
                               GCW_INDICES|GCW_NOFLOAT|GCW_WIN32,
                               (PVOID) lpBuffer );
}

/*
 * @implemented
 */
DWORD
WINAPI
GetFontLanguageInfo(
    HDC 	hDc
)
{
    DWORD Gcp = 0, Ret = 0;
    if (gbLpk)
    {
        Ret = NtGdiGetTextCharsetInfo(hDc, NULL, 0);
        if ((Ret == ARABIC_CHARSET) || (Ret == HEBREW_CHARSET))
            Ret = (GCP_KASHIDA|GCP_DIACRITIC|GCP_LIGATE|GCP_GLYPHSHAPE|GCP_REORDER);
    }
    Gcp = GetDCDWord(hDc, GdiGetFontLanguageInfo, GCP_ERROR);
    if ( Gcp == GCP_ERROR)
        return Gcp;
    else
        Ret = Gcp | Ret;
    return Ret;
}

/*
 * @implemented
 */
DWORD
WINAPI
GetGlyphIndicesA(
    HDC hdc,
    LPCSTR lpstr,
    INT count,
    LPWORD pgi,
    DWORD flags
)
{
    DWORD Ret;
    WCHAR *lpstrW;
    INT countW;

    lpstrW = FONT_mbtowc(hdc, lpstr, count, &countW, NULL);

    if (lpstrW == NULL)
        return GDI_ERROR;

    Ret = NtGdiGetGlyphIndicesW(hdc, lpstrW, countW, pgi, flags);
    HeapFree(GetProcessHeap(), 0, lpstrW);
    return Ret;
}

/*
 * @implemented
 */
DWORD
WINAPI
GetGlyphOutlineA(
    HDC		hdc,
    UINT		uChar,
    UINT		uFormat,
    LPGLYPHMETRICS	lpgm,
    DWORD		cbBuffer,
    LPVOID		lpvBuffer,
    CONST MAT2	*lpmat2
)
{

    LPWSTR p = NULL;
    DWORD ret;
    UINT c;
    DPRINT("GetGlyphOutlineA uChar %x\n", uChar);
    if (!lpgm || !lpmat2) return GDI_ERROR;
    if(!(uFormat & GGO_GLYPH_INDEX))
    {
        int len;
        char mbchs[2];
        if(uChar > 0xff)   /* but, 2 bytes character only */
        {
            len = 2;
            mbchs[0] = (uChar & 0xff00) >> 8;
            mbchs[1] = (uChar & 0xff);
        }
        else
        {
            len = 1;
            mbchs[0] = (uChar & 0xff);
        }
        p = FONT_mbtowc(hdc, mbchs, len, NULL, NULL);
        c = p[0];
    }
    else
        c = uChar;
    ret = NtGdiGetGlyphOutline(hdc, c, uFormat, lpgm, cbBuffer, lpvBuffer, (CONST LPMAT2)lpmat2, TRUE);
    HeapFree(GetProcessHeap(), 0, p);
    return ret;
}


/*
 * @implemented
 */
DWORD
WINAPI
GetGlyphOutlineW(
    HDC		hdc,
    UINT		uChar,
    UINT		uFormat,
    LPGLYPHMETRICS	lpgm,
    DWORD		cbBuffer,
    LPVOID		lpvBuffer,
    CONST MAT2	*lpmat2
)
{
    DPRINT("GetGlyphOutlineW uChar %x\n", uChar);
    if (!lpgm || !lpmat2) return GDI_ERROR;
    if (!lpvBuffer) cbBuffer = 0;
    return NtGdiGetGlyphOutline ( hdc, uChar, uFormat, lpgm, cbBuffer, lpvBuffer, (CONST LPMAT2)lpmat2, TRUE);
}


/*
 * @implemented
 */
UINT
APIENTRY
GetOutlineTextMetricsA(
    HDC			hdc,
    UINT			cbData,
    LPOUTLINETEXTMETRICA	lpOTM
)
{
    char buf[512], *ptr;
    UINT ret, needed;
    OUTLINETEXTMETRICW *lpOTMW = (OUTLINETEXTMETRICW *)buf;
    OUTLINETEXTMETRICA *output = lpOTM;
    INT left, len;

    if((ret = GetOutlineTextMetricsW(hdc, 0, NULL)) == 0)
        return 0;
    if(ret > sizeof(buf))
    {
        lpOTMW = HeapAlloc(GetProcessHeap(), 0, ret);
        if (lpOTMW == NULL)
        {
            return 0;
        }
    }
    GetOutlineTextMetricsW(hdc, ret, lpOTMW);

    needed = sizeof(OUTLINETEXTMETRICA);
    if(lpOTMW->otmpFamilyName)
        needed += WideCharToMultiByte(CP_ACP, 0,
                                      (WCHAR*)((char*)lpOTMW + (int)lpOTMW->otmpFamilyName), -1,
                                      NULL, 0, NULL, NULL);
    if(lpOTMW->otmpFaceName)
        needed += WideCharToMultiByte(CP_ACP, 0,
                                      (WCHAR*)((char*)lpOTMW + (int)lpOTMW->otmpFaceName), -1,
                                      NULL, 0, NULL, NULL);
    if(lpOTMW->otmpStyleName)
        needed += WideCharToMultiByte(CP_ACP, 0,
                                      (WCHAR*)((char*)lpOTMW + (int)lpOTMW->otmpStyleName), -1,
                                      NULL, 0, NULL, NULL);
    if(lpOTMW->otmpFullName)
        needed += WideCharToMultiByte(CP_ACP, 0,
                                      (WCHAR*)((char*)lpOTMW + (int)lpOTMW->otmpFullName), -1,
                                      NULL, 0, NULL, NULL);

    if(!lpOTM)
    {
        ret = needed;
        goto end;
    }

    DPRINT("needed = %u\n", needed);
    if(needed > cbData)
    {
        /* Since the supplied buffer isn't big enough, we'll alloc one
           that is and memcpy the first cbData bytes into the lpOTM at
           the end. */
        output = HeapAlloc(GetProcessHeap(), 0, needed);
        if (output == NULL)
        {
            goto end;
        }
    }

    ret = output->otmSize = min(needed, cbData);
    FONT_TextMetricWToA( &lpOTMW->otmTextMetrics, &output->otmTextMetrics );
    output->otmFiller = 0;
    output->otmPanoseNumber = lpOTMW->otmPanoseNumber;
    output->otmfsSelection = lpOTMW->otmfsSelection;
    output->otmfsType = lpOTMW->otmfsType;
    output->otmsCharSlopeRise = lpOTMW->otmsCharSlopeRise;
    output->otmsCharSlopeRun = lpOTMW->otmsCharSlopeRun;
    output->otmItalicAngle = lpOTMW->otmItalicAngle;
    output->otmEMSquare = lpOTMW->otmEMSquare;
    output->otmAscent = lpOTMW->otmAscent;
    output->otmDescent = lpOTMW->otmDescent;
    output->otmLineGap = lpOTMW->otmLineGap;
    output->otmsCapEmHeight = lpOTMW->otmsCapEmHeight;
    output->otmsXHeight = lpOTMW->otmsXHeight;
    output->otmrcFontBox = lpOTMW->otmrcFontBox;
    output->otmMacAscent = lpOTMW->otmMacAscent;
    output->otmMacDescent = lpOTMW->otmMacDescent;
    output->otmMacLineGap = lpOTMW->otmMacLineGap;
    output->otmusMinimumPPEM = lpOTMW->otmusMinimumPPEM;
    output->otmptSubscriptSize = lpOTMW->otmptSubscriptSize;
    output->otmptSubscriptOffset = lpOTMW->otmptSubscriptOffset;
    output->otmptSuperscriptSize = lpOTMW->otmptSuperscriptSize;
    output->otmptSuperscriptOffset = lpOTMW->otmptSuperscriptOffset;
    output->otmsStrikeoutSize = lpOTMW->otmsStrikeoutSize;
    output->otmsStrikeoutPosition = lpOTMW->otmsStrikeoutPosition;
    output->otmsUnderscoreSize = lpOTMW->otmsUnderscoreSize;
    output->otmsUnderscorePosition = lpOTMW->otmsUnderscorePosition;


    ptr = (char*)(output + 1);
    left = needed - sizeof(*output);

    if(lpOTMW->otmpFamilyName)
    {
        output->otmpFamilyName = (LPSTR)(ptr - (char*)output);
        len = WideCharToMultiByte(CP_ACP, 0,
                                  (WCHAR*)((char*)lpOTMW + (int)lpOTMW->otmpFamilyName), -1,
                                  ptr, left, NULL, NULL);
        left -= len;
        ptr += len;
    }
    else
        output->otmpFamilyName = 0;

    if(lpOTMW->otmpFaceName)
    {
        output->otmpFaceName = (LPSTR)(ptr - (char*)output);
        len = WideCharToMultiByte(CP_ACP, 0,
                                  (WCHAR*)((char*)lpOTMW + (int)lpOTMW->otmpFaceName), -1,
                                  ptr, left, NULL, NULL);
        left -= len;
        ptr += len;
    }
    else
        output->otmpFaceName = 0;

    if(lpOTMW->otmpStyleName)
    {
        output->otmpStyleName = (LPSTR)(ptr - (char*)output);
        len = WideCharToMultiByte(CP_ACP, 0,
                                  (WCHAR*)((char*)lpOTMW + (int)lpOTMW->otmpStyleName), -1,
                                  ptr, left, NULL, NULL);
        left -= len;
        ptr += len;
    }
    else
        output->otmpStyleName = 0;

    if(lpOTMW->otmpFullName)
    {
        output->otmpFullName = (LPSTR)(ptr - (char*)output);
        len = WideCharToMultiByte(CP_ACP, 0,
                                  (WCHAR*)((char*)lpOTMW + (int)lpOTMW->otmpFullName), -1,
                                  ptr, left, NULL, NULL);
        left -= len;
    }
    else
        output->otmpFullName = 0;

    assert(left == 0);

    if(output != lpOTM)
    {
        memcpy(lpOTM, output, cbData);
        HeapFree(GetProcessHeap(), 0, output);

        /* check if the string offsets really fit into the provided size */
        /* FIXME: should we check string length as well? */
        if ((UINT_PTR)lpOTM->otmpFamilyName >= lpOTM->otmSize)
            lpOTM->otmpFamilyName = 0; /* doesn't fit */

        if ((UINT_PTR)lpOTM->otmpFaceName >= lpOTM->otmSize)
            lpOTM->otmpFaceName = 0; /* doesn't fit */

        if ((UINT_PTR)lpOTM->otmpStyleName >= lpOTM->otmSize)
            lpOTM->otmpStyleName = 0; /* doesn't fit */

        if ((UINT_PTR)lpOTM->otmpFullName >= lpOTM->otmSize)
            lpOTM->otmpFullName = 0; /* doesn't fit */
    }

end:
    if(lpOTMW != (OUTLINETEXTMETRICW *)buf)
        HeapFree(GetProcessHeap(), 0, lpOTMW);

    return ret;
}


/*
 * @implemented
 */
UINT
APIENTRY
GetOutlineTextMetricsW(
    HDC			hdc,
    UINT			cbData,
    LPOUTLINETEXTMETRICW	lpOTM
)
{
    TMDIFF Tmd;   // Should not be zero.

    return NtGdiGetOutlineTextMetricsInternalW(hdc, cbData, lpOTM, &Tmd);
}

/*
 * @implemented
 */
DWORD
WINAPI
GetKerningPairsW(HDC hdc,
                 ULONG cPairs,
                 LPKERNINGPAIR pkpDst)
{
    if ((cPairs != 0) || (pkpDst == 0))
    {
        return NtGdiGetKerningPairs(hdc,cPairs,pkpDst);
    }
    else
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }
}

/*
 * @implemented
 */
DWORD
WINAPI
GetKerningPairsA( HDC hDC,
                  DWORD cPairs,
                  LPKERNINGPAIR kern_pairA )
{
    INT charset;
    CHARSETINFO csi;
    CPINFO cpi;
    DWORD i, total_kern_pairs, kern_pairs_copied = 0;
    KERNINGPAIR *kern_pairW;

    if (!cPairs && kern_pairA)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    charset = GetTextCharset(hDC);
    if (!TranslateCharsetInfo(ULongToPtr(charset), &csi, TCI_SRCCHARSET))
    {
        DPRINT1("Can't find codepage for charset %d\n", charset);
        return 0;
    }
    /* GetCPInfo() will fail on CP_SYMBOL, and WideCharToMultiByte is supposed
     * to fail on an invalid character for CP_SYMBOL.
     */
    cpi.DefaultChar[0] = 0;
    if (csi.ciACP != CP_SYMBOL && !GetCPInfo(csi.ciACP, &cpi))
    {
        DPRINT1("Can't find codepage %u info\n", csi.ciACP);
        return 0;
    }
    DPRINT("charset %d => codepage %u\n", charset, csi.ciACP);

    total_kern_pairs = NtGdiGetKerningPairs(hDC, 0, NULL);
    if (!total_kern_pairs) return 0;

    if (!cPairs && !kern_pairA) return total_kern_pairs;

    kern_pairW = HeapAlloc(GetProcessHeap(), 0, total_kern_pairs * sizeof(*kern_pairW));
    if (kern_pairW == NULL)
    {
        return 0;
    }
    GetKerningPairsW(hDC, total_kern_pairs, kern_pairW);

    for (i = 0; i < total_kern_pairs; i++)
    {
        char first, second;

        if (!WideCharToMultiByte(csi.ciACP, 0, &kern_pairW[i].wFirst, 1, &first, 1, NULL, NULL))
            continue;

        if (!WideCharToMultiByte(csi.ciACP, 0, &kern_pairW[i].wSecond, 1, &second, 1, NULL, NULL))
            continue;

        if (first == cpi.DefaultChar[0] || second == cpi.DefaultChar[0])
            continue;

        if (kern_pairA)
        {
            if (kern_pairs_copied >= cPairs) break;

            kern_pairA->wFirst = (BYTE)first;
            kern_pairA->wSecond = (BYTE)second;
            kern_pairA->iKernAmount = kern_pairW[i].iKernAmount;
            kern_pairA++;
        }
        kern_pairs_copied++;
    }

    HeapFree(GetProcessHeap(), 0, kern_pairW);

    return kern_pairs_copied;
}



/*
 * @implemented
 */
HFONT
WINAPI
CreateFontIndirectExA(const ENUMLOGFONTEXDVA *elfexd)
{
    if (elfexd)
    {
        ENUMLOGFONTEXDVW Logfont;

        EnumLogFontExW2A( (LPENUMLOGFONTEXA) elfexd,
                          &Logfont.elfEnumLogfontEx );

        RtlCopyMemory( &Logfont.elfDesignVector,
                       (PVOID) &elfexd->elfDesignVector,
                       sizeof(DESIGNVECTOR));

        return NtGdiHfontCreate( &Logfont, 0, 0, 0, NULL);
    }
    else return NULL;
}


/*
 * @implemented
 */
HFONT
WINAPI
CreateFontIndirectExW(const ENUMLOGFONTEXDVW *elfexd)
{
    /* Msdn: Note, this function ignores the elfDesignVector member in
             ENUMLOGFONTEXDV.
     */
    if ( elfexd )
    {
        return NtGdiHfontCreate((PENUMLOGFONTEXDVW) elfexd, 0, 0, 0, NULL );
    }
    else return NULL;
}


/*
 * @implemented
 */
HFONT
WINAPI
CreateFontIndirectA(
    CONST LOGFONTA		*lplf
)
{
    if (lplf)
    {
        LOGFONTW tlf;

        LogFontA2W(&tlf, lplf);
        return CreateFontIndirectW(&tlf);
    }
    else return NULL;
}


/*
 * @implemented
 */
HFONT
WINAPI
CreateFontIndirectW(
    CONST LOGFONTW		*lplf
)
{
    if (lplf)
    {
        ENUMLOGFONTEXDVW Logfont;

        RtlCopyMemory( &Logfont.elfEnumLogfontEx.elfLogFont, lplf, sizeof(LOGFONTW));
        // Need something other than just cleaning memory here.
        // Guess? Use caller data to determine the rest.
        RtlZeroMemory( &Logfont.elfEnumLogfontEx.elfFullName,
                       sizeof(Logfont.elfEnumLogfontEx.elfFullName));
        RtlZeroMemory( &Logfont.elfEnumLogfontEx.elfStyle,
                       sizeof(Logfont.elfEnumLogfontEx.elfStyle));
        RtlZeroMemory( &Logfont.elfEnumLogfontEx.elfScript,
                       sizeof(Logfont.elfEnumLogfontEx.elfScript));

        Logfont.elfDesignVector.dvNumAxes = 0; // No more than MM_MAX_NUMAXES

        RtlZeroMemory( &Logfont.elfDesignVector, sizeof(DESIGNVECTOR));

        return CreateFontIndirectExW(&Logfont);
    }
    else return NULL;
}


/*
 * @implemented
 */
HFONT
WINAPI
CreateFontA(
    int	nHeight,
    int	nWidth,
    int	nEscapement,
    int	nOrientation,
    int	fnWeight,
    DWORD	fdwItalic,
    DWORD	fdwUnderline,
    DWORD	fdwStrikeOut,
    DWORD	fdwCharSet,
    DWORD	fdwOutputPrecision,
    DWORD	fdwClipPrecision,
    DWORD	fdwQuality,
    DWORD	fdwPitchAndFamily,
    LPCSTR	lpszFace
)
{
    ANSI_STRING StringA;
    UNICODE_STRING StringU;
    HFONT ret;

    RtlInitAnsiString(&StringA, (LPSTR)lpszFace);
    RtlAnsiStringToUnicodeString(&StringU, &StringA, TRUE);

    ret = CreateFontW(nHeight,
                      nWidth,
                      nEscapement,
                      nOrientation,
                      fnWeight,
                      fdwItalic,
                      fdwUnderline,
                      fdwStrikeOut,
                      fdwCharSet,
                      fdwOutputPrecision,
                      fdwClipPrecision,
                      fdwQuality,
                      fdwPitchAndFamily,
                      StringU.Buffer);

    RtlFreeUnicodeString(&StringU);

    return ret;
}


/*
 * @implemented
 */
HFONT
WINAPI
CreateFontW(
    int	nHeight,
    int	nWidth,
    int	nEscapement,
    int	nOrientation,
    int	nWeight,
    DWORD	fnItalic,
    DWORD	fdwUnderline,
    DWORD	fdwStrikeOut,
    DWORD	fdwCharSet,
    DWORD	fdwOutputPrecision,
    DWORD	fdwClipPrecision,
    DWORD	fdwQuality,
    DWORD	fdwPitchAndFamily,
    LPCWSTR	lpszFace
)
{
    LOGFONTW logfont;

    logfont.lfHeight = nHeight;
    logfont.lfWidth = nWidth;
    logfont.lfEscapement = nEscapement;
    logfont.lfOrientation = nOrientation;
    logfont.lfWeight = nWeight;
    logfont.lfItalic = (BYTE)fnItalic;
    logfont.lfUnderline = (BYTE)fdwUnderline;
    logfont.lfStrikeOut = (BYTE)fdwStrikeOut;
    logfont.lfCharSet = (BYTE)fdwCharSet;
    logfont.lfOutPrecision = (BYTE)fdwOutputPrecision;
    logfont.lfClipPrecision = (BYTE)fdwClipPrecision;
    logfont.lfQuality = (BYTE)fdwQuality;
    logfont.lfPitchAndFamily = (BYTE)fdwPitchAndFamily;

    if (NULL != lpszFace)
    {
        int Size = sizeof(logfont.lfFaceName) / sizeof(WCHAR);
        wcsncpy((wchar_t *)logfont.lfFaceName, lpszFace, Size - 1);
        /* Be 101% sure to have '\0' at end of string */
        logfont.lfFaceName[Size - 1] = '\0';
    }
    else
    {
        logfont.lfFaceName[0] = L'\0';
    }

    return CreateFontIndirectW(&logfont);
}


/*
 * @unimplemented
 */
BOOL
WINAPI
CreateScalableFontResourceA(
    DWORD		fdwHidden,
    LPCSTR		lpszFontRes,
    LPCSTR		lpszFontFile,
    LPCSTR		lpszCurrentPath
)
{
    return FALSE;
}


/*
 * @implemented
 */
int
WINAPI
AddFontResourceExW ( LPCWSTR lpszFilename, DWORD fl, PVOID pvReserved )
{
    if (fl & ~(FR_PRIVATE | FR_NOT_ENUM))
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }

    return GdiAddFontResourceW(lpszFilename, fl,0);
}


/*
 * @implemented
 */
int
WINAPI
AddFontResourceExA ( LPCSTR lpszFilename, DWORD fl, PVOID pvReserved )
{
    NTSTATUS Status;
    PWSTR FilenameW;
    int rc;

    if (fl & ~(FR_PRIVATE | FR_NOT_ENUM))
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }

    Status = HEAP_strdupA2W ( &FilenameW, lpszFilename );
    if ( !NT_SUCCESS (Status) )
    {
        SetLastError (RtlNtStatusToDosError(Status));
        return 0;
    }

    rc = GdiAddFontResourceW ( FilenameW, fl, 0 );
    HEAP_free ( FilenameW );
    return rc;
}


/*
 * @implemented
 */
int
WINAPI
AddFontResourceA ( LPCSTR lpszFilename )
{
    NTSTATUS Status;
    PWSTR FilenameW;
    int rc = 0;

    Status = HEAP_strdupA2W ( &FilenameW, lpszFilename );
    if ( !NT_SUCCESS (Status) )
    {
        SetLastError (RtlNtStatusToDosError(Status));
    }
    else
    {
        rc = GdiAddFontResourceW ( FilenameW, 0, 0);

        HEAP_free ( FilenameW );
    }
    return rc;
}


/*
 * @implemented
 */
int
WINAPI
AddFontResourceW ( LPCWSTR lpszFilename )
{
    return GdiAddFontResourceW ( lpszFilename, 0, 0 );
}


/*
 * @implemented
 */
BOOL
WINAPI
RemoveFontResourceW(LPCWSTR lpFileName)
{
    return RemoveFontResourceExW(lpFileName,0,0);
}


/*
 * @implemented
 */
BOOL
WINAPI
RemoveFontResourceA(LPCSTR lpFileName)
{
    return RemoveFontResourceExA(lpFileName,0,0);
}

/*
 * @unimplemented
 */
BOOL
WINAPI
RemoveFontResourceExA(LPCSTR lpFileName,
                      DWORD fl,
                      PVOID pdv
                     )
{
    NTSTATUS Status;
    LPWSTR lpFileNameW;

    /* FIXME the flags */
    /* FIXME the pdv */
    /* FIXME NtGdiRemoveFontResource handle flags and pdv */

    Status = HEAP_strdupA2W ( &lpFileNameW, lpFileName );
    if (!NT_SUCCESS (Status))
        SetLastError (RtlNtStatusToDosError(Status));
    else
    {

        HEAP_free ( lpFileNameW );
    }

    return 0;
}

/*
 * @unimplemented
 */
BOOL
WINAPI
RemoveFontResourceExW(LPCWSTR lpFileName,
                      DWORD fl,
                      PVOID pdv)
{
    /* FIXME the flags */
    /* FIXME the pdv */
    /* FIXME NtGdiRemoveFontResource handle flags and pdv */
    return 0;
}


/***********************************************************************
 *           GdiGetCharDimensions
 *
 * Gets the average width of the characters in the English alphabet.
 *
 * PARAMS
 *  hdc    [I] Handle to the device context to measure on.
 *  lptm   [O] Pointer to memory to store the text metrics into.
 *  height [O] On exit, the maximum height of characters in the English alphabet.
 *
 * RETURNS
 *  The average width of characters in the English alphabet.
 *
 * NOTES
 *  This function is used by the dialog manager to get the size of a dialog
 *  unit. It should also be used by other pieces of code that need to know
 *  the size of a dialog unit in logical units without having access to the
 *  window handle of the dialog.
 *  Windows caches the font metrics from this function, but we don't and
 *  there doesn't appear to be an immediate advantage to do so.
 *
 * SEE ALSO
 *  GetTextExtentPointW, GetTextMetricsW, MapDialogRect.
 *
 * Despite most of MSDN insisting that the horizontal base unit is
 * tmAveCharWidth it isn't.  Knowledge base article Q145994
 * "HOWTO: Calculate Dialog Units When Not Using the System Font",
 * says that we should take the average of the 52 English upper and lower
 * case characters.
 */
/*
 * @implemented
 */
LONG
WINAPI
GdiGetCharDimensions(HDC hdc, LPTEXTMETRICW lptm, LONG *height)
{
    SIZE sz;
    TEXTMETRICW tm;
    static const WCHAR alphabet[] =
    {
        'a','b','c','d','e','f','g','h','i','j','k','l','m','n','o','p','q',
        'r','s','t','u','v','w','x','y','z','A','B','C','D','E','F','G','H',
        'I','J','K','L','M','N','O','P','Q','R','S','T','U','V','W','X','Y','Z',0
    };

    if(!GetTextMetricsW(hdc, &tm)) return 0;

    if(!GetTextExtentPointW(hdc, alphabet, 52, &sz)) return 0;

    if (lptm) *lptm = tm;
    if (height) *height = tm.tmHeight;

    return (sz.cx / 26 + 1) / 2;
}

/*************************************************************************
 * TranslateCharsetInfo [GDI32.@]
 *
 * Fills a CHARSETINFO structure for a character set, code page, or
 * font. This allows making the correspondance between different labelings
 * (character set, Windows, ANSI, and OEM codepages, and Unicode ranges)
 * of the same encoding.
 *
 * Only one codepage will be set in lpCs->fs. If TCI_SRCFONTSIG is used,
 * only one codepage should be set in *lpSrc.
 *
 * RETURNS
 *   TRUE on success, FALSE on failure.
 *
 */
/*
 * @implemented
 */
BOOL
WINAPI
TranslateCharsetInfo(
    LPDWORD lpSrc, /* [in]
       if flags == TCI_SRCFONTSIG: pointer to fsCsb of a FONTSIGNATURE
       if flags == TCI_SRCCHARSET: a character set value
       if flags == TCI_SRCCODEPAGE: a code page value
		 */
    LPCHARSETINFO lpCs, /* [out] structure to receive charset information */
    DWORD flags /* [in] determines interpretation of lpSrc */)
{
    int index = 0;
    switch (flags)
    {
    case TCI_SRCFONTSIG:
        while (index < MAXTCIINDEX && !(*lpSrc>>index & 0x0001)) index++;
        break;
    case TCI_SRCCODEPAGE:
        while (index < MAXTCIINDEX && PtrToUlong(lpSrc) != FONT_tci[index].ciACP) index++;
        break;
    case TCI_SRCCHARSET:
        while (index < MAXTCIINDEX && PtrToUlong(lpSrc) != FONT_tci[index].ciCharset) index++;
        break;
    case TCI_SRCLOCALE:
    {
        LCID lCid = (LCID)PtrToUlong(lpSrc);
        LOCALESIGNATURE LocSig;
        INT Ret = GetLocaleInfoW(lCid, LOCALE_FONTSIGNATURE, (LPWSTR)&LocSig, 0);
        if ( GetLocaleInfoW(lCid, LOCALE_FONTSIGNATURE, (LPWSTR)&LocSig, Ret))
        {
           while (index < MAXTCIINDEX && !(LocSig.lsCsbDefault[0]>>index & 0x0001)) index++;
           break;
        }
    }
    default:
        GdiSetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    if (index >= MAXTCIINDEX || FONT_tci[index].ciCharset == DEFAULT_CHARSET) return FALSE;
    DPRINT("Index %d Charset %u CodePage %u FontSig %lu\n",
             index,FONT_tci[index].ciCharset,FONT_tci[index].ciACP,FONT_tci[index].fs.fsCsb[0]);
    memcpy(lpCs, &FONT_tci[index], sizeof(CHARSETINFO));
    return TRUE;
}


/*
 * @implemented
 */
DWORD
WINAPI
SetMapperFlags(
    HDC	hDC,
    DWORD	flags
)
{
    DWORD Ret = GDI_ERROR;
    PDC_ATTR Dc_Attr;
#if 0
    if (GDI_HANDLE_GET_TYPE(hDC) != GDI_OBJECT_TYPE_DC)
    {
        if (GDI_HANDLE_GET_TYPE(hDC) == GDI_OBJECT_TYPE_METADC)
            return MFDRV_SetMapperFlags( hDC, flags);
        else
        {
            PLDC pLDC = Dc_Attr->pvLDC;
            if ( !pLDC )
            {
                SetLastError(ERROR_INVALID_HANDLE);
                return GDI_ERROR;
            }
            if (pLDC->iType == LDC_EMFLDC)
            {
                return EMFDRV_SetMapperFlags( hDC, flags);
            }
        }
    }
#endif
    if (!GdiGetHandleUserData((HGDIOBJ) hDC, GDI_OBJECT_TYPE_DC, (PVOID) &Dc_Attr)) return GDI_ERROR;

    if (NtCurrentTeb()->GdiTebBatch.HDC == hDC)
    {
        if (Dc_Attr->ulDirty_ & DC_FONTTEXT_DIRTY)
        {
            NtGdiFlush();
            Dc_Attr->ulDirty_ &= ~(DC_MODE_DIRTY|DC_FONTTEXT_DIRTY);
        }
    }

    if ( flags & ~1 )
        SetLastError(ERROR_INVALID_PARAMETER);
    else
    {
        Ret = Dc_Attr->flFontMapper;
        Dc_Attr->flFontMapper = flags;
    }
    return Ret;
}


/*
 * @unimplemented
 */
int
WINAPI
EnumFontsW(
    HDC  hDC,
    LPCWSTR lpFaceName,
    FONTENUMPROCW  FontFunc,
    LPARAM  lParam
)
{
#if 0
    return NtGdiEnumFonts ( hDC, lpFaceName, FontFunc, lParam );
#else
    return EnumFontFamiliesW( hDC, lpFaceName, FontFunc, lParam );
#endif
}

/*
 * @unimplemented
 */
int
WINAPI
EnumFontsA (
    HDC  hDC,
    LPCSTR lpFaceName,
    FONTENUMPROCA  FontFunc,
    LPARAM  lParam
)
{
#if 0
    NTSTATUS Status;
    LPWSTR lpFaceNameW;
    int rc = 0;

    Status = HEAP_strdupA2W ( &lpFaceNameW, lpFaceName );
    if (!NT_SUCCESS (Status))
        SetLastError (RtlNtStatusToDosError(Status));
    else
    {
        rc = NtGdiEnumFonts ( hDC, lpFaceNameW, FontFunc, lParam );

        HEAP_free ( lpFaceNameW );
    }
    return rc;
#else
    return EnumFontFamiliesA( hDC, lpFaceName, FontFunc, lParam );
#endif
}

#define EfdFontFamilies 3

INT
WINAPI
NewEnumFontFamiliesExW(
    HDC hDC,
    LPLOGFONTW lpLogfont,
    FONTENUMPROCW lpEnumFontFamExProcW,
    LPARAM lParam,
    DWORD dwFlags)
{
    ULONG_PTR idEnum, cbDataSize, cbRetSize;
    PENUMFONTDATAW pEfdw;
    PBYTE pBuffer;
    PBYTE pMax;
    INT ret = 1;

    /* Open enumeration handle and find out how much memory we need */
    idEnum = NtGdiEnumFontOpen(hDC,
                               EfdFontFamilies,
                               0,
                               LF_FACESIZE,
                               (lpLogfont && lpLogfont->lfFaceName[0])? lpLogfont->lfFaceName : NULL,
                               lpLogfont? lpLogfont->lfCharSet : DEFAULT_CHARSET,
                               (ULONG*)&cbDataSize);
    if (idEnum == 0)
    {
        return 0;
    }
    if (cbDataSize == 0)
    {
        NtGdiEnumFontClose(idEnum);
        return 0;
    }

    /* Allocate memory */
    pBuffer = HeapAlloc(GetProcessHeap(), 0, cbDataSize);
    if (pBuffer == NULL)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        NtGdiEnumFontClose(idEnum);
        return 0;
    }

    /* Do the enumeration */
    if (!NtGdiEnumFontChunk(hDC, idEnum, cbDataSize, (ULONG*)&cbRetSize, (PVOID)pBuffer))
    {
        HeapFree(GetProcessHeap(), 0, pBuffer);
        NtGdiEnumFontClose(idEnum);
        return 0;
    }

    /* Get start and end address */
    pEfdw = (PENUMFONTDATAW)pBuffer;
    pMax = pBuffer + cbDataSize;

    /* Iterate through the structures */
    while ((PBYTE)pEfdw < pMax && ret)
    {
        PNTMW_INTERNAL pNtmwi = (PNTMW_INTERNAL)((ULONG_PTR)pEfdw + pEfdw->ulNtmwiOffset);

        ret = lpEnumFontFamExProcW((VOID*)&pEfdw->elfexdv.elfEnumLogfontEx,
                                   (VOID*)&pNtmwi->ntmw,
                                   pEfdw->dwFontType,
                                   lParam);

        pEfdw = (PENUMFONTDATAW)((ULONG_PTR)pEfdw + pEfdw->cbSize);
    }

    /* Release the memory and close handle */
    HeapFree(GetProcessHeap(), 0, pBuffer);
    NtGdiEnumFontClose(idEnum);

    return ret;
}
