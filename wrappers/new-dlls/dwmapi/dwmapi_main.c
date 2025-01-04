/*
 * Dwmapi
 *
 * Copyright 2007 Andras Kovacs
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

#include <stdarg.h>

#include "rtlfuncs.h"
#define COBJMACROS
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "dwmapi.h"
#include "new-winerror.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(dwmapi);

#define DWM_E_COMPOSITIONDISABLED  _HRESULT_TYPEDEF_(0x80263001)
#define MILERR_MISMATCHED_SIZE                             _HRESULT_TYPEDEF_(0x88980090)

DPI_AWARENESS_CONTEXT WINAPI SetThreadDpiAwarenessContext( DPI_AWARENESS_CONTEXT context );

typedef struct DWM_COLORIZATION_PARAMS {
      DWORD clrColor;
      DWORD clrAfterGlow;
      DWORD nIntensity;
      DWORD clrAfterGlowBalance;
      DWORD clrBlurBalance;
      DWORD clrGlassReflectionIntensity;
      BOOLEAN fOpaque;
}DWM_COLORIZATION_PARAMS;

/**********************************************************************
 *           DwmIsCompositionEnabled         (DWMAPI.@)
 */
HRESULT WINAPI DwmIsCompositionEnabled(BOOL *enabled)
{
    RTL_OSVERSIONINFOW version;

    TRACE("%p\n", enabled);

    if (!enabled)
        return E_INVALIDARG;

    *enabled = FALSE;
    version.dwOSVersionInfoSize = sizeof(version);
    if (!RtlGetVersion(&version))
        *enabled = (version.dwMajorVersion > 6 || (version.dwMajorVersion == 6 && version.dwMinorVersion >= 3));

    return S_OK;
}

/**********************************************************************
 *           DwmEnableComposition         (DWMAPI.102)
 */
HRESULT WINAPI DwmEnableComposition(UINT uCompositionAction)
{
    //FIXME("(%d) stub\n", uCompositionAction);

	BOOL isCompositionEnabled;
	DwmIsCompositionEnabled(&isCompositionEnabled);
	
	if (isCompositionEnabled) 
		return S_OK;
	else 
		return DWM_E_COMPOSITIONDISABLED;
}

/**********************************************************************
 *           DwmExtendFrameIntoClientArea    (DWMAPI.@)
 */
HRESULT WINAPI DwmExtendFrameIntoClientArea(HWND hwnd, const MARGINS* margins)
{
    //FIXME("(%p, %p) stub\n", hwnd, margins);

	BOOL isCompositionEnabled;
	DwmIsCompositionEnabled(&isCompositionEnabled);
	
	if (isCompositionEnabled) 
		return S_OK;
	else 
		return DWM_E_COMPOSITIONDISABLED;
}

/**********************************************************************
 *           DwmGetColorizationColor      (DWMAPI.@)
 */
HRESULT WINAPI DwmGetColorizationColor(DWORD *colorization, BOOL opaque_blend)
{
    *colorization = 0x6874B8FC; // Default Windows Vista theme color. TODO: infer from XP theme color
    return S_OK;
}

static int get_display_frequency(void)
{
    DEVMODEW mode;
    BOOL ret;

    memset(&mode, 0, sizeof(mode));
    mode.dmSize = sizeof(mode);
    ret = EnumDisplaySettingsExW(NULL, ENUM_CURRENT_SETTINGS, &mode, 0);
    if (ret && mode.dmFields & DM_DISPLAYFREQUENCY && mode.dmDisplayFrequency)
    {
        return mode.dmDisplayFrequency;
    }
    else
    {
        WARN("Failed to query display frequency, returning a fallback value.\n");
        return 60;
    }
}

/**********************************************************************
 *                  DwmFlush              (DWMAPI.@)
 */
