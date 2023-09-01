/*
 * Performance Data Helper (pdh.dll)
 *
 * Copyright 2007 Andrey Turkin
 * Copyright 2007 Hans Leidekker
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

#define WIN32_NO_STATUS

#include <stdarg.h>
#include <math.h>

#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#include <windef.h>
#include <winbase.h>

//#include "winperf.h"

#include <wine/debug.h>
#include <wine/list.h>
#include <wine/unicode.h>
#include <wincrypt.h>

WINE_DEFAULT_DEBUG_CHANNEL(pdh);

typedef struct _CRYPT_TIMESTAMP_PARA {
  LPCSTR             pszTSAPolicyId;
  BOOL               fRequestCerts;
  CRYPT_INTEGER_BLOB Nonce;
  DWORD              cExtension;
  PCERT_EXTENSION    rgExtension;
} CRYPT_TIMESTAMP_PARA, *PCRYPT_TIMESTAMP_PARA;

typedef struct _CRYPT_TIMESTAMP_ACCURACY {
  DWORD dwSeconds;
  DWORD dwMillis;
  DWORD dwMicros;
} CRYPT_TIMESTAMP_ACCURACY, *PCRYPT_TIMESTAMP_ACCURACY;

typedef struct _CRYPT_TIMESTAMP_INFO {
  DWORD                      dwVersion;
  LPSTR                      pszTSAPolicyId;
  CRYPT_ALGORITHM_IDENTIFIER HashAlgorithm;
  CRYPT_DER_BLOB             HashedMessage;
  CRYPT_INTEGER_BLOB         SerialNumber;
  FILETIME                   ftTime;
  PCRYPT_TIMESTAMP_ACCURACY  pvAccuracy;
  BOOL                       fOrdering;
  CRYPT_DER_BLOB             Nonce;
  CRYPT_DER_BLOB             Tsa;
  DWORD                      cExtension;
  PCERT_EXTENSION            rgExtension;
} CRYPT_TIMESTAMP_INFO, *PCRYPT_TIMESTAMP_INFO;

typedef struct _CRYPT_TIMESTAMP_CONTEXT {
  DWORD                 cbEncoded;
  BYTE                  *pbEncoded;
  PCRYPT_TIMESTAMP_INFO pTimeStamp;
} CRYPT_TIMESTAMP_CONTEXT, *PCRYPT_TIMESTAMP_CONTEXT;

//Just returning False for stub
BOOL 
CryptRetrieveTimeStamp(
  LPCWSTR                    wszUrl,
  DWORD                      dwRetrievalFlags,
  DWORD                      dwTimeout,
  LPCSTR                     pszHashId,
  const CRYPT_TIMESTAMP_PARA *pPara,
  const BYTE                 *pbData,
  DWORD                      cbData,
  PCRYPT_TIMESTAMP_CONTEXT   *ppTsContext,
  PCCERT_CONTEXT             *ppTsSigner,
  HCERTSTORE                 *phStore
)
{
	return FALSE;
}

BOOL 
CryptVerifyTimeStampSignature(
  const BYTE               *pbTSContentInfo,
  DWORD                    cbTSContentInfo,
  const BYTE               *pbData,
  DWORD                    cbData,
  HCERTSTORE               hAdditionalStore,
  PCRYPT_TIMESTAMP_CONTEXT *ppTsContext,
  PCCERT_CONTEXT           *ppTsSigner,
  HCERTSTORE               *phStore
)
{
	return FALSE;
}