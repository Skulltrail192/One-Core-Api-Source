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

#include <main.h>

void WINAPI FreeUMHandleEntry(PRTL_CRITICAL_SECTION a1, PBOOL a2)
{
  PBOOL resp; // ebx@1
  LONG recursion; // esi@2
  LONG count; // esi@2
  PBOOL v5; // [sp+10h] [bp+Ch]@2

  resp = a2;
  if ( a2[1] )
  {
    recursion = (LONG)(a2 - a1->RecursionCount);
    v5 = a2;
    count = recursion >> 4;
    if ( resp[3] )
      RtlFreeHeap(pUserHeap, 0, (HANDLE)resp[3]);
    *resp = 0;
    resp[1] = 0;
    resp[2] = 0;
    resp[3] = 0;
    *resp = (LONG)v5 + 1;
    --*&a1->DebugInfo->CreatorBackTraceIndexHigh;
    if ( count < a1->LockCount )
      a1->LockCount = count;
  }
}

PBOOL WINAPI UMHandleActiveEntryFromHandle(RTL_CRITICAL_SECTION a1, HANDLE a2)
{
  PBOOL result = FALSE; // eax@2 
  DbgPrint("UMHandleActiveEntryFromHandle is UNIMPLEMENTED\n");  
  return result;
}

BOOL WINAPI UnlockUMHandleList(PRTL_CRITICAL_SECTION CriticalSectionObject)
{
  NTSTATUS status = STATUS_SUCCESS; // eax@1
  BOOL result; // eax@2

  RtlLeaveCriticalSection(CriticalSectionObject);
  if ( status >= 0 )
  {
    result = TRUE;
  }
  else
  {
    SetLastError(status);
    result = FALSE;
  }
  return result;
}

BOOL WINAPI LockUMHandleList(PRTL_CRITICAL_SECTION CriticalSectionObject)
{
  NTSTATUS status = STATUS_SUCCESS; // eax@1
  BOOL result; // eax@2

  RtlEnterCriticalSection(CriticalSectionObject);
  if ( status >= 0 )
  {
    result = TRUE;
  }
  else
  {
    SetLastError(status);
    result = FALSE;
  }
  return result;
}

BOOL WINAPI CloseTouchInputHandle(HTOUCHINPUT a1)
{
    return FALSE;
}

BOOL WINAPI CloseGestureInfoHandle(HGESTUREINFO  hGestureInfo)
{
    return FALSE;
}

BOOL WINAPI RegisterTouchWindow(
  _In_  HWND hWnd,
  _In_  ULONG ulFlags
)
{
	DbgPrint("RegisterTouchWindow is UNIMPLEMENTED\n");  
	return TRUE;
}

BOOL WINAPI UnregisterTouchWindow(
  _In_  HWND hWnd
)
{
	DbgPrint("UnregisterTouchWindow is UNIMPLEMENTED\n");  
	return TRUE;
}

DWORD64 WINAPI GetTouchInputDataSize(UINT parameter)
{
  DWORD64 resp; // qax@1

  resp = 40i64 * parameter;
  if ( resp > 0xFFFFFFFF )
    resp = 0;
  return resp;
}

