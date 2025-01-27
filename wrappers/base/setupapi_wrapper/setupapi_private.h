#include <stdarg.h>
#include <stdio.h>

#include "windef.h"
#include "winbase.h"
#include "winnt.h"
#include "winreg.h"
#include "wingdi.h"
#include "winuser.h"
#include "winnls.h"
#include "winsvc.h"
#include "setupapi.h"
#include "wine/debug.h"
#include "wine/heap.h"
#include "wine/list.h"
#include "cfgmgr32.h"
#include "winioctl.h"
#include "rpc.h"
#include "rpcdce.h"
#include "cguid.h"
#include "devpropdef.h"

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

typedef HANDLE HCMNOTIFICATION, *PHCMNOTIFICATION;

DWORD
GetErrorCodeFromCrCode(const IN CONFIGRET cr);

static inline void * __WINE_ALLOC_SIZE(2) heap_realloc_zero(void *mem, size_t len)
{
    return HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, mem, len);
}

struct DeviceInfoSet
{
    DWORD magic;        /* if is equal to SETUP_DEVICE_INFO_SET_MAGIC struct is okay */
    GUID ClassGuid;
    HWND hwndParent;
    struct list devices;
    /* Used when dealing with CM_* functions */
    HMACHINE hMachine;	
    /* Contains the name of the remote computer ('\\COMPUTERNAME' for example),
     * or NULL if related to local machine. Points into szData field at the
     * end of the structure */
    PCWSTR MachineName;	
};

struct device
{
    struct DeviceInfoSet *set;
    HKEY                  key;
    BOOL                  phantom;
    WCHAR                *instanceId;
    struct list           interfaces;
    GUID                  class;
    DEVINST               devnode;
    struct list           entry;
    BOOL                  removed;
    SP_DEVINSTALL_PARAMS_W params;
    struct driver        *drivers;
    unsigned int          driver_count;
    struct driver        *selected_driver;
};

struct ClassInstallParams
{
    PSP_PROPCHANGE_PARAMS PropChangeParams;
    PSP_ADDPROPERTYPAGE_DATA AddPropertyPageData;
};

struct DeviceInfo /* Element of DeviceInfoSet.ListHead */
{
    LIST_ENTRY ListEntry;
    /* Used when dealing with CM_* functions */
    DEVINST dnDevInst;

    /* Link to parent DeviceInfoSet */
    struct DeviceInfoSet *set;

    /* Reserved Field of SP_DEVINSTALL_PARAMS_W structure
     * points to a struct DriverInfoElement */
    SP_DEVINSTALL_PARAMS_W InstallParams;

    /* Information about devnode:
     * - instanceId:
     *       "Root\*PNP0501" for example.
     *       It doesn't contain the unique ID for the device
     *       (points into the Data field at the end of the structure)
     *       WARNING: no NULL char exist between instanceId and UniqueId
     *       in Data field!
     * - UniqueId
     *       "5&1be2108e&0" or "0000"
     *       If DICD_GENERATE_ID is specified in creation flags,
     *       this unique ID is autogenerated using 4 digits, base 10
     *       (points into the Data field at the end of the structure)
     * - DeviceDescription
     *       String which identifies the device. Can be NULL. If not NULL,
     *       points into the Data field at the end of the structure
     * - ClassGuid
     *       Identifies the class of this device. It is GUID_NULL if the
     *       device has not been installed
     * - CreationFlags
     *       Is a combination of:
     *       - DICD_GENERATE_ID
     *              the unique ID needs to be generated
     *       - DICD_INHERIT_CLASSDRVS
     *              inherit driver of the device info set (== same pointer)
     */
    PCWSTR instanceId;
    PCWSTR UniqueId;
    PCWSTR DeviceDescription;
    GUID ClassGuid;
    DWORD CreationFlags;

    /* If CreationFlags contains DICD_INHERIT_CLASSDRVS, this list is invalid */
    /* If the driver is not searched/detected, this list is empty */
    LIST_ENTRY DriverListHead; /* List of struct DriverInfoElement */

    /* List of interfaces implemented by this device */
    LIST_ENTRY InterfaceListHead; /* List of struct DeviceInterface */

    /* Used by SetupDiGetClassInstallParamsW/SetupDiSetClassInstallParamsW */
    struct ClassInstallParams ClassInstallParams;

    /* Device property page provider data */
    HMODULE hmodDevicePropPageProvider;
    PVOID pDevicePropPageProvider;

    /* Variable size array (contains data for instanceId, UniqueId, DeviceDescription) */
    WCHAR Data[ANYSIZE_ARRAY];
};

typedef enum _CM_NOTIFY_FILTER_TYPE
{
    CM_NOTIFY_FILTER_TYPE_DEVICEINTERFACE,
    CM_NOTIFY_FILTER_TYPE_DEVICEHANDLE,
    CM_NOTIFY_FILTER_TYPE_DEVICEINSTANCE,
    CM_NOTIFY_FILTER_TYPE_MAX
} CM_NOTIFY_FILTER_TYPE, *PCM_NOTIFY_FILTER_TYPE;

typedef enum _CM_NOTIFY_ACTION
{
    CM_NOTIFY_ACTION_DEVICEINTERFACEARRIVAL,
    CM_NOTIFY_ACTION_DEVICEINTERFACEREMOVAL,
    CM_NOTIFY_ACTION_DEVICEQUERYREMOVE,
    CM_NOTIFY_ACTION_DEVICEQUERYREMOVEFAILED,
    CM_NOTIFY_ACTION_DEVICEREMOVEPENDING,
    CM_NOTIFY_ACTION_DEVICEREMOVECOMPLETE,
    CM_NOTIFY_ACTION_DEVICECUSTOMEVENT,
    CM_NOTIFY_ACTION_DEVICEINSTANCEENUMERATED,
    CM_NOTIFY_ACTION_DEVICEINSTANCESTARTED,
    CM_NOTIFY_ACTION_DEVICEINSTANCEREMOVED,
    CM_NOTIFY_ACTION_MAX
} CM_NOTIFY_ACTION, *PCM_NOTIFY_ACTION;

typedef struct _CM_NOTIFY_FILTER
{
    DWORD cbSize;
    DWORD Flags;
    CM_NOTIFY_FILTER_TYPE FilterType;
    DWORD Reserved;
    union
    {
        struct
        {
            GUID ClassGuid;
        } DeviceInterface;
        struct
        {
            HANDLE hTarget;
        } DeviceHandle;
        struct
        {
            WCHAR InstanceId[MAX_DEVICE_ID_LEN];
        } DeviceInstance;
    } u;
} CM_NOTIFY_FILTER, *PCM_NOTIFY_FILTER;

typedef struct _CM_NOTIFY_EVENT_DATA
{
    CM_NOTIFY_FILTER_TYPE FilterType;
    DWORD Reserved;
    union
    {
        struct
        {
            GUID  ClassGuid;
            WCHAR SymbolicLink[ANYSIZE_ARRAY];
        } DeviceInterface;
        struct
        {
            GUID  EventGuid;
            LONG  NameOffset;
            DWORD DataSize;
            BYTE  Data[ANYSIZE_ARRAY];
        } DeviceHandle;
        struct
        {
            WCHAR InstanceId[ANYSIZE_ARRAY];
        } DeviceInstance;
    } u;
} CM_NOTIFY_EVENT_DATA, *PCM_NOTIFY_EVENT_DATA;

typedef DWORD (WINAPI *PCM_NOTIFY_CALLBACK)(HCMNOTIFICATION,void*,CM_NOTIFY_ACTION,CM_NOTIFY_EVENT_DATA*,DWORD);