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
#define NDEBUG 
 
#include <main.h>

BOOLEAN globalVerificationTablet = TRUE;
BOOLEAN globalVerificationMediaCenter = TRUE;
BOOLEAN globalVerificationAppliance = TRUE;

#define SystemLogicalProcessorInformationEx 107

typedef struct _SYSTEM_LOGICAL_INFORMATION_FILLED{
	CACHE_DESCRIPTOR  CacheLevel1;
	CACHE_DESCRIPTOR  CacheLevel2;
	CACHE_DESCRIPTOR  CacheLevel3;
	DWORD PackagesNumber;
	DWORD CoresNumber;
	DWORD LogicalProcessorsNumber;
	DWORD NumaNumber;
}SYSTEM_LOGICAL_INFORMATION_FILLED, *PSYSTEM_LOGICAL_INFORMATION_FILLED;

BOOLEAN 
IsVersionInstalled(LSA_UNICODE_STRING *Handle)
{
  BOOLEAN verification; // bl@1
  OBJECT_ATTRIBUTES ObjectAttributes; // [sp+4h] [bp-38h]@1
  BOOLEAN KeyValueInformation; // [sp+1Ch] [bp-20h]@2
  int value = 0; // [sp+28h] [bp-14h]@3
  UNICODE_STRING DestinationString; // [sp+30h] [bp-Ch]@2
  ULONG ResultLength; // [sp+38h] [bp-4h]@2

  ObjectAttributes.ObjectName = Handle;
  verification = 0;
  ObjectAttributes.Length = 24;
  ObjectAttributes.RootDirectory = 0;
  ObjectAttributes.Attributes = 64;
  ObjectAttributes.SecurityDescriptor = 0;
  ObjectAttributes.SecurityQualityOfService = 0;
  if ( NtOpenKey((PHANDLE)&Handle, 0x2000000u, &ObjectAttributes) >= 0 )
  {
    RtlInitUnicodeString(&DestinationString, L"Installed");
    if ( ZwQueryValueKey(
           Handle,
           &DestinationString,
           KeyValuePartialInformation,
           &KeyValueInformation,
           0x14u,
           &ResultLength) >= 0
      && value )
      verification = 1;
    ZwClose(Handle);
  }
  return verification;
}

BOOLEAN 
IsMediaCenterInstalled()
{
  BOOLEAN result; // al@1
  UNICODE_STRING DestinationString; // [sp+0h] [bp-8h]@2

  result = globalVerificationMediaCenter;
  if ( globalVerificationMediaCenter == -1 )
  {
    RtlInitUnicodeString(&DestinationString, L"\\Registry\\Machine\\System\\WPA\\MediaCenter");
    result = IsVersionInstalled(&DestinationString);
    globalVerificationMediaCenter = result;
  }
  return result;
}

BOOLEAN 
IsTabletEdtionInstalled()
{
  BOOLEAN result; // al@1
  UNICODE_STRING DestinationString; // [sp+0h] [bp-8h]@2

  result = globalVerificationTablet;
  if ( globalVerificationTablet == -1 )
  {
    RtlInitUnicodeString(&DestinationString, L"\\Registry\\Machine\\System\\WPA\\TabletPC");
    result = IsVersionInstalled(&DestinationString);
    globalVerificationTablet = result;
  }
  return result;
}

BOOLEAN 
IsApplianceInstalled()
{
  BOOLEAN result; // al@1
  UNICODE_STRING DestinationString; // [sp+0h] [bp-8h]@2

  result = globalVerificationAppliance;
  if ( globalVerificationAppliance == -1 )
  {
    RtlInitUnicodeString(&DestinationString, L"\\Registry\\Machine\\System\\WPA\\ApplianceServer");
    result = IsVersionInstalled(&DestinationString);
    globalVerificationAppliance = result;
  }
  return result;
}

NTSTATUS 
RtlGetOSProductNameStringHelper(
	PUNICODE_STRING Destination, 
	BOOLEAN verification, 
	PCWSTR Source
)
{
  NTSTATUS result; // eax@2

  if ( !verification || (result = RtlAppendUnicodeToString(Destination, Source), result >= 0) )
    result = RtlAppendUnicodeToString(Destination, Source);
  return result;
}

