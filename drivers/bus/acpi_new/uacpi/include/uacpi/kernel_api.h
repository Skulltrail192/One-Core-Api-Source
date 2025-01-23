#pragma once

#include <uacpi/types.h>
#include <uacpi/platform/arch_helpers.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Convenience initialization/deinitialization hooks that will be called by
 * uACPI automatically when appropriate if compiled-in.
 */
#ifdef UACPI_KERNEL_INITIALIZATION
/*
 * This API is invoked for each initialization level so that appropriate parts
 * of the host kernel and/or glue code can be initialized at different stages.
 *
 * uACPI API that triggers calls to uacpi_kernel_initialize and the respective
 * 'current_init_lvl' passed to the hook at that stage:
 * 1. uacpi_initialize() -> UACPI_INIT_LEVEL_EARLY
 * 2. uacpi_namespace_load() -> UACPI_INIT_LEVEL_SUBSYSTEM_INITIALIZED
 * 3. (start of) uacpi_namespace_initialize() -> UACPI_INIT_LEVEL_NAMESPACE_LOADED
 * 4. (end of) uacpi_namespace_initialize() -> UACPI_INIT_LEVEL_NAMESPACE_INITIALIZED
 */
uacpi_status uacpi_kernel_initialize(uacpi_init_level current_init_lvl);
void uacpi_kernel_deinitialize(void);
#endif

// Returns the PHYSICAL address of the RSDP structure via *out_rsdp_address.
uacpi_status uacpi_kernel_get_rsdp(uacpi_phys_addr *out_rsdp_address);

/*
 * Open a PCI device at 'address' for reading & writing.
 *
 * The handle returned via 'out_handle' is used to perform IO on the
 * configuration space of the device.
 */
uacpi_status uacpi_kernel_pci_device_open(
    uacpi_pci_address address, uacpi_handle *out_handle
);
void uacpi_kernel_pci_device_close(uacpi_handle);

/*
 * Read & write the configuration space of a previously open PCI device.
 *
 * NOTE:
 * 'byte_width' is ALWAYS one of 1, 2, 4. Since PCI registers are 32 bits wide
 * this must be able to handle e.g. a 1-byte access by reading at the nearest
 * 4-byte aligned offset below, then masking the value to select the target
 * byte.
 */
uacpi_status uacpi_kernel_pci_read(
    uacpi_handle device, uacpi_size offset,
    uacpi_u8 byte_width, uacpi_u64 *value
);
uacpi_status uacpi_kernel_pci_write(
    uacpi_handle device, uacpi_size offset,
    uacpi_u8 byte_width, uacpi_u64 value
);

/*
 * Map a SystemIO address at [base, base + len) and return a kernel-implemented
 * handle that can be used for reading and writing the IO range.
 */
uacpi_status uacpi_kernel_io_map(
    uacpi_io_addr base, uacpi_size len, uacpi_handle *out_handle
);
void uacpi_kernel_io_unmap(uacpi_handle handle);

/*
 * Read/Write the IO range mapped via uacpi_kernel_io_map
 * at a 0-based 'offset' within the range.
 *
 * NOTE:
 * 'byte_width' is ALWAYS one of 1, 2, 4. You are NOT allowed to break e.g. a
 * 4-byte access into four 1-byte accesses. Hardware ALWAYS expects accesses to
 * be of the exact width.
 */
uacpi_status uacpi_kernel_io_read(
    uacpi_handle, uacpi_size offset,
    uacpi_u8 byte_width, uacpi_u64 *value
);
uacpi_status uacpi_kernel_io_write(
    uacpi_handle, uacpi_size offset,
    uacpi_u8 byte_width, uacpi_u64 value
);

void *uacpi_kernel_map(uacpi_phys_addr addr, uacpi_size len);
void uacpi_kernel_unmap(void *addr, uacpi_size len);

/*
 * Allocate a block of memory of 'size' bytes.
 * The contents of the allocated memory are unspecified.
 */
void *uacpi_kernel_alloc(uacpi_size size);

#ifdef UACPI_NATIVE_ALLOC_ZEROED
/*
 * Allocate a block of memory of 'size' bytes.
 * The returned memory block is expected to be zero-filled.
 */
void *uacpi_kernel_alloc_zeroed(uacpi_size size);
#endif

/*
 * Free a previously allocated memory block.
 *
 * 'mem' might be a NULL pointer. In this case, the call is assumed to be a
 * no-op.
 *
 * An optionally enabled 'size_hint' parameter contains the size of the original
 * allocation. Note that in some scenarios this incurs additional cost to
 * calculate the object size.
 */
#ifndef UACPI_SIZED_FREES
void uacpi_kernel_free(void *mem);
#else
void uacpi_kernel_free(void *mem, uacpi_size size_hint);
#endif

#ifndef UACPI_FORMATTED_LOGGING
void uacpi_kernel_log(uacpi_log_level, const uacpi_char*);
#else
UACPI_PRINTF_DECL(2, 3)
void uacpi_kernel_log(uacpi_log_level, const uacpi_char*, ...);
void uacpi_kernel_vlog(uacpi_log_level, const uacpi_char*, uacpi_va_list);
#endif

/*
 * Returns the number of nanosecond ticks elapsed since boot,
 * strictly monotonic.
 */