// Applied Wine PR #6006 to fix WINE Bug 56935: "Softube VST plugin UI breaks"
HRESULT WINAPI DwmFlush(void)
{
    static volatile LONG last_time;
    static BOOL once;
    DWORD now, interval, last = last_time, target;
    int freq;

    if (!once++) FIXME("() semi-stub\n");

    // simulate the WaitForVBlank-like blocking behavior
    freq = get_display_frequency();
    interval = 1000 / freq;
    now = GetTickCount();
    if (now - last < interval)
        target = last + interval;
    else
    {
        // act as if we were called midway between 2 vsyncs
        target = now + interval / 2;
    }
    Sleep(target - now);
    InterlockedCompareExchange(&last_time, target, last);

    return S_OK;
}

/**********************************************************************
 *        DwmInvalidateIconicBitmaps      (DWMAPI.@)
 */
HRESULT WINAPI DwmInvalidateIconicBitmaps(HWND hwnd)
{
    static BOOL once;
	BOOL isCompositionEnabled;

    if (!once++) //FIXME("(%p) stub\n", hwnd);
	
	DwmIsCompositionEnabled(&isCompositionEnabled);
	
	if (isCompositionEnabled) 
		return S_OK;
	else 
		return DWM_E_COMPOSITIONDISABLED;
}

/**********************************************************************
 *           DwmSetWindowAttribute         (DWMAPI.@)
 */
HRESULT WINAPI DwmSetWindowAttribute(HWND hwnd, DWORD attributenum, LPCVOID attribute, DWORD size)
{
    static BOOL once;
	BOOL isCompositionEnabled;

    if (!once++) //FIXME("(%p, %x, %p, %x) stub\n", hwnd, attributenum, attribute, size);
	
	DwmIsCompositionEnabled(&isCompositionEnabled);
	
	return S_OK;
}

/**********************************************************************
 *           DwmGetGraphicsStreamClient         (DWMAPI.@)
 */
HRESULT WINAPI DwmGetGraphicsStreamClient(UINT uIndex, UUID *pClientUuid)
{
    //FIXME("(%d, %p) stub\n", uIndex, pClientUuid);

	BOOL isCompositionEnabled;
	DwmIsCompositionEnabled(&isCompositionEnabled);
	
	if (isCompositionEnabled) 
		return S_OK;
	else 
		return DWM_E_COMPOSITIONDISABLED;
}

/**********************************************************************
 *           DwmGetTransportAttributes         (DWMAPI.@)
 */
HRESULT WINAPI DwmGetTransportAttributes(BOOL *pfIsRemoting, BOOL *pfIsConnected, DWORD *pDwGeneration)
{
    //FIXME("(%p, %p, %p) stub\n", pfIsRemoting, pfIsConnected, pDwGeneration);

	BOOL isCompositionEnabled;
	DwmIsCompositionEnabled(&isCompositionEnabled);
	
	if (isCompositionEnabled) 
		return S_OK;
	else 
		return DWM_E_COMPOSITIONDISABLED;
}

/**********************************************************************
 *           DwmUnregisterThumbnail         (DWMAPI.@)
 */
HRESULT WINAPI DwmUnregisterThumbnail(HTHUMBNAIL thumbnail)
{
    //FIXME("(%p) stub\n", thumbnail);

	BOOL isCompositionEnabled;
	DwmIsCompositionEnabled(&isCompositionEnabled);
	
	if (isCompositionEnabled) 
		return S_OK;
	else 
		return DWM_E_COMPOSITIONDISABLED;
}

/**********************************************************************
 *           DwmEnableMMCSS         (DWMAPI.@)
 */
HRESULT WINAPI DwmEnableMMCSS(BOOL enableMMCSS)
{
    //FIXME("(%d) stub\n", enableMMCSS);

	BOOL isCompositionEnabled;
	DwmIsCompositionEnabled(&isCompositionEnabled);
	
	if (isCompositionEnabled) 
		return S_OK;
	else 
		return DWM_E_COMPOSITIONDISABLED;
}

/**********************************************************************
 *           DwmGetGraphicsStreamTransformHint         (DWMAPI.@)
 */
