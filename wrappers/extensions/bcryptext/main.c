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

#include <stdarg.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
//#include "ntsecapi.h"
#include "wincrypt.h"
#include "wine/winternl.h"
#include "bcrypt.h"

#include "bcrypt_internal.h"

#include "wine/debug.h"
#include "wine/heap.h"

#define SIZE_MAX 0xFFFFFFFF
#define kMaxHashLength 512/8

WINE_DEFAULT_DEBUG_CHANNEL(bcrypt);

NTSTATUS 
WINAPI 
BCryptCreateHashNative( 
    BCRYPT_ALG_HANDLE algorithm, 
    BCRYPT_HASH_HANDLE *handle, 
    UCHAR *object, 
    ULONG objectlen,
    UCHAR *secret, 
    ULONG secretlen,
    ULONG flags 
);

static int StringCompareIgnoreCaseByAscii(const char* string1, const char* string2, size_t count)
{
    wchar_t f, l;
    int result = 0;

    if (count)
    {
        /* validation section */
        do {
            f = towlower(*string1);
            l = towlower(*string2);
            string1++;
            string2++;
        } while ((--count) && f && (f == l));

        result = (int)(f - l);
    }

    return result;
}

VOID
WINAPI
BCryptFree(
    _In_ PVOID   pvBuffer)
{
	RtlFreeHeap(NtCurrentTeb()->Peb->ProcessHeap, 0, pvBuffer);
}

NTSTATUS WINAPI BCryptHash(BCRYPT_ALG_HANDLE hAlgorithm, PUCHAR pbSecret, ULONG cbSecret, PUCHAR pbInput, 
                    ULONG cbInput, PUCHAR pbOutput, ULONG cbOutput)
{
   BCRYPT_HASH_HANDLE hHash;
   NTSTATUS Status;
   PUCHAR pbHashObject;
   ULONG cbHashObject;
   ULONG ResultLength;
   
   BCryptGetProperty(hAlgorithm, L"ObjectLength", NULL, 0, &ResultLength, 0);
   
   cbHashObject = ResultLength;
   
   pbHashObject = (PUCHAR)RtlAllocateHeap(NtCurrentTeb()->Peb->ProcessHeap, 0, cbHashObject);
   
   if(!pbHashObject)
	   return STATUS_NO_MEMORY;
   
   Status = BCryptGetProperty(hAlgorithm, L"ObjectLength", pbHashObject, cbHashObject, &ResultLength, 0);
   
   if(Status < STATUS_SUCCESS)
	    goto ReturnAndDeallocate;

   Status = BCryptCreateHash(hAlgorithm, &hHash, pbHashObject, cbHashObject, pbSecret, cbSecret, 0);

   if(Status < STATUS_SUCCESS)
	   goto ReturnAndDeallocate;

   Status = BCryptHashData(hHash, pbInput, cbInput, 0);   
   
   if(Status < STATUS_SUCCESS)
	   goto DestroyAndReturn;
   
   Status = BCryptFinishHash(hHash, pbOutput, cbOutput, 0);   
   
   if(Status < STATUS_SUCCESS)
	   goto DestroyAndReturn;   
   
  DestroyAndReturn:
     BCryptDestroyHash(hHash);
  ReturnAndDeallocate:
     RtlFreeHeap(NtCurrentTeb()->Peb->ProcessHeap, 0, pbHashObject);
     return Status;
}

NTSTATUS 
WINAPI 
BCryptCreateHashInternal( 
    BCRYPT_ALG_HANDLE hAlgorithm, 
    BCRYPT_HASH_HANDLE *phHash, 
    UCHAR *pbHashObject, 
    ULONG cbHashObject,
    UCHAR *pbSecret, 
    ULONG cbSecret,
    ULONG dwFlags 
)
{
	ULONG Unused;
	NTSTATUS Status;
	if (pbHashObject == NULL && cbHashObject == 0) {
		// FIX: BCryptCreateHash of Windows Vista does not support this parameter. .NET Core 6 relies on it.
		Status = BCryptGetProperty(hAlgorithm, L"ObjectLength", (PUCHAR)&cbHashObject, 4, &Unused, 0);
		if (Status != 0) 
			return Status;
		// Try allocating required memory.
		// FIXME: Memory leak occours when this memory management feature is used (matches Vista extended kernel). On a future One-Core-API version bcrypt.dll will be replaced with Windows 8 by making modern ksecdd.sys run side-by-side with 5456.5's ksecdd.sys.
		pbHashObject = (PUCHAR)RtlAllocateHeap(NtCurrentTeb()->Peb->ProcessHeap, 0, cbHashObject);
	}
	return BCryptCreateHashNative(hAlgorithm, phHash, pbHashObject, cbHashObject, pbSecret, cbSecret, dwFlags);
}

