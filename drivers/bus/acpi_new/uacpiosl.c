
#include <acpi.h>
#include <kernel_api.h>
#include <uacpi/uacpi.h>
#include <uacpi/event.h>


#include <initguid.h>
#include <ntddk.h>
#include <ntifs.h>
#include <mountdev.h>
#include <mountmgr.h>
#include <ketypes.h>
#include <iotypes.h>
#include <rtlfuncs.h>
#include <arc/arc.h>
//#define NDEBUG
#include <debug.h>

typedef struct _UACPI_ALLOCATION
{
   PHYSICAL_ADDRESS PhyAddress;
   PVOID            VirtAddress;
   SIZE_T           Size;
} UACPI_ALLOCATION, *PUACPI_ALLOCATION;

UINT32
ACPIInitUACPI()
{
    uacpi_status status = uacpi_initialize(0);
    if (uacpi_unlikely_error(status)) {
        DPRINT1("uacpi_initialize error: %s\n", uacpi_status_to_string(status));
    }
    DPRINT1("UACPI Initial Initialization Successful\n");
    // load the acpi namespace
    status = uacpi_namespace_load();
    if (uacpi_unlikely_error(status)) {
        DPRINT1("uacpi_namespace_load error: %s\n", uacpi_status_to_string(status));
    }

    // initialize the namespace
    status = uacpi_namespace_initialize();
    if (uacpi_unlikely_error(status)) {
        DPRINT1("uacpi_namespace_initialize error: %s\n", uacpi_status_to_string(status));
    }

    // initialize the namespace
    status = uacpi_finalize_gpe_initialization();
    if (uacpi_unlikely_error(status)) {
        DPRINT1("uACPI GPE initialization error: %s\n", uacpi_status_to_string(status));
    }

    return 0;
}

//TODO:
#ifndef UACPI_FORMATTED_LOGGING
void uacpi_kernel_log(uacpi_log_level Level, const uacpi_char* Char)
{
    DPRINT1("uACPI: %s", Char);
}
#else
UACPI_PRINTF_DECL(2, 3)
void uacpi_kernel_log(uacpi_log_level Level, const uacpi_char* Char, ...)
{
    DPRINT1("uACPI: %s", Char);
}
void uacpi_kernel_vlog(uacpi_log_level Level, const uacpi_char* Char, uacpi_va_list list)
{
    DPRINT1("uACPI: %s", Char);
}
#endif
// END OF TODO:

uacpi_u64
uacpi_kernel_get_nanoseconds_since_boot(void)
{
    /*  The number of milliseconds that have elapsed since the system was started. */
    LARGE_INTEGER PerfFrequency, PerformanceCounter;
    PerformanceCounter = KeQueryPerformanceCounter(&PerfFrequency);
    uacpi_u64 MiliSecSinceBoot = (uacpi_u64)(PerformanceCounter.QuadPart * 1000) / PerfFrequency.QuadPart;

    /* Now let's return nanoseconds. */
    return (MiliSecSinceBoot * 1000000);
}

void
uacpi_kernel_stall(uacpi_u8 usec)
{
    /* both are microseconds so this just applies. */
    KeStallExecutionProcessor(usec);
}

void uacpi_kernel_sleep(uacpi_u64 msec)
{
    KeStallExecutionProcessor(1000 * msec);
}

/*
 * (semaphore-like) event object.
 */
typedef struct _ACPI_SEM {
    KEVENT Event;
    KSPIN_LOCK Lock;
} ACPI_SEM, *PACPI_SEM;

uacpi_handle
uacpi_kernel_create_event(void)
{
    PACPI_SEM Sem;
    DPRINT("uacpi_kernel_create_event: enter\n");
    Sem = ExAllocatePoolWithTag(NonPagedPool, sizeof(ACPI_SEM), 'LpcA');
    ASSERT(Sem);

    KeInitializeEvent(&Sem->Event, SynchronizationEvent, FALSE);
    KeInitializeSpinLock(&Sem->Lock);

    return (uacpi_handle)Sem;
}