NTSTATUS 
NTAPI
RtlGetOSProductName(
	PUNICODE_STRING Destination, 
	int verification
)
{
  NTSTATUS result; // eax@2
  LSA_UNICODE_STRING *localDestination; // esi@3
  const WCHAR *windowsString; // eax@10
  const WCHAR *edition; // eax@16
  const WCHAR *windowsStringHelper; // [sp-8h] [bp-128h]@8
  struct _OSVERSIONINFOW VersionInformation; // [sp+4h] [bp-11Ch]@4
  int other = 1; // [sp+11Ch] [bp-4h]@10
  BOOLEAN Destinationa; // [sp+128h] [bp+8h]@5

  if ( !verification )
    return STATUS_INVALID_PARAMETER;
  localDestination = Destination;
  if ( Destination->MaximumLength <= 0u )
    return STATUS_BUFFER_TOO_SMALL;
  *Destination->Buffer = 0;
  Destination->Length = 0;
  VersionInformation.dwOSVersionInfoSize = sizeof(RTL_OSVERSIONINFOEXW);
  result = RtlGetVersion(&VersionInformation);
  if ( result < 0 )
    return result;
  Destinationa = 0;
  if ( verification & 1 )
  {
    result = RtlGetOSProductNameStringHelper(localDestination, 0, L"Microsoft");
    Destinationa = 1;
  }
  if ( verification & 2 )
  {
    windowsStringHelper = L"Windows";
LABEL_13:
    result = RtlGetOSProductNameStringHelper(localDestination, Destinationa, windowsStringHelper);
    Destinationa = 1;
    goto LABEL_14;
  }
  if ( verification & 4 )
  {
    windowsString = L"Longhorn XP";
    if ( other != 1 )
      windowsString = L"Longhorn .Net";
    windowsStringHelper = windowsString;
    goto LABEL_13;
  }
LABEL_14:
  if ( verification & 8 )
  {
    if ( other & 0x40 )
    {
      edition = L"Embedded";
    }
    else
    {
      if ( IsMediaCenterInstalled() )
      {
        edition = L"Media Center Edition";
      }
      else
      {
        if ( IsTabletEdtionInstalled() )
        {
          edition = L"Tablet PC Edition";
        }
        else
        {
          if ( other & 2 )
          {
            edition = L"Home Edition";
          }
          else
          {
            if ( other == 1 )
            {
              edition = L"Professional";
            }
            else
            {
              if ( IsApplianceInstalled() )
              {
                edition = L"Appliance Server";
              }
              else
              {
                if ( other & 4 )
                {
                  edition = L"Web Server";
                }
                else
                {
                  if ( other & 0x21 )
                  {
                    edition = L"Small Business Server";
                  }
                  else
                  {
                    if ( (char)other >= 0 )
                    {
                      if ( other & 2 )
                        edition = L"Enterprise Server";
                      else
                        edition = L"Standard Server";
                    }
                    else
                    {
                      edition = L"Datacenter Server";
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
    result = RtlGetOSProductNameStringHelper(localDestination, Destinationa, edition);
    Destinationa = 1;
  }
  if ( verification & 0x10 )
  {
    result = RtlGetOSProductNameStringHelper(localDestination, Destinationa, L"Version 2003");
    Destinationa = 1;
  }
  if ( verification & 0x20 && VersionInformation.szCSDVersion[0] )
  {
    result = RtlGetOSProductNameStringHelper(localDestination, Destinationa, VersionInformation.szCSDVersion);
    Destinationa = 1;
  }
  if ( verification & 0x40 )
    result = RtlGetOSProductNameStringHelper(localDestination, Destinationa, L"Copyright ");
  return result;
}

/***********************************************************************
 *           RtlGetProductInfo    (NTDLL.@)
 *
 * Gives info about the current Windows product type, in a format compatible
 * with the given Windows version
 *
 * Returns TRUE if the input is valid, FALSE otherwise
 */
BOOLEAN 
WINAPI 
RtlGetProductInfo(
	DWORD dwOSMajorVersion, 
	DWORD dwOSMinorVersion, 
	DWORD dwSpMajorVersion,
    DWORD dwSpMinorVersion, 
	PDWORD pdwReturnedProductType
)
{
    RTL_OSVERSIONINFOEXW VersionInformation;
	
	VersionInformation.dwOSVersionInfoSize = sizeof(RTL_OSVERSIONINFOEXW);
	
	RtlGetVersion((PRTL_OSVERSIONINFOW)&VersionInformation);

    if (!pdwReturnedProductType)
        return FALSE;

    if (VersionInformation.wProductType == VER_NT_WORKSTATION)
	{
		if(VersionInformation.wSuiteMask == VER_SUITE_PERSONAL)
			*pdwReturnedProductType = PRODUCT_HOME_PREMIUM;
		else
			*pdwReturnedProductType = PRODUCT_ULTIMATE;
	}else{
		if(VersionInformation.wSuiteMask == VER_SUITE_BLADE)
			*pdwReturnedProductType = PRODUCT_WEB_SERVER;
		if(VersionInformation.wSuiteMask == VER_SUITE_COMPUTE_SERVER)
			*pdwReturnedProductType = PRODUCT_CLUSTER_SERVER;
		if(VersionInformation.wSuiteMask == VER_SUITE_DATACENTER)
			*pdwReturnedProductType = PRODUCT_DATACENTER_SERVER;
		if(VersionInformation.wSuiteMask == VER_SUITE_ENTERPRISE)
			*pdwReturnedProductType = PRODUCT_ENTERPRISE_SERVER;
		if(VersionInformation.wSuiteMask == VER_SUITE_SMALLBUSINESS)
			*pdwReturnedProductType = PRODUCT_SMALLBUSINESS_SERVER;		
		if(VersionInformation.wSuiteMask == VER_SUITE_SMALLBUSINESS_RESTRICTED)
			*pdwReturnedProductType = PRODUCT_SB_SOLUTION_SERVER;	
		if(VersionInformation.wSuiteMask == VER_SUITE_STORAGE_SERVER)
			*pdwReturnedProductType = PRODUCT_STORAGE_ENTERPRISE_SERVER;		
		if(VersionInformation.wSuiteMask == VER_SUITE_WH_SERVER)
			*pdwReturnedProductType = PRODUCT_HOME_PREMIUM_SERVER;			
	}        

    return TRUE;
}

/*************************************************************************
 * NtQueryLicenseValue   [NTDLL.@]
 *
 * NOTES
 *  On Windows all license properties are stored in a single key, but
 *  unless there is some app which explicitly depends on that, there is
 *  no good reason to reproduce that.
 */
NTSTATUS 
WINAPI 
NtQueryLicenseValue( 
	const UNICODE_STRING *name, 
	ULONG *result_type,
    PVOID data, 
	ULONG length, 
	ULONG *result_len 
)
{
    static const WCHAR LicenseInformationW[] = {'M','a','c','h','i','n','e','\\',
                                                'S','o','f','t','w','a','r','e','\\',
												'M','i','c','r','o','s','o','f','t','\\',
                                                'W','i','n','d','o','w','s','\\','L','i','c','e','n','s','e',
                                                'I','n','f','o','r','m','a','t','i','o','n',0};
    KEY_VALUE_PARTIAL_INFORMATION *info;
    NTSTATUS status = STATUS_OBJECT_NAME_NOT_FOUND;
    DWORD info_length, count;
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING keyW;
    HANDLE hkey;

    if (!name || !name->Buffer || !name->Length || !result_len)
        return STATUS_INVALID_PARAMETER;

    info_length = FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION, Data) + length;
    info = RtlAllocateHeap( RtlProcessHeap(), 0, info_length );
    if (!info) return STATUS_NO_MEMORY;

    attr.Length = sizeof(attr);
    attr.RootDirectory = 0;
    attr.ObjectName = &keyW;
    attr.Attributes = 0;
    attr.SecurityDescriptor = NULL;
    attr.SecurityQualityOfService = NULL;
    RtlInitUnicodeString( &keyW, LicenseInformationW );

    /* @@ Wine registry key: HKLM\Software\Microsoft\Windows\LicenseInformation */
    if (!NtOpenKey( &hkey, KEY_READ, &attr ))
    {
        status = NtQueryValueKey( hkey, name, KeyValuePartialInformation,
                                  info, info_length, &count );
        if (!status || status == STATUS_BUFFER_OVERFLOW)
        {
            if (result_type)
                *result_type = info->Type;

            *result_len = info->DataLength;

            if (status == STATUS_BUFFER_OVERFLOW)
                status = STATUS_BUFFER_TOO_SMALL;
            else
                memcpy( data, info->Data, info->DataLength );
        }
        NtClose( hkey );
    }

    if (status == STATUS_OBJECT_NAME_NOT_FOUND)
        DbgPrint( "License key %s not found\n",name->Buffer );

    RtlFreeHeap( RtlProcessHeap(), 0, info );
    return status;
}

void 
WINAPI 
RtlGetNtVersionNumbers(
  LPDWORD major,
  LPDWORD minor,
  LPDWORD build
)
{
	if(major)
		*major = 6;
	if(minor)
		*minor = 0;
	if(build)
		*build = 3790;
}

static BOOL grow_logical_proc_buf(SYSTEM_LOGICAL_PROCESSOR_INFORMATION **pdata,
        SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX **pdataex, DWORD *max_len)
{
    if (pdata)
    {
        SYSTEM_LOGICAL_PROCESSOR_INFORMATION *new_data;

        *max_len *= 2;
        new_data = RtlReAllocateHeap(NtCurrentTeb()->ProcessEnvironmentBlock->ProcessHeap, 0, *pdata, *max_len*sizeof(*new_data));
        if (!new_data)
            return FALSE;

        *pdata = new_data;
    }
    else
    {
        SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX *new_dataex;

        *max_len *= 2;
        new_dataex = RtlReAllocateHeap(NtCurrentTeb()->ProcessEnvironmentBlock->ProcessHeap, HEAP_ZERO_MEMORY, *pdataex, *max_len*sizeof(*new_dataex));
        if (!new_dataex)
            return FALSE;

        *pdataex = new_dataex;
    }

    return TRUE;
}

static DWORD log_proc_ex_size_plus(DWORD size)
{
    /* add SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX.Relationship and .Size */
    return sizeof(LOGICAL_PROCESSOR_RELATIONSHIP) + sizeof(DWORD) + size;
}

static inline BOOL logical_proc_info_add_by_id(
    SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX **pdataex, 
	DWORD *len, 
	DWORD *pmax_len,
    LOGICAL_PROCESSOR_RELATIONSHIP rel, 
	DWORD id, 
	ULONG_PTR mask
)
{
        SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX *dataex;
        DWORD ofs = 0;

        while(ofs < *len)
        {
            dataex = (SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX *)(((char *)*pdataex) + ofs);
            if (rel == RelationProcessorPackage && dataex->Relationship == rel && dataex->Processor.Reserved[1] == id)
            {
                dataex->Processor.GroupMask[0].Mask |= mask;
                return TRUE;
            }
            ofs += dataex->Size;
        }

        /* TODO: For now, just one group. If more than 64 processors, then we
         * need another group. */

        while (ofs + log_proc_ex_size_plus(sizeof(PROCESSOR_RELATIONSHIP)) > *pmax_len)
        {
            if (!grow_logical_proc_buf(NULL, pdataex, pmax_len))
                return FALSE;
        }

        dataex = (SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX *)(((char *)*pdataex) + ofs);

        dataex->Relationship = rel;
        dataex->Size = log_proc_ex_size_plus(sizeof(PROCESSOR_RELATIONSHIP));
        dataex->Processor.Flags = 0; /* TODO */
        //dataex->Processor.EfficiencyClass = 0;
        dataex->Processor.GroupCount = 1;
        dataex->Processor.GroupMask[0].Mask = mask;
        dataex->Processor.GroupMask[0].Group = 0;
        /* mark for future lookup */
        dataex->Processor.Reserved[0] = 0;
        dataex->Processor.Reserved[1] = id;

        *len += dataex->Size;
    return TRUE;
}

static inline BOOL logical_proc_info_add_cache(
	SYSTEM_LOGICAL_PROCESSOR_INFORMATION **pdata,
    SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX **pdataex, 
	DWORD *len,
    DWORD *pmax_len, 
	ULONG_PTR mask, 
	CACHE_DESCRIPTOR *cache
)
{
        SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX *dataex;
        DWORD ofs;

        for (ofs = 0; ofs < *len; )
        {
            dataex = (SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX *)(((char *)*pdataex) + ofs);
            if (dataex->Relationship == RelationCache && dataex->Cache.GroupMask.Mask == mask &&
                    dataex->Cache.Level == cache->Level && dataex->Cache.Type == cache->Type)
                return TRUE;
            ofs += dataex->Size;
        }

        while (ofs + log_proc_ex_size_plus(sizeof(CACHE_RELATIONSHIP)) > *pmax_len)
        {
            if (!grow_logical_proc_buf(NULL, pdataex, pmax_len))
                return FALSE;
        }

        dataex = (SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX *)(((char *)*pdataex) + ofs);

        dataex->Relationship = RelationCache;
        dataex->Size = log_proc_ex_size_plus(sizeof(CACHE_RELATIONSHIP));
        dataex->Cache.Level = cache->Level;
        dataex->Cache.Associativity = cache->Associativity;
        dataex->Cache.LineSize = cache->LineSize;
        dataex->Cache.CacheSize = cache->Size;
        dataex->Cache.Type = cache->Type;
        dataex->Cache.GroupMask.Mask = mask;
        dataex->Cache.GroupMask.Group = 0;
        dataex->Cache.GroupCount = 1;

        *len += dataex->Size;
    return TRUE;
}

static inline BOOL logical_proc_info_add_numa_node(
	SYSTEM_LOGICAL_PROCESSOR_INFORMATION **pdata,
    SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX **pdataex, 
	DWORD *len, 
	DWORD *pmax_len, 
	ULONG_PTR mask,
    DWORD node_id
)
{
        SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX *dataex;

        while (*len + log_proc_ex_size_plus(sizeof(NUMA_NODE_RELATIONSHIP)) > *pmax_len)
        {
            if (!grow_logical_proc_buf(NULL, pdataex, pmax_len))
                return FALSE;
        }

        dataex = (SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX *)(((char *)*pdataex) + *len);

        dataex->Relationship = RelationNumaNode;
        dataex->Size = log_proc_ex_size_plus(sizeof(NUMA_NODE_RELATIONSHIP));
        dataex->NumaNode.NodeNumber = node_id;
        dataex->NumaNode.GroupMask.Mask = mask;
        dataex->NumaNode.GroupMask.Group = 0;
		dataex->NumaNode.GroupCount = 0;

        *len += dataex->Size;
    return TRUE;
}

static inline BOOL LogicalProcessorInfoAddGroupInfo(
	SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX **pdataex,
    DWORD *len, 
	DWORD *pmax_len, 
	DWORD num_cpus, 
	ULONG_PTR mask
)
{
    SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX *dataex;

    while (*len + log_proc_ex_size_plus(sizeof(GROUP_RELATIONSHIP)) > *pmax_len)
    {
        if (!grow_logical_proc_buf(NULL, pdataex, pmax_len))
            return FALSE;
    }

    dataex = (SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX *)(((char *)*pdataex) + *len);

    dataex->Relationship = RelationGroup;
    dataex->Size = log_proc_ex_size_plus(sizeof(GROUP_RELATIONSHIP));
    dataex->Group.MaximumGroupCount = 1;
    dataex->Group.ActiveGroupCount = 1;
    dataex->Group.GroupInfo[0].MaximumProcessorCount = num_cpus;
    dataex->Group.GroupInfo[0].ActiveProcessorCount = num_cpus;
    dataex->Group.GroupInfo[0].ActiveProcessorMask = mask;

    *len += dataex->Size;

    return TRUE;
}
//Helper function to count set bits in the processor mask.
DWORD CountSetBits(ULONG_PTR bitMask)
{
    DWORD LSHIFT = sizeof(ULONG_PTR)*8 - 1;
    DWORD bitSetCount = 0;
    ULONG_PTR bitTest = (ULONG_PTR)1 << LSHIFT;    
    DWORD i;
    
    for (i = 0; i <= LSHIFT; ++i)
    {
        bitSetCount += ((bitMask & bitTest)?1:0);
        bitTest/=2;
    }

    return bitSetCount;
}

//Code based in example on documentation https://learn.microsoft.com/pt-br/windows/win32/api/sysinfoapi/nf-sysinfoapi-getlogicalprocessorinformation
SYSTEM_LOGICAL_INFORMATION_FILLED GetLogicalInfo()
{
    BOOL done = FALSE;
    PSYSTEM_LOGICAL_PROCESSOR_INFORMATION buffer = NULL;
    PSYSTEM_LOGICAL_PROCESSOR_INFORMATION ptr = NULL;
	SYSTEM_LOGICAL_INFORMATION_FILLED fill = {1};
    DWORD returnLength = 0;
    DWORD logicalProcessorCount = 0;
    DWORD numaNodeCount = 0;
    DWORD processorCoreCount = 0;
    DWORD processorL1CacheCount = 0;
    DWORD processorL2CacheCount = 0;
    DWORD processorL3CacheCount = 0;
    DWORD processorPackageCount = 1;
    DWORD byteOffset = 0;
    PCACHE_DESCRIPTOR Cache;
	NTSTATUS Status;

    // glpi = (LPFN_GLPI) GetProcAddress(
                            // GetModuleHandle(TEXT("kernel32")),
                            // "GetLogicalProcessorInformation");
    // if (NULL == glpi) 
    // {
        // DbgPrint(TEXT("\nGetLogicalProcessorInformation is not supported.\n"));
        // return fill;
    // }
	
	

    while (!done)
    {		
		Status = NtQuerySystemInformation(SystemLogicalProcessorInformation,
										  buffer,
										  returnLength,
										  &returnLength);

		/* Normalize the error to what Win32 expects */
		if (Status == STATUS_INFO_LENGTH_MISMATCH) 
			Status = STATUS_BUFFER_TOO_SMALL;

        if (!NT_SUCCESS(Status)) 
        {
            if (Status == STATUS_BUFFER_TOO_SMALL) 
            {
                if (buffer) 
                    RtlFreeHeap(NtCurrentTeb()->ProcessEnvironmentBlock->ProcessHeap,0,buffer);

                buffer = (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION)RtlAllocateHeap(NtCurrentTeb()->ProcessEnvironmentBlock->ProcessHeap, HEAP_ZERO_MEMORY, returnLength);

                if (NULL == buffer) 
                {
                    DbgPrint("\nError: Allocation failure\n");
                    return fill;
                }
            } 
            else 
            {
                //DbgPrint(TEXT("\nError %d\n"), GetLastError());
                return fill;
            }
        } 
        else
        {
            done = TRUE;
        }
    }

    ptr = buffer;

    while (byteOffset + sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION) <= returnLength) 
    {
        switch (ptr->Relationship) 
        {
        case RelationNumaNode:
            // Non-NUMA systems report a single record of this type.
            numaNodeCount++;
            break;

        case RelationProcessorCore:
            processorCoreCount++;

            // A hyperthreaded core supplies more than one logical processor.
            logicalProcessorCount += CountSetBits(ptr->ProcessorMask);
            break;

        case RelationCache:
            // Cache data is in ptr->Cache, one CACHE_DESCRIPTOR structure for each cache. 
            Cache = &ptr->Cache;
			
            if (Cache->Level == 1)
            {
                processorL1CacheCount++;
				fill.CacheLevel1 = *Cache;
            }
            else if (Cache->Level == 2)
            {
				fill.CacheLevel2 = *Cache;
                processorL2CacheCount++;
            }
            else if (Cache->Level == 3)
            {
				fill.CacheLevel3 = *Cache;
                processorL3CacheCount++;
            }
            break;

        case RelationProcessorPackage:
            // Logical processors share a physical package.
			DbgPrint("processor Packages\n");
            processorPackageCount++;
            break;

        default:
            DbgPrint("\nError: Unsupported LOGICAL_PROCESSOR_RELATIONSHIP value.\n");
            break;
        }
        byteOffset += sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION);
        ptr++;
    }
	
	fill.NumaNumber = numaNodeCount;
	fill.PackagesNumber = processorPackageCount;
	fill.CoresNumber = processorCoreCount;
	fill.LogicalProcessorsNumber = logicalProcessorCount;
	
	DbgPrint("Number of processor packages is %d\n", processorPackageCount);
    
    RtlFreeHeap(NtCurrentTeb()->ProcessEnvironmentBlock->ProcessHeap,0,buffer);

    return fill;
}

/* for 'data', max_len is the array count. for 'dataex', max_len is in bytes */
static 
NTSTATUS 
CreateLogicalProcessorInfo(
	SYSTEM_LOGICAL_PROCESSOR_INFORMATION **data,
    SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX **dataex, 
	DWORD *max_len
)
{
    DWORD pkgs_no, cores_no, lcpu_no, lcpu_per_core, cores_per_package, len = 0;
    DWORD cache_ctrs[10] = {0};
    ULONG_PTR all_cpus_mask = 0;
    CACHE_DESCRIPTOR cache[10];
    LONGLONG cache_sharing[10];
    DWORD p,i,j,k;
	SYSTEM_LOGICAL_INFORMATION_FILLED fill;
	
	fill = GetLogicalInfo();

    lcpu_no = fill.LogicalProcessorsNumber;
	
	pkgs_no = fill.PackagesNumber;
	cores_no = fill.CoresNumber;

    DbgPrint("Ntext::CreateLogicalProcessorInfo :: %u logical CPUs from %u physical cores across %u packages\n",
            lcpu_no, cores_no, pkgs_no);

    lcpu_per_core = lcpu_no / cores_no;
    cores_per_package = cores_no / pkgs_no;
	
//(DWORD)32 << 10; /* 16 KB */
    memset(cache, 0, sizeof(cache));
    cache[1].Level = 1;
	cache[1].Size = fill.CacheLevel1.Size;
    cache[1].Type = CacheInstruction;
    cache[1].Associativity = 8; /* reasonable default */
    cache[1].LineSize = 0x40; /* reasonable default */
    cache[2].Level = 1;
	cache[2].Size = fill.CacheLevel1.Size;
    cache[2].Type = CacheData;
    cache[2].Associativity = 8;
    cache[2].LineSize = 0x40;
    cache[3].Level = 2;
	cache[3].Size = fill.CacheLevel2.Size;
    cache[3].Type = CacheUnified;
    cache[3].Associativity = fill.CacheLevel2.Associativity;
    cache[3].LineSize = 0x40;
    cache[4].Level = 3;
	cache[4].Size = fill.CacheLevel3.Size;
    cache[4].Type = CacheUnified;
    cache[4].Associativity = 12;
    cache[4].LineSize = 0x40;
	
    cache_sharing[1] = lcpu_per_core;
    cache_sharing[2] = lcpu_per_core;
    cache_sharing[3] = lcpu_per_core;
    cache_sharing[4] = lcpu_no;

    for(p = 0; p < pkgs_no; ++p){
        for(j = 0; j < cores_per_package && p * cores_per_package + j < cores_no; ++j){
            ULONG_PTR mask = 0;

            for(k = 0; k < lcpu_per_core; ++k)
                mask |= (ULONG_PTR)1 << (j * lcpu_per_core + k);

            all_cpus_mask |= mask;

            /* add to package */
            if(!logical_proc_info_add_by_id(dataex, &len, max_len, RelationProcessorPackage, p, mask))
                return STATUS_NO_MEMORY;

            /* add new core */
            if(!logical_proc_info_add_by_id(dataex, &len, max_len, RelationProcessorCore, p, mask))
                return STATUS_NO_MEMORY;

            for(i = 1; i < 5; ++i){
                if(cache_ctrs[i] == 0 && cache[i].Size > 0){
                    mask = 0;
                    for(k = 0; k < cache_sharing[i]; ++k)
                        mask |= (ULONG_PTR)1 << (j * lcpu_per_core + k);

                    if(!logical_proc_info_add_cache(data, dataex, &len, max_len, mask, &cache[i]))
                        return STATUS_NO_MEMORY;
                }

                cache_ctrs[i] += lcpu_per_core;

                if(cache_ctrs[i] == cache_sharing[i])
                    cache_ctrs[i] = 0;
            }
        }
    }
	
	DbgPrint("CreateLogicalProcessorInfo part 6\n");	

    /* OSX doesn't support NUMA, so just make one NUMA node for all CPUs */
    if(!logical_proc_info_add_numa_node(data, dataex, &len, max_len, all_cpus_mask, 0))
        return STATUS_NO_MEMORY;

    if(dataex)
        LogicalProcessorInfoAddGroupInfo(dataex, &len, max_len, lcpu_no, all_cpus_mask);

    if(data)
        *max_len = len * sizeof(**data);
    else
        *max_len = len;
	
	DbgPrint("CreateLogicalProcessorInfo part 7\n");	

    return STATUS_SUCCESS;
}

/******************************************************************************
 * NtQuerySystemInformationEx [NTDLL.@]
 * ZwQuerySystemInformationEx [NTDLL.@]
 */
NTSTATUS 
NTAPI 
NtQuerySystemInformationEx(
	SYSTEM_INFORMATION_CLASS SystemInformationClass,
    void *Query, 
	ULONG QueryLength, 
	void *SystemInformation, 
	ULONG Length,
	ULONG *ResultLength
)
{
    ULONG len = 0;
    NTSTATUS ret = STATUS_NOT_IMPLEMENTED;
	SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX *buf;

    DbgPrint("(0x%08x,%p,%u,%p,%u,%p) stub\n", SystemInformationClass, Query, QueryLength, SystemInformation,
        Length, ResultLength);

    switch (SystemInformationClass) {
    case SystemLogicalProcessorInformationEx:
       { 
            if (!Query || QueryLength < sizeof(DWORD))
            {
                ret = STATUS_INVALID_PARAMETER;
                return ret;
            }

            if (*(DWORD*)Query != RelationAll)
                DbgPrint("Relationship filtering not implemented: 0x%x\n", *(DWORD*)Query);

             len = 3 * sizeof(*buf);
             buf = RtlAllocateHeap(NtCurrentTeb()->ProcessEnvironmentBlock->ProcessHeap, 0, len);
            if (!buf)
            {
                ret = STATUS_NO_MEMORY;
                return ret;
            }

            ret = CreateLogicalProcessorInfo(NULL, &buf, &len);
			DbgPrint("NtQuerySystemInformationEx:: Status: %0x%08x\n", ret);
            if (ret != STATUS_SUCCESS)
            {
                RtlFreeHeap(NtCurrentTeb()->ProcessEnvironmentBlock->ProcessHeap, 0, buf);
                return ret;
            }

            if (Length >= len)
            {
                if (!SystemInformation)
                    ret = STATUS_ACCESS_VIOLATION;
                else
                    memcpy( SystemInformation, buf, len);
            }
            else
                ret = STATUS_INFO_LENGTH_MISMATCH;

            RtlFreeHeap(NtCurrentTeb()->ProcessEnvironmentBlock->ProcessHeap, 0, buf);

           break;
       }
    default:
		return NtQuerySystemInformation(SystemInformationClass,
										SystemInformation,
										Length,
										ResultLength);
    }

    if (ResultLength)
        *ResultLength = len;

    return ret;
}