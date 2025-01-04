/*++

Copyright (c) 2017  Shorthorn Project

Module Name:

    fileopcr.c

Abstract:

    This module implements File open and Create APIs for Win32

Author:

    Skulltrail 20-March-2017

Revision History:

--*/

#include "main.h"

HANDLE
WINAPI
ReOpenFile(
    HANDLE  hOriginalFile,
    DWORD   dwDesiredAccess,
    DWORD   dwShareMode,
    DWORD   dwFlags
    )
{
    ULONG CreateFlags = 0;
    ULONG CreateDisposition;
    NTSTATUS Status;
    OBJECT_ATTRIBUTES Obja;
    HANDLE Handle;
    IO_STATUS_BLOCK IoStatusBlock;
    DWORD SQOSFlags;
    SECURITY_QUALITY_OF_SERVICE SecurityQualityOfService;
    UNICODE_STRING  FileName;

    if (((ULONG)hOriginalFile & 0x10000003)) {
        BaseSetLastNTError(STATUS_INVALID_HANDLE);
        return INVALID_HANDLE_VALUE;
    }

    /*
    / The attributes are useless as this reopen of an existing file.
    */

    if (dwFlags &  FILE_ATTRIBUTE_VALID_FLAGS) {
        BaseSetLastNTError(STATUS_INVALID_PARAMETER);
        return INVALID_HANDLE_VALUE;
    }

    /*
    / Initialize all the create flags from the Attribute flags.
    */

    CreateFlags |= (dwFlags & FILE_FLAG_NO_BUFFERING ? FILE_NO_INTERMEDIATE_BUFFERING : 0 );
    CreateFlags |= (dwFlags & FILE_FLAG_WRITE_THROUGH ? FILE_WRITE_THROUGH : 0 );
    CreateFlags |= (dwFlags & FILE_FLAG_OVERLAPPED ? 0 : FILE_SYNCHRONOUS_IO_NONALERT );
    CreateFlags |= (dwFlags & FILE_FLAG_SEQUENTIAL_SCAN ? FILE_SEQUENTIAL_ONLY : 0 );
    CreateFlags |= (dwFlags & FILE_FLAG_RANDOM_ACCESS ? FILE_RANDOM_ACCESS : 0 );
    CreateFlags |= (dwFlags & FILE_FLAG_BACKUP_SEMANTICS ? FILE_OPEN_FOR_BACKUP_INTENT : 0 );
    CreateFlags |= (dwFlags & FILE_FLAG_OPEN_REPARSE_POINT ? FILE_OPEN_REPARSE_POINT : 0 );
    CreateFlags |= (dwFlags & FILE_FLAG_OPEN_NO_RECALL ? FILE_OPEN_NO_RECALL : 0 );

    if ( dwFlags & FILE_FLAG_DELETE_ON_CLOSE ) {
        CreateFlags |= FILE_DELETE_ON_CLOSE;
        dwDesiredAccess |= DELETE;
        }

    CreateFlags |= FILE_NON_DIRECTORY_FILE;
    CreateDisposition = FILE_OPEN;

    RtlInitUnicodeString( &FileName, L"");
    
    /*
    / Pass a NULL name relative to the original handle.
    */

    InitializeObjectAttributes(
        &Obja,
        &FileName,  
        dwFlags & FILE_FLAG_POSIX_SEMANTICS ? 0 : OBJ_CASE_INSENSITIVE,
        hOriginalFile, 
        NULL
        );

    SQOSFlags = dwFlags & SECURITY_VALID_SQOS_FLAGS;

    if ( SQOSFlags & SECURITY_SQOS_PRESENT ) {

        SQOSFlags &= ~SECURITY_SQOS_PRESENT;

        if (SQOSFlags & SECURITY_CONTEXT_TRACKING) {

            SecurityQualityOfService.ContextTrackingMode = (SECURITY_CONTEXT_TRACKING_MODE) TRUE;
            SQOSFlags &= ~SECURITY_CONTEXT_TRACKING;

        } else {

            SecurityQualityOfService.ContextTrackingMode = (SECURITY_CONTEXT_TRACKING_MODE) FALSE;
        }

        if (SQOSFlags & SECURITY_EFFECTIVE_ONLY) {

            SecurityQualityOfService.EffectiveOnly = TRUE;
            SQOSFlags &= ~SECURITY_EFFECTIVE_ONLY;

        } else {

            SecurityQualityOfService.EffectiveOnly = FALSE;
        }

        SecurityQualityOfService.ImpersonationLevel = SQOSFlags >> 16;


    } else {

        SecurityQualityOfService.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
        SecurityQualityOfService.ImpersonationLevel = SecurityImpersonation;
        SecurityQualityOfService.EffectiveOnly = TRUE;
    }

    SecurityQualityOfService.Length = sizeof( SECURITY_QUALITY_OF_SERVICE );
    Obja.SecurityQualityOfService = &SecurityQualityOfService;

    Status = NtCreateFile(
                &Handle,
                (ACCESS_MASK)dwDesiredAccess | SYNCHRONIZE | FILE_READ_ATTRIBUTES,
                &Obja,
                &IoStatusBlock,
                NULL,
                0,
                dwShareMode,
                CreateDisposition,
                CreateFlags,
                NULL,
                0
                );

    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        return INVALID_HANDLE_VALUE;
    }

    SetLastError(0);

    return Handle;
}

