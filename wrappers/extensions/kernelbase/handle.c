/*++

Copyright (c) 2017 Shorthorn Project

Module Name:

    handle.c

Abstract:

    This module implements the Win32 handle management services.

Author:

    Skulltrail 10-May-2017

Revision History:

--*/

#include <main.h>

WINE_DEFAULT_DEBUG_CHANNEL(kernel32);

static BOOL oem_file_apis;

#define MOUNTMGR_DOS_DEVICE_NAME                    L"\\\\.\\MountPointManager"
#define MOUNTMGRCONTROLTYPE                         0x0000006D // 'm'
#define IOCTL_MOUNTMGR_QUERY_POINTS                 CTL_CODE(MOUNTMGRCONTROLTYPE, 2, METHOD_BUFFERED, FILE_ANY_ACCESS)	//0x6D0008
#define IOCTL_MOUNTMGR_QUERY_DOS_VOLUME_PATH        CTL_CODE(MOUNTMGRCONTROLTYPE, 12, METHOD_BUFFERED, FILE_ANY_ACCESS)	//0x6D0030

//IOCTL_MOUNTMGR_QUERY_POINTS Input
typedef struct _MOUNTMGR_MOUNT_POINT {
	ULONG   SymbolicLinkNameOffset;
	USHORT  SymbolicLinkNameLength;
	ULONG   UniqueIdOffset;
	USHORT  UniqueIdLength;
	ULONG   DeviceNameOffset;
	USHORT  DeviceNameLength;
} MOUNTMGR_MOUNT_POINT, *PMOUNTMGR_MOUNT_POINT;

//IOCTL_MOUNTMGR_QUERY_POINTS Output
typedef struct _MOUNTMGR_MOUNT_POINTS {
	ULONG                   Size;
	ULONG                   NumberOfMountPoints;
	MOUNTMGR_MOUNT_POINT    MountPoints[1];
} MOUNTMGR_MOUNT_POINTS, *PMOUNTMGR_MOUNT_POINTS;

//IOCTL_MOUNTMGR_QUERY_DOS_VOLUME_PATH Input
typedef struct _MOUNTMGR_TARGET_NAME {
	USHORT  DeviceNameLength;
	WCHAR   DeviceName[1];
} MOUNTMGR_TARGET_NAME, *PMOUNTMGR_TARGET_NAME;

//IOCTL_MOUNTMGR_QUERY_DOS_VOLUME_PATH Output
typedef struct _MOUNTMGR_VOLUME_PATHS {
	ULONG   MultiSzLength;
	WCHAR   MultiSz[1];
} MOUNTMGR_VOLUME_PATHS, *PMOUNTMGR_VOLUME_PATHS;

#define MOUNTMGR_IS_VOLUME_NAME(s) (                                          \
	((s)->Length == 96 || ((s)->Length == 98 && (s)->Buffer[48] == '\\')) && \
	(s)->Buffer[0] == '\\' &&                                                \
	((s)->Buffer[1] == '?' || (s)->Buffer[1] == '\\') &&                     \
	(s)->Buffer[2] == '?' &&                                                 \
	(s)->Buffer[3] == '\\' &&                                                \
	(s)->Buffer[4] == 'V' &&                                                 \
	(s)->Buffer[5] == 'o' &&                                                 \
	(s)->Buffer[6] == 'l' &&                                                 \
	(s)->Buffer[7] == 'u' &&                                                 \
	(s)->Buffer[8] == 'm' &&                                                 \
	(s)->Buffer[9] == 'e' &&                                                 \
	(s)->Buffer[10] == '{' &&                                                \
	(s)->Buffer[19] == '-' &&                                                \
	(s)->Buffer[24] == '-' &&                                                \
	(s)->Buffer[29] == '-' &&                                                \
	(s)->Buffer[34] == '-' &&                                                \
	(s)->Buffer[47] == '}'                                                   \
	)