HRESULT WINAPI DwmGetGraphicsStreamTransformHint(UINT uIndex, MilMatrix3x2D *pTransform)
{
    //FIXME("(%d, %p) stub\n", uIndex, pTransform);

	BOOL isCompositionEnabled;
	DwmIsCompositionEnabled(&isCompositionEnabled);
	
	if (isCompositionEnabled) 
		return S_OK;
	else 
		return DWM_E_COMPOSITIONDISABLED;
}

/**********************************************************************
 *           DwmEnableBlurBehindWindow         (DWMAPI.@)
 */
HRESULT WINAPI DwmEnableBlurBehindWindow(HWND hWnd, const DWM_BLURBEHIND *pBlurBuf)
{
    //FIXME("%p %p\n", hWnd, pBlurBuf);

	BOOL isCompositionEnabled;
	DwmIsCompositionEnabled(&isCompositionEnabled);
	
	if (isCompositionEnabled) 
		return S_OK;
	else 
		return DWM_E_COMPOSITIONDISABLED;
}

/**********************************************************************
 *           DwmDefWindowProc         (DWMAPI.@)
 */
BOOL WINAPI DwmDefWindowProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam, LRESULT *plResult)
{
    static int i;

    //if (!i++) FIXME("stub\n");

    return FALSE;
}

/**********************************************************************
 *           DwmGetWindowAttribute         (DWMAPI.@)
 */
HRESULT WINAPI DwmGetWindowAttribute(HWND hwnd, DWORD attribute, PVOID pv_attribute, DWORD size)
{
    //BOOL enabled = FALSE;
    HRESULT hr;

    TRACE("(%p %ld %p %ld)\n", hwnd, attribute, pv_attribute, size);

    // if (DwmIsCompositionEnabled(&enabled) == S_OK && !enabled)
        // return E_HANDLE;
    // if (!IsWindow(hwnd))
        // return E_HANDLE;

    switch (attribute) {
    case DWMWA_EXTENDED_FRAME_BOUNDS:
    {
        RECT *rect = (RECT *)pv_attribute;
        //DPI_AWARENESS_CONTEXT context;

        if (!rect)
            return E_INVALIDARG;
        if (size < sizeof(*rect))
            return E_NOT_SUFFICIENT_BUFFER;
        if (GetWindowLongW(hwnd, GWL_STYLE) & WS_CHILD)
            return E_HANDLE;

        /* DWM frame bounds are always in physical coords */
        //context = SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE);
        if (GetWindowRect(hwnd, rect))
            hr = S_OK;
        else
            hr = HRESULT_FROM_WIN32(GetLastError());

        //SetThreadDpiAwarenessContext(context);
        break;
    }
    default:
        FIXME("attribute %ld not implemented.\n", attribute);
        hr = E_NOTIMPL;
        break;
    }

    return hr;
}

/**********************************************************************
 *           DwmRegisterThumbnail         (DWMAPI.@)
 */
HRESULT WINAPI DwmRegisterThumbnail(HWND dest, HWND src, PHTHUMBNAIL thumbnail_id)
{
    //FIXME("(%p %p %p) stub\n", dest, src, thumbnail_id);

	BOOL isCompositionEnabled;
	DwmIsCompositionEnabled(&isCompositionEnabled);
	
	if (isCompositionEnabled) 
		return S_OK;
	else 
		return DWM_E_COMPOSITIONDISABLED;
}

/**********************************************************************
 *           DwmGetCompositionTimingInfo         (DWMAPI.@)
 */
HRESULT WINAPI DwmGetCompositionTimingInfo(HWND hwnd, DWM_TIMING_INFO *info)
{
    LARGE_INTEGER performance_frequency, qpc;
    static int i, display_frequency;

    if (!info)
        return E_INVALIDARG;

    if (info->cbSize != sizeof(DWM_TIMING_INFO))
        return MILERR_MISMATCHED_SIZE;

    if(!i++) //FIXME("(%p %p)\n", hwnd, info);

    memset(info, 0, info->cbSize);
    info->cbSize = sizeof(DWM_TIMING_INFO);

    display_frequency = get_display_frequency();
    info->rateRefresh.uiNumerator = display_frequency;
    info->rateRefresh.uiDenominator = 1;
    info->rateCompose.uiNumerator = display_frequency;
    info->rateCompose.uiDenominator = 1;

    QueryPerformanceFrequency(&performance_frequency);
    info->qpcRefreshPeriod = performance_frequency.QuadPart / display_frequency;

    QueryPerformanceCounter(&qpc);
    info->qpcVBlank = (qpc.QuadPart / info->qpcRefreshPeriod) * info->qpcRefreshPeriod;

    return S_OK;
}