void
uacpi_kernel_free_event(uacpi_handle Handle)
{
    DPRINT1("uacpi_kernel_free_event: enter\n");
    if (Handle)
        ExFreePoolWithTag(Handle, 'LpcA');
}

/*
 * Try to wait for an event (counter > 0) with a millisecond timeout.
 * A timeout value of 0xFFFF implies infinite wait.
 *
 * The internal counter is decremented by 1 if wait was successful.
 *
 * A successful wait is indicated by returning UACPI_TRUE.
 */
uacpi_bool
uacpi_kernel_wait_for_event(uacpi_handle Handle, uacpi_u16 Timeout)
{
    PACPI_SEM Sem = Handle;
    KIRQL OldIrql;
    LARGE_INTEGER TimeoutNT;

    TimeoutNT.QuadPart = Timeout * 10000;
    KeWaitForSingleObject(&Sem->Event,
                              Executive,
                              KernelMode,
                              FALSE,
                              &TimeoutNT);
    KeAcquireSpinLock(&Sem->Lock, &OldIrql);
    KeSetEvent(&Sem->Event, IO_NO_INCREMENT, FALSE);

    KeReleaseSpinLock(&Sem->Lock, OldIrql);
    return TRUE;
}

/*
 * Signal the event object by incrementing its internal counter by 1.
 *
 * This function may be used in interrupt contexts.
 */
void uacpi_kernel_signal_event(uacpi_handle Handle)
{
    PACPI_SEM Sem = Handle;
    KIRQL OldIrql;


    KeAcquireSpinLock(&Sem->Lock, &OldIrql);
    KeSetEvent(&Sem->Event, IO_NO_INCREMENT, FALSE);

    KeReleaseSpinLock(&Sem->Lock, OldIrql);
}

/*
 * Reset the event counter to 0.
 */
void uacpi_kernel_reset_event(uacpi_handle Handle)
{

}

/*
 * spinlocks may be used in interrupt contexts.
 */
uacpi_handle
uacpi_kernel_create_spinlock(void)
{
    PKSPIN_LOCK SpinLock;

    SpinLock = ExAllocatePoolWithTag(NonPagedPool, sizeof(KSPIN_LOCK), 'LpcA');
    ASSERT(SpinLock);

    KeInitializeSpinLock(SpinLock);

    return (uacpi_handle)SpinLock;
}

void
uacpi_kernel_free_spinlock(uacpi_handle Handle)
{
    if (Handle)
        ExFreePoolWithTag(Handle, 'LpcA');
}

uacpi_cpu_flags
uacpi_kernel_lock_spinlock(uacpi_handle Handle)
{
    KIRQL OldIrql;

    if ((OldIrql = KeGetCurrentIrql()) >= DISPATCH_LEVEL)
    {
        KeAcquireSpinLockAtDpcLevel((PKSPIN_LOCK)Handle);
    }
    else
    {
        KeAcquireSpinLock((PKSPIN_LOCK)Handle, &OldIrql);
    }

    return (uacpi_cpu_flags)OldIrql;
}

void
uacpi_kernel_unlock_spinlock(uacpi_handle Handle, uacpi_cpu_flags Flags)
{
    KIRQL OldIrql = (KIRQL)Flags;

    if (OldIrql >= DISPATCH_LEVEL)
    {
        KeReleaseSpinLockFromDpcLevel((PKSPIN_LOCK)Handle);
    }
    else
    {
        KeReleaseSpinLock((PKSPIN_LOCK)Handle, OldIrql);
    }
}

void*
uacpi_kernel_alloc(uacpi_size size)
{
    void* outptr = ExAllocatePoolWithTag(NonPagedPool, size, 'ipcA');
    ASSERT(outptr);
    return outptr;
}

