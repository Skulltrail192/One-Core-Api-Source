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
#include <ntddndis.h>
#include <ndis.h>
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

#define NET_BUFFER_DATA_LENGTH(_NB) ((_NB)->DataLength)

typedef struct _NET_BUFFER_DATA {
  struct NET_BUFFER             *Next;
  MDL                    *CurrentMdl;
  ULONG                  CurrentMdlOffset;
  /*NET_BUFFER_DATA_LENGTH*/PVOID NbDataLength;
  MDL                    *MdlChain;
  ULONG                  DataOffset;
} NET_BUFFER_DATA, *PNET_BUFFER_DATA;

typedef union _NET_BUFFER_HEADER {
  NET_BUFFER_DATA NetBufferData;
  SLIST_HEADER    Link;
} NET_BUFFER_HEADER, *PNET_BUFFER_HEADER;

//
// NET_BUFFER_SHARED_MEMORY is used to describe the
// shared memory segments used in each NET_BUFFER.
// for NDIS 6.20, they are used in VM queue capable NICs
// used in virtualization environment
//
typedef struct _NET_BUFFER_SHARED_MEMORY
{
    struct NET_BUFFER_SHARED_MEMORY   *NextSharedMemorySegment;
    ULONG                       SharedMemoryFlags;
    NDIS_HANDLE                 SharedMemoryHandle;
    ULONG                       SharedMemoryOffset;
    ULONG                       SharedMemoryLength;
} NET_BUFFER_SHARED_MEMORY, *PNET_BUFFER_SHARED_MEMORY;

typedef struct _NET_BUFFER {
  union {
    struct {
      struct _NET_BUFFER *Next;
      MDL        *CurrentMdl;
      ULONG      CurrentMdlOffset;
      union {
        ULONG  DataLength;
        SIZE_T stDataLength;
      };
      MDL        *MdlChain;
      ULONG      DataOffset;
    };
    SLIST_HEADER      Link;
    NET_BUFFER_HEADER NetBufferHeader;
  };
  USHORT           ChecksumBias;
  USHORT           Reserved;
  NDIS_HANDLE      NdisPoolHandle;
  PVOID            NdisReserved[2];
  PVOID            ProtocolReserved[6];
  PVOID            MiniportReserved[4];
  PHYSICAL_ADDRESS DataPhysicalAddress;
  union {
    NET_BUFFER_SHARED_MEMORY *SharedMemoryInfo;
    SCATTER_GATHER_LIST      *ScatterGatherList;
  };
} NET_BUFFER, *PNET_BUFFER;

NDIS_STATUS 
NTAPI 
NdisCopyFromNetBufferToNetBuffer(
    PNET_BUFFER Destination,
    ULONG DestinationOffset,
    ULONG BytesToCopy,
    PNET_BUFFER Source,
    ULONG SourceOffset,
    PULONG BytesCopied)
{
    // Obtenha os MDLs (Memory Descriptor Lists) das buffers de origem e destino
    struct _MDL *sourceMdl = Source->CurrentMdl;
    struct _MDL *destinationMdl = Destination->CurrentMdl;

    // Calcule os deslocamentos reais dentro dos MDLs
    void *sourceOffsetPtr = (void *)(SourceOffset + Source->CurrentMdlOffset);
    size_t destinationOffset = DestinationOffset + Destination->CurrentMdlOffset;

    // Inicialize o valor de retorno
    NDIS_STATUS status;

    // Chame a função RtlCopyMdlToMdl para copiar os dados
    status = RtlCopyMdlToMdl(
        sourceMdl,             // MDL da origem
        sourceOffsetPtr,       // Ponteiro para o deslocamento na origem
        destinationMdl,        // MDL do destino
        destinationOffset,     // Deslocamento no destino
        BytesToCopy,           // Quantidade de bytes a copiar
        (int)BytesCopied       // Ponteiro para o número de bytes copiados
    );

    // Atualize o número de bytes copiados no buffer fornecido
    *BytesCopied = (ULONG)*BytesCopied;

    return status;
}