NTSTATUS
WINAPI
BCryptDeriveKeyPBKDF2(
   _In_                                 BCRYPT_ALG_HANDLE   hPrf,
   _In_reads_bytes_opt_( cbPassword )   PUCHAR              pbPassword,
   _In_                                 ULONG               cbPassword,
   _In_reads_bytes_opt_( cbSalt )       PUCHAR              pbSalt,
   _In_                                 ULONG               cbSalt,
   _In_                                 ULONGLONG           cIterations,
   _Out_writes_bytes_( cbDerivedKey )   PUCHAR              pbDerivedKey,
   _In_                                 ULONG               cbDerivedKey,
   _In_                                 ULONG               dwFlags
)
{
	UCHAR* aSalt, *obuf; // Update this, use variable digest length obtained from BCryptGetProperty call
	UCHAR* d1, *d2;
	ULONG i, j;
	ULONG count, ResultLength;
	size_t r;
	DWORD DigestLength;
	//BCRYPT_HASH_HANDLE hHash;
	NTSTATUS Status = STATUS_SUCCESS;

	if (cIterations < 1 || cbDerivedKey == 0 || !hPrf)
		return STATUS_INVALID_PARAMETER;
	if (cbSalt == 0 || cbSalt > SIZE_MAX - 4)
		return STATUS_INVALID_PARAMETER;
	
	Status = BCryptGetProperty(hPrf, L"HashDigestLength", (PUCHAR)&DigestLength, sizeof(DWORD), &ResultLength, 0);
	if(Status < STATUS_SUCCESS)
		return Status;
	
	
	if ((aSalt = (PUCHAR)RtlAllocateHeap(NtCurrentTeb()->Peb->ProcessHeap, 0, cbSalt + 4)) == NULL)
		return STATUS_NO_MEMORY;
	
	if ((obuf = (PUCHAR)RtlAllocateHeap(NtCurrentTeb()->Peb->ProcessHeap, 0, DigestLength)) == NULL)
	{
		BCryptFree(aSalt); // undocumented function exported from BCrypt - basically a wrapper for RtlFreeHeap with flags = 0 and PEB heap
		return STATUS_NO_MEMORY;
	}
	
	if ((d1 = (PUCHAR)RtlAllocateHeap(NtCurrentTeb()->Peb->ProcessHeap, 0, DigestLength)) == NULL)
	{
		BCryptFree(aSalt);
		BCryptFree(obuf);
		return STATUS_NO_MEMORY;
	}

	if ((d2 = (PUCHAR)RtlAllocateHeap(NtCurrentTeb()->Peb->ProcessHeap, 0, DigestLength)) == NULL)
	{
		BCryptFree(aSalt);
		BCryptFree(obuf);
		BCryptFree(d1);
		return STATUS_NO_MEMORY;
	}	

	memcpy(aSalt, pbSalt, cbSalt);

	for (count = 1; cbDerivedKey > 0; count++) {
		aSalt[cbSalt + 0] = (count >> 24) & 0xff;
		aSalt[cbSalt + 1] = (count >> 16) & 0xff;
		aSalt[cbSalt + 2] = (count >> 8) & 0xff;
		aSalt[cbSalt + 3] = count & 0xff;
		Status = BCryptHash(hPrf, (PUCHAR)pbPassword, cbPassword, aSalt, cbSalt + 4, d1, DigestLength);
		if (Status < STATUS_SUCCESS)
			goto FreeMemory;
		memcpy(obuf, d1, DigestLength);

		for (i = 1; i < cIterations; i++) {
			Status = BCryptHash(hPrf, (PUCHAR)pbPassword, cbPassword, d1, DigestLength, d2, DigestLength);
		if (Status < STATUS_SUCCESS)
			goto FreeMemory;			
			memcpy(d1, d2, DigestLength);
			for (j = 0; j < DigestLength; j++)
				obuf[j] ^= d1[j];
		}

		r = min(cbDerivedKey, DigestLength);
		memcpy(pbDerivedKey, obuf, r);
		pbDerivedKey += r;
		cbDerivedKey -= r;
	};
	
FreeMemory:	
	memset(aSalt, 0, cbSalt + 4);
	BCryptFree(aSalt);
	memset(d1, 0, DigestLength);
	BCryptFree(d1);
	memset(d2, 0, DigestLength);
    BCryptFree(d2);
	memset(obuf, 0, DigestLength);
	BCryptFree(obuf);
	
	return Status;
}