BOOL WINAPI GetTouchInputInfoWorker(HTOUCHINPUT hTouchInput, UINT cInputs, PTOUCHINPUT pInputs, int cbSize)
{
  UINT localInputs; // edi@1
  BOOL resp; // ebx@1
  PBOOL verification; // eax@4
  UINT sizeVerification; // eax@6
  UINT division; // esi@6
  DWORD64 dataSize; // eax@6
  DWORD64 LocalSizeA; // ecx@6
  UINT resulOperation; // eax@6
  BOOL error; // [sp-4h] [bp-Ch]@5
  PBOOL used; // [sp+10h] [bp+8h]@4
  UINT Sizea; // [sp+14h] [bp+Ch]@6

  localInputs = cInputs;
  resp = 1;
  if ( cInputs && pInputs )
  {
    if ( LockUMHandleList(&CriticalSectionObject) )
    {
      verification = (PBOOL)UMHandleActiveEntryFromHandle(CriticalSectionObject, hTouchInput);
      used = verification;
      if ( !verification )
      {
        error = 6;
LABEL_9:
        RtlSetLastWin32Error(error);
        resp = 0;
LABEL_13:
        UnlockUMHandleList(&CriticalSectionObject);
        return resp;
      }
      sizeVerification = verification[2];
      Sizea = sizeVerification;
      division = sizeVerification / 0x28;
      dataSize = GetTouchInputDataSize(sizeVerification / 0x28);
      LocalSizeA = Sizea;
      resulOperation = dataSize == Sizea ? division : 0;
      if ( cbSize )
      {
        if ( resulOperation != localInputs )
        {
          error = 87;
          goto LABEL_9;
        }
      }
      else
      {
        if ( resulOperation > localInputs )
          LocalSizeA = GetTouchInputDataSize(localInputs);
      }
      memcpy(pInputs, (const void *)used[3], LocalSizeA);
      goto LABEL_13;
    }
  }
  else
  {
    RtlSetLastWin32Error(87);
  }
  return 0;
}

BOOL WINAPI GetTouchInputInfo(
  _In_   HTOUCHINPUT hTouchInput,
  _In_   UINT cInputs,
  _Out_  PTOUCHINPUT pInputs,
  _In_   int cbSize
)
{
	  // BOOL result; // eax@2
	  // if ( cbSize == 40 )
	  // {
		// result = GetTouchInputInfoWorker(hTouchInput, cInputs, pInputs, 0);
	  // }
	  // else
	  // {
		// RtlSetLastWin32Error(87);
		// result = 0;
	  // }
	  // return result;
	return FALSE;
}

BOOL WINAPI IsTouchWindow(
  _In_       HWND hWnd,
  _Out_opt_  PULONG pulFlags
)
{
	DbgPrint("IsTouchWindow is UNIMPLEMENTED\n"); 
	SetLastError(0);
	return FALSE;
}

BOOL WINAPI UnregisterPointerInputTarget(
  _In_  HWND hwnd,
  _In_  POINTER_INPUT_TYPE  pointerType
)
{
	DbgPrint("UnregisterPointerInputTarget is UNIMPLEMENTED\n");  
	return TRUE;
}

HRESULT WINAPI PackTouchHitTestingProximityEvaluation(const TOUCH_HIT_TESTING_INPUT *pHitTestingInput, const TOUCH_HIT_TESTING_PROXIMITY_EVALUATION *pProximityEval)
{
  int proximityEval; // esi@1
  int proximityDistance; // edi@1
  HRESULT result; // eax@4

  proximityEval = pProximityEval->adjustedPoint.x - pHitTestingInput->point.x;
  proximityDistance = pProximityEval->adjustedPoint.y - pHitTestingInput->point.y;
  if ( pProximityEval->score > 0xFFFu || abs(proximityEval) >= 511 || abs(proximityDistance) >= 511 )
    result = 0xFFF00000u;
  else
    result = (proximityDistance & 0x3FF) + (((proximityEval & 0x3FF) + (pProximityEval->score << 10)) << 10);
  return result;
}

BOOL WINAPI PtInRect(const RECT *lprc, POINT pt)
{
  return lprc && pt.x >= lprc->left && pt.x < lprc->right && pt.y >= lprc->top && pt.y < lprc->bottom;
}