/*
 * @implemented - need test
 */
BOOL 
WINAPI 
CopyFileTransactedW(
	LPCWSTR lpExistingFileName, 
	LPCWSTR lpNewFileName, 
	LPPROGRESS_ROUTINE lpProgressRoutine, 
	LPVOID lpData, 
	LPBOOL pbCancel, 
	DWORD dwCopyFlags, 
	HANDLE hTransaction
)
{
    return CopyFileExW(lpExistingFileName, lpNewFileName, lpProgressRoutine, lpData, pbCancel, dwCopyFlags);
}

/*
 * @implemented - need test
 */
BOOL 
WINAPI 
CopyFileTransactedA(
	LPCSTR lpExistingFileName, 
	LPCSTR lpNewFileName, 
	LPPROGRESS_ROUTINE lpProgressRoutine, 
	LPVOID lpData, 
	LPBOOL pbCancel, 
	DWORD dwCopyFlags, 
	HANDLE hTransaction
)
{
    return CopyFileExA(lpExistingFileName, lpNewFileName, lpProgressRoutine, lpData, pbCancel, dwCopyFlags);
}

HANDLE
WINAPI
CreateFileTransactedW(
    LPCWSTR lpFileName,
    DWORD dwDesiredAccess,
    DWORD dwShareMode,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    DWORD dwCreationDisposition,
    DWORD dwFlagsAndAttributes,
    HANDLE hTemplateFile
)
{
	return CreateFileW(lpFileName, 
					   dwDesiredAccess, 
					   dwShareMode, 
					   lpSecurityAttributes, 
					   dwCreationDisposition, 
					   dwFlagsAndAttributes,
					   hTemplateFile);
}

HANDLE
WINAPI
CreateFileTransactedA(
    LPSTR lpFileName,
    DWORD dwDesiredAccess,
    DWORD dwShareMode,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    DWORD dwCreationDisposition,
    DWORD dwFlagsAndAttributes,
    HANDLE hTemplateFile
)
{
	return CreateFileA(lpFileName, 
					   dwDesiredAccess, 
					   dwShareMode, 
					   lpSecurityAttributes, 
					   dwCreationDisposition, 
					   dwFlagsAndAttributes,
					   hTemplateFile);
}

HANDLE WINAPI CreateFile2(
	IN	PCWSTR								FileName,
	IN	ULONG								DesiredAccess,
	IN	ULONG								ShareMode,
	IN	ULONG								CreationDisposition,
	IN	PCREATEFILE2_EXTENDED_PARAMETERS	ExtendedParameters OPTIONAL)
{
	if (ExtendedParameters) {
		ULONG FlagsAndAttributes;

		if (ExtendedParameters->dwSize < sizeof(CREATEFILE2_EXTENDED_PARAMETERS)) {
			BaseSetLastNTError(STATUS_INVALID_PARAMETER);
			return INVALID_HANDLE_VALUE;
		}

		FlagsAndAttributes = ExtendedParameters->dwFileFlags |
							 ExtendedParameters->dwSecurityQosFlags |
							 ExtendedParameters->dwFileAttributes;

		return CreateFileW(
			FileName,
			DesiredAccess,
			ShareMode,
			ExtendedParameters->lpSecurityAttributes,
			CreationDisposition,
			FlagsAndAttributes,
			ExtendedParameters->hTemplateFile);
	} else {
		return CreateFileW(
			FileName,
			DesiredAccess,
			ShareMode,
			NULL,
			CreationDisposition,
			0,
			NULL);
	}
}