uacpi_u64 uacpi_kernel_get_nanoseconds_since_boot(void);

/*
 * Spin for N microseconds.
 */
void uacpi_kernel_stall(uacpi_u8 usec);

/*
 * Sleep for N milliseconds.
 */
void uacpi_kernel_sleep(uacpi_u64 msec);

/*
 * Create/free an opaque non-recursive kernel mutex object.
 */
uacpi_handle uacpi_kernel_create_mutex(void);
void uacpi_kernel_free_mutex(uacpi_handle);

/*
 * Create/free an opaque kernel (semaphore-like) event object.
 */
uacpi_handle uacpi_kernel_create_event(void);
void uacpi_kernel_free_event(uacpi_handle);

/*
 * Returns a unique identifier of the currently executing thread.
 *
 * The returned thread id cannot be UACPI_THREAD_ID_NONE.
 */
uacpi_thread_id uacpi_kernel_get_thread_id(void);

/*
 * Try to acquire the mutex with a millisecond timeout.
 *
 * The timeout value has the following meanings:
 * 0x0000 - Attempt to acquire the mutex once, in a non-blocking manner
 * 0x0001...0xFFFE - Attempt to acquire the mutex for at least 'timeout'
 *                   milliseconds
 * 0xFFFF - Infinite wait, block until the mutex is acquired
 *
 * The following are possible return values:
 * 1. UACPI_STATUS_OK - successful acquire operation
 * 2. UACPI_STATUS_TIMEOUT - timeout reached while attempting to acquire (or the
 *                           single attempt to acquire was not successful for
 *                           calls with timeout=0)
 * 3. Any other value - signifies a host internal error and is treated as such
 */
uacpi_status uacpi_kernel_acquire_mutex(uacpi_handle, uacpi_u16);
void uacpi_kernel_release_mutex(uacpi_handle);

/*
 * Try to wait for an event (counter > 0) with a millisecond timeout.
 * A timeout value of 0xFFFF implies infinite wait.
 *
 * The internal counter is decremented by 1 if wait was successful.
 *
 * A successful wait is indicated by returning UACPI_TRUE.
 */
uacpi_bool uacpi_kernel_wait_for_event(uacpi_handle, uacpi_u16);

/*
 * Signal the event object by incrementing its internal counter by 1.
 *
 * This function may be used in interrupt contexts.
 */
void uacpi_kernel_signal_event(uacpi_handle);

/*
 * Reset the event counter to 0.
 */
void uacpi_kernel_reset_event(uacpi_handle);

/*
 * Handle a firmware request.
 *
 * Currently either a Breakpoint or Fatal operators.
 */
uacpi_status uacpi_kernel_handle_firmware_request(uacpi_firmware_request*);

/*
 * Install an interrupt handler at 'irq', 'ctx' is passed to the provided
 * handler for every invocation.
 *
 * 'out_irq_handle' is set to a kernel-implemented value that can be used to
 * refer to this handler from other API.
 */
uacpi_status uacpi_kernel_install_interrupt_handler(
    uacpi_u32 irq, uacpi_interrupt_handler, uacpi_handle ctx,
    uacpi_handle *out_irq_handle
);

/*
 * Uninstall an interrupt handler. 'irq_handle' is the value returned via
 * 'out_irq_handle' during installation.
 */
uacpi_status uacpi_kernel_uninstall_interrupt_handler(
    uacpi_interrupt_handler, uacpi_handle irq_handle
);

/*
 * Create/free a kernel spinlock object.
 *
 * Unlike other types of locks, spinlocks may be used in interrupt contexts.
 */
uacpi_handle uacpi_kernel_create_spinlock(void);
void uacpi_kernel_free_spinlock(uacpi_handle);

/*
 * Lock/unlock helpers for spinlocks.
 *
 * These are expected to disable interrupts, returning the previous state of cpu
 * flags, that can be used to possibly re-enable interrupts if they were enabled
 * before.
 *
 * Note that lock is infalliable.
 */
uacpi_cpu_flags uacpi_kernel_lock_spinlock(uacpi_handle);
void uacpi_kernel_unlock_spinlock(uacpi_handle, uacpi_cpu_flags);

typedef enum uacpi_work_type {
    /*
     * Schedule a GPE handler method for execution.
     * This should be scheduled to run on CPU0 to avoid potential SMI-related
     * firmware bugs.
     */
    UACPI_WORK_GPE_EXECUTION,

    /*
     * Schedule a Notify(device) firmware request for execution.
     * This can run on any CPU.
     */
    UACPI_WORK_NOTIFICATION,
} uacpi_work_type;

typedef void (*uacpi_work_handler)(uacpi_handle);

/*
 * Schedules deferred work for execution.
 * Might be invoked from an interrupt context.
 */
uacpi_status uacpi_kernel_schedule_work(
    uacpi_work_type, uacpi_work_handler, uacpi_handle ctx
);

/*
 * Waits for two types of work to finish:
 * 1. All in-flight interrupts installed via uacpi_kernel_install_interrupt_handler
 * 2. All work scheduled via uacpi_kernel_schedule_work
 *
 * Note that the waits must be done in this order specifically.
 */
uacpi_status uacpi_kernel_wait_for_work_completion(void);

#ifdef __cplusplus
}
#endif