void *uacpi_kernel_calloc(uacpi_size count, uacpi_size size)
{
    return ExAllocatePoolWithTag(NonPagedPool, size * count, 'ipcA');
}

#ifndef UACPI_SIZED_FREES
void uacpi_kernel_free(void *mem)
{
    if (mem)
        ExFreePoolWithTag(mem, 'ipcA');
}
#else
void uacpi_kernel_free(void *mem, uacpi_size size_hint)
{
    if (mem)
        ExFreePoolWithTag(mem, 'ipcA');
}
#endif

uacpi_handle
uacpi_kernel_create_mutex(void)
{
    PFAST_MUTEX Mutex;

    DPRINT("uacpi_kernel_create_mutex: Enter\n");
    Mutex = ExAllocatePoolWithTag(NonPagedPool, sizeof(FAST_MUTEX), 'LpcA');
    ASSERT(Mutex);

    ExInitializeFastMutex(Mutex);

    return (uacpi_handle)Mutex;
}

void
uacpi_kernel_free_mutex(uacpi_handle handle)
{
    DPRINT("uacpi_kernel_free_mutex: enter\n");
    if (handle)
        ExFreePoolWithTag(handle, 'LpcA');
}

uacpi_status
uacpi_kernel_acquire_mutex(uacpi_handle Handle, uacpi_u16 Timeout)
{
    DPRINT("uacpi_kernel_acquire_mutex: Entry\n");
    
    /* Check what the caller wants us to do */
    if (Timeout == 0)
    {
        /* Try to acquire without waiting */
        if (!ExTryToAcquireFastMutex((PFAST_MUTEX)Handle))
            return UACPI_STATUS_TIMEOUT;
    }
    else
    {
        /* Block until we get it */
        ExAcquireFastMutexUnsafe((PFAST_MUTEX)Handle);
    }

    return UACPI_STATUS_OK;
}

void
uacpi_kernel_release_mutex(uacpi_handle Handle)
{
    DPRINT("uacpi_kernel_release_mutex: Enter\n");
    ExReleaseFastMutexUnsafe((PFAST_MUTEX)Handle);
}

uacpi_status uacpi_kernel_io_map(
    uacpi_io_addr base, uacpi_size len, uacpi_handle *out_handle
)
{
    PUACPI_ALLOCATION Allocation = ExAllocatePoolWithTag(NonPagedPool, sizeof(UACPI_ALLOCATION), 'LpcA');;
    PHYSICAL_ADDRESS Address;
    PVOID OutPtr;

    Address.QuadPart = (LONGLONG)base;
    OutPtr = MmMapIoSpace(Address, len, MmNonCached);
 
    Allocation->PhyAddress = Address;
    Allocation->Size = len;
    Allocation->VirtAddress = OutPtr;
    
    DPRINT("uacpi_kernel_io_map(phys 0x%p  size 0x%X)\n", Address.QuadPart, len);
    *out_handle = (uacpi_handle)Allocation;
    return UACPI_STATUS_OK;
}

void uacpi_kernel_io_unmap(uacpi_handle handle)
{
    PUACPI_ALLOCATION Allocation = (PUACPI_ALLOCATION)handle;
    DPRINT("Entry: uacpi_kernel_io_unmap\n");
    MmUnmapIoSpace(Allocation->VirtAddress, Allocation->Size);
    ExFreePoolWithTag(handle, 'LpcA');    
}

uacpi_status uacpi_kernel_io_read(
    uacpi_handle handle, uacpi_size offset,
    uacpi_u8 byte_width, uacpi_u64 *value)
{
    PUACPI_ALLOCATION Allocation = (PUACPI_ALLOCATION)handle;
    ULONG_PTR Address = (ULONG_PTR)Allocation->VirtAddress;
    Address += offset;
    DPRINT("uacpi_kernel_io_read %p, width %d\n",Address,byte_width);
    switch (byte_width)
    {
    case 1:
        *value = READ_PORT_UCHAR((PUCHAR)(ULONG_PTR)Address);
        break;

    case 2:
        *value = READ_PORT_USHORT((PUSHORT)(ULONG_PTR)Address);
        break;

    case 4:
        *value = READ_PORT_ULONG((PULONG)(ULONG_PTR)Address);
        break;

    default:
        DPRINT1("uacpi_kernel_io_read got bad width: %d\n",byte_width);
        break;
    }

    return UACPI_STATUS_OK;
}