#define COPY_FILE_COPY_SYMLINK                  0x00000800
#define COPY_FILE_NO_BUFFERING                  0x00001000
#define COPY_FILE_DIRECTORY						0x00000080		// Win10
#define COPY_FILE_REQUEST_SECURITY_PRIVILEGES	0x00002000		// Win8
#define COPY_FILE_RESUME_FROM_PAUSE				0x00004000		// Win8
#define COPY_FILE_SKIP_ALTERNATE_STREAMS		0x00008000		// Win10
#define COPY_FILE_NO_OFFLOAD					0x00040000		// Win8
#define COPY_FILE_OPEN_AND_COPY_REPARSE_POINT	0x00200000		// Win10
#define COPY_FILE_IGNORE_EDP_BLOCK				0x00400000		// Win10
#define COPY_FILE_IGNORE_SOURCE_ENCRYPTION		0x00800000		// Win10
#define COPY_FILE_DONT_REQUEST_DEST_WRITE_DAC	0x02000000		// Win10
#define COPY_FILE_DISABLE_PRE_ALLOCATION		0x04000000		// Win10
#define COPY_FILE_ENABLE_LOW_FREE_SPACE_MODE	0x08000000		// Win10
#define COPY_FILE_REQUEST_COMPRESSED_TRAFFIC	0x10000000		// Win10
#define COPY_FILE_ENABLE_SPARSE_COPY			0x20000000		// Win11

#define COPY_FILE_WIN7_VALID_FLAGS				(COPY_FILE_FAIL_IF_EXISTS | COPY_FILE_RESTARTABLE | \
												 COPY_FILE_OPEN_SOURCE_FOR_WRITE | COPY_FILE_ALLOW_DECRYPTED_DESTINATION | \
												 COPY_FILE_COPY_SYMLINK | COPY_FILE_NO_BUFFERING)

#define COPY_FILE_WIN8_VALID_FLAGS				(COPY_FILE_WIN7_VALID_FLAGS | COPY_FILE_REQUEST_SECURITY_PRIVILEGES | \
												 COPY_FILE_RESUME_FROM_PAUSE | COPY_FILE_NO_OFFLOAD)

#define COPY_FILE_WIN10_VALID_FLAGS				(COPY_FILE_WIN8_VALID_FLAGS | COPY_FILE_DIRECTORY | COPY_FILE_SKIP_ALTERNATE_STREAMS | \
												 COPY_FILE_OPEN_AND_COPY_REPARSE_POINT | COPY_FILE_IGNORE_EDP_BLOCK | \
												 COPY_FILE_IGNORE_SOURCE_ENCRYPTION | COPY_FILE_DONT_REQUEST_DEST_WRITE_DAC | \
												 COPY_FILE_DISABLE_PRE_ALLOCATION | COPY_FILE_ENABLE_LOW_FREE_SPACE_MODE | \
												 COPY_FILE_REQUEST_COMPRESSED_TRAFFIC)

#define COPY_FILE_WIN11_VALID_FLAGS				(COPY_FILE_WIN10_VALID_FLAGS | COPY_FILE_ENABLE_SPARSE_COPY)

#define COPY_FILE_ALL_VALID_FLAGS				(COPY_FILE_WIN11_VALID_FLAGS)


#ifndef _DEBUG
#  define ASSUME __assume
#  define NOT_REACHED ASSUME(FALSE)
#else
#  define ASSUME ASSERT
#  define NOT_REACHED ASSUME(("Execution should not have reached this point", FALSE))
#endif

