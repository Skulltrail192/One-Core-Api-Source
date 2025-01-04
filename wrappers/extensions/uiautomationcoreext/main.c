/*
 * Copyright 2020 Nikolay Sivov for CodeWeavers
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
#include "main.h"

WINE_DEFAULT_DEBUG_CHANNEL(uiautomationcoreext);

typedef WCHAR OLECHAR;
typedef OLECHAR* BSTR;
typedef BSTR* LPBSTR;

typedef enum NotificationKind {
  NotificationKind_ItemAdded = 0,
  NotificationKind_ItemRemoved = 1,
  NotificationKind_ActionCompleted = 2,
  NotificationKind_ActionAborted = 3,
  NotificationKind_Other = 4
} NotificationKind;

typedef enum NotificationProcessing {
  NotificationProcessing_ImportantAll = 0,
  NotificationProcessing_ImportantMostRecent = 1,
  NotificationProcessing_All = 2,
  NotificationProcessing_MostRecent = 3,
  NotificationProcessing_CurrentThenMostRecent = 4
} NotificationProcessing;

// qbittorrent 5.1 Beta apparently asks for this
HRESULT 
WINAPI 
UiaRaiseNotificationEvent(           
    void *provider,
    NotificationKind          notificationKind,
    NotificationProcessing    notificationProcessing, 
	BSTR displayString, 
	BSTR activityId
) {
    return S_OK;
};