static BOOL __fastcall BasepGetVolumeGUIDFromNTName(const UNICODE_STRING* NtName, wchar_t szVolumeGUID[MAX_PATH])
{
#define __szVolumeMountPointPrefix__ L"\\\\?\\GLOBALROOT"

	//一个设备名称 512 长度够多了吧？
	wchar_t szVolumeMountPoint[512];
				
	//检查缓冲区是否充足
	DWORD cbBufferNeed = sizeof(__szVolumeMountPointPrefix__) + NtName->Length;

	if (cbBufferNeed > sizeof(szVolumeMountPoint))
	{
		SetLastError(ERROR_NOT_ENOUGH_MEMORY);
		return FALSE;
	}
				
	memcpy(szVolumeMountPoint, __szVolumeMountPointPrefix__, sizeof(__szVolumeMountPointPrefix__) - sizeof(__szVolumeMountPointPrefix__[0]));
	memcpy((char*)szVolumeMountPoint + sizeof(__szVolumeMountPointPrefix__) - sizeof(__szVolumeMountPointPrefix__[0]), NtName->Buffer, NtName->Length);

	szVolumeMountPoint[cbBufferNeed / 2 - 1] = L'\0';


	return GetVolumeNameForVolumeMountPointW(szVolumeMountPoint, szVolumeGUID, MAX_PATH);

#undef __szVolumeMountPointPrefix__
}

static BOOL __fastcall BasepGetVolumeDosLetterNameFromNTName(const UNICODE_STRING* NtName, wchar_t szVolumeDosLetter[MAX_PATH])
{
	wchar_t szVolumeName[MAX_PATH];
	DWORD cchVolumePathName = 0;

	if (!BasepGetVolumeGUIDFromNTName(NtName, szVolumeName))
	{
		return FALSE;
	}				

	if (!GetVolumePathNamesForVolumeNameW(szVolumeName, szVolumeDosLetter + 4, MAX_PATH - 4, &cchVolumePathName))
	{
		return FALSE;
	}

	szVolumeDosLetter[0] = L'\\';
	szVolumeDosLetter[1] = L'\\';
	szVolumeDosLetter[2] = L'?';
	szVolumeDosLetter[3] = L'\\';

	return TRUE;
}