static DWORD CALLBACK KxBasepCopyFile2ProgressRoutine(
	IN	LARGE_INTEGER	TotalFileSize,
	IN	LARGE_INTEGER	TotalBytesTransferred,
	IN	LARGE_INTEGER	StreamSize,
	IN	LARGE_INTEGER	StreamBytesTransferred,
	IN	DWORD			StreamNumber,
	IN	DWORD			CallbackReason,
	IN	HANDLE			SourceFile,
	IN	HANDLE			DestinationFile,
	IN	PVOID			Context OPTIONAL)
{
	COPYFILE2_MESSAGE Message;
	PCOPYFILE2_EXTENDED_PARAMETERS Copyfile2Parameters;

	ASSERT (Context != NULL);

	RtlZeroMemory(&Message, sizeof(Message));

	switch (CallbackReason) {
	case CALLBACK_CHUNK_FINISHED:
		Message.Type = COPYFILE2_CALLBACK_CHUNK_FINISHED;
		Message.Info.ChunkFinished.dwStreamNumber = StreamNumber;
		Message.Info.ChunkFinished.uliTotalFileSize.QuadPart = TotalFileSize.QuadPart;
		Message.Info.ChunkFinished.uliTotalBytesTransferred.QuadPart = TotalBytesTransferred.QuadPart;
		Message.Info.ChunkFinished.uliStreamSize.QuadPart = StreamSize.QuadPart;
		Message.Info.ChunkFinished.uliStreamBytesTransferred.QuadPart = StreamBytesTransferred.QuadPart;
		break;
	case CALLBACK_STREAM_SWITCH:
		Message.Type = COPYFILE2_CALLBACK_STREAM_STARTED;
		Message.Info.StreamStarted.dwStreamNumber = StreamNumber;
		Message.Info.StreamStarted.hDestinationFile = DestinationFile;
		Message.Info.StreamStarted.hSourceFile = SourceFile;
		Message.Info.StreamStarted.uliStreamSize.QuadPart = StreamSize.QuadPart;
		Message.Info.StreamStarted.uliTotalFileSize.QuadPart = TotalFileSize.QuadPart;
		break;
	default:
		ASSUME (FALSE);
	}

	Copyfile2Parameters = (PCOPYFILE2_EXTENDED_PARAMETERS) Context;

	return Copyfile2Parameters->pProgressRoutine(
		&Message,
		Copyfile2Parameters->pvCallbackContext);
}

HRESULT 
WINAPI 
CopyFile2(
	IN	PCWSTR							ExistingFileName,
	IN	PCWSTR							NewFileName,
	IN	PCOPYFILE2_EXTENDED_PARAMETERS	ExtendedParameters OPTIONAL)
{
	BOOL Success;
	ULONG EffectiveCopyFlags;

	if (ExtendedParameters == NULL) {
		Success = CopyFileW(
			ExistingFileName,
			NewFileName,
			FALSE);

		if (Success) {
			return S_OK;
		} else {
			return HRESULT_FROM_WIN32(GetLastError());
		}
	}

	if (ExtendedParameters->dwSize != sizeof(COPYFILE2_EXTENDED_PARAMETERS)) {
		//
		// Windows 11 defines a COPYFILE2_EXTENDED_PARAMETERS_V2 struture.
		// When apps start using it, we will support it too.
		//

		DbgPrint(
			"Unrecognized dwSize member of ExtendedParameters: %lu\n",
			ExtendedParameters->dwSize);

		return E_INVALIDARG;
	}

	if (ExtendedParameters->dwCopyFlags & ~COPY_FILE_ALL_VALID_FLAGS) {
		return E_INVALIDARG;
	}

	EffectiveCopyFlags = ExtendedParameters->dwCopyFlags & COPY_FILE_WIN7_VALID_FLAGS;

	Success = CopyFileExW(
		ExistingFileName,
		NewFileName,
		ExtendedParameters->pProgressRoutine ? KxBasepCopyFile2ProgressRoutine : NULL,
		ExtendedParameters,
		ExtendedParameters->pfCancel,
		EffectiveCopyFlags  & ~(COPY_FILE_NO_BUFFERING + COPY_FILE_COPY_SYMLINK));

	if (Success) {
		return S_OK;
	} else {
		return HRESULT_FROM_WIN32(GetLastError());
	}
}