BOOL WINAPI _ValidatePointerTargetingInput(const TOUCH_HIT_TESTING_INPUT *testingInput)
{
  POINT pointY; // ST04_8@1
  RECT localRect; // qdi@1
  LONG right; // eax@8
  LONG left; // eax@10

  pointY.y = testingInput->point.y;
  localRect = testingInput->boundingBox;
  pointY.x = testingInput->point.x;
  return (PtInRect(&testingInput->boundingBox, pointY)
       || (right = testingInput->point.x, localRect.left == right)
       && testingInput->boundingBox.right == right
       && (left = testingInput->point.y, testingInput->boundingBox.top == left)
       && testingInput->boundingBox.bottom == left)
      && localRect.left <= testingInput->nonOccludedBoundingBox.left
      && testingInput->boundingBox.right >= testingInput->nonOccludedBoundingBox.right
      && testingInput->boundingBox.top <= testingInput->nonOccludedBoundingBox.top
      && testingInput->boundingBox.bottom >= testingInput->nonOccludedBoundingBox.bottom;
}

BOOL WINAPI GetGestureInfo(
  _In_   HGESTUREINFO hGestureInfo,
  _Out_  PGESTUREINFO pGestureInfo
)
{
	pGestureInfo = NULL;
	DbgPrint("GetGestureInfo is UNIMPLEMENTED\n");  
	return TRUE;
}

BOOL WINAPI SetGestureConfig(
  _In_  HWND hwnd,
  _In_  DWORD dwReserved,
  _In_  UINT cIDs,
  _In_  PGESTURECONFIG pGestureConfig,
  _In_  UINT cbSize
)
{
	pGestureConfig = NULL;
	DbgPrint("SetGestureConfig is UNIMPLEMENTED\n");  
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}

BOOL WINAPI GetGestureConfig(
  _In_     HWND hwnd,
  _In_     DWORD dwReserved,
  _In_     DWORD dwFlags,
  _In_     PUINT pcIDs,
  _Inout_  PGESTURECONFIG pGestureConfig,
  _In_     UINT cbSize
)
{
	pGestureConfig = NULL;
	DbgPrint("GetGestureConfig is UNIMPLEMENTED\n");  
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


BOOL WINAPI GetGestureExtraArgs(
  _In_   HGESTUREINFO hGestureInfo,
  _In_   UINT cbExtraArgs,
  _Out_  PBYTE pExtraArgs
)
{
	pExtraArgs = NULL;
	DbgPrint("GetGestureExtraArgs is UNIMPLEMENTED\n");  	
	return TRUE;
}

BOOL 
WINAPI 
EvaluateProximityToRect(
  _In_  const RECT                                   *controlBoundingBox,
  _In_  const TOUCH_HIT_TESTING_INPUT                *pHitTestingInput,
  _Out_       TOUCH_HIT_TESTING_PROXIMITY_EVALUATION *pProximityEval
)
{
	DbgPrint("EvaluateProximityToRect is UNIMPLEMENTED\n");  	
	return FALSE;
}

// /**********************************************************************
 // * GetPointerDevices [USER32.@]
 // */
// BOOL WINAPI GetPointerDevices(UINT32 *device_count, POINTER_DEVICE_INFO *devices)
// {
    // DbgPrint("GetPointerDevices (%p %p): partial stub\n", device_count, devices);

    // if (!device_count)
        // return FALSE;

    // if (devices)
        // return FALSE;

    // *device_count = 0;
    // return TRUE;
// }

BOOL 
WINAPI 
GetPointerType(
  _In_  UINT32             pointerId,
  _Out_ POINTER_INPUT_TYPE *pointerType
)
{
	DbgPrint("GetPointerType is UNIMPLEMENTED\n");		
	*pointerType = PT_MOUSE;
	return TRUE;
}

BOOL 
WINAPI 
RegisterPointerDeviceNotifications(
  _In_ HWND window,
  _In_ BOOL notifyRange
)
{
	DbgPrint("RegisterPointerDeviceNotifications is UNIMPLEMENTED\n");		
	return TRUE;
}

BOOL 
WINAPI 
RegisterTouchHitTestingWindow(
  _In_ HWND  hwnd,
  _In_ ULONG value
)
{
	DbgPrint("RegisterTouchHitTestingWindow is UNIMPLEMENTED\n");		
	return TRUE;
}