/**********************************************************************
 *           DwmAttachMilContent         (DWMAPI.@)
 */
HRESULT WINAPI DwmAttachMilContent(HWND hwnd)
{
    //FIXME("(%p) stub\n", hwnd);
    return S_OK;
	
	// if (isCompositionEnabled) 
		// return S_OK;
	// else 
		// return DWM_E_COMPOSITIONDISABLED;
}

/**********************************************************************
 *           DwmDetachMilContent         (DWMAPI.@)
 */
HRESULT WINAPI DwmDetachMilContent(HWND hwnd)
{
    //FIXME("(%p) stub\n", hwnd);
	BOOL isCompositionEnabled;
	DwmIsCompositionEnabled(&isCompositionEnabled);
	
	if (isCompositionEnabled) 
		return S_OK;
	else 
		return DWM_E_COMPOSITIONDISABLED;
}

/**********************************************************************
 *           DwmUpdateThumbnailProperties         (DWMAPI.@)
 */
HRESULT WINAPI DwmUpdateThumbnailProperties(HTHUMBNAIL thumbnail, const DWM_THUMBNAIL_PROPERTIES *props)
{
    //FIXME("(%p, %p) stub\n", thumbnail, props);
	BOOL isCompositionEnabled;
	DwmIsCompositionEnabled(&isCompositionEnabled);
	
	if (isCompositionEnabled) 
		return S_OK;
	else 
		return DWM_E_COMPOSITIONDISABLED;
}

/**********************************************************************
 *           DwmSetPresentParameters         (DWMAPI.@)
 */
HRESULT WINAPI DwmSetPresentParameters(HWND hwnd, DWM_PRESENT_PARAMETERS *params)
{
    //FIXME("(%p %p) stub\n", hwnd, params);
	BOOL isCompositionEnabled;
	DwmIsCompositionEnabled(&isCompositionEnabled);
	
	if (isCompositionEnabled) 
		return S_OK;
	else 
		return DWM_E_COMPOSITIONDISABLED;
};

/**********************************************************************
 *           DwmSetIconicLivePreviewBitmap         (DWMAPI.@)
 */
HRESULT WINAPI DwmSetIconicLivePreviewBitmap(HWND hwnd, HBITMAP hbmp, POINT *pos, DWORD flags)
{
    //FIXME("(%p %p %p %x) stub\n", hwnd, hbmp, pos, flags);
	BOOL isCompositionEnabled;
	DwmIsCompositionEnabled(&isCompositionEnabled);
	
	if (isCompositionEnabled) 
		return S_OK;
	else 
		return DWM_E_COMPOSITIONDISABLED;
};

/**********************************************************************
 *           DwmSetIconicThumbnail         (DWMAPI.@)
 */
HRESULT WINAPI DwmSetIconicThumbnail(HWND hwnd, HBITMAP hbmp, DWORD flags)
{
    //FIXME("(%p %p %x) stub\n", hwnd, hbmp, flags);
	BOOL isCompositionEnabled;
	DwmIsCompositionEnabled(&isCompositionEnabled);
	
	if (isCompositionEnabled) 
		return S_OK;
	else 
		return DWM_E_COMPOSITIONDISABLED;
};

/**********************************************************************
 *           DwmpGetColorizationParameters         (DWMAPI.@)
 */
HRESULT WINAPI DwmpGetColorizationParameters(DWM_COLORIZATION_PARAMS *parameters) {
    memset(parameters, 0, sizeof(DWM_COLORIZATION_PARAMS));
    parameters->clrColor = 0x6874B8FC;
    parameters->fOpaque = TRUE;
    return S_OK;
} 