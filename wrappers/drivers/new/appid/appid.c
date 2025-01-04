/*
 * PROJECT:         ReactOS WMI driver
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * FILE:            wrappers/drivers/bootanim/bootanim.c
 * PURPOSE:         Windows Management Intstrumentation
 * PROGRAMMERS:     Aleksey Bragin (aleksey@reactos.org)
 *                  
 */

/* INCLUDES *****************************************************************/

#include <stdio.h>
#include <ntddk.h>
#include <wmilib.h>

#define NDEBUG
#include <debug.h>
#include <ldrfuncs.h>
#include <wdm.h>

NTSTATUS
NTAPI
DriverEntry(
	IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
)
{
    PDEVICE_OBJECT DeviceObject;
    UNICODE_STRING DeviceName;
    NTSTATUS Status;
	
    DriverObject->Flags = 0;	
	
    /* Return success */
    
    RtlInitUnicodeString(&DeviceName,
                         L"\\Device\\SrpDevice");
    Status = IoCreateDevice(DriverObject,
                            0,
                            &DeviceName,
                            FILE_DEVICE_UNKNOWN,
                            0,
                            FALSE,
                            &DeviceObject);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }
	

    /* initialize the device object */
    DeviceObject->Flags |= DO_DIRECT_IO;	

	return STATUS_SUCCESS;
}