//Windows Vista,  Windows Server 2008
DWORD
WINAPI
GetFinalPathNameByHandleW(
	_In_ HANDLE hFile,
	_Out_writes_(cchFilePath) LPWSTR lpszFilePath,
	_In_ DWORD cchFilePath,
	_In_ DWORD dwFlags
)
{
	UNICODE_STRING VolumeNtName = {0};
	wchar_t szVolumeRoot[MAX_PATH];
	PVOID pNewBuffer;
	IO_STATUS_BLOCK IoStatusBlock;
	NTSTATUS Status;
	wchar_t* szLongPathNameBuffer = NULL;	
	//目标所需的分区名称，不包含最后的 '\\'
	UNICODE_STRING TargetVolumeName = {0};
    //目标所需的文件名，开始包含 '\\'
	UNICODE_STRING TargetFileName = {0};
	PVOID ProcessHeap = ((TEB*)NtCurrentTeb())->ProcessEnvironmentBlock->ProcessHeap;
	LSTATUS lStatus = ERROR_SUCCESS;
	DWORD   cchReturn = 0;
	OBJECT_NAME_INFORMATION* pObjectName = NULL;
	ULONG cbObjectName = 528;
	FILE_NAME_INFORMATION* pFileNameInfo = NULL;
	ULONG cbFileNameInfo = 528;
	DWORD cbszVolumeRoot;
	DWORD cbLongPathNameBufferSize;
	DWORD cchLongPathNameBufferSize;
	wchar_t* pNewLongPathName;
	DWORD result;
			
	//参数检查
	if (INVALID_HANDLE_VALUE == hFile)
	{
		SetLastError(ERROR_INVALID_HANDLE);
		return 0;
	}

	switch (dwFlags & (VOLUME_NAME_DOS | VOLUME_NAME_GUID | VOLUME_NAME_NONE | VOLUME_NAME_NT))
	{
		case VOLUME_NAME_DOS:
			break;
		case VOLUME_NAME_GUID:
			break;
		case VOLUME_NAME_NT:
			break;
		case VOLUME_NAME_NONE:
			break;
		default:
			SetLastError(ERROR_INVALID_PARAMETER);
			return 0;
		break;
	}

	szVolumeRoot[0] = L'\0';

	for (;;)
	{
		if (pObjectName)
		{
			pNewBuffer = (OBJECT_NAME_INFORMATION*)HeapReAlloc(ProcessHeap, 0, pObjectName, cbObjectName);

			if (!pNewBuffer)
			{
				lStatus = ERROR_NOT_ENOUGH_MEMORY;
				goto __Exit;
			}

			pObjectName = pNewBuffer;
     		}
			else
			{
				pObjectName = (OBJECT_NAME_INFORMATION*)HeapAlloc(ProcessHeap, 0, cbObjectName);

				if (!pObjectName)
				{
					//内存不足？
					lStatus = ERROR_NOT_ENOUGH_MEMORY;
					goto __Exit;
				}
			}

			Status = NtQueryObject(hFile, ObjectNameInformation, pObjectName, cbObjectName, &cbObjectName);

			if (STATUS_BUFFER_OVERFLOW == Status)
			{
				continue;
			}
			else if (Status < 0)
			{
				lStatus = RtlNtStatusToDosError(Status);

				goto __Exit;
			}
			else
			{
				break;
			}
		}

		for (;;)
		{
			if (pFileNameInfo)
			{
				pNewBuffer = (FILE_NAME_INFORMATION*)HeapReAlloc(ProcessHeap, 0, pFileNameInfo, cbFileNameInfo);
				if (!pNewBuffer)
				{
					lStatus = ERROR_NOT_ENOUGH_MEMORY;
					goto __Exit;
				}

				pFileNameInfo = pNewBuffer;
			}
			else
			{
				pFileNameInfo = (FILE_NAME_INFORMATION*)HeapAlloc(ProcessHeap, 0, cbFileNameInfo);

				if (!pFileNameInfo)
				{
					//内存不足？
					lStatus = ERROR_NOT_ENOUGH_MEMORY;
					goto __Exit;
				}
			}

			Status = NtQueryInformationFile(hFile, &IoStatusBlock, pFileNameInfo, cbFileNameInfo, FileNameInformation);

			if (STATUS_BUFFER_OVERFLOW == Status)
			{
				cbFileNameInfo = pFileNameInfo->FileNameLength + sizeof(FILE_NAME_INFORMATION);
				continue;
			}
			else if (Status < 0)
			{
				lStatus = RtlNtStatusToDosError(Status);

				goto __Exit;
			}
			else
			{
				break;
			}
		}

		if (pFileNameInfo->FileName[0] != '\\')
		{
			lStatus = ERROR_ACCESS_DENIED;
			goto __Exit;
		}


		if (pFileNameInfo->FileNameLength >= pObjectName->Name.Length)
		{
			lStatus = ERROR_BAD_PATHNAME;
			goto __Exit;
		}

		VolumeNtName.Buffer = pObjectName->Name.Buffer;
		VolumeNtName.Length = VolumeNtName.MaximumLength = pObjectName->Name.Length - pFileNameInfo->FileNameLength + sizeof(wchar_t);

		if (VOLUME_NAME_NT & dwFlags)
		{
			//返回NT路径
			TargetVolumeName.Buffer = VolumeNtName.Buffer;
			TargetVolumeName.Length = TargetVolumeName.MaximumLength = VolumeNtName.Length - sizeof(wchar_t);
		}
		else if (VOLUME_NAME_NONE & dwFlags)
		{
			//仅返回文件名
		}
		else
		{
			if (VOLUME_NAME_GUID & dwFlags)
			{
				//返回分区GUID名称
				if (!BasepGetVolumeGUIDFromNTName(&VolumeNtName, szVolumeRoot))
				{
					lStatus = GetLastError();
					goto __Exit;
				}
			}
			else
			{
				//返回Dos路径
				if (!BasepGetVolumeDosLetterNameFromNTName(&VolumeNtName, szVolumeRoot))
				{
					lStatus = GetLastError();
					goto __Exit;
				}
			}

			TargetVolumeName.Buffer = szVolumeRoot;
			TargetVolumeName.Length = TargetVolumeName.MaximumLength = (wcslen(szVolumeRoot) - 1) * sizeof(szVolumeRoot[0]);
		}

		//将路径进行规范化
		if ((FILE_NAME_OPENED & dwFlags) == 0)
		{
			//由于 Windows XP不支持 FileNormalizedNameInformation，所以我们直接调用 GetLongPathNameW 获取完整路径。

			cbszVolumeRoot = TargetVolumeName.Length;

			if (szVolumeRoot[0] == L'\0')
			{
				//转换分区信息

				if (!BasepGetVolumeDosLetterNameFromNTName(&VolumeNtName, szVolumeRoot))
				{
					lStatus = GetLastError();

					if(lStatus == ERROR_NOT_ENOUGH_MEMORY)
						goto __Exit;

					if (!BasepGetVolumeGUIDFromNTName(&VolumeNtName, szVolumeRoot))
					{
						lStatus = GetLastError();
						goto __Exit;
					}
				}

				cbszVolumeRoot = (wcslen(szVolumeRoot) - 1) * sizeof(szVolumeRoot[0]);
			}


			cbLongPathNameBufferSize = cbszVolumeRoot + pFileNameInfo->FileNameLength + 1024;

			szLongPathNameBuffer = (wchar_t*)HeapAlloc(ProcessHeap, 0, cbLongPathNameBufferSize);
			if (!szLongPathNameBuffer)
			{
				lStatus = ERROR_NOT_ENOUGH_MEMORY;
				goto __Exit;
			}

			cchLongPathNameBufferSize = cbLongPathNameBufferSize / sizeof(szLongPathNameBuffer[0]);

			memcpy(szLongPathNameBuffer, szVolumeRoot, cbszVolumeRoot);
			memcpy((char*)szLongPathNameBuffer + cbszVolumeRoot, pFileNameInfo->FileName, pFileNameInfo->FileNameLength);
			szLongPathNameBuffer[(cbszVolumeRoot + pFileNameInfo->FileNameLength) / sizeof(wchar_t)] = L'\0';

			for (;;)
			{
				result = GetLongPathNameW(szLongPathNameBuffer, szLongPathNameBuffer, cchLongPathNameBufferSize);

				if (result == 0)
				{
						//失败
					lStatus = GetLastError();
					goto __Exit;
				}
				else if (result >= cchLongPathNameBufferSize)
				{
					cchLongPathNameBufferSize = result + 1;

					pNewLongPathName = (wchar_t*)HeapReAlloc(ProcessHeap, 0, szLongPathNameBuffer, cchLongPathNameBufferSize * sizeof(wchar_t));
					if (!pNewLongPathName)
					{
						lStatus = ERROR_NOT_ENOUGH_MEMORY;
						goto __Exit;
					}

					szLongPathNameBuffer = pNewLongPathName;
			
				}
				else
				{
					//转换成功
					TargetFileName.Buffer = (wchar_t*)((char*)szLongPathNameBuffer + cbszVolumeRoot);
					TargetFileName.Length = TargetFileName.MaximumLength = result * sizeof(wchar_t) - cbszVolumeRoot;
					break;
				}
			}
		}
		else
		{
			//直接返回原始路径
			TargetFileName.Buffer = pFileNameInfo->FileName;
			TargetFileName.Length = TargetFileName.MaximumLength = pFileNameInfo->FileNameLength;
		}


		//返回结果，根目录 + 文件名 的长度
		cchReturn = (TargetVolumeName.Length + TargetFileName.Length) / sizeof(wchar_t);

		if (cchFilePath <= cchReturn)
		{
			//长度不足……

			cchReturn += 1;
		}
		else
		{
			//复制根目录
			memcpy(lpszFilePath, TargetVolumeName.Buffer, TargetVolumeName.Length);
			//复制文件名
			memcpy((char*)lpszFilePath + TargetVolumeName.Length, TargetFileName.Buffer, TargetFileName.Length);
			//保证字符串 '\0' 截断
			lpszFilePath[cchReturn] = L'\0';
		}

__Exit:
	if (pFileNameInfo)
		HeapFree(ProcessHeap, 0, pFileNameInfo);
	if (pObjectName)
		HeapFree(ProcessHeap, 0, pObjectName);
	if (szLongPathNameBuffer)
		HeapFree(ProcessHeap, 0, szLongPathNameBuffer);

	if (lStatus != ERROR_SUCCESS)
	{
		SetLastError(lStatus);
		return 0;
	}
	else
	{
		return cchReturn;
	}
}