uacpi_status uacpi_kernel_io_write(
    uacpi_handle handle, uacpi_size offset,
    uacpi_u8 byte_width, uacpi_u64 value
)
{
    PUACPI_ALLOCATION Allocation = (PUACPI_ALLOCATION)handle;
    ULONG_PTR Address = (ULONG_PTR)Allocation->VirtAddress;
    Address += offset;
    DPRINT("uacpi_kernel_io_write %p, width %d\n",Address,byte_width);
    switch (byte_width)
    {
    case 1:
        WRITE_PORT_UCHAR((PUCHAR)(ULONG_PTR)Address, value);
        break;

    case 2:
        WRITE_PORT_USHORT((PUSHORT)(ULONG_PTR)Address, value);
        break;

    case 4:
        WRITE_PORT_ULONG((PULONG)(ULONG_PTR)Address, value);
        break;

    default:
        DPRINT1("uacpi_kernel_io_write got bad width: %d\n",byte_width);
 
        break;
    }
    return UACPI_STATUS_OK;
}

uacpi_status
uacpi_kernel_handle_firmware_request(uacpi_firmware_request* Req)
{
    if (Req == 0)
    {
        DPRINT1("uacpi_kernel_handle_firmware_request: BreakPoint!\n");
        ASSERT(FALSE);
    }
    else
    {
        DPRINT1("uacpi_kernel_handle_firmware_request: Fatal!\n");
        ASSERT(FALSE);
    }
    return UACPI_STATUS_OK;
}

uacpi_thread_id
uacpi_kernel_get_thread_id(void)
{
    /* Thread ID must be non-zero */
    ULONG_PTR ThreadID = (ULONG_PTR)PsGetCurrentThreadId() + 1;
    return (VOID*)ThreadID;
}

//TODO: ----------------------------------

void *
uacpi_kernel_map(uacpi_phys_addr addr, uacpi_size len)
{
    PHYSICAL_ADDRESS Address;
    PVOID Ptr;

    DPRINT("uacpi_kernel_map(phys 0x%p  size 0x%X)\n", addr, len);
    ASSERT(len);
    Address.QuadPart = (ULONG)addr;
    Ptr = MmMapIoSpace(Address, len, MmNonCached);
    if (!Ptr)
    {
        DPRINT1("Mapping failed\n");
    }

    return Ptr;
}

void
uacpi_kernel_unmap(void *addr, uacpi_size len)
{
    if (addr)
       MmUnmapIoSpace(addr, len);
}

uacpi_status
uacpi_kernel_get_rsdp(uacpi_phys_addr *out_rdsp_address)
{
    UNIMPLEMENTED;
    __debugbreak();
    return UACPI_STATUS_OK;
}

static PKINTERRUPT AcpiInterrupt;
static BOOLEAN AcpiInterruptHandlerRegistered = FALSE;
static uacpi_interrupt_handler AcpiIrqHandler = NULL;
static PVOID AcpiIrqContext = NULL;
static ULONG AcpiIrqNumber = 0;

BOOLEAN NTAPI
OslIsrStub(
  PKINTERRUPT Interrupt,
  PVOID ServiceContext)
{
  INT32 Status;

  Status = (*AcpiIrqHandler)(AcpiIrqContext);

  if (Status == UACPI_INTERRUPT_HANDLED)
    return TRUE;
  else
    return FALSE;
}

