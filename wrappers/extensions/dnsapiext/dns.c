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

WINE_DEFAULT_DEBUG_CHANNEL(dnsapiext);

//Code from VxKex //Thanks VxKex team
DNS_STATUS WINAPI DnsQueryEx(
	IN		PDNS_QUERY_REQUEST	Request,
	IN OUT	PDNS_QUERY_RESULT	Result,
	IN OUT	PDNS_QUERY_CANCEL	Cancel)
{
	DNS_STATUS DnsStatus;

	// ASSERT (Request != NULL);
	// ASSERT (Result != NULL);
	// ASSERT (Cancel == NULL);
	// ASSERT (Request->Version == 1);
	// ASSERT (Request->QueryCompletionCallback == NULL);
	// ASSERT (Result->Version == 1);

	//
	// Validate parameters.
	//

	if (!Request || !Result) {
		return ERROR_INVALID_PARAMETER;
	}

	if (Request->Version != 1 || Result->Version != 1) {
		// Version could be 3, which is only available in Win11 and above.
		return ERROR_INVALID_PARAMETER;
	}

	if (Request->pQueryCompletionCallback) {
		return DNS_RCODE_NOT_IMPLEMENTED;
	}

	if (Cancel) {
		return DNS_RCODE_NOT_IMPLEMENTED;
	}

	//
	// Perform the query.
	// Note: the current implementation ignores DnsServerList and InterfaceIndex.
	//

	DnsStatus = DnsQuery_W(
		Request->QueryName,
		Request->QueryType,
		(ULONG) Request->QueryOptions,
		NULL,
		&Result->pQueryRecords,
		NULL);

	//
	// Fill out remaining fields of DNS_QUERY_RESULT.
	//

	Result->QueryStatus = DnsStatus;
	Result->QueryOptions = Request->QueryOptions;
	Result->Reserved = NULL;

	return DnsStatus;
}