//Windows Vista,  Windows Server 2008
DWORD
WINAPI
GetFinalPathNameByHandleA(
	_In_ HANDLE hFile,
	_Out_writes_(cchFilePath) LPSTR lpszFilePath,
	_In_ DWORD cchFilePath,
	_In_ DWORD dwFlags
)
{
	PVOID ProcessHeap = ((TEB*)NtCurrentTeb())->ProcessEnvironmentBlock->ProcessHeap;
	wchar_t* szFilePathUnicode = NULL;
	wchar_t* pNewBuffer;
	DWORD cchReturn;
	DWORD cchszFilePathUnicode;
	DWORD lStatus;
	int cchReturnANSI;
			
	for (cchszFilePathUnicode = 1040;;)
	{
		if (szFilePathUnicode)
		{
			pNewBuffer = (wchar_t*)HeapReAlloc(ProcessHeap, 0, szFilePathUnicode, cchszFilePathUnicode * sizeof(wchar_t));
			if (!pNewBuffer)
			{
				HeapFree(ProcessHeap, 0, szFilePathUnicode);
				SetLastError(ERROR_NOT_ENOUGH_MEMORY);
				return 0;
			}

			szFilePathUnicode = pNewBuffer;
		}
		else
		{
			szFilePathUnicode = (wchar_t*)HeapAlloc(ProcessHeap, 0, cchszFilePathUnicode * sizeof(wchar_t));
			if (!szFilePathUnicode)
			{
				SetLastError(ERROR_NOT_ENOUGH_MEMORY);
				return 0;
			}
		}

		cchReturn = GetFinalPathNameByHandleW(hFile, szFilePathUnicode, cchszFilePathUnicode, dwFlags);

		if (cchReturn == 0)
		{
			__Error:

			lStatus = GetLastError();
			HeapFree(ProcessHeap, 0, szFilePathUnicode);
			SetLastError(lStatus);

			return 0;
		}
		else if (cchReturn > cchszFilePathUnicode)
		{
			//缓冲区不足
			cchszFilePathUnicode = cchReturn;
			continue;
		}
		else
		{
			//操作成功！
			const UINT CodePage = AreFileApisANSI() ? CP_ACP : CP_OEMCP;

			cchReturnANSI = WideCharToMultiByte(CodePage, WC_NO_BEST_FIT_CHARS, szFilePathUnicode, cchReturn, NULL, 0, NULL, NULL);

			if (0 == cchReturnANSI)
			{
				goto __Error;
			}
			else if (cchReturnANSI >= cchFilePath)
			{
				//长度不足
				++cchReturnANSI;
			}
			else
			{
				WideCharToMultiByte(CodePage, WC_NO_BEST_FIT_CHARS, szFilePathUnicode, cchReturn, lpszFilePath, cchFilePath, NULL, NULL);
				lpszFilePath[cchReturnANSI] = '\0';
			}

			HeapFree(ProcessHeap, 0, szFilePathUnicode);
			return cchReturnANSI;
		}
	}
}