NTSTATUS
WINAPI
BCryptDeriveKeyCapi(
    _In_                            BCRYPT_HASH_HANDLE  hHash,
    _In_opt_                        BCRYPT_ALG_HANDLE   hTargetAlg,
    _Out_writes_bytes_( cbDerivedKey )    PUCHAR              pbDerivedKey,
    _In_                            ULONG               cbDerivedKey,
    _In_                            ULONG               dwFlags
)
{
    ULONG _cbResult = 0;
    DWORD _uHashLength = 0;
	UCHAR _Hash[kMaxHashLength * 2];
    int _Status;
    BOOLEAN checkHashLength;
	wchar_t szAlgorithmNameBuffer[4];
    UCHAR _FirstHash[kMaxHashLength];
    UCHAR _SecondHash[kMaxHashLength];
	BCRYPT_ALG_HANDLE hProvider;
	DWORD i;
	DWORD _cbHashObjectLength = 0;
	BCRYPT_HASH_HANDLE hHash2 = NULL;
	PUCHAR _pHashObjectBuffer;

    if (dwFlags != 0 || (pbDerivedKey == NULL && cbDerivedKey))
    {
        return STATUS_INVALID_PARAMETER;
    }


    _Status = BCryptGetProperty(hHash, BCRYPT_HASH_LENGTH, (PUCHAR)&_uHashLength, sizeof(_uHashLength), &_cbResult, 0);
    if (_Status < 0)
            return _Status;

    if (kMaxHashLength < _uHashLength || _uHashLength * 2 < cbDerivedKey)
    {
        return STATUS_INVALID_PARAMETER;
    }

        
    _Status = BCryptFinishHash(hHash, _Hash, _uHashLength, 0);
    if (_Status < 0)
        return _Status;

    checkHashLength = _uHashLength < cbDerivedKey;
        
    if (hTargetAlg && cbDerivedKey == 16 && _uHashLength < 32
        && BCryptGetProperty(hTargetAlg, BCRYPT_ALGORITHM_NAME, (PUCHAR)szAlgorithmNameBuffer, sizeof(szAlgorithmNameBuffer), &_cbResult, 0) >= 0
        && StringCompareIgnoreCaseByAscii((const char* )szAlgorithmNameBuffer, (const char* )L"AES", -1) == 0)
    {
        checkHashLength = TRUE;
    }

    if (!checkHashLength)
    {
        memcpy(pbDerivedKey, _Hash, cbDerivedKey);
        return STATUS_SUCCESS;
    }
		
    memset(_FirstHash, 0x36, sizeof(_FirstHash));
    memset(_SecondHash, 0x5C, sizeof(_SecondHash));

    for (i = 0; i != _uHashLength; ++i)
    {
        _FirstHash[i] ^= _Hash[i];
        _SecondHash[i] ^= _Hash[i];
    }
        
    _Status = BCryptGetProperty(hHash, BCRYPT_PROVIDER_HANDLE, (PUCHAR)&hProvider, sizeof(hProvider), &_cbResult, 0);
    if (_Status < 0)
        return _Status;
        
    _Status = BCryptGetProperty(hProvider, BCRYPT_OBJECT_LENGTH, (PUCHAR)&_cbHashObjectLength, sizeof(_cbHashObjectLength), &_cbResult, 0);
    if (_Status < 0)
        return _Status;

    _pHashObjectBuffer = (PUCHAR)RtlAllocateHeap(NtCurrentTeb()->Peb->ProcessHeap, 0, _cbHashObjectLength);
    if (!_pHashObjectBuffer)
        return STATUS_NO_MEMORY;
        
    do
    {
        _Status = BCryptCreateHash(hProvider, &hHash2, _pHashObjectBuffer, _cbHashObjectLength, NULL, 0, 0);
        if (_Status < 0)
            break;

        _Status = BCryptHashData(hHash2, _FirstHash, sizeof(_FirstHash), 0);
        if (_Status < 0)
            break;

        _Status = BCryptFinishHash(hHash2, _Hash, _uHashLength, 0);
        if (_Status < 0)
            break;
        BCryptDestroyHash(hHash2);
        hHash2 = NULL;

        _Status = BCryptCreateHash(hProvider, &hHash2, _pHashObjectBuffer, _cbHashObjectLength, NULL, 0, 0);
        if (_Status < 0)
            break;

        _Status = BCryptHashData(hHash2, _SecondHash, sizeof(_SecondHash), 0);
        if (_Status < 0)
            break;

        _Status = BCryptFinishHash(hHash2, _Hash + _uHashLength, _uHashLength, 0);
        if (_Status < 0)
            break;

        memcpy(pbDerivedKey, _Hash, cbDerivedKey);
        _Status = STATUS_SUCCESS;

    } while (FALSE);

    if (hHash2)
        BCryptDestroyHash(hHash2);

    if (_pHashObjectBuffer)
        RtlFreeHeap(NtCurrentTeb()->Peb->ProcessHeap, 0, _pHashObjectBuffer);
	
    return _Status;
}