/*++

Copyright (c) 2023 Shorthorn Project

Module Name:

    ldrapi.c

Abstract:

    Implement Ldr APIs

Author:

    Skulltrail 12-October-2023

Revision History:

--*/
 
#define NDEBUG

#include <main.h>
#include <compatguid_undoc.h>
#include <compat_undoc.h>
#include <debug.h>

typedef enum {
  ACTCTX_COMPATIBILITY_ELEMENT_TYPE_UNKNOWN = 0,
  ACTCTX_COMPATIBILITY_ELEMENT_TYPE_OS
} ACTCTX_COMPATIBILITY_ELEMENT_TYPE;

/* convert PE image VirtualAddress to Real Address */
static inline void *get_rva( HMODULE module, DWORD va )
{
    return (void *)((char *)module + va);
}

/****************************************************************************
 *              LdrResolveDelayLoadedAPI   (NTDLL.@)
 */
void* WINAPI LdrResolveDelayLoadedAPI( void* base, const IMAGE_DELAYLOAD_DESCRIPTOR* desc,
                                       PDELAYLOAD_FAILURE_DLL_CALLBACK dllhook,
                                       PDELAYLOAD_FAILURE_SYSTEM_ROUTINE syshook,
                                       IMAGE_THUNK_DATA* addr, ULONG flags )
{
    IMAGE_THUNK_DATA *pIAT, *pINT;
    DELAYLOAD_INFO delayinfo;
    UNICODE_STRING mod;
    const CHAR* name;
    HMODULE *phmod;
    NTSTATUS nts;
    FARPROC fp;
    DWORD id;

    DbgPrint( "LdrResolveDelayLoadedAPI::(%p, %p, %p, %p, %p, 0x%08x)\n", base, desc, dllhook, syshook, addr, flags );

    phmod = get_rva(base, desc->ModuleHandleRVA);
    pIAT = get_rva(base, desc->ImportAddressTableRVA);
    pINT = get_rva(base, desc->ImportNameTableRVA);
    name = get_rva(base, desc->DllNameRVA);
    id = addr - pIAT;

    if (!*phmod)
    {
        if (!RtlCreateUnicodeStringFromAsciiz(&mod, name))
        {
            nts = STATUS_NO_MEMORY;
            goto fail;
        }
        nts = LdrLoadDll(NULL, 0, &mod, phmod);
        RtlFreeUnicodeString(&mod);
        if (nts) goto fail;
    }

    if (IMAGE_SNAP_BY_ORDINAL(pINT[id].u1.Ordinal))
        nts = LdrGetProcedureAddress(*phmod, NULL, LOWORD(pINT[id].u1.Ordinal), (void**)&fp);
    else
    {
        const IMAGE_IMPORT_BY_NAME* iibn = get_rva(base, pINT[id].u1.AddressOfData);
        ANSI_STRING fnc;

        RtlInitAnsiString(&fnc, (char*)iibn->Name);
        nts = LdrGetProcedureAddress(*phmod, &fnc, 0, (void**)&fp);
    }
    if (!nts)
    {
        pIAT[id].u1.Function = (ULONG_PTR)fp;
        return fp;
    }

fail:
    delayinfo.Size = sizeof(delayinfo);
    delayinfo.DelayloadDescriptor = desc;
    delayinfo.ThunkAddress = addr;
    delayinfo.TargetDllName = name;
    delayinfo.TargetApiDescriptor.ImportDescribedByName = !IMAGE_SNAP_BY_ORDINAL(pINT[id].u1.Ordinal);
    delayinfo.TargetApiDescriptor.Description.Ordinal = LOWORD(pINT[id].u1.Ordinal);
    delayinfo.TargetModuleBase = *phmod;
    delayinfo.Unused = NULL;
    delayinfo.LastError = nts;

    if (dllhook)
        return dllhook(4, &delayinfo);

    if (IMAGE_SNAP_BY_ORDINAL(pINT[id].u1.Ordinal))
    {
        DWORD_PTR ord = LOWORD(pINT[id].u1.Ordinal);
        return syshook(name, (const char *)ord);
    }
    else
    {
        const IMAGE_IMPORT_BY_NAME* iibn = get_rva(base, pINT[id].u1.AddressOfData);
        return syshook(name, (const char *)iibn->Name);
    }
}