uacpi_status uacpi_kernel_install_interrupt_handler(
    uacpi_u32 irq, uacpi_interrupt_handler handler, uacpi_handle ctx,
    uacpi_handle *out_irq_handle)
{
    ULONG Vector;
    KIRQL DIrql;
    KAFFINITY Affinity;
    NTSTATUS Status;

    if (AcpiInterruptHandlerRegistered)
    {
        DPRINT1("Reregister interrupt attempt failed\n");
        return 1;
    }

    DPRINT("AcpiOsInstallInterruptHandler()\n");
    Vector = HalGetInterruptVector(
        Internal,
        0,
        irq,
        irq,
        &DIrql,
        &Affinity);

    AcpiIrqNumber = irq;
    AcpiIrqHandler = handler;
    AcpiIrqContext = ctx;
    AcpiInterruptHandlerRegistered = TRUE;

    Status = IoConnectInterrupt(
        &AcpiInterrupt,
        OslIsrStub,
        NULL,
        NULL,
        Vector,
        DIrql,
        DIrql,
        LevelSensitive,
        TRUE,
        Affinity,
        FALSE);

    if (!NT_SUCCESS(Status))
    {
        DPRINT("Could not connect to interrupt %d\n", Vector);
        return 1;
    }
    return UACPI_STATUS_OK;
}

uacpi_status uacpi_kernel_uninstall_interrupt_handler(
    uacpi_interrupt_handler handler, uacpi_handle irq_handle
)
{
    DPRINT("AcpiOsRemoveInterruptHandler()\n");

    if (AcpiInterruptHandlerRegistered)
    {
        IoDisconnectInterrupt(AcpiInterrupt);
        AcpiInterrupt = NULL;
        AcpiInterruptHandlerRegistered = FALSE;
    }
    else
    {
        DPRINT1("Trying to remove non-existing interrupt handler\n");
        return 1;
    }
    return UACPI_STATUS_OK;
}

uacpi_status uacpi_kernel_schedule_work(
    uacpi_work_type type, uacpi_work_handler Handler, uacpi_handle ctx)
{
    HANDLE ThreadHandle;
    OBJECT_ATTRIBUTES ObjectAttributes;
    NTSTATUS Status;

    DPRINT("uacpi_kernel_schedule_work: Entry\n");

    InitializeObjectAttributes(&ObjectAttributes,
                               NULL,
                               OBJ_KERNEL_HANDLE,
                               NULL,
                               NULL);

    Status = PsCreateSystemThread(&ThreadHandle,
                                  THREAD_ALL_ACCESS,
                                  &ObjectAttributes,
                                  NULL,
                                  NULL,
                                  (PKSTART_ROUTINE)Handler,
                                  ctx);
    if (!NT_SUCCESS(Status))
        return 1;

    ZwClose(ThreadHandle);
    return UACPI_STATUS_OK;

}

uacpi_status uacpi_kernel_wait_for_work_completion(void)
{
    DPRINT("uacpi_kernel_wait_for_work_completion: Enter\n");
    return UACPI_STATUS_OK;
}

//TODO: this one isn't going to simple.
uacpi_status 
uacpi_kernel_pci_device_open(
    uacpi_pci_address address, uacpi_handle *out_handle
)
{
     DPRINT("uacpi_kernel_pci_device_open: Entry\n");
     return 0;
}

void
uacpi_kernel_pci_device_close(uacpi_handle Handle)
{
    DPRINT("uacpi_kernel_pci_device_close: Entry\n");
}

uacpi_status
uacpi_kernel_pci_read(
    uacpi_handle device, uacpi_size offset,
    uacpi_u8 byte_width, uacpi_u64 *value
)
{
    DPRINT("uacpi_kernel_pci_read: Entry\n");
    return 0;
}
uacpi_status
uacpi_kernel_pci_write(
    uacpi_handle device, uacpi_size offset,
    uacpi_u8 byte_width, uacpi_u64 value
)
{
    DPRINT("uacpi_kernel_pci_write: Entry\n");
    return 0;
}

