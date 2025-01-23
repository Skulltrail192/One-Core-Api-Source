#pragma once

#ifdef UACPI_OVERRIDE_CONFIG
#include "uacpi_config.h"
#else

#include <uacpi/helpers.h>
#include <uacpi/types.h>

/*
 * =======================
 * Context-related options
 * =======================
 */
#ifndef UACPI_DEFAULT_LOG_LEVEL
    #define UACPI_DEFAULT_LOG_LEVEL UACPI_LOG_INFO
#endif

UACPI_BUILD_BUG_ON_WITH_MSG(
    UACPI_DEFAULT_LOG_LEVEL < UACPI_LOG_ERROR ||
    UACPI_DEFAULT_LOG_LEVEL > UACPI_LOG_DEBUG,
    "configured default log level is invalid"
);

#ifndef UACPI_DEFAULT_LOOP_TIMEOUT_SECONDS
    #define UACPI_DEFAULT_LOOP_TIMEOUT_SECONDS 30
#endif

UACPI_BUILD_BUG_ON_WITH_MSG(
    UACPI_DEFAULT_LOOP_TIMEOUT_SECONDS < 1,
    "configured default loop timeout is invalid (expecting at least 1 second)"
);

#ifndef UACPI_DEFAULT_MAX_CALL_STACK_DEPTH
    #define UACPI_DEFAULT_MAX_CALL_STACK_DEPTH 256
#endif

UACPI_BUILD_BUG_ON_WITH_MSG(
    UACPI_DEFAULT_MAX_CALL_STACK_DEPTH < 4,
    "configured default max call stack depth is invalid "
    "(expecting at least 4 frames)"
);

/*
 * ===================
 * Kernel-api options
 * ===================
 */

/*
 * Convenience initialization/deinitialization hooks that will be called by
 * uACPI automatically when appropriate if compiled-in.
 */
// #define UACPI_KERNEL_INITIALIZATION

/*
 * Makes kernel api logging callbacks work with unformatted printf-style
 * strings and va_args instead of a pre-formatted string. Can be useful if
 * your native logging is implemented in terms of this format as well.
 */
// #define UACPI_FORMATTED_LOGGING

/*
 * Makes uacpi_kernel_free take in an additional 'size_hint' parameter, which
 * contains the size of the original allocation. Note that this comes with a
 * performance penalty in some cases.
 */
// #define UACPI_SIZED_FREES


/*
 * Makes uacpi_kernel_alloc_zeroed mandatory to implement by the host, uACPI
 * will not provide a default implementation if this is enabled.
 */
// #define UACPI_NATIVE_ALLOC_ZEROED

/*
 * =========================
 * Platform-specific options
 * =========================
 */

/*
 * Turns uacpi_phys_addr and uacpi_io_addr into a 32-bit type, and adds extra
 * code for address truncation. Needed for e.g. i686 platforms without PAE
 * support.
 */
// #define UACPI_PHYS_ADDR_IS_32BITS

/*
 * Switches uACPI into reduced-hardware-only mode. Strips all full-hardware
 * ACPI support code at compile-time, including the event subsystem, the global
 * lock, and other full-hardware features.
 */
// #define UACPI_REDUCED_HARDWARE

/*
 * =============
 * Misc. options
 * =============
 */

/*
 * If UACPI_FORMATTED_LOGGING is not enabled, this is the maximum length of the
 * pre-formatted message that is passed to the logging callback.
 */
#ifndef UACPI_PLAIN_LOG_BUFFER_SIZE
    #define UACPI_PLAIN_LOG_BUFFER_SIZE 128
#endif

UACPI_BUILD_BUG_ON_WITH_MSG(
    UACPI_PLAIN_LOG_BUFFER_SIZE < 16,
    "configured log buffer size is too small (expecting at least 16 bytes)"
);

/*
 * The size of the table descriptor inline storage. All table descriptors past
 * this length will be stored in a dynamically allocated heap array. The size
 * of one table descriptor is approximately 56 bytes.
 */
#ifndef UACPI_STATIC_TABLE_ARRAY_LEN
    #define UACPI_STATIC_TABLE_ARRAY_LEN 16
#endif

UACPI_BUILD_BUG_ON_WITH_MSG(
    UACPI_STATIC_TABLE_ARRAY_LEN < 1,
    "configured static table array length is too small (expecting at least 1)"
);

#endif