NTSTATUS
NTAPI
LdrQueryModuleServiceTags(
    _In_ PVOID DllHandle,
    _Out_writes_(*BufferSize) PULONG ServiceTagBuffer,
    _Inout_ PULONG BufferSize
)
{
	DbgPrint("UNIMPLEMENTED: LdrQueryModuleServiceTags");		
	return STATUS_SUCCESS;
}

NTSTATUS 
WINAPI 
LdrResolveDelayLoadsFromDll(
  _In_        PVOID ParentBase,
  _In_        LPCSTR TargetDllName,
  _Reserved_  ULONG Flags
)
{
	DbgPrint("UNIMPLEMENTED: LdrResolveDelayLoadsFromDll");		
	return STATUS_SUCCESS;	
}

HANDLE ImageExecOptionsKey;

HANDLE Wow64ExecOptionsKey;

UNICODE_STRING Wow64OptionsString = RTL_CONSTANT_STRING(L"");

UNICODE_STRING ImageExecOptionsString = RTL_CONSTANT_STRING(L"\\Registry\\Machine\\Software\\Microsoft\\Windows NT\\CurrentVersion\\Image File Execution Options");

NTSTATUS 
NTAPI 
LdrOpenImageFileOptionsKey( 	
	IN PUNICODE_STRING  	SubKey,
	IN BOOLEAN  	Wow64,
	OUT PHANDLE  	NewKeyHandle 
	) 		
{
    PHANDLE RootKeyLocation;
    HANDLE RootKey;
    UNICODE_STRING SubKeyString;
    OBJECT_ATTRIBUTES ObjectAttributes;
    NTSTATUS Status;
    PWCHAR p1;

    /* Check which root key to open */
    if (Wow64)
        RootKeyLocation = &Wow64ExecOptionsKey;
    else
        RootKeyLocation = &ImageExecOptionsKey;

    /* Get the current key */
    RootKey = *RootKeyLocation;

    /* Setup the object attributes */
    InitializeObjectAttributes(&ObjectAttributes,
                               Wow64 ?
                               &Wow64OptionsString : &ImageExecOptionsString,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    /* Open the root key */
    Status = ZwOpenKey(&RootKey, KEY_ENUMERATE_SUB_KEYS, &ObjectAttributes);
    if (NT_SUCCESS(Status))
    {
        /* Write the key handle */
        if (_InterlockedCompareExchange((LONG*)RootKeyLocation, (LONG)RootKey, 0) != 0)
        {
            /* Someone already opened it, use it instead */
            NtClose(RootKey);
            RootKey = *RootKeyLocation;
        }

        /* Extract the name */
        SubKeyString = *SubKey;
        p1 = (PWCHAR)((ULONG_PTR)SubKeyString.Buffer + SubKeyString.Length);
        while (SubKeyString.Length)
        {
            if (p1[-1] == L'\\') break;
            p1--;
            SubKeyString.Length -= sizeof(*p1);
        }
        SubKeyString.Buffer = p1;
        SubKeyString.Length = SubKey->Length - SubKeyString.Length;

        /* Setup the object attributes */
        InitializeObjectAttributes(&ObjectAttributes,
                                   &SubKeyString,
                                   OBJ_CASE_INSENSITIVE,
                                   RootKey,
                                   NULL);

        /* Open the setting key */
        Status = ZwOpenKey((PHANDLE)NewKeyHandle, GENERIC_READ, &ObjectAttributes);
    }

    /* Return to caller */
    return Status;
}

NTSTATUS 
NTAPI 
LdrQueryImageFileKeyOption( 	
	IN HANDLE  	KeyHandle,
	IN PCWSTR  	ValueName,
	IN ULONG  	Type,
	OUT PVOID  	Buffer,
	IN ULONG  	BufferSize,
	OUT PULONG ReturnedLength  	OPTIONAL 
	) 		
{
    ULONG KeyInfo[256];
    UNICODE_STRING ValueNameString, IntegerString;
    ULONG KeyInfoSize, ResultSize;
    PKEY_VALUE_PARTIAL_INFORMATION KeyValueInformation = (PKEY_VALUE_PARTIAL_INFORMATION)&KeyInfo;
    BOOLEAN FreeHeap = FALSE;
    NTSTATUS Status;

    /* Build a string for the value name */
    Status = RtlInitUnicodeStringEx(&ValueNameString, ValueName);
    if (!NT_SUCCESS(Status)) return Status;

    /* Query the value */
    Status = ZwQueryValueKey(KeyHandle,
                             &ValueNameString,
                             KeyValuePartialInformation,
                             KeyValueInformation,
                             sizeof(KeyInfo),
                             &ResultSize);
    if (Status == STATUS_BUFFER_OVERFLOW)
    {
        /* Our local buffer wasn't enough, allocate one */
        KeyInfoSize = sizeof(KEY_VALUE_PARTIAL_INFORMATION) +
                      KeyValueInformation->DataLength;
        KeyValueInformation = RtlAllocateHeap(RtlGetProcessHeap(),
                                              0,
                                              KeyInfoSize);
        if (KeyValueInformation != NULL)
        {
            /* Try again */
            Status = ZwQueryValueKey(KeyHandle,
                                     &ValueNameString,
                                     KeyValuePartialInformation,
                                     KeyValueInformation,
                                     KeyInfoSize,
                                     &ResultSize);
            FreeHeap = TRUE;
        }
        else
        {
            /* Give up this time */
            Status = STATUS_NO_MEMORY;
        }
    }

    /* Check for success */
    if (NT_SUCCESS(Status))
    {
        /* Handle binary data */
        if (KeyValueInformation->Type == REG_BINARY)
        {
            /* Check validity */
            if ((Buffer) && (KeyValueInformation->DataLength <= BufferSize))
            {
                /* Copy into buffer */
                RtlMoveMemory(Buffer,
                              &KeyValueInformation->Data,
                              KeyValueInformation->DataLength);
            }
            else
            {
                Status = STATUS_BUFFER_OVERFLOW;
            }

            /* Copy the result length */
            if (ReturnedLength) *ReturnedLength = KeyValueInformation->DataLength;
        }
        else if (KeyValueInformation->Type == REG_DWORD)
        {
            /* Check for valid type */
            if (KeyValueInformation->Type != Type)
            {
                /* Error */
                Status = STATUS_OBJECT_TYPE_MISMATCH;
            }
            else
            {
                /* Check validity */
                if ((Buffer) &&
                    (BufferSize == sizeof(ULONG)) &&
                    (KeyValueInformation->DataLength <= BufferSize))
                {
                    /* Copy into buffer */
                    RtlMoveMemory(Buffer,
                                  &KeyValueInformation->Data,
                                  KeyValueInformation->DataLength);
                }
                else
                {
                    Status = STATUS_BUFFER_OVERFLOW;
                }

                /* Copy the result length */
                if (ReturnedLength) *ReturnedLength = KeyValueInformation->DataLength;
            }
        }
        else if (KeyValueInformation->Type != REG_SZ)
        {
            /* We got something weird */
            Status = STATUS_OBJECT_TYPE_MISMATCH;
        }
        else
        {
            /*  String, check what you requested */
            if (Type == REG_DWORD)
            {
                /* Validate */
                if (BufferSize != sizeof(ULONG))
                {
                    /* Invalid size */
                    BufferSize = 0;
                    Status = STATUS_INFO_LENGTH_MISMATCH;
                }
                else
                {
                    /* OK, we know what you want... */
                    IntegerString.Buffer = (PWSTR)KeyValueInformation->Data;
                    IntegerString.Length = (USHORT)KeyValueInformation->DataLength -
                                           sizeof(WCHAR);
                    IntegerString.MaximumLength = (USHORT)KeyValueInformation->DataLength;
                    Status = RtlUnicodeStringToInteger(&IntegerString, 0, (PULONG)Buffer);
                }
            }
            else
            {
                /* Validate */
                if (KeyValueInformation->DataLength > BufferSize)
                {
                    /* Invalid */
                    Status = STATUS_BUFFER_OVERFLOW;
                }
                else
                {
                    /* Set the size */
                    BufferSize = KeyValueInformation->DataLength;
                }

                /* Copy the string */
                RtlMoveMemory(Buffer, &KeyValueInformation->Data, BufferSize);
            }

            /* Copy the result length */
            if (ReturnedLength) *ReturnedLength = KeyValueInformation->DataLength;
        }
    }

    /* Check if buffer was in heap */
    if (FreeHeap) RtlFreeHeap(RtlGetProcessHeap(), 0, KeyValueInformation);

	DbgPrint("Function: LdrQueryImageFileKeyOption. Status: %08x\n",Status);	
	
    /* Return status */
    return Status;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
LdrQueryImageFileExecutionOptionsEx(
	IN PUNICODE_STRING SubKey,
    IN PCWSTR ValueName,
    IN ULONG Type,
    OUT PVOID Buffer,
    IN ULONG BufferSize,
    OUT PULONG ReturnedLength OPTIONAL,
    IN BOOLEAN Wow64)
{
    NTSTATUS Status;
    HANDLE KeyHandle;

    /* Open a handle to the key */
    Status = LdrOpenImageFileOptionsKey(SubKey, Wow64, &KeyHandle);

    /* Check for success */
    if (NT_SUCCESS(Status))
    {
        /* Query the data */
        Status = LdrQueryImageFileKeyOption(KeyHandle,
                                            ValueName,
                                            Type,
                                            Buffer,
                                            BufferSize,
                                            ReturnedLength);

        /* Close the key */
        NtClose(KeyHandle);
    }

	DbgPrint("Function: LdrQueryImageFileExecutionOptionsEx. Status: %08x\n",Status);
    /* Return to caller */
    return Status;
}

NTSTATUS
NTAPI
LdrGetProcedureAddressEx(
    __in PVOID DllHandle,
    __in_opt PANSI_STRING ProcedureName,
    __in_opt ULONG ProcedureNumber,
    __out PVOID *ProcedureAddress,
    __in ULONG Flags
)
{
	//The flags are unknown
	return LdrGetProcedureAddress(DllHandle,
								  ProcedureName,
								  ProcedureNumber,
								  ProcedureAddress);
}

NTSTATUS 
LdrAddLoadAsDataTable(
	PVOID Str2, 
	PWSTR FilePath,
	SIZE_T a3, 
	int a4)
{
	return STATUS_NOT_IMPLEMENTED;
}


BOOLEAN
NTAPI
LdrpDisableProcessCompatGuidDetection(VOID)
{
    UNICODE_STRING PolicyKey = RTL_CONSTANT_STRING(L"\\Registry\\MACHINE\\Software\\Policies\\Microsoft\\Windows\\AppCompat");
    UNICODE_STRING DisableDetection = RTL_CONSTANT_STRING(L"DisableCompatGuidDetection");
    OBJECT_ATTRIBUTES PolicyKeyAttributes = RTL_CONSTANT_OBJECT_ATTRIBUTES(&PolicyKey, OBJ_CASE_INSENSITIVE);
    KEY_VALUE_PARTIAL_INFORMATION KeyInfo;
    ULONG ResultLength;
    NTSTATUS Status;
    HANDLE KeyHandle;

    Status = NtOpenKey(&KeyHandle, KEY_QUERY_VALUE, &PolicyKeyAttributes);
    if (NT_SUCCESS(Status))
    {
        Status = NtQueryValueKey(KeyHandle,
                                 &DisableDetection,
                                 KeyValuePartialInformation,
                                 &KeyInfo,
                                 sizeof(KeyInfo),
                                 &ResultLength);
        NtClose(KeyHandle);
        if ((NT_SUCCESS(Status)) &&
            (KeyInfo.Type == REG_DWORD) &&
            (KeyInfo.DataLength == sizeof(ULONG)) &&
            (KeyInfo.Data[0] == TRUE))
        {
            return TRUE;
        }
    }
    return FALSE;
}

VOID
NTAPI
LdrpInitializeProcessCompat(PVOID pProcessActctx, PVOID* pOldShimData)
{
    static const struct
    {
        const GUID* Guid;
        const DWORD Version;
    } KnownCompatGuids[] = {
        { &COMPAT_GUID_WIN10, _WIN32_WINNT_WIN10 },
        { &COMPAT_GUID_WIN81, _WIN32_WINNT_WINBLUE },
        { &COMPAT_GUID_WIN8, _WIN32_WINNT_WIN8 },
        { &COMPAT_GUID_WIN7, _WIN32_WINNT_WIN7 },
        { &COMPAT_GUID_VISTA, _WIN32_WINNT_VISTA },
    };

    ULONG Buffer[(sizeof(COMPATIBILITY_CONTEXT_ELEMENT) * 10 + sizeof(ACTIVATION_CONTEXT_COMPATIBILITY_INFORMATION)) / sizeof(ULONG)];
    ACTIVATION_CONTEXT_COMPATIBILITY_INFORMATION* ContextCompatInfo;
    SIZE_T SizeRequired;
    NTSTATUS Status;
    DWORD n, cur;
    ReactOS_ShimData* pShimData = *pOldShimData;

    if (pShimData)
    {
        if (pShimData->dwMagic != REACTOS_SHIMDATA_MAGIC ||
            pShimData->dwSize != sizeof(ReactOS_ShimData))
        {
            DPRINT1("LdrpInitializeProcessCompat: Corrupt pShimData (0x%x, %u)\n", pShimData->dwMagic, pShimData->dwSize);
            return;
        }
        if (pShimData->dwRosProcessCompatVersion)
        {
            if (pShimData->dwRosProcessCompatVersion == REACTOS_COMPATVERSION_IGNOREMANIFEST)
            {
                DPRINT1("LdrpInitializeProcessCompat: ProcessCompatVersion set to ignore manifest\n");
            }
            else
            {
                DPRINT1("LdrpInitializeProcessCompat: ProcessCompatVersion already set to 0x%x\n", pShimData->dwRosProcessCompatVersion);
            }
            return;
        }
    }

    SizeRequired = sizeof(Buffer);
    Status = RtlQueryInformationActivationContext(RTL_QUERY_ACTIVATION_CONTEXT_FLAG_NO_ADDREF,
                                                  pProcessActctx,
                                                  NULL,
                                                  CompatibilityInformationInActivationContext,
                                                  Buffer,
                                                  sizeof(Buffer),
                                                  &SizeRequired);

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("LdrpInitializeProcessCompat: Unable to query process actctx with status %x\n", Status);
        return;
    }

    ContextCompatInfo = (ACTIVATION_CONTEXT_COMPATIBILITY_INFORMATION*)Buffer;
    /* No Compatibility elements present, bail out */
    if (ContextCompatInfo->ElementCount == 0)
        return;

    /* Search for known GUIDs, starting from oldest to newest.
       Note that on Windows it is somewhat reversed, starting from the latest known
       version, going down. But we are not Windows, trying to allow a lower version,
       we are ReactOS trying to fake a higher version. So we interpret what Windows
       does as "try the closest version to the actual version", so we start with the
       lowest version, which is closest to Windows 2003, which we mostly are. */
    for (cur = RTL_NUMBER_OF(KnownCompatGuids) - 1; cur != -1; --cur)
    {
        for (n = 0; n < ContextCompatInfo->ElementCount; ++n)
        {
            if (ContextCompatInfo->Elements[n].Type == ACTCTX_COMPATIBILITY_ELEMENT_TYPE_OS &&
                RtlCompareMemory(&ContextCompatInfo->Elements[n].Id, KnownCompatGuids[cur].Guid, sizeof(GUID)) == sizeof(GUID))
            {
                if (LdrpDisableProcessCompatGuidDetection())
                {
                    DPRINT1("LdrpInitializeProcessCompat: Not applying automatic fix for winver 0x%x due to policy\n", KnownCompatGuids[cur].Version);
                    return;
                }

                /* If this process did not need shim data before, allocate and store it */
                if (pShimData == NULL)
                {
                    PPEB Peb = NtCurrentPeb();

                    ASSERT(Peb->pShimData == NULL);
                    pShimData = RtlAllocateHeap(Peb->ProcessHeap, HEAP_ZERO_MEMORY, sizeof(*pShimData));

                    if (!pShimData)
                    {
                        DPRINT1("LdrpInitializeProcessCompat: Unable to allocated %u bytes\n", sizeof(*pShimData));
                        return;
                    }

                    pShimData->dwSize = sizeof(*pShimData);
                    pShimData->dwMagic = REACTOS_SHIMDATA_MAGIC;

                    Peb->pShimData = pShimData;
                    *pOldShimData = pShimData;
                }

                /* Store the lowest found version, and bail out. */
                pShimData->dwRosProcessCompatVersion = KnownCompatGuids[cur].Version;
                DPRINT1("LdrpInitializeProcessCompat: Found guid for winver 0x%x in manifest from %wZ\n",
                        KnownCompatGuids[cur].Version,
                        &(NtCurrentPeb()->ProcessParameters->ImagePathName));
                return;
            }
        